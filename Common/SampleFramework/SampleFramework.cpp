//--------------------------------------------------------------------------------------
// SampleFramework.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "pch.h"
#include "common.h"
#include "SampleFramework.h"
#ifdef XSF_USE_DX_12_0
#include "ScreenGrab12.h"
#else
#include "ScreenGrab.h"
#endif

#if defined _XBOX_ONE
// Include telemetry generated code.
// The file is generated using xcetool.exe (that it is in the XDK)
// To generate the .h run the following command ?XceTool.exe XDKSampleFramework.man -c XDKSampleFramework.h?
#include "XDKSampleFramework.h"

#include <initguid.h>

#include "XMemAllocHooks.h"
#endif

#include <wincodec.h>


// PIX captures for Xbox One.
#ifdef _XBOX_ONE

#if defined(_DEBUG) || defined(PROFILE)
#define DURANGO_PIX
#endif

#endif


namespace XboxSampleFramework
{
    void SetContentFileRoot();
}


// Set this to 1 to display timing and pipeline statistics each frame
#define XSF_DEBUG_PRINT_TIMINGS 0

// Set this to 0 to disable seeting the XMem sample framework debug hooks (Only on ERAs)
#if defined(_DEBUG)
#define XSF_DEBUG_SET_XMEM_HOOKS 0
#endif

// This is the default frame update time to use when we don't actually know the time or when we're testing
static const FLOAT  DEFAULT_FRAME_TIME = 1.f / 60.f;

// This is how many frames we will count in test mode before exiting
#define NUM_FRAMES_TO_RUN_IN_TEST 100

// This is how many seconds we will count in test mode before exiting
#define NUM_SECONDS_TO_RUN_IN_TEST 20

// Frame timing and pipeline statistics
SampleFramework::FrameStatistics   SampleFramework::ms_frameStats;
SampleFramework::AveragedFrameStatistics   SampleFramework::ms_avgFrameStats;
const FLOAT  SampleFramework::cms_avgFrameStatsInterval = 0.25f;

//--------------------------------------------------------------------------------------
// Name: SampleFramework
// Desc: Constructs the base class for the samples
//--------------------------------------------------------------------------------------
SampleFramework::SampleFramework() :    
#ifdef XSF_USE_DX_12_0
                                        m_pd3dCommandQueue(nullptr),
                                        m_pd3dCommandList(nullptr),
                                        m_commandListClosed(false),
                                        m_pd3dClearCommandList(nullptr),
                                        m_clearCommandListClosed(false),
                                        m_fenceEvent(0),
                                        m_iFrame(0),
#else
                                        m_pd3dContext(nullptr),
#endif
                                        m_pd3dDevice(nullptr),
                                        m_printTimings(XSF_DEBUG_PRINT_TIMINGS),
                                        m_bQuitRequested(FALSE)
{
    ZeroMemory(&m_sampleSettings, sizeof(m_sampleSettings));

    DiscardDeviceResources();

    // If we are running an ERA on Xbox One then we hook up the allocators
#if defined(_XBOX_ONE) && defined(_TITLE) && XSF_DEBUG_SET_XMEM_HOOKS
    XMemSetAllocationHooks( XSF::XMemAllocHook, XSF::XMemFreeHook );
#endif

    // Initialize default settings here, the sample will have a chance to override them
#ifdef XSF_USE_DX_12_0
    ZeroMemory(m_pd3dCommandAllocator, sizeof(m_pd3dCommandAllocator));
    ZeroMemory(m_pd3dClearCommandAllocator, sizeof(m_pd3dClearCommandAllocator));
    m_sampleSettings.m_deviceCreationFlags =      0
#if defined(_XBOX_ONE) && defined(_TITLE)
#if defined(_DEBUG) || defined(DEBUG)
        | D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED
#endif
#ifdef DURANGO_PIX
        | D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED
#endif
#endif
        ;
#else
    m_sampleSettings.m_deviceCreationFlags =      0
#if defined(_DEBUG) || defined(DEBUG)
        | D3D11_CREATE_DEVICE_DEBUG // includes D3D11_CREATE_DEVICE_VALIDATED
#endif
#ifdef DURANGO_PIX
#if defined(_XBOX_ONE) && defined(_TITLE)
        | D3D11_CREATE_DEVICE_INSTRUMENTED
#endif
#endif
        ;
#endif // XSF_USE_DX_12_0

#ifdef _XBOX_ONE
    m_sampleSettings.m_frameBufferWidth = 1920;
    m_sampleSettings.m_frameBufferHeight = 1080;
#ifdef _TITLE
    m_sampleSettings.m_swapChainCreateFlags = DXGIX_SWAP_CHAIN_FLAG_QUANTIZATION_RGB_FULL;
    m_BackgroundFilterFlags = 0;
    m_ForegroundFilterFlags = 0;
#endif
#else
    m_sampleSettings.m_frameBufferWidth = 1920;
    m_sampleSettings.m_frameBufferHeight = 1080;
#endif
    m_sampleSettings.m_numBackbuffers = 2;
    m_sampleSettings.m_swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_sampleSettings.m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    m_sampleSettings.m_swapChainBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
#if !defined(XSF_USE_DX_12_0) && (!defined(_XBOX_ONE) || defined(_TITLE))
    m_sampleSettings.m_swapChainBufferUsage |= DXGI_USAGE_UNORDERED_ACCESS;
#endif
    m_sampleSettings.m_swapChainCreateFlags = 0;

    m_sampleSettings.m_createReferenceDevice = FALSE;

    m_sampleSettings.m_clearBackbufferBeforeRender = TRUE;
    m_sampleSettings.m_clearStateBeforeRender = TRUE;

    m_sampleSettings.m_autoHandleRequestsForExit = TRUE;
    m_sampleSettings.m_canScreenshot = TRUE;
    m_sampleSettings.m_enablePerfQuery = TRUE;
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_sampleSettings.m_pixCaptureAtFrame = UINT_MAX;
#endif

#ifdef XSF_USE_DX_12_0
   m_sampleSettings.m_numBackbuffers = 3;
   m_sampleSettings.m_frameLatency = 1;
#endif

#ifdef _XBOX_ONE
    m_pBkPlane = &m_backbufferSwapChain;
    m_pFgPlane = nullptr;
		
   // Telemetry Code
   if (EventRegisterXDKSampleFramework() == 0)
   {
	   // If registration succeeded
	   // Get name Exe
	   TCHAR exepath[MAX_PATH+1];
#if defined(_XBOX_ONE) && defined(_TITLE)    // #tbb - These APIs only exist for Desktop/UAP under TH
	   if (GetModuleFileName(0, exepath, MAX_PATH) != 0)
	   {		
	   }
	   else
#endif
	   {
		   // If fails to get name exe report it as Unknown
		   wcscpy_s(exepath, L"Unknown");
	   }
       EventWriteFrameworkLoaded(exepath);
   }
#endif
}

//--------------------------------------------------------------------------------------
// Name: ~SampleFramework
// Desc: Cleans up the base class for the samples. Full clean up isn't needed on Xbox One
//--------------------------------------------------------------------------------------
SampleFramework::~SampleFramework()
{

#ifdef _XBOX_ONE
    // Unregister provider
    EventUnregisterXDKSampleFramework();
#endif
    DiscardDeviceResources();
}


//--------------------------------------------------------------------------------------
// Name: ParseCommandLine
// Desc: Parses the command line
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void SampleFramework::ParseCommandLine( const wchar_t* commandLine )
{
    m_commandLineParams.clear();
    if( commandLine != nullptr )
    {
        wchar_t commandLineParams[MAX_PATH];
        wcscpy_s( commandLineParams, commandLine );
        wchar_t *nextToken;
        wchar_t *token = wcstok_s( commandLineParams, L" /", &nextToken );
        while( token != nullptr )
        {
            wchar_t *nextAssignmentToken;
            wchar_t *tokenName = wcstok_s( token, L"/=", &nextAssignmentToken );
            wchar_t *tokenValue = wcstok_s( nullptr, L"/=", &nextAssignmentToken );
            float parameterValue = 0.0f;
            if( tokenValue != nullptr )
            {
                if( wcscmp( tokenValue, L"true" ) == 0 || wcscmp( tokenValue, L"TRUE" ) == 0 )
                {
                    parameterValue = 1.0f;
                }
                else if( wcscmp( tokenValue, L"false" ) == 0 || wcscmp( tokenValue, L"FALSE" ) == 0 )
                {
                    parameterValue = 0.0f;
                }
                else
                {
                    parameterValue = static_cast< float >( _wtof( tokenValue ) );
                }
            }
            m_commandLineParams.insert( TParametersPair( tokenName, parameterValue ) );

            token = wcstok_s( nullptr, L" |+", &nextToken );
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetCommandLineParameter
// Desc: Returns true if the command line parameter exists and fill the value if requested
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
BOOL SampleFramework::GetCommandLineParameter( const wchar_t* const name, float *pValue ) const
{
    auto itParameter = m_commandLineParams.find( name );
    BOOL parameterExists = itParameter != m_commandLineParams.end();
    if( parameterExists && pValue != nullptr )
    {
        *pValue = itParameter->second;
    }
    
    return parameterExists;
}

BOOL SampleFramework::GetCommandLineParameter( const wchar_t* const name, BOOL *pValue ) const
{
    float pfValue = 0.0f;
    BOOL parameterExists = GetCommandLineParameter( name, &pfValue );

    if( parameterExists )
    {
        *pValue = (pfValue != 0.0f);
    }

    return parameterExists;
}


//--------------------------------------------------------------------------------------
// Name: ObtainSampleSettings
// Desc: This is called before a device is created so to give a chance to override the
// settings to the sample
//--------------------------------------------------------------------------------------
void SampleFramework::ObtainSampleSettings()
{
    m_sampleSettings.m_enablePerfQuery = !GetCommandLineParameter(L"DisableQuery");
    ModifySettings( m_sampleSettings );

#ifdef _XBOX_ONE
    m_sampleSettings.m_createReferenceDevice = FALSE;
#endif
}


//--------------------------------------------------------------------------------------
// Name: FrameworkInitialize
// Desc: Performs platform independent initialization of the sample base class first,
// then forwards this call to the sample
//--------------------------------------------------------------------------------------
void SampleFramework::FrameworkInitialize()
{
    // By default run on core 0
    XSF::XSetThreadProcessor( GetCurrentThread(), 0 );

    XSF::SetContentFileRoot();

    // Determine if this is an automated run (either performance or screenshot)
    m_thisIsPerfRun = GetCommandLineParameter( L"perf" );
    m_thisIsScreenshotRun = GetCommandLineParameter( L"screenshot" );
    m_thisIsStressRun = GetCommandLineParameter( L"stress" );

#if defined( ATG_PROFILE ) || defined( ATG_PROFILE_VERBOSE )
    
    m_thisIsTestRun = TRUE;
    m_printTimings = FALSE;
    XSF::SetEnableLogging( FALSE );

#else
    // Determine if this is a test run
    std::vector< BYTE > data;
    if( SUCCEEDED( XSF::LoadBlob( L"test", data ) ) )
    {
        m_thisIsTestRun = TRUE;
        m_printTimings = TRUE;
        XSF::SetEnableLogging( TRUE );
    } else if( SUCCEEDED( XSF::LoadBlob( L"log", data ) ) || m_thisIsPerfRun )
    {
        m_thisIsTestRun = FALSE;
        m_printTimings = FALSE;
        XSF::SetEnableLogging( TRUE );
    } else
    {
        m_thisIsTestRun = FALSE;
        m_printTimings = FALSE;
        XSF::SetEnableLogging( FALSE );
        if( m_thisIsStressRun )
        {
            XSF::SetExitOnError( TRUE );
        }
    }
#endif

    XSF::CrossCheckpoint( XSF::CP_IN_FRAMEWORK_INIT );

    // Build the stock D3Dx states for our device...
    XSF_ERROR_IF_FAILED(XSF::StockRenderStates::Initialize(m_pd3dDevice));

    // Initialize perf framework
#ifdef XSF_USE_DX_12_0
    XSF_ERROR_IF_FAILED( m_perfQueries.Create( m_pd3dDevice, m_pd3dCommandQueue, m_spFence ) );
#else
    XSF_ERROR_IF_FAILED( m_perfQueries.Create( m_pd3dDevice ) );
    XSF_ERROR_IF_FAILED( m_gpuCounters.Initialize( m_pd3dDevice, m_pd3dContext ) );
#endif

    m_input.Initialize();
    ZeroMemory( m_keyboardStatus, sizeof( m_keyboardStatus ) );
    ZeroMemory( m_keyboardPreviousStatus, sizeof( m_keyboardPreviousStatus ) );

    // 60 hz presentation by default
    SetPresentationParameters( 1, 0 );
    // 60 hz update by default
    // Switch to constant time step for testing, this is to get comparable results between the test runs
    m_targetElapsedTime = 0;
    m_fixedTimeStep = FALSE;
    m_totalUpdateTime = 0;
    SetFixedTimestep( (m_thisIsTestRun || m_thisIsScreenshotRun)? DEFAULT_FRAME_TIME : 0 );

#ifdef XSF_USE_DX_12_0
#else
#if defined(_XBOX_ONE) && defined(_TITLE)
#else
    // Set up a break on error in D3D
    ID3D11InfoQueue* pD3DInfoQueue;
    if( SUCCEEDED( m_pd3dDevice->QueryInterface< ID3D11InfoQueue >( &pD3DInfoQueue ) ) )
    {
        pD3DInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, TRUE );
        pD3DInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE );
        pD3DInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_WARNING, TRUE );
        XSF_SAFE_RELEASE( pD3DInfoQueue );
    }

#if defined( DXGI_DEBUG_ENABLED )
    // Set up a break on error in DXGI
    IDXGIInfoQueue* pDXGIInfoQueue;
    if( SUCCEEDED( DXGIGetDebugInterface( __uuidof( IDXGIInfoQueue ), (void**)&pDXGIInfoQueue ) ) )
    {
        pDXGIInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE );
        pDXGIInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE );
        XSF_SAFE_RELEASE( pDXGIInfoQueue );
    }
#endif
#endif
#endif // XSF_USE_DX_12_0

    // Cache timing frequency
    QueryPerformanceFrequency( &m_ticksPerSecond );
    m_invTicksPerSecond = 1.f / static_cast< float >( m_ticksPerSecond.QuadPart );

    // Sample init & initial Resize
    Initialize();

#ifdef XSF_USE_DX_12_0
    // close the command list and use it to execute the initial GPU setup
    // Without this, none of the initialization work will execute
    WaitForPrevFrame(false, true);
#endif // XSF_USE_DX_12_0

    Resize(m_backbufferSwapChain.GetCreateDesc().m_Width, m_backbufferSwapChain.GetCreateDesc().m_Height);

    // Grab the time value after initialize is finished
    QueryPerformanceCounter( &m_lastTick );

    // Set denormals to flush, should be set by default at some point
    UINT control;
    _controlfp_s( &control, _DN_FLUSH, _MCW_DN );
}


//--------------------------------------------------------------------------------------
// Name: FrameworkShutdown
// Desc: Performs shutdown operations for the framework. This is the last call that is
// made to the framework
//--------------------------------------------------------------------------------------
void SampleFramework::FrameworkShutdown()
{
    XSF::CrossCheckpoint( XSF::CP_IN_FRAMEWORK_SHUTDOWN );

#ifdef XSF_USE_DX_12_0
    WaitForPrevFrame(true);
#else
#if defined(_XBOX_ONE) && defined(_TITLE)
    if (m_pd3dContext != nullptr)
    {
        UINT64 terminationFence = m_pd3dContext->InsertFence(0);
        while (m_pd3dDevice->IsFencePending(terminationFence)) { SwitchToThread(); }
    }
#endif

#endif // XSF_USE_DX_12_0

    OnShutdown();

    XSF::StockRenderStates::Shutdown();

#if defined( ATG_PROFILE ) || defined( ATG_PROFILE_VERBOSE )
    XSF::ATGProfiler::EmitResults( (XSF::ATGProfiler::OUTPUTFORMAT)(XSF::ATGProfiler::CSV_COMPATIBLE | XSF::ATGProfiler::FLAG_TO_FILE | XSF::ATGProfiler::FLAG_TO_STD_OUT | XSF::ATGProfiler::FLAG_TO_DEBUG_CHANNEL), L"ProfileResults.csv" );
#endif

    XSF::CloseLoggingFile();
}


//--------------------------------------------------------------------------------------
// Name: FrameworkSuspend
// Desc: This is called when the application goes out of focus. The sample should save
// its state as there might be no Resume after it
//--------------------------------------------------------------------------------------
void SampleFramework::FrameworkSuspend()
{
    XSF::CrossCheckpoint( XSF::CP_IN_FRAMEWORK_SUSPEND );

    // save the data or call virtual OnSuspend
    OnSuspend();

#if defined(_XBOX_ONE) && defined(_TITLE)
    // Call the D3D API for Suspending the GPU. This API preserves all necessary GPU state so 
    // that rendering can restart later when the title calls Resume. After calling D3D::Suspend,
    // the title may not submit any Draw calls until D3D::Resume is called.
    // All XDK samples have their Render() function called from the main game loop implemented
    // in IFrameworkView::Run, and this event handler is called via CoreApplication::ProcessEvents,
    // which is also called from IFrameworkView::Run, so it's guaranteed to run in the same thread.
    // For this reason, it's fine to call D3D::Suspend here without need for synchronization.
    // The same reasoning applies to the FrameworkResume event handler.
    // Titles that run their main rendering loop in a different thread need to use explicit
    // synchronization to coordinate between their suspending event handler and the 
    // rendering thread. For more details please see the GPUSuspendAndResume sample.
#ifdef XSF_USE_DX_12_0
    XSF_ERROR_IF_FAILED( GetCommandQueue()->SuspendX( 0 ) );
#else
    XSF_ERROR_IF_FAILED( GetImmediateContext()->Suspend( 0 ) );
#endif
#endif
}


//--------------------------------------------------------------------------------------
// Name: FrameworkResume
// Desc: Called when the application gets focus again. The sample should restore its state.
//--------------------------------------------------------------------------------------
void SampleFramework::FrameworkResume()
{
    XSF::CrossCheckpoint( XSF::CP_IN_FRAMEWORK_RESUME );

#if defined(_XBOX_ONE) && defined(_TITLE)
    // Reload the GPU state we last saved in suspending event handler (see FrameworkSuspend method) 
#ifdef XSF_USE_DX_12_0
    XSF_ERROR_IF_FAILED( GetCommandQueue()->ResumeX() );
#else
    XSF_ERROR_IF_FAILED( GetImmediateContext()->Resume() );
#endif
#endif

    m_justResumed = TRUE;

    // read the data or call virtual OnResume
    OnResume();
}


//--------------------------------------------------------------------------------------
// Name: FrameworkUpdateAndRender
// Desc: This is called as frequently as possible
//--------------------------------------------------------------------------------------
BOOL SampleFramework::FrameworkUpdateAndRender()
{
    if( m_bQuitRequested )
        return FALSE;

#ifdef XSF_USE_DX_12_0
    XSFScopedNamedEvent( m_pd3dCommandQueue, XSF_COLOR_FRAME, L"Frame %d", ms_frameStats.m_frameNumber );
    XSF::CrossCheckpoint( XSF::CP_IN_TICK );

#if defined(_XBOX_ONE) && defined(_TITLE)
    bool pixCaptureFrame = false;
    if( m_sampleSettings.m_pixCaptureAtFrame != UINT_MAX && m_sampleSettings.m_pixCaptureAtFrame == ms_frameStats.m_frameNumber )
    {
        WCHAR wstrExecutableName[256];
        XSF::GetExecutableName( wstrExecutableName, _countof(wstrExecutableName) );

        WCHAR wstrFilename[1024];
        _snwprintf_s( wstrFilename, _countof(wstrFilename), _TRUNCATE, L"%s%s%s", XSF::GetStorageFileRoot(), wstrExecutableName, L".pix3" );
        XSF_ERROR_IF_FAILED( m_pd3dCommandQueue->PIXGpuBeginCapture( 0x00290000, wstrFilename ) );
        pixCaptureFrame = true;
    }
#endif
#else
    XSFScopedNamedEvent( m_pd3dContext, XSF_COLOR_FRAME, L"Frame %d", ms_frameStats.m_frameNumber );
    XSF::CrossCheckpoint( XSF::CP_IN_TICK );
#if defined(_XBOX_ONE) && defined(_TITLE)
    bool pixCaptureFrame = false;
    if( m_sampleSettings.m_pixCaptureAtFrame != UINT_MAX && m_sampleSettings.m_pixCaptureAtFrame == ms_frameStats.m_frameNumber )
    {
        WCHAR wstrExecutableName[256];
        XSF::GetExecutableName( wstrExecutableName, _countof(wstrExecutableName) );

        WCHAR wstrFilename[1024];
        _snwprintf_s( wstrFilename, _countof(wstrFilename), _TRUNCATE, L"%s%s%s", XSF::GetStorageFileRoot(), wstrExecutableName, L".pix3" );
        XSF_ERROR_IF_FAILED( m_pd3dContext->PIXGpuBeginCapture( 0x00290000, wstrFilename ) );
        pixCaptureFrame = true;
    }
#endif
#endif

    LARGE_INTEGER currentTicks;
    LARGE_INTEGER deltaTicks;
    LARGE_INTEGER updateFinishedTicks;
    LARGE_INTEGER renderFinishedTicks;

    QueryPerformanceCounter( &currentTicks );

    deltaTicks.QuadPart = currentTicks.QuadPart - m_lastTick.QuadPart;
    const float deltaInSeconds = static_cast< float >( deltaTicks.QuadPart ) * m_invTicksPerSecond;
    m_lastTick.QuadPart = currentTicks.QuadPart;

    // How much to update this frame by
    float updateTime = deltaInSeconds;

    // If the execution has just resumed, the previous time is invalid so set frame time to TargetElapsedTime
    if( m_justResumed )
    {
        updateTime = DEFAULT_FRAME_TIME;
        m_justResumed = FALSE;
    }

    // Fixed time step is for testing. This will assume 60 Hz by default and can be set via SetFixedTimestep
    if( m_fixedTimeStep )
        updateTime = m_targetElapsedTime;

    XSF::CrossCheckpoint( XSF::CP_BEFORE_FRAME_UPDATE );
    const BOOL bKeepRunning = RunOneUpdate( m_totalUpdateTime, updateTime );
    XSF::CrossCheckpoint( XSF::CP_AFTER_FRAME_UPDATE );

    if( !bKeepRunning )
        return FALSE;

    m_totalUpdateTime += updateTime;
    m_lastUpdateTime = updateTime;

    QueryPerformanceCounter( &updateFinishedTicks );

    XSF::CrossCheckpoint( XSF::CP_BEFORE_FRAME_RENDER );
    RunOneRender();
    XSF::CrossCheckpoint( XSF::CP_AFTER_FRAME_RENDER );

    QueryPerformanceCounter( &renderFinishedTicks );

    // m_lastCpuMeasuredFrameTime counts time spent in this function
    deltaTicks.QuadPart = renderFinishedTicks.QuadPart - m_lastTick.QuadPart;
    ms_frameStats.m_lastCpuMeasuredFrameTime = static_cast< float >( deltaTicks .QuadPart ) * m_invTicksPerSecond;

    // this measures how long it took from the previous update
    ms_frameStats.m_measuredTimeSinceLastUpdate = deltaInSeconds;
    
    // render time on the cpu
    deltaTicks.QuadPart = renderFinishedTicks.QuadPart - updateFinishedTicks.QuadPart;
    ms_frameStats.m_lastCpuMeasuredRenderTime = static_cast< float >( deltaTicks .QuadPart ) * m_invTicksPerSecond;

    // update time on the cpu
    deltaTicks.QuadPart = updateFinishedTicks.QuadPart - m_lastTick.QuadPart;
    ms_frameStats.m_lastCpuMeasuredUpdateTime = static_cast< float >( deltaTicks .QuadPart ) * m_invTicksPerSecond;

    // Update the average frame stats
    ms_avgFrameStats.m_totalCpuMeasuredFrameTime += ms_frameStats.m_lastCpuMeasuredFrameTime;
    ms_avgFrameStats.m_totalCpuMeasuredRenderTime += ms_frameStats.m_lastCpuMeasuredRenderTime;
    ms_avgFrameStats.m_totalCpuMeasuredUpdateTime += ms_frameStats.m_lastCpuMeasuredUpdateTime;
    ms_avgFrameStats.m_totalGpuMeasuredFrameTime += ms_frameStats.m_lastGpuMeasuredFrameTime;
    ++ms_avgFrameStats.m_nFrames;

    if( ms_avgFrameStats.m_totalCpuMeasuredFrameTime >= cms_avgFrameStatsInterval )
    {
        ms_avgFrameStats.m_avgCpuMeasuredFrameTime = ms_avgFrameStats.m_totalCpuMeasuredFrameTime / ms_avgFrameStats.m_nFrames;
        ms_avgFrameStats.m_avgCpuMeasuredRenderTime = ms_avgFrameStats.m_totalCpuMeasuredRenderTime / ms_avgFrameStats.m_nFrames;
        ms_avgFrameStats.m_avgCpuMeasuredUpdateTime = ms_avgFrameStats.m_totalCpuMeasuredUpdateTime / ms_avgFrameStats.m_nFrames;
        ms_avgFrameStats.m_avgGpuMeasuredFrameTime = ms_avgFrameStats.m_totalGpuMeasuredFrameTime / ms_avgFrameStats.m_nFrames;

        ms_avgFrameStats.m_totalCpuMeasuredFrameTime = 0.0f;
        ms_avgFrameStats.m_totalCpuMeasuredRenderTime = 0.0f;
        ms_avgFrameStats.m_totalCpuMeasuredUpdateTime = 0.0f;
        ms_avgFrameStats.m_totalGpuMeasuredFrameTime = 0.0f;
        ms_avgFrameStats.m_nFrames = 0;
    }

    if( m_printTimings )
    {
        XSF::DebugPrint( "-------\nm_lastGpuMeasuredFrameTime = %f\n"
                         "m_lastCpuMeasuredFrameTime = %f\n"
                         "m_measuredTimeSinceLastUpdate = %f\n"
                         "m_lastCpuMeasuredRenderTime = %f\n"
                         "m_lastCpuMeasuredUpdateTime = %f\n",
                         1000 * ms_frameStats.m_lastGpuMeasuredFrameTime, 1000 * ms_frameStats.m_lastCpuMeasuredFrameTime,
                         1000 * ms_frameStats.m_measuredTimeSinceLastUpdate,
                         1000 * ms_frameStats.m_lastCpuMeasuredRenderTime, 1000 * ms_frameStats.m_lastCpuMeasuredUpdateTime );

#ifdef XSF_USE_DX_12_0
        D3D12_QUERY_DATA_PIPELINE_STATISTICS stats = GetStatistics().m_lastPipelineStatistics;
#else
        D3D11_QUERY_DATA_PIPELINE_STATISTICS stats = GetStatistics().m_lastPipelineStatistics;
#endif
        XSF::DebugPrint(    "IAVertices     = %d\n"
                            "IAPrimitives   = %d\n"
                            "VSInvocations  = %d\n"
                            "GSInvocations  = %d\n"
                            "GSPrimitives   = %d\n"
                            "CInvocations   = %d\n"
                            "CPrimitives    = %d\n"
                            "PSInvocations  = %d\n"
                            "HSInvocations  = %d\n"
                            "DSInvocations  = %d\n"
                            "CSInvocations  = %d\n",
                            stats.IAVertices, stats.IAPrimitives, stats.VSInvocations, stats.GSInvocations, stats.GSPrimitives,
                            stats.CInvocations, stats.CPrimitives, stats.PSInvocations, stats.HSInvocations, stats.DSInvocations, stats.CSInvocations );
    }
#if defined(_XBOX_ONE) && defined(_TITLE)
    if( pixCaptureFrame )
    {
#ifdef XSF_USE_DX_12_0
        XSF_ERROR_IF_FAILED( m_pd3dCommandQueue->PIXGpuEndCapture() );
#else
        XSF_ERROR_IF_FAILED( m_pd3dContext->PIXGpuEndCapture() );
#endif  // XSF_USE_DX_12_0
        if( !m_thisIsScreenshotRun )
        {
            return FALSE;
        }
    }
#endif 

    // run for a 100 frames in test mode
    if( CanClose() && ( ( m_thisIsTestRun && ms_frameStats.m_frameNumber >= NUM_FRAMES_TO_RUN_IN_TEST ) ||
                        ( m_thisIsPerfRun && m_totalUpdateTime >= NUM_SECONDS_TO_RUN_IN_TEST ) ) )
    {   
#if defined(ATG_PROFILE) || defined (ATG_PROFILE_VERBOSE)
        XSF::ATGProfiler::StopCapture();
#endif
        return FALSE;
    }

    // quit after screenshot
    if( CanScreenShot() && m_thisIsScreenshotRun )
    {
        return FALSE;
    }

    return TRUE;
}


//--------------------------------------------------------------------------------------
// Name: RunOneUpdate
// Desc: Perform one update of the framework and the sample
//--------------------------------------------------------------------------------------
BOOL SampleFramework::RunOneUpdate( FLOAT timeTotal, FLOAT timeDelta )
{
    // update user input from the controllers    
    AsyncReadInput();

    // Handle a convenient reboot sequence for all samples
    const XSF::GamepadReading& gamepadReading = m_input.GetCurrentGamepadReading();

    BOOL bQuitRequested = FALSE;
    if( m_sampleSettings.m_autoHandleRequestsForExit && ( ( gamepadReading.LeftTrigger()  > 0.5f ) &&
                                                        ( gamepadReading.RightTrigger() > 0.5f ) &&
                                                        ( gamepadReading.IsRightShoulderPressed() ) ) ||
                                                        GetKeyState( VK_ESCAPE ) )
    {
        bQuitRequested = TRUE;
    }

    // Call Sample update if not quitting
    if( !bQuitRequested )
    {
#ifdef XSF_USE_DX_12_0
        WaitForPrevFrame(false, true);
        TrimUploadHeaps();

        XSFBeginNamedEvent( m_pd3dCommandList, XSF_COLOR_UPDATE, L"SampleFramework::Update" );
        Update( timeTotal, timeDelta );
        XSFEndNamedEvent( m_pd3dCommandList );
#else
        XSFBeginNamedEvent( m_pd3dContext, XSF_COLOR_UPDATE, L"SampleFramework::Update" );
        Update( timeTotal, timeDelta );
        XSFEndNamedEvent( m_pd3dContext );
#endif
    }
    
    return !bQuitRequested;
}


//--------------------------------------------------------------------------------------
// Name: RunOneRender
// Desc: Perform one render frame of the framework and the sample
//--------------------------------------------------------------------------------------
void SampleFramework::RunOneRender()
{
#ifdef XSF_USE_DX_12_0
    if (SampleSettings().m_enablePerfQuery)
        m_perfQueries.OnNewFrame(m_pd3dCommandList, ms_frameStats.m_lastGpuMeasuredFrameTime, ms_frameStats.m_lastPipelineStatistics);

    // indicate this resource will be in use as a render target
    ID3D12Resource *pBackBuffer = m_backbufferSwapChain.GetBuffer();
    XSF::PrepareBackBufferForRendering(m_pd3dClearCommandList, pBackBuffer);

    m_backbufferSwapChain.SetAndClear(m_pd3dClearCommandList);
    XSF_ERROR_IF_FAILED(m_pd3dClearCommandList->Close());
    m_clearCommandListClosed = true;
    m_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)(&m_pd3dClearCommandList));

    m_backbufferSwapChain.SetAndClear(m_pd3dCommandList, nullptr, true);

    // sample render
    XSFBeginNamedEvent(m_pd3dCommandList, XSF_COLOR_RENDER, L"SampleFramework::Render" );
    Render();

    // indicate that the render target will now be used to present when the command list is done executing
    XSF::PrepareBackBufferForPresent(m_pd3dCommandList, pBackBuffer);

    if (SampleSettings().m_enablePerfQuery)
        m_perfQueries.OnEndFrame(m_pd3dCommandList);

    // all we need to do now is execute the command list
    XSFEndNamedEvent(m_pd3dCommandList);
    XSF_ERROR_IF_FAILED(m_pd3dCommandList->Close());
    m_commandListClosed = true;
    m_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)(&m_pd3dCommandList));
#else
    if( !SampleSettings().FastSemanticsEnabled() && SampleSettings().m_clearStateBeforeRender )
    {
        m_pd3dContext->ClearState();
    }

    if( SampleSettings().m_enablePerfQuery )
        m_perfQueries.OnNewFrame( m_pd3dContext, ms_frameStats.m_lastGpuMeasuredFrameTime, ms_frameStats.m_lastPipelineStatistics );

#if defined(_XBOX_ONE) && defined(_TITLE)
    ID3D11Texture2D *pBackBuffer = m_backbufferSwapChain.GetBuffer();
    ID3D11Texture2D *pFgBackBuffer = nullptr;
    ID3D11Texture2D *pBgBackBuffer = nullptr;

    if( SampleSettings().FastSemanticsEnabled() )
    {
        m_pd3dContext->InsertWaitOnPresent( 0, pBackBuffer );

        if( m_pFgPlane != nullptr && m_pFgPlane != &m_backbufferSwapChain )
        {
            pFgBackBuffer = m_pFgPlane->GetBuffer();
            m_pd3dContext->InsertWaitOnPresent( 0, pFgBackBuffer );
        }
        if( m_pBkPlane != nullptr && m_pBkPlane != &m_backbufferSwapChain && m_pBkPlane != m_pFgPlane )
        {
            pBgBackBuffer = m_pBkPlane->GetBuffer();
            m_pd3dContext->InsertWaitOnPresent( 0, pBgBackBuffer );
        }
    }
#endif

    m_backbufferSwapChain.SetAndClear( m_pd3dContext );

    // sample render
    XSFBeginNamedEvent( m_pd3dContext, XSF_COLOR_RENDER, L"SampleFramework::Render" );
    Render();
    XSFEndNamedEvent( m_pd3dContext );

#if defined(_XBOX_ONE) && defined(_TITLE)
    if( SampleSettings().FastSemanticsEnabled() )
    {
        // Decompress the back buffer and rebind it to the pipeline
        if( m_pd3dContext->GetResourceCompression( pBackBuffer ) != 0 )
        {
            m_pd3dContext->DecompressResource( pBackBuffer, 0, nullptr, pBackBuffer, 0, nullptr, DXGI_FORMAT_UNKNOWN, D3D11X_DECOMPRESS_ALL );
        }
        if( pFgBackBuffer != nullptr )
        {
            m_pd3dContext->DecompressResource( pFgBackBuffer, 0, nullptr, pFgBackBuffer, 0, nullptr, DXGI_FORMAT_UNKNOWN, D3D11X_DECOMPRESS_ALL );
        }
        if( pBgBackBuffer != nullptr )
        {
            m_pd3dContext->DecompressResource( pBgBackBuffer, 0, nullptr, pBgBackBuffer, 0, nullptr, DXGI_FORMAT_UNKNOWN, D3D11X_DECOMPRESS_ALL );
        }
        m_pd3dContext->OMSetRenderTargets( 1, m_backbufferSwapChain.GetRTVpp(), m_backbufferSwapChain.GetDSV() );
    }
#endif
#endif // XSF_USE_DX_12_0

    // increment frame number
    ms_frameStats.m_frameNumber++;

    // Handle a convenient screenshot
    const XSF::GamepadReading& gamepadReading = m_input.GetCurrentGamepadReading();
    BOOL bScreenshotRequested = FALSE;
    if( ( ( gamepadReading.IsLeftThumbstickPressed() ) || m_thisIsScreenshotRun ) && CanScreenShot() )
    {
        bScreenshotRequested = TRUE;
    }

    // VK_SNAPSHOT is reserved by the OS...
#ifdef _XBOX_ONE
    if( GetKeyState( 'P' /* VK_SNAPSHOT */ ) )
#else
    if( GetKeyState( VK_SNAPSHOT ) )
#endif
    {
        bScreenshotRequested = TRUE;
    }

    if( bScreenshotRequested )
    {
        Screenshot();
    }

    HRESULT hr = S_OK;
    
    do
    {
        hr = PresentNow();
    } while ( DXGI_ERROR_WAS_STILL_DRAWING == hr );

#ifdef XSF_USE_DX_12_0
    if (!m_backbufferSwapChain.GetCreateDesc().m_flipSequential)
    {
        m_backbufferSwapChain.CreateColorBufferViews(0);
    }
#else
    if (!m_backbufferSwapChain.GetCreateDesc().m_flipSequential && m_sampleSettings.FastSemanticsEnabled())
    {
        m_backbufferSwapChain.CreateColorBufferViews(0);
    }
#endif
}

//--------------------------------------------------------------------------------------
// Name: Resize
// Desc: Resize the buffers in response to a change in window size
//--------------------------------------------------------------------------------------
void SampleFramework::Resize( UINT width, UINT height )
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER( width );
    UNREFERENCED_PARAMETER( height );
#else
    SetBackbufferSize( width, height );
    CreateWindowSizeDependentResources( nullptr );
#endif
}


//--------------------------------------------------------------------------------------
// Name: Screenshot
// Desc: Take a screenshot of the backbuffer
//--------------------------------------------------------------------------------------
void SampleFramework::Screenshot()
{
    static UINT iCount = 0;
    WCHAR wstrFilename[1024] = L""; // The first shot will end up as screenshot000.png in the app's startup folder
    _snwprintf_s(wstrFilename, _countof(wstrFilename), _TRUNCATE, L"%s%s%3.3d%s", XSF::GetStorageFileRoot(), L"screenshot", iCount++, L".png");

#ifdef XSF_USE_DX_12_0
    ID3D12Resource* pBackBuffer = GetBackbuffer().GetBuffer();
    XSF_ASSERT(pBackBuffer != nullptr);

    XSF_ERROR_IF_FAILED(DirectX::SaveWICTextureToFile(m_pd3dDevice, m_pd3dCommandQueue, pBackBuffer, GUID_ContainerFormatPng, wstrFilename));
#else
    ID3D11Texture2D* pBackBuffer = GetBackbuffer().GetBuffer();
    XSF_ASSERT( pBackBuffer != nullptr );

    XSF_ERROR_IF_FAILED( DirectX::SaveWICTextureToFile( m_pd3dContext, pBackBuffer, GUID_ContainerFormatPng, wstrFilename ) );
#endif
}


//--------------------------------------------------------------------------------------
// Name: CreateDeviceResources
// Desc: Create D3D Device and the immediate context
//--------------------------------------------------------------------------------------
void SampleFramework::CreateDeviceResources()
{
#ifndef _XBOX_ONE
    if (GetCommandLineParameter(L"warp"))
    {
        m_sampleSettings.m_createReferenceDevice = TRUE;
    }
#endif

#ifdef XSF_USE_DX_12_0
#if defined(_XBOX_ONE) && defined(_TITLE)
    if (GetCommandLineParameter(L"profile"))
    {
        m_sampleSettings.m_deviceCreationFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
    }

    // check whether a pix capture is requested
    if ((m_sampleSettings.m_deviceCreationFlags & D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED) != 0)
    {
        GetCommandLineParameter(L"PixCaptureAtFrame", &m_sampleSettings.m_pixCaptureAtFrame);
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> spAdapter;

    XSF::D3DDebugPtr spDebug;
    XSF_ERROR_IF_FAILED(D3D12GetDebugInterface(IID_GRAPHICS_PPV_ARGS(spDebug.GetAddressOf())));
    spDebug->SetProcessDebugFlags(static_cast<D3D12XBOX_PROCESS_DEBUG_FLAGS>(m_sampleSettings.m_deviceCreationFlags));
#else
#if defined(_DEBUG) || defined(DEBUG)
    XSF::D3DDebugPtr spDebug;
    XSF_ERROR_IF_FAILED(D3D12GetDebugInterface(IID_GRAPHICS_PPV_ARGS(spDebug.GetAddressOf())));
    spDebug->EnableDebugLayer();
#endif
    //m_sampleSettings.m_createReferenceDevice ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
    Microsoft::WRL::ComPtr<IDXGIFactory4> spFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter3> spAdapter;
    if (m_sampleSettings.m_createReferenceDevice)
    {
        XSF_ERROR_IF_FAILED(CreateDXGIFactory2(0, IID_GRAPHICS_PPV_ARGS(spFactory.GetAddressOf())));
        XSF_ERROR_IF_FAILED(spFactory->EnumWarpAdapter(IID_GRAPHICS_PPV_ARGS(spAdapter.GetAddressOf())));
    }
#endif

    XSF_ERROR_IF_FAILED(D3D12CreateDevice(
        spAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_GRAPHICS_PPV_ARGS(&m_pd3dDevice)
        ));

    // create command queue and allocator objects
    for (UINT iFrame = 0; iFrame < m_sampleSettings.m_frameLatency; ++iFrame)
    {
        XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(&m_pd3dClearCommandAllocator[iFrame])));
        XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(&m_pd3dCommandAllocator[iFrame])));
    }
    

    // create command queue and command lists
    m_iFrame = 0;
    D3D12_COMMAND_QUEUE_DESC descQueue = {};
    descQueue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    descQueue.Priority = 0;
    descQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommandQueue(&descQueue, IID_GRAPHICS_PPV_ARGS(&m_pd3dCommandQueue)));

    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator[m_iFrame], nullptr, IID_GRAPHICS_PPV_ARGS(&m_pd3dCommandList)));
    m_commandListClosed = false;
    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dClearCommandAllocator[m_iFrame], nullptr, IID_GRAPHICS_PPV_ARGS(&m_pd3dClearCommandList)));
    XSF_ERROR_IF_FAILED(m_pd3dClearCommandList->Close());
    m_clearCommandListClosed = true;

    // create sync objects
    m_currentFence = 0;
    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateFence(m_currentFence++, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_spFence.GetAddressOf())));
    m_currentSignalFence = 0;
    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateFence(m_currentSignalFence++, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_spSignalFence.GetAddressOf())));
    ZeroMemory(m_allocatorFence, sizeof(m_allocatorFence));
    m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

#else
    // This array defines the ordering of feature levels that D3D should attempt to create.
    D3D_FEATURE_LEVEL featureLevels[] = 
    {
#ifdef XSF_USE_DX_11_1
        D3D_FEATURE_LEVEL_11_1,
#endif
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

#if defined(_XBOX_ONE) && defined(_TITLE)
    if( GetCommandLineParameter( L"profile" ) )
    {
        m_sampleSettings.m_deviceCreationFlags |= D3D11_CREATE_DEVICE_INSTRUMENTED;
    }
    if( GetCommandLineParameter( L"fastsemantics" ) )
    {
        m_sampleSettings.m_deviceCreationFlags |= D3D11_CREATE_DEVICE_IMMEDIATE_CONTEXT_FAST_SEMANTICS;
    }

    // check whether a pix capture is requested
    if( ( m_sampleSettings.m_deviceCreationFlags & D3D11_CREATE_DEVICE_INSTRUMENTED ) != 0 )
    {
        GetCommandLineParameter( L"PixCaptureAtFrame", &m_sampleSettings.m_pixCaptureAtFrame );
    }
#endif

    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* pD3DDevice;
    ID3D11DeviceContext* pD3DDeviceContext;
    XSF_ERROR_IF_FAILED(
        D3D11CreateDevice(
            nullptr,                    // specify null to use the default adapter
            m_sampleSettings.m_createReferenceDevice ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
            nullptr,                    // leave as null if hardware is used
            m_sampleSettings.m_deviceCreationFlags,// | D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY,
            featureLevels,
            _countof( featureLevels ),
            D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION
            &pD3DDevice,
            &featureLevel,
            &pD3DDeviceContext
        )
    );

    // You can disable this to test 11 codepaths on Xbox One if you need to
#ifdef XSF_USE_DX_11_1
    XSF_ERROR_IF_FAILED( pD3DDevice->QueryInterface( IID_GRAPHICS_PPV_ARGS(&m_pd3dDevice) ) );
    XSF_SAFE_RELEASE( pD3DDevice );

    XSF_ERROR_IF_FAILED( pD3DDeviceContext->QueryInterface( IID_GRAPHICS_PPV_ARGS(&m_pd3dContext) ) );
    XSF_SAFE_RELEASE( pD3DDeviceContext );

    XSF::DebugPrint( "Feature level: %x, 11.1 codepath ON\n", featureLevel );
#else
    m_pd3dDevice = pD3DDevice;
    m_pd3dContext = pD3DDeviceContext;

    XSF::DebugPrint( "Feature level: %x, 11.1 codepath OFF\n", featureLevel );
#endif
#endif // XSF_USE_DX_12_0
}

//--------------------------------------------------------------------------------------
// Name: DiscardDeviceResources
// Desc: Release all resources that framework created.
//--------------------------------------------------------------------------------------
void SampleFramework::DiscardDeviceResources()
{
#ifdef XSF_USE_DX_12_0
    WaitForPrevFrame(true);

    XSF_SAFE_RELEASE(m_pd3dCommandList);
    XSF_SAFE_RELEASE(m_pd3dCommandQueue);
    XSF_SAFE_RELEASE(m_pd3dClearCommandList);
    for (UINT iFrame = 0; iFrame < m_sampleSettings.m_frameLatency; ++iFrame)
    {
        XSF_SAFE_RELEASE(m_pd3dCommandAllocator[iFrame]);
        XSF_SAFE_RELEASE(m_pd3dClearCommandAllocator[iFrame]);
    }

    CloseHandle(m_fenceEvent);
    m_fenceEvent = 0;
#else
    if (m_pd3dContext)
    {
        m_pd3dContext->ClearState();
        XSF_SAFE_RELEASE(m_pd3dContext);
    }
#endif

    m_backbufferSwapChain.Destroy();
    XSF_SAFE_RELEASE(m_pd3dDevice);
}


//--------------------------------------------------------------------------------------
// Name: CreateWindowSizeDependentResources
// Desc: Create swap chain and all render target views
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void SampleFramework::CreateWindowSizeDependentResources( void* pWindow )
{
    if( m_pd3dDevice == nullptr )
    {
        return;
    }

#ifdef XSF_USE_DX_12_0
    WaitForPrevFrame(true);
    GetBackbuffer().ResetSwapChainBuffer();
#else
    if( m_pd3dContext != nullptr )
    {
        m_pd3dContext->ClearState();
    }
#endif

    if( pWindow == nullptr )
    {
        m_backbufferSwapChain.DestroySwapchainRTVs();
    }
    else
    {
        m_backbufferSwapChain.Destroy();
    }

    // If the swap chain already exists, resize it
    if( m_backbufferSwapChain.GetSwapChain() != nullptr )
    {
#if !(defined(_XBOX_ONE) && defined(_TITLE))
        XSF_ERROR_IF_FAILED( m_backbufferSwapChain.Resize( 0, m_sampleSettings.m_frameBufferWidth, m_sampleSettings.m_frameBufferHeight ) );
#endif

        m_backbufferSwapChain.CreateSwapChainRTVs();
    } else
    {
        // Forward to platform-dependent code
        CreateSwapChain( pWindow );
    }
}


#ifdef XSF_USE_DX_12_0
//--------------------------------------------------------------------------------------
// Name: SignalAndWait
// Desc: Signal the fence and wait for it to complete
//--------------------------------------------------------------------------------------
UINT64 SampleFramework::SignalAndWait()
{
    // signal and increment the fence value
    const UINT64 signalFence = m_currentSignalFence;
    XSF_ERROR_IF_FAILED(GetCommandQueue()->Signal(m_spSignalFence.Get(), m_currentSignalFence++));

    // wait for the fence value to pass
    if (m_spSignalFence->GetCompletedValue() < signalFence)
    {
        XSF_ERROR_IF_FAILED(m_spSignalFence->SetEventOnCompletion(signalFence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    return signalFence;
}

//--------------------------------------------------------------------------------------
// Name: WaitForPrevFrame
// Desc: Let the previous frame finish before continuing
//--------------------------------------------------------------------------------------
void SampleFramework::WaitForPrevFrame(bool waitAllFrames, bool resetAllocatorsAndCommandLists)
{
    if (GetCommandQueue() == nullptr)
        return;

    UINT endIndex = waitAllFrames ? m_sampleSettings.m_frameLatency : 1;
    for (UINT index = 0; index < endIndex; ++index)
    {
        if (resetAllocatorsAndCommandLists)
        {
            if (!m_clearCommandListClosed)
            {
                XSF_ERROR_IF_FAILED(m_pd3dClearCommandList->Close());
                m_clearCommandListClosed = true;
                m_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)(&m_pd3dClearCommandList));
            }

            if (!m_commandListClosed)
            {
                XSF_ERROR_IF_FAILED(m_pd3dCommandList->Close());
                m_commandListClosed = true;
                m_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)(&m_pd3dCommandList));
            }
        }

        // mark the fence for the current allocator
        m_allocatorFence[m_iFrame] = m_currentFence;
        XSF_ERROR_IF_FAILED(GetCommandQueue()->Signal(m_spFence.Get(), m_currentFence++));

        // increment to the next allocator
        m_iFrame = ++m_iFrame % m_sampleSettings.m_frameLatency;

        // wait for the fence value to pass
        const UINT64 allocatorFence = m_allocatorFence[m_iFrame];
        const UINT64 completedFence = m_spFence->GetCompletedValue();
        if (completedFence < allocatorFence)
        {
            XSF_ERROR_IF_FAILED(m_spFence->SetEventOnCompletion(allocatorFence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        if (resetAllocatorsAndCommandLists)
        {
            // command list allocators can be only be reset when the associated command lists have finished execution on the GPU
            // apps should use fences to determine GPU execution progress
            XSF_ERROR_IF_FAILED(m_pd3dCommandAllocator[m_iFrame]->Reset());
            XSF_ERROR_IF_FAILED(m_pd3dClearCommandAllocator[m_iFrame]->Reset());

            // however, when ExecuteCommandList() is called on a particular command list, that command list can then be reset anytime and must be before rerecording
            XSF_ERROR_IF_FAILED(m_pd3dClearCommandList->Reset(m_pd3dClearCommandAllocator[m_iFrame], nullptr));
            m_clearCommandListClosed = false;
            XSF_ERROR_IF_FAILED(m_pd3dCommandList->Reset(m_pd3dCommandAllocator[m_iFrame], nullptr));
            m_commandListClosed = false;
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: TrimUploadHeaps
// Desc: Terminates the upload heaps whose fence has passed and optionally removes them
//--------------------------------------------------------------------------------------
void SampleFramework::TrimUploadHeaps(bool removeTerminatedHeaps)
{
    const UINT64 fenceValue = m_spFence->GetCompletedValue();
    for (std::list<FencedHeap>::const_iterator iterManagedHeap = m_managedUploadHeaps.begin(); iterManagedHeap != m_managedUploadHeaps.end(); ++iterManagedHeap)
    {
        if (iterManagedHeap->m_pUploadHeap != nullptr && iterManagedHeap->m_fenceValue <= fenceValue)
        {
            iterManagedHeap->m_pUploadHeap->Terminate();
        }
    }

    if (removeTerminatedHeaps)
    {
        m_managedUploadHeaps.remove_if(is_heap_terminated());
    }
}

//--------------------------------------------------------------------------------------
// Name: ManageUploadHeap
// Desc: Add the upload heap to the managed list
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void SampleFramework::ManageUploadHeap(XSF::CpuGpuHeap* pUploadHeap)
{
    m_managedUploadHeaps.push_back(FencedHeap(pUploadHeap, GetCurrentFenceValue()));
}
#endif


//--------------------------------------------------------------------------------------
// Name: SetPresentationParameters
// Desc: Sets sync interval (0 -- no vsync, 1 - one vsync, 2 - two vsyncs, etc) and D3D11
// presentation flags
//--------------------------------------------------------------------------------------
void SampleFramework::SetPresentationParameters( UINT syncInterval, UINT flags, UINT threshold )
{
    m_presentationSyncInterval = syncInterval;
    m_presentationFlags = flags;
    m_presentImmediateThreshold = threshold;
}


//--------------------------------------------------------------------------------------
// Name: SetFixedTimestep
// Desc: Enables or disables fixed timestep. By default it's enabled because it allows
// for a deterministic debug replay.
//--------------------------------------------------------------------------------------
void SampleFramework::SetFixedTimestep( FLOAT fMilliseconds )
{
    m_targetElapsedTime = fMilliseconds;
    m_fixedTimeStep = fMilliseconds > 0;
    m_totalUpdateTime = 0;
}


//--------------------------------------------------------------------------------------
// Name: AsyncReadInput
// Desc: If ever you need to update gamepad and keyboard status mid-frame...
//--------------------------------------------------------------------------------------
void SampleFramework::AsyncReadInput()
{
    m_input.Update();

    // update user input from the keyboard
    memcpy( m_keyboardPreviousStatus, m_keyboardStatus, sizeof( m_keyboardStatus ) );
#ifndef _XBOX_ONE
    for( UINT i=0; i < _countof( m_keyboardPreviousStatus ); ++i )
        m_keyboardStatus[ i ] = GetAsyncKeyState( i ) >> 15;
#endif
}



//--------------------------------------------------------------------------------------
// Name: CreateSwapChain
// Desc: Create the swap chain
//--------------------------------------------------------------------------------------
void SampleFramework::CreateSwapChain( void* pWindow )
{
    ATGPROFILETHIS;

    SwapChainCreateDesc desc;

    desc.m_pWindow = pWindow;
    desc.m_Height = m_sampleSettings.m_frameBufferHeight;
    desc.m_Width = m_sampleSettings.m_frameBufferWidth;
    desc.m_Usage = m_sampleSettings.m_swapChainBufferUsage;
    desc.m_Format = m_sampleSettings.m_swapChainFormat;
    desc.m_DSFormat = m_sampleSettings.m_depthStencilFormat;
    desc.m_Flags = m_sampleSettings.m_swapChainCreateFlags;
    desc.m_clearBeforeRender = m_sampleSettings.m_clearBackbufferBeforeRender;
    desc.m_extraSwapChain = FALSE;
    desc.m_NumBuffers = m_sampleSettings.m_numBackbuffers;
#if defined(_XBOX_ONE) && defined(_TITLE)
    desc.m_noClearCompression = GetCommandLineParameter( L"NoClearCompression" );
#endif

    desc.m_flipSequential = TRUE;
#ifdef XSF_USE_DX_12_0
    
    XSF_ERROR_IF_FAILED(m_backbufferSwapChain.Create(m_pd3dDevice, m_pd3dCommandQueue, &desc));
#else
    XSF_ERROR_IF_FAILED(m_backbufferSwapChain.Create(m_pd3dDevice, &desc));
#endif
}


//--------------------------------------------------------------------------------------
// Name: GetBackbuffer
// Desc: Returns backbuffer SwapChain object
//--------------------------------------------------------------------------------------
SampleFramework::SwapChain&  SampleFramework::GetBackbuffer() const
{
    return const_cast< SwapChain& >( m_backbufferSwapChain );
}


//--------------------------------------------------------------------------------------
// Name: SetBackbufferSize
// Desc: Overrides the backbuffer size (must be called before the buffer is created)
//--------------------------------------------------------------------------------------
void SampleFramework::SetBackbufferSize( UINT width, UINT height )
{
    m_sampleSettings.m_frameBufferWidth = width;
    m_sampleSettings.m_frameBufferHeight = height;
}


//--------------------------------------------------------------------------------------
// Name: GetDepthStencilFormats
// Desc: From the given DS format, retrieve all the formats required to create DS resources
//--------------------------------------------------------------------------------------
bool SampleFramework::GetDepthStencilFormats( DXGI_FORMAT DSFormat, DXGI_FORMAT* pDSTypelessFormat, DXGI_FORMAT* pDepthFormat, DXGI_FORMAT* pStencilFormat )
{
    bool ret = true;
    DXGI_FORMAT DSTypelessFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT DepthFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT StencilFormat = DXGI_FORMAT_UNKNOWN;

    switch( DSFormat )
    {
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        DSTypelessFormat = DXGI_FORMAT_R24G8_TYPELESS;
        DepthFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        StencilFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        break;

    case DXGI_FORMAT_D32_FLOAT:
        DSTypelessFormat = DXGI_FORMAT_R32_TYPELESS;
        DepthFormat = DXGI_FORMAT_R32_FLOAT;
        StencilFormat = DXGI_FORMAT_R32_FLOAT;
        break;

    case DXGI_FORMAT_UNKNOWN:
    default:
        ret = false;
    }

    if( pDSTypelessFormat != nullptr )
    {
        *pDSTypelessFormat = DSTypelessFormat;
    }
    if( pDepthFormat != nullptr )
    {
        *pDepthFormat = DepthFormat;
    }
    if( pStencilFormat != nullptr )
    {
        *pStencilFormat = StencilFormat;
    }

    return ret;
}


//--------------------------------------------------------------------------------------
// Name: SwapChain
// Desc: Ctor
//--------------------------------------------------------------------------------------
SampleFramework::SwapChain::SwapChain() :   
#ifdef XSF_USE_DX_12_0    
    m_pd3dCmdQueue(nullptr),
#endif
    m_pd3dDevice(nullptr),
    m_backBuffer(0),
    m_backbuffersMax(1)
{
}


//--------------------------------------------------------------------------------------
// Name: ~SwapChain
// Desc: Destory swap chain
//--------------------------------------------------------------------------------------
SampleFramework::SwapChain::~SwapChain()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
// Name: Create
// Desc: Create swap chain
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
#ifdef XSF_USE_DX_12_0
HRESULT SampleFramework::SwapChain::Create( XSF::D3DDevice* const pDevice, XSF::D3DCommandQueue* const pCmdQueue, const SwapChainCreateDesc* pDesc )
#else
HRESULT SampleFramework::SwapChain::Create( XSF::D3DDevice* const pDevice, const SwapChainCreateDesc* pDesc )
#endif
{
    m_pd3dDevice = pDevice;
    m_pd3dDevice->AddRef();
    m_createDesc = *pDesc;

#ifdef XSF_USE_DX_12_0
    m_pd3dCmdQueue = pCmdQueue;
    m_pd3dCmdQueue->AddRef();
    m_backbuffersMax = pDesc->m_NumBuffers;

    XSF_ERROR_IF_FAILED(m_SRVHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DXGI_MAX_SWAP_CHAIN_BUFFERS + 2));
#else
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_backbuffersMax = ((m_pd3dDevice->GetCreationFlags() & D3D11_CREATE_DEVICE_IMMEDIATE_CONTEXT_FAST_SEMANTICS) != 0) ? pDesc->m_NumBuffers : 1;
#else
    m_backbuffersMax = 1;
#endif
#endif
    // Discard model only allows to query backbuffer at index 0
    if (!pDesc->m_flipSequential)
    {
        m_backbuffersMax = 1;
    }

    XSF_ERROR_IF_FAILED( PlatformCreate() );
    XSF_ERROR_IF_FAILED( CreateSwapChainRTVs() );
    
    XSF::DebugPrint( "Create swap chain res: %d x %d\n", pDesc->m_Width, pDesc->m_Height );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Resize
// Desc: Resize swap chain
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT SampleFramework::SwapChain::Resize( UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format )
{
    UNREFERENCED_PARAMETER( bufferCount );
 
    HRESULT hr = m_spSwapChain->ResizeBuffers( 0, width, height, format, 0 );

    DXGI_SWAP_CHAIN_DESC desc;
    if( SUCCEEDED( hr ) && SUCCEEDED( m_spSwapChain->GetDesc( &desc ) ) )
    {
        m_createDesc.m_Width = desc.BufferDesc.Width;
        m_createDesc.m_Height = desc.BufferDesc.Height;
        m_createDesc.m_Format = desc.BufferDesc.Format;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: Destroy
// Desc: Destory swap chain
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::Destroy()
{
    DestroySwapchainRTVs();

    PlatformDestroy();
}


#ifdef XSF_USE_DX_12_0
//--------------------------------------------------------------------------------------
// Name: SetAndClear
// Desc: Set and optionally clear render target & depth stencil views for the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void SampleFramework::SwapChain::SetAndClear( XSF::D3DCommandList* const pCommandList, const FLOAT* pColor, bool bSetOnly ) const
{
    XSFScopedNamedEventFunc( pCommandList, XSF_COLOR_SET_AND_CLEAR );
    
    const FLOAT color[4] = { 100.0f/255.0f, 149.f/255.f, 237.f/255.0f, 1.0f };

    if( !pColor )
    {
        pColor = color;
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtd = GetRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE dsd = GetDSV();
    if (m_createDesc.m_clearBeforeRender && !bSetOnly)
    {
        if (rtd.ptr != 0)
        {
            pCommandList->ClearRenderTargetView(rtd, pColor, 0, nullptr);
        }
        if (dsd.ptr != 0)
        {
#ifdef _XBOX_ONE
            D3D12XBOX_HISTENCIL_CONTROL control;
            control.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_EQUAL;
            control.State0.CompareValue = 0;
            control.State0.CompareMask = 0xFF;
            control.State0.Enabled = TRUE;
            control.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_NEVER;
            control.State1.CompareValue = 0;
            control.State1.CompareMask = 0;
            control.State1.Enabled = FALSE;
            pCommandList->SetHiStencilStateX(GetDepthStencil(), &control);
#endif
            pCommandList->ClearDepthStencilView(dsd, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
        }
    }

    pCommandList->RSSetViewports(1, &m_Viewport);
    pCommandList->RSSetScissorRects(1, &m_ScissorRect);
    pCommandList->OMSetRenderTargets(1, &rtd, TRUE, &dsd);
}

//--------------------------------------------------------------------------------------
// Name: CreateSwapchainRTVs
// Desc: Create render target views etc for the backbuffer
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::SwapChain::CreateSwapChainRTVs()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
    m_spSwapChain->GetDesc( &swapChainDesc );

    XSF_RETURN_IF_FAILED(m_DSHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1));
    XSF_RETURN_IF_FAILED(m_RTHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DXGI_MAX_SWAP_CHAIN_BUFFERS));
    for( UINT backBuffer = 0; backBuffer < m_backbuffersMax; backBuffer++ )
    {
        XSF_RETURN_IF_FAILED(CreateColorBufferViews(backBuffer));
    }

    XSF::DebugPrint( "CreateSwapChainRTVs: %d x %d\n", swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height );

    // Readable depth buffer needs an r24g8 typeless buffer format, a d24_s8 RTV and a R24_x8 SRV. note that on Xbox One that's wasting memory if you don't use stencil
    DXGI_FORMAT DSTypelessFormat;
    SampleFramework::GetDepthStencilFormats(m_createDesc.m_DSFormat, &DSTypelessFormat, nullptr, nullptr);
    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_RESOURCE_DESC tex2DDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DSTypelessFormat,
        swapChainDesc.BufferDesc.Width,
        swapChainDesc.BufferDesc.Height,
        1,
        1,
        1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    const D3D12_CLEAR_VALUE dsvClearValue = CD3DX12_CLEAR_VALUE(m_createDesc.m_DSFormat, 1.0f, 0);
    XSF_ERROR_IF_FAILED(m_pd3dDevice->CreateCommittedResource(
        &defaultHeapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &tex2DDesc, 
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &dsvClearValue, 
        IID_GRAPHICS_PPV_ARGS(m_spDepthStencil.ReleaseAndGetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    descDSV.Format = m_createDesc.m_DSFormat;
    m_pd3dDevice->CreateDepthStencilView( m_spDepthStencil, &descDSV, m_DSHeap.hCPU(0) );

    CreateDepthBufferViews();

    SetEffectiveArea( 0, 0, swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: DestroySwapchainRTVs
// Desc: Destroy render target views etc for the backbuffer
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::DestroySwapchainRTVs()
{
    for( UINT backBuffer = 0; backBuffer < m_backbuffersMax; backBuffer++ )
    {
        m_spBuffers[backBuffer].Reset();
    }
    m_RTHeap.Terminate();
    m_spDepthStencil.Reset();
    m_DSHeap.Terminate();
}
#else
//--------------------------------------------------------------------------------------
// Name: SetAndClear
// Desc: Set and optionally clear render target & depth stencil views for the backbuffer
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::SetAndClear( XSF::D3DDeviceContext* pCtx, const FLOAT* pColor, bool bSetOnly ) const
{
    XSFScopedNamedEventFunc( pCtx, XSF_COLOR_SET_AND_CLEAR );
    
    const FLOAT color[4] = { 100.0f/255.0f, 149.f/255.f, 237.f/255.0f, 1.0f };

    if( !pColor )
    {
        pColor = color;
    }
    
    if( m_createDesc.m_clearBeforeRender && !bSetOnly )
    {
        if( GetRTV() != nullptr )
        {
#if defined(_XBOX_ONE) && defined(_TITLE)
            const UINT clearFlags = m_createDesc.m_noClearCompression? D3D11X_CLEAR_COLOR_NO_COMPRESSION : 0;
            pCtx->ClearRenderTargetViewX( GetRTV(), clearFlags, pColor );
#else
            pCtx->ClearRenderTargetView( GetRTV(), pColor );
#endif
        }
        if( GetDSV() != nullptr )
            pCtx->ClearDepthStencilView( GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
    }

    pCtx->OMSetRenderTargets( 1, GetRTVpp(), GetDSV() );
    pCtx->RSSetViewports( 1, &m_Viewport );
}


//--------------------------------------------------------------------------------------
// Name: CreateSwapchainRTVs
// Desc: Create render target views etc for the backbuffer
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::SwapChain::CreateSwapChainRTVs()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
    m_spSwapChain->GetDesc( &swapChainDesc );

    for( UINT backBuffer = 0; backBuffer < m_backbuffersMax; backBuffer++ )
    {
        XSF_RETURN_IF_FAILED(CreateColorBufferViews(backBuffer));
    }

    XSF::DebugPrint( "CreateSwapChainRTVs: %d x %d\n", swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height );

    // only create the depth buffer if the format is specified and supported
    DXGI_FORMAT DSTypelessFormat;
    if( SampleFramework::GetDepthStencilFormats( m_createDesc.m_DSFormat, &DSTypelessFormat, nullptr, nullptr ) )
    {
        // Readable depth buffer needs an r24g8 typeless buffer format, a d24_s8 RTV and a R24_x8 SRV. note that on Xbox One that's wasting memory if you don't use stencil
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            DSTypelessFormat,
            swapChainDesc.BufferDesc.Width,
            swapChainDesc.BufferDesc.Height,
            1,
            1,
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE );

        XSF_RETURN_IF_FAILED( m_pd3dDevice->CreateTexture2D( &depthStencilDesc, nullptr, m_spDepthStencil.GetAddressOf() ) );

        const CD3D11_DEPTH_STENCIL_VIEW_DESC descDSV( D3D11_DSV_DIMENSION_TEXTURE2D, m_createDesc.m_DSFormat );
        XSF_RETURN_IF_FAILED( m_pd3dDevice->CreateDepthStencilView( m_spDepthStencil, &descDSV, m_spDSV.GetAddressOf() ) );

        CreateDepthBufferViews();
    }
    
    SetEffectiveArea( 0, 0, swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: DestroySwapchainRTVs
// Desc: Destroy render target views etc for the backbuffer
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::DestroySwapchainRTVs()
{
    for( UINT backBuffer = 0; backBuffer < m_backbuffersMax; backBuffer++ )
    {
        m_spBuffers[backBuffer].Reset();
        m_spRTVs[backBuffer].Reset();
        m_spColorUAVs[backBuffer].Reset();
        m_spColorSRVs[backBuffer].Reset();
    }
    m_spDepthStencil.Reset();
    m_spDepthSRV.Reset();
    m_spStencilSRV.Reset();
    m_spDSV.Reset();
}
#endif // XSF_USE_DX_12_0


//--------------------------------------------------------------------------------------
// Name: GetCreateDesc
// Desc: Return swap chain creation parameters
//--------------------------------------------------------------------------------------
const SampleFramework::SwapChainCreateDesc&  SampleFramework::SwapChain::GetCreateDesc() const
{
    return m_createDesc;
}


//--------------------------------------------------------------------------------------
// Name: SetEffectiveArea
// Desc: Sets the active viewport
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::SetEffectiveArea( UINT x, UINT y, UINT w, UINT h )
{
    XSF_ASSERT( x <= m_createDesc.m_Width );
    XSF_ASSERT( y <= m_createDesc.m_Height );
    XSF_ASSERT( x + w <= m_createDesc.m_Width );
    XSF_ASSERT( y + h <= m_createDesc.m_Height );

    m_useThisArea[ 0 ] = x;
    m_useThisArea[ 1 ] = y;
    m_useThisArea[ 2 ] = w;
    m_useThisArea[ 3 ] = h;

    m_Viewport.Width = static_cast<FLOAT>(w);
    m_Viewport.Height = static_cast<FLOAT>(h);
    m_Viewport.TopLeftX = static_cast<FLOAT>(x);
    m_Viewport.TopLeftY = static_cast<FLOAT>(y);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

#ifdef XSF_USE_DX_12_0
    m_ScissorRect = CD3DX12_RECT(x, y, w, h);
#endif
}


//--------------------------------------------------------------------------------------
// Name: GetRotatedBuffer
// Desc: Get the index of the active buffer in the swap chain
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
UINT SampleFramework::SwapChain::GetRotatedBuffer() const
{
    return m_backBuffer;
}


//--------------------------------------------------------------------------------------
// Name: RotateSwapChainBuffer
// Desc: Rotate the swap chain buffer so all the views point to the active buffer. 
//       Required for fast semantics.
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::RotateSwapChainBuffer()
{
    m_backBuffer = ( m_backBuffer + 1 ) % m_backbuffersMax;
}


//--------------------------------------------------------------------------------------
// Name: ResetSwapChainBuffer
// Desc: Reset the index of the active swap chain buffer
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::ResetSwapChainBuffer()
{
    m_backBuffer = 0;
}


//--------------------------------------------------------------------------------------
// Name: CreateColorBufferViews
// Desc: Create a backbuffer views or descriptors
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT SampleFramework::SwapChain::CreateColorBufferViews(UINT bufferIndex)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    m_spSwapChain->GetDesc(&swapChainDesc);

    UINT index = bufferIndex < m_createDesc.m_NumBuffers ? bufferIndex : m_backBuffer;
#ifdef XSF_USE_DX_12_0
    XSF_RETURN_IF_FAILED(m_spSwapChain->GetBuffer(index, IID_GRAPHICS_PPV_ARGS(m_spBuffers[index].ReleaseAndGetAddressOf())));
    m_pd3dDevice->CreateRenderTargetView(m_spBuffers[index], nullptr, m_RTHeap.hCPU(index));

    //  Create a shader view if we are using the depth buffer directly.
    if (swapChainDesc.BufferUsage & DXGI_USAGE_SHADER_INPUT)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Format = swapChainDesc.BufferDesc.Format;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_pd3dDevice->CreateShaderResourceView(m_spBuffers[index], &srvDesc, m_SRVHeap.hCPU(index));
    }
#else
    XSF_RETURN_IF_FAILED(m_spSwapChain->GetBuffer(index, IID_GRAPHICS_PPV_ARGS(m_spBuffers[index].ReleaseAndGetAddressOf())));
    XSF_RETURN_IF_FAILED(m_pd3dDevice->CreateRenderTargetView(m_spBuffers[index], nullptr, m_spRTVs[index].ReleaseAndGetAddressOf()));

    //  Create a shader view if we are using the depth buffer directly.
    if (swapChainDesc.BufferUsage & DXGI_USAGE_SHADER_INPUT)
    {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, swapChainDesc.BufferDesc.Format, 0, 1);
        XSF_RETURN_IF_FAILED(m_pd3dDevice->CreateShaderResourceView(m_spBuffers[index], &srvDesc, m_spColorSRVs[index].ReleaseAndGetAddressOf()));
    }

    // Create color buffer UAV
    if (m_pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 && (swapChainDesc.BufferUsage & DXGI_USAGE_UNORDERED_ACCESS))
    {
        CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_TEXTURE2D, swapChainDesc.BufferDesc.Format);
        XSF_RETURN_IF_FAILED(m_pd3dDevice->CreateUnorderedAccessView(m_spBuffers[index], &uavDesc, m_spColorUAVs[index].ReleaseAndGetAddressOf()));
    }
#endif

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: CreateDepthBufferViews
// Desc: Create a depth/stencil views or descriptors
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::SwapChain::CreateDepthBufferViews()
{
    DXGI_FORMAT DepthFormat;
    DXGI_FORMAT StencilFormat;

    SampleFramework::GetDepthStencilFormats( m_createDesc.m_DSFormat, nullptr, &DepthFormat, &StencilFormat );

#ifdef XSF_USE_DX_12_0
    // create depth source SRV
    {
        //  Create a shader view if we are using the depth buffer directly.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Format = DepthFormat;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_pd3dDevice->CreateShaderResourceView(m_spDepthStencil, &srvDesc, m_SRVHeap.hCPU(DXGI_MAX_SWAP_CHAIN_BUFFERS));
    }

    // create stencil source SRV
    {
        //  Create a shader view if we are using the depth buffer directly.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Format = StencilFormat;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_pd3dDevice->CreateShaderResourceView(m_spDepthStencil, &srvDesc, m_SRVHeap.hCPU(DXGI_MAX_SWAP_CHAIN_BUFFERS + 1));
    }
#else
    // create depth source SRV
    {
        //  Create a shader view if we are using the depth buffer directly.
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc( D3D11_SRV_DIMENSION_TEXTURE2D, DepthFormat, 0, 1 );
        XSF_RETURN_IF_FAILED( m_pd3dDevice->CreateShaderResourceView( m_spDepthStencil, &srvDesc, m_spDepthSRV.ReleaseAndGetAddressOf() ) );
    }

    // create stencil source SRV
    {
        //  Create a shader view if we are using the depth buffer directly.
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc( D3D11_SRV_DIMENSION_TEXTURE2D, StencilFormat, 0, 1 );
        XSF_RETURN_IF_FAILED( m_pd3dDevice->CreateShaderResourceView( m_spDepthStencil, &srvDesc, m_spStencilSRV.ReleaseAndGetAddressOf() ) );
    }
#endif

    return S_OK;
}


