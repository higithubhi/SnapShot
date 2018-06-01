#include "stdafx.h"
#include "SnapShotWnd.h"
#include <GdiPlus.h>
#include "resource.h"


#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#pragma comment(lib, "msimg32")     // ��AlphaBlend,͸������

WNDPROC wpOrigEditProc;
LRESULT APIENTRY EditSubclassProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SETCURSOR:
			TRACE("hwnd=%x,wp=%x,lp=%x\r\n");
			::SetCursor(LoadCursor(NULL, IDC_IBEAM));
			SetFocus(hwnd);
			return TRUE;
			break;
		case WM_LBUTTONDOWN:
		{
			if (::GetCapture() != NULL)
			{
				break;
			}		
			::SetCursor(LoadCursor(NULL, IDC_SIZEALL));
			SetCapture(hwnd);
			break;
		}
		case WM_LBUTTONUP:
		{
			if (GetCapture() == hwnd)
			{
				ReleaseCapture();
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (::GetCapture() == hwnd)
			{

				//int x = LOWORD(lParam);
				//int y = HIWORD(lParam);
				POINT point;
				GetCursorPos(&point);
				XRect rc;
				GetWindowRect(hwnd, &rc);
				rc.right = rc.Width() + point.x;
				rc.left = point.x;
				rc.bottom = rc.Height() + point.y;
				rc.top = point.y;
				MoveWindow(hwnd, point.x, point.y, rc.Width(), rc.Height(), TRUE);
				HWND pHwnd=GetParent(hwnd);
				InvalidateRect(pHwnd, NULL, TRUE);
				UpdateWindow(pHwnd);
				return FALSE;
			}
		}
		break;
		case WM_DESTROY:
			// Remove the subclass from the edit control. 
			SetWindowLong(hwnd, GWL_WNDPROC,
				(LONG)wpOrigEditProc);
			break;
	}		
	return CallWindowProc(wpOrigEditProc, hwnd, uMsg,
		wParam, lParam);
}



CSnapShotWnd::CSnapShotWnd(void)
{
    IsMove=FALSE;
    m_DesktopDC=NULL;
    m_pDcOldBitmap=NULL;
    m_pGray=NULL;
	
	GetObject(GetStockObject(DEVICE_DEFAULT_FONT), sizeof(m_lf), &m_lf);
	m_cf.lStructSize = sizeof(CHOOSEFONT);
	m_cf.hwndOwner = NULL;
	m_cf.hDC = NULL;
	m_cf.lpLogFont = &m_lf;
	m_cf.iPointSize = 0;
	m_cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_SCREENFONTS;
	m_cf.rgbColors = 0;
	m_cf.lCustData = 0;
	m_cf.lpfnHook = NULL;
	m_cf.lpTemplateName = NULL;
	m_cf.hInstance = NULL;
	m_cf.lpszStyle = NULL;
	m_cf.nFontType = 0;
	m_cf.nSizeMin = 0;
	m_cf.nSizeMax = 0;

	static COLORREF acrCustClr[16];
	ZeroMemory(&m_color, sizeof(m_color));
	m_color.lStructSize = sizeof(CHOOSECOLOR);
	m_color.hwndOwner = NULL;
	m_color.lpCustColors = (LPDWORD)acrCustClr;
	m_color.rgbResult = RGB(0,0,0);
	m_color.Flags = CC_FULLOPEN | CC_RGBINIT;

	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
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

    // �����������Ӵ��ڣ�����������
    m_AllWindowsRect.EnumAllWindows();
    m_RectTracker.SetMousePoint(&m_MousePoint);

}

void CSnapShotWnd::OnLButtonDblClk(POINT point)
{
    TRACE("˫��\n");
    //PostQuitMessage(0);
    if (m_RectTracker.m_rect.IsRectNull())
    {        
        MessageBox(m_hWnd,TEXT("��Ҫ��ѡ�н�ͼ����"),TEXT("��ʾ"),MB_ICONWARNING);
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
    //�ѱ�עд���ͼ
    for (auto wnd : m_vEdit)
    {
        HDC edc=GetWindowDC(wnd);
        XRect rc;
        GetWindowRect(wnd,&rc);        
        ::BitBlt(hdcMem, rc.left-m_RectTracker.m_rect.left,rc.top-m_RectTracker.m_rect.top,rc.Width(),rc.Height(), edc, 0,0,  SRCCOPY);                
        ::ReleaseDC(wnd,edc);
    }

    SelectObject(hdcMem,hOldbmp);
    //�ͷ���Դ
    DeleteDC(hdcMem); 

	//���ļ�����Ի���
	TCHAR filename[1024] = {0};
	OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };//or  {sizeof (OPENFILENAME)}      
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = TEXT("JPEG\0 * .jpg\0All\0 * .*\0"); 
	//������ ���Ϊ NULL ��ʹ�ù�����    	
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = TEXT("�����ļ�");
	ofn.lpstrInitialDir = szCurDir;//���öԻ�����ʾ�ĳ�ʼĿ¼    
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	BOOL bOk = GetSaveFileName(&ofn);	
	if (!bOk)
	{		
		if (MessageBox(m_hWnd,TEXT("ȷ����������"), TEXT("��ʾ"), MB_YESNO) == IDYES)
		{
			return;
		}
	}
	StrCpyN(szCurDir, filename, ofn.nFileOffset);
	if (ofn.nFileExtension == 0)
	{
		StrCat(filename, TEXT(".jpg"));
	}
    //����λͼ���ļ�
    Bitmap *p_bmp = Bitmap::FromHBITMAP(hBitmap, NULL);

    CLSID jpgClsid;
    int result = GetEncoderClsid(TEXT("image/jpeg"), &jpgClsid);
    if(result != -1)
    {
        //std::cout << "Encoder succeeded" << std::endl;
    }
    else
    {
        MessageBox(NULL,TEXT("����jpeg�ļ�ʧ��"),TEXT("����"),MB_ICONWARNING);
        return;
    }
	Gdiplus::EncoderParameters eps;
	int lQuality = 100;
	eps.Count = 1;
	eps.Parameter[0].Guid = Gdiplus::EncoderQuality;
	eps.Parameter[0].NumberOfValues = 1;
	eps.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
	eps.Parameter[0].Value = &lQuality;
    p_bmp->Save(filename, &jpgClsid, &eps);
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
	int width = 50; m_cf.lpLogFont->lfWidth;
	int height = 50; m_cf.lpLogFont->lfHeight;
	//Ĭ��5���ַ���С��2��
    HWND hEditWnd= CreateWindow(TEXT("edit"), nullptr,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN |ES_NOHIDESEL | ES_AUTOVSCROLL |ES_AUTOHSCROLL | CS_OWNDC,
            point.x-width, point.y-height, width*5, height*2,
            m_hWnd, (HMENU)IDM_REMARK_EDIT, hInst, NULL
            ); 	
	HFONT hFont = CreateFontIndirect(m_cf.lpLogFont);	
	::SendMessage(hEditWnd, WM_SETFONT, (WPARAM)hFont, TRUE);	
	SetWindowLong(hEditWnd, GWL_USERDATA, m_color.rgbResult);

	wpOrigEditProc = (WNDPROC)SetWindowLong(hEditWnd,
		GWL_WNDPROC, (LONG)EditSubclassProc);
    SetWindowText(hEditWnd,NULL);
    ShowWindow(hEditWnd,SW_SHOW);

    SetFocus(hEditWnd);
	m_vEdit.push_back(hEditWnd);
	UpdateRemark(hEditWnd, EN_CHANGE);
    return hEditWnd;
}

void CSnapShotWnd::SelectFont()
{
	m_cf.hwndOwner = m_hWnd;
	if (ChooseFont(&m_cf))
	{
		m_color.rgbResult = m_cf.rgbColors;
	}
}


void CSnapShotWnd::SelectColor()
{
	m_color.hwndOwner = m_hWnd;
	ChooseColor(&m_color);
}

void CSnapShotWnd::OnLButtonDown(POINT point)
{
    TRACE("����\n");
    m_LastPoint = point;
    if(!m_RectTracker.m_rect.IsRectNull()){
        //�������ѡȡ�����ƶ���ͼѡ��
        m_RectTracker.Track(m_hWnd,point);
    }
    //���windowѡ����Ϊ�գ������windowѡ��������ͼѡ��
    if(!m_WindowRect.IsRectNull())
    {
        m_RectTracker.m_rect=m_WindowRect;
        InvalidateRect(m_hWnd,NULL,true);
        UpdateWindow(m_hWnd);
        m_IsCreateWindow = TRUE;
        m_WindowRect.SetRectEmpty();
    }

    //���±�־
    m_IsLButtonDown=TRUE;
}
void CSnapShotWnd::OnRButtonUp(POINT point)
{
    TRACE("------------�Ҽ�̧��----------\n");
    //m_RectTracker.m_rect.SetRectEmpty();
    //InvalidateRect(m_hWnd,NULL,false);
    //UpdateWindow(m_hWnd);
    
	ShowPopupMenu();
}
void CSnapShotWnd::OnLButtonUp(POINT point)
{
    TRACE("------------̧��----------\n");
    m_IsLButtonDown = FALSE;
    IsMove = FALSE;
    m_IsCreateWindow=FALSE;
}
void CSnapShotWnd::OnMouseMove(POINT point)
{
    TRACE("-----------OnMouseMove------------\n");
    //�ж��Ƿ����ƶ���������ƶ�ʱ���ִ�����windowѡ��������������´�����ͼѡ��
    if((abs(m_LastPoint.x -point.x) > 1 || abs(m_LastPoint.y -point.y) > 1) && m_IsLButtonDown && m_IsCreateWindow){
        //�ƶ���
        m_RectTracker.TrackRubberBand(m_hWnd,m_LastPoint);
        m_IsCreateWindow =FALSE;        
    }    
    //�����ƻ�������
	/*if (GetFocus() != m_hWnd)
	{
		SetFocus(m_hWnd);
	}*/
    m_LastPoint = point;
    m_MousePoint = point;
    InvalidateRect(m_hWnd,NULL,false);
    UpdateWindow(m_hWnd);
}

void CSnapShotWnd::UpdateRemark( HWND hWnd, int wmEvent )
{
    if(wmEvent==EN_CHANGE)
    {	
		TCHAR buf[1024] = { TCHAR('8') };
		HDC hdc = GetWindowDC(hWnd);	
		SIZE sz;
		HFONT hFont =(HFONT) ::SendMessage(hWnd, WM_GETFONT, 0, 0);
		HFONT hOld=(HFONT)SelectObject(hdc, hFont);
		GetTextExtentPoint32(hdc, buf, 1, &sz);		
		SelectObject(hdc, hOld);		
		ReleaseDC(hWnd,hdc);
		int n = GetWindowText(hWnd, buf, 1024);		
		int row=2;
		int nMaxRow = 5;		
		LPTSTR pLineHead=buf, pLineEnd=buf,pMaxLine=buf;
		do 
		{
			pLineEnd = StrChr(pLineHead, TCHAR('\n'));
			if (pLineEnd != NULL)
			{
				row++;
				if (pLineEnd - pLineHead > nMaxRow)
				{
					nMaxRow = pLineEnd - pLineHead+1;
					pMaxLine = pLineHead;
				}
				pLineHead = pLineEnd + 1;
			}
			else
			{
				pLineEnd = buf + n;
				if (pLineEnd - pLineHead > nMaxRow)
				{
					nMaxRow = pLineEnd - pLineHead+1;
					pMaxLine = pLineHead;
				}
			}
		} while (pLineEnd<buf+n);
		RECT rc;
		GetWindowRect(hWnd, &rc);			
		rc.right = rc.left + sz.cx*nMaxRow;
		rc.bottom = rc.top + sz.cy*row+3;

		MoveWindow(hWnd,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    } 
    InvalidateRect(m_hWnd,NULL,true);
    UpdateWindow(m_hWnd);
}

BOOL CSnapShotWnd::OnSetCursor(HWND pWnd, UINT nHitTest)
{
	SetFocus(pWnd);

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


    //������windowDC���ݵ��ڴ��豸����  
    HDC bufferDC=CreateCompatibleDC(pDc);     
    //λͼ�ĳ�ʼ��������λͼ     
    HBITMAP bufferBMP=CreateCompatibleBitmap(pDc,m_ScreenWidth,m_ScreenHeight); 
    HBITMAP pOldBitmap = (HBITMAP)SelectObject(bufferDC,bufferBMP);



    //��������
    BitBlt(bufferDC,0,0,m_ScreenWidth,m_ScreenHeight,m_DesktopDC,0,0,SRCCOPY);
    XRect rect;
    rect.left=0;
    rect.top=0;
    rect.right=100;
    rect.bottom=100;
    //���ƻ�ɫ���ֲ�
    DrawMask(bufferDC,m_RectTracker.m_rect);
    //������Ƥ��
    m_RectTracker.Draw(bufferDC);

    //���ƴ��ڿ���ѡ��ľ���
    DrawAutoWindow(bufferDC,m_MousePoint);
    //���Ŵ󾵲�
    DrawMagnifier(bufferDC);

    //��edit�����
    DrawEditWindow(bufferDC);

    //˫���弼��
    BitBlt(pDc,0,0,m_ScreenWidth,m_ScreenHeight,bufferDC,0,0,SRCCOPY);
    SelectObject(bufferDC,pOldBitmap);
    //�ͷ���Դ
    DeleteDC(bufferDC); 
    DeleteObject(pOldBitmap); 
    DeleteObject(bufferBMP); 
    //memDC.Ellipse(m_mousePos.x-50,m_mousePos.y-50,m_mousePos.x+50,m_mousePos.y+50);  
}
void CSnapShotWnd::InitScreenDC()
{
    //������Ļ�Ŀ�͸�
    m_ScreenWidth  = GetSystemMetrics(SM_CXSCREEN);  
    m_ScreenHeight = GetSystemMetrics(SM_CYSCREEN); 
    //��ʼ����Ļ����
    m_ScreenRect.left=0;
    m_ScreenRect.top=0;
    m_ScreenRect.right=m_ScreenWidth;
    m_ScreenRect.bottom=m_ScreenHeight;

    m_RectTracker.m_rectMax=m_ScreenRect;
    //�õ�������
    //��ȡ���洰�ھ��
    HWND hwnd = GetDesktopWindow();
    HDC dc =GetWindowDC(hwnd);
    //��ȡ���洰��DC
    
    m_DesktopDC =CreateCompatibleDC(dc); 
    m_DesktopBitmap = CreateCompatibleBitmap(dc, m_ScreenWidth, m_ScreenHeight);
    m_pDcOldBitmap=(HBITMAP)SelectObject(m_DesktopDC,m_DesktopBitmap); 
    BitBlt(m_DesktopDC,0, 0, m_ScreenWidth, m_ScreenHeight, dc, 0, 0, SRCCOPY); 
}
void CSnapShotWnd::InitGrayBitMap()
{
    if (NULL == m_pGray)
    {
        //��ɫλͼ
        HDC memdc = CreateCompatibleDC(m_DesktopDC);
        m_pGray = CreateCompatibleBitmap(m_DesktopDC, 1, 1);
        HBITMAP pOld=(HBITMAP)SelectObject(memdc, m_pGray);
        RECT rect;
        rect.left = 0;
        rect.right = 1;
        rect.top = 0;
        rect.bottom = 1;
        HBRUSH brush = CreateSolidBrush(RGB(0,0,0));//��ɫ
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


void CSnapShotWnd::ShowPopupMenu()
{
	POINT point;
	GetCursorPos(&point);

	HMENU hMenu = CreatePopupMenu();  //��������ʽ�˵�  
	HMENU hSrcMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_SNAPSHOT)); //���ز˵���Դ  
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSrcMenu, _T("Popup")); //���ӵ������˵�  
	HMENU hTackMenu = GetSubMenu(hMenu, 0); //ȡ��Ŀ��˵�  
	TrackPopupMenuEx(GetSubMenu(hTackMenu,0), TPM_LEFTALIGN, point.x, point.y, m_hWnd, NULL); //�����˵�  
	DestroyMenu(hSrcMenu); //���ټ��صĲ˵�  
	DestroyMenu(hMenu); //���ٵ����˵�  
}

//************************************
// ��������: DrawAutoWindow
// ��������: 2017/03/31
// �� �� ��: admin
// ����˵��: ��һ�δ�ʱ���������λ�ÿ���ѡȡ����
// ��������: HDC dc
// ��������: POINT point
// �� �� ֵ: void
//************************************
void CSnapShotWnd::DrawAutoWindow(HDC dc,POINT point)
{
    if( ! m_RectTracker.m_rect.IsRectNull()){
        return ;
    }
    if (!m_AllWindowsRect.GetRect(point, m_WindowRect))   // �������ж���û�ڿ�ʼ��ȡ�����Ӵ����С�
    {
        memset((void*)&m_WindowRect, 0, sizeof(m_WindowRect) );
    }
    if(!m_WindowRect.IsRectNull())
    {
        DrawMask(dc,m_WindowRect);

        HPEN pen=CreatePen(PS_SOLID,3,RGB(255,0,0));          // ��������Ƴɺ�ɫ��
        HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // ��ʹ�û��ʻ����Ρ�

        HBRUSH pbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH pOldBrush = (HBRUSH)SelectObject(dc,pbrush);

        Rectangle(dc,m_WindowRect.left,m_WindowRect.top,m_WindowRect.right,m_WindowRect.bottom);//��������

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
            HPEN pen=CreatePen(PS_DASH,1,RGB(255,255,255));          // ��������Ƴɺ�ɫ��
            HPEN blkpen=CreatePen(PS_DASHDOT,1,RGB(0,0,0));          // ��������Ƴɺ�ɫ��

            HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // ��ʹ�û��ʻ����Ρ�

            HBRUSH pbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH pOldBrush = (HBRUSH)SelectObject(dc,pbrush);

            Rectangle(dc,rc.left-1,rc.top-1,rc.right+1,rc.bottom+1);//��������
            SelectObject(dc,blkpen);      // ��ʹ�û��ʻ����Ρ�
            Rectangle(dc,rc.left-1,rc.top-1,rc.right+1,rc.bottom+1);//��������

            if(NULL != pOldPen && NULL != pOldBrush)
            {
                SelectObject(dc,pOldBrush);
                SelectObject(dc,pOldPen);
            }

            DeleteObject(pen);
            DeleteObject(blkpen);
            DeleteObject(pbrush);            
        }
    }
}

//************************************
// ��������: DrawMask
// ��������: 2017/03/31
// �� �� ��: admin
// ����˵��: ����ɫ���ֲ�
// ��������: HDC dc
// ��������: XRect rect
// �� �� ֵ: void
//************************************
void CSnapShotWnd::DrawMask(HDC dc,XRect rect)
{
    HDC memdc;
    memdc=CreateCompatibleDC(dc);
    SelectObject(memdc, m_pGray);

    BLENDFUNCTION blend;
    memset( &blend, 0, sizeof( blend) );
    blend.BlendOp= AC_SRC_OVER;
    blend.SourceConstantAlpha= 70; // ͸���� ���255

    if (rect.left == rect.right || rect.top == rect.bottom)
    {
        //��ûѡ�����,ȫ���һ��� 
        AlphaBlend(dc,0,0,m_ScreenWidth,m_ScreenHeight,memdc,0,0,1,1, blend);
    }
    else
    {
        //����ȥ���κ�alpha��������ĸ����򶼻һ���
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
// ��������: DrawMagnifier
// ��������: 2017/03/31
// �� �� ��: admin
// ����˵��: ���Ŵ󾵲�
// ��������: HDC dc
// ��������: POINT point
// �� �� ֵ: void
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

    //1.������
    DrawMagnifierBg(dc);
    //2.������
    //����ǰRGB
    int rgbX=m_MagnifierRect.left+m_MagnifierSize+8;
    int rgbY=m_MagnifierRect.top+6;
    COLORREF pixel=GetPixel(dc,point.x,point.y);
    int r=GetRValue(pixel);
    int g=GetGValue(pixel);
    int b=GetBValue(pixel);
    CString rgbText;
    rgbText.Format(TEXT("��ǰRGB:(%d,%d,%d)"),r,g,b);
    DrawText(dc,rgbX,rgbY,rgbText.GetBuffer(0),10);
    //����ǰ�����С
    int areaX=m_MagnifierRect.left+m_MagnifierSize+8;
    int areaY=m_MagnifierRect.top+24;
    int areaWidth=m_RectTracker.m_rect.Width();
    int areaHeight=m_RectTracker.m_rect.Height();
    CString areaText;
    areaText.Format(TEXT("�����С:(%d,%d)"),areaWidth,areaHeight);
    DrawText(dc,areaX,areaY,areaText.GetBuffer(0),10);
    //����ʾ
    int tipX=m_MagnifierRect.left+m_MagnifierSize+8;
    int tipY=m_MagnifierRect.top+42;
    CString tipText;
    tipText.Format(TEXT("˫�����Կ�����ɽ�ͼ"),areaWidth,areaHeight);
    DrawText(dc,tipX,tipY,tipText.GetBuffer(0),10);



    //3.�Ŵ����
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
    //4.��������ʮ��ͼ��
    HPEN pen=CreatePen(PS_SOLID,2,RGB(0,0,0));          // ��������Ƴɺ�ɫ��
    HPEN pOldPen = (HPEN)SelectObject(dc,pen);      // ��ʹ�û��ʻ����Ρ�
    //������
    MoveToEx(dc, m_MagnifierRect.left + m_MagnifierSize/2, m_MagnifierRect.top, NULL); 
    LineTo(dc, m_MagnifierRect.left + m_MagnifierSize/2, m_MagnifierRect.bottom);  
    //������
    MoveToEx(dc, m_MagnifierRect.left, m_MagnifierRect.top + m_MagnifierRect.Height()/2, NULL); 
    LineTo(dc, m_MagnifierRect.right, m_MagnifierRect.top + m_MagnifierRect.Width()/2); 

    DeleteObject(pen);

}

//************************************
// ��������: DrawMagnifierBg
// ��������: 2017/03/31
// �� �� ��: admin
// ����˵��: ���Ŵ󾵱���
// ��������: HDC dc
// �� �� ֵ: void
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
    HBRUSH brush = CreateSolidBrush(RGB(0,0,0));//��ɫ
    int res = FillRect(memdc, &rect, brush);

    SelectObject(memdc,bitmap);

    BLENDFUNCTION blend;
    memset( &blend, 0, sizeof( blend) );
    blend.BlendOp= AC_SRC_OVER;
    blend.SourceConstantAlpha= 120; // ͸���� ���255

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
    HFONT hOldFont = (HFONT)SelectObject(dc,hFont); //��������ѡ��Ϊ�豸�����ĵ�ǰ���壬������֮ǰ������  
    SetTextColor(dc, RGB(255,255,255));   
    SetBkMode(dc, TRANSPARENT); 
    TextOut(dc, x,y,lpString, lstrlen(lpString)); 
}