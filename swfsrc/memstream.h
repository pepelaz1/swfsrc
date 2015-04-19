#pragma once
#include <streams.h>
#include "asyncio.h"

class CMemStream : public CAsyncStream
{
public:
    CMemStream();
    void Init(LPBYTE pbData, LONGLONG llLength, DWORD dwKBPerSec = INFINITE);
    HRESULT SetPointer(LONGLONG llPos);
    HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead,BOOL bAlign,LPDWORD pdwBytesRead);
    LONGLONG Size(LONGLONG *pSizeAvailable);
    DWORD Alignment();
    void Lock();
    void Unlock();
private:
    CCritSec       m_csLock;
    PBYTE          m_pbData;
    LONGLONG       m_llLength;
    LONGLONG       m_llPosition;
    DWORD          m_dwKBPerSec;
    DWORD          m_dwTimeStart;
};