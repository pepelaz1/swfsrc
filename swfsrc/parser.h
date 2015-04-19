#pragma once
#include <string>
#include <fstream>
#include <istream>
#include <bitset>
using namespace std;

class CSwfParser
{
private:
	string m_file;
	int m_fsize;
	double m_framerate;
	int m_width;
	int m_height;
	int m_framecnt;
public:
	CSwfParser(string file);
	~CSwfParser();
	void Process();

	double FrameRate();
	int Width();
	int Height();
	int FileSize();
	int FrameCnt();
};

