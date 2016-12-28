//--------------------------------------------------------------------------------------
// SampleFramework.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_H_INCLUDED
#define XSF_H_INCLUDED

#ifdef _XBOX_ONE
#include <xdk.h>
#ifndef _XDK_VER
#error XDK version not found
#endif

#if !defined( MIN_EXPECTED_XDK_VER ) || !defined( MAX_EXPECTED_XDK_VER )
#error Framework should define XDK expected version range
#endif

// We only test the samples against the XDK they were published with.  If you decide to comment out these errors,
// we recommend leaving the #pragma message lines enabled.
#define STRINGIFY1(a) #a
#define STRINGIFY(a) STRINGIFY1(a)
#define _XDK_VER_STR STRINGIFY(_XDK_VER)
#define MIN_EXPECTED_XDK_VER_STR STRINGIFY(MIN_EXPECTED_XDK_VER)
#define MAX_EXPECTED_XDK_VER_STR STRINGIFY(MAX_EXPECTED_XDK_VER)
#if defined( MIN_EXPECTED_XDK_VER ) && MIN_EXPECTED_XDK_VER > _XDK_VER
#pragma message ("Warning: Installed XDK version ("_XDK_VER_STR") is too old.  Sample expects at least version ("MIN_EXPECTED_XDK_VER_STR").")
#error Ending compilation due to invalid XDK version
#endif
#if defined( MAX_EXPECTED_XDK_VER ) && MAX_EXPECTED_XDK_VER < _XDK_VER
#pragma message ("Warning: Installed XDK version ("_XDK_VER_STR") is too new.  Sample expects no later than version ("MAX_EXPECTED_XDK_VER_STR").")
#error Ending compilation due to invalid XDK version
#endif
#endif

// if this is defined, windows.h will define min/max as macros
// that will make std::min and max cause compile errors
#define    NOMINMAX
#include <Windows.h>


// define XSF_USE_DX_11_1 to use 11.1
#ifndef XSF_USE_DX_12_0
#define XSF_USE_DX_11_1
#endif

#include <wrl.h>

// DURANGO
#if defined(_XBOX_ONE)

#if defined(_TITLE)

#if defined(_RELEASE)
#define D3DCOMPILE_NO_DEBUG 1
#endif

#include <d3dcompiler_x.h>
#include <xg.h>

#ifdef XSF_USE_DX_12_0
#include <d3d12_x.h>
#include <d3dx12_x.h>
#else
#include <d3d11_x.h>
#endif

#define DCOMMON_H_INCLUDED

#else

#include <d3dcompiler.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#endif  // TITLE

#include <DirectXMath.h>
#include <directxpackedvector.h>

// XAudio2 is Xbox One only for now
#include <xaudio2.h>
#include <xma2defs.h>

#else   // PC

#include <d3dcompiler.h>
#ifdef XSF_USE_DX_12_0
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#define D3D12_SDK_VERSION_MINOR     D3D12_SDK_VERSION
#elif defined(XSF_USE_DX_11_1)
#include <d3d11_1.h>
#else
#include <d3d11.h>
#endif
#include <DirectXMath.h>
#include <directxpackedvector.h>

#endif

namespace DirectX
{
#if (DIRECTX_MATH_VERSION < 305) && !defined(XM_CALLCONV)
#define XM_CALLCONV __fastcall
typedef const XMVECTOR& HXMVECTOR;
typedef const XMMATRIX& FXMMATRIX;
#endif
};

using namespace DirectX;
using namespace DirectX::PackedVector;


#include <stdio.h>
#include <sal.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <string>
#include <map>

// sample framework related
#include "Perf.h"
#include "Common.h"
#ifdef XSF_USE_DX_12_0
#include "D3D12Util.h"
#include "GpuPerformanceQueries12.h"
#include "StockRenderStates12.h"
#else
#include "GpuPerformanceQueries.h"
#include "gpucounters.h"
#include "StockRenderStates.h"
#endif
#include "Input.h"


//--------------------------------------------------------------------------------------
// Name: SampleFramework
// Desc: Base class for all samples
//--------------------------------------------------------------------------------------
class SampleFramework
{
public:
    static const UINT c_MaxFrameLatency = DXGI_MAX_SWAP_CHAIN_BUFFERS;

    // Contains settings that can be overridden by the sample
    struct Settings
    {
        UINT        m_deviceCreationFlags;
        UINT        m_frameBufferWidth;
        UINT        m_frameBufferHeight;
        _Field_range_(1,DXGI_MAX_SWAP_CHAIN_BUFFERS) UINT m_numBackbuffers;
        DXGI_FORMAT m_swapChainFormat;
        DXGI_FORMAT m_depthStencilFormat;
        UINT        m_swapChainBufferUsage;
        UINT        m_swapChainCreateFlags;
        BOOL        m_createReferenceDevice;
        BOOL        m_clearBackbufferBeforeRender;
        BOOL        m_clearStateBeforeRender;
        BOOL        m_autoHandleRequestsForExit;    // If FALSE, the SampleFramework will not handle requests to exit from users. Instead, the application will be
                                                    // responsible to initiate the shutdown process by calling RequestShutdown().
        BOOL        m_canScreenshot;
        BOOL        m_enablePerfQuery;

#if defined(_XBOX_ONE) && defined(_TITLE)
        UINT        m_pixCaptureAtFrame;

        BOOL        FastSemanticsEnabled() const { return (m_deviceCreationFlags & D3D11_CREATE_DEVICE_IMMEDIATE_CONTEXT_FAST_SEMANTICS) != 0; }
#else
        BOOL        FastSemanticsEnabled() const { return FALSE; }
#endif

#ifdef XSF_USE_DX_12_0
        _Field_range_(0, c_MaxFrameLatency - 1) UINT m_frameLatency;
#endif
    };
    const Settings& SampleSettings() const { return m_sampleSettings; }

    struct SwapChainCreateDesc
    {
        void*       m_pWindow;
        UINT        m_Width;
        UINT        m_Height;
        DXGI_FORMAT m_Format;
        DXGI_FORMAT m_DSFormat;
        UINT        m_Usage;
        UINT        m_Flags;
        _Field_range_(1,DXGI_MAX_SWAP_CHAIN_BUFFERS) UINT m_NumBuffers;
        UINT        m_clearBeforeRender : 1;
		UINT        m_extraSwapChain    : 1;
		UINT        m_flipSequential    : 1;
#if defined(_XBOX_ONE) && defined(_TITLE)
		UINT        m_noClearCompression : 1;
#endif
    };

    class SwapChain
    {
        XSF::D3DDevice*                     m_pd3dDevice;
        SwapChainCreateDesc                 m_createDesc;
#ifdef XSF_USE_DX_12_0
        XSF::D3DCommandQueue*               m_pd3dCmdQueue;
        XSF::D3DResourcePtr                 m_spBuffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
        XSF::DescriptorHeapWrapper          m_RTHeap;
        XSF::D3DResourcePtr                 m_spDepthStencil;
        XSF::DescriptorHeapWrapper          m_DSHeap;
        D3D12_VIEWPORT                      m_Viewport;
        D3D12_RECT                          m_ScissorRect;
        XSF::DescriptorHeapWrapper          m_SRVHeap;
#else
        XSF::D3DTexture2DPtr                m_spBuffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
        XSF::D3DRenderTargetViewPtr         m_spRTVs[DXGI_MAX_SWAP_CHAIN_BUFFERS];
        XSF::D3DTexture2DPtr                m_spDepthStencil;
        XSF::D3DDepthStencilViewPtr         m_spDSV;
        XSF::D3DShaderResourceViewPtr       m_spDepthSRV;
        XSF::D3DShaderResourceViewPtr       m_spStencilSRV;
        XSF::D3DShaderResourceViewPtr       m_spColorSRVs[DXGI_MAX_SWAP_CHAIN_BUFFERS];
        XSF::D3DUnorderedAccessViewPtr      m_spColorUAVs[DXGI_MAX_SWAP_CHAIN_BUFFERS];
        D3D11_VIEWPORT                      m_Viewport;
#endif
        XSF::D3DTypePtr<XSF::DXGISwapChain> m_spSwapChain;
        UINT                                m_useThisArea[ 4 ];
        _Field_range_(0,DXGI_MAX_SWAP_CHAIN_BUFFERS-1) UINT m_backBuffer;
        _Field_range_(0,DXGI_MAX_SWAP_CHAIN_BUFFERS) UINT m_backbuffersMax;    // with fast semantics, it's the number of back buffers, else it's 0

    public:
        SwapChain();
        ~SwapChain();

        
        HRESULT Resize( _In_opt_ UINT bufferCount = 0, _In_opt_ UINT width = 0, _In_opt_ UINT height = 0, _In_opt_ DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN );
        void Destroy();

        HRESULT CreateSwapChainRTVs();
        void DestroySwapchainRTVs();

#ifdef XSF_USE_DX_12_0
        HRESULT Create( _In_ XSF::D3DDevice* const pDevice, _In_ XSF::D3DCommandQueue* const pCmdQueue, const SwapChainCreateDesc* pDesc );
        void SetAndClear( _In_ XSF::D3DCommandList* pCommandList, const FLOAT* pColor = nullptr, bool bSetOnly = false ) const;
#else
        HRESULT Create( _In_ XSF::D3DDevice* pDev, const SwapChainCreateDesc* pDesc );
        void SetAndClear( _In_ XSF::D3DDeviceContext* pCtx, const FLOAT* pColor = nullptr, bool bSetOnly = false ) const;
#endif

        void SetEffectiveArea( UINT x, UINT y, UINT w, UINT h );

        const SwapChainCreateDesc&  GetCreateDesc() const;

#ifdef XSF_USE_DX_12_0
        void SetResourceHeap( _In_ XSF::DescriptorHeapWrapper* const pResourceHeap, UINT baseIndex );
        ID3D12Resource* GetBuffer( _In_opt_ UINT bufferIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV( _In_opt_ UINT rtIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetColorSRV( _In_opt_ UINT srvIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D12Resource* GetDepthStencil() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const;
        const D3D12_VIEWPORT& GetViewport() const;
        const D3D12_RECT& GetScissorRect() const;
#else
        ID3D11Texture2D* GetBuffer( _In_opt_ UINT bufferIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D11RenderTargetView* GetRTV( _In_opt_ UINT rtIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D11RenderTargetView* const * GetRTVpp( _In_opt_ UINT rtIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D11ShaderResourceView* GetColorSRV( _In_opt_ UINT srvIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D11UnorderedAccessView* GetColorUAV( _In_opt_ UINT uavIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS ) const;
        ID3D11Texture2D* GetDepthStencil() const;
        ID3D11DepthStencilView* GetDSV() const;
        ID3D11ShaderResourceView* GetDepthSRV() const;
        ID3D11ShaderResourceView* GetStencilSRV() const;
        const D3D11_VIEWPORT& GetViewport() const;
#endif
        XSF::DXGISwapChain* GetSwapChain() const;
        const UINT* GetEffectiveArea() const;

        _Out_range_(0,DXGI_MAX_SWAP_CHAIN_BUFFERS-1) UINT GetRotatedBuffer() const;
        void RotateSwapChainBuffer();
        void ResetSwapChainBuffer();
        HRESULT CreateColorBufferViews(_In_opt_ UINT bufferIndex = DXGI_MAX_SWAP_CHAIN_BUFFERS);
        HRESULT CreateDepthBufferViews();

    private:
        HRESULT PlatformCreate();
        void PlatformDestroy();
    };

public:
    SampleFramework();
    virtual ~SampleFramework();

    // To be overridden by the sample class
    virtual void ModifySettings( Settings& /*settings*/ ) {}
    virtual void Initialize() = 0;
    virtual void Resize( UINT width, UINT height );
    virtual void Update( FLOAT timeTotal, FLOAT timeDelta ) = 0;
    virtual void Render() = 0;
    virtual void OnVisibilityChanged( bool visible ) { UNREFERENCED_PARAMETER( visible ); }
    virtual void OnSuspend()    {}
    virtual void OnResume()     {}
    virtual void OnShutdown()   {}
#if defined(_XBOX_ONE) && defined(_TITLE)
    virtual void OnResourceAvailabilityChanged( Windows::ApplicationModel::Core::ResourceAvailability resourceAvailability ) { UNREFERENCED_PARAMETER( resourceAvailability ); }
#endif

    // device and context
    XSF::D3DDevice* GetDevice() const;
#ifdef XSF_USE_DX_12_0
    UINT GetFrameIndex() const { return m_iFrame; }
    XSF::D3DCommandAllocator* GetCommandAllocator() const { return m_pd3dCommandAllocator[m_iFrame]; }
    XSF::D3DCommandQueue* GetCommandQueue() const { return m_pd3dCommandQueue; }
    XSF::D3DCommandList* GetCommandList() const { return m_pd3dCommandList; }
    ID3D12Fence* GetFence() const { return m_spFence.Get(); }
    UINT64 GetCurrentFenceValue() const { return m_currentFence; }
#else
    XSF::D3DDeviceContext* GetImmediateContext() const;
#endif

    // backbuffer
    SwapChain& GetBackbuffer() const;
    void SetBackbufferSize( UINT width, UINT height );

    // presentation and update
#if defined(_XBOX_ONE) && defined(_TITLE)
    void SetPresentSurfaces( const SwapChain* pBackground, const SwapChain* pForeground );
    void SetPresentSurfaceFilterFlags( DWORD BackgroundFlags, DWORD ForegroundFlags );
#endif
    void SetPresentationParameters( UINT syncInterval, UINT flags = 0, UINT threshold = 0 );
    void SetFixedTimestep( FLOAT fMilliseconds );

    // startup
    void ParseCommandLine( _In_opt_z_ const wchar_t* commandLine = nullptr );
    BOOL GetCommandLineParameter( _In_z_ const wchar_t* const name, _Out_opt_ float *pValue = nullptr ) const;
    BOOL GetCommandLineParameter( _In_z_ const wchar_t* const name, _Out_ BOOL *pValue ) const;
    template<typename T>
    BOOL GetCommandLineParameter( _In_z_ const wchar_t* const name, _Out_ T *pValue ) const;
#if defined(_XBOX_ONE) && defined(_TITLE)
    virtual void OnProtocolActivation( Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ args ) { }
#endif

    // input
    const XSF::GamepadReading& GetGamepadReading() const;
#ifdef _XBOX_ONE
     Windows::Xbox::Input::IGamepad^ GetGamepad() const;
#endif
    UINT    GetKeyState( BYTE bVk ) const;
    BOOL    IsKeyPressed( BYTE bVk ) const;
    VOID    SetKeyDown( BYTE bVk );
    VOID    SetKeyUp( BYTE bVk );

    // GPU timing and statistics queries, API level
    XSF::GpuPerformanceQueries   m_perfQueries;

#ifndef XSF_USE_DX_12_0
    // GPU timing, hardware level
    GPUCounters         m_gpuCounters;
#endif

public:
    // Helps to track some timings in one place
    struct FrameStatistics
    {
        UINT            m_frameNumber;
        FLOAT           m_lastGpuMeasuredFrameTime;
        FLOAT           m_lastCpuMeasuredFrameTime;
        FLOAT           m_lastCpuMeasuredUpdateTime;
        FLOAT           m_lastCpuMeasuredRenderTime;
        FLOAT           m_measuredTimeSinceLastUpdate;
#ifdef XSF_USE_DX_12_0
        D3D12_QUERY_DATA_PIPELINE_STATISTICS    m_lastPipelineStatistics;
#else
        D3D11_QUERY_DATA_PIPELINE_STATISTICS    m_lastPipelineStatistics;
#endif
    };

    // Helps to track some timings in one place
    struct AveragedFrameStatistics
    {
        UINT            m_nFrames;
        FLOAT           m_totalGpuMeasuredFrameTime;
        FLOAT           m_totalCpuMeasuredFrameTime;
        FLOAT           m_totalCpuMeasuredUpdateTime;
        FLOAT           m_totalCpuMeasuredRenderTime;
        FLOAT           m_avgGpuMeasuredFrameTime;
        FLOAT           m_avgCpuMeasuredFrameTime;
        FLOAT           m_avgCpuMeasuredUpdateTime;
        FLOAT           m_avgCpuMeasuredRenderTime;

        AveragedFrameStatistics() :
            m_nFrames( 0 ),
            m_totalGpuMeasuredFrameTime( 0.0f ),
            m_totalCpuMeasuredFrameTime( 0.0f ),
            m_totalCpuMeasuredUpdateTime( 0.0f ),
            m_totalCpuMeasuredRenderTime( 0.0f ),
            m_avgGpuMeasuredFrameTime( 0.0f ),
            m_avgCpuMeasuredFrameTime( 0.0f ),
            m_avgCpuMeasuredUpdateTime( 0.0f ),
            m_avgCpuMeasuredRenderTime( 0.0f )
        {};
    };

    static const FrameStatistics& GetStatistics();
    FLOAT    GetLastGpuFrameTime() const;
    FLOAT    GetLastCpuFrameTime() const;
    static const AveragedFrameStatistics& GetAvgStatistics();
    FLOAT    GetAvgGpuFrameTime() const;
    FLOAT    GetAvgCpuFrameTime() const;

    void Screenshot();

    static bool GetDepthStencilFormats( DXGI_FORMAT DSFormat, _Out_opt_ DXGI_FORMAT* pDSTypelessFormat, _Out_opt_ DXGI_FORMAT* pDepthFormat, _Out_opt_ DXGI_FORMAT* pStencilFormat );

public:
    BOOL    InTestRun() const
    {
        return m_thisIsTestRun || m_thisIsPerfRun || m_thisIsScreenshotRun;
    }
    BOOL    InStressRun() const
    {
        return m_thisIsStressRun;
    }

protected:
    virtual BOOL CanScreenShot() const { return m_sampleSettings.m_canScreenshot; }
    virtual BOOL CanClose() const { return true; }
    void    AsyncReadInput();
    virtual HRESULT PresentNow( VOID** pCookie = nullptr );
    void    RequestShutdown() { m_bQuitRequested = TRUE; }

    static  FrameStatistics         ms_frameStats;
    static  AveragedFrameStatistics ms_avgFrameStats;
    static const FLOAT              cms_avgFrameStatsInterval;

    FLOAT           m_invTicksPerSecond;

#ifdef XSF_USE_DX_12_0
    UINT64          SignalAndWait();
    void            WaitForPrevFrame(bool waitAllFrames = false, bool resetAllocatorsAndCommandLists = false);

    // Trim upload heaps when they're no longer in use
    struct FencedHeap
    {
        XSF::CpuGpuHeap* m_pUploadHeap;
        UINT64 m_fenceValue;

        FencedHeap(_In_ XSF::CpuGpuHeap *pUploadHeap, UINT64 fenceValue) :
            m_pUploadHeap(pUploadHeap),
            m_fenceValue(fenceValue)
        {
        }

        FencedHeap()
        {
            FencedHeap(nullptr, 0);
        }
    };

    std::list<FencedHeap>   m_managedUploadHeaps;

    class is_heap_terminated : public std::unary_function<FencedHeap, bool>
    {
    public:
        bool operator( ) (FencedHeap& fencedHeap)
        {
            return (fencedHeap.m_pUploadHeap == nullptr) || fencedHeap.m_pUploadHeap->IsTerminated();
        }
    };
#endif

public:
#ifdef XSF_USE_DX_12_0
    void TrimUploadHeaps(bool removeTerminatedHeaps = true);
    void ManageUploadHeap(_In_ XSF::CpuGpuHeap* pUploadHeap);
#endif

private:
    typedef std::map<std::wstring, float> TParametersMap;
    typedef std::pair<std::wstring, float> TParametersPair;
    TParametersMap  m_commandLineParams;
    BOOL            m_printTimings;
    BOOL            m_thisIsTestRun;
    BOOL            m_thisIsPerfRun;
    BOOL            m_thisIsScreenshotRun;
    BOOL            m_thisIsStressRun;

    // this is for fixed/variable time step
    FLOAT           m_targetElapsedTime;
    FLOAT           m_totalUpdateTime;
    FLOAT           m_lastUpdateTime;
    BOOL            m_fixedTimeStep;

    // input structures
    XSF::Input      m_input;
    BYTE            m_keyboardStatus[ 256 ];
    BYTE            m_keyboardPreviousStatus[ 256 ];

    // device and immediate context
    XSF::D3DDevice*         m_pd3dDevice;
#ifdef XSF_USE_DX_12_0
    _Field_range_(0, c_MaxFrameLatency - 1) UINT m_iFrame;
    XSF::D3DCommandAllocator*  m_pd3dCommandAllocator[c_MaxFrameLatency];
    XSF::D3DCommandQueue*      m_pd3dCommandQueue;
    XSF::D3DCommandList*       m_pd3dCommandList;
    bool                       m_commandListClosed;
    XSF::D3DCommandAllocator*  m_pd3dClearCommandAllocator[c_MaxFrameLatency];
    XSF::D3DCommandList*       m_pd3dClearCommandList;
    bool                       m_clearCommandListClosed;

    // sync fences for command allocator
    XSF::D3DFencePtr           m_spFence;
    XSF::D3DFencePtr           m_spSignalFence;
    UINT                       m_currentFence;
    UINT                       m_currentSignalFence;
    UINT64                     m_allocatorFence[c_MaxFrameLatency];
    HANDLE                     m_fenceEvent;
#else
    XSF::D3DDeviceContext*  m_pd3dContext;
#endif

    SwapChain               m_backbufferSwapChain;
#ifdef _XBOX_ONE
    const SwapChain*        m_pBkPlane;
    const SwapChain*        m_pFgPlane;
    DWORD                   m_BackgroundFilterFlags;
    DWORD                   m_ForegroundFilterFlags;
#endif

    UINT                    m_presentationSyncInterval;
    UINT                    m_presentImmediateThreshold;
    UINT                    m_presentationFlags;

    // timing
    LARGE_INTEGER       m_ticksPerSecond;
    LARGE_INTEGER       m_lastTick;
    BOOL                m_justResumed;

    // Settings set by the sample
    Settings            m_sampleSettings;

    BOOL                m_bQuitRequested;


#ifdef _XBOX_ONE
    friend ref class ViewProvider;
    friend int WINAPIV main( Platform::Array< Platform::String^ >^ );
    static int XboxOneEntryPoint( SampleFramework* pSample );
#else   // PC
    friend int WINAPI wWinMain( HINSTANCE, HINSTANCE, PWSTR, int );
    static int PCEntryPoint( SampleFramework* pSample, HINSTANCE hInstance, int nCmdShow );
    static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
    static void SetActivationStatus( BOOL bActive );
    static void ReportLiveObjects();
#endif
    friend void XSF::ReportFailureAndStop( const CHAR* pFmt, LPCSTR strFileName, DWORD dwLineNumber, ... );

    // common functions shared between platforms
    void ObtainSampleSettings();
    void FrameworkInitialize();
    void FrameworkShutdown();
    BOOL FrameworkUpdateAndRender();
    void FrameworkSuspend();
    void FrameworkResume();

private:
    void CreateSwapChain( void* pWindow );
    void CreateWindowSizeDependentResources( _In_opt_ void* pWindow );
    void CreateDeviceResources();
    void DiscardDeviceResources();

    BOOL RunOneUpdate( FLOAT timeTotal, FLOAT timeDelta );
    void RunOneRender();    
};

// PIX event colors
#ifdef XSF_USE_PIX_EVENTS
const DWORD XSF_COLOR_FRAME = PIX_COLOR_INDEX(1);
const DWORD XSF_COLOR_UPDATE = PIX_COLOR_INDEX(2);
const DWORD XSF_COLOR_RENDER = PIX_COLOR_INDEX(3);
const DWORD XSF_COLOR_SET_AND_CLEAR = PIX_COLOR_INDEX(4);
const DWORD XSF_COLOR_PRESENT = PIX_COLOR_INDEX(5);
const DWORD XSF_COLOR_MESH_RENDER = PIX_COLOR_INDEX(6);
const DWORD XTF_COLOR_DRAW_TEXT = PIX_COLOR_INDEX(7);
#endif

// inline functions

//--------------------------------------------------------------------------------------
// Name: GetDevice
// Desc: Gets D3D device
//--------------------------------------------------------------------------------------
inline 
XSF::D3DDevice* SampleFramework::GetDevice() const
{
    return m_pd3dDevice;
}

#ifdef XSF_USE_DX_12_0
#else
//--------------------------------------------------------------------------------------
// Name: GetImmediateContext
// Desc: Gets immediate D3D context
//--------------------------------------------------------------------------------------
inline
XSF::D3DDeviceContext* SampleFramework::GetImmediateContext() const
{
    return m_pd3dContext;
}
#endif


#ifdef XSF_USE_DX_12_0
//--------------------------------------------------------------------------------------
// Name: GetBuffer
// Desc: Get the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D12Resource* SampleFramework::SwapChain::GetBuffer( UINT bufferIndex ) const
{
    return (bufferIndex < m_createDesc.m_NumBuffers)? m_spBuffers[bufferIndex].Get() : m_spBuffers[m_backBuffer].Get();
}


//--------------------------------------------------------------------------------------
// Name: GetRTV
// Desc: Get render target descriptor of the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
D3D12_CPU_DESCRIPTOR_HANDLE SampleFramework::SwapChain::GetRTV( UINT rtIndex ) const
{
    UINT index = (rtIndex < m_createDesc.m_NumBuffers)? rtIndex : m_backBuffer;

    return m_RTHeap.hCPU(index);
}


//--------------------------------------------------------------------------------------
// Name: GetColorSRV
// Desc: Get SRV descriptor of the backbuffer texture
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
D3D12_CPU_DESCRIPTOR_HANDLE SampleFramework::SwapChain::GetColorSRV( UINT srvIndex ) const
{
    UINT index = (srvIndex < m_createDesc.m_NumBuffers)? srvIndex : m_backBuffer;
    return m_SRVHeap.hCPU(index);
}


//--------------------------------------------------------------------------------------
// Name: GetDepthStencil
// Desc: Get depth stencil buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
ID3D12Resource* SampleFramework::SwapChain::GetDepthStencil() const
{
    return m_spDepthStencil.Get();
}

//--------------------------------------------------------------------------------------
// Name: GetDSD
// Desc: Get depth stencil descriptor of the depth buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
D3D12_CPU_DESCRIPTOR_HANDLE SampleFramework::SwapChain::GetDSV() const
{
    return m_DSHeap.hCPU(0);
}

//--------------------------------------------------------------------------------------
// Name: GetDepthSRV
// Desc: Get shader resource view of the depth buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
D3D12_CPU_DESCRIPTOR_HANDLE SampleFramework::SwapChain::GetDepthSRV() const
{
    return m_SRVHeap.hCPU(DXGI_MAX_SWAP_CHAIN_BUFFERS);
}

//--------------------------------------------------------------------------------------
// Name: GetStencilSRV
// Desc: Get shader resource view of the stencil buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
D3D12_CPU_DESCRIPTOR_HANDLE SampleFramework::SwapChain::GetStencilSRV() const
{
    return m_SRVHeap.hCPU(DXGI_MAX_SWAP_CHAIN_BUFFERS + 1);
}

//--------------------------------------------------------------------------------------
// Name: GetViewport
// Desc: Get the viewport
//--------------------------------------------------------------------------------------
inline
const D3D12_VIEWPORT& SampleFramework::SwapChain::GetViewport() const
{
    return m_Viewport;
}

//--------------------------------------------------------------------------------------
// Name: GetScissorRect
// Desc: Get the scissor rect
//--------------------------------------------------------------------------------------
inline
const D3D12_RECT& SampleFramework::SwapChain::GetScissorRect() const
{
    return m_ScissorRect;
}
#else
//--------------------------------------------------------------------------------------
// Name: GetBuffer
// Desc: Get the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D11Texture2D* SampleFramework::SwapChain::GetBuffer( UINT bufferIndex ) const
{
    return (bufferIndex < m_createDesc.m_NumBuffers)? m_spBuffers[bufferIndex].Get() : m_spBuffers[m_backBuffer].Get();
}


//--------------------------------------------------------------------------------------
// Name: GetRTV
// Desc: Get render target view of the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D11RenderTargetView* SampleFramework::SwapChain::GetRTV( UINT rtIndex ) const
{
    return (rtIndex < m_createDesc.m_NumBuffers)? m_spRTVs[rtIndex].Get() : m_spRTVs[m_backBuffer].Get();
}

//--------------------------------------------------------------------------------------
// Name: GetRTVpp
// Desc: Get render target view of the backbuffer, ready to be passed to OMSetRenderTargets
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D11RenderTargetView* const * SampleFramework::SwapChain::GetRTVpp( UINT rtIndex ) const
{
    return (rtIndex < m_createDesc.m_NumBuffers)? m_spRTVs[rtIndex].GetAddressOf() : m_spRTVs[m_backBuffer].GetAddressOf();
}

//--------------------------------------------------------------------------------------
// Name: GetColorSRV
// Desc: Get shader resource view of the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D11ShaderResourceView* SampleFramework::SwapChain::GetColorSRV( UINT srvIndex ) const
{
    return (srvIndex < m_createDesc.m_NumBuffers)? m_spColorSRVs[srvIndex].Get() : m_spColorSRVs[m_backBuffer].Get();
}

//--------------------------------------------------------------------------------------
// Name: GetColorUAV
// Desc: Get unordered access view of the backbuffer
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
inline
ID3D11UnorderedAccessView* SampleFramework::SwapChain::GetColorUAV( UINT uavIndex ) const
{
    return (uavIndex < m_createDesc.m_NumBuffers)? m_spColorUAVs[uavIndex].Get() : m_spColorUAVs[m_backBuffer].Get();
}

//--------------------------------------------------------------------------------------
// Name: GetDepthStencil
// Desc: Get depth stencil buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
ID3D11Texture2D* SampleFramework::SwapChain::GetDepthStencil() const
{
    return m_spDepthStencil.Get();
}

//--------------------------------------------------------------------------------------
// Name: GetDSV
// Desc: Get depth stencil view of the depth buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
ID3D11DepthStencilView* SampleFramework::SwapChain::GetDSV() const
{
    return m_spDSV.Get();
}

//--------------------------------------------------------------------------------------
// Name: GetDepthSRV
// Desc: Get shader resource view of the depth buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
ID3D11ShaderResourceView* SampleFramework::SwapChain::GetDepthSRV() const
{
    return m_spDepthSRV.Get();
}


//--------------------------------------------------------------------------------------
// Name: GetStencilSRV
// Desc: Get shader resource view of the stencil buffer associated with the backbuffer
//--------------------------------------------------------------------------------------
inline
ID3D11ShaderResourceView* SampleFramework::SwapChain::GetStencilSRV() const
{
    return m_spStencilSRV.Get();
}

//--------------------------------------------------------------------------------------
// Name: GetViewport
// Desc: Get viewport that is of the size of the backbuffer
//--------------------------------------------------------------------------------------
inline
const D3D11_VIEWPORT& SampleFramework::SwapChain::GetViewport() const
{
    return m_Viewport;
}
#endif // XSF_USE_DX_12_0

//--------------------------------------------------------------------------------------
// Name: GetSwapChain
// Desc: Get d3d swap chain
//--------------------------------------------------------------------------------------
inline 
XSF::DXGISwapChain* SampleFramework::SwapChain::GetSwapChain() const
{
    return m_spSwapChain.Get();
}


//--------------------------------------------------------------------------------------
// Name: GetEffectiveArea
// Desc: 
//--------------------------------------------------------------------------------------
inline
const UINT* SampleFramework::SwapChain::GetEffectiveArea() const
{
    return m_useThisArea;
}


//--------------------------------------------------------------------------------------
// Name: GetGamepadReading
// Desc: Get reading from the current gamepad
//--------------------------------------------------------------------------------------
inline
const XSF::GamepadReading& SampleFramework::GetGamepadReading() const
{
    return m_input.GetCurrentGamepadReading();
}


#ifdef _XBOX_ONE
//--------------------------------------------------------------------------------------
// Name: GetCurrentGamepad
// Desc: Get the current gamepad
//--------------------------------------------------------------------------------------
inline
 Windows::Xbox::Input::IGamepad^ SampleFramework::GetGamepad() const
{
    return m_input.GetGamepad();
}
#endif


//--------------------------------------------------------------------------------------
// Name: GetKeyState
// Desc: Get current state of a given key. Note that it may not work on Xbox One
//--------------------------------------------------------------------------------------
inline
UINT    SampleFramework::GetKeyState( BYTE bVk ) const
{
    return (m_keyboardStatus[ bVk ] >> 7);
}

//--------------------------------------------------------------------------------------
// Name: IsKeyPressed
// Desc: Get current state of a given key.
//--------------------------------------------------------------------------------------
inline
BOOL    SampleFramework::IsKeyPressed( BYTE bVk ) const
{
    if( (UINT)( m_keyboardPreviousStatus[ bVk ] >> 7) == GetKeyState( bVk ) )
        return FALSE;

    return GetKeyState( bVk );
}


//--------------------------------------------------------------------------------------
// Name: SetKeyDown
// Desc: Notification of key down
//--------------------------------------------------------------------------------------
inline
VOID    SampleFramework::SetKeyDown( BYTE bVk ) 
{
    m_keyboardStatus[ bVk ] |= ( 1 << 7 );
}


//--------------------------------------------------------------------------------------
// Name: SetKeyUp
// Desc: Notification of key up
//--------------------------------------------------------------------------------------
inline
VOID    SampleFramework::SetKeyUp( BYTE bVk ) 
{
    m_keyboardStatus[ bVk ] &= ~( 1 << 7 );
}


//--------------------------------------------------------------------------------------
// Name: GetStatistics
// Desc: Gets some timings, not thread safe
//--------------------------------------------------------------------------------------
inline
const SampleFramework::FrameStatistics& SampleFramework::GetStatistics()
{
    return ms_frameStats;
}

//--------------------------------------------------------------------------------------
// Name: GetStatistics
// Desc: Gets some timings, not thread safe
//--------------------------------------------------------------------------------------
inline
const SampleFramework::AveragedFrameStatistics& SampleFramework::GetAvgStatistics()
{
    return ms_avgFrameStats;
}

//--------------------------------------------------------------------------------------
// Name: GetLastGpuFrameTime
// Desc: Get how long the last frame took on the GPU
//--------------------------------------------------------------------------------------
inline
FLOAT    SampleFramework::GetLastGpuFrameTime() const
{
    return GetStatistics().m_lastGpuMeasuredFrameTime;
}

//--------------------------------------------------------------------------------------
// Name: GetLastGpuFrameTime
// Desc: Get how long the last frame took on the GPU
//--------------------------------------------------------------------------------------
inline
FLOAT    SampleFramework::GetAvgGpuFrameTime() const
{
    return GetAvgStatistics().m_avgGpuMeasuredFrameTime;
}


//--------------------------------------------------------------------------------------
// Name: GetLastCpuFrameTime
// Desc: Get how long the last frame took on the CPU
//--------------------------------------------------------------------------------------
inline
FLOAT    SampleFramework::GetAvgCpuFrameTime() const
{
    return GetAvgStatistics().m_avgCpuMeasuredFrameTime;
}

//--------------------------------------------------------------------------------------
// Name: GetLastCpuFrameTime
// Desc: Get how long the last frame took on the CPU
//--------------------------------------------------------------------------------------
inline
FLOAT    SampleFramework::GetLastCpuFrameTime() const
{
    return GetStatistics().m_lastCpuMeasuredFrameTime;
}


//--------------------------------------------------------------------------------------
// Name: GetCommandLineParameter
// Desc: Returns true if the command line parameter exists and fill the value if requested
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
template<typename T>
BOOL     SampleFramework::GetCommandLineParameter( const wchar_t* const name, T *pValue ) const
{
    float pfValue = 0.0f;
    BOOL parameterExists = GetCommandLineParameter( name, &pfValue );

    if( parameterExists )
    {
        *pValue = static_cast<T>(pfValue);
    }

    return parameterExists;
}


//--------------------------------------------------------------------------------------
// Platform specific entry point
//--------------------------------------------------------------------------------------

// XBOX_ONE
#ifdef _XBOX_ONE

#define XSF_DECLARE_TEST_ENTRY_POINT( className, pLoggingFunc, pErrorFunc )                     \
                                                                                                \
[Platform::MTAThread]                                                                           \
int WINAPIV main( Platform::Array< Platform::String^ >^ /*params*/ )                            \
{                                                                                               \
    XSF::InstallExceptionHandler();                                                             \
    XSF::SetTestOptions( pLoggingFunc, pErrorFunc, TRUE );                                      \
    XSF::CrossCheckpoint( XSF::CP_MAIN );                                                       \
    std::shared_ptr<SampleFramework> sample( new className );                                   \
    return SampleFramework::XboxOneEntryPoint( sample.get() );                                  \
}

#define XSF_DECLARE_ENTRY_POINT( className )                                                    \
            XSF_DECLARE_TEST_ENTRY_POINT( className, NULL, NULL )

#else   // PC

#define XSF_DECLARE_ENTRY_POINT( className )                                                                \
                                                                                                            \
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*lpCmdLine*/, int nCmdShow )  \
{                                                                                                           \
    XSF::CrossCheckpoint( XSF::CP_MAIN );                                                                   \
    std::shared_ptr<SampleFramework> sample( new className );                                               \
    int retCode = SampleFramework::PCEntryPoint( sample.get(), hInstance, nCmdShow );                       \
    sample = nullptr;                                                                                       \
    SampleFramework::ReportLiveObjects();                                                                   \
    return retCode;                                                                                         \
}

#endif

#endif  // XSF_H_INCLUDED
