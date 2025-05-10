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

enum class OBJECT_TYPE
{
    PLAYER,
    ARROW,
    MONSTER,

    END
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
    SPACESKILL,
    ESKILL,
    DAMAGE,
    DEAD,
    END
};

enum class COLLISION_STATE
{
    NONE,
    ENTER,
    STAY,
    EXIT
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

struct Collider
{
    COLLISION_STATE state;
    vPoint offsetPos;
    vPoint finalPos;
    vPoint scale;
    bool isColliding;
};

struct Object
{
    OBJECT_TYPE type;

    vPoint pos;
    vPoint scale;
    float speed;

    DIR_TYPE currDir;
    OBJECT_STATE currState;

    bool isAttacking;
    bool isDead;
    
    Collider collider;
};

struct Player
{
    Object object;
    int32_t hp;
    int32_t attack;
    int32_t damage;
    vPoint pushVelocity;
};

struct Monster
{
    Object object;
    int32_t hp;
    int32_t attack;
    int32_t damage;
};

struct Arrow
{
    Object object;
    vPoint dirPos;

    float elapsedTime;
    float fireTime;
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
    
    bool isLoop;
};

// ===============================================================
// 함수
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static Texture* LoadTexture(const std::string& name);
static void DrawTexture(const Texture& texture, const vPoint pos, const vPoint scale);

static void LoadAnimation(Animation* animation, const std::string& name, int32_t maxFrame, float time, bool bLoop);
static void ResetAnimation(Animation* animation);
static void DrawAnimation(const Animation& animation, const vPoint pos, const vPoint scale);
static void UpdateAnimation(Animation* animation);

static void UpdateArrows(OBJECT_STATE objectState, std::vector<Arrow>* inactivateArrows, std::vector<Arrow>* activeArrows, bool isESkill);
static Arrow SponeArrow(float offsetX, float offsetY, float fireTime);

static void DrawColliderBox(HDC dc, const Collider& collider, bool isColliding);
static bool IsColliding(const Object& objectA, const Object& objectB);
static void CollisionsState(Collider* PrevCollider, bool isCurrentlyColliding);

static void Initialize();
static void Update();
static void FinalUpdate();
static void UpdateColliders();
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

static HBITMAP gBitmap;
static HBITMAP gOldBit;

// 배경색
static HBRUSH gBackGroundBrush;
static HBRUSH gBackGroundOldBrush;

// 충돌 박스
static bool gIsPen;

static float gDeltaTime;

static std::vector<KeyInfo> gVecKeyInfo;

static std::unordered_map<std::string, Texture*> gMapTextures;

static Player gPlayerInformation;
static Animation gPlayerAnimations[(int32_t)OBJECT_STATE::END][(int32_t)DIR_TYPE::END];

static std::vector<Arrow> gActivateSpaceSkill;
static std::vector<Arrow> gInactivateSpaceSkill;
static std::vector<Arrow> gActivateESkill;
static std::vector<Arrow> gInactivateESkill;
static Texture* gArrowTextures[(int32_t)DIR_TYPE::END];
static bool gArrowFired;

static std::array<Monster, MONSTER_COUNT>gMonstersInformation;
static Animation gMonsterAnimations[(int32_t)OBJECT_STATE::END][(int32_t)DIR_TYPE::END];

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

    gBitmap = CreateCompatibleBitmap(gHDC, VIEWSIZE_X, VIEWSIZE_Y);
    gOldBit = (HBITMAP)SelectObject(gMemDC, gBitmap);

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
        FinalUpdate();

        // 충돌 업데이트
        UpdateColliders();

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
    animation->isLoop = bLoop;

    for (int64_t i = 0; i < maxFrame; ++i)
    {
        std::string fileName = name + std::to_string(i) + ".bmp";
        Texture* texture = LoadTexture(fileName);
        assert(texture != nullptr and "애니메이션 오류");

        animation->texture.push_back(texture);
    }
}

void ResetAnimation(Animation* animation)
{
    assert(animation != nullptr);

    animation->elapsedTime = 0.0f;
    animation->currentFrame = 0;
}

void UpdateAnimation(Animation* animation)
{
    assert(animation != nullptr);

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
            if (animation->isLoop)
            {
                animation->currentFrame = 0;
            }
            else
            {
                animation->currentFrame = animation->frameCount - 1;
            }
        }
    }
}

void UpdateArrows(OBJECT_STATE objectState, std::vector<Arrow>* inactivateArrows, std::vector<Arrow>* activeArrows, bool isESkill)
{
    if ((gPlayerInformation.object.currState == objectState) and gArrowFired)
    {
        Animation& keySkill = gPlayerAnimations[(int32_t)objectState][(int32_t)gPlayerInformation.object.currDir];

        constexpr int32_t ARROW_COUNT = 3;
        constexpr float ARROW_INTERVAL_X = 10.0f;
        constexpr float ARROW_INTERVAL_Y = 20.0f;

        Arrow recyclingArrows{};

        if (keySkill.currentFrame == 5)
        {
            if (isESkill)
            {
                for (int32_t cnt = 0; cnt < ARROW_COUNT; ++cnt)
                {
                   float offsetX = (cnt - 1) * ARROW_INTERVAL_X;
                   float offsetY = (cnt - 1) * ARROW_INTERVAL_Y;

                   if (not inactivateArrows->empty())
                   {
                       recyclingArrows = std::move(inactivateArrows->back());
                       inactivateArrows->pop_back();
                   }
                   else
                   {
                       recyclingArrows = {};
                   }

                   float fireTime = cnt * 0.2f;

                   // 초기화
                   recyclingArrows = SponeArrow(offsetX, offsetY, fireTime);
                   activeArrows->push_back(std::move(recyclingArrows));
                }
            }
            else
            {
                if (not inactivateArrows->empty())
                {
                    recyclingArrows = std::move(inactivateArrows->back());
                    inactivateArrows->pop_back();
                }
                else
                {
                    recyclingArrows = {};
                }

                recyclingArrows = SponeArrow(0.001f, 0.001f, 0.001f);
                activeArrows->push_back(std::move(recyclingArrows));
            }

            gArrowFired = false; // 한 번만 그려지도록
        }
    }

    // 화살 이동 업데이트
    for (auto arrowIter = activeArrows->begin(); arrowIter != activeArrows->end();)
    {
        Arrow& currentArrows = *arrowIter;
        
        currentArrows.elapsedTime += gDeltaTime;

        if (currentArrows.elapsedTime >= currentArrows.fireTime)
        {
            currentArrows.object.pos.x += currentArrows.dirPos.x * currentArrows.object.speed * gDeltaTime;
        }

        if (currentArrows.elapsedTime >= (currentArrows.fireTime + 2.0f))
        {
            currentArrows.elapsedTime = 0.0f;

            inactivateArrows->push_back(std::move(currentArrows));
            arrowIter = activeArrows->erase(arrowIter);   // 지우면 다음 번째를 가리키게 된다.
        }
        else
        {
            ++arrowIter;
        }
    }
}

Arrow SponeArrow(float offsetX, float offsetY, float fireTime)
{
    float dirX = (gPlayerInformation.object.currDir == DIR_TYPE::LEFT) ? -1.0f : 1.0f;

    return 
    {   
        .object = 
        {
            .type = OBJECT_TYPE::ARROW,
            .pos {.x = (gPlayerInformation.object.pos.x + 100.f) + offsetX, .y = (gPlayerInformation.object.pos.y + 90.0f) + offsetY},
            .scale {.x = 2.0f, .y = 2.0f},
            .speed = 200.0f, 
            .currDir = gPlayerInformation.object.currDir,

            .collider =
            {
                .state = COLLISION_STATE::NONE,
                .offsetPos = {.x = 30.0f, .y = 35.0f },
                .scale = {.x = 27.0f, .y = 27.0f },
            },
        },
        .dirPos = {.x = dirX, .y = 0.0f },
        .elapsedTime = 0.0f,
        .fireTime = fireTime
    };
}

void DrawColliderBox(HDC dc, const Collider& collider, bool isColliding)
{
    COLORREF color = isColliding ? RGB(255, 255, 0) : RGB(255, 0, 0);

    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oldPen = (HPEN)SelectObject(dc, pen);

    HBRUSH oldBrush = (HBRUSH)SelectObject(dc, GetStockObject(HOLLOW_BRUSH));

    Rectangle(dc, int(collider.finalPos.x - collider.scale.x / 2.0f), int(collider.finalPos.y - collider.scale.y / 2.0f),
        int(collider.finalPos.x + collider.scale.x / 2.0f), int(collider.finalPos.y + collider.scale.y / 2.0f));

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
}

bool IsColliding(const Object& objectA, const Object& objectB)
{
    if (objectA.type == objectB.type)
    {
        return false;
    }

    const Collider& A = objectA.collider;
    const Collider& B = objectB.collider;

    return 
        (abs(A.finalPos.x - B.finalPos.x) < (A.scale.x + B.scale.x) / 2.f &&
        abs(A.finalPos.y - B.finalPos.y) < (A.scale.y + B.scale.y) / 2.f);
}

void CollisionsState(Collider* PrevCollider, bool isCurrentlyColliding)
{
    if (isCurrentlyColliding)
    {
        if (not PrevCollider->isColliding)
        {
            PrevCollider->state = COLLISION_STATE::ENTER;
        }
        else
        {
            PrevCollider->state = COLLISION_STATE::STAY;
        }
    }
    else
    {
        if (PrevCollider->isColliding)
        {
            PrevCollider->state = COLLISION_STATE::EXIT;
        }
        else
        {
            PrevCollider->state = COLLISION_STATE::NONE;
        }
    }

    PrevCollider->isColliding = isCurrentlyColliding;
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
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::SPACESKILL][dir], playerFolderName + "eSkill", 9, 0.07f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::ESKILL][dir], playerFolderName + "eSkill", 9, 0.07f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DAMAGE][dir], playerFolderName + "damage", 4, 1.0f, false);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DEAD][dir], playerFolderName + "dead", 4, 1.0f, false);

        // 몬스터 이미지
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::IDLE][dir], monsterFolderName + "idle", 6, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::WALK][dir], monsterFolderName + "walk", 8, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][dir], monsterFolderName + "attack_base", 6, 0.1f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::DAMAGE][dir], monsterFolderName + "damage", 4, 1.0f, true);
    }

    // 화살 초기화
    {
        gArrowTextures[(int32_t)DIR_TYPE::LEFT] = LoadTexture("Player\\Left\\arrow.bmp");
        gArrowTextures[(int32_t)DIR_TYPE::RIGHT] = LoadTexture("Player\\Right\\arrow.bmp");

        gActivateSpaceSkill.reserve(3);
        gInactivateSpaceSkill.reserve(3);

        gActivateESkill.reserve(9);
        gInactivateESkill.reserve(9);
    }

    // 키 입력 초기화
    for (int32_t i = 0; i < (int32_t)KEY::END; ++i)
    {
        gVecKeyInfo.emplace_back(KeyInfo{ KEY_STATE::NONE, false });
    }

    // 플레이어 초기화
    gPlayerInformation =
    {
        .object =
        {
            .type = OBJECT_TYPE::PLAYER,
            .pos = {.x = float(VIEWSIZE_X / 2.0f - 120.0f), .y = float(VIEWSIZE_Y / 2.0f - 100.0f)},
            .scale = {.x = 2.5f, .y = 2.5f },
            .speed = 100.0f,
            .currDir = DIR_TYPE::RIGHT,
            .currState = OBJECT_STATE::IDLE,
            .isAttacking = false,
            .isDead = false,
            .collider = 
            {
                .state = COLLISION_STATE::NONE,
                .offsetPos = {.x = 122.0f, .y = 120.0f },
                .finalPos = {},
                .scale = {.x = 40.0f, .y = 40.0f },
                .isColliding = false,
            }
        },
        .hp = 100,
        .attack = 10,
        .damage = 10
    };

    // 몬스터 초기화
    for (int32_t i = 0; i < MONSTER_COUNT; ++i)
    {
        gMonstersInformation[i] =
        {
            .object =
            {
                .type = OBJECT_TYPE::MONSTER,
                .pos = {.x = 120.0f * i, .y = 20.0f},
                .scale = {.x = 2.5f, .y = 2.5f },
                .speed = 70.0f,
                .currDir = DIR_TYPE::RIGHT,
                .currState = OBJECT_STATE::IDLE,
                .isAttacking = false,
                .isDead = false,

                .collider = 
                {
                    .state = COLLISION_STATE::NONE,
                    .offsetPos = {.x = 130.0f, .y = 125.0f },
                    .finalPos = {},
                    .scale = {.x = 50.0f, .y = 50.0f },
                    .isColliding = false,
                }
            },
            .hp = 100,
            .attack = 10,
            .damage = 10
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
    if (gVecKeyInfo[(int32_t)KEY::T].eState == KEY_STATE::TAP)
    {
        gIsPen = not gIsPen;
    }

    if (gVecKeyInfo[(int32_t)KEY::LBUTTON].eState == KEY_STATE::TAP and (not gPlayerInformation.object.isAttacking))
    {
        gPlayerInformation.object.currState = OBJECT_STATE::ATTACK;
        ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);

        // 방향 별로 공격 범위 증가
        Collider& playerAttackCollider = gPlayerInformation.object.collider;

        if (gPlayerInformation.object.currDir == DIR_TYPE::LEFT)
        {
            playerAttackCollider.offsetPos = { 105.0f, 120.0f };
            playerAttackCollider.scale = { 65.0f, 60.0f };
        }
        else
        {
            playerAttackCollider.offsetPos = { 145.0f, 120.0f };
            playerAttackCollider.scale = { 65.0f, 60.0f };
        }

        gPlayerInformation.object.isAttacking = true;
    }

    if (gVecKeyInfo[(int32_t)KEY::SPACE].eState == KEY_STATE::TAP and not gPlayerInformation.object.isAttacking)
    {
        gPlayerInformation.object.currState = OBJECT_STATE::SPACESKILL;
        gPlayerInformation.object.isAttacking = true;

        ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);

        printf("누름\n");
        gArrowFired = true;
    }

    if (gVecKeyInfo[(int32_t)KEY::E].eState == KEY_STATE::TAP and not gPlayerInformation.object.isAttacking)
    {
        gPlayerInformation.object.currState = OBJECT_STATE::ESKILL;
        gPlayerInformation.object.isAttacking = true;

        ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);

        gArrowFired = true;
    }

    // 더이상 좌클릭을 하지 않으면 원래 충돌박스 크기로 돌아간다.
    if (gVecKeyInfo[(int32_t)KEY::LBUTTON].eState == KEY_STATE::NONE and (not gPlayerInformation.object.isAttacking))
    {
        Collider& playerAttackCollider = gPlayerInformation.object.collider;

        if (gPlayerInformation.object.currDir == DIR_TYPE::LEFT)
        {
            playerAttackCollider.offsetPos = { .x = 128.0f, .y = 120.0f };
            playerAttackCollider.scale = { .x = 40.0f, .y = 40.0f };
        }
        else
        {
            playerAttackCollider.offsetPos = { .x = 122.0f, .y = 120.0f };
            playerAttackCollider.scale = { .x = 40.0f, .y = 40.0f };
        }
    }

    int32_t moveDirX = int32_t(gVecKeyInfo[(int32_t)KEY::D].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::A].eState == KEY_STATE::HOLD);
    int32_t moveDirY = int32_t(gVecKeyInfo[(int32_t)KEY::S].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::W].eState == KEY_STATE::HOLD);
    bool pressingMoveKey = (moveDirX != 0 or moveDirY != 0);

    if (pressingMoveKey and (not gPlayerInformation.object.isAttacking))
    {
        float velocityX = moveDirX * gPlayerInformation.object.speed * gDeltaTime;
        gPlayerInformation.object.pos.x += velocityX;

        float velocityY = moveDirY * gPlayerInformation.object.speed * gDeltaTime;
        gPlayerInformation.object.pos.y += velocityY;

        if (moveDirX != 0)
        {
            gPlayerInformation.object.currDir = (moveDirX > 0) ? DIR_TYPE::RIGHT : DIR_TYPE::LEFT; // 삼항 연산자.
        }

        gPlayerInformation.object.currState = OBJECT_STATE::WALK;
    }
    else // 방향키는 안 눌렀고 공격 준비가 되었을 때
    {
        if (gPlayerInformation.object.currState == OBJECT_STATE::WALK)  // 애니메이션 강제로 바꾸기
        {
            gPlayerInformation.object.currState = OBJECT_STATE::IDLE;
        }
        else
        {
            Animation& animation = gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir];

            if (animation.currentFrame == animation.frameCount - 1
                and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
            {
                if (gPlayerInformation.object.currState != OBJECT_STATE::DEAD)
                {
                    gPlayerInformation.object.currState = OBJECT_STATE::IDLE;
                    gPlayerInformation.object.isAttacking = false;
                }
            }
        }
    }

    // 상태 업데이트
    if (gPlayerInformation.object.currState == OBJECT_STATE::DAMAGE)
    {
        //ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);
        
        Animation& animation = gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir];

        if (animation.currentFrame == animation.frameCount - 1
            and animation.elapsedTime >= animation.frameIntervalTime - 0.02f)
        {
            gPlayerInformation.object.currState = OBJECT_STATE::IDLE;
        }
    }

    {   // Damage 좌표 업데이트
        float drag = 500.0f;

        vPoint& velocity = gPlayerInformation.pushVelocity;
        vPoint& pos = gPlayerInformation.object.pos;

        // 위치 갱신
        pos.x += velocity.x * gDeltaTime;
        pos.y += velocity.y * gDeltaTime;

        // 감속
        if (velocity.x > 0.0f)
        {
            velocity.x = std::max(0.0f, velocity.x - drag * gDeltaTime);
        }
        else
        {
            velocity.x = std::min(0.0f, velocity.x + drag * gDeltaTime);
        }

        if (velocity.y > 0.0f)
        {
            velocity.y = std::max(0.0f, velocity.y - drag * gDeltaTime);
        }
        else
        {
            velocity.y = std::min(0.0f, velocity.y + drag * gDeltaTime);
        }


        if (gPlayerInformation.hp <= 0)
        {
            gPlayerInformation.object.isDead = true;
            gPlayerInformation.object.currState = OBJECT_STATE::DEAD;
        }
    }


    // 화살 업데이트
    // move는 복사X, 소유권 이전O
    UpdateArrows(OBJECT_STATE::SPACESKILL, &gInactivateSpaceSkill, &gActivateSpaceSkill, false);
    UpdateArrows(OBJECT_STATE::ESKILL, &gInactivateESkill, &gActivateESkill, true);

    // 몬스터 업데이트
    constexpr float TRACKING_RANGE_SQRTF = 120.0f * 120.0f;
    constexpr float STOP_DISTANCE_SQRTF = 60.0f * 60.0f;

    for (Monster& monster : gMonstersInformation)
    {
        if (monster.object.currState != OBJECT_STATE::ATTACK)
        {
            Collider& monsterAttackCollider = monster.object.collider;

            if (monster.object.currDir == DIR_TYPE::LEFT)
            {
                monsterAttackCollider.offsetPos = { .x = 120.0f, .y = 125.0f };
                monsterAttackCollider.scale = { .x = 50.0f, .y = 50.0f };
            }
            else
            {
                monsterAttackCollider.offsetPos = { .x = 130.0f, .y = 125.0f };
                monsterAttackCollider.scale = { .x = 50.0f, .y = 50.0f };
            }
        }

        if(monster.object.currState == OBJECT_STATE::ATTACK)
        {
            Animation& animation = gMonsterAnimations[(int32_t)monster.object.currState][(int32_t)monster.object.currDir];

            if (animation.currentFrame == animation.frameCount - 1
                and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
            {
                if (monster.object.currState != OBJECT_STATE::DEAD)
                {
                    monster.object.isAttacking = false;
                    ResetAnimation(&animation);
                }
            }
        }

        float dx = gPlayerInformation.object.pos.x - monster.object.pos.x;
        float dy = gPlayerInformation.object.pos.y - monster.object.pos.y;

        // 방향 * 거리 = 최종 위치
        float distanceSqrtf = dx * dx + dy * dy;

        if (distanceSqrtf > TRACKING_RANGE_SQRTF)
        {
            monster.object.currState = OBJECT_STATE::IDLE;
            monster.object.isAttacking = false;
            continue;
        } 
        else
        {
            monster.object.currDir = (dx < 0) ? DIR_TYPE::LEFT : DIR_TYPE::RIGHT;
        }

        if (bool bTrackingable = (abs(dx) <= 120.0f and abs(dy) <= 120.0f);
            bTrackingable)
        {
            if (distanceSqrtf > STOP_DISTANCE_SQRTF and (not monster.object.isAttacking))
            {
                float len = sqrtf(distanceSqrtf);
                vPoint dir = { .x = dx / len, .y = dy / len }; // 정규화, 방향 정보만 남긴다.

                monster.object.pos.x += dir.x * monster.object.speed * gDeltaTime;
                monster.object.pos.y += dir.y * monster.object.speed * gDeltaTime;

                monster.object.currState = OBJECT_STATE::WALK;
            }
            else
            {
                monster.object.currState = OBJECT_STATE::ATTACK;
                monster.object.isAttacking = true;
                
                Collider& monsterAttackCollider = monster.object.collider;

                if (monster.object.currDir == DIR_TYPE::LEFT)
                {
                    monsterAttackCollider.offsetPos = { .x = 100.0f, .y = 125.0f };
                    monsterAttackCollider.scale = { .x = 50.0f, .y = 50.0f };
                }
                else
                {
                    monsterAttackCollider.offsetPos = { .x = 150.0f, .y = 125.0f };
                    monsterAttackCollider.scale = { .x = 50.0f, .y = 50.0f };
                }
            }
        }
        else
        {
            monster.object.currState = OBJECT_STATE::IDLE;
            monster.object.isAttacking = false;
        }
    }

    // 몬스터 상태 업데이트


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

void FinalUpdate()
{
    // 충돌 좌표 갱신

    // 플레이어
    Collider& playerCollider = gPlayerInformation.object.collider;
    playerCollider.finalPos.x = gPlayerInformation.object.pos.x + playerCollider.offsetPos.x;
    playerCollider.finalPos.y = gPlayerInformation.object.pos.y + playerCollider.offsetPos.y;

    // 화살
    for (size_t i = 0; i < gActivateSpaceSkill.size(); ++i)
    {
        Collider& spaceSkillArrowCollider = gActivateSpaceSkill[i].object.collider;

        spaceSkillArrowCollider.finalPos.x = gActivateSpaceSkill[i].object.pos.x + spaceSkillArrowCollider.offsetPos.x;
        spaceSkillArrowCollider.finalPos.y = gActivateSpaceSkill[i].object.pos.y + spaceSkillArrowCollider.offsetPos.y;
    }

    for (size_t i = 0; i < gActivateESkill.size(); ++i)
    {
        Collider& eSkillArrowCollider = gActivateESkill[i].object.collider;

        eSkillArrowCollider.finalPos.x = gActivateESkill[i].object.pos.x + eSkillArrowCollider.offsetPos.x;
        eSkillArrowCollider.finalPos.y = gActivateESkill[i].object.pos.y + eSkillArrowCollider.offsetPos.y;
    }

    // 몬스터
    for (size_t i = 0; i < gMonstersInformation.size(); ++i)
    {
        Collider& monsterCollider = gMonstersInformation[i].object.collider;

        monsterCollider.finalPos.x = gMonstersInformation[i].object.pos.x + monsterCollider.offsetPos.x;
        monsterCollider.finalPos.y = gMonstersInformation[i].object.pos.y + monsterCollider.offsetPos.y;
    }
}

void UpdateColliders()
{
    // 변수를 선언해야 플레이어 값이 덮여쓰여지지 않는다.
    bool playerCollided = false;

    for (Monster& monster : gMonstersInformation)
    {
        Collider& monsterCollider = monster.object.collider;
        Collider& playerCollider = gPlayerInformation.object.collider;
        bool isCurrentlyColliding = IsColliding(gPlayerInformation.object, monster.object);
        
        CollisionsState(&monster.object.collider, isCurrentlyColliding);

        if (gPlayerInformation.hp > 0)
        {
            if (not gPlayerInformation.object.isAttacking)
            {
                if (monsterCollider.state == COLLISION_STATE::STAY)
                {
                    playerCollided = true;
                    monsterCollider.isColliding = true;

                    float pushDirX = gPlayerInformation.object.collider.finalPos.x - monster.object.collider.finalPos.x;
                    float pushDirY = gPlayerInformation.object.collider.finalPos.y - monster.object.collider.finalPos.y;

                    // 정규화
                    float pushSqrtf = pushDirX * pushDirX + pushDirY * pushDirY;
                    float len = std::sqrt(pushSqrtf);

                    if (len != 0.0f)
                    {
                        pushDirX /= len;
                        pushDirY /= len;
                    }

                    float power = 60.0f;
                    gPlayerInformation.pushVelocity.x += pushDirX * power;
                    gPlayerInformation.pushVelocity.y += pushDirY * power;
                }
                else if (monsterCollider.state == COLLISION_STATE::ENTER)
                {
                    gPlayerInformation.object.currState = OBJECT_STATE::DAMAGE;
                    //ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);

                    //Animation& animation = gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir];
                    //if (animation.currentFrame == animation.frameCount - 1
                    //    and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
                    //{
                    //    gPlayerInformation.object.currState == OBJECT_STATE::IDLE;
                    //}

                    playerCollided = true;
                    monsterCollider.isColliding = true;

                    gPlayerInformation.hp -= monster.attack;

                    printf("Player Hp: %d \n", gPlayerInformation.hp);
                }
            }
            else
            {
                if (monster.hp > 0)
                {
                    if (monsterCollider.state == COLLISION_STATE::STAY)
                    {
                        playerCollided = true;
                        monsterCollider.isColliding = true;

                        float pushDirX = gPlayerInformation.object.collider.finalPos.x - monster.object.collider.finalPos.x;
                        float pushDirY = gPlayerInformation.object.collider.finalPos.y - monster.object.collider.finalPos.y;

                        // 정규화
                        float pushSqrtf = pushDirX * pushDirX + pushDirY * pushDirY;
                        float len = std::sqrt(pushSqrtf);

                        if (len != 0.0f)
                        {
                            pushDirX /= len;
                            pushDirY /= len;
                        }

                        float power = 30.0f;
                        monster.object.pos.x += pushDirX * power;
                        monster.object.pos.y += pushDirY * power;

                    }
                    else if (monsterCollider.state == COLLISION_STATE::ENTER)
                    {
                        monster.object.currState = OBJECT_STATE::DAMAGE;
                        //ResetAnimation(&gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir]);

                        //Animation& animation = gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir];
                        //if (animation.currentFrame == animation.frameCount - 1
                        //    and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
                        //{
                        //    gPlayerInformation.object.currState == OBJECT_STATE::IDLE;
                        //}

                        playerCollided = true;
                        monsterCollider.isColliding = true;

                        monster.hp -= gPlayerInformation.attack;

                        printf("Monster Hp: %d \n", monster.hp);
                    }
                }
                else
                {
                    monster.object.isDead = true;

                    if (monsterCollider.state == COLLISION_STATE::EXIT or monsterCollider.state == COLLISION_STATE::NONE)
                    {
                        playerCollider.isColliding = false;
                        monsterCollider.isColliding = false;
                    }

                    continue;
                }
            }
        }
        else
        {
            gPlayerInformation.object.isDead = true;

            if (monsterCollider.state == COLLISION_STATE::EXIT or monsterCollider.state == COLLISION_STATE::NONE)
            {
                playerCollider.isColliding = false;
                monsterCollider.isColliding = false;
            }

            continue;
        }
        
    }

    gPlayerInformation.object.collider.isColliding = playerCollided;
}

void Draw()
{  
    // 배경
    gBackGroundBrush = CreateSolidBrush(RGB(30, 30, 30));
    gBackGroundOldBrush = (HBRUSH)SelectObject(gMemDC, gBackGroundBrush);

    Rectangle(gMemDC, -1, -1, VIEWSIZE_X + 1, VIEWSIZE_Y + 1);

    // 플레이어 그리기
    DrawAnimation(gPlayerAnimations[(int32_t)gPlayerInformation.object.currState][(int32_t)gPlayerInformation.object.currDir], 
        gPlayerInformation.object.pos, gPlayerInformation.object.scale);

    // 몬스터 그리기
    for (Monster& monster : gMonstersInformation)
    {
        DrawAnimation(gMonsterAnimations[(int32_t)monster.object.currState][(int32_t)monster.object.currDir], monster.object.pos, monster.object.scale);
    }
    
    // Space Skill 화살
    for (Arrow& arrows : gActivateSpaceSkill)
    {
        DrawTexture(*gArrowTextures[(int32_t)arrows.object.currDir], arrows.object.pos, arrows.object.scale);
    }

    // E Skill 화살
    for (Arrow& arrows : gActivateESkill)
    {
        DrawTexture(*gArrowTextures[(int32_t)arrows.object.currDir], arrows.object.pos, arrows.object.scale);
    }

    // 충돌박스 그리기
    if (gIsPen)
    {
        Collider& playerCollider = gPlayerInformation.object.collider;
        DrawColliderBox(gMemDC, playerCollider, playerCollider.isColliding);

        // 몬스터 충돌박스
        for (Monster& monster : gMonstersInformation)
        {
            Collider& monsterCollider = monster.object.collider;
            DrawColliderBox(gMemDC, monsterCollider, monsterCollider.isColliding);
        }

         // 화살 충돌 박스
         for (Arrow& spaceSkill : gActivateSpaceSkill)
         {
             Collider& spaceSkillArrowCollider = spaceSkill.object.collider;
             DrawColliderBox(gMemDC, spaceSkillArrowCollider, spaceSkillArrowCollider.isColliding);
         }

         for (Arrow& eSkill : gActivateESkill)
         {
             Collider& eSkillArrowCollider = eSkill.object.collider;
             DrawColliderBox(gMemDC, eSkillArrowCollider, eSkillArrowCollider.isColliding);
         }
    }
    else
    {
        return;
    }
}

void Finalize()
{   
    // 선택
    SelectObject(gMemDC, gBackGroundOldBrush);
    SelectObject(gMemDC, gOldBit);

    // 해제
    ReleaseDC(gHWnd, gHDC);
    DeleteDC(gMemDC);
    DeleteObject(gBackGroundBrush);
    DeleteObject(gBitmap);

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