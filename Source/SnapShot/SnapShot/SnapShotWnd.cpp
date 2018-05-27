#include "stdafx.h"
#include "SnapShotWnd.h"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#pragma comment(lib, "msimg32")     // 【AlphaBlend,透明化】
CSnapShotWnd::CSnapShotWnd(void)
{
    IsMove=FALSE;
    m_DesktopDC=NULL;
    m_pDcOldBitmap=NULL;
    m_pGray=NULL;
    
}



CSnapShotWnd::~CSnapShotWnd(void)
{
    Reset();
    if(m_pGray!=NULL)
    {
        DeleteObject(m_pGray);
        m_pGray=NULL;
    }
}
extern HINSTANCE hInst;	
#define  IDM_REMARK_EDIT  201
void CSnapShotWnd::InitWindow(HWND hWnd)
{
    m_hWnd = hWnd;
    
    Reset();

    TRACE("-----------InitWindow------------\n");
   
    InitScreenDC();
    InitGrayBitMap();

    // 【遍历所有子窗口，保存起来】
    m_AllWindowsRect.EnumAllWindows();
    m_RectTracker.SetMousePoint(&m_MousePoint);

}
void CSnapShotWnd::OnLButtonDblClk(POINT point)
{
    TRACE("双击\n");
    //PostQuitMessage(0);
    if (m_RectTracker.m_rect.IsRectNull())
    {        
        MessageBox(NULL,LPCTSTR("需要先选中截图区域"),LPCTSTR("提示"),MB_ICONWARNING);
        return;
    }
    
    long nWidth=m_RectTracker.m_rect.Width();    
    long nHeight=m_RectTracker.m_rect.Height();    
    HDC hdcMem = ::CreateCompatibleDC(m_DesktopDC);
    HBITMAP hBitmap = ::CreateCompatibleBitmap(m_DesktopDC,nWidth,nHeight);
    HGDIOBJ hOldbmp = ::SelectObject(hdcMem, hBitmap);
    if(!::BitBlt(hdcMem, 0,0,nWidth,nHeight, m_DesktopDC,m_RectTracker.m_rect.left, m_RectTracker.m_rect.top,  SRCCOPY))
    {
        DWORD err=GetLastError();
        return;
    }
    //把备注写入截图
    for (auto wnd : m_vEdit)
    {
        HDC edc=GetWindowDC(wnd);
        XRect rc;
        GetWindowRect(wnd,&rc);        
        ::BitBlt(hdcMem, rc.left-m_RectTracker.m_rect.left,rc.top-m_RectTracker.m_rect.top,rc.Width(),rc.Height(), edc, 0,0,  SRCCOPY);                
        ::ReleaseDC(wnd,edc);
    }

    SelectObject(hdcMem,hOldbmp);
    //释放资源
    DeleteDC(hdcMem); 

    //保存位图到文件
    Bitmap *p_bmp = Bitmap::FromHBITMAP(hBitmap, NULL);

    CLSID jpgClsid;
    int result = GetEncoderClsid(L"image/jpeg", &jpgClsid);
    if(result != -1)
    {
        //std::cout << "Encoder succeeded" << std::endl;
    }
    else
    {
        MessageBox(NULL,LPCTSTR("创建jpeg文件失败"),LPCTSTR("警告"),MB_ICONWARNING);
        return;
    }
    p_bmp->Save(L"screen.jpg", &jpgClsid, NULL);
    delete p_bmp;
    DeleteObject(hBitmap);
}
int CSnapShotWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)  
{  
    UINT  num = 0;  
    UINT  size = 0;  
    ImageCodecInfo* pImageCodecInfo = NULL;  
    GetImageEncodersSize(&num, &size);  
    if(size == 0)  
        return -1;  

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));  
    if(pImageCodecInfo == NULL)  
        return -1;  

    GetImageEncoders(num, size, pImageCodecInfo);  
    for(UINT j = 0; j < num; ++j)  
    {  
        if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )  
        {  
            *pClsid = pImageCodecInfo[j].Clsid;  
            free(pImageCodecInfo);  
            return j;  
        }      
    }  
    free(pImageCodecInfo);  
    return -1;  
}  

HWND CSnapShotWnd::CreateRemarkWnd()
{
    TRACE("-----------Create Remark Window------------\n");
    POINT point;
    GetCursorPos(&point);
    HWND hEditWnd= CreateWindow(TEXT("edit"), nullptr,
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL|ES_MULTILINE | ES_WANTRETURN |ES_NOHIDESEL |ES_AUTOVSCROLL,// |ES_AUTOHSCROLL,
            point.x, point.y, 40, 26,
            m_hWnd, (HMENU)IDM_REMARK_EDIT, hInst, NULL
            ); 
    SetWindowText(hEditWnd,NULL);
    ShowWindow(hEditWnd,SW_SHOW);
    SetFocus(hEditWnd);
    return hEditWnd;
}

void CSnapShotWnd::OnLButtonDown(POINT point)
{
    TRACE("按下\n");
    m_LastPoint = point;
    if(!m_RectTracker.m_rect.IsRectNull()){
        //如果存在选取，则移动截图选区
        m_RectTracker.Track(m_hWnd,point);
    }
    //如果window选区不为空，则根据window选区创建截图选区
    if(!m_WindowRect.IsRectNull())
    {
        m_RectTracker.m_rect=m_WindowRect;
        InvalidateRect(m_hWnd,NULL,true);
        UpdateWindow(m_hWnd);
        m_IsCreateWindow = TRUE;
        m_WindowRect.SetRectEmpty();
    }

    //按下标志
    m_IsLButtonDown=TRUE;
}
void CSnapShotWnd::OnRButtonUp(POINT point)
{
    TRACE("------------右键抬起----------\n");
    //m_RectTracker.m_rect.SetRectEmpty();
    //InvalidateRect(m_hWnd,NULL,false);
    //UpdateWindow(m_hWnd);
    m_vEdit.push_back(CreateRemarkWnd());

}
void CSnapShotWnd::OnLButtonUp(POINT point)
{
    TRACE("------------抬起----------\n");
    m_IsLButtonDown = FALSE;
    IsMove = FALSE;
    m_IsCreateWindow=FALSE;
}
void CSnapShotWnd::OnMouseMove(POINT point)
{
    TRACE("-----------OnMouseMove------------\n");
    //判断是否发生移动，如果在移动时发现创建了window选区，则放弃，从新创建截图选区
    if((abs(m_LastPoint.x -point.x) > 1 || abs(m_LastPoint.y -point.y) > 1) && m_IsLButtonDown && m_IsCreateWindow){
        //移动了
        m_RectTracker.TrackRubberBand(m_hWnd,m_LastPoint);
        m_IsCreateWindow =FALSE;        
    }    
    //焦点移回主窗口
    SetFocus(m_hWnd);
    m_LastPoint = point;
    m_MousePoint = point;
    InvalidateRect(m_hWnd,NULL,false);
    UpdateWindow(m_hWnd);
}

void CSnapShotWnd::UpdateRemark( HWND hWnd, int wmEvent )
{
    if(wmEvent==EN_CHANGE)
    {

    }    
    else if(wmEvent==EN_VSCROLL)
    {
        RECT rc;
        GetWindowRect(hWnd,&rc);
        rc.bottom+=20;
        MoveWindow(hWnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    }
    else if (wmEvent==EN_HSCROLL)
    {                
        RECT rc;
        GetWindowRect(hWnd,&rc);
        rc.right+=20;
        MoveWindow(hWnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    }
    InvalidateRect(m_hWnd,NULL,true);
    UpdateWindow(m_hWnd);
}

BOOL CSnapShotWnd::OnSetCursor(HWND pWnd, UINT nHitTest)
{
    if(pWnd!=m_hWnd)
    {
        SetFocus(pWnd);
    }
    if(!m_RectTracker.m_rect.IsRectNull())
    {
        if (m_RectTracker.SetCursor(m_hWnd,nHitTest) )
        {
            return FALSE;
        }
    }
    SetCursor(LoadCursor(NULL,IDC_ARROW));
    return true;
}
void CSnapShotWnd::OnPaint(HDC pDc)
{

    TRACE("-----------OnPaint------------\n");


    //创建与windowDC兼容的内存设备环境  
    HDC bufferDC=CreateCompatibleDC(pDc);     
    //位图的初始化和载入位图     
    HBITMAP bufferBMP=CreateCompatibleBitmap(pDc,m_ScreenWidth,m_ScreenHeight); 
    HBITMAP pOldBitmap = (HBITMAP)SelectObject(bufferDC,bufferBMP);



    //绘制桌面
    BitBlt(bufferDC,0,0,m_ScreenWidth,m_ScreenHeight,m_DesktopDC,0,0,SRCCOPY);
    XRect rect;
    rect.left=0;
    rect.top=0;
    rect.right=100;
    rect.bottom=100;
    //绘制灰色遮罩层
    DrawMask(bufferDC,m_RectTracker.m_rect);
    //绘制橡皮筋
    m_RectTracker.Draw(bufferDC);

    //绘制窗口快速选择的矩形
    DrawAutoWindow(bufferDC,m_MousePoint);
    //画放大镜层
    DrawMagnifier(bufferDC);

    //画edit的外框
    DrawEditWindow(bufferDC);

    //双缓冲技术
    BitBlt(pDc,0,0,m_ScreenWidth,m_ScreenHeight,bufferDC,0,0,SRCCOPY);
    SelectObject(bufferDC,pOldBitmap);
    //释放资源
    DeleteDC(bufferDC); 
    DeleteObject(pOldBitmap); 
    DeleteObject(bufferBMP); 
    //memDC.Ellipse(m_mousePos.x-50,m_mousePos.y-50,m_mousePos.x+50,m_mousePos.y+50);  
}
void CSnapShotWnd::InitScreenDC()
{
    //计算屏幕的宽和高
    m_ScreenWidth  = GetSystemMetrics(SM_CXSCREEN);  
    m_ScreenHeight = GetSystemMetrics(SM_CYSCREEN); 
    //初始化屏幕矩形
    m_ScreenRect.left=0;
    m_ScreenRect.top=0;
    m_ScreenRect.right=m_ScreenWidth;
    m_ScreenRect.bottom=m_ScreenHeight;

    m_RectTracker.m_rectMax=m_ScreenRect;
    //拿到桌面句柄
    //获取桌面窗口句柄
    HWND hwnd = GetDesktopWindow();
    HDC dc =GetWindowDC(hwnd);
    //获取桌面窗口DC
    
    m_DesktopDC =CreateCompatibleDC(dc); 
    m_DesktopBitmap = CreateCompatibleBitmap(dc, m_ScreenWidth, m_ScreenHeight);
    m_pDcOldBitmap=(HBITMAP)SelectObject(m_DesktopDC,m_DesktopBitmap); 
    BitBlt(m_DesktopDC,0, 0, m_ScreenWidth, m_ScreenHeight, dc, 0, 0, SRCCOPY); 
}
void CSnapShotWnd::InitGrayBitMap()
{
    if (NULL == m_pGray)
    {
        //灰色位图
        HDC memdc = CreateCompatibleDC(m_DesktopDC);
        m_pGray = CreateCompatibleBitmap(m_DesktopDC, 1, 1);
        HBITMAP pOld=(HBITMAP)SelectObject(memdc, m_pGray);
        RECT rect;
        rect.left = 0;
        rect.right = 1;
        rect.top = 0;
        rect.bottom = 1;
        HBRUSH brush = CreateSolidBrush(RGB(0,0,0));//黑色
        int res = FillRect(memdc, &rect, brush);

        SelectObject(memdc,pOld);
        DeleteObject(brush);
        DeleteDC(memdc);
    }
}

void CSnapShotWnd::Reset()
{
    IsMove=FALSE;    
    if(m_pDcOldBitmap!=NULL)
    {
        ASSERT(m_DesktopDC!=NULL);
        SelectObject(m_DesktopDC,m_pDcOldBitmap);
        DeleteObject(m_DesktopBitmap);
        DeleteDC(m_DesktopDC);
        m_pDcOldBitmap=NULL;
        m_DesktopBitmap=NULL;        
        m_DesktopDC=NULL;
    }
    for (auto wnd : m_vEdit)
    {
        CloseWindow(wnd);
    }
    m_vEdit.clear();
    m_RectTracker.m_rect.SetRectEmpty();
    
}

//************************************
// 方法名称: DrawAutoWindow
// 创建日期: 2017/03/31
// 创 建 人: admin
// 函数说明: 第一次打开时，根据鼠标位置快速选取窗口
// 函数参数: HDC dc
// 函数参数: POINT point
// 返 回 值: void
//************************************
void CSnapShotWnd::DrawAutoWindow(HDC dc,POINT point)
{
    if( ! m_RectTracker.m_rect.IsRectNull()){
        return ;
    }
    if (!m_AllWindowsRect.GetRect(point, m_WindowRect))   // 【这里判断在没在开始获取到的子窗口中】
    {
        memset((void*)&m_WindowRect, 0, sizeof(m_WindowRect) );
    }
    if(!m_WindowRect.IsRectNull())
    {
        DrawMask(dc,m_WindowRect);

        HPEN pen=CreatePen(PS_SOLID,3,RGB(255,0,0));          // 【这里绘制成红色】
        HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // 【使用画笔画矩形】

        HBRUSH pbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH pOldBrush = (HBRUSH)SelectObject(dc,pbrush);

        Rectangle(dc,m_WindowRect.left,m_WindowRect.top,m_WindowRect.right,m_WindowRect.bottom);//画出矩形

        if(NULL != pOldPen && NULL != pOldBrush)
        {
            SelectObject(dc,pOldBrush);
            SelectObject(dc,pOldPen);
        }
        DeleteObject(pen);
        DeleteObject(pbrush);
    }

}

void CSnapShotWnd::DrawEditWindow( HDC dc )
{
    for(auto wnd : m_vEdit)
    {
        if(GetFocus()==wnd)
        {
            XRect rc;
            GetWindowRect(wnd,&rc);
            HPEN pen=CreatePen(PS_DASHDOT,1,RGB(0,0,0));          // 【这里绘制成黑色】
            HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // 【使用画笔画矩形】

            HBRUSH pbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH pOldBrush = (HBRUSH)SelectObject(dc,pbrush);

            Rectangle(dc,rc.left-1,rc.top-1,rc.right+1,rc.bottom+1);//画出矩形
            if(NULL != pOldPen && NULL != pOldBrush)
            {
                SelectObject(dc,pOldBrush);
                SelectObject(dc,pOldPen);
            }

            DeleteObject(pen);
            DeleteObject(pbrush);            
        }
    }
}

//************************************
// 方法名称: DrawMask
// 创建日期: 2017/03/31
// 创 建 人: admin
// 函数说明: 画灰色遮罩层
// 函数参数: HDC dc
// 函数参数: XRect rect
// 返 回 值: void
//************************************
void CSnapShotWnd::DrawMask(HDC dc,XRect rect)
{
    HDC memdc;
    memdc=CreateCompatibleDC(dc);
    SelectObject(memdc, m_pGray);

    BLENDFUNCTION blend;
    memset( &blend, 0, sizeof( blend) );
    blend.BlendOp= AC_SRC_OVER;
    blend.SourceConstantAlpha= 70; // 透明度 最大255

    if (rect.left == rect.right || rect.top == rect.bottom)
    {
        //【没选择矩形,全屏灰化】 
        AlphaBlend(dc,0,0,m_ScreenWidth,m_ScreenHeight,memdc,0,0,1,1, blend);
    }
    else
    {
        //【除去矩形后，alpha混合其余四个区域都灰化】
        int x1 = rect.left < rect.right  ? rect.left   : rect.right;
        int y1 = rect.top  < rect.bottom ? rect.top    : rect.bottom;
        int x2 = rect.left < rect.right  ? rect.right  : rect.left;
        int y2 = rect.top  < rect.bottom ? rect.bottom : rect.top;

        if (y1 > 0)
        {
            AlphaBlend(dc,0,0,m_ScreenWidth, y1,memdc,0,0,1,1, blend);
        }

        if (y2 < m_ScreenHeight)
        {
            AlphaBlend(dc,0,y2,m_ScreenWidth, m_ScreenHeight-y2,memdc,0,0,1,1, blend);
        }

        if (x1 > 0)
        {
            AlphaBlend(dc,0,y1,x1, y2 - y1,memdc,0,0,1,1, blend);
        }

        if (x2 < m_ScreenWidth)
        {
            AlphaBlend(dc,x2,y1,m_ScreenWidth - x2, y2 - y1,memdc,0,0,1,1, blend);
        }
    }
    //dc->BitBlt(0,0,m_ScreenWidth,m_ScreenHeight,&memdc,0,0,SRCCOPY);
    DeleteDC(memdc);

}

//************************************
// 方法名称: DrawMagnifier
// 创建日期: 2017/03/31
// 创 建 人: admin
// 函数说明: 画放大镜层
// 函数参数: HDC dc
// 函数参数: POINT point
// 返 回 值: void
//************************************
void CSnapShotWnd::DrawMagnifier(HDC dc)
{
    POINT point = m_MousePoint;
    //GetCursorPos(&point);

    m_MagnifierSize =72;
    m_MagnifierRect.SetRect(10,10,90,90);

    m_MagnifierRect.left = m_RectTracker.m_rect.left;
    m_MagnifierRect.top = m_RectTracker.m_rect.top - m_MagnifierSize - 20;
    m_MagnifierRect.right = m_MagnifierRect.left + m_MagnifierSize;
    m_MagnifierRect.bottom = m_MagnifierRect.top + m_MagnifierSize;



    int width = m_MagnifierSize*0.5;
    int height = width;
    int x =point.x - width/2;
    int y =point.y - height/2;

    //1.画背景
    DrawMagnifierBg(dc);
    //2.画文字
    //画当前RGB
    int rgbX=m_MagnifierRect.left+m_MagnifierSize+8;
    int rgbY=m_MagnifierRect.top+6;
    COLORREF pixel=GetPixel(dc,point.x,point.y);
    int r=GetRValue(pixel);
    int g=GetGValue(pixel);
    int b=GetBValue(pixel);
    CString rgbText;
    rgbText.Format(L"当前RGB:(%d,%d,%d)",r,g,b);
    DrawText(dc,rgbX,rgbY,rgbText.GetBuffer(0),10);
    //画当前区域大小
    int areaX=m_MagnifierRect.left+m_MagnifierSize+8;
    int areaY=m_MagnifierRect.top+24;
    int areaWidth=m_RectTracker.m_rect.Width();
    int areaHeight=m_RectTracker.m_rect.Height();
    CString areaText;
    areaText.Format(L"区域大小:(%d,%d)",areaWidth,areaHeight);
    DrawText(dc,areaX,areaY,areaText.GetBuffer(0),10);
    //画提示
    int tipX=m_MagnifierRect.left+m_MagnifierSize+8;
    int tipY=m_MagnifierRect.top+42;
    CString tipText;
    tipText.Format(L"双击可以快速完成截图",areaWidth,areaHeight);
    DrawText(dc,tipX,tipY,tipText.GetBuffer(0),10);



    //3.放大绘制
    StretchBlt(dc, m_MagnifierRect.left,
        m_MagnifierRect.top,   
        m_MagnifierRect.Width(),  
        m_MagnifierRect.Height(),   
        dc,      
        x,
        y,   
        width,   
        height, 
        SRCCOPY);
    //4.接下来画十字图标
    HPEN pen=CreatePen(PS_SOLID,2,RGB(0,0,0));          // 【这里绘制成红色】
    HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // 【使用画笔画矩形】
    //画竖线
    MoveToEx(dc, m_MagnifierRect.left + m_MagnifierSize/2, m_MagnifierRect.top, NULL); 
    LineTo(dc, m_MagnifierRect.left + m_MagnifierSize/2, m_MagnifierRect.bottom);  
    //画横线
    MoveToEx(dc, m_MagnifierRect.left, m_MagnifierRect.top + m_MagnifierRect.Height()/2, NULL); 
    LineTo(dc, m_MagnifierRect.right, m_MagnifierRect.top + m_MagnifierRect.Width()/2); 

    DeleteObject(pen);

}

//************************************
// 方法名称: DrawMagnifierBg
// 创建日期: 2017/03/31
// 创 建 人: admin
// 函数说明: 画放大镜背景
// 函数参数: HDC dc
// 返 回 值: void
//************************************
void CSnapShotWnd::DrawMagnifierBg(HDC dc)
{

    int x =m_MagnifierRect.left - 2;
    int y =m_MagnifierRect.top - 2;
    int width = m_MagnifierSize + 160;
    int height = m_MagnifierSize + 4;

    HDC memdc;
    memdc=CreateCompatibleDC(dc);

    HBITMAP bitmap = CreateCompatibleBitmap(dc, 1, 1);
    SelectObject(memdc, bitmap);
    RECT rect;
    rect.left = 0;
    rect.right = 1;
    rect.top = 0;
    rect.bottom = 1;
    HBRUSH brush = CreateSolidBrush(RGB(0,0,0));//黑色
    int res = FillRect(memdc, &rect, brush);

    SelectObject(memdc,bitmap);

    BLENDFUNCTION blend;
    memset( &blend, 0, sizeof( blend) );
    blend.BlendOp= AC_SRC_OVER;
    blend.SourceConstantAlpha= 120; // 透明度 最大255

    /*HBRUSH pbrush = (HBRUSH)CreateSolidBrush(RGB(255,100,100));
    HBRUSH pOldBrush = (HBRUSH)SelectObject(memdc,pbrush); */

    AlphaBlend(dc,x,y,width,height,memdc,0,0,1,1, blend);

    DeleteObject(bitmap);
    DeleteDC(memdc);
}
void CSnapShotWnd::DrawText(HDC dc,int x,int y,LPCWSTR lpString,int size)
{
    HFONT hFont = CreateFont(-MulDiv(size,GetDeviceCaps(dc,LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL , FALSE, FALSE, FALSE,  
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,  
        DEFAULT_QUALITY, FF_DONTCARE, TEXT("Microsoft YaHei"));  
    HFONT hOldFont = (HFONT)SelectObject(dc,hFont); //把新字体选定为设备环境的当前字体，并返回之前的字体  
    SetTextColor(dc, RGB(255,255,255));   
    SetBkMode(dc, TRANSPARENT); 
    TextOut(dc, x,y,lpString, lstrlen(lpString)); 
}