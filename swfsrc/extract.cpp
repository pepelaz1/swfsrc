#include "extract.h"
#include "resource.h"
#include <Windows.h>
#include "logger.h"

extern CLogger g_log;

static int g_id = 100;


CExtractor::CExtractor(void) :
	m_buff(0), m_size(0)
{
	char s[10];
	itoa(g_id,s,10);
	m_id = s;
	g_id++;
}


CExtractor::~CExtractor(void)
{
	delete m_buff;
}

void ClearDirectory(string directory)
{
	return;

	string dir = directory + "\\*";
	WIN32_FIND_DATA ffdata;
	HANDLE find = FindFirstFile(dir.c_str(), &ffdata);
	do
	{
		if (strcmp(ffdata.cFileName,".") !=0 && 
			strcmp(ffdata.cFileName,"..") != 0)
		{
			if ( ffdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				string cdir = directory + "\\" + ffdata.cFileName;
				ClearDirectory(cdir);
				RemoveDirectory(cdir.c_str());
			}
			else
			{
				string dfile = directory + "\\" + ffdata.cFileName;
				DeleteFile(dfile.c_str());
			}
		}
	}
	while(FindNextFileA(find, &ffdata));
	FindClose(find);
}

extern HMODULE g_Module;

void CExtractor::ReadExtractor(string path)
{
	HRSRC resource = FindResource(g_Module,MAKEINTRESOURCE(IDR_EXTRACTOR),"EXE");
	if (resource)
	{
		HGLOBAL loaded = LoadResource(g_Module, resource);
		if (loaded)
		{
			void *p = LockResource(loaded);
			if (p)
			{
				DWORD size = SizeofResource(g_Module, resource);
				ofstream os(path + "\\extractor.exe", std::ios::binary);
				os.write((char *)p,size);
				os.close();
			}
			UnlockResource(loaded);
		}
	}
}

void CExtractor::Execute(string path, string args)
{
	string pname = path + "/extractor.exe " + args;
	string oname = path + "/out" + m_id;
	

	SECURITY_ATTRIBUTES  sa;
	memset(&sa,0,sizeof(SECURITY_ATTRIBUTES));
	sa.bInheritHandle = TRUE;
	HANDLE h = CreateFile( oname.c_str(),GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL );

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(STARTUPINFO)); 
	memset(&pi, 0, sizeof(PROCESS_INFORMATION)); 
	si.cb = sizeof(STARTUPINFO); 
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = h;	
	si.hStdError = h;

	//CreateProcess(pname.c_str(), (LPSTR)args.c_str(),NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
	CreateProcess(NULL, (LPSTR)pname.c_str(),NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);

	WaitForSingleObject(pi.hProcess, INFINITE);
		
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(h);
}


void CExtractor::ReadLines(string path)
{
	ifstream is(path + "/out" + m_id, ios_base::in);
	string line;
	while (getline(is, line, '\n'))
		m_lines.push_back(line);
}

bool CExtractor::CheckFileExists(string file)
{
	return ifstream( file ).is_open();
}

bool CExtractor::Extract(string path, string src)
{
	string args = "";
	args = src;
	Execute(path, args);
	
	ReadLines(path);
	
	bool result = false;
	// try "MP3 Soundstream" first
	for( int i = 0; i < (int)m_lines.size(); i++)
	{
		if (m_lines[i].find("MP3 Soundstream") != string::npos)
		{
			args = " -m ";
			args += src + " -o ";
			args += path + "\\sound" + m_id;
			Execute(path, args);
			
			result = CheckFileExists(path + "\\sound" + m_id);
			break;
		}
	}

	if (result)
		return true;

	// Next, try "Sounds:"
	for( int i = 0; i < (int)m_lines.size(); i++)
	{
		int b = m_lines[i].find("Sounds: ID(s)");
		if (b != string::npos)
		{
			b += 14;
			int e = m_lines[i].find(",");
			if (e == string::npos)	e = m_lines[i].length();

			string num = m_lines[i].substr(b,e-b);

			args = " -s " + num + " ";
			args += src + " -o ";
			args += path + "\\sound" + m_id;
			Execute(path, args);
			
			result = CheckFileExists(path + "\\sound" + m_id);
		}
	}

	if (result)
		return true;

	// Next, try "Embedded MP3: ID(s)"
	for( int i = 0; i < (int)m_lines.size(); i++)
	{
		int b = m_lines[i].find("Embedded MP3: ID(s)");
		if (b != string::npos)
		{
			b += 19;
			int e = m_lines[i].find(",");
			if (e == string::npos)	e = m_lines[i].length();

			string num = m_lines[i].substr(b,e-b);

			args = " -M " + num + " ";
			args += src + " -o ";
			args += path + "\\sound" + m_id;
			Execute(path, args);
			
			result = CheckFileExists(path + "\\sound" + m_id);
		}
	}

	return true;
}

bool CExtractor::ReadIntoBuffer(string path)
{
	ifstream is(path + "\\sound" + m_id, std::ios::binary);
	if (!is.is_open())
		return true;
	is.seekg(0, std::ios::end);
	m_size = (int)is.tellg();
	is.seekg(0);
	m_buff = new unsigned char[m_size];
	is.read((char *)m_buff,m_size);
	is.close();
	return true;
}

bool CExtractor::Load(string file)
{
	// Get temporary path
	char s[MAX_PATH];
	GetTempPath(MAX_PATH,s);
	string path = s;
	path = path + "\\"+"swfsrc";

	// Check whether directory exists and create if not
	CreateDirectory(path.c_str(), NULL);

	// Read extrator and save into temp path
	ReadExtractor(path);

	// Run it and extract audio data into temp file
	if (!Extract(path,file))
	{
		//ClearDirectory(path);	
		DeleteTmpFiles(path);
		return false;
	}

	// Read temp file into buffer
	if (!ReadIntoBuffer(path))
	{
		//ClearDirectory(path);	
		DeleteTmpFiles(path);
		return false;
	}
	//ClearDirectory(path);	
	DeleteTmpFiles(path);
	
	return true;
}

void CExtractor::DeleteTmpFiles(string path)
{
	string f = path + "\\sound" + m_id;
	DeleteFile(f.c_str());
	f = path + "\\out" + m_id;
	DeleteFile(f.c_str());
}

unsigned int CExtractor::GetSize()
{
	return m_size;
}

unsigned char *CExtractor::GetPointer()
{
	return m_buff;
}