// bmpsrv.cpp : Defines the entry point for the application.
//

#include "stdafx.h"


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	CSwfBmp bmp;
	bmp.Run();
	return 0;
}

