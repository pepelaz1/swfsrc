#pragma once
#include <atlbase.h>
#include <atlwin.h>
#include <comutil.h>
#include <string>
using namespace std;
#include "iflash.h"
#include <gdiplus.h>
using namespace Gdiplus;
#include <streams.h>

class CSwfPainter
{
private:
	string m_file;
	int m_width;
	int m_height;
	//CAxWindow *m_axwnd;
	//HDC m_hdccomp;
	//HBITMAP m_bitmap;
	//RECTL m_rectl;
	int m_frameno;
	int m_framecnt;
	unsigned char *m_bmp_buff;

	HANDLE m_hPipe; 

	//ULONG_PTR m_gdiplusToken;
	//Graphics *m_g;
	//Bitmap *m_bmp;
	

	//CComPtr<IShockwaveFlash> m_flash;
	//CComPtr<IViewObjectEx> m_viewobject;

	//void ReleaseInterfaces();
	//void ReleaseGDI();
	//void DeleteAxWnd();
	//HRESULT Reload();
	HRESULT LoadSwf();
	void CloseBmpSrv();
	void GetFrame(int num);
	CCritSec m_lock;
public:
	CSwfPainter();
	~CSwfPainter();
	HRESULT Init(string file, int width, int height, int framecnt);
	void Reset();
	HRESULT OnPlay();
	void OnStop();
	int OnDraw(unsigned char *p, int size, bool bottomup);
	void SetFrameNo( int frameno );
	int FrameCount();
	int CurrentFrameNo();
};

