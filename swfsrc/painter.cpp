#include <windows.h>
#include "painter.h"
#include <streams.h>
#include "logger.h"
const char *g_flash_clsid_c="{D27CDB6E-AE6D-11CF-96B8-444553540000}";

//CComModule _Module;
extern CLogger g_log;


CSwfPainter::CSwfPainter() :
	m_file(""),
	m_width(0),
	m_height(0),
//	m_axwnd(NULL),
//	m_flash(0),
//	m_viewobject(0),
//	m_bitmap(0),
	m_bmp_buff(NULL),
//	m_hdccomp(0),
	m_frameno(0),
	m_framecnt(0),
	m_hPipe(0)
//	m_g(NULL),
//	m_bmp(NULL)
{
	CoInitialize(NULL);
//	GdiplusStartupInput gdiplusStartupInput; 
//GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}
	
CSwfPainter::~CSwfPainter()
{
	Reset();
}

//void CSwfPainter::DeleteAxWnd()
//{
//	if (m_axwnd)
//	{
//		m_axwnd->DestroyWindow();
//		delete m_axwnd;
//	}	
//}

void CSwfPainter::Reset()
{
	//ReleaseInterfaces();
	//ReleaseGDI();

/*	if (m_g)
	{
		delete m_g;
		m_g = 0;
	}
	if (m_bmp)
	{
		delete m_bmp;
		m_bmp = 0;
	}	
	GdiplusShutdown(m_gdiplusToken);*/
	if (m_bmp_buff)
		delete [] m_bmp_buff;
	CloseBmpSrv();
	DisconnectNamedPipe(m_hPipe);
	CloseHandle(m_hPipe); 
}

//void CSwfPainter::ReleaseInterfaces()
//{
//	if (m_flash)
//		m_flash.Release();
//
//	if (m_viewobject)
//		m_viewobject.Release();
//}

//void CSwfPainter::ReleaseGDI()
//{
//	if (m_bitmap)
//		DeleteObject(m_bitmap);
//	
//	if (m_hdccomp)
//		DeleteDC(m_hdccomp);
//}

HRESULT CSwfPainter::LoadSwf()
{
	HRESULT hr = S_OK;
	Reset();
	string pipename = "\\\\.\\pipe\\swfsrc";

	while(true)
	{
		m_hPipe = CreateFile( 
			pipename.c_str(),			// Pipe name 
			GENERIC_READ |			// Read and write access 
			GENERIC_WRITE,
			0,						// No sharing 
			NULL,					// Default security attributes
			OPEN_EXISTING,			// Opens existing pipe 
			0,						// Default attributes 
			NULL);					// No template file 
		// Break if the pipe handle is valid. 
		if (m_hPipe == INVALID_HANDLE_VALUE) 
		{
			DWORD err = GetLastError();
			if (ERROR_PIPE_BUSY == GetLastError())
			{
				if (!WaitNamedPipe(pipename.c_str(),500))
					return E_FAIL;
			}
			else
				return E_FAIL; 
		}
		else
			break;
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL b = SetNamedPipeHandleState(m_hPipe, &dwMode, NULL, NULL);
	if (!b) 
		return E_FAIL;

	unsigned char buff[256];
	unsigned char *p = buff;
	*p = 1;  // CMD_LOAD
	p++;
	*p = m_file.size();
	p++;
	memcpy(p, m_file.data(), m_file.size());
	p += m_file.size();
	int *ip = (int *)p;
	*ip = m_width;
	p += 4;
	ip = (int *)p;
	*ip = m_height;
	DWORD wrt;
	WriteFile(m_hPipe, buff, m_file.size()+10, &wrt, NULL);

	unsigned char response[1];
	DWORD rd;
	ReadFile(m_hPipe, response, 1, &rd, NULL);
	// response == RESP_OK


	/*_bstr_t bstrf(m_file.c_str());
	hr = m_flash->LoadMovie(0,bstrf);
	if (FAILED(hr))
	return hr;

	m_flash->StopPlay();
	m_flash->put_Playing(0);

	hr = m_flash.QueryInterface(&m_viewobject);
	if (FAILED(hr))
	return hr;*/
	return hr;
}

void CSwfPainter::CloseBmpSrv()
{
	unsigned char request[1];
	request[0] = 3; // CMD_EXIT
	DWORD wrt;
	WriteFile(m_hPipe, request, 1, &wrt, NULL);
}


//HRESULT CSwfPainter::Reload()
//{
//	HRESULT hr = S_OK;
			
	//ReleaseInterfaces();
	//ReleaseGDI();

	//hr = m_axwnd->QueryControl(&m_flash);
	//if (FAILED(hr))
	//	return hr;

	
	//try
	//{
	//hr = LoadSwf();
	//if (FAILED(hr))
	//	return hr;
	//	
	////}
	//catch(...)
	//{
	//}

//	m_rectl.left = 0;
//	m_rectl.right = m_width;
//	m_rectl.top = 0;
//	m_rectl.bottom = m_height;
//
//	ReleaseGDI();
//		
//	HDC desktop_dc = GetDC(GetDesktopWindow());
//	m_hdccomp = CreateCompatibleDC(desktop_dc);	
//
//	BITMAPINFO binfo;
//	memset(&binfo,0, sizeof(binfo));
//	binfo.bmiHeader.biSize = sizeof(binfo);
//	binfo.bmiHeader.biPlanes = 1;
//	binfo.bmiHeader.biBitCount = 32;
//	binfo.bmiHeader.biCompression = BI_RGB;
//	binfo.bmiHeader.biHeight = m_rectl.bottom;
//	binfo.bmiHeader.biWidth = m_rectl.right;
//	binfo.bmiHeader.biSizeImage  = GetBitmapSize(&binfo.bmiHeader);
//    binfo.bmiHeader.biClrImportant = 0;
//	binfo.bmiHeader.biClrUsed = 0;
//	binfo.bmiHeader.biXPelsPerMeter = 0;
//	binfo.bmiHeader.biYPelsPerMeter = 0;
//
////	m_bmp = new Bitmap(binfo.bmiHeader.biWidth, binfo.bmiHeader.biHeight, PixelFormat32bppRGB);
////	m_g  = new Graphics(m_bmp);
//	
//	m_bitmap = CreateDIBSection(m_hdccomp, &binfo, DIB_RGB_COLORS, (void **)&m_bmp_buff, NULL, 0);
//	//m_bitmap = CreateCompatibleBitmap(desktop_dc,m_rectl.right,m_rectl.bottom);
//	SelectObject(m_hdccomp, m_bitmap);
//
//	// vietdoor's code start here
//	SetMapMode(m_hdccomp, MM_TEXT);	
//	return hr;
//}

HRESULT CSwfPainter::Init(string file, int width, int height, int framecnt)
{
	HRESULT hr = S_OK;

	m_file = file;
	m_width = width;
	m_height = height;
	m_framecnt = framecnt;
	
	hr = LoadSwf();
	if (FAILED(hr))
		return hr;

	return hr;
}

int CSwfPainter::FrameCount()
{
	return m_framecnt;
}


int CSwfPainter::CurrentFrameNo()
{
	CAutoLock lck(&m_lock);
	return m_frameno;
}

HRESULT CSwfPainter::OnPlay()
{
	HRESULT hr = S_OK;
	m_frameno = 0;
	return hr;
}

void CSwfPainter::OnStop()
{
	//m_flash->StopPlay();
}


void CSwfPainter::SetFrameNo( int frameno )
{
	m_frameno = frameno;
}

void CSwfPainter::GetFrame(int num)
{
	unsigned char request[5];
	request[0] = 2; // CMD_GETBMP
	int *p = (int *)(request+1);
	*p = num;
	DWORD wrt;
	WriteFile(m_hPipe, request, 5, &wrt, NULL);

	int size = m_width * m_height * 4  + 1;
	unsigned char *response = new unsigned char[size];
	DWORD rd;
	
	//char str[256];
	//sprintf_s(str,256,"CSwfPainter::GetFrame; num = %d",num);
	//g_log.Put(str);

	BOOL b = ReadFile(m_hPipe, response, size, &rd, NULL);
	if (b)
	{
		if (!m_bmp_buff)
			m_bmp_buff = new unsigned char[size-1];
		memcpy(m_bmp_buff, response+1, size-1);
	}

	delete response;

	//memset(str,0,256);
	//sprintf_s(str,256,"CSwfPainter::GetFrame; exit");
	//g_log.Put(str);
}

int CSwfPainter::OnDraw(unsigned char *p, int size, bool bottomup)
{
	
	//char str[256];
	//sprintf_s(str,256,"CSwfPainter::OnDraw; m_frameno = %d",m_frameno);
	//g_log.Put(str);
	
	GetFrame(m_frameno);
	
	int stride = size / m_height;
	
	//memset(str,0,256);
	//sprintf_s(str,256,"CSwfPainter::OnDraw; m_viewobject->Draw; stride = %d", stride);
	//g_log.Put(str);

	if (m_bmp_buff)
	{
		if (!bottomup)
		{
			BYTE *src = m_bmp_buff;
			BYTE *dst = p;
			for ( int i = 0; i < m_height; i++)
			{
				memcpy(dst,src, m_width*4);
				dst += stride;
				src += m_width * 4;
			}
		}
		else
		{
			BYTE *src = m_bmp_buff + (m_height-1)*m_width*4;
			BYTE *dst = p ;
			for ( int i = 0; i < m_height; i++)
			{
				memcpy(dst,src, m_width*4);
				dst += stride;
				src -= m_width * 4;
			}
		}
	}

	//memset(str,0,256);
	//sprintf_s(str,256,"CSwfPainter::OnDraw; exit");
	//g_log.Put(str);
	/*if (!m_bmp)
	{
		m_bmp = new Bitmap(m_width, m_height, PixelFormat32bppRGB);
		m_g  = new Graphics(m_bmp);
	}
	int stride = size / m_height;

	BitmapData bd;
	Rect r(0,0,m_width, abs(m_height));
	Status st = m_bmp->LockBits(&r, ImageLockModeRead, PixelFormat32bppRGB, &bd);       
	if (st != Status::InvalidParameter)
	{
		//BYTE *src = p + (m_height-1)*bd.Stride;
		BYTE *src = p + (m_height-1)*stride;
		BYTE *dst = (BYTE *)bd.Scan0;
		for ( int i = 0; i < abs(m_height); i++)
		{
			CopyMemory(dst, src, bd.Stride);
			src -= stride;
			dst += bd.Stride;   
		}
		m_bmp->UnlockBits(&bd);

		//Reload();
		m_flash->StopPlay();
		m_flash->put_FrameNum(m_frameno);
		HRESULT hr = m_viewobject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, m_hdccomp, &m_rectl, &m_rectl, NULL, NULL);
		Bitmap *bmpflash = Bitmap::FromHBITMAP(m_bitmap,NULL);
		//bmpflash->Im

		Rect r2;
		r2.X = m_rectl.left;
		r2.Y = m_rectl.top;
		r2.Width = m_rectl.right ;
		r2.Height = m_rectl.bottom;

		ImageAttributes ia;
		st = m_g->DrawImage(bmpflash,r2,0,0,m_rectl.right,m_rectl.bottom, UnitPixel, &ia);
		if (st != Status::InvalidParameter)
		{
			// Copy bitmap bits into output buffer
			BitmapData bd1;
			Rect r1(0,0,m_width, abs(m_height));
			st = m_bmp->LockBits(&r, ImageLockModeRead, PixelFormat32bppRGB, &bd1);      
			if (st != Status::InvalidParameter)
			{
				//BYTE *out = p + (m_height-1)*stride;
				BYTE *out = p;
				BYTE *in = (BYTE *)bd1.Scan0;
				for ( int i = 0; i < abs(m_height); i++)
				{
					CopyMemory(out, in, bd1.Stride);
					in += bd1.Stride;
					out += stride;   
				}
				m_bmp->UnlockBits(&bd1);
			}
		}
		delete bmpflash;
	}*/

	int ret = (m_frameno >= m_framecnt) ? -1 : m_frameno;
	m_frameno++;

	//delete m_bmp;
	//delete m_g;
	return ret;
}