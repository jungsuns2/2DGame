#include "pch.h"
#pragma comment(lib, "msimg32.lib")
using namespace std::chrono;

// =========================================
// Enum
enum class DIR_TYPE
{
    LEFT,
    RIGHT,
    UP,
    DOWN,

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
    DIR_TYPE dir;
    float speed;
};

struct Monster
{
    vPoint pos;
    vPoint scale;
    DIR_TYPE dir;
};

struct KeyInfo
{
    KEY_STATE eState;
    bool bPrevPush;
};

// ===============================================================
// 함수
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static Texture* LoadTexture(const std::string& name);
static void DrawTexture(const Texture& texture, const vPoint pos, const vPoint scale);

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

static std::unordered_map<std::string, Texture*> gMapTexture;

static Player gPlayer;
static Texture* gPlayerTexture;
static std::array<Monster, MONSTER_COUNT>gArrayMonster;
static Texture* gMonsterTexture;

static std::vector<KeyInfo> gVecKeyInfo;

constexpr int32_t KeyMap[(int32_t)KEY::END]
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

static float gDeltaTime;
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

    gHWnd = CreateWindowW(gSzWindowClass, gSzTitle, WS_OVERLAPPEDWINDOW, centerX, centerY, windowX, windowY, nullptr, nullptr, hInstance, nullptr);

    if (not gHWnd)
    {
        return FALSE;
    }

    ShowWindow(gHWnd, nCmdShow);
    UpdateWindow(gHWnd);
    
    // 더블 버퍼링 생성
    gHDC = GetDC(gHWnd);
    gMemDC = CreateCompatibleDC(gHDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(gHDC, VIEWSIZE_X, VIEWSIZE_Y);
    HBITMAP hOldBit = (HBITMAP)SelectObject(gMemDC, hBitmap);

    {   // 게임의 기본 정보를 초기화
        gPlayerTexture = LoadTexture("ex.bmp");
        assert(gPlayerTexture and "Player.bmp 못 찾음");

        gMonsterTexture = LoadTexture("arrow.bmp");
        assert(gMonsterTexture and "Monster.bmp 못 찾음");

        // 키 입력 초기화
        for (int32_t i = 0; i < (int32_t)KEY::END; ++i)
        {
            gVecKeyInfo.emplace_back(KeyInfo{ KEY_STATE::NONE, false });
        }

        // 플레이어 초기화
        gPlayer = 
        {
            .pos = {.x = float(VIEWSIZE_X / 2.0f - 20.0f), .y = float(VIEWSIZE_Y / 2.0f)},
            .scale = { .x = 1.5f, .y = 1.5f },
            .dir = DIR_TYPE::RIGHT,
            .speed = 100.0f
        };

        // 몬스터 초기화
        for (int32_t i = 0; i < MONSTER_COUNT; ++i)
        {
            gArrayMonster[i] =
            {
                .pos = {.x = 80.0f * i, .y = 200.0f},
                .scale = { .x = 1.5f, .y = 1.5f }
            };
        }
    }

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
        {   // 키 입력 업데이트
            for (int32_t i = 0; i < (int32_t)KEY::END; ++i)
            {
                if (GetAsyncKeyState(KeyMap[i]) & 0x8000)  // 눌림
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

            // 플레이어 업데이트
            if (gVecKeyInfo[(int32_t)KEY::W].eState == KEY_STATE::HOLD)
            {
                gPlayer.pos.y -= gPlayer.speed * gDeltaTime;
            }
            if (gVecKeyInfo[(int32_t)KEY::S].eState == KEY_STATE::HOLD)
            {
                gPlayer.pos.y += gPlayer.speed * gDeltaTime;
            }
            if (gVecKeyInfo[(int32_t)KEY::A].eState == KEY_STATE::HOLD)
            {
                gPlayer.pos.x -= gPlayer.speed * gDeltaTime;
            }
            if (gVecKeyInfo[(int32_t)KEY::D].eState == KEY_STATE::HOLD)
            {
                gPlayer.pos.x += gPlayer.speed * gDeltaTime;
            }
        }

        {   // 그리기
            // 배경
            HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
            HBRUSH hOldBrush = (HBRUSH)SelectObject(gMemDC, hBrush);

            Rectangle(gMemDC, -1, -1, VIEWSIZE_X + 1, VIEWSIZE_Y + 1);

            SelectObject(gMemDC, hOldBrush);
            DeleteObject(hBrush);


            // 플레이어 그리기
            DrawTexture(*gPlayerTexture, gPlayer.pos, gPlayer.scale);

            // 몬스터 그리기
            for (Monster& monster : gArrayMonster)
            {
                DrawTexture(*gMonsterTexture, monster.pos, monster.scale);
            }
        }

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
    

    // 리소스 해제
    for (auto& iter : gMapTexture)
    {
        Texture* texture = iter.second;

        DeleteDC(texture->hDC);
        DeleteObject(texture->hBit);
    }

    // bmp 삭제
    std::unordered_map<std::string, Texture*> ::iterator iterTexture = gMapTexture.begin();

    for (; iterTexture != gMapTexture.end(); ++iterTexture)
    {
        delete iterTexture->second;
    }

    return (int) msg.wParam;
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
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
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
    std::unordered_map<std::string, Texture*>::iterator iterFindTexture = gMapTexture.find(name);

    if (iterFindTexture != gMapTexture.end())  // 있는 파일이면
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

    gMapTexture.insert(make_pair(name, texture));

    return texture;
}

void DrawTexture(const Texture& texture, const vPoint pos, const vPoint scale)
{
    TransparentBlt(gMemDC, (int)pos.x, (int)pos.y,
        (int)(texture.bitInfo.bmWidth * scale.x), (int)(texture.bitInfo.bmHeight * scale.y),
        texture.hDC, 0, 0, (int)texture.bitInfo.bmWidth, (int)texture.bitInfo.bmHeight, RGB(255, 255, 255));
}