#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

class CExtractor
{
private:
	string m_id;
	vector<string>m_lines;
	unsigned char *m_buff;
	unsigned int m_size;

	void ReadLines(string path);
	bool CheckFileExists(string file);
	void ReadExtractor(string path);
	bool Extract(string path, string src);
	void Execute(string path, string args);
	bool ReadIntoBuffer(string path);
	void DeleteTmpFiles(string path);
public:
	CExtractor();
	~CExtractor();
	bool Load(string file);
	unsigned int GetSize();
	unsigned char *GetPointer();
};

