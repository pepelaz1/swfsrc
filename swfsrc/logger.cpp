#include "logger.h"
#include "Shlobj.h"
#include <time.h>

CLogger::CLogger(void)
{
	Reset();
}


CLogger::~CLogger(void)
{
}

void CLogger::Reset()
{
	char path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
	m_filename = path;
	m_filename += "\\swfsrc.log";
	
	ofstream out(m_filename, fstream::trunc);
	out << "";
	out.close();	
}

void CLogger::Put(string msg)
{
	ofstream out(m_filename, fstream::app);

	time_t now;
	time(&now);
	struct tm *current;
	current = localtime(&now);
	char tstr[256];
	sprintf(tstr, "%02d:%02d:%02d",  current->tm_hour, current->tm_min, current->tm_sec);

	out << tstr;
	out << ": ";
	out << msg;
	out << "\n";
	out.close();
}