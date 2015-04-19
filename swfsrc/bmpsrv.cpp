#include "bmpsrv.h"
#include "resource.h"
#include <Windows.h>

extern HMODULE g_Module;

CBmpSrv::CBmpSrv(void)
{
}


CBmpSrv::~CBmpSrv(void)
{
}

extern void ClearDirectory(string directory);
void CBmpSrv::Run()
{
	// Get temporary path
	char s[MAX_PATH];
	GetTempPath(MAX_PATH,s);
	string path = s;
	path = path + "\\"+"swfsrc";

	// Check whether \img2docx directory exists and create if not
	CreateDirectory(path.c_str(), NULL);

	// Read bmp srv
	Read(path);

	// Run it 
	Execute(path);

	ClearDirectory(path);	
}

void CBmpSrv::Read(string path)
{
	HRSRC resource = FindResource(g_Module,MAKEINTRESOURCE(IDR_BMPSRV),"EXE");
	if (resource)
	{
		HGLOBAL loaded = LoadResource(g_Module, resource);
		if (loaded)
		{
			void *p = LockResource(loaded);
			if (p)
			{
				DWORD size = SizeofResource(g_Module, resource);
				ofstream os(path + "\\bmpsrv.exe", std::ios::binary);
				os.write((char *)p,size);
				os.close();
			}
			UnlockResource(loaded);
		}
	}
}


void CBmpSrv::Execute(string path)
{
	string pname = path + "/bmpsrv.exe ";
		
	SECURITY_ATTRIBUTES  sa;
	memset(&sa,0,sizeof(SECURITY_ATTRIBUTES));
	//sa.bInheritHandle = TRUE;
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(STARTUPINFO)); 
	memset(&pi, 0, sizeof(PROCESS_INFORMATION)); 
	si.cb = sizeof(STARTUPINFO); 
	//si.dwFlags |= STARTF_USESTDHANDLES;

	BOOL b = CreateProcess(pname.c_str(), NULL,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	
	Sleep(300);
	//BOOL b = CreateProcess(NULL, (LPSTR)pname.c_str(),NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	//WaitForSingleObject(pi.hProcess, INFINITE);
		
	//CloseHandle(pi.hProcess);
	//CloseHandle(pi.hThread);
}