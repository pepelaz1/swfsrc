#include "StdAfx.h"

CComModule _Module;
const char *g_flash_clsid_c="{D27CDB6E-AE6D-11CF-96B8-444553540000}";

CContext::CContext(CSwfBmp *swfbmp, HANDLE hPipe):
	m_swfbmp(swfbmp),
	m_hPipe(hPipe),
	m_axwnd(0),
	m_flash(0),
	m_viewobject(0),
	m_hdccomp(0),
	m_bitmap(0),
	m_bmp_buff(0),
	m_size(0)
{
}

CContext::~CContext()
{
	DeleteAxWnd();
}

CSwfBmp *CContext::SwfBmp()
{
	return m_swfbmp;
}

HANDLE CContext::Pipe()
{
	return m_hPipe;
}

void CContext::Init(string filename, int width, int height)
{
	AtlAxWinInit();

	
	if (m_axwnd == NULL)
		m_axwnd = new CAxWindow();
	if (m_axwnd->IsWindow())
		m_axwnd->DestroyWindow();

	RECT rc = {0, 0, width + 7, height + 28};
	AdjustWindowRect(&rc, 0, FALSE);
	m_axwnd->Create(NULL, &rc, g_flash_clsid_c, 0);
	//HWND hwnd = m_axwnd->Create(NULL, &rc, g_flash_clsid_c, 0);
	//RECT client;
	//GetWindowRect(hwnd,&client);

			
	m_axwnd->QueryControl(&m_flash);	
	_bstr_t bstrf(filename.c_str());
	m_flash->LoadMovie(0,bstrf);
	//m_flash->StopPlay();
	//m_flash->put_Playing(0);
	m_flash->Play();
	m_flash.QueryInterface(&m_viewobject);

	m_rectl.left = 0;
	m_rectl.right = width;
	m_rectl.top = 0;
	m_rectl.bottom = height;

	m_size =  width * height * 4;

	ReleaseGDI();

	HDC desktop_dc = GetDC(GetDesktopWindow());
	m_hdccomp = CreateCompatibleDC(desktop_dc);	

	BITMAPINFO binfo;
	memset(&binfo,0, sizeof(binfo));
	binfo.bmiHeader.biSize = sizeof(binfo);
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biBitCount = 32;
	binfo.bmiHeader.biCompression = BI_RGB;
	binfo.bmiHeader.biHeight = m_rectl.bottom;
	binfo.bmiHeader.biWidth = m_rectl.right;
	binfo.bmiHeader.biSizeImage  = m_size;
	binfo.bmiHeader.biClrImportant = 0;
	binfo.bmiHeader.biClrUsed = 0;
	binfo.bmiHeader.biXPelsPerMeter = 0;
	binfo.bmiHeader.biYPelsPerMeter = 0;
	
	m_bitmap = CreateDIBSection(m_hdccomp, &binfo, DIB_RGB_COLORS, (void **)&m_bmp_buff, NULL, 0);
	SelectObject(m_hdccomp, m_bitmap);

	SetMapMode(m_hdccomp, MM_TEXT);
}

void CContext::DeleteAxWnd()
{
	if (m_axwnd)
	{
		m_axwnd->DestroyWindow();
		delete m_axwnd;
	}	
}

void CContext::ReleaseGDI()
{
	if (m_bitmap)
		DeleteObject(m_bitmap);
	
	if (m_hdccomp)
		DeleteDC(m_hdccomp);
}


void CContext::ReleaseInterfaces()
{
	if (m_flash)
		m_flash.Release();

	if (m_viewobject)
		m_viewobject.Release();
}

HRESULT CContext::GetBmp(int num)
{
	//HRESULT hr = m_flash->put_FrameNum(num);
	m_flash->GotoFrame(num);


	LONG frm;
	m_flash->CurrentFrame(&frm);
	

	m_viewobject->Draw(DVASPECT_CONTENT, -1, NULL, NULL,
		NULL, m_hdccomp, &m_rectl, &m_rectl, NULL, NULL);
			
	return frm;
}

unsigned char *CContext::GetBuffer()
{
	return m_bmp_buff;
}
int  CContext::GetSize()
{
	return m_size;
}