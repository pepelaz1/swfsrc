#pragma once
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

class CLogger
{
private:
	string  m_filename;
public:
	CLogger();
	~CLogger();
	void Reset();
	void Put(string msg);
};

