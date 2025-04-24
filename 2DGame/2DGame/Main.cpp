#include "pch.h"
#pragma comment(lib, "msimg32.lib")
using namespace std::chrono;

// =========================================
// Enum
enum class DIR_TYPE
{
    LEFT,
    RIGHT,

    END
};

enum class KEY
{
    W,
    A,
    S,
    D,

    Q,
    E,
    T,
    Z,
    X,
    C,
    V,

    UP,
    DOWN,
    LEFT,
    RIGHT,

    SPACE,
    ENTER,
    CTRL,
    LSHIFT,
    RSHIFT,
    ESC,

    LBUTTON,
    RBUTTON,

    END,
};

enum class KEY_STATE
{
    NONE,
    TAP,
    HOLD,
    AWAY,
};

enum class OBJECT_STATE
{
    IDLE,
    WALK,
    ATTACK,
    ESKILL,
    DAMAGE,
    DEAD,
    END
};

// =========================================
// 구조체
struct vPoint
{
    float x;
    float y;
};

struct Texture
{
    HDC hDC;
    HBITMAP hBit;
    BITMAP bitInfo;
};

struct Player
{
    vPoint pos;
    vPoint scale;
    float speed;

    DIR_TYPE currDir;
    OBJECT_STATE currState;

    bool attacking;
    bool dead;
};

struct Monster
{
    vPoint pos;
    vPoint scale;
    float speed;

    DIR_TYPE currDir;
    OBJECT_STATE currState;

    bool attacking;
    bool dead;
};

struct Arrow
{
    vPoint pos;
    vPoint scale;
    float speed;

    vPoint dirPos;
    DIR_TYPE curDir;

    float elapsedTime;

    bool active;
};

struct KeyInfo
{
    KEY_STATE eState;
    bool bPrevPush;
};

struct Animation
{
    std::vector<Texture*> texture;
    int32_t currentFrame;
    int32_t frameCount;
    float frameIntervalTime;
    float elapsedTime;
    bool loop;
    bool dead;
};

// ===============================================================
// 함수
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static Texture* LoadTexture(const std::string& name);
static void DrawTexture(const Texture& texture, const vPoint pos, const vPoint scale);

static void LoadAnimation(Animation* animation, const std::string& name, int32_t maxFrame, float time, bool bLoop);
static void InitializeAnimation(Animation* animation, bool dead);
static void DrawAnimation(const Animation& animation, const vPoint pos, const vPoint scale);
static void UpdateAnimation(Animation* animation);

static void Initialize();
static void Update();
static void Draw();
static void Finalize();

// ==========================================================
// 전역 변수
constexpr int32_t MAX_LOADSTRING = 100;
constexpr int32_t VIEWSIZE_X = 1280;
constexpr int32_t VIEWSIZE_Y = 720;
constexpr int32_t FPS = 60;
constexpr int32_t MONSTER_COUNT = 10;

static HINSTANCE gHInst;
static WCHAR gSzTitle[MAX_LOADSTRING];
static WCHAR gSzWindowClass[MAX_LOADSTRING];

static HWND gHWnd;
static HDC gHDC;
static HDC gMemDC;

static std::unordered_map<std::string, Texture*> gMapTextures;

static Player gPlayerInformation;
static Animation gPlayerAnimations[(int32_t)OBJECT_STATE::END][(int32_t)DIR_TYPE::END];

static std::vector<Arrow> gArrowsInformation;
static Texture* gArrowTextures[(int32_t)DIR_TYPE::END];

static bool arrowFired;
static std::array<Monster, MONSTER_COUNT>gMonstersInformation;
static Animation gMonsterAnimations[(int32_t)OBJECT_STATE::END][(int32_t)DIR_TYPE::END];

static std::vector<KeyInfo> gVecKeyInfo;

static float gDeltaTime;


constexpr int32_t KeyMaps[(int32_t)KEY::END]
{
    'W',
    'A',
    'S',
    'D',

    'Q',
    'E',
    'T',
    'Z',
    'X',
    'C',
    'V',

    VK_UP,
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,

    VK_SPACE,
    VK_RETURN,
    VK_CONTROL,
    VK_LSHIFT,
    VK_RSHIFT,
    VK_ESCAPE,

    VK_LBUTTON,
    VK_RBUTTON,
};
// ==========================================================


// 윈메인
int WINAPI _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, gSzTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY2DGAME, gSzWindowClass, MAX_LOADSTRING);

    {   // 콘솔창 생성
        AllocConsole();

        FILE* fp{};
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::ios::sync_with_stdio();
    }

    // 윈도우 설정
    WNDCLASSEXW wcex =
    {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY2DGAME)),
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 2),
        .lpszMenuName = NULL,
        .lpszClassName = gSzWindowClass,
        .hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL))
    };

    RegisterClassExW(&wcex);


    // 화면이 가운데에 생성되도록 조정
    RECT rSize =
    {
        .left = 0,
        .top = 0,
        .right = VIEWSIZE_X,
        .bottom = VIEWSIZE_Y
    };

    AdjustWindowRect(&rSize, WS_OVERLAPPEDWINDOW, false);

    int windowX = rSize.right - rSize.left;
    int windowY = rSize.bottom - rSize.top;

    int sceenX = GetSystemMetrics(SM_CXSCREEN);
    int sceenY = GetSystemMetrics(SM_CYSCREEN);

    int centerX = (sceenX - windowX) / 2;
    int centerY = (sceenY - windowY) / 2;

    gHWnd = CreateWindow(gSzWindowClass, gSzTitle, WS_OVERLAPPEDWINDOW, centerX, centerY, windowX, windowY, nullptr, nullptr, hInstance, nullptr);
    assert(gHWnd != nullptr);

    ShowWindow(gHWnd, nCmdShow);
    UpdateWindow(gHWnd);

    // 더블 버퍼링 생성
    gHDC = GetDC(gHWnd);
    gMemDC = CreateCompatibleDC(gHDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(gHDC, VIEWSIZE_X, VIEWSIZE_Y);
    HBITMAP hOldBit = (HBITMAP)SelectObject(gMemDC, hBitmap);

    // 초기화
    Initialize();

    // 메시지 문
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY2DGAME));
    MSG msg{};

    while (WM_QUIT != msg.message)
    {
        auto startTime = high_resolution_clock::now();

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
                break;

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // 업데이트
        Update(); 

        // 그리기
        Draw();

        // 픽셀을 윈도우로 옮김
        BitBlt(gHDC, 0, 0, VIEWSIZE_X, VIEWSIZE_Y, gMemDC, 0, 0, SRCCOPY);

        // 프레임 그리기
        static float timeAcc = 0.0f;
        static int frameCount = 0;

        timeAcc += gDeltaTime;
        ++frameCount;

        if (timeAcc >= 1.0f)
        {
            wchar_t buffer[100];
            swprintf_s(buffer, 100, L"%s  |  FPS: %d", gSzTitle, frameCount);
            SetWindowText(gHWnd, buffer);

            timeAcc = 0.0f;
            frameCount = 0;
        }

        // 이벤트 처리하기

        // 타이머
        auto elapsedTime = duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count();
        float waitTime = 1.0f / FPS * 1000.0f - elapsedTime;

        if (waitTime > 0.0f)
        {
            Sleep(DWORD(waitTime)); // 고정
        }

        gDeltaTime = duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count() * 0.001f;
    }

    // 선택
    SelectObject(gMemDC, hOldBit);

    // 해제
    ReleaseDC(gHWnd, gHDC);
    DeleteDC(gMemDC);
    DeleteObject(hBitmap);

    Finalize();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(gHInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK or LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

Texture* LoadTexture(const std::string& name)
{
    std::unordered_map<std::string, Texture*>::iterator iterFindTexture = gMapTextures.find(name);

    if (iterFindTexture != gMapTextures.end())  // 있는 파일이면
    {
        return iterFindTexture->second;      // 재사용
    }

    Texture* texture = new Texture;
    texture->hDC = CreateCompatibleDC(gHDC); // 비트맵과 연결할 DC

    std::wstring wFileName = std::wstring(L"Resource\\") + std::wstring(name.begin(), name.end());
    texture->hBit = (HBITMAP)LoadImage(nullptr, wFileName.c_str(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    assert(texture->hBit != nullptr and "파일 오류");

    // 비트맵과 DC연결
    HBITMAP hPrevBit = (HBITMAP)SelectObject(texture->hDC, texture->hBit);
    DeleteObject(hPrevBit);

    // 비트맵 정보
    GetObject(texture->hBit, sizeof(BITMAP), &texture->bitInfo);

    gMapTextures.insert(make_pair(name, texture));

    return texture;
}

void DrawTexture(const Texture& texture, const vPoint pos, const vPoint scale)
{
    TransparentBlt(gMemDC, (int)pos.x, (int)pos.y,
        (int)(texture.bitInfo.bmWidth * scale.x), (int)(texture.bitInfo.bmHeight * scale.y),
        texture.hDC, 0, 0, (int)texture.bitInfo.bmWidth, (int)texture.bitInfo.bmHeight, RGB(0, 255, 0));
}

void LoadAnimation(Animation* animation, const std::string& name, int32_t maxFrame, float time, bool bLoop)
{
    assert(animation != nullptr);

    animation->texture.reserve(maxFrame);
    animation->currentFrame = 0;
    animation->frameCount = maxFrame;
    animation->frameIntervalTime = time;
    animation->elapsedTime = 0.0f;
    animation->loop = bLoop;

    for (int64_t i = 0; i < maxFrame; ++i)
    {
        std::string fileName = name + std::to_string(i) + ".bmp";
        Texture* texture = LoadTexture(fileName);
        assert(texture != nullptr and "애니메이션 오류");

        animation->texture.push_back(texture);
    }
}

void InitializeAnimation(Animation* animation, bool dead)
{
    animation->elapsedTime = 0.0f;
    animation->currentFrame = 0;
    animation->dead = dead;
}

void UpdateAnimation(Animation* animation)
{
    if (animation->texture.empty())
    {
        return;
    }

    animation->elapsedTime += gDeltaTime;

    if (animation->elapsedTime >= animation->frameIntervalTime)
    {
        animation->elapsedTime = 0.0f;
        ++animation->currentFrame;

        if (animation->currentFrame >= animation->frameCount)
        {
            if (animation->loop)
            {
                animation->currentFrame = 0;
            }
            else
            {
                if (animation->dead)
                {
                    animation->currentFrame = animation->frameCount - 1;
                }
            }
        }
    }
}

void DrawAnimation(const Animation& animation, const vPoint pos, const vPoint scale)
{
    if (animation.texture.empty())
    {
        return;
    }

    Texture* current = animation.texture[animation.currentFrame];
    assert(current != nullptr);

    DrawTexture(*current, pos, scale);
}

void Initialize()
{
    // 이미지
    constexpr int MAX_DIR_NUM = int(DIR_TYPE::END);
    const std::string dirName[MAX_DIR_NUM] = { "Left", "Right" };

    for (int dir = 0; dir < MAX_DIR_NUM; ++dir)
    {
        std::string playerFolderName = "Player\\" + dirName[dir] + "\\";
        std::string monsterFolderName = "Monster\\" + dirName[dir] + "\\";

        // 플레이어 이미지
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::IDLE][dir], playerFolderName + "idle", 6, 0.3f, true);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::WALK][dir], playerFolderName + "walk", 8, 0.3f, true);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::ATTACK][dir], playerFolderName + "attack_base", 6, 0.07f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::ESKILL][dir], playerFolderName + "eSkill", 9, 0.07f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DAMAGE][dir], playerFolderName + "damage", 4, 0.3f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DEAD][dir], playerFolderName + "dead", 4, 0.3f, false);

        // 몬스터 이미지
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::IDLE][dir], monsterFolderName + "idle", 6, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::WALK][dir], monsterFolderName + "walk", 8, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][dir], monsterFolderName + "attack_base", 6, 0.1f, true);
    }

    // 화살 이미지
    gArrowTextures[(int32_t)DIR_TYPE::LEFT] = LoadTexture("Player\\Left\\arrow.bmp");
    gArrowTextures[(int32_t)DIR_TYPE::RIGHT] = LoadTexture("Player\\Right\\arrow.bmp");
  

    // 키 입력 초기화
    for (int32_t i = 0; i < (int32_t)KEY::END; ++i)
    {
        gVecKeyInfo.emplace_back(KeyInfo{ KEY_STATE::NONE, false });
    }

    // 플레이어 초기화
    gPlayerInformation =
    {
        .pos = {.x = float(VIEWSIZE_X / 2.0f - 120.0f), .y = float(VIEWSIZE_Y / 2.0f - 100.0f)},
        .scale = {.x = 2.5f, .y = 2.5f },
        .speed = 100.0f,
        .currDir = DIR_TYPE::RIGHT,
        .currState = OBJECT_STATE::IDLE,
        .attacking = false,
        .dead = false
    };

    // 몬스터 초기화
    for (int32_t i = 0; i < MONSTER_COUNT; ++i)
    {
        gMonstersInformation[i] =
        {
            .pos = {.x = 120.0f * i, .y = 20.0f},
            .scale = {.x = 2.5f, .y = 2.5f },
            .speed = 70.0f,
            .currDir = DIR_TYPE::RIGHT,
            .currState = OBJECT_STATE::IDLE,
            .attacking = false,
            .dead = false
        };
    }
}

void Update()
{
    // 키 입력 업데이트
    for (int32_t i = 0; i < (int32_t)KEY::END; ++i)
    {
        if (GetAsyncKeyState(KeyMaps[i]) & 0x8000)  // 눌림
        {
            if (gVecKeyInfo[i].bPrevPush)
            {
                gVecKeyInfo[i].eState = KEY_STATE::HOLD;
            }
            else
            {
                gVecKeyInfo[i].eState = KEY_STATE::TAP;
            }

            gVecKeyInfo[i].bPrevPush = true;
        }
        else
        {
            if (gVecKeyInfo[i].bPrevPush)
            {
                gVecKeyInfo[i].eState = KEY_STATE::AWAY;
            }
            else
            {
                gVecKeyInfo[i].eState = KEY_STATE::NONE;
            }

            gVecKeyInfo[i].bPrevPush = false;
        }
    }

    // 플레이어 키 업데이트
    if (gVecKeyInfo[(int32_t)KEY::LBUTTON].eState == KEY_STATE::TAP and not gPlayerInformation.attacking)
    {
        gPlayerInformation.currState = OBJECT_STATE::ATTACK;
        InitializeAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir], false);

        gPlayerInformation.attacking = true;
    }

    if (gVecKeyInfo[(int32_t)KEY::Q].eState == KEY_STATE::TAP)
    {
        gPlayerInformation.currState = OBJECT_STATE::DAMAGE;
        InitializeAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir], false);
    }

    if (gVecKeyInfo[(int32_t)KEY::V].eState == KEY_STATE::TAP)
    {
        gPlayerInformation.currState = OBJECT_STATE::DEAD;
        gPlayerInformation.dead = true;

        InitializeAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir], gPlayerInformation.dead);
    }

    if (gVecKeyInfo[(int32_t)KEY::SPACE].eState == KEY_STATE::TAP and not gPlayerInformation.attacking)
    {
        gPlayerInformation.currState = OBJECT_STATE::ESKILL;
        gPlayerInformation.attacking = true;

        InitializeAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir], false);

        arrowFired = true;
    }

    int32_t moveDirX = int32_t(gVecKeyInfo[(int32_t)KEY::D].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::A].eState == KEY_STATE::HOLD);
    int32_t moveDirY = int32_t(gVecKeyInfo[(int32_t)KEY::S].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::W].eState == KEY_STATE::HOLD);
    bool PressMovingKey = (moveDirX != 0 or moveDirY != 0);

    if (PressMovingKey and (not gPlayerInformation.attacking))
    {
        float velocityX = moveDirX * gPlayerInformation.speed * gDeltaTime;
        gPlayerInformation.pos.x += velocityX;

        float velocityY = moveDirY * gPlayerInformation.speed * gDeltaTime;
        gPlayerInformation.pos.y += velocityY;

        if (moveDirX != 0)
        {
            gPlayerInformation.currDir = (moveDirX > 0) ? DIR_TYPE::RIGHT : DIR_TYPE::LEFT; // 삼항 연산자.
        }

        gPlayerInformation.currState = OBJECT_STATE::WALK;
    }
    else // 방향키는 안 눌렀고 공격 준비가 되었을 때
    {
        if (gPlayerInformation.currState == OBJECT_STATE::WALK)  // 애니메이션 강제로 바꾸기
        {
            gPlayerInformation.currState = OBJECT_STATE::IDLE;
        }

        Animation& animation = gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir];

        if (animation.currentFrame == animation.frameCount - 1
            and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
        {
            if (not(gPlayerInformation.currState == OBJECT_STATE::DEAD))
            {
                gPlayerInformation.currState = OBJECT_STATE::IDLE;
                gPlayerInformation.attacking = false;

            }
        }
    }
    
    // 화살 업데이트
    if (gPlayerInformation.currState == OBJECT_STATE::ESKILL and arrowFired)
    {
        Animation& eSkill = gPlayerAnimations[(int32_t)OBJECT_STATE::ESKILL][(int32_t)gPlayerInformation.currDir];

        if (eSkill.currentFrame == 5)
        {
            float dirX = (gPlayerInformation.currDir == DIR_TYPE::LEFT) ? -1.0f : 1.0f;
            bool Recycling = false;

            for (Arrow& currentArrows : gArrowsInformation)
            {
                if (not currentArrows.active)
                {
                    currentArrows =
                    {
                        .pos = {.x = gPlayerInformation.pos.x + 100.f, .y = gPlayerInformation.pos.y + 90.0f },
                        .scale = {.x = 2.0f, .y = 2.0f },
                        .speed = 200.0f,
                        .dirPos = {.x = dirX, .y = 0.0f },
                        .curDir = gPlayerInformation.currDir,
                        .elapsedTime = 0.0f,
                        .active = true   // 바로 그려지니까
                    };

                    Recycling = true;
                    break;
                }
            }

            if (!Recycling)
            {
                Arrow newArrow =
                {
                    .pos = {.x = gPlayerInformation.pos.x + 100.f, .y = gPlayerInformation.pos.y + 90.0f },
                    .scale = {.x = 2.0f, .y = 2.0f },
                    .speed = 200.0f,
                    .dirPos = {.x = dirX, .y = 0.0f },
                    .curDir = gPlayerInformation.currDir,
                    .elapsedTime = 0.0f,
                    .active = true   // 바로 그려지니까
                };

                gArrowsInformation.push_back(newArrow);
            }

            arrowFired = false; // 한 번만 그려지도록
        }

        //std::cout << gArrows.size() << std::endl;
    }

    // 화살 이동
    for (Arrow& move : gArrowsInformation)
    {
        if (move.active)
        {
            move.pos.x += move.dirPos.x * move.speed * gDeltaTime;
            move.elapsedTime += gDeltaTime;

            if (move.elapsedTime >= 2.0f)
            {
                move.active = false;
                move.elapsedTime = 0.0f;
            }
        }
    }
    
    // 몬스터 업데이트
    constexpr float TRACKING_RANGE_SQRTF = 120.0f * 120.0f;
    constexpr float STOP_DISTANCE_SQRTF = 50.0f * 50.0f;

    for (Monster& monster : gMonstersInformation)
    {
        float dx = gPlayerInformation.pos.x - monster.pos.x;
        float dy = gPlayerInformation.pos.y - monster.pos.y;

        // 방향 * 거리 = 최종 위치
        float distanceSqrtf = dx * dx + dy * dy;

        if (distanceSqrtf <= TRACKING_RANGE_SQRTF)
        {
            monster.currDir = (dx < 0) ? DIR_TYPE::LEFT : DIR_TYPE::RIGHT;
        }

        if (distanceSqrtf > TRACKING_RANGE_SQRTF)
        {
            monster.currState = OBJECT_STATE::IDLE;
            monster.attacking = false;
            continue;
        }

        if (abs(dx) <= 120.0f and abs(dy) <= 120.0f) // 추적
        {
            if ((distanceSqrtf > STOP_DISTANCE_SQRTF) and (not monster.attacking))
            {
                float len = sqrtf(distanceSqrtf);
                vPoint dir = { .x = dx / len, .y = dy / len };    // 정규화, 방향 정보만 남긴다

                monster.pos.x += dir.x * monster.speed * gDeltaTime;
                monster.pos.y += dir.y * monster.speed * gDeltaTime;

                monster.currState = OBJECT_STATE::WALK;
            }
            else
            {
                monster.currState = OBJECT_STATE::ATTACK;
                monster.attacking = true;
            }
        }
        else
        {
            monster.currState = OBJECT_STATE::IDLE;
            monster.attacking = false;
        }
    }
  
    // 애니메이션 업데이트
    for (int32_t state = 0; state < (int32_t)OBJECT_STATE::END; ++state)
    {
        for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
        {
            // 플레이어
            UpdateAnimation(&gPlayerAnimations[state][dir]);

            // 몬스터
            UpdateAnimation(&gMonsterAnimations[state][dir]);
        }
    }
}

void Draw()
{  
    // 배경
    HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(gMemDC, hBrush);

    Rectangle(gMemDC, -1, -1, VIEWSIZE_X + 1, VIEWSIZE_Y + 1);

    SelectObject(gMemDC, hOldBrush);
    DeleteObject(hBrush);

    // 플레이어 그리기
    DrawAnimation(gPlayerAnimations[(int32_t)gPlayerInformation.currState][(int32_t)gPlayerInformation.currDir], gPlayerInformation.pos, gPlayerInformation.scale);

    // 몬스터 그리기
    for (Monster& monster : gMonstersInformation)
    {
        DrawAnimation(gMonsterAnimations[(int32_t)monster.currState][(int32_t)monster.currDir], monster.pos, monster.scale);
    }

    // 화살 그리기
    for (Arrow& arrow : gArrowsInformation)
    {
        if (arrow.active)
        {
            DrawTexture(*gArrowTextures[(int32_t)arrow.curDir], arrow.pos, arrow.scale);
        }
    }
}

void Finalize()
{    
    // 리소스 해제
    for (auto& iter : gMapTextures)
    {
        Texture* texture = iter.second;

        DeleteDC(texture->hDC);
        DeleteObject(texture->hBit);

        // bmp 삭제
        delete iter.second;
    }
}