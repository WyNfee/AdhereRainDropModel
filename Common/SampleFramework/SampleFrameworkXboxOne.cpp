//--------------------------------------------------------------------------------------
// SampleFrameworkXboxOne.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "pch.h"
#include "ViewProvider.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;



//--------------------------------------------------------------------------------------
// Name: XboxOneEntryPoint
// Desc: Main framework entry point on Xbox One platform
//--------------------------------------------------------------------------------------
int SampleFramework::XboxOneEntryPoint( SampleFramework* pSample )
{
    auto viewProviderFactory = ref new ViewProviderFactory( reinterpret_cast< UINT_PTR >( pSample ) );
    Windows::ApplicationModel::Core::CoreApplication::Run( viewProviderFactory );
    return 0;
}


//--------------------------------------------------------------------------------------
// Name: PlatformCreate()
// Desc: Platform dependent create swap chain
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::SwapChain::PlatformCreate()
{
    const SwapChainCreateDesc* pDesc = &m_createDesc;

    // First, retrieve the underlying DXGI Device from the D3D Device
    XSF::D3DTypePtr<IDXGIDevice1> spdxgiDevice;
    XSF_RETURN_IF_FAILED( m_pd3dDevice->QueryInterface( IID_IDXGIDevice1, reinterpret_cast<void**>(spdxgiDevice.GetAddressOf()) ) );

    XSF::D3DTypePtr<IDXGIAdapter> spdxgiAdapter;
    XSF_RETURN_IF_FAILED( spdxgiDevice->GetAdapter( spdxgiAdapter.GetAddressOf() ) );

    XSF::D3DTypePtr<IDXGIFactory2> spdxgiFactory;
    XSF_RETURN_IF_FAILED( spdxgiAdapter->GetParent( IID_IDXGIFactory2, reinterpret_cast<void**>(spdxgiFactory.GetAddressOf()) ) );

    if( pDesc->m_extraSwapChain )
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferDesc.Width = pDesc->m_Width;
        swapChainDesc.BufferDesc.Height = pDesc->m_Height;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.Format = pDesc->m_Format;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
        swapChainDesc.SampleDesc.Count = 1;                         // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = pDesc->m_Usage;
        swapChainDesc.BufferCount = pDesc->m_NumBuffers;            // use two buffers to enable flip effect
        swapChainDesc.SwapEffect = pDesc->m_flipSequential ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = pDesc->m_Flags;

        XSF_RETURN_IF_FAILED( spdxgiFactory->CreateSwapChain( m_pd3dDevice, &swapChainDesc, reinterpret_cast<IDXGISwapChain**>(m_spSwapChain.GetAddressOf()) ) );
    } else
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width = pDesc->m_Width;
        swapChainDesc.Height = pDesc->m_Height;
        swapChainDesc.Format = pDesc->m_Format;
        swapChainDesc.Stereo = false; 
        swapChainDesc.SampleDesc.Count = 1;                         // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = pDesc->m_Usage;
        swapChainDesc.BufferCount = pDesc->m_NumBuffers;            // use two buffers to enable flip effect
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = pDesc->m_Flags;

        XSF_RETURN_IF_FAILED( spdxgiFactory->CreateSwapChainForCoreWindow( m_pd3dDevice, reinterpret_cast<IUnknown*>( pDesc->m_pWindow ), &swapChainDesc, nullptr, reinterpret_cast<IDXGISwapChain1**>(m_spSwapChain.GetAddressOf()) ) );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: PlatformDestroy
// Desc: Destroy the swap chain
//--------------------------------------------------------------------------------------
void SampleFramework::SwapChain::PlatformDestroy()
{
    m_spSwapChain.Reset();
    XSF_SAFE_RELEASE( m_pd3dDevice );
}


//--------------------------------------------------------------------------------------
// Name: PresentNow
// Desc: If ever you need to present mid-frame...
//--------------------------------------------------------------------------------------
HRESULT SampleFramework::PresentNow( VOID** pCookie )
{
#if defined(XSF_USE_DX_12_0)
    XSFScopedNamedEventFunc( m_pd3dCommandQueue, XSF_COLOR_PRESENT );
#else
    XSFScopedNamedEventFunc( m_pd3dContext, XSF_COLOR_PRESENT );
#endif
    
    HRESULT hr = S_OK;

#if defined(_XBOX_ONE) && defined(_TITLE)

    static const FLOAT COMPOSITE_SX = 1920.f;
    static const FLOAT COMPOSITE_SY = 1080.f;
    IDXGISwapChain1* ppSwapChains[ 2 ] = { 0 };
    DXGIX_PRESENTARRAY_PARAMETERS presentParameterSets[ 2 ] = { 0 };
    UINT numPlanesToPresent = 0;
    
    if( m_pBkPlane && m_pFgPlane )
    {
        // !!! NOTICE:  Swap chains are presented from front to back, not in draw order. !!!
        // This is apparently for consistency with Windows.

        ppSwapChains[ 0 ] = m_pFgPlane->GetSwapChain();
        ppSwapChains[ 1 ] = m_pBkPlane->GetSwapChain();

        const UINT* fgArea = m_pFgPlane->GetEffectiveArea();
        const UINT* bkArea = m_pBkPlane->GetEffectiveArea();

        presentParameterSets[ 0 ].SourceRect.left   = fgArea[ 0 ];
        presentParameterSets[ 0 ].SourceRect.top    = fgArea[ 1 ];
        presentParameterSets[ 0 ].SourceRect.right  = fgArea[ 2 ];
        presentParameterSets[ 0 ].SourceRect.bottom = fgArea[ 3 ];
        presentParameterSets[ 0 ].ScaleFactorHorz   = COMPOSITE_SX / static_cast< FLOAT >( fgArea[ 2 ] );
        presentParameterSets[ 0 ].ScaleFactorVert   = COMPOSITE_SY / static_cast< FLOAT >( fgArea[ 3 ] );
        presentParameterSets[ 0 ].Flags = m_ForegroundFilterFlags;

        presentParameterSets[ 0 ].Cookie            = pCookie ? pCookie[ 0 ] : nullptr;

        presentParameterSets[ 1 ].SourceRect.left   = bkArea[ 0 ];
        presentParameterSets[ 1 ].SourceRect.top    = bkArea[ 1 ];
        presentParameterSets[ 1 ].SourceRect.right  = bkArea[ 2 ];
        presentParameterSets[ 1 ].SourceRect.bottom = bkArea[ 3 ];
        presentParameterSets[ 1 ].ScaleFactorHorz   = COMPOSITE_SX / static_cast< FLOAT >( bkArea[ 2 ] );
        presentParameterSets[ 1 ].ScaleFactorVert   = COMPOSITE_SY / static_cast< FLOAT >( bkArea[ 3 ] );
        presentParameterSets[ 1 ].Flags = m_BackgroundFilterFlags;

        presentParameterSets[ 1 ].Cookie            = pCookie ? pCookie[ 1 ] : nullptr;

        numPlanesToPresent = 2;
    } else if( m_pBkPlane || m_pFgPlane )
    {
        const SwapChain* p = m_pBkPlane ? m_pBkPlane : m_pFgPlane;

        ppSwapChains[ 0 ] = p->GetSwapChain();

        const UINT* area = p->GetEffectiveArea();

        presentParameterSets[ 0 ].SourceRect.left   = area[ 0 ];
        presentParameterSets[ 0 ].SourceRect.top    = area[ 1 ];
        presentParameterSets[ 0 ].SourceRect.right  = area[ 2 ];
        presentParameterSets[ 0 ].SourceRect.bottom = area[ 3 ];
        presentParameterSets[ 0 ].ScaleFactorHorz   = COMPOSITE_SX / static_cast< FLOAT >( area[ 2 ] );
        presentParameterSets[ 0 ].ScaleFactorVert   = COMPOSITE_SY / static_cast< FLOAT >( area[ 3 ] );
        presentParameterSets[ 0 ].Flags = m_pBkPlane ? m_BackgroundFilterFlags : m_ForegroundFilterFlags;

        presentParameterSets[ 0 ].Cookie            = pCookie ? pCookie[ 0 ] : nullptr;

        presentParameterSets[ 1 ].Disable = TRUE;
        presentParameterSets[ 1 ].UsePreviousBuffer = FALSE;

        numPlanesToPresent = 1;
    }
     
    if( numPlanesToPresent  )
    {
        hr = DXGIXPresentArray( m_presentationSyncInterval, m_presentImmediateThreshold, m_presentationFlags, numPlanesToPresent, ppSwapChains, presentParameterSets );
    }
#else
    UNREFERENCED_PARAMETER(pCookie);    // avoid warning for unreferenced parameter
    hr = m_backbufferSwapChain.GetSwapChain()->Present( m_presentationSyncInterval, m_presentationFlags );
#endif

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
#if defined(_XBOX_ONE) && defined(_TITLE)
        if( m_pFgPlane != nullptr && m_pFgPlane != &m_backbufferSwapChain )
        {
            const_cast<SwapChain*>(m_pFgPlane)->RotateSwapChainBuffer();
        }
        if( m_pBkPlane != nullptr && m_pBkPlane != &m_backbufferSwapChain && m_pBkPlane != m_pFgPlane )
        {
            const_cast<SwapChain*>(m_pBkPlane)->RotateSwapChainBuffer();
        }
#endif

    }

    return hr;
}


#if defined(_XBOX_ONE) && defined(_TITLE)
//--------------------------------------------------------------------------------------
// Name: SetPresentSurfaces
// Desc: Sets surfaces to present
//--------------------------------------------------------------------------------------
void SampleFramework::SetPresentSurfaces( const SwapChain* pBackground, const SwapChain* pForeground )
{
    m_pBkPlane = pBackground;
    m_pFgPlane = pForeground;
}

//--------------------------------------------------------------------------------------
// Name: SetPresentSurfaces
// Desc: Sets surfaces to present
//--------------------------------------------------------------------------------------
void SampleFramework::SetPresentSurfaceFilterFlags( DWORD BackgroundFlags, DWORD ForegroundFlags )
{
    m_BackgroundFilterFlags = BackgroundFlags;
    m_ForegroundFilterFlags = ForegroundFlags;
}
#endif

