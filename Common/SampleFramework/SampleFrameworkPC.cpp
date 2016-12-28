//--------------------------------------------------------------------------------------
// SampleFrameworkPc.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include <pch.h>
#include <dxgi.h>
#ifdef _DEBUG
#include <initguid.h>
#include <DXGIDebug.h>
#endif

#define WINDOW_NAME TEXT( "Sample Framework" )
#define CLASS_NAME  TEXT( "XSF_CLASS" )

// Some globals
static HINSTANCE        g_hInst;        // Current instance handle
static HWND             g_hWnd;         // Current window handle
static SampleFramework* g_pSample;      // Current sample
static UINT             g_wndSX;        // Window width
static UINT             g_wndSY;        // Window height
static BOOL             g_bAppActive;   // Whether we've got focus or not. This is supposed to impose the behaviour of the Xbox One codepath


//--------------------------------------------------------------------------------------
// Name: InitInstance
// Desc: Saves instance handle and creates main window
//--------------------------------------------------------------------------------------
static
BOOL InitInstance( HINSTANCE hInstance, int nCmdShow, const SampleFramework::Settings& settings )
{
    g_hInst = hInstance; // Store instance handle in our global variable

    const DWORD windowStyle = WS_OVERLAPPEDWINDOW;

    RECT rc;
    rc.bottom = settings.m_frameBufferHeight;
    rc.top = 0;
    rc.left = 0;
    rc.right = settings.m_frameBufferWidth;
    AdjustWindowRectEx( &rc, windowStyle, FALSE, 0 );

    g_wndSX = rc.right - rc.left;
    g_wndSY = rc.bottom - rc.top;

    g_hWnd = CreateWindow( CLASS_NAME, WINDOW_NAME, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, g_wndSX, g_wndSY, nullptr, nullptr, hInstance, nullptr);
    if( !g_hWnd )
        return FALSE;

    ShowWindow( g_hWnd, nCmdShow );
    UpdateWindow( g_hWnd );
    SetForegroundWindow( g_hWnd );
    SetFocus( g_hWnd ); 

    // Pump win32 messages
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, TRUE))
    {
        if (!TranslateAccelerator(msg.hwnd, nullptr, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return TRUE;
}



//--------------------------------------------------------------------------------------
// Name: SetActivationStatus
// Desc: Pauses the main loop when the application loses focus
//--------------------------------------------------------------------------------------
void SampleFramework::SetActivationStatus( BOOL bActive )
{
    g_bAppActive = bActive;
    if( g_pSample )
    {
        if( g_bAppActive )
            g_pSample->FrameworkResume();
        else
            g_pSample->FrameworkSuspend();
    }
}

//--------------------------------------------------------------------------------------
// Name: WndProc
// Desc: Processes messages for the main window.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK SampleFramework::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* p = (MINMAXINFO*)lParam;

            p->ptMaxTrackSize.x = g_wndSX;
            p->ptMaxTrackSize.y = g_wndSY;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_INITMENU:
    case WM_ENTERSIZEMOVE:
        SetActivationStatus( FALSE );
        break;

    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
        SetActivationStatus( TRUE );
        break;

    case WM_ACTIVATEAPP:
        SetActivationStatus( wParam != 0 );
        break;

    case WM_SIZE:
        if (g_pSample)
        {
            g_pSample->SetBackbufferSize(LOWORD(lParam), HIWORD(lParam));
            g_pSample->CreateWindowSizeDependentResources(nullptr);
        }
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}


//--------------------------------------------------------------------------------------
// Name: PCEntryPoint
// Desc: Entry point
//--------------------------------------------------------------------------------------
int SampleFramework::PCEntryPoint( SampleFramework* pSample, HINSTANCE hInstance, int nCmdShow )
{
    // Mark the process as DPI aware to prevent system scaling
    SetProcessDPIAware();

    // Register window class
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = SampleFramework::WndProc;
    wcex.hInstance     = hInstance;
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName = CLASS_NAME;
    RegisterClassEx( &wcex );

    {
        g_pSample = pSample;

        g_pSample->ParseCommandLine( GetCommandLine() );

        XSF::CrossCheckpoint( XSF::CP_BEFORE_D3D_INIT );

        pSample->ObtainSampleSettings();

        // Perform application initialization:
        if( !InitInstance( hInstance, nCmdShow, pSample->m_sampleSettings ) )
        {
            return FALSE;
        }

        pSample->CreateDeviceResources();
        pSample->CreateWindowSizeDependentResources( g_hWnd );
        pSample->FrameworkInitialize();

        // Main message loop:
        for( ;; )
        {
            MSG msg;

            // pump win32 messages
            while( PeekMessage( &msg, nullptr, 0, 0, TRUE ) )
            {
                if( !TranslateAccelerator( msg.hwnd, nullptr, &msg ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
            }

            // Call Update and Render. Note that for test runs on PC we ignore app suspend because this causes intermittent test failures.
            BOOL bKeepRunning = TRUE;
            if( g_bAppActive || pSample->InTestRun() || pSample->InStressRun() )
            {
                bKeepRunning = pSample->FrameworkUpdateAndRender();
            }

            // exit loop if the window or the sample want to
            if( msg.message == WM_QUIT    ||
                !bKeepRunning )
            {
                pSample->FrameworkShutdown();
                break;
            }
        }
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Name: ReportLiveObjects
// Desc: Reports live objects from DXGI
//--------------------------------------------------------------------------------------
void SampleFramework::ReportLiveObjects()
{
#ifdef _DEBUG
    typedef HRESULT ( WINAPI *fnDXGIGetDebugInterface )( REFIID, void ** );
    HMODULE hdxgiDebug = LoadLibraryEx( L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    if( hdxgiDebug != nullptr )
    {
        fnDXGIGetDebugInterface pDXGIGetDebugInterface = (fnDXGIGetDebugInterface)GetProcAddress( hdxgiDebug, "DXGIGetDebugInterface" );
        if( pDXGIGetDebugInterface != nullptr )
        {
            IDXGIDebug *pDxgiDebug = nullptr;
            if( SUCCEEDED( (*pDXGIGetDebugInterface)( IID_IDXGIDebug, reinterpret_cast< void** >( &pDxgiDebug ) ) ) )
            {
                pDxgiDebug->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL );
                XSF_SAFE_RELEASE( pDxgiDebug );
            }
        }

        FreeLibrary( hdxgiDebug );
    }
#endif
}


//--------------------------------------------------------------------------------------
// Name: PlatformCreate
// Desc: Create the swap chain
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::SwapChain::PlatformCreate()
{
    const SwapChainCreateDesc* pDesc = &m_createDesc;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
    swapChainDesc.SampleDesc.Count = 1;                         // don't use multi-sampling
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = pDesc->m_Usage;
    swapChainDesc.BufferCount = pDesc->m_NumBuffers;            // use two buffers to enable flip effect
    swapChainDesc.SwapEffect = pDesc->m_flipSequential ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = pDesc->m_Flags | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.OutputWindow = reinterpret_cast<HWND>(pDesc->m_pWindow);
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.BufferDesc.Width = pDesc->m_Width;
    swapChainDesc.BufferDesc.Height = pDesc->m_Height;
    swapChainDesc.BufferDesc.Format = pDesc->m_Format;

    // Once the desired swap chain description is configured, it must be created on the same adapter as our D3D Device
#if defined(XSF_USE_DX_12_0)
    XSF::D3DTypePtr<IDXGIFactory2> spdxgiFactory;
    XSF_RETURN_IF_FAILED(CreateDXGIFactory1(IID_GRAPHICS_PPV_ARGS(spdxgiFactory.GetAddressOf())));

    XSF_RETURN_IF_FAILED(spdxgiFactory->CreateSwapChain(m_pd3dCmdQueue, &swapChainDesc, reinterpret_cast<IDXGISwapChain**>(m_spSwapChain.GetAddressOf())));
#else
    // First, retrieve the underlying DXGI Device from the D3D Device
    XSF::D3DTypePtr<IDXGIDevice1> spdxgiDevice;
    XSF_RETURN_IF_FAILED(m_pd3dDevice->QueryInterface(IID_GRAPHICS_PPV_ARGS(spdxgiDevice.GetAddressOf())));

    XSF::D3DTypePtr<IDXGIAdapter> spdxgiAdapter;
    XSF_RETURN_IF_FAILED(spdxgiDevice->GetAdapter(spdxgiAdapter.GetAddressOf()));

    XSF::D3DTypePtr<IDXGIFactory1> spdxgiFactory;
    XSF_RETURN_IF_FAILED(spdxgiAdapter->GetParent(IID_GRAPHICS_PPV_ARGS(spdxgiFactory.GetAddressOf())));

    XSF_RETURN_IF_FAILED(spdxgiFactory->CreateSwapChain(m_pd3dDevice, &swapChainDesc, reinterpret_cast<IDXGISwapChain**>(m_spSwapChain.GetAddressOf())));
#endif

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: PlatformDestroy
// Desc: Destroy the swap chain
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::PlatformDestroy()
{
    m_spSwapChain.Reset();
#if defined(XSF_USE_DX_12_0)
    XSF_SAFE_RELEASE(m_pd3dCmdQueue);
#endif
    XSF_SAFE_RELEASE(m_pd3dDevice);
}


//--------------------------------------------------------------------------------------
// Name: PresentNow
// Desc: If ever you need to present mid-frame...
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::PresentNow( VOID** pCookie )
{
    UNREFERENCED_PARAMETER(pCookie);

#if defined(XSF_USE_DX_12_0)
    XSFScopedNamedEventFunc( m_pd3dCommandQueue, XSF_COLOR_PRESENT );
#else
    XSFScopedNamedEventFunc( m_pd3dContext, XSF_COLOR_PRESENT );
#endif

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    const HRESULT hr = m_backbufferSwapChain.GetSwapChain()->Present( m_presentationSyncInterval, m_presentationFlags );

    if( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
    {
        Initialize();
    } 
    else if ( hr == DXGI_ERROR_WAS_STILL_DRAWING )
    {
    }
    else
    {
        XSF_ERROR_IF_FAILED( hr );

        m_backbufferSwapChain.RotateSwapChainBuffer();
    }

    return hr;
}
