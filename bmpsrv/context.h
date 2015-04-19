#pragma once
class CContext
{
private:
	HANDLE m_hPipe;
	CSwfBmp *m_swfbmp;
	HDC m_hdccomp;
	HBITMAP m_bitmap;
	CAxWindow *m_axwnd;
	RECTL m_rectl;
	unsigned char *m_bmp_buff;
	int m_size;
	CComPtr<IShockwaveFlash> m_flash;
	CComPtr<IViewObjectEx> m_viewobject;

	void ReleaseInterfaces();
	void ReleaseGDI();
	void DeleteAxWnd();	
public:
	CContext(CSwfBmp *swfbmp, HANDLE hPipe);
	~CContext();	
	void Init(string filename, int width, int height);
	LONG GetBmp(int num);

	CSwfBmp *SwfBmp();
	HANDLE Pipe();
	unsigned char *GetBuffer();
	int  GetSize();
};

