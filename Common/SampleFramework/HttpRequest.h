//------------------------------------------------------------------------------
// HttpRequest.h
//
// An example use of IXMLHTTPRequest2Callback interface presented
// as a simplified HTTP request object.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once
#ifdef _XBOX_ONE

#include <ixmlhttprequest2.h>
#include <wrl.h>
#include <Windows.h>
#include <atomic>
#include <ppltasks.h>
#include <concrt.h>
#include "BasicTimer.h"

namespace XboxSampleFramework
{

//------------------------------------------------------------------------------
// Name: HttpHeaderInfo
// Desc: Headers which can be submitted to Http Requests
//------------------------------------------------------------------------------
struct HttpHeaderInfo
{
    std::wstring Name;
    std::wstring Value;
    bool         UsedForXSTSToken;

    HttpHeaderInfo() : UsedForXSTSToken(FALSE) {}
    HttpHeaderInfo( const std::wstring& inName, const std::wstring& inValue, bool inShouldSubmitToXSTS ) :
        Name(inName), Value(inValue), UsedForXSTSToken(inShouldSubmitToXSTS) {}
    HttpHeaderInfo( const HttpHeaderInfo& other ) : 
        Name(other.Name), Value(other.Value), UsedForXSTSToken(other.UsedForXSTSToken) {}
    HttpHeaderInfo& operator = ( const HttpHeaderInfo& other )
    {
        if (&other != this)
        {
            Name = other.Name;
            Value = other.Value;
            UsedForXSTSToken = other.UsedForXSTSToken;
        }
        return *this;
    }
};

//------------------------------------------------------------------------------
// Name: HttpCallback
// Desc: Implement the IXMLHTTPRequest2Callback functions for our sample with
//       basic error reporting and an Event signalling when the request is
//       complete.
//------------------------------------------------------------------------------
class HttpCallback : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IXMLHTTPRequest2Callback>
{
public:

    HttpCallback();
    ~HttpCallback();

    friend HRESULT Microsoft::WRL::MakeAndInitialize<HttpCallback,HttpCallback>( HttpCallback ** );
    STDMETHODIMP RuntimeClassInitialize();

    // Required functions
    STDMETHODIMP OnRedirect( IXMLHTTPRequest2 *pXHR, const WCHAR *pwszRedirectUrl );
    STDMETHODIMP OnHeadersAvailable( IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const WCHAR *pwszStatus );
    STDMETHODIMP OnDataAvailable( IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream );
    STDMETHODIMP OnResponseReceived( IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream );
    STDMETHODIMP OnError( IXMLHTTPRequest2 *pXHR, HRESULT hrError );

    bool    IsCompleted();
    bool    WaitForCompletion( const DWORD dwTimeoutInMs );
    bool    CompleteWithResult( const HRESULT hr );

    HRESULT GetHR() const { return m_hr; };
    DWORD   GetHTTPStatus() const { return m_httpStatus; };

    const std::wstring& GetHeaders() const { return m_headers; };
    const std::vector<BYTE>& GetData() const { return m_data; }

private:

    HRESULT ReadDataFromStream( ISequentialStream *pStream );

    concurrency::event      m_completionEvent;
    std::atomic<HRESULT>    m_hr;
    DWORD                   m_httpStatus;
    std::wstring            m_headers;
    std::vector<BYTE>       m_data;
};

// ----------------------------------------------------------------------------
// Name: HttpRequestStream
// Desc: Encapsulates a request data stream. It inherits ISequentialStream,
// which the IXMLHTTPRequest2 class uses to read from our buffer. It also 
// inherits IDispatch, which the IXMLHTTPRequest2 interface on Xbox One requires 
// (unlike on Windows, where only ISequentialStream is necessary).
// ----------------------------------------------------------------------------
class HttpRequestStream : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, ISequentialStream, IDispatch>
{
public:

	HttpRequestStream();
    ~HttpRequestStream();

    // ISequentialStream
    STDMETHODIMP Open( const void *psBuffer, ULONG cbBufferSize );
    STDMETHODIMP Read( void *pv, ULONG cb, ULONG *pcbRead );
    STDMETHODIMP Write( const void *pv,  ULONG cb, ULONG *pcbWritten );

    //Helper
    STDMETHODIMP_(ULONGLONG) Size();

    //IUnknown
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface( REFIID riid, void **ppvObject );

	//IDispatch
    STDMETHODIMP GetTypeInfoCount( unsigned int FAR*  pctinfo );
    STDMETHODIMP GetTypeInfo( unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR*  ppTInfo );
    STDMETHODIMP GetIDsOfNames( REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId );
    STDMETHODIMP Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pDispParams,
        VARIANT FAR* pVarResult,
        EXCEPINFO FAR* pExcepInfo,
        unsigned int FAR* puArgErr);

private:

    LONG    m_cRef;
    BYTE*   m_pBuffer;
    size_t  m_buffSize;
    size_t  m_buffSeekIndex;
};

// ----------------------------------------------------------------------------
// Name: HttpRequest
// Desc: Encapsulates a single HTTP request.
// ----------------------------------------------------------------------------
class HttpRequest : public std::enable_shared_from_this<HttpRequest>
{
public:

    HttpRequest();
    ~HttpRequest();

    HRESULT Open( Windows::Xbox::System::User^ user, const std::wstring& verb, const std::wstring& url );
    HRESULT Open( Windows::Xbox::System::User^ user, const std::wstring& verb, const std::wstring& url, 
        const std::wstring& contentType, const BYTE* contentBytes, size_t contentLength );
    HRESULT Open( Windows::Xbox::System::User^ user, const std::wstring& verb, const std::wstring& url, 
        const std::wstring& contentType, const BYTE* contentBytes, size_t contentLength, 
        std::vector<HttpHeaderInfo> headers );

    void    Abort();

    // Polling
    bool    IsResponseReceived();
    // Blocking
    bool    WaitForResponse( const DWORD dwTimeoutInMs=INFINITE );
    // Async
    concurrency::task<bool> WaitForResponseAsync( const DWORD dwTimeoutInMs=INFINITE );

    HRESULT GetResult() const;
    DWORD   GetStatus() const;

    const std::wstring& GetHeaders() const;
    const std::vector<BYTE>& GetDataBuffer() const;
    std::wstring GetDataAsUTF8String() const;

    // NOTE: Enabling XSTS directly on the HttpRequest object will cause the Open()
    // call to do a blocking fetch of the XSTS token and signature
    // This NOT recommended for general use and is only here for demonstration
    // The title should ideally be using the automatic token insertion which 
    // is based on the web service endpoint definitions in XDP (Xbox Developer Portal)
    // OR the title code should be caching the token and signature for the service 
    // and inserting them as headers so that they do not need to be looked up for each call
    bool    UseXSTSToken() const { return m_bUseXSTSToken; };
    void    SetUseXSTSToken( bool useXSTSToken ) { m_bUseXSTSToken = useXSTSToken; };

    // The timeout is applied to the overall duration of the request
    // We will fail the request if it exceeds the timeout duration
    DWORD   GetTimeout() const { return m_dwTimeoutInMs; }
    void    SetTimeout( DWORD dwTimeoutInMs ) { m_dwTimeoutInMs = dwTimeoutInMs; }
    
public:

    // Default settings
    static std::wstring GetDefaultUserAgent();
    static std::vector<HttpHeaderInfo> GetDefaultHeaders( bool useXSTSToken );

    // Helper method
    static std::wstring BufferToUTF8( const std::vector<BYTE>& buffer );

private:

    HRESULT SendRequest();
    void    CancelRequest(HRESULT hr);

    Microsoft::WRL::ComPtr<IXMLHTTPRequest2>            m_pXHR;
    Microsoft::WRL::ComPtr<IXMLHTTPRequest2Callback>    m_pXHRCallback;
    Microsoft::WRL::ComPtr<HttpCallback>                m_pHttpCallback;
    Microsoft::WRL::ComPtr<HttpRequestStream>           m_pHttpRequestStream;

    bool        m_bUseXSTSToken;
    DWORD       m_dwTimeoutInMs;
    BasicTimer  m_timer;
};

}  // namespace XboxSampleFramework

#endif   // _XBOX_ONE