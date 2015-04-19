#include "memstream.h"

CMemStream::CMemStream() : 
	m_llPosition(0)
{
}

void CMemStream::Init(LPBYTE pbData, LONGLONG llLength, DWORD dwKBPerSec)
{
    m_pbData = pbData;
    m_llLength = llLength;
    m_dwKBPerSec = dwKBPerSec;
    m_dwTimeStart = timeGetTime();
}

HRESULT CMemStream::SetPointer(LONGLONG llPos)
{
    if (llPos < 0 || llPos > m_llLength) 
	{
        return S_FALSE;
    }
	else 
	{
        m_llPosition = llPos;
        return S_OK;
    }
}

 HRESULT  CMemStream::Read(PBYTE pbBuffer,DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
    CAutoLock lck(&m_csLock);
    DWORD dwReadLength;

    /*  Wait until the bytes are here! */
    DWORD dwTime = timeGetTime();

    if (m_llPosition + dwBytesToRead > m_llLength) {
        dwReadLength = (DWORD)(m_llLength - m_llPosition);
    } else {
        dwReadLength = dwBytesToRead;
    }
    DWORD dwTimeToArrive =
        ((DWORD)m_llPosition + dwReadLength) / m_dwKBPerSec;

    if (dwTime - m_dwTimeStart < dwTimeToArrive) {
        Sleep(dwTimeToArrive - dwTime + m_dwTimeStart);
    }

    CopyMemory((PVOID)pbBuffer, (PVOID)(m_pbData + m_llPosition),
                dwReadLength);

    m_llPosition += dwReadLength;
    *pdwBytesRead = dwReadLength;
    return S_OK;
}

LONGLONG CMemStream::Size(LONGLONG *pSizeAvailable)
{
    LONGLONG llCurrentAvailable =
        static_cast <LONGLONG> (UInt32x32To64((timeGetTime() - m_dwTimeStart),m_dwKBPerSec));
 
    *pSizeAvailable =  min(m_llLength, llCurrentAvailable);
    return m_llLength;
}

DWORD CMemStream::Alignment()
{
    return 1;
}

void CMemStream::Lock()
{
    m_csLock.Lock();
}

void CMemStream::Unlock()
{
    m_csLock.Unlock();
}