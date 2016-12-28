//------------------------------------------------------------------------------
// HttpRequest.cpp
//
// An example use of IXMLHTTPRequest2Callback
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "pch.h"
#include "HttpRequest.h"

using namespace Microsoft::WRL;
using namespace Windows::Xbox::System;
using namespace XboxSampleFramework;

// We've set the max size for our outbound request stream to 4MB. 
// You can choose to set it to variable or to what's appropriate for your scenario
static const int HTTP_REQUEST_MAX_BUFFER_SIZE = 4 * 1024 * 1024;
// Read incoming streams in chunks of 16K
static const int HTTP_RESPONSE_READ_CHUNK_SIZE = 16 * 1024;

// --------------------------------------------------------------------------------------
// Name: HttpCallback::HttpCallback
// Desc: Constructor
// --------------------------------------------------------------------------------------
HttpCallback::HttpCallback() : 
    m_hr(S_FALSE), m_httpStatus(0), m_headers(), m_data()
{
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::~HttpCallback
// Desc: Destructor
// --------------------------------------------------------------------------------------
HttpCallback::~HttpCallback()
{
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::RuntimeClassInitialize
// Desc: Used by WRL to instance the COM object
// --------------------------------------------------------------------------------------
STDMETHODIMP HttpCallback::RuntimeClassInitialize()
{
    return S_OK;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::OnRedirect
// Desc: The requested URI was redirected by the HTTP server to a new URI.
// Arguments:
//     pXHR         - The interface pointer of originating IXMLHTTPRequest2 object.
//     pRedirectUrl - The new URL to for the request.
// --------------------------------------------------------------------------------------
IFACEMETHODIMP HttpCallback::OnRedirect( IXMLHTTPRequest2* pXHR, const wchar_t* pRedirectUrl )
{
    UNREFERENCED_PARAMETER(pXHR);
    UNREFERENCED_PARAMETER(pRedirectUrl);

    // If the URI was redirected by the HTTP server to a new URI, do nothing.

    return S_OK;
};

// --------------------------------------------------------------------------------------
// Name: HttpCallback::OnHeadersAvailable
// Desc: The HTTP Headers have been downloaded and are ready for parsing. The string that is
//       returned is owned by this function and should be copied or deleted before exit.
// Arguments:
//     pXHR       - The interface pointer of originating IXMLHTTPRequest2 object.
//     dwStatus   - The value of HTTP status code, e.g. 200, 404
//     pwszStatus - The description text of HTTP status code.
// --------------------------------------------------------------------------------------
IFACEMETHODIMP HttpCallback::OnHeadersAvailable( IXMLHTTPRequest2* pXHR, DWORD dwStatus, const wchar_t* pwszStatus )
{
    UNREFERENCED_PARAMETER(pwszStatus);
    
    // We need a pointer to the originating HttpRequest object, otherwise this
    // makes no sense.
    if( pXHR == nullptr )
    {
        return E_INVALIDARG;
    }

    // Get all response headers. We could equally select a single header using:
    //     hr = pXHR->GetResponseHeader(L"Content-Length", &pwszContentLength);
    wchar_t* headers = nullptr;
    HRESULT hr = pXHR->GetAllResponseHeaders( &headers );
    if( SUCCEEDED( hr ) )
    {
        // Take a copy of the header data to the local wstring.
        m_headers += headers;
    }

    // The header string that was passed in needs to be deleted here.
    if( headers != nullptr )
    {
        ::CoTaskMemFree( headers );
        headers = nullptr;
    }

    // Copy the http status for later use.
    m_httpStatus = dwStatus;
    XboxSampleFramework::DebugPrint( "IXMLHTTP: OnHeadersAvailable: Status code: %u\n", m_httpStatus );

    return hr;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::OnDataAvailable
// Desc: Part of the HTTP Data payload is available, we can start processing it
//       here or copy it off and wait for the whole request to finish loading.
// Arguments:
//    pXHR            - Pointer to the originating IXMLHTTPRequest2 object.
//    pResponseStream - Pointer to the input stream, which may only be part of the
//                      whole stream.
// --------------------------------------------------------------------------------------
IFACEMETHODIMP HttpCallback::OnDataAvailable( IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream )
{
    UNREFERENCED_PARAMETER( pXHR );
#ifdef XSF_USE_DX_12_0
    XSFScopedNamedEvent( static_cast<XSF::D3DCommandQueue*>(nullptr), 0, L"HttpCallback::OnDataAvailable" );
#else
    XSFScopedNamedEvent( nullptr, 0, L"HttpCallback::OnDataAvailable" );
#endif

    // Add the contents of the stream to our running result.
    const HRESULT hr = ReadDataFromStream( pResponseStream );
    XboxSampleFramework::DebugPrint( "IXMLHTTP: OnDataAvailable: 0x%x\n", hr );

    return hr;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::OnResponseReceived
// Desc: Called when the entire body has been received.
//       At this point the application can begin processing the data by calling
//       ISequentialStream::Read on the pResponseStream or store a reference to
//       the ISequentialStream for later processing.
// Arguments:
//    pXHR            - Pointer to the originating IXMLHTTPRequest2 object.
//    pResponseStream - Pointer to the complete input stream.
// --------------------------------------------------------------------------------------
IFACEMETHODIMP HttpCallback::OnResponseReceived( IXMLHTTPRequest2* pXHR, ISequentialStream* pResponseStream )
{
    UNREFERENCED_PARAMETER( pXHR );
#ifdef XSF_USE_DX_12_0
    XSFScopedNamedEvent( static_cast<XSF::D3DCommandQueue*>(nullptr), 0, L"HttpCallback::OnResponseReceived" );
#else
    XSFScopedNamedEvent( nullptr, 0, L"HttpCallback::OnResponseReceived" );
#endif

    // Add the contents of the stream to our running result.
    const HRESULT hr = ReadDataFromStream( pResponseStream );

    // Set the completion event to "triggered".
    CompleteWithResult( hr );
    XboxSampleFramework::DebugPrint( "IXMLHTTP: OnResponseReceived: 0x%x\n", hr );

    return hr;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::OnError
// Desc: Handle errors that have occurred during the HTTP request.
// Arguments:
//    pXHR - The interface pointer of IXMLHTTPRequest2 object.
//    hrError - The errocode for the httprequest.
// --------------------------------------------------------------------------------------
IFACEMETHODIMP HttpCallback::OnError( IXMLHTTPRequest2* pXHR, HRESULT hrError )
{
    UNREFERENCED_PARAMETER(pXHR);

    // The Request is complete, but broken.
    CompleteWithResult( hrError );
    XboxSampleFramework::DebugPrint( "IXMLHTTP: OnError: 0x%x\n", hrError );

    return S_OK;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::ReadFromStream
// Desc: Demonstrate how to read from the HTTP response stream.
// Arguments:
//    pStream - the data stream read form the http response.
// --------------------------------------------------------------------------------------
HRESULT HttpCallback::ReadDataFromStream( ISequentialStream* pStream )
{
    if( pStream == nullptr )
    {
        return E_INVALIDARG;
    }

#ifdef XSF_USE_DX_12_0
    XSFScopedNamedEvent( static_cast<XSF::D3DCommandQueue*>(nullptr), 0, L"HttpCallback::ReadDataFromStream" );
#else
    XSFScopedNamedEvent( nullptr, 0, L"HttpCallback::ReadDataFromStream" );
#endif

    CCHAR buffer[HTTP_RESPONSE_READ_CHUNK_SIZE];
    DWORD totalBytesRead = 0;
    DWORD bytesRead = 0;
    HRESULT hr = S_OK;

    do
    {
        // Read returns S_FALSE if there is no data
        hr = pStream->Read( buffer, HTTP_RESPONSE_READ_CHUNK_SIZE, &bytesRead );
        if( FAILED(hr) || bytesRead == 0 )
        {
            break;
        }

        m_data.insert( m_data.end(), &buffer[0], buffer + bytesRead );
        totalBytesRead += bytesRead;

    } while ( hr == S_OK );

    XboxSampleFramework::DebugPrint( "IXMLHTTP: ReadDataFromStream: 0x%x, Read %u bytes, Total %u\n", 
        hr, totalBytesRead, m_data.size() );

    // Return S_OK instead of S_FALSE
    if( SUCCEEDED(hr) )
        return S_OK;
    return hr;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::IsCompleted
// Desc: Non-blocking test for completion of the HTTP request.
// --------------------------------------------------------------------------------------
bool HttpCallback::IsCompleted()
{
    return WaitForCompletion( 0 );
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::WaitForCompletion
// Desc: Blocking wait for completion of the HTTP request. 
// Arguments:
//    timeoutInMs - Time in milliseconds to wait for completion
// --------------------------------------------------------------------------------------
bool HttpCallback::WaitForCompletion( const DWORD timeoutInMs )
{
    const size_t result = m_completionEvent.wait( timeoutInMs ==  0 ? concurrency::COOPERATIVE_TIMEOUT_INFINITE : static_cast<unsigned int>(timeoutInMs) );
    if( result == concurrency::COOPERATIVE_WAIT_TIMEOUT )
        return false;
    return true;
}

// --------------------------------------------------------------------------------------
// Name: HttpCallback::CompleteWithResult
// Desc: Complete the request with the provided result code
// Arguments:
//    hr - Result code
// --------------------------------------------------------------------------------------
bool HttpCallback::CompleteWithResult( const HRESULT hr )
{
    HRESULT expectedResult = S_FALSE;
    if( m_hr.compare_exchange_strong( expectedResult, hr ) )
    {
        // Trigger event to unblock any waiting threads
        m_completionEvent.set();
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::HttpRequest
// Desc: Constructor.
// ----------------------------------------------------------------------------
HttpRequest::HttpRequest() :
        m_pXHR( nullptr ),
        m_pXHRCallback( nullptr ),
        m_pHttpCallback( nullptr ),
        m_dwTimeoutInMs( 0 )
{
    // Create the IXmlHttpRequest2 object.
    XSF_ERROR_IF_FAILED( ::CoCreateInstance( __uuidof(FreeThreadedXMLHTTP60),
                                            nullptr,
                                            CLSCTX_SERVER,
                                            __uuidof(IXMLHTTPRequest2),
                                            &m_pXHR ) );

    // Create the IXmlHttpRequest2Callback object and initialize it.
    XSF_ERROR_IF_FAILED( Microsoft::WRL::Details::MakeAndInitialize<HttpCallback>( &m_pHttpCallback ) );
    XSF_ERROR_IF_FAILED( m_pHttpCallback.As( &m_pXHRCallback ) );

    m_bUseXSTSToken = false;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::~HttpRequest
// Desc: Destructor.
// ----------------------------------------------------------------------------
HttpRequest::~HttpRequest()
{
    // ComPtr<> smart pointers should handle releasing the COM objects.
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::Open()
// Desc: Set up and kickstart an asynchronous HTTP request on a URL.
// Params:
//     user      - User that we are making the call on behalf of
//     verb      - HTTP verb as a wchar_t string.
//     url       - URI for the HTTP request as a wchar_t string.
// ----------------------------------------------------------------------------
HRESULT HttpRequest::Open( User^ user, const std::wstring& verb, const std::wstring& url )
{
    return Open( user, verb, url, std::wstring(), nullptr, 0 );
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::Open()
// Desc: Set up and kickstart an asynchronous HTTP request on a URL.
// Params:
//     user             - User that we are making the call on behalf of
//     verb             - HTTP verb as a wchar_t string.
//     url              - URI for the HTTP request as a wchar_t string.
//     contentType      - Optional, contentType header, must be specified with contentBytes.
//     contentBytes     - Optional, Data payload for the request as byte pointer.
//     contentLength    - Optional, Data payload length in bytes, must be specified with contentBytes.
// ----------------------------------------------------------------------------
HRESULT HttpRequest::Open( User^ user, const std::wstring& verb, const std::wstring& url, 
                           const std::wstring& contentType, const BYTE* contentBytes, size_t contentLength )
{
    return Open( user, verb, url, contentType, contentBytes, contentLength, GetDefaultHeaders(UseXSTSToken()) );
}

//--------------------------------------------------------------------------------------
// Name: HttpRequest::Open()
// Desc: Set up and kickstart an asynchronous HTTP request on a URL given specific headers
// Params:
//     user                - User that we are making the call on behalf of
//     verb                - HTTP verb as a wchar_t string
//     url                 - URI for the HTTP request
//     headers             - Vector of HTTPHeaderInfo objects to include with the request.
//     contentType         - Optional, contentType header, must be specified with contentBytes.
//     contentBytes        - Optional, Data payload for the request as byte pointer.
//     contentLength       - Optional, Data payload length in bytes, must be specified with contentBytes.
//--------------------------------------------------------------------------------------

HRESULT HttpRequest::Open( Windows::Xbox::System::User^ user, const std::wstring& verb, const std::wstring& url, 
        const std::wstring& contentType, const BYTE* contentBytes, size_t contentLength,
        std::vector<HttpHeaderInfo> headers )
{
    XSF_ASSERT( ( user && m_bUseXSTSToken ) || !m_bUseXSTSToken );
    XSF_ASSERT( !verb.empty() );
    XSF_ASSERT( !url.empty() );
    XSF_ASSERT( contentBytes == nullptr && contentLength == 0 || contentBytes != nullptr && contentLength != 0 );
    XSF_ASSERT( contentBytes == nullptr || !contentType.empty() )

    HRESULT hr = E_FAIL;

    // Open a connection for an HTTP GET request.
    // NOTE: This is where the IXMLHTTPRequest2 object gets given a
    // pointer to the IXMLHTTPRequest2Callback object.
    hr = m_pXHR->Open( verb.c_str(),            // HTTP method
                       url.c_str(),             // URL string as wchar*
                       m_pXHRCallback.Get(),    // callback object from a ComPtr<>
                       nullptr,                 // username
                       nullptr,                 // password
                       nullptr,                 // proxy username
                       nullptr );               // proxy password
    if( FAILED( hr ) )
    {
        XboxSampleFramework::DebugPrint( "IXMLHTTP: Failed to create request [%S] for \"%S\", Result: 0x%x\n", 
            verb.c_str(), url.c_str(), hr );
        return hr;
    }

    // NOTE: Properties must be set AFTER the Open() call
    if( m_dwTimeoutInMs != 0)
    {
        m_pXHR->SetProperty( XHR_PROP_TIMEOUT, m_dwTimeoutInMs ); 
    }

    // XHR_PROP_ONDATA_THRESHOLD is an Xbox One specific property to control how often OnDataAvailable is called.
    // Extra callbacks slow performance and are unnecessary if the handler is not going to be utilizing data in chunks
    m_pXHR->SetProperty( XHR_PROP_ONDATA_THRESHOLD, HTTP_RESPONSE_READ_CHUNK_SIZE ); 

    // Add the content type header if provided
    if( !contentType.empty() )
    {
        headers.emplace_back( L"Content-Type", contentType, true );
    }

    // Add the provided headers to the request from the caller, if one is
    // a required header for the signature, add it to the SigningHeaders
    // string following the standard header format "Header: value\r\n" to
    // be used with the token and signature retrieval API
    //
    std::wstring wstrXSTSHeaders = L"";
    for( const auto& header : headers )
    {
        hr = m_pXHR->SetRequestHeader( header.Name.c_str(), header.Value.c_str() );
        if( FAILED( hr ) )
        {
            return hr;
        }
        if( m_bUseXSTSToken && header.UsedForXSTSToken )
        {
            wstrXSTSHeaders += header.Name + L": " + header.Value + L"\r\n";
        }
    }

    if( contentBytes != nullptr )
    {
        // Prepare request stream with provided content bytes 
        m_pHttpRequestStream = Make<HttpRequestStream>();
        m_pHttpRequestStream->Open( contentBytes, static_cast<ULONG>(contentLength) );
    }

    //
    // Get and add the authorization token and signature to the request
    //
    if( m_bUseXSTSToken )
    {
        // Set our state to pending and prepare strings for the returned token and signature
        hr = E_PENDING;

        // NOTE: Even if there is no body to the request, the body parameter
        // is required to be at least a 1 byte aray
        Platform::Array<unsigned char>^ body;
        if( contentBytes != nullptr )
        {
            body = ref new Platform::Array<unsigned char>( 
                const_cast<unsigned char*>(contentBytes), static_cast<unsigned int>(contentLength) );
        }
        else
        {
            body = ref new Platform::Array<unsigned char>(1);
            body[0] = 0;
        }

        std::wstring wstrToken;
        std::wstring wstrSignature;

        auto asyncOp = user->GetTokenAndSignatureAsync( ref new Platform::String( verb.c_str() ),               // HTTP method for the token
                                                        ref new Platform::String( url.c_str() ),                // URL for the token
                                                        ref new Platform::String( wstrXSTSHeaders.c_str() ),    // Headers
                                                        body );                                                 // Body

        // Construct an object that waits for the AsyncOperation to finish
        asyncOp->Completed = ref new AsyncOperationCompletedHandler<GetTokenAndSignatureResult^>(
            [&hr, &wstrToken, &wstrSignature]( IAsyncOperation<GetTokenAndSignatureResult^>^ operation, Windows::Foundation::AsyncStatus status)
        {
            if( status == Windows::Foundation::AsyncStatus::Completed )
            {
                try
                {
                    // NOTE: this can throw if there is no data
                    auto results = operation->GetResults();
                        
                    wstrToken.assign(results->Token->Data());
                    wstrSignature.assign(results->Signature->Data());

                    if( !wstrToken.empty() )
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_UNEXPECTED;
                    }
                } 
                catch ( Platform::Exception ^e )
                {
                    hr = e->HResult;
                    XboxSampleFramework::DebugPrint( "IXMLHTTP: Failed to get token: 0x%x\n", hr );
                }
            }
            else
            {
                // An error occurred
                hr = operation->ErrorCode.Value;
                XboxSampleFramework::DebugPrint( "IXMLHTTP: Auth Error: 0x%x\n", hr );
            }
        } );

        // Wait for completion
        while ( ( asyncOp->Status == Windows::Foundation::AsyncStatus::Started ) || ( hr == E_PENDING ) )
        {
            ::Sleep(1);
        }

        if ( FAILED( hr ) )
        {
            CancelRequest( hr );
            XboxSampleFramework::DebugPrint( "IXMLHTTP: Failed to get token: 0x%x\n", hr );
            return hr;
        }

        // Add the Authorization header with token
        hr = m_pXHR->SetRequestHeader( L"Authorization", wstrToken.c_str() );
        if( FAILED( hr ) )
        {
            CancelRequest( hr );
            XboxSampleFramework::DebugPrint( "IXMLHTTP: Failed to set Authorization header: 0x%x\n", hr );
            return hr;
        }

        // NOTE: this is a temporary placeholder for the Signature header. The actual
        // header will be defined in a future XDK. This Header is not actually verified
        // yet with the services.
        hr = m_pXHR->SetRequestHeader( L"Signature", wstrSignature.c_str() );
        if( FAILED( hr ) )
        {
            CancelRequest( hr );
            XboxSampleFramework::DebugPrint( "IXMLHTTP: Failed to set Signature header: 0x%x\n", hr );
            return hr;
        }
    }

    XboxSampleFramework::DebugPrint( "IXMLHTTP: Opened request [%S] for \"%S\", Result: 0x%x\n", 
        verb.c_str(), url.c_str(), hr );

    hr = SendRequest();

    return hr;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::SendRequest
// Desc: Sends the request with any provided content
// ----------------------------------------------------------------------------
HRESULT HttpRequest::SendRequest()
{
    HRESULT hr = S_OK;

    // Reset timer to implement timeouts
    m_timer.Reset();

    if( m_pHttpRequestStream )
    {
        hr = m_pXHR->Send( m_pHttpRequestStream.Get(),        // body message as an ISequentialStream*
                           m_pHttpRequestStream->Size() );    // count of bytes in the stream.
    }
    else
    {
        hr = m_pXHR->Send( nullptr, 0 );
    }

    if( FAILED(hr) )
    {
        CancelRequest( hr );
    }

    XboxSampleFramework::DebugPrint( "IXMLHTTP: SendRequest: Result:0x%x\n", hr );
    return hr;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::CancelRequest
// Desc: Cancels the request with the provided error code
// ----------------------------------------------------------------------------
void HttpRequest::CancelRequest( HRESULT hr )
{
    if( m_pHttpCallback->CompleteWithResult( hr ) )
    {
        m_pXHR->Abort();
    }
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::IsResponseReceived
// Desc: Test whether the request has finished, either with or without an
//       error. The HTTP state can be tested once the request is finished.
// ----------------------------------------------------------------------------
bool HttpRequest::IsResponseReceived()
{
    return WaitForResponse( 0 );
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::WaitForResponse
// Desc: Blocking wait for the request to complete, with or without an error
// ----------------------------------------------------------------------------
bool HttpRequest::WaitForResponse( const DWORD dwTimeoutInMs )
{
    if( m_pHttpCallback->WaitForCompletion( dwTimeoutInMs ) )
    {
        return true;
    }

    // Apply timeout if applicable
    if( m_dwTimeoutInMs != 0 )
    {
        const double elapsedTime = m_timer.Update();
        const DWORD elapsedTimeInMs = static_cast<DWORD>(elapsedTime*1000);
        if( elapsedTimeInMs >= m_dwTimeoutInMs )
        {
            CancelRequest( HRESULT_FROM_WIN32(ERROR_TIMEOUT) );
            XboxSampleFramework::DebugPrint( "IXMLHTTP: WaitForResponse: Cancelled request after %d ms\n", elapsedTimeInMs );
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::WaitForResponseAsync
// Desc: Blocking wait for the request to complete, with or without an error
// ----------------------------------------------------------------------------
concurrency::task<bool> HttpRequest::WaitForResponseAsync( const DWORD dwTimeoutInMs )
{
    auto waitForResponseTask = concurrency::create_task( [this, dwTimeoutInMs] ()
    {
        const bool result = m_pHttpCallback->WaitForCompletion( dwTimeoutInMs );
        if (!result)
        {
            CancelRequest( HRESULT_FROM_WIN32(ERROR_TIMEOUT) );
            XboxSampleFramework::DebugPrint( "IXMLHTTP: WaitForResponseAsync: Cancelled request after %d ms\n", dwTimeoutInMs );
        }
        return result;

    });
    return waitForResponseTask;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::Abort
// Desc: Aborts the request
// ----------------------------------------------------------------------------
void HttpRequest::Abort()
{
    CancelRequest(E_ABORT);
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetResult
// Desc: Test whether the request has finished, either with or without an
//       error. The HTTP state can be tested once the request is finished.
// ----------------------------------------------------------------------------
HRESULT HttpRequest::GetResult() const 
{ 
    XSF_ASSERT( m_pHttpCallback->IsCompleted() );
    return m_pHttpCallback->GetHR(); 
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetStatus
// Desc: Test whether the request has finished, either with or without an
//       error. The HTTP state can be tested once the request is finished.
// ----------------------------------------------------------------------------
DWORD HttpRequest::GetStatus() const 
{ 
    XSF_ASSERT( m_pHttpCallback->IsCompleted() );
    return m_pHttpCallback->GetHTTPStatus(); 
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetHeaders
// Desc: Returns the received data as a UTF8 string
// ----------------------------------------------------------------------------
const std::wstring& HttpRequest::GetHeaders() const 
{ 
    XSF_ASSERT( m_pHttpCallback->IsCompleted() );
    return m_pHttpCallback->GetHeaders(); 
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetData
// Desc: Returns the received data as a buffer
// ----------------------------------------------------------------------------
const std::vector<BYTE>& HttpRequest::GetDataBuffer() const 
{ 
    XSF_ASSERT( m_pHttpCallback->IsCompleted() );
    return m_pHttpCallback->GetData(); 
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetDataAsWString
// Desc: Returns the received data as a UTF8 string
// ----------------------------------------------------------------------------
std::wstring HttpRequest::GetDataAsUTF8String() const
{
    XSF_ASSERT( m_pHttpCallback->IsCompleted() );
    return BufferToUTF8( GetDataBuffer() );
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetDefaultUserAgent
// Desc: Returns the user agent used by the default headers
// ----------------------------------------------------------------------------
std::wstring HttpRequest::GetDefaultUserAgent()
{
    return L"ATG_IXHR2_HTTP\r\n";
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::GetDefaultHeaders
// Desc: Returns the headers used by the HttpRequest object by default when not specified
// ----------------------------------------------------------------------------
std::vector<HttpHeaderInfo> HttpRequest::GetDefaultHeaders( bool useXSTSToken )
{
    std::vector<HttpHeaderInfo> defaultHeaders;
    defaultHeaders.emplace_back( HttpHeaderInfo( L"User-Agent",   GetDefaultUserAgent(),  true ) );
    if( useXSTSToken )
    {
        // Set up an array of the common headers required Xbox RESTful services,
        // if the service call fails, check the required headers for the service.
        defaultHeaders.emplace_back( HttpHeaderInfo( L"x-xbl-device-type",        L"XboxOne\r\n",     true ) );
        defaultHeaders.emplace_back( HttpHeaderInfo( L"x-xbl-client-type",        L"Console\r\n",     true ) );
        defaultHeaders.emplace_back( HttpHeaderInfo( L"x-xbl-client-version",     L"1.0\r\n",         true ) );
        defaultHeaders.emplace_back( HttpHeaderInfo( L"x-xbl-contract-version",   L"1\r\n",           true ) );
    }
    return defaultHeaders;
}

// ----------------------------------------------------------------------------
// Name: HttpRequest::BufferToWString
// Desc: Converts a vector of bytes to a UTF-8 wstring
// ----------------------------------------------------------------------------
std::wstring HttpRequest::BufferToUTF8( const std::vector<BYTE>& buffer )
{
    // Get the length
    int wcharLength = ::MultiByteToWideChar( CP_UTF8,        // code page (Xbox LIVE uses UTF-8 for all JSON)
										     0,              // flags (how to handle composite characters and errors)
										     reinterpret_cast<const char*>(buffer.data()),   // UTF-8 string to convert.
										     static_cast<int>(buffer.size()),                // Length of UTF-8 string in BYTEs, -1 if zero terminated.
										     nullptr,        // pointer to WCHAR buffer
										     0 );            // size of WCHAR buffer in characters
    // Get the string
    std::wstring result;
    result.assign(wcharLength, L'\0');
    wcharLength = ::MultiByteToWideChar( CP_UTF8,            // code page (Xbox LIVE uses UTF-8 for all JSON)
									     0,                  // flags (how to handle composite characters and errors)
										 reinterpret_cast<const char*>(buffer.data()),   // UTF-8 string to convert.
										 static_cast<int>(buffer.size()),                // Length of UTF-8 string in BYTEs, -1 if zero terminated.
									     const_cast<wchar_t*>(result.data()),           // pointer to WCHAR buffer
									     wcharLength );      // size of WCHAR buffer in characters
    return result;
}

//--------------------------------------------------------------------------------------
// Name: HttpRequestStream::HttpRequestStream
// Desc: Constructor
//--------------------------------------------------------------------------------------
HttpRequestStream::HttpRequestStream()
	: m_cRef(1)
	, m_pBuffer(nullptr)
{
}

//--------------------------------------------------------------------------------------
// Name: HttpRequestStream::~HttpRequestStream
// Desc: Destructor
//--------------------------------------------------------------------------------------
HttpRequestStream::~HttpRequestStream()
{
    delete[] m_pBuffer;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Open
//  Desc: Opens the buffer populated with the supplied data
//--------------------------------------------------------------------------------------
STDMETHODIMP HttpRequestStream::Open( const void *psBuffer, ULONG cbBufferSize )
{
    if( psBuffer == nullptr || cbBufferSize > HTTP_REQUEST_MAX_BUFFER_SIZE )
    {
        return E_INVALIDARG;
    }

    m_buffSize = cbBufferSize;
    m_buffSeekIndex = 0;

	// Create a buffer to store a copy of the request (and include space for the null 
	// terminator, as generally this method can accept the result of strlen() for 
	// cbBufferSize). This buffer is deleted in the destructor.
    m_pBuffer = new (std::nothrow) BYTE[ cbBufferSize ];
	if( m_pBuffer == nullptr )
    {
        return E_OUTOFMEMORY;
    }

    memcpy_s( m_pBuffer, m_buffSize, psBuffer, m_buffSize );

    return S_OK;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Size
//  Desc: Returns the size of the buffer
//--------------------------------------------------------------------------------------
STDMETHODIMP_(ULONGLONG) HttpRequestStream::Size()
{
    return m_buffSize;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Read
//  Desc: ISequentialStream overload: Reads data from the buffer
//--------------------------------------------------------------------------------------
STDMETHODIMP HttpRequestStream::Read( void *pv, ULONG cb, ULONG *pcbNumReadBytes )
{
    if( pv == nullptr )
    {
        return E_INVALIDARG;
    }

#ifdef XSF_USE_DX_12_0
    XSFScopedNamedEvent( static_cast<XSF::D3DCommandQueue*>(nullptr), 0, L"HttpRequestStream::Read" );
#else
    XSFScopedNamedEvent( nullptr, 0, L"HttpRequestStream::Read" );
#endif

    HRESULT hr = S_OK;
	BYTE* pbOutput = reinterpret_cast<BYTE*>( pv );
	const BYTE* pbInput = reinterpret_cast<BYTE*>( m_pBuffer );

    for( *pcbNumReadBytes = 0; *pcbNumReadBytes < cb; (*pcbNumReadBytes)++ )
    {
        if( m_buffSeekIndex == m_buffSize )
        {
            hr = S_FALSE;
            break;
        }

        pbOutput[*pcbNumReadBytes] = pbInput[ m_buffSeekIndex ];
        m_buffSeekIndex++;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Write
//  Desc: ISequentialStream overload: Writes to the buffer. Not implmented, as the buffer is "read only"
//--------------------------------------------------------------------------------------
STDMETHODIMP HttpRequestStream::Write( const void *pv, ULONG cb, ULONG *pcbWritten )
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    UNREFERENCED_PARAMETER(pcbWritten);

    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::QueryInterface
//  Desc: IUnknown overload: Queries for a particular interface
//--------------------------------------------------------------------------------------
STDMETHODIMP HttpRequestStream::QueryInterface( REFIID riid, void **ppvObject )
{
    if( ppvObject == nullptr )
    { 
        return E_INVALIDARG;
    }

	*ppvObject = nullptr;

	HRESULT hr = S_OK;
    void *pObject = nullptr;

    if( riid == IID_IUnknown )
    {
        pObject = static_cast<IUnknown *>((IDispatch*)this);
    }
	else if( riid == IID_IDispatch )
    {
        pObject = static_cast<IDispatch *>(this);
    }
    else if( riid == IID_ISequentialStream )
    {
        pObject = static_cast<ISequentialStream *>(this);
    }
    else 
    {
        return E_NOINTERFACE;
    }

    AddRef();

    *ppvObject = pObject;
    pObject = nullptr;

    return hr;
} 

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::AddRef
//  Desc: IUnknown: Increments the reference count
//--------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) HttpRequestStream::AddRef()
{ 
    return ::InterlockedIncrement( &m_cRef );
} 

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Release
//  Desc: IUnknown overload: Decrements the reference count, possibly deletes the instance
//--------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) HttpRequestStream::Release()
{
    ULONG ulRefCount = ::InterlockedDecrement( &m_cRef );

    if( 0 == ulRefCount )
    {
        delete this;
    }

    return ulRefCount;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::GetTypeInfoCount
//  Desc: IDispatch overload: IXMLHTTPRequest2 expects a complete IDispatch interface,
//  but doesn't actually make use of this.
//--------------------------------------------------------------------------------------
HRESULT HttpRequestStream::GetTypeInfoCount( unsigned int* pctinfo )
{
    if( pctinfo )
    {
        *pctinfo = 0;
    }

    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::GetTypeInfo
//  Desc: IDispatch overload: IXMLHTTPRequest2 expects a complete IDispatch interface,
//  but doesn't actually make use of this.
//--------------------------------------------------------------------------------------
HRESULT HttpRequestStream::GetTypeInfo( unsigned int iTInfo, LCID  lcid, ITypeInfo** ppTInfo )
{
    if( ppTInfo )
    {
        *ppTInfo = nullptr;
    }

    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(iTInfo);

    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::GetIDsOfNames
//  Desc: IDispatch overload: IXMLHTTPRequest2 expects a complete IDispatch interface,
//  but doesn't actually make use of this.
//--------------------------------------------------------------------------------------
HRESULT HttpRequestStream::GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, 
	unsigned int cNames, LCID lcid, DISPID* rgDispId )
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(rgszNames);
    UNREFERENCED_PARAMETER(cNames);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(rgDispId);

    return DISP_E_UNKNOWNNAME;
}

//--------------------------------------------------------------------------------------
//  Name: HttpRequestStream::Invoke
//  Desc: IDispatch overload: IXMLHTTPRequest2 expects a complete IDispatch interface,
//  but doesn't actually make use of this.
//--------------------------------------------------------------------------------------
HRESULT HttpRequestStream::Invoke( 
	DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, 
	unsigned int* puArgErr )
{
    UNREFERENCED_PARAMETER(dispIdMember);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(pDispParams);
    UNREFERENCED_PARAMETER(pVarResult);
    UNREFERENCED_PARAMETER(pExcepInfo);
    UNREFERENCED_PARAMETER(puArgErr);

    return S_OK;
}

// ----------------------------------------------------------------------------
