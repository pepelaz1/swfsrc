#include "videopin.h"
#include "parser.h"
#include "asyncio.h"
#include "asyncrdr.h"
#include "filter.h"
#include "logger.h"

CLogger g_log;

CVideoPin::CVideoPin(CSwfFilter *flt, CCritSec *lock, HRESULT *phr) 
	: CBaseOutputPin("Swf Video Pin", (CBaseFilter *)flt, lock, phr, L"Video"),
	m_pFilter(flt),
	m_file(""),
	m_framerate(0),
	m_vwidth(0),
	m_vheight(0),
	m_framecnt(0),
	m_bDiscontinuity(true),
	m_hThread22(0),
	m_hThread33(0)
{	
}

CVideoPin::~CVideoPin()
{
	CloseThreads();
}

void CVideoPin::CloseThreads()
{
	WaitForSingleObject(m_hThread22, INFINITE);
	WaitForSingleObject(m_hThread33, INFINITE);

	CloseHandle(m_hThread22);
	CloseHandle(m_hThread33);
	m_hThread22 = NULL;
	m_hThread33 = NULL;
}


void CVideoPin::SetFile(string file)
{
	m_file = file;
	CSwfParser p(m_file);
	p.Process();
	m_framerate = p.FrameRate();
	m_vwidth = p.Width();
	m_vheight = p.Height();
	m_framecnt = p.FrameCnt();

	m_framerate = 10;
	m_framecnt = 100;

	m_bottomup = false;
	m_painter.Init(m_file,  m_vwidth, m_vheight, m_framecnt);
	
	char str[256];
	sprintf_s(str,256,"CVideoPin::SetFile");
	g_log.Put(str);
}

STDMETHODIMP CVideoPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking) 
	{
		return GetInterface((IMediaSeeking *)this, ppv);
	} 
	else {
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}
}

HRESULT CVideoPin::CheckMediaType(const CMediaType *pmt)
{
	if (IsEqualGUID(*pmt->Type(), MEDIATYPE_Video))
	{
		if (IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB32))
		{
			return S_OK;
		}
	}
	return S_FALSE;
}

HRESULT CVideoPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
    HRESULT hr;
  //  CAutoLock cAutoLock(m_pFilter->pStateLock());

    CheckPointer(pAlloc, E_POINTER);
    CheckPointer(pRequest, E_POINTER);

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*) m_mt.Format();
    
    // Ensure a minimum number of buffers
    if (pRequest->cBuffers == 0)
    {
        pRequest->cBuffers = 2;
    }
    pRequest->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pRequest, &Actual);
    if (FAILED(hr)) 
    {
        return hr;
    }

    // Is this allocator unsuitable?
    if (Actual.cbBuffer < pRequest->cbBuffer) 
    {
        return E_FAIL;
    }

    return S_OK;
}


//
// ThreadProc
//
// When this returns the thread exits
// Return codes > 0 indicate an error occured
DWORD CVideoPin::ThreadProc(void) {

    HRESULT hr;  // the return code from calls
    Command com;

    do {
	com = GetRequest();
	if (com != CMD_INIT) {
	    DbgLog((LOG_ERROR, 1, TEXT("Thread expected init command")));
	    Reply((DWORD) E_UNEXPECTED);
	}
    } while (com != CMD_INIT);

    DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread initializing")));

    hr = OnThreadCreate(); // perform set up tasks
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadCreate failed. Aborting thread.")));
		OnThreadDestroy();
		Reply(hr);	// send failed return code from OnThreadCreate
		return 1;
	}

    // Initialisation suceeded
    Reply(NOERROR);

    Command cmd;
	do {
		cmd = GetRequest();

		switch (cmd) {

		case CMD_EXIT:
			Reply(NOERROR);
			break;

		case CMD_RUN:
			DbgLog((LOG_ERROR, 1, TEXT("CMD_RUN received before a CMD_PAUSE???")));
			// !!! fall through???

		case CMD_PAUSE:
			Reply(NOERROR);
			DoBufferProcessingLoop();
			break;

		case CMD_STOP:
			Reply(NOERROR);
			break;

		default:
			DbgLog((LOG_ERROR, 1, TEXT("Unknown command %d received!"), cmd));
			Reply((DWORD) E_NOTIMPL);
			break;
		}
	} while (cmd != CMD_EXIT);

    hr = OnThreadDestroy();	// tidy up.
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadDestroy failed. Exiting thread.")));
        return 1;
    }

    DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread exiting")));
    return 0;
}

#define UNITS 10000000 

HRESULT CVideoPin::GetMediaType(int iPosition, CMediaType *pmt)
{

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pmt,E_POINTER);

    VIDEOINFO *pvi = (VIDEOINFO *)pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(!pvi)
        return E_OUTOFMEMORY;

	pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 32;
	pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_vwidth;
    pvi->bmiHeader.biHeight     = m_vheight;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;
	pvi->bmiHeader.biClrUsed = 0;
	pvi->bmiHeader.biXPelsPerMeter = 0;
	pvi->bmiHeader.biYPelsPerMeter = 0;
	pvi->AvgTimePerFrame = (REFERENCE_TIME)(UNITS / m_framerate);
	pvi->dwBitErrorRate = 0;
	pvi->dwBitRate = 0;
	

    SetRectEmpty(&(pvi->rcSource)); 
	pvi->rcSource.right = m_vwidth;
	pvi->rcSource.bottom = m_vheight;

    SetRectEmpty(&(pvi->rcTarget)); 
	pvi->rcTarget.right = m_vwidth;
	pvi->rcTarget.bottom = m_vheight;


	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetSubtype(&MEDIASUBTYPE_RGB32);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
	pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

	return S_OK;
}

//
// Active
//
// The pin is active - start up the worker thread
HRESULT CVideoPin::Active()
{
	CAutoLock lock(m_pFilter->GetStateLock());

	char str[256];
	sprintf_s(str,256,"CVideoPin::Active");
	g_log.Put(str);


    HRESULT hr;

    if (m_pFilter->IsActive()) {
		return S_FALSE;	// succeeded, but did not allocate resources (they already exist...)
    }

    // do nothing if not connected - its ok not to connect to
    // all pins of a source filter
    if (!IsConnected()) {
        return NOERROR;
    }

    hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }

    ASSERT(!ThreadExists());

    // start the thread
    if (!Create()) {
        return E_FAIL;
    }

    // Tell thread to initialize. If OnThreadCreate Fails, so does this.
    hr = Init();
    if (FAILED(hr))
	return hr;

    return Pause();
}


//
// Inactive
//
// Pin is inactive - shut down the worker thread
// Waits for the worker to exit before returning.
HRESULT CVideoPin::Inactive(void) 
{
	CAutoLock lock(m_pFilter->GetStateLock());

	char str[256];
	sprintf_s(str,256,"CVideoPin::Inactive");
	g_log.Put(str);

	HRESULT hr;

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) {
		return NOERROR;
	}

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr)) {
		return hr;
	}

	if (ThreadExists()) {
		hr = Stop();

		if (FAILED(hr)) {
			return hr;
		}

		hr = Exit();
		if (FAILED(hr)) {
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
	}

	// hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	//if (FAILED(hr)) {
	//	return hr;
	//}

	return NOERROR;
}

//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CVideoPin::DoBufferProcessingLoop(void) {

	Command com;

	OnThreadStartPlay();

	do {
		while (!CheckRequest(&com))
		{
			IMediaSample *pSample;

			HRESULT hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
			if (FAILED(hr)) {
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
				// or the allocator is decommited & we will be asked to
				// exit soon.
			}


			// Virtual function user will override.
			hr = FillBuffer(pSample);

			if (hr == S_OK) 
			{
				char str[256];
				sprintf_s(str,256,"CVideoPin::DoBufferProcessingLoop before Deliver");
				g_log.Put(str);

				
				hr = Deliver(pSample);
				pSample->Release();

				memset(str,0,256);
				sprintf_s(str,256,"CVideoPin::DoBufferProcessingLoop after deliver Deliver, result = %d",hr);
				g_log.Put(str);

				// downstream filter returns S_FALSE if it wants us to
				// stop or an error if it's reporting an error.
				if(hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					
					char str[256];
					sprintf_s(str,256,"CVideoPin::DoBufferProcessingLoop Deliver() returned %08x; stopping",hr);
					g_log.Put(str);

					return S_OK;
				}

			} else if (hr == S_FALSE) {
				// derived class wants us to stop pushing data
				pSample->Release();
				DeliverEndOfStream();
				return S_OK;
			} else {
				// derived class encountered an error
				pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			// all paths release the sample
		}
		// For all commands sent to us there must be a Reply call!

		if (com == CMD_RUN || com == CMD_PAUSE) {
			Reply(NOERROR);
		} else if (com != CMD_STOP) {
			Reply((DWORD) E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}

HRESULT CVideoPin::OnThreadCreate() 
{
	m_painter.OnPlay();

	char str[256];
	sprintf_s(str,256,"CVideoPin::OnThreadCreate");
	g_log.Put(str);
	
	return S_OK;
}

HRESULT CVideoPin::OnThreadDestroy()
{
	m_painter.OnStop();
	
	char str[256];
	sprintf_s(str,256,"CVideoPin::OnThreadDestroy");
	g_log.Put(str);

	return S_OK;
}

HRESULT CVideoPin::OnThreadStartPlay()
{
	CAutoLock lck(&m_lock);
	int frameno = m_painter.CurrentFrameNo();
	//REFERENCE_TIME tStart = (LONGLONG)(frameno * (UNITS/m_framerate));
	//m_rtStart = (LONGLONG)(frameno * (UNITS/m_framerate));
	m_rtStart = 0;
	REFERENCE_TIME tEnd = (LONGLONG)(m_painter.FrameCount() * (UNITS/m_framerate));
	m_bDiscontinuity = true;

	char str[256];
	sprintf_s(str,256,"CVideoPin::OnThreadStartPlay");
	g_log.Put(str);

	return DeliverNewSegment(m_rtStart, tEnd, 1.0);
}


HRESULT CVideoPin::FillBuffer(IMediaSample *pSample)
{
	CAutoLock lock(m_pFilter->GetStateLock());
	CComQIPtr<IMediaSample2>pSample2 = pSample;
	AM_SAMPLE2_PROPERTIES props;
	pSample2->GetProperties(sizeof(AM_SAMPLE2_PROPERTIES), (BYTE *)&props);
	
	AM_MEDIA_TYPE *pmt = NULL;
	HRESULT hr = pSample->GetMediaType(&pmt);
	if (SUCCEEDED(hr) && pmt)
	{
		VIDEOINFO *pvi = (VIDEOINFO *)pmt->pbFormat;
		m_bottomup = (pvi->bmiHeader.biHeight < 0);
		DeleteMediaType(pmt);
	}
	
	BYTE *p = NULL;                
    pSample->GetPointer(&p);
	long size = pSample->GetActualDataLength();

	CAutoLock lck(&m_lock);
	int frameno = m_painter.OnDraw(p,size, m_bottomup);
	if (frameno < 0)
		return S_FALSE;

	int duration = (int)(UNITS / m_framerate);
	REFERENCE_TIME rtStart = m_rtStart;
	REFERENCE_TIME rtStop  = rtStart + duration;
	m_rtStart += duration;
	
	pSample->SetTime(&rtStart, &rtStop);
	
	char str[256];
	sprintf_s(str,256,"CVideoPin::FillBuffer duration=%d, rtStart=%d, rtStop=%d, num=%d", duration, (int)rtStart, (int)rtStop, frameno);
	g_log.Put(str);
	
	//pSample->SetMediaTime(&rtStart, &rtStop);
	pSample->SetSyncPoint(TRUE);
	pSample->SetPreroll(FALSE);

	if (m_bDiscontinuity)
	{
		char str[256];
		sprintf_s(str,256,"CVideoPin::FillBuffer Discontinuity");
		g_log.Put(str);

		pSample->SetDiscontinuity(m_bDiscontinuity);
		m_bDiscontinuity = false;
	}
	//Sleep(3);
	return S_OK;
}


STDMETHODIMP CVideoPin::GetDuration(LONGLONG *pDuration)
{
	*pDuration = (LONGLONG)(m_painter.FrameCount() * (UNITS/m_framerate));
	return S_OK;
}

STDMETHODIMP CVideoPin::GetCapabilities(DWORD *pCapabilities)
{
	*pCapabilities =  AM_SEEKING_CanSeekAbsolute 
		|  AM_SEEKING_CanGetDuration
		|  AM_SEEKING_CanSeekForwards
		|  AM_SEEKING_CanSeekBackwards;
	return S_OK;
}

STDMETHODIMP CVideoPin::IsFormatSupported(const GUID *pFormat)
{
	if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat))
		return S_OK;
	return E_FAIL;
}

STDMETHODIMP CVideoPin::IsUsingTimeFormat(const GUID *pFormat)
{
	if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat))
		return S_OK;
	return E_FAIL;
}

STDMETHODIMP CVideoPin::GetPreroll(LONGLONG *pllPreroll)
{
	*pllPreroll = 0;
	return S_OK;
}


STDMETHODIMP CVideoPin::GetStopPosition(LONGLONG *pStop)
{
	*pStop = (LONGLONG)(m_painter.FrameCount() * (UNITS/m_framerate));// * 100000;
	return S_OK;
}

STDMETHODIMP CVideoPin::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,  DWORD dwStopFlags)
{

	DWORD StopPosBits = dwStopFlags & AM_SEEKING_PositioningBitsMask;

	if ( dwCurrentFlags & AM_SEEKING_AbsolutePositioning)
	{
		{
			CAutoLock lck(&m_lock);
			int frameno = (int)(*pCurrent / (UNITS/m_framerate));
			m_painter.SetFrameNo(frameno);
		}
		

		char str[256];
				
		LONGLONG start = !pCurrent ? 0 : *pCurrent;
		LONGLONG end = !pStop ? 0 : *pStop;

		sprintf(str,"CVideoPin::SetPositions 1 current=%I64d, stop=%I64d, stopposbits=%d", start, end, StopPosBits);
		g_log.Put(str);

		if (ThreadExists()) 
		{
			memset(str,0,256);
			sprintf_s(str,256,"CVideoPin::SetPositions 2");
			g_log.Put(str);

			DeliverBeginFlush();

			memset(str,0,256);
			sprintf_s(str,256,"CVideoPin::SetPositions 3");
			g_log.Put(str);

			ExecuteStop();

			memset(str,0,256);
			sprintf_s(str,256,"CVideoPin::SetPositions 4");
			g_log.Put(str);

			memset(str,0,256);
			sprintf_s(str,256,"CVideoPin::SetPositions 5");
			g_log.Put(str);

			DeliverEndFlush();

			memset(str,0,256);
			sprintf_s(str,256,"CVideoPin::SetPositions 6");
			g_log.Put(str);

			ExecuteRun();
		}		


		//char str[256];
		memset(str,0,256);
		sprintf_s(str,256,"CVideoPin::SetPositions 7");
		g_log.Put(str);
	}

	return S_OK;
}


void CVideoPin::ExecuteRun()
{
	DWORD dwID = 0;
	if(m_hThread22)
	{
		CloseHandle(m_hThread22);
		m_hThread22 = NULL;
	}
	m_hThread22 = CreateThread(NULL, 0, RunThreadEntry, (LPVOID)this, 0, &dwID);
	WaitForSingleObject(m_hThread22, INFINITE);
}

DWORD __stdcall CVideoPin::RunThreadEntry(LPVOID lpParam)
{
	CVideoPin *pin = (CVideoPin*)lpParam;
	pin->Run();
	return 0;
}

void CVideoPin::ExecuteStop()
{
	DWORD dwID = 0;
	if(m_hThread33)
	{
		CloseHandle(m_hThread33);
		m_hThread33 = NULL;
	}
	m_hThread33 = CreateThread(NULL, 0, StopThreadEntry, (LPVOID)this, 0, &dwID);
	WaitForSingleObject(m_hThread33, INFINITE);
}

DWORD __stdcall CVideoPin::StopThreadEntry(LPVOID lpParam)
{
	CVideoPin *pin = (CVideoPin*)lpParam;
	pin->Stop();
	return 0;
}


STDMETHODIMP CVideoPin::SetSink(IQualityControl * piqc)
{
	return CBaseOutputPin::SetSink(piqc);
}

STDMETHODIMP CVideoPin::Notify(IBaseFilter * pSender, Quality q)
{
    UNREFERENCED_PARAMETER(pSender);
    ValidateReadPtr(pSender,sizeof(IBaseFilter));

	char str[256];
	sprintf_s(str,256,"CVideoPin::Notify");
	g_log.Put(str);
	
    // First see if we want to handle this ourselves
    //HRESULT hr = m_pTransformFilter->AlterQuality(q);
   // if (hr!=S_FALSE) {
   //     return hr;        // either S_OK or a failure
    //}

    // S_FALSE means we pass the message on.
    // Find the quality sink for our input pin and send it there

    //ASSERT(m_pTransformFilter->m_pInput != NULL);

    //turn m_pTransformFilter->m_pInput->PassNotify(q);
	return S_OK;

} // Notify
