#pragma once
#include <streams.h>
#include <string>
using namespace std;
#include "painter.h"

//CSourceStream
class CSwfFilter;
class CVideoPin : public CBaseOutputPin, public CAMThread, public IMediaSeeking
{
public:
	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
private:
	string m_file;
	double m_framerate;
	int m_vwidth, m_vheight;
	int m_framecnt;
	CSwfFilter *m_pFilter;
	CSwfPainter m_painter;
	bool m_bDiscontinuity;
	CCritSec m_lock;
	REFERENCE_TIME m_rtStart;
	bool m_bottomup;
public:
    // thread commands
    enum Command {CMD_INIT, CMD_PAUSE, CMD_RUN, CMD_STOP, CMD_EXIT};
    HRESULT Init(void) { return CallWorker(CMD_INIT); }
    HRESULT Exit(void) { return CallWorker(CMD_EXIT); }
    HRESULT Run(void) { return CallWorker(CMD_RUN); }
    HRESULT Pause(void) { return CallWorker(CMD_PAUSE); }
    HRESULT Stop(void) { return CallWorker(CMD_STOP); }
protected:
    HRESULT Active();    // Starts up the worker thread
    HRESULT Inactive();  // Exits the worker thread.
	virtual DWORD ThreadProc();  	
	virtual HRESULT DoBufferProcessingLoop(void);    // the loop executed whilst running
	virtual HRESULT FillBuffer(IMediaSample *pSample);

    // Called as the thread is created/destroyed - use to perform
    // jobs such as start/stop streaming mode
    // If OnThreadCreate returns an error the thread will exit.
    virtual HRESULT OnThreadCreate(void);
    virtual HRESULT OnThreadDestroy(void);
    virtual HRESULT OnThreadStartPlay(void);

    Command GetRequest(void) { return (Command) CAMThread::GetRequest(); }
    BOOL    CheckRequest(Command *pCom) { return CAMThread::CheckRequest( (DWORD *) pCom); }

public:
	HRESULT CheckMediaType(const CMediaType *);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	CVideoPin(CSwfFilter *flt, CCritSec *lock, HRESULT *phr);
	~CVideoPin();
	void SetFile(string file);
public:
	// Seeking
	/*HRESULT GetDuration(LONGLONG *pDuration);
	HRESULT GetCapabilities(DWORD *pCapabilities);
	HRESULT IsFormatSupported(const GUID *pFormat);
	HRESULT IsUsingTimeFormat(const GUID *pFormat);
	HRESULT GetPreroll(LONGLONG *pllPreroll);
	HRESULT GetStopPosition(LONGLONG *pStop);
	HRESULT SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop,  DWORD dwStopFlags);*/

public:
    // inherited from IQualityControl via CBasePin
	STDMETHODIMP SetSink(IQualityControl * piqc);
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

private:
	static DWORD __stdcall RunThreadEntry(LPVOID lpParam);
	static DWORD __stdcall StopThreadEntry(LPVOID lpParam);

	HANDLE	m_hThread22;
	HANDLE	m_hThread33;

	void ExecuteStop();
	void ExecuteRun();
	void CloseThreads();

public:
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

