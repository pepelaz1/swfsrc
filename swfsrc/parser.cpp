#include "parser.h"
#include "windows.h"
#include "bitarray.h"



CSwfParser::CSwfParser(string file):
	m_framerate(0),
	m_fsize(0),
	m_width(0),
	m_height(0),
	m_framecnt(0)
{
	m_file = file;
}


CSwfParser::~CSwfParser()
{
}

void CSwfParser::Process()
{
	ifstream is(m_file, std::ios::binary);
	int size = 300;
	unsigned char *buff = new unsigned char[size];
	is.read((char *)buff,size);
	is.close();
	unsigned char *p = buff;
	p += 4;
	m_fsize = *(int *)p;
	p += 4;

	unsigned int xmin = 0;
	unsigned int xmax = 0;
	unsigned int ymin = 0;
	unsigned int ymax = 0;
	{
		unsigned char *arr = new unsigned char[100];
		memcpy(arr,p,100);
		bit_array_c ba(arr,100);
		int k = 0;
		int tot = 0;
		unsigned char nbits = 0;
		for (int i = 0; i < 5; i++)
		{
			unsigned int bit = ba[i+k];
			bit <<= (4-i);
			nbits |= bit;		
		}
		k += 5;
		tot += 5;
		for (int i = 0; i < nbits; i++)
		{
			unsigned int bit = ba[i+k];
			bit <<= (nbits-i-1);
			xmin |= bit;		
		}
		k += nbits;
		tot += nbits;
		for (int i = 0; i < nbits; i++)
		{
			unsigned int bit = ba[i+k];
			bit <<= (nbits-i-1);
			xmax |= bit;		
		}
		k += nbits;
		tot += nbits;
		for (int i = 0; i < nbits; i++)
		{
			unsigned int bit = ba[i+k];
			bit <<= (nbits-i-1);
			ymin |= bit;		
		}
		k += nbits;
		tot += nbits;
		for (int i = 0; i < nbits; i++)
		{
			unsigned int bit = ba[i+k];
			bit <<= (nbits-i-1);
			ymax |= bit;		
		}
		tot += nbits;
		tot = (tot + 8) / 8;
		p += tot;
	}
		
	m_width = xmax / 20;
	m_height = ymax / 20;
	m_framerate = *(short *)p / 256.0;

	p += 2;
	m_framecnt = *(short *)p;


	m_framerate = 30.00;
	m_framecnt = 1000.00;
	m_width =  1584.00;
	m_height =  847.00;
	 

	
	delete [] buff;
}

double CSwfParser::FrameRate()
{
	return m_framerate;
}

int CSwfParser::FileSize()
{
	return m_fsize;
}

int  CSwfParser::FrameCnt()
{
	return m_framecnt;
}

int CSwfParser::Width()
{
	return m_width;
}

int CSwfParser::Height()
{
	return m_height;
}