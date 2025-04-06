#include "framework.h"
// =========================================
// 구조체

struct Texture
{
    HDC hDC;
    HBITMAP hBit;
    BITMAP bitInfo;

    wstring strName;
    wstring strPath;
};
// =========================================


// =========================================
// 전역 변수
static HINSTANCE hInst{};
static WCHAR szTitle[MAX_LOADSTRING]{};
static WCHAR szWindowClass[MAX_LOADSTRING]{};

static HWND g_hWnd{};
static HDC g_hDC{};
static HDC g_memDC{};

static unordered_map<wstring, Texture*> g_mapTexture{};
// =========================================


// ===============================================================
// 함수
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static Texture* LoadTexture(const wstring& name, const wstring& path);
static Texture* FindTexture(const wstring& name);

// ===============================================================



// 윈메인
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY2DGAME, szWindowClass, MAX_LOADSTRING);
   
    {   // 콘솔창 생성
        AllocConsole();

        FILE* fp{};
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        std::ios::sync_with_stdio();
    }

    {   // 윈도우 설정
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
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 2),    // TODO: 배경색 변경 (누끼 확인하려고 바꿈)
            .lpszMenuName = NULL,
            .lpszClassName = szWindowClass,
            .hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL))
        };

        RegisterClassExW(&wcex);
    }

    {   // 화면이 가운데에 생성되도록 조정
        RECT rSize =
        {
            .left = 0,
            .top = 0,
            .right = g_iViewsizeX,
            .bottom = g_iViewsizeY
        };

        AdjustWindowRect(&rSize, WS_OVERLAPPEDWINDOW, false);

        int windowX = rSize.right - rSize.left;
        int windowY = rSize.bottom - rSize.top;

        int sceenX = GetSystemMetrics(SM_CXSCREEN);
        int sceenY = GetSystemMetrics(SM_CYSCREEN);

        int centerX = (sceenX - windowX) / 2;
        int centerY = (sceenY - windowY) / 2;

        g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, centerX, centerY, windowX, windowY, nullptr, nullptr, hInstance, nullptr);

        if (!g_hWnd)
        {
            return FALSE;
        }

        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);
    }

    // 더블 버퍼링 생성
    g_hDC = GetDC(g_hWnd);

    HBITMAP hBitmap = CreateCompatibleBitmap(g_memDC, g_iViewsizeX, g_iViewsizeY);
    g_memDC = CreateCompatibleDC(g_hDC);

    HBITMAP hOldBit = (HBITMAP)SelectObject(g_memDC, hBitmap);
    //DeleteObject(hOldBit);

    {   // 게임의 기본 정보를 초기화
        LoadTexture(L"Player", L"Resource\\ex.bmp");
        LoadTexture(L"Monster", L"Resource\\ex.bmp");
    }

    // 메시지 문
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY2DGAME));
    MSG msg{};

    while (WM_QUIT != msg.message)
    {
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
        else
        {
            // 업데이트
            {

            }

            // 화면을 지움
            Rectangle(g_memDC, -1, -1, g_iViewsizeX + 1, g_iViewsizeY + 1);

            {   // 바닥 그리기

            }

            {   // 플레이어 그리기
                vPoint vPos{ 100.f, 100.f };
                vPoint vScale{ 1.5f, 1.5f };
                DIR_TYPE dDir{ DIR_TYPE::RIGHT };
                
                Texture* pTexture = nullptr;
                pTexture = FindTexture(L"Player");
                assert(pTexture && "Player.bmp 못 찾음");
                
                TransparentBlt(g_hDC, (int)vPos.x, (int)vPos.y,
                    (int)(pTexture->bitInfo.bmWidth * vScale.x), (int)(pTexture->bitInfo.bmHeight * vScale.y),
                    pTexture->hDC, 0, 0, (int)pTexture->bitInfo.bmWidth, (int)pTexture->bitInfo.bmHeight, RGB(255, 255, 255));
            }

            {   // 몬스터 그리기
                vPoint vPos{ 300.f, 300.f };
                vPoint vScale{ 1.f, 1.f };
                DIR_TYPE dDir{ DIR_TYPE::RIGHT };
                
                Texture* pTexture = nullptr;
                pTexture = FindTexture(L"Monster");
                assert(pTexture && "Monster.bmp 못 찾음");
                
                TransparentBlt(g_hDC, (int)vPos.x, (int)vPos.y,
                    (int)(pTexture->bitInfo.bmWidth * vScale.x), (int)(pTexture->bitInfo.bmHeight * vScale.y),
                    pTexture->hDC, 0, 0, (int)pTexture->bitInfo.bmWidth, (int)pTexture->bitInfo.bmHeight, RGB(255, 255, 255));
            }

            // 픽셀을 윈도우로 옮김
            BitBlt(g_hDC, 0, 0, g_iViewsizeX, g_iViewsizeY, g_memDC, 0, 0, SRCCOPY);

            // 프레임 그리기
            {

            }

            // 이벤트 처리하기
            {

            }
        }
    }

    // 선택
    SelectObject(g_memDC, hOldBit); 

    // 해제
    ReleaseDC(g_hWnd, g_hDC);
    DeleteDC(g_memDC);
    DeleteObject(hBitmap);
    

    // 리소스 해제
    for (auto& iter : g_mapTexture)
    {
        Texture* texture = iter.second;

        DeleteDC(texture->hDC);
        DeleteObject(texture->hBit);
    }

    // bmp 삭제
    unordered_map<wstring, Texture*> ::iterator iterTexture = g_mapTexture.begin();

    for (; iterTexture != g_mapTexture.end(); ++iterTexture)
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
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
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

Texture* LoadTexture(const wstring& name, const wstring& path)
{
    Texture* pTexture = nullptr;

    unordered_map<wstring, Texture*>::iterator iterFindTexture = g_mapTexture.find(name);

    if (iterFindTexture != g_mapTexture.end())  // 있는 파일이면
    {
        pTexture = iterFindTexture->second;      // 재사용
    }
    else
    {
        pTexture = new Texture;

        filesystem::path currentPath = filesystem::current_path();
        filesystem::path parentPath = currentPath.parent_path();
        filesystem::path totalPath = parentPath / path;

        pTexture->strName = name;
        pTexture->strPath = totalPath;

        pTexture->hBit = (HBITMAP)LoadImage(nullptr, pTexture->strPath.c_str(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);

        assert(pTexture->hBit);

        // 비트맵과 연결할 DC
        pTexture->hDC = CreateCompatibleDC(g_hDC);

        // 비트맵과 DC연결
        HBITMAP hPrevBit = (HBITMAP)SelectObject(pTexture->hDC, pTexture->hBit);
        DeleteObject(hPrevBit);

        // 비트맵 정보
        GetObject(pTexture->hBit, sizeof(BITMAP), &pTexture->bitInfo);

        assert(pTexture->hBit);

        g_mapTexture.insert(make_pair(pTexture->strName, pTexture));
    }

    return pTexture;
}

Texture* FindTexture(const wstring& name)
{
    unordered_map<wstring, Texture*>::iterator iter = g_mapTexture.find(name);

    if (iter == g_mapTexture.end())
    {
        return nullptr;
    }

    return (*iter).second;
}
