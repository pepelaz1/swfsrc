#pragma once

class CContext;
class CSwfBmp
{
private:
	int m_clientscnt;
	enum {CMD_LOAD = 1, CMD_GETBMP, CMD_EXIT};
	enum {RESP_FAILED = 0, RESP_OK};
	void Load(CContext *ctx, unsigned char *buff);
	void GetBmp(CContext *ctx, unsigned char *buff);
public:
	CSwfBmp();
	~CSwfBmp();
	int Run();
	void ThreadProc(CContext *ctx);
};

