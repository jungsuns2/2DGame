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
    GROUND,

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

enum class COLLISION_TYPE
{
    PLAYER,
    PLAYER_SWORD,
    PLAYER_ARROW,
    MONSTER,
    MONSTER_SWORD,
    GROUND,
    END
};

enum class ATTACK_TYPE
{

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
    COLLISION_TYPE type;
    COLLISION_STATE state;

    vPoint offsetPos;
    vPoint finalPos;
    vPoint scale;
};

struct ColliderSet
{
    std::array<Collider, (int32_t)DIR_TYPE::END> collider;
    std::array<Collider, (int32_t)DIR_TYPE::END> attackCollider;
};

struct Object
{
    OBJECT_TYPE type;

    vPoint pos;
    vPoint scale;
    float speed;

    DIR_TYPE dir;
    OBJECT_STATE state;

    bool isDead;
};

struct Player
{
    Object object;
    int32_t hp;
    int32_t attack;
    int32_t damage;
    vPoint pushVelocity;

    bool isAttacking;
    bool isDamaging;

    ColliderSet colliderSet;
};

struct Monster
{
    Object object;
    int32_t hp;
    int32_t attack;
    int32_t damage;
    vPoint pushVelocity;

    bool isAttacking;
    bool isDamaging;

    ColliderSet colliderSet;
};

struct Arrow
{
    Object object;
    vPoint dirPos;

    float elapsedTime;
    float fireTime;

    int32_t attack;

    Collider collider;
};

struct Ground
{
    Object object;
    Collider collider;
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
static Arrow SpawnArrow(float offsetY, float fireTime);

static void DrawColliderBox(HDC dc, const Collider& collider, COLLISION_TYPE originalType);
static bool IsColliding(const Collider& objectA, const Collider& objectB);
static void UpdateCollisionsState(Collider* PrevCollider, bool isColliding, COLLISION_TYPE originalType, COLLISION_TYPE otherType);
static void ProcessCollision(Collider& colliderA, Collider& colliderB, COLLISION_TYPE typeA, COLLISION_TYPE typeB, bool isColliding,
                                 std::function<void()> onEnter = nullptr, std::function<void()> onStay = nullptr,
                                 std::function<void()> onExit = nullptr, std::function<void()> None = nullptr);

static void PrintText(HDC dc, int startX, int startY, const std::string& printStr, int fontSize, COLORREF fontColor);

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
constexpr int32_t MONSTER_COUNT = 1; // TODO) 10으로 바꾸자

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
static bool gIsArrowFired;

static std::array<Monster, MONSTER_COUNT>gMonstersInformation;
static Animation gMonsterAnimations[(int32_t)OBJECT_STATE::END][(int32_t)DIR_TYPE::END];

static std::vector<COLLISION_TYPE> gCollisionType;

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
    if ((gPlayerInformation.object.state == objectState) and gIsArrowFired)
    {
        Animation& keySkill = gPlayerAnimations[(int32_t)objectState][(int32_t)gPlayerInformation.object.dir];

        constexpr int32_t ARROW_COUNT = 3;
        constexpr float ARROW_INTERVAL_Y = 20.0f;

        Arrow recyclingArrows{};

        if (keySkill.currentFrame == 5)
        {
            if (isESkill)
            {
                for (int32_t cnt = 0; cnt < ARROW_COUNT; ++cnt)
                {
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
                   recyclingArrows = SpawnArrow(offsetY, fireTime);
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

                recyclingArrows = SpawnArrow(0.001f, 0.001f);
                activeArrows->push_back(std::move(recyclingArrows));
            }

            gIsArrowFired = false; // 한 번만 그려지도록
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

Arrow SpawnArrow(float offsetY, float fireTime)
{
    float dirX = (gPlayerInformation.object.dir == DIR_TYPE::LEFT) ? -1.0f : 1.0f;

    return 
    {   
        .object = 
        {
            .type = OBJECT_TYPE::ARROW,
            .pos {.x = gPlayerInformation.object.pos.x + 100.f, .y = (gPlayerInformation.object.pos.y + 90.0f) + offsetY},
            .scale {.x = 2.0f, .y = 2.0f},
            .speed = 200.0f, 
            .dir = gPlayerInformation.object.dir,
        },
        .dirPos = {.x = dirX, .y = 0.0f },
        .elapsedTime = 0.0f,
        .fireTime = fireTime,
        .attack = 20,

        .collider =
        {
            .type = COLLISION_TYPE::PLAYER_ARROW,
            .state = COLLISION_STATE::NONE,
            .offsetPos = {.x = 30.0f, .y = 35.0f },
            .scale = {.x = 27.0f, .y = 27.0f },
        },
    };
}

void DrawColliderBox(HDC dc, const Collider& collider, COLLISION_TYPE originalType)
{
    COLORREF color = (collider.type != originalType) ? RGB(255, 255, 0) : RGB(255, 0, 0);

    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oldPen = (HPEN)SelectObject(dc, pen);

    HBRUSH oldBrush = (HBRUSH)SelectObject(dc, GetStockObject(HOLLOW_BRUSH));

    Rectangle(dc, int(collider.finalPos.x - collider.scale.x / 2.0f), int(collider.finalPos.y - collider.scale.y / 2.0f),
        int(collider.finalPos.x + collider.scale.x / 2.0f), int(collider.finalPos.y + collider.scale.y / 2.0f));

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
}

bool IsColliding(const Collider& colliderA, const Collider& colliderB)
{
    if (colliderA.type == colliderB.type)
    {
        return false;
    }

    return 
        ( colliderA.finalPos.x < (colliderB.finalPos.x + colliderB.scale.x)
        && colliderB.finalPos.x < (colliderA.finalPos.x + colliderA.scale.x)
        && colliderA.finalPos.y < (colliderB.finalPos.y + colliderB.scale.y) 
        && colliderB.finalPos.y < (colliderA.finalPos.y + colliderA.scale.y) );
}

void UpdateCollisionsState(Collider* collider, bool isColliding, COLLISION_TYPE originalType, COLLISION_TYPE otherType)
{
    if (isColliding)
    {
        if (collider->state == COLLISION_STATE::NONE || collider->state == COLLISION_STATE::EXIT)
        {
            collider->state = COLLISION_STATE::ENTER;
            collider->type = otherType;
        }
        else
        {
            collider->state = COLLISION_STATE::STAY;
            collider->type = otherType;
        }
    }
    else
    {
        if (collider->state == COLLISION_STATE::ENTER || collider->state == COLLISION_STATE::STAY)
        {
            collider->state = COLLISION_STATE::EXIT;
            collider->type = originalType;
        }
        else
        {
            collider->state = COLLISION_STATE::NONE;
            collider->type = originalType;
        }
    }
}

void ProcessCollision(Collider& colliderA, Collider& colliderB, COLLISION_TYPE typeA, COLLISION_TYPE typeB, bool isColliding, 
    std::function<void()> Enter, std::function<void()> Stay, std::function<void()> Exit, std::function<void()> None)
{
    UpdateCollisionsState(&colliderA, isColliding, typeA, typeB);
    UpdateCollisionsState(&colliderB, isColliding, typeB, typeA);
    
    switch(colliderB.state)
    {
        case COLLISION_STATE::ENTER: 
        {
            if (Enter) Enter();
            break;
        }
        case COLLISION_STATE::STAY:  
        {
            if (Stay)  Stay();
            break;
        }
        case COLLISION_STATE::EXIT:  
        {
            if (Exit)  Exit();
            break;
        }
        case COLLISION_STATE::NONE:
        {
            if (None)  None();
            break;
        }
        default: 
            break;
    }
}

void PrintText(HDC dc, int startX, int startY, const std::string& printStr, int fontSize, COLORREF fontColor)
{
    std::wstring text = std::wstring(printStr.begin(), printStr.end());

    HFONT hFont = CreateFontW(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,             
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"궁서체");

    HFONT hOldFont = (HFONT)SelectObject(dc, hFont);
    COLORREF oldFontColor = SetTextColor(dc, fontColor);
    int oldBackGroundColor = SetBkMode(dc, TRANSPARENT);

    TextOutW(dc, startX, startY, text.c_str(), (int)text.size());

    SelectObject(dc, hOldFont);
    DeleteObject(hFont);
    SetTextColor(dc, oldFontColor);
    SetBkMode(dc, oldBackGroundColor);
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
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DAMAGE][dir], playerFolderName + "damage", 4, 0.3f, true);
        LoadAnimation(&gPlayerAnimations[(int32_t)OBJECT_STATE::DEAD][dir], playerFolderName + "dead", 4, 0.3f, false);

        // 몬스터 이미지
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::IDLE][dir], monsterFolderName + "idle", 6, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::WALK][dir], monsterFolderName + "walk", 8, 0.3f, true);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][dir], monsterFolderName + "attack_base", 6, 0.07f, false);
        LoadAnimation(&gMonsterAnimations[(int32_t)OBJECT_STATE::DAMAGE][dir], monsterFolderName + "damage", 4, 0.3f, false);
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
            .dir = DIR_TYPE::RIGHT,
            .state = OBJECT_STATE::IDLE,
            .isDead = false,
        },
        .hp = 10000,
        .attack = 10,
        .damage = 0,
        .isAttacking = false,
        .isDamaging = false,
    };

    gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::LEFT] =
    {     
         .type = COLLISION_TYPE::PLAYER,
        .state = COLLISION_STATE::NONE,
        .offsetPos = {.x = 122.0f, .y = 120.0f },
        .finalPos = {},
        .scale = {.x = 40.0f, .y = 40.0f },
    },
    gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::RIGHT] =
    {
        .type = COLLISION_TYPE::PLAYER,
        .state = COLLISION_STATE::NONE,
        .offsetPos = {.x = 122.0f, .y = 120.0f },
        .finalPos = {},
        .scale = {.x = 40.0f, .y = 40.0f },
    };

    gPlayerInformation.colliderSet.attackCollider[(int32_t)DIR_TYPE::LEFT] =
    {
        .type = COLLISION_TYPE::PLAYER_SWORD,
        .state = COLLISION_STATE::NONE,
        .offsetPos = {.x = 105.0f, .y = 120.0f },
        .finalPos = {},
        .scale = {.x = 65.0f, .y = 60.0f },
    };
    gPlayerInformation.colliderSet.attackCollider[(int32_t)DIR_TYPE::RIGHT] =
    {
        .type = COLLISION_TYPE::PLAYER_SWORD,
        .state = COLLISION_STATE::NONE,
        .offsetPos = {.x = 140.0f, .y = 120.0f },
        .finalPos = {},
        .scale = {.x = 65.0f, .y = 60.0f },
    };

    // 몬스터 초기화
    for (int32_t i = 0; i < MONSTER_COUNT; ++i)
    {
        gMonstersInformation[i] =
        {
            .object =
            {
                .type = OBJECT_TYPE::MONSTER,
                .pos = {.x = float(VIEWSIZE_X / 2.0f - 260.0f), .y = float(VIEWSIZE_Y / 2.0f - 100.0f)},
                /*.pos = {.x = 120.0f * i, .y = 20.0f},*/
                .scale = {.x = 2.5f, .y = 2.5f },
                .speed = 70.0f,
                .dir = DIR_TYPE::RIGHT,
                .state = OBJECT_STATE::IDLE,
                .isDead = false,
            },
            .hp = 100,
            .attack = 10,
            .damage = 0,
        };

        gMonstersInformation[i].colliderSet.collider[(int32_t)DIR_TYPE::LEFT] =
        {
                .type = COLLISION_TYPE::MONSTER,
                .state = COLLISION_STATE::NONE,
                .offsetPos = {.x = 120.0f, .y = 125.0f },
                .finalPos = {},
                .scale = {.x = 45.0f, .y = 45.0f },
        };
        gMonstersInformation[i].colliderSet.collider[(int32_t)DIR_TYPE::RIGHT] =
        {
                .type = COLLISION_TYPE::MONSTER,
                .state = COLLISION_STATE::NONE,
                .offsetPos = {.x = 125.0f, .y = 125.0f },
                .finalPos = {},
                .scale = {.x = 45.0f, .y = 45.0f },
        };

        gMonstersInformation[i].colliderSet.attackCollider[(int32_t)DIR_TYPE::LEFT] =
        {
            .type = COLLISION_TYPE::MONSTER_SWORD,
            .state = COLLISION_STATE::NONE,
            .offsetPos = {.x = 100.0f, .y = 125.0f },   
            .finalPos = {},
            .scale = {.x = 60.0f, .y = 50.0f },
        };
        gMonstersInformation[i].colliderSet.attackCollider[(int32_t)DIR_TYPE::RIGHT] =
        {
            .type = COLLISION_TYPE::MONSTER_SWORD,
            .state = COLLISION_STATE::NONE,
            .offsetPos = {.x = 140.0f, .y = 125.0f },   
            .finalPos = {},
            .scale = {.x = 60.0f, .y = 50.0f },
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

    Object& playerObject = gPlayerInformation.object;

    if (gVecKeyInfo[(int32_t)KEY::LBUTTON].eState == KEY_STATE::TAP and (not gPlayerInformation.isAttacking) and (not gPlayerInformation.isDamaging))
    {
        playerObject.state = OBJECT_STATE::ATTACK;
        ResetAnimation(&gPlayerAnimations[(int32_t)playerObject.state][(int32_t)playerObject.dir]);

        gPlayerInformation.isAttacking = true;
    }

    if (gVecKeyInfo[(int32_t)KEY::SPACE].eState == KEY_STATE::TAP and (not gPlayerInformation.isAttacking) and (not gPlayerInformation.isDamaging))
    {
        playerObject.state = OBJECT_STATE::SPACESKILL;
        ResetAnimation(&gPlayerAnimations[(int32_t)playerObject.state][(int32_t)playerObject.dir]);

        gPlayerInformation.isAttacking = true;

        printf("누름\n");
        gIsArrowFired = true;
    }

    if (gVecKeyInfo[(int32_t)KEY::E].eState == KEY_STATE::TAP and (not gPlayerInformation.isAttacking) and (not gPlayerInformation.isDamaging))
    {
        playerObject.state = OBJECT_STATE::ESKILL;
        ResetAnimation(&gPlayerAnimations[(int32_t)playerObject.state][(int32_t)playerObject.dir]);

        gPlayerInformation.isAttacking = true;
        gIsArrowFired = true;
    }

    int32_t moveDirX = int32_t(gVecKeyInfo[(int32_t)KEY::D].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::A].eState == KEY_STATE::HOLD);
    int32_t moveDirY = int32_t(gVecKeyInfo[(int32_t)KEY::S].eState == KEY_STATE::HOLD) - int32_t(gVecKeyInfo[(int32_t)KEY::W].eState == KEY_STATE::HOLD);
    bool pressingMoveKey = (moveDirX != 0 or moveDirY != 0);

    if (pressingMoveKey and (not gPlayerInformation.isAttacking))
    {
        float velocityX = moveDirX * playerObject.speed * gDeltaTime;
        playerObject.pos.x += velocityX;

        float velocityY = moveDirY * playerObject.speed * gDeltaTime;
        playerObject.pos.y += velocityY;

        if (moveDirX != 0)
        {
            playerObject.dir = (moveDirX > 0) ? DIR_TYPE::RIGHT : DIR_TYPE::LEFT; // 삼항 연산자.
        }

        playerObject.state = OBJECT_STATE::WALK;
    }
    else // 방향키는 안 눌렀고 공격 준비 되었을 때
    {
        if (playerObject.state == OBJECT_STATE::WALK)  // 애니메이션 강제로 바꾸기
        {
            playerObject.state = OBJECT_STATE::IDLE;
        }
        else
        {
            Animation& anim = gPlayerAnimations[(int32_t)playerObject.state][(int32_t)playerObject.dir];

            if (anim.currentFrame == anim.frameCount - 1
                and anim.elapsedTime >= anim.frameIntervalTime - 0.05f)
            {
                if (playerObject.state != OBJECT_STATE::DEAD)
                {
                    playerObject.state = OBJECT_STATE::IDLE;
                    gPlayerInformation.isAttacking = false;
                }
            }
        }
    }

    // 화살 업데이트
// move는 복사X, 소유권 이전O
    UpdateArrows(OBJECT_STATE::SPACESKILL, &gInactivateSpaceSkill, &gActivateSpaceSkill, false);
    UpdateArrows(OBJECT_STATE::ESKILL, &gInactivateESkill, &gActivateESkill, true);
    
    {   // 플레이어 상태 업데이트
        if (gPlayerInformation.isDamaging)
        {
            for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
            {
                if (gPlayerInformation.colliderSet.collider[dir].type == COLLISION_TYPE::MONSTER_SWORD)
                {
                    playerObject.state = OBJECT_STATE::DAMAGE;
                    ResetAnimation(&gPlayerAnimations[(int32_t)playerObject.state][(int32_t)playerObject.dir]);
                }

                printf("%d", gPlayerInformation.colliderSet.collider[dir].type);
            }
        }

    }
    
    {   // Damage 좌표 업데이트
        float drag = 500.0f;

        vPoint& velocity = gPlayerInformation.pushVelocity;
        vPoint& pos = playerObject.pos;

        // 위치 갱신
        pos.x += velocity.x * gDeltaTime;
        pos.y += velocity.y * gDeltaTime;

        // 감속
        if (velocity.x > 0.001f)
        {
            velocity.x = std::max(0.001f, velocity.x - drag * gDeltaTime);
        }
        else
        {
            velocity.x = std::min(0.001f, velocity.x + drag * gDeltaTime);
        }

        if (velocity.y > 0.001f)
        {
            velocity.y = std::max(0.001f, velocity.y - drag * gDeltaTime);
        }
        else
        {
            velocity.y = std::min(0.001f, velocity.y + drag * gDeltaTime);
        }

        if (gPlayerInformation.hp <= 0)
        {
            playerObject.isDead = true;
            playerObject.state = OBJECT_STATE::DEAD;
        }
    }

    // 몬스터 업데이트
    constexpr float VIEW_TRACKING_RANGE_SQRTF = 120.0f * 120.0f;
    constexpr float ATTACK_DISTANCE_SQRTF = 50.0f * 50.0f;

    for (Monster& monster : gMonstersInformation)
    {
        // 공격을 처리합니다.
        if (monster.object.state == OBJECT_STATE::ATTACK)
        {
            monster.isAttacking = true;

            Animation& animation = gMonsterAnimations[(int32_t)monster.object.state][(int32_t)monster.object.dir];
            if (animation.currentFrame == animation.frameCount - 1
                and animation.elapsedTime >= animation.frameIntervalTime - 0.05f)
            {
                monster.object.state = OBJECT_STATE::IDLE;
                monster.isAttacking = false;
            }

            break;
        }

        float dx = playerObject.pos.x - monster.object.pos.x;
        float dy = playerObject.pos.y - monster.object.pos.y;
        float distanceSqrtf = dx * dx + dy * dy;

        // 방향 * 거리 = 최종 위치
        if (distanceSqrtf > VIEW_TRACKING_RANGE_SQRTF)
        {
            monster.object.state = OBJECT_STATE::IDLE;
            continue;
        }

        monster.object.dir = (dx < 0) ? DIR_TYPE::LEFT : DIR_TYPE::RIGHT;

        // 이동을 처리합니다.
        if (distanceSqrtf > ATTACK_DISTANCE_SQRTF)
        {
            //if (monster.object.currState != OBJECT_STATE::WALK)
            //{
            //    // 충돌체를 기본으로 지정합니다.
            //}

            float len = sqrtf(distanceSqrtf);
            vPoint dir = { .x = dx / len, .y = dy / len }; // 정규화, 방향 정보만 남긴다.

            monster.object.pos.x += dir.x * monster.object.speed * gDeltaTime;
            monster.object.pos.y += dir.y * monster.object.speed * gDeltaTime;
            monster.object.state = OBJECT_STATE::WALK;
        }
        // 공격 상태로 전환합니다.
        else
        {
            if (monster.object.state != OBJECT_STATE::ATTACK)
            {
                monster.object.state = OBJECT_STATE::ATTACK;
                ResetAnimation(&gMonsterAnimations[(int32_t)monster.object.state][(int32_t)monster.object.dir]);

                // 충돌체를 공격 범위로 지정합니다.
            }
        }

        // TODO: 애니메이션이 너무 짧게 보인다.
        if (monster.isDamaging)
        {
            for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
            {
                if (monster.colliderSet.collider[dir].type != COLLISION_TYPE::MONSTER)
                {
                    monster.object.state = OBJECT_STATE::DAMAGE;
                    //ResetAnimation(&gMonsterAnimations[(int32_t)monster.object.state][(int32_t)monster.object.dir]);
                }
            }
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

void FinalUpdate()
{
    // 충돌 좌표 갱신

    // 플레이어
    for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
    {
        ColliderSet& colliderSet = gPlayerInformation.colliderSet;
        Object& playerObject = gPlayerInformation.object;

        Collider& collider = colliderSet.collider[dir];
        collider.finalPos.x = gPlayerInformation.object.pos.x + collider.offsetPos.x;
        collider.finalPos.y = gPlayerInformation.object.pos.y + collider.offsetPos.y;
        
        Collider& attackCollider = colliderSet.attackCollider[dir];
        attackCollider.finalPos.x = playerObject.pos.x + attackCollider.offsetPos.x;
        attackCollider.finalPos.y = playerObject.pos.y + attackCollider.offsetPos.y;
    }

    // 화살
    for (int32_t i = 0; i < gActivateSpaceSkill.size(); ++i)
    {
        Collider& spaceSkillCollider = gActivateSpaceSkill[i].collider;
        spaceSkillCollider.finalPos.x = gActivateSpaceSkill[i].object.pos.x + spaceSkillCollider.offsetPos.x;
        spaceSkillCollider.finalPos.y = gActivateSpaceSkill[i].object.pos.y + spaceSkillCollider.offsetPos.y;
    }

    for (int32_t i = 0; i < gActivateESkill.size(); ++i)
    {
        Collider& eSkillCollider = gActivateESkill[i].collider;
        eSkillCollider.finalPos.x = gActivateESkill[i].object.pos.x + eSkillCollider.offsetPos.x;
        eSkillCollider.finalPos.y = gActivateESkill[i].object.pos.y + eSkillCollider.offsetPos.y;
    }

    // 몬스터
    for (int32_t cnt = 0; cnt < gMonstersInformation.size(); ++cnt)
    {
        for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
        {
            ColliderSet& colliderSet = gMonstersInformation[cnt].colliderSet;
            Object& monsterObject = gMonstersInformation[cnt].object;

            Collider& collider = colliderSet.collider[dir];
            collider.finalPos.x = monsterObject.pos.x + collider.offsetPos.x;
            collider.finalPos.y = monsterObject.pos.y + collider.offsetPos.y;

            Collider& attackCollider = colliderSet.attackCollider[dir];
            attackCollider.finalPos.x = monsterObject.pos.x + attackCollider.offsetPos.x;
            attackCollider.finalPos.y = monsterObject.pos.y + attackCollider.offsetPos.y;
        }
    }
}

void UpdateColliders()
{
    for (Monster& monsterInfo : gMonstersInformation)
    {
        for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
        {
            Collider& playerCollider = gPlayerInformation.colliderSet.collider[dir];
            Collider& monsterAttackCollider = monsterInfo.colliderSet.attackCollider[dir];
           
            bool playerToMonsterAttack = false;
            if (not gPlayerInformation.object.isDead)
            {
                playerToMonsterAttack = IsColliding(playerCollider, monsterAttackCollider);
            }
            
            // 플레이어 몸체 - 몬스터의 칼
            ProcessCollision(playerCollider, monsterAttackCollider, COLLISION_TYPE::PLAYER, COLLISION_TYPE::MONSTER_SWORD, playerToMonsterAttack,
                [&]() 
                {
                    gPlayerInformation.hp -= monsterInfo.attack;
                }, 
                [&]()
                {
                    // 밀리도록 하기
                    Animation& anim = gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][(int32_t)monsterInfo.object.dir];
                    if (anim.currentFrame == 4)
                    {
                        float pushDirX = playerCollider.finalPos.x - monsterAttackCollider.finalPos.x;
                        float pushDirY = playerCollider.finalPos.y - monsterAttackCollider.finalPos.y;

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

                        gPlayerInformation.isDamaging = true;
                    }
                },
                [&]()
                {
                    monsterInfo.isDamaging = false;
                }
            );

        }
    }

    for (Monster& monsterInfo : gMonstersInformation)
    {
        for (int32_t dir = 0; dir < (int32_t)DIR_TYPE::END; ++dir)
        {
            Collider& playerAttackCollider = gPlayerInformation.colliderSet.attackCollider[dir];
            Collider& monsterCollider = monsterInfo.colliderSet.collider[dir];

            bool playerAttackToMonster = false;

            if (not gPlayerInformation.object.isDead)
            {
                playerAttackToMonster = IsColliding(playerAttackCollider, monsterCollider);
            }

            // 플레이어 칼 - 몬스터 몸체
            if (gPlayerInformation.isAttacking)
            {
                ProcessCollision(playerAttackCollider, monsterCollider, COLLISION_TYPE::PLAYER_SWORD, COLLISION_TYPE::MONSTER, playerAttackToMonster,
                    [&]()
                    {
                        monsterInfo.hp -= gPlayerInformation.attack;
                    },
                    [&]()
                    {
                        // 밀리도록 하기
                        Animation& anim = gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][(int32_t)monsterInfo.object.dir];
                        if (anim.currentFrame == 4)
                        {
                            float pushDirX = monsterCollider.finalPos.x - playerAttackCollider.finalPos.x;
                            float pushDirY = monsterCollider.finalPos.y - playerAttackCollider.finalPos.y;

                            // 정규화
                            float pushSqrtf = pushDirX * pushDirX + pushDirY * pushDirY;
                            float len = std::sqrt(pushSqrtf);

                            if (len != 0.0f)
                            {
                                pushDirX /= len;
                                pushDirY /= len;
                            }

                            float power = 60.0f;
                            monsterInfo.pushVelocity.x += pushDirX * power;
                            monsterInfo.pushVelocity.y += pushDirY * power;

                            monsterInfo.isDamaging = true;
                        }
                    },
                    [&]()
                    {
                        monsterInfo.isDamaging = false;
                    }
                );
            }
        }
    }
}

void Draw()
{  
    // 배경
    gBackGroundBrush = CreateSolidBrush(RGB(30, 30, 30));
    gBackGroundOldBrush = (HBRUSH)SelectObject(gMemDC, gBackGroundBrush);

    Rectangle(gMemDC, -1, -1, VIEWSIZE_X + 1, VIEWSIZE_Y + 1);

    // 플레이어 그리기
    DrawAnimation(gPlayerAnimations[(int32_t)gPlayerInformation.object.state][(int32_t)gPlayerInformation.object.dir], 
        gPlayerInformation.object.pos, gPlayerInformation.object.scale);
    
    char buffer[64];
    sprintf_s(buffer, "hp: %d", gPlayerInformation.hp);
    PrintText(gMemDC, int(gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::LEFT].finalPos.x - 25),
        int(gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::LEFT].finalPos.y - 40), buffer, 12, RGB(255, 255, 255));

    // 몬스터 그리기
    for (Monster& monster : gMonstersInformation)
    {
        DrawAnimation(gMonsterAnimations[(int32_t)monster.object.state][(int32_t)monster.object.dir], monster.object.pos, monster.object.scale);

        char buffer[64];
        sprintf_s(buffer, "hp: %d", monster.hp);
        PrintText(gMemDC, int(monster.colliderSet.collider[(int32_t)DIR_TYPE::LEFT].finalPos.x - 25),
            int(monster.colliderSet.collider[(int32_t)DIR_TYPE::LEFT].finalPos.y - 40), buffer, 12, RGB(255, 255, 255));
    }
    
    // Space Skill 화살
    for (Arrow& arrows : gActivateSpaceSkill)
    {
        DrawTexture(*gArrowTextures[(int32_t)arrows.object.dir], arrows.object.pos, arrows.object.scale);
    }

    // E Skill 화살
    for (Arrow& arrows : gActivateESkill)
    {
        DrawTexture(*gArrowTextures[(int32_t)arrows.object.dir], arrows.object.pos, arrows.object.scale);
    }

    // 충돌박스 그리기
    if (gIsPen)
    {
        // 플레이어
        if (gPlayerInformation.object.dir == DIR_TYPE::LEFT)
        {
            if (not gPlayerInformation.isAttacking)
            {
                Collider& collider = gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::LEFT];
                DrawColliderBox(gMemDC, collider, COLLISION_TYPE::PLAYER);
            }
            else
            {
                const Animation& anim = gPlayerAnimations[(int32_t)OBJECT_STATE::ATTACK][(int32_t)gPlayerInformation.object.dir];
                if (anim.currentFrame == 4)
                {
                    Collider& attackCollider = gPlayerInformation.colliderSet.attackCollider[(int32_t)DIR_TYPE::LEFT];
                    DrawColliderBox(gMemDC, attackCollider, COLLISION_TYPE::PLAYER_SWORD);

                }
            }
        }
        else
        {
            if (not gPlayerInformation.isAttacking)
            {
                Collider& collider = gPlayerInformation.colliderSet.collider[(int32_t)DIR_TYPE::RIGHT];
                DrawColliderBox(gMemDC, collider, COLLISION_TYPE::PLAYER);
            }
            else
            {
                const Animation& anim = gPlayerAnimations[(int32_t)OBJECT_STATE::ATTACK][(int32_t)gPlayerInformation.object.dir];
                if (anim.currentFrame == 4)
                {
                    Collider& attackCollider = gPlayerInformation.colliderSet.attackCollider[(int32_t)DIR_TYPE::RIGHT];
                    DrawColliderBox(gMemDC, attackCollider, COLLISION_TYPE::PLAYER_SWORD);

                }
            }
        }

        // 화살 충돌 박스
        for (Arrow& spaceSkill : gActivateSpaceSkill)
        {
            Collider& spaceSkillArrowCollider = spaceSkill.collider;
            DrawColliderBox(gMemDC, spaceSkillArrowCollider, COLLISION_TYPE::PLAYER_ARROW);
        }

        for (Arrow& eSkill : gActivateESkill)
        {
            Collider& eSkillArrowCollider = eSkill.collider;
            DrawColliderBox(gMemDC, eSkillArrowCollider, COLLISION_TYPE::PLAYER_ARROW);
        }

        // 몬스터 충돌박스
        for (Monster& monster : gMonstersInformation)
        {   
            if (monster.object.dir == DIR_TYPE::LEFT)
            {
                if (not monster.isAttacking)
                {
                    Collider& collider = monster.colliderSet.collider[(int32_t)DIR_TYPE::LEFT];
                    DrawColliderBox(gMemDC, collider, COLLISION_TYPE::MONSTER);
                }
                else
                {
                    const Animation& anim = gMonsterAnimations[(int32_t)OBJECT_STATE::ATTACK][(int32_t)monster.object.dir];
                    if (anim.currentFrame == 4)
                    {
                        Collider& attackCollider = monster.colliderSet.attackCollider[(int32_t)DIR_TYPE::LEFT];
                        DrawColliderBox(gMemDC, attackCollider, COLLISION_TYPE::MONSTER_SWORD);

                    }
                }
            }
            else
            {
                if (not monster.isAttacking)
                {
                    Collider& collider = monster.colliderSet.collider[(int32_t)DIR_TYPE::RIGHT];
                    DrawColliderBox(gMemDC, collider, COLLISION_TYPE::MONSTER);
                }
                else
                {
                    Animation& anim = gMonsterAnimations[(int32_t)monster.object.state][(int32_t)monster.object.dir];
                    if (anim.currentFrame == 4)
                    {
                        Collider& attackCollider = monster.colliderSet.attackCollider[(int32_t)DIR_TYPE::RIGHT];
                        DrawColliderBox(gMemDC, attackCollider, COLLISION_TYPE::MONSTER_SWORD);

                    }
                }
            }
        }
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