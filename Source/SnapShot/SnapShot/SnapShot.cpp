// SnapShot.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "SnapShot.h"
#include <GdiPlus.h>
#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
HWND g_hWnd;
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR    lpCmdLine,
                       _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput; 

    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MSG msg;
    HACCEL hAccelTable;

    // 初始化全局字符串
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SNAPSHOT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    ATOM  _atom=GlobalAddAtom(L"coder");
    RegisterHotKey(g_hWnd,_atom,MOD_CONTROL,0x51);//ctrl+Q

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SNAPSHOT));

    // 主消息循环:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterHotKey(g_hWnd,_atom);
    GlobalDeleteAtom(_atom);
    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNAPSHOT));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= NULL;
    //wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SNAPSHOT);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // 将实例句柄存储在全局变量中

    hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP|WS_MAXIMIZEBOX,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }
    g_hWnd=hWnd;
    



    ////初始化窗体
    //m_SnapShotWnd.InitWindow(hWnd);


    //ShowWindow(hWnd,SW_SHOWMAXIMIZED);
    //UpdateWindow(hWnd);

    return TRUE;
}
//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
#define IDM_REMARK_EDIT                 201
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    POINT point;  
    UINT nHitTest;
    switch (message)
    {
    case  WM_HOTKEY:
        {
            m_SnapShotWnd.InitWindow(g_hWnd);
            ShowWindow(hWnd,SW_SHOWMAXIMIZED);
            UpdateWindow(hWnd);
        }
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
            //屏蔽Esc消息
        case VK_ESCAPE:
            //PostQuitMessage(0);
            ShowWindow(hWnd,SW_HIDE);
            UpdateWindow(hWnd);
            return true;
            break;
        }
        break;

    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_REMARK_EDIT:      
            TRACE("edit event=%d",wmEvent);
            if(wmEvent==EN_CHANGE)
            {
                m_SnapShotWnd.UpdateRemark();
            }            
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此添加任意绘图代码...

        m_SnapShotWnd.OnPaint(hdc);

        EndPaint(hWnd, &ps);
        break;
    case WM_LBUTTONDBLCLK:
        TRACE("WM_LBUTTONDBLCLK\n");

        point.x=LOWORD(lParam);  
        point.y=HIWORD(lParam);  
        m_SnapShotWnd.OnLButtonDblClk(point);
        break;
    case WM_LBUTTONDOWN:  
        point.x=LOWORD(lParam);  
        point.y=HIWORD(lParam);  
        m_SnapShotWnd.OnLButtonDown(point);
        break;
    case WM_RBUTTONUP:
        point.x=LOWORD(lParam);  
        point.y=HIWORD(lParam);  
        m_SnapShotWnd.OnRButtonUp(point);
        break;
    case EN_CHANGE:
        {
            int n=0;
        }
        break;
    case WM_MOUSEMOVE:  
        point.x=LOWORD(lParam);  
        point.y=HIWORD(lParam);  
        m_SnapShotWnd.OnMouseMove(point);
        break;
    case WM_LBUTTONUP:  
        point.x=LOWORD(lParam);  
        point.y=HIWORD(lParam);  
        m_SnapShotWnd.OnLButtonUp(point);
        break;
    case WM_SETCURSOR:
        nHitTest=LOWORD(lParam);
        m_SnapShotWnd.OnSetCursor(hWnd,nHitTest);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

