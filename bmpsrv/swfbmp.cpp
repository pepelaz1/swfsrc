#include "StdAfx.h"


//#define BUFFER_SIZE		2097152 // 2M bytes
#define BUFFER_SIZE		4096 // 4K bytes

CSwfBmp::CSwfBmp() 
	: m_clientscnt(0)

{
	CoInitialize(0);
}

CSwfBmp::~CSwfBmp()
{
}

DWORD WINAPI InstanceThread(LPVOID); 

int CSwfBmp::Run()
{
	string pipename = "\\\\.\\pipe\\swfsrc";

	SECURITY_ATTRIBUTES sa;
	sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	InitializeSecurityDescriptor(sa.lpSecurityDescriptor, 
		SECURITY_DESCRIPTOR_REVISION);

	SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, NULL, FALSE);
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	for (;;)
	{
		// Create the named pipe.
		HANDLE hPipe = CreateNamedPipe(
			pipename.c_str(),				// The unique pipe name. This string must 
			// have the form of \\.\pipe\pipename
			PIPE_ACCESS_DUPLEX,			// The pipe is bi-directional; both  
			// server and client processes can read 
			// from and write to the pipe
			PIPE_TYPE_MESSAGE |			// Message type pipe 
			PIPE_READMODE_MESSAGE |		// Message-read mode 
			PIPE_WAIT,					
			PIPE_UNLIMITED_INSTANCES,	// Max. instances

			// These two buffer sizes have nothing to do with the buffers that 
			// are used to read from or write to the messages. The input and 
			// output buffer sizes are advisory. The actual buffer size reserved 
			// for each end of the named pipe is either the system default, the 
			// system minimum or maximum, or the specified size rounded up to the 
			// next allocation boundary. The buffer size specified should be 
			// small enough that your process will not run out of nonpaged pool, 
			// but large enough to accommodate typical requests.
			BUFFER_SIZE,				// Output buffer size in bytes
			BUFFER_SIZE,				// Input buffer size in bytes

			NMPWAIT_USE_DEFAULT_WAIT,	// Time-out interval
			&sa							// Security attributes
			);

		BOOL b = ConnectNamedPipe(hPipe, NULL);
		if (b)
		{
			//Create separate thread for this client
			CContext *ctx = new CContext(this, hPipe);
			DWORD dwThreadId = 0;
			HANDLE hThread = CreateThread( 
				NULL,             
				0,                  
				InstanceThread,   
				(LPVOID) ctx,    
				0,                 
				&dwThreadId);      
			CloseHandle(hThread);
			m_clientscnt++;
		}
		/*else
		{
			return 1;
		}*/

	}
	return 0;
}


void CSwfBmp::Load(CContext *ctx, unsigned char *buff)
{
	int len = buff[1];
	string filename;
	filename.append((const char *)(buff+2),len);
	int width = *(int *)(buff + len + 2);
	int height = *(int *)(buff + len + 6);
	ctx->Init(filename, width, height);

	ofstream out("c:\\bmpsrv.txt",fstream::app); 
	out << filename << "\n";
	out << width << "\n";
	out << height << "\n";
	out.close();

	unsigned char response[1];
	response[0] = RESP_OK;
	DWORD w;
	WriteFile(ctx->Pipe(), response, 1, &w, NULL);
}

#include <sstream>
void  CSwfBmp::GetBmp(CContext *ctx, unsigned char *buff)
{
	int num = *(int *)(buff+1);
	LONG frm= ctx->GetBmp(num);

	ofstream out("c:\\bmpsrv.txt", fstream::app); 
	out << "Num = " << num <<"  frm loaded = " << frm << "\n";
	out.close();
	
	unsigned char *response = new unsigned char[ctx->GetSize() + 1];
	response[0] = RESP_OK;
	memcpy(response+1,ctx->GetBuffer(), ctx->GetSize());
	DWORD wrt;
	BOOL b = WriteFile(ctx->Pipe(), response, ctx->GetSize() + 1, &wrt, NULL);

	//stringstream ss;
	//ss << "c:\\Img" << num << ".bmp";
	//ofstream imgfile (ss.str(),ofstream::binary);
	//imgfile.write((const char *)ctx->GetBuffer(), ctx->GetSize());
	//imgfile.close();

	delete [] response;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	CContext *ctx = (CContext *)lpvParam;
	if (ctx->SwfBmp())
		ctx->SwfBmp()->ThreadProc(ctx);
	return 0;
}

void CSwfBmp::ThreadProc(CContext *ctx)
{
	HANDLE hPipe = ctx->Pipe();
	unsigned char request[BUFFER_SIZE];	// Client -> Server
	DWORD cbBytesRead, cbRequestBytes;
	//unsigned char  chReply[BUFFER_SIZE];		// Server -> Client
	//DWORD cbBytesWritten, cbReplyBytes;

	BOOL bResult;

	while(true)
	{
		// Receive one message from the pipe.
		cbRequestBytes = sizeof(TCHAR) * BUFFER_SIZE;
		bResult = ReadFile(			// Read from the pipe.
			hPipe,					// Handle of the pipe
			request,				// Buffer to receive data
			cbRequestBytes,			// Size of buffer in bytes
			&cbBytesRead,			// Number of bytes read
			NULL);					// Not overlapped I/O

		if (!bResult/*Failed*/ || cbBytesRead == 0/*Finished*/) 
			break;

		if (request[0] == CMD_LOAD)
		{
			Load(ctx,request);
		}
		else if (request[0] == CMD_GETBMP)
		{
 			GetBmp(ctx, request);
		}
		else if (request[0] == CMD_EXIT)
		{
			/*ofstream out("bmpsrv.txt"); 
			out << "EXIT" << "\n";
			out.close();*/
			break;
		}
	}
	
	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe);
	delete ctx;
	m_clientscnt--;
	if (m_clientscnt <= 0)
	{
		ExitProcess(0);
	}
}