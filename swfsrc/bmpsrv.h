#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

class CBmpSrv
{
private:
	void Read(string path);
	void Execute(string path);
public:
	CBmpSrv();
	~CBmpSrv();
	void Run();
};

