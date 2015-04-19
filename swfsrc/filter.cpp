#include <streams.h>

#include "asyncio.h"
#include "asyncrdr.h"

#pragma warning(disable:4710)  // 'function' not inlined (optimization)
#include "filter.h"
#include "extract.h"
#include "bmpsrv.h"
#include "logger.h"

extern CLogger g_log;

//
// Setup data for filter registration
//
const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{ &MEDIATYPE_Stream     // clsMajorType
, &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN sudOpPin =
{ L"Output"          // strName
, FALSE              // bRendered
, TRUE               // bOutput
, FALSE              // bZero
, FALSE              // bMany
, &CLSID_NULL        // clsConnectsToFilter
, L"Input"           // strConnectsToPin
, 1                  // nTypes
, &sudOpPinTypes };  // lpTypes

const AMOVIESETUP_FILTER sudAsync =
{ &CLSID_SwfSrc              // clsID
, L"Swf Source"  // strName
, MERIT_UNLIKELY                  // dwMerit
, 1                               // nPins
, &sudOpPin };                    // lpPin


//
//  Object creation template
//
CFactoryTemplate g_Templates[1] = {
    { L"Swf Source"
    , &CLSID_SwfSrc
    , CSwfFilter::CreateInstance
    , NULL
    , &sudAsync }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


HRESULT UnRegisterFileType(char *szExtension)
{
	DWORD disp;
	HKEY m_hKey;


	char szMediaExt[MAX_PATH];
	if( !strstr(szExtension,".") )
	{
		strcpy(szMediaExt,".");
		strcat(szMediaExt, szExtension);
	}
	else
	{
		strcpy(szMediaExt,szExtension);
	}

	if( ERROR_SUCCESS != RegCreateKeyExW(HKEY_CLASSES_ROOT, L"Media Type\\Extensions", 0, L"", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp) )
	{
		return E_FAIL;
	}

	RegDeleteKey(m_hKey,szMediaExt);

	return S_OK;
}


HRESULT RegisterFileType(wchar_t *szExtension)
{
	//Register the filter as default filter for .mov files
	DWORD disp;
	HKEY m_hKey;

	wchar_t szMediaExt[MAX_PATH] = L"Media Type\\Extensions\\";
	if( !wcsstr(szExtension,L".") )
		wcscat(szMediaExt,L".");
	
	wcscat(szMediaExt,szExtension);
	
	if( ERROR_SUCCESS != RegCreateKeyExW(HKEY_CLASSES_ROOT, szMediaExt, 0, L"", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp) )
		return E_FAIL;
	
	LPOLESTR pwszClsid;
	
	StringFromCLSID(CLSID_SwfSrc,&pwszClsid);

	
	if( ERROR_SUCCESS != RegSetValueExW(m_hKey, L"Source Filter", 0, REG_SZ, (BYTE *)(LPCWSTR)pwszClsid, wcslen(pwszClsid)*sizeof(wchar_t)) )
		return E_FAIL;

    // Free memory used by StringFromCLSID
    CoTaskMemFree(pwszClsid);
		
	RegCloseKey(m_hKey);
	
	return S_OK;
}


STDAPI DllRegisterServer()
{
	RegisterFileType(L"swf");
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterFileType("swf");
    return AMovieDllRegisterServer2(FALSE);
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
HMODULE g_Module;

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	
	g_Module = (HINSTANCE)hModule;
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}


//* Create a new instance of this class
CUnknown * WINAPI CSwfFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    ASSERT(phr);

    //  DLLEntry does the right thing with the return code and
    //  the returned value on failure

    return new CSwfFilter(pUnk, phr);
}

 CSwfFilter::CSwfFilter(LPUNKNOWN pUnk, HRESULT *phr) :
        CAsyncReader(NAME("Mem Reader"), pUnk, &m_Stream, phr),
        m_pFileName(NULL),
        m_pbData(NULL),
		m_vpin(this,&m_csFilter,phr)
{
}

CSwfFilter::~CSwfFilter()
{
    delete [] m_pbData;
    delete [] m_pFileName;
}

 STDMETHODIMP CSwfFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
 {
	 if (riid == IID_IFileSourceFilter) 
	 {
		 return GetInterface((IFileSourceFilter *)this, ppv);
	 }
	 //else if (riid == IID_IMediaSeeking) 
	 //{
		// return GetInterface((IMediaSeeking *)this, ppv);
	 //} 
	 else if (riid == IID_IAMFilterMiscFlags) 
	 {
		 return GetInterface((IAMFilterMiscFlags *)this, ppv);
	 } 
	 else {
		 return CAsyncReader::NonDelegatingQueryInterface(riid, ppv);
	 }
 }

 STDMETHODIMP_(ULONG) CSwfFilter::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

STDMETHODIMP CSwfFilter::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
	USES_CONVERSION;
	char str[256];
	sprintf_s(str, 256, "CSwfFitler::Load lpwszFileName = %s", W2A(lpwszFileName));
	g_log.Put(str);
	
	char curdir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,curdir);

    CheckPointer(lpwszFileName, E_POINTER);

    // lstrlenW is one of the few Unicode functions that works on win95
    int cch = lstrlenW(lpwszFileName) + 1;

#ifndef UNICODE
    TCHAR *lpszFileName=0;
    lpszFileName = new char[cch * 2];
    if (!lpszFileName) {
      	return E_OUTOFMEMORY;
    }
    WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1,
    		lpszFileName, cch, NULL, NULL);
#else
    TCHAR lpszFileName[MAX_PATH]={0};
    (void)StringCchCopy(lpszFileName, NUMELMS(lpszFileName), lpwszFileName);
#endif
    CAutoLock lck(&m_csFilter);

    /*  Check the file type */
    CMediaType cmt;
    if (NULL == pmt) {
        cmt.SetType(&MEDIATYPE_Stream);
        cmt.SetSubtype(&MEDIASUBTYPE_NULL);
    } else {
        cmt = *pmt;
    }

    if (!ReadTheFile(lpszFileName)) {
#ifndef UNICODE
        delete [] lpszFileName;
#endif
		g_log.Put("CSwfFitler::Load ReadTheFile failed");
        return E_FAIL;
    }
    m_Stream.Init(m_pbData, m_llSize);

    m_pFileName = new WCHAR[cch];

    if (m_pFileName!=NULL)
    	CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));

    // this is not a simple assignment... pointers and format
    // block (if any) are intelligently copied
    m_mt = cmt;

    /*  Work out file type */
    cmt.bTemporalCompression = TRUE;	       //???
    cmt.lSampleSize = 1;
	
	m_vpin.SetFile(lpszFileName);

    return S_OK;
}

STDMETHODIMP CSwfFilter::GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
{
	char str[256];
	sprintf_s(str, 256, "CSwfFitler::GetCurFile");
	g_log.Put(str);

    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;

    if (m_pFileName!=NULL) {
        DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pFileName));

        *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
        if (*ppszFileName!=NULL) {
                CopyMemory(*ppszFileName, m_pFileName, n);
        }
    }

    if (pmt!=NULL) {
        CopyMediaType(pmt, &m_mt);
    }

    return NOERROR;
}
STDMETHODIMP CSwfFilter::Pause()
{
	HRESULT hr = S_OK;
	hr = CAsyncReader::Pause();
	if (FAILED(hr))
		return hr;

	g_log.Put("CSwfFitler::Pause");
	return hr;
}

BOOL CSwfFilter::ReadTheFile(LPCTSTR lpszFileName)
{
    //DWORD dwBytesRead;

	CExtractor swfp;
	if (!swfp.Load(lpszFileName))
		return FALSE;

	unsigned int size = swfp.GetSize();
	PBYTE pbMem = new BYTE[size];
	CopyMemory(pbMem, swfp.GetPointer(), size);
	m_pbData = pbMem;
    m_llSize = size;

    // Open the requested file
   /* HANDLE hFile = CreateFile(lpszFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DbgLog((LOG_TRACE, 2, TEXT("Could not open %s\n"), lpszFileName));
        return FALSE;
    }

    // Determine the file size
    ULARGE_INTEGER uliSize;
    uliSize.LowPart = GetFileSize(hFile, &uliSize.HighPart);

    PBYTE pbMem = new BYTE[uliSize.LowPart];
    if (pbMem == NULL) 
    {
        CloseHandle(hFile);
        return FALSE;
    }

    // Read the data from the file
    if (!ReadFile(hFile,
                  (LPVOID) pbMem,
                  uliSize.LowPart,
                  &dwBytesRead,
                  NULL) ||
        (dwBytesRead != uliSize.LowPart))
    {
        DbgLog((LOG_TRACE, 1, TEXT("Could not read file\n")));

        delete [] pbMem;
        CloseHandle(hFile);
        return FALSE;
    }

    // Save a pointer to the data that was read from the file
    m_pbData = pbMem;
    m_llSize = (LONGLONG)uliSize.QuadPart;

    // Close the file
    CloseHandle(hFile);*/
	
	CBmpSrv bs;
	bs.Run();
    return TRUE;
}


// --- CBaseFilter methods ---
int CSwfFilter::GetPinCount()
{
	return 2;
}

CBasePin *CSwfFilter::GetPin(int n)
{
	if (n == 0)
	{
		return &m_vpin;
	}
	else if ( n == 1)
	{
		return &m_apin;
	}
    else
    {
        return NULL;
    }

}

CCritSec *CSwfFilter::GetStateLock()
{
	return &m_cStateLock;
}

STDMETHODIMP CSwfFilter::GetDuration(LONGLONG *pDuration) 
{
	return m_vpin.GetDuration(pDuration);
}

STDMETHODIMP CSwfFilter::GetCapabilities(DWORD *pCapabilities)
{
	return m_vpin.GetCapabilities(pCapabilities);
}

STDMETHODIMP CSwfFilter::IsFormatSupported(const GUID *pFormat) 
{
	return m_vpin.IsFormatSupported(pFormat);
}

STDMETHODIMP CSwfFilter::IsUsingTimeFormat(const GUID *pFormat) 
{
	return m_vpin.IsUsingTimeFormat(pFormat);
}

STDMETHODIMP CSwfFilter::GetPreroll(LONGLONG *pllPreroll) 
{
	return m_vpin.GetPreroll(pllPreroll);
}  

STDMETHODIMP CSwfFilter::GetStopPosition(LONGLONG *pStop) 
{
	return m_vpin.GetStopPosition(pStop);
}   

STDMETHODIMP CSwfFilter::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,  DWORD dwStopFlags) 
{
	return m_vpin.SetPositions(pCurrent,dwCurrentFlags,pStop,dwStopFlags);
}


