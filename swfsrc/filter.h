#pragma once
#include "guids.h"
#include "memstream.h"
#include "videopin.h"

class CSwfFilter : public CAsyncReader
				 , public IFileSourceFilter
				// , public IMediaSeeking
				 , public IAMFilterMiscFlags
{
public:
    CSwfFilter(LPUNKNOWN pUnk, HRESULT *phr);
    ~CSwfFilter();
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    //  IFileSourceFilter implementation
    //  Load a (new) file
    STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);
 
    // Modeled on IPersistFile::Load
    // Caller needs to CoTaskMemFree or equivalent.
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt);

	// CBaseFilter methods 
    int GetPinCount();
    CBasePin *GetPin(int n);

	CCritSec *GetStateLock();

	 STDMETHODIMP Pause();
	 
	 //IAMFilterMiscFlags
	STDMETHOD_	(ULONG, GetMiscFlags)();
private:
    BOOL ReadTheFile(LPCTSTR lpszFileName);

private:
	CVideoPin m_vpin;
    LPWSTR     m_pFileName;
    LONGLONG   m_llSize;
    PBYTE      m_pbData;
    CMemStream m_Stream;

	CCritSec m_cStateLock;	// Lock this to serialize function accesses to the filter state

public:
	// IMediaSeeking imlpementation
	STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities) 
	{
		return E_NOTIMPL;
	}        
    STDMETHODIMP IsFormatSupported(const GUID *pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP GetTimeFormat(GUID *pFormat) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID *pFormat) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);	
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget,const GUID *pTargetFormat,
		LONGLONG Source, const GUID *pSourceFormat) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags);
    STDMETHODIMP GetPositions(LONGLONG *pCurrent,LONGLONG *pStop) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP SetRate(double dRate) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP GetRate(double *pdRate) 
	{
		return E_NOTIMPL;
	}
    STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);
};
