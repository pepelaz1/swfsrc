//------------------------------------------------------------------------------
// File: AsyncRdr.cpp
//
// Desc: DirectShow sample code - base library with I/O functionality.
//       This file implements I/O source filter methods and output pin 
//       methods for CAsyncReader and CAsyncOutputPin.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include <streams.h>
#include "asyncio.h"
#include "asyncrdr.h"
#include <initguid.h>
#include "filter.h"
#include "logger.h"

extern CLogger g_log;



// --- CAsyncOutputPin implementation ---

CAsyncOutputPin::CAsyncOutputPin(
    HRESULT * phr,
    CAsyncReader *pReader,
    CAsyncIo *pIo,
    CCritSec * pLock)
  : CBasePin(
    NAME("Async output pin"),
    pReader,
    pLock,
    phr,
    L"Audio",
    PINDIR_OUTPUT),
    m_pReader(pReader),
    m_pIo(pIo)
{
}

CAsyncOutputPin::~CAsyncOutputPin()
{
}


STDMETHODIMP CAsyncOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if(riid == IID_IAsyncReader)
    {
        m_bQueriedForAsyncReader = TRUE;
        return GetInterface((IAsyncReader*) this, ppv);
    }
    else
    {
        return CBasePin::NonDelegatingQueryInterface(riid, ppv);
    }
}


HRESULT CAsyncOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	char str[256];
	sprintf_s(str, 256, "CSwfFitler::GetMediaType iPosition = %d", iPosition);
	g_log.Put(str);

    if(iPosition < 0)
    {
        return E_INVALIDARG;
    }
    if(iPosition > 0)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER); 
    CheckPointer(m_pReader,E_UNEXPECTED); 
    
    *pMediaType = *m_pReader->LoadType();
    return S_OK;
}


HRESULT CAsyncOutputPin::CheckMediaType(const CMediaType* pType)
{
    CAutoLock lck(m_pLock);
	return S_OK;

 //   /*  We treat MEDIASUBTYPE_NULL subtype as a wild card */
 //   if((m_pReader->LoadType()->majortype == pType->majortype) &&
 //      (m_pReader->LoadType()->subtype == MEDIASUBTYPE_NULL   ||
 //       m_pReader->LoadType()->subtype == pType->subtype))
 //   {
	//	char str[256];
	//	sprintf_s(str, 256, "CSwfFitler::CheckMediaType ok");
	//	g_log.Put(str);

 //       return S_OK;
 //   }

	//char str[256];
	//sprintf_s(str, 256, "CSwfFitler::CheckMediaType false");
	//g_log.Put(str);


 //   return S_FALSE;
	
}


HRESULT CAsyncOutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
    CheckPointer(ppAlloc,E_POINTER);

	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::InitAllocator");
	g_log.Put(str);


    HRESULT hr = NOERROR;
    CMemAllocator *pMemObject = NULL;

    *ppAlloc = NULL;

    /* Create a default memory allocator */
    pMemObject = new CMemAllocator(NAME("Base memory allocator"), NULL, &hr);
    if(pMemObject == NULL)
    {
        return E_OUTOFMEMORY;
    }
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::InitAllocator pMemObject = new CMemAllocator failed");
		g_log.Put(str);
		
        delete pMemObject;
        return hr;
    }

    /* Get a reference counted IID_IMemAllocator interface */
    hr = pMemObject->QueryInterface(IID_IMemAllocator,(void **)ppAlloc);
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::InitAllocator pMemObject->QueryInterface failed");
		g_log.Put(str);

        delete pMemObject;
        return E_NOINTERFACE;
    }

    ASSERT(*ppAlloc != NULL);
    return NOERROR;
}


// we need to return an addrefed allocator, even if it is the preferred
// one, since he doesn't know whether it is the preferred one or not.
STDMETHODIMP 
CAsyncOutputPin::RequestAllocator(
    IMemAllocator* pPreferred,
    ALLOCATOR_PROPERTIES* pProps,
    IMemAllocator ** ppActual)
{
    CheckPointer(pPreferred,E_POINTER);
    CheckPointer(pProps,E_POINTER);
    CheckPointer(ppActual,E_POINTER);
    ASSERT(m_pIo);

	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::RequestAllocator");
	g_log.Put(str);

    // we care about alignment but nothing else
    if(!pProps->cbAlign || !m_pIo->IsAligned(pProps->cbAlign))
    {
        m_pIo->Alignment(&pProps->cbAlign);
    }

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;

    if(pPreferred)
    {
        hr = pPreferred->SetProperties(pProps, &Actual);

        if(SUCCEEDED(hr) && m_pIo->IsAligned(Actual.cbAlign))
        {
            pPreferred->AddRef();
            *ppActual = pPreferred;
            return S_OK;
        }
    }

    // create our own allocator
    IMemAllocator* pAlloc;
    hr = InitAllocator(&pAlloc);
    if(FAILED(hr))
    {
        return hr;
    }

    //...and see if we can make it suitable
    hr = pAlloc->SetProperties(pProps, &Actual);
    if(SUCCEEDED(hr) && m_pIo->IsAligned(Actual.cbAlign))
    {
        // we need to release our refcount on pAlloc, and addref
        // it to pass a refcount to the caller - this is a net nothing.
        *ppActual = pAlloc;
        return S_OK;
    }

    // failed to find a suitable allocator
    pAlloc->Release();

    // if we failed because of the IsAligned test, the error code will
    // not be failure
    if(SUCCEEDED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::RequestAllocator VFW_E_BADALIGN");
		g_log.Put(str);


        hr = VFW_E_BADALIGN;
    }
    return hr;
}


// queue an aligned read request. call WaitForNext to get
// completion.
STDMETHODIMP CAsyncOutputPin::Request(
    IMediaSample* pSample,
    DWORD_PTR dwUser)           // user context
{
    CheckPointer(pSample,E_POINTER);

	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::Request");
	g_log.Put(str);

    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::Request pSample->GetTime failed hr = %08x", hr);
		g_log.Put(str);

		return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal=0, llAvailable=0;

    hr = m_pIo->Length(&llTotal, &llAvailable);
    if(llPos + lLength > llTotal)
    {
        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pIo->Alignment(&lAlign);

        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if(llPos + lLength > llTotal)
        {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }

    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::Request pSample->GetPointer failed hr = %08x", hr);
		g_log.Put(str);

        return hr;
    }

    hr =  m_pIo->Request(llPos,
                          lLength,
                          TRUE,
                          pBuffer,
                          (LPVOID)pSample,
                          dwUser);
	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::Request m_pIo->Request failed hr = %08x", hr);
		g_log.Put(str);
	}
	return hr;
}


// sync-aligned request. just like a request/waitfornext pair.
STDMETHODIMP 
CAsyncOutputPin::SyncReadAligned(
                  IMediaSample* pSample)
{
    CheckPointer(pSample,E_POINTER);

	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::SyncReadAligned");
	g_log.Put(str);

    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::SyncReadAligned pSample->GetTime failed hr = %08x", hr);
		g_log.Put(str);
        return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal;
    LONGLONG llAvailable;

    hr = m_pIo->Length(&llTotal, &llAvailable);
    if(llPos + lLength > llTotal)
    {
        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pIo->Alignment(&lAlign);

        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if(llPos + lLength > llTotal)
        {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }

    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if(FAILED(hr))
    {
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::SyncReadAligned pSample->GetPointer failed hr = %08x", hr);
		g_log.Put(str);

        return hr;
    }

    LONG cbActual;
    hr = m_pIo->SyncReadAligned(llPos,
                                lLength,
                                pBuffer,
                                &cbActual,
                                pSample);

	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::SyncReadAligned m_pIo->SyncReadAligned failed hr = %08x", hr);
		g_log.Put(str);
	}
    pSample->SetActualDataLength(cbActual);
    return hr;
}


//
// collect the next ready sample
STDMETHODIMP
CAsyncOutputPin::WaitForNext(
    DWORD dwTimeout,
    IMediaSample** ppSample,  // completed sample
    DWORD_PTR * pdwUser)        // user context
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::WaitForNext");
	g_log.Put(str);

    CheckPointer(ppSample,E_POINTER);

    LONG cbActual;
    IMediaSample* pSample=0;

    HRESULT hr = m_pIo->WaitForNext(dwTimeout,
                                    (LPVOID*) &pSample,
                                    pdwUser,
                                    &cbActual);

    if(SUCCEEDED(hr))
    {
        pSample->SetActualDataLength(cbActual);
    }

    *ppSample = pSample;

	memset(str,0,256);
	sprintf_s(str, 256, "CAsyncOutputPin::WaitForNext failed m_pIo->WaitForNext hr = %08x", hr);
	g_log.Put(str);

    return hr;
}


//
// synchronous read that need not be aligned.
STDMETHODIMP
CAsyncOutputPin::SyncRead(
    LONGLONG llPosition,    // absolute Io position
    LONG lLength,       // nr bytes required
    BYTE* pBuffer)      // write data here
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::SyncRead");
	g_log.Put(str);

    HRESULT hr =  m_pIo->SyncRead(llPosition, lLength, pBuffer);
	
	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::SyncRead failed m_pIo->SyncRead hr = %08x", hr);
		g_log.Put(str);
	}

	return hr;
}


// return the length of the file, and the length currently
// available locally. We only support locally accessible files,
// so they are always the same
STDMETHODIMP
CAsyncOutputPin::Length(
    LONGLONG* pTotal,
    LONGLONG* pAvailable)
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::Length");
	g_log.Put(str);

    HRESULT hr =  m_pIo->Length(pTotal, pAvailable);
	
	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::Length failed m_pIo->Length hr = %08x", hr);
		g_log.Put(str);
		return hr;
	}

	return hr;
}


STDMETHODIMP
CAsyncOutputPin::BeginFlush(void)
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::BeginFlush");
	g_log.Put(str);

    HRESULT hr = m_pIo->BeginFlush();
	
	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::Length failed m_pIo->Length hr = %08x", hr);
		g_log.Put(str);
		return hr;
	}

	return hr;
}


STDMETHODIMP
CAsyncOutputPin::EndFlush(void)
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::EndFlush");
	g_log.Put(str);

    HRESULT hr =  m_pIo->EndFlush();
	
	if (FAILED(hr))
	{
		char str[256];
		sprintf_s(str, 256, "CAsyncOutputPin::EndFlush failed m_pIo->EndFlush hr = %08x", hr);
		g_log.Put(str);
		return hr;
	}

	return hr;
}


STDMETHODIMP
CAsyncOutputPin::Connect(
    IPin * pReceivePin,
    const AM_MEDIA_TYPE *pmt   // optional media type
)
{
	char str[256];
	sprintf_s(str, 256, "CAsyncOutputPin::Connect");
	g_log.Put(str);

    HRESULT hr = m_pReader->Connect(pReceivePin, pmt);

	if (FAILED(hr))
	{
		char str[256];
		sprintf(str, "CAsyncOutputPin::Connect failed m_pReader->Connect hr = %08x", hr);
		g_log.Put(str);
		return hr;
	}

	memset(str,0,256);
	sprintf_s(str, 256, "CAsyncOutputPin::Connect ok");
	g_log.Put(str);
	
	return hr;
}



// --- CAsyncReader implementation ---

#pragma warning(disable:4355)

CAsyncReader::CAsyncReader(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    CAsyncStream *pStream,
    HRESULT *phr)
  : CBaseFilter(
                pName,
                pUnk,
                &m_csFilter,
                CLSID_SwfSrc,
                NULL
                ),
    m_apin(
                phr,
                this,
                &m_Io,
                &m_csFilter),
    m_Io(pStream)
{
}

CAsyncReader::~CAsyncReader()
{
}

int CAsyncReader::GetPinCount()
{
    return 1;
}

CBasePin * CAsyncReader::GetPin(int n)
{
    if((GetPinCount() > 0) && (n == 0))
    {
        return &m_apin;
    }
    else
    {
        return NULL;
    }
}



