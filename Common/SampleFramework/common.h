//--------------------------------------------------------------------------------------
// Common.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once
#ifndef XSF_COMMON_H
#define XSF_COMMON_H

#ifndef XSF_H_INCLUDED
#error  please include SampleFramework.h instead of this file
#endif


// Whether or not to compile out PIX events
#if defined( _XBOX_ONE ) && defined( _TITLE )
#if defined( PROFILE ) || defined( _DEBUG )
#define XSF_USE_PIX_EVENTS
#endif
#endif

// safe release for com pointers
#define XSF_SAFE_ADDREF( o ) if( o ) { (o)->AddRef(); }
#define XSF_SAFE_RELEASE( o ) if( o ) { (o)->Release(); o = nullptr; }

// only set the pointer to NULL if it has 0 refs left
#define XSF_SAFE_RELEASE_C( o ) if( o ) { if( (o)->Release() == 0 ) (o) = nullptr; }

// safe delete
#define XSF_SAFE_DELETE_ARRAY( a )  { delete[] a; a = nullptr; }
#define XSF_SAFE_DELETE( a )        { delete a; a = nullptr; }
#define XSF_SAFE_ALIGNED_FREE( a )  { _aligned_free( a ); a = nullptr; }

// This is a platform defined constant
#define XSF_CACHE_LINE_SIZE 0x40 // 64-byte cache line size

#if defined( _XBOX_ONE ) && defined( _TITLE )
#define XSF_TEXTURE_DATA_PITCH_ALIGNMENT D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT
#else
#define XSF_TEXTURE_DATA_PITCH_ALIGNMENT D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
#endif

#if defined( _XBOX_ONE ) && defined( _TITLE )
// The support of mapping USAGE_DEFAULT resources on Deferred Contexts is added in Apr QFE6
#define MAP_USAGE_DEFAULT_PLACEMENT
#ifdef MAP_USAGE_DEFAULT_PLACEMENT
#define MAP_PLACEMENT_BUFFERS_CACHE_LINE_ALIGNMENT_PACK // force placement buffers to be cache line aligned and packed
#define BUFFER_CACHE_LINE_SIZE XSF_CACHE_LINE_SIZE
#else
// Using default resources with round robin buffers for dynamic resources results in worse performance
// than dynamic buffers in current implementation (See Deferred Contexts sample's readme). 
// Use the default resources with round robin placement buffers instead for an optimal use
//#define MAP_USAGE_DEFAULT
#endif
#endif

#ifdef XSF_USE_PIX_EVENTS
#ifdef _XBOX_ONE
#include <pix.h>
#pragma comment(lib, "pixEvt")
#else
// PC doesn't support CPU timing
#define PIXBeginEvent( ctx, color, text, ... )
#define PIXEndEvent( ctx )
#define PIXSetMarker( ctx, color, text, ... )
#endif

// Pass the d3d device context as a first parameter, and NULL if there is no relevant context (say, it's a CPU function)
// The "No-op" versions of those are to enforce calling convention in release
// Begin and EndNamedEvents should always come in pairs. SetMarker needs a single call.
// The xxxF versions allow to pass printf style formatted string to the macro. The non-F versions will pass the string
// directly to the device. This is to ensure as little overhead as possible and to enable outputting strings like 0%
// These macros all pre-append L to strings so that calling code can exclude, e.g. XSFBeginNamedEvent( ctx, color, "stuff")
// Note: "ctx" can also be a CmdList or CmdQueue for D3D12.
#define XSFBeginNamedEventF( ctx, color, text, ... )   do{ if( ctx ) { PIXBeginEvent(ctx, color, text, __VA_ARGS__ ); } else { PIXBeginEvent(color, text, __VA_ARGS__ ); } } while( FALSE )
#define XSFBeginNamedEvent( ctx, color, text )         do{ if( ctx ) { PIXBeginEvent(ctx, color, text); } else { PIXBeginEvent(color, text); } } while( FALSE ) 
#define XSFEndNamedEvent( ctx )                        do{ if( ctx ) { PIXEndEvent(ctx); } else { PIXEndEvent(); } } while( FALSE )

#if defined(ATG_PROFILE) || defined(ATG_PROFILE_VERBOSE)

// Scoped versions. XSFScopedNamedEvent will open an event in constructor and close it in destructor, so it will wrap a C++ scope
// XSFScopedNamedEventFunc is commonly used to wrap the entire function body in Begin/End named event
#define XSFScopedNamedEvent( ctx, color, text, ... )   ATGPROFILELABEL( text ); ::XboxSampleFramework::XsfScopedNamedEvent   XSF_PASTE( pixEvent, __LINE__ ) ( ctx, color, text, __VA_ARGS__ );
#define XSFScopedNamedEventFunc( ctx, color )          ATGPROFILETHIS; ::XboxSampleFramework::XsfScopedNamedEvent   XSF_PASTE( pixEvent, __LINE__ ) ( ctx, color, XSF_PASTE( L, __FUNCTION__ ) );

#else

// Scoped versions. XSFScopedNamedEvent will open an event in constructor and close it in destructor, so it will wrap a C++ scope
// XSFScopedNamedEventFunc is commonly used to wrap the entire function body in Begin/End named event
#define XSFScopedNamedEvent( ctx, color, text, ... )   ::XboxSampleFramework::XsfScopedNamedEvent   XSF_PASTE( pixEvent, __LINE__ ) ( ctx, color, text, __VA_ARGS__ );
#define XSFScopedNamedEventFunc( ctx, color )          ::XboxSampleFramework::XsfScopedNamedEvent   XSF_PASTE( pixEvent, __LINE__ ) ( ctx, color,  XSF_PASTE( L, __FUNCTION__ ) );

#endif

#define XSFSetMarkerF( ctx, color, text, ... )         do{ if( ctx ) { PIXSetMarker(ctx, color, text, __VA_ARGS__ ); } else { PIXSetMarker(color, text, __VA_ARGS__ ); } } while( FALSE )
#define XSFSetMarker( ctx, color, text )               do{ if( ctx ) { PIXSetMarker(ctx, color, text); } else { PIXSetMarker(color, text); } } while( FALSE )

#else

#define XSFBeginNamedEventF( ctx, color, text, ... )   
#define XSFBeginNamedEvent( ctx, color, text )         
#define XSFEndNamedEvent( ctx )                        

#if defined(ATG_PROFILE) || defined(ATG_PROFILE_VERBOSE)
#define XSFScopedNamedEvent( ctx, color, text, ... )   ATGPROFILELABEL(text);
#define XSFScopedNamedEventFunc( ctx, color )          ATGPROFILETHIS;
#else
#define XSFSetMarkerF( ctx, color, text, ... )         
#define XSFSetMarker( ctx, color, text )               
#define XSFScopedNamedEvent( ctx, color, text, ... )   
#define XSFScopedNamedEventFunc( ctx, color )          
#endif

#endif

// IUnknown methods implementation macro
#define XSF_IMPLEMENT_IUNKNOWN( I , REFCOUNT)       \
    STDMETHODIMP_(ULONG) AddRef()                   \
    {                                               \
        return InterlockedIncrement(&REFCOUNT);     \
    }                                               \
    STDMETHODIMP_(ULONG) Release()                  \
    {                                               \
        LONG cRef = InterlockedDecrement(&REFCOUNT);\
        if (cRef == 0)                              \
        {                                           \
            delete this;                            \
        }                                           \
        return cRef;                                \
    }                                               \
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)       \
    {                                                           \
        if(__uuidof(I) == riid)                                 \
        {                                                       \
            *ppv = static_cast<I*>(this);                       \
            AddRef();                                           \
            return S_OK;                                        \
        }                                                       \
        else                                                    \
        {                                                       \
            *ppv = nullptr;                                     \
            return E_NOINTERFACE;                               \
        }                                                       \
    }                                                           \


// XSF_ASSERT
#ifdef NDEBUG

#define XSF_ASSERT( exp )   _Analysis_assume_( exp ); ((void)(exp));

#else   // NDEBUG

#define XSF_ASSERT( exp )   _Analysis_assume_( exp ); if( !(exp) ) { XboxSampleFramework::ReportFailureAndStop( "assertion failed: %s\n", __FILE__, __LINE__, #exp ); }

#endif  // NDEBUG

#define XSF_ERROR_MESSAGE( str ) { XboxSampleFramework::ReportFailureAndStop( "Failure: %s", __FILE__, __LINE__, str ); }

#define XSF_ERROR_IF_FAILED( exp ) { HRESULT _hr_ = (exp); if( FAILED( _hr_ ) ) XboxSampleFramework::ReportFailureAndStop( "Failure with HRESULT of %x", __FILE__, __LINE__, ( _hr_ ) ); }

#define XSF_SAFE_RETURN_IF_NULL( val, exp ) { if( val == NULL ) { return FALSE; } else { return exp; } }

#ifdef _DEBUG

// Stopping at the point of error helps debugging
#define XSF_RETURN_IF_FAILED( exp ) { HRESULT _hr_ = (exp); if( FAILED( _hr_ ) ) { XboxSampleFramework::ReportFailureAndStop( "Failure with HRESULT of %x", __FILE__, __LINE__, ( _hr_ ) ); return _hr_; } }

#else

#define XSF_RETURN_IF_FAILED( exp ) { HRESULT _hr_ = (exp); if( FAILED( _hr_ ) ) return _hr_; }

#endif

#define XSF_END_IF_FAILED( exp, ... ) \
	if ( FAILED( hr = ( exp ) ) ) \
	{ \
        XboxSampleFramework::DebugPrint( "Error (HRESULT 0x%x) %s(%d): ", hr, __FILE__, __LINE__ - 2 ); \
	    XboxSampleFramework::DebugPrint( __VA_ARGS__ ); \
        goto end; \
	}

#define XSF_PASTE_( a, b )   a ## b
#define XSF_PASTE( a, b )    XSF_PASTE_( a, b )

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS IID_PPV_ARGS
#endif

namespace XboxSampleFramework
{
#if defined(XSF_USE_DX_12_0)
    typedef ID3D12Device            D3DDevice;
    typedef ID3D12CommandAllocator  D3DCommandAllocator;
    typedef ID3D12GraphicsCommandList D3DCommandList;
#ifdef _XBOX_ONE
    typedef ID3D12XboxDmaCommandList XboxDMACommandList;
#endif
    typedef ID3D12CommandQueue      D3DCommandQueue;
    typedef IDXGISwapChain1         DXGISwapChain;
#elif defined(_XBOX_ONE) && defined(_TITLE)
    typedef ID3D11DeviceX           D3DDevice;
    typedef ID3D11DeviceContextX    D3DDeviceContext;
    typedef ID3D11ComputeContextX   D3DComputeContext;
    typedef ID3D11RasterizerState1  D3DRasterizerState;
    typedef D3D11_RASTERIZER_DESC1  D3DRasterizerDesc;
    typedef IDXGISwapChain1         DXGISwapChain;
#elif defined( XSF_USE_DX_11_1 )
    typedef ID3D11Device1           D3DDevice;
    typedef ID3D11DeviceContext1    D3DDeviceContext;
    typedef ID3D11DeviceContext1    D3DComputeContext;
    typedef ID3D11RasterizerState1  D3DRasterizerState;
    typedef D3D11_RASTERIZER_DESC1  D3DRasterizerDesc;
    typedef IDXGISwapChain1         DXGISwapChain;
#else
    typedef ID3D11Device            D3DDevice;
    typedef ID3D11DeviceContext     D3DDeviceContext;
    typedef ID3D11DeviceContext     D3DComputeContext;
    typedef ID3D11RasterizerState   D3DRasterizerState;
    typedef D3D11_RASTERIZER_DESC   D3DRasterizerDesc;
    typedef IDXGISwapChain          DXGISwapChain;
#endif

    // Auto-releasing D3D resources
    template< typename t_Resource >
    struct D3DTypePtr : public Microsoft::WRL::ComPtr< t_Resource >
    {
        operator t_Resource* () { return Get(); }
        operator const t_Resource* () const { return Get(); }

        t_Resource** operator &() { return GetAddressOf(); }

        // This type traits infrastructure allows you to cast D3DTypePtr< t_Resource > to D3DTypePtr< t_Other > 
        // whenever you can cast t_Resource to t_Other.
        //
        // The cast operator is declared for all t_Other, but only implemented when 
        // std::is_convertible< t_Resource, t_Other >::value == true
        // 
        // Note this facility already exists for ComPtr, but we need to lift it to D3DTypePtr.
        template< typename t_Other, bool t_bAllowed > struct Typecast;

        template< typename t_Other > 
        struct Typecast< t_Other, true >
        {
            static D3DTypePtr< t_Other >& allowed_cast( D3DTypePtr< t_Resource >& from ) 
            { 
                return reinterpret_cast< D3DTypePtr< t_Other >& >( from ); 
            }
        };

        template< typename t_Other > 
        operator D3DTypePtr< t_Other >& () { return Typecast< t_Other, std::is_convertible< t_Resource, t_Other >::value >::allowed_cast( *this ); }
    };

    typedef D3DTypePtr<ID3DBlob>                        D3DBlobPtr;
#ifdef XSF_USE_DX_12_0
    typedef D3DTypePtr<ID3D12Object>                    D3DObjectPtr;
    typedef D3DTypePtr<ID3D12DeviceChild>               D3DDeviceChildPtr;
    typedef D3DTypePtr<ID3D12RootSignature>             D3DRootSignaturePtr;
    typedef D3DTypePtr<ID3D12RootSignatureDeserializer> D3DRootSignatureDeserializerPtr;
    typedef D3DTypePtr<ID3D12Pageable>                  D3DPageablePtr;
    typedef D3DTypePtr<ID3D12Heap>                      D3DHeapPtr;
    typedef D3DTypePtr<ID3D12Resource>                  D3DResourcePtr;
    typedef D3DTypePtr<ID3D12CommandAllocator>          D3DCommandAllocatorPtr;
    typedef D3DTypePtr<ID3D12Fence>                     D3DFencePtr;
    typedef D3DTypePtr<ID3D12PipelineState>             D3DPipelineStatePtr;
    typedef D3DTypePtr<ID3D12DescriptorHeap>            D3DDescriptorHeapPtr;
    typedef D3DTypePtr<ID3D12QueryHeap>                 D3DQueryHeapPtr;
    typedef D3DTypePtr<ID3D12CommandSignature>          D3DCommandSignaturePtr;
    typedef D3DTypePtr<ID3D12CommandList>               D3DCommandListPtr;
    typedef D3DTypePtr<ID3D12GraphicsCommandList>       D3DGraphicsCommandListPtr;
    typedef D3DTypePtr<ID3D12CommandQueue>              D3DCommandQueuePtr;
    typedef D3DTypePtr<ID3D12Device>                    D3DDevicePtr;
    typedef D3DTypePtr<ID3D12Debug>                     D3DDebugPtr;
#ifdef _XBOX_ONE
    typedef D3DTypePtr<ID3D12XboxDmaCommandList>        XboxDmaCommandListPtr;
#endif
#else
    typedef D3DTypePtr<ID3D11BlendState>          D3DBlendStatePtr;
    typedef D3DTypePtr<ID3D11Buffer>              D3DBufferPtr;
    typedef D3DTypePtr<ID3D11CommandList>         D3DCommandListPtr;
    typedef D3DTypePtr<D3DComputeContext>         D3DComputeContextPtr;
    typedef D3DTypePtr<ID3D11ComputeShader>       D3DComputeShaderPtr;
#if defined(_XBOX_ONE) && defined(_TITLE)
    typedef D3DTypePtr<ID3D11CounterSetX>         D3DCounterSetPtr;
    typedef D3DTypePtr<ID3D11CounterSampleX>      D3DCounterSamplePtr;
#endif
    typedef D3DTypePtr<ID3D11DepthStencilState>   D3DDepthStencilStatePtr;
    typedef D3DTypePtr<ID3D11DepthStencilView>    D3DDepthStencilViewPtr;
    typedef D3DTypePtr<D3DDeviceContext>          D3DDeviceContextPtr;
    typedef D3DTypePtr<ID3D11DomainShader>        D3DDomainShaderPtr;
    typedef D3DTypePtr<ID3D11GeometryShader>      D3DGeometryShaderPtr;
    typedef D3DTypePtr<ID3D11HullShader>          D3DHullShaderPtr;
    typedef D3DTypePtr<ID3D11InputLayout>         D3DInputLayoutPtr;
    typedef D3DTypePtr<ID3D11PixelShader>         D3DPixelShaderPtr;
    typedef D3DTypePtr<ID3D11Predicate>           D3DPredicatePtr;
    typedef D3DTypePtr<ID3D11Query>               D3DQueryPtr;
    typedef D3DTypePtr<ID3D11RenderTargetView>    D3DRenderTargetViewPtr;
    typedef D3DTypePtr<ID3D11SamplerState>        D3DSamplerStatePtr;
    typedef D3DTypePtr<ID3D11ShaderResourceView>  D3DShaderResourceViewPtr;
    typedef D3DTypePtr<ID3D11RasterizerState>     D3DRasterizerStatePtr;
    typedef D3DTypePtr<ID3D11Resource>            D3DResourcePtr;
    typedef D3DTypePtr<ID3D11Texture1D>           D3DTexture1DPtr;
    typedef D3DTypePtr<ID3D11Texture2D>           D3DTexture2DPtr;
    typedef D3DTypePtr<ID3D11Texture3D>           D3DTexture3DPtr;
    typedef D3DTypePtr<ID3D11UnorderedAccessView> D3DUnorderedAccessViewPtr;
    typedef D3DTypePtr<ID3D11VertexShader>        D3DVertexShaderPtr;
#endif

    const wchar_t* GetContentFileRoot();
    const wchar_t* GetStorageFileRoot();
    BOOL GetExecutableName( _Out_writes_(executableNameSize) wchar_t* strExecutableName, DWORD _In_range_(1,256) executableNameSize );
    void DebugPrint( _In_z_ const char* msg, ... );
    void DebugPrint( _In_z_ const wchar_t* msg, ... );
    void PrintNoVarargs( _In_z_ const wchar_t* msg );
    inline
    void  DebugPrint() {}
    void ReportFailureAndStop( _In_z_ const char* pFormat, _In_z_ LPCSTR strFileName, DWORD dwLineNumber, ... );
    _Check_return_
    HRESULT LoadBlob( _In_z_ const wchar_t* pFilename, std::vector< BYTE >& data );

#ifdef XSF_USE_DX_12_0
    _Check_return_
    HRESULT LoadShader( _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3DBlob** ppShader );

    HRESULT CreateColorTextureAndViews(_In_ D3DDevice* pDevice, UINT width, UINT height, DXGI_FORMAT fmt,
        _COM_Outptr_ ID3D12Resource** ppTexture, D3D12_CPU_DESCRIPTOR_HANDLE hRTV, D3D12_CPU_DESCRIPTOR_HANDLE hSRV,
        _In_opt_ D3D12_CLEAR_VALUE *pOptimizedClearValue = nullptr, _In_opt_ D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

#else // XSF_USE_DX_12_0
    _Check_return_
    HRESULT LoadPixelShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11PixelShader** ppPS, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT LoadVertexShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11VertexShader** ppVS,
                              _In_opt_ const D3D11_INPUT_ELEMENT_DESC* pInputElementDesc = NULL, _In_opt_ UINT numElements = 0, _COM_Outptr_ ID3D11InputLayout** ppInputLayout = NULL, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT LoadHullShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11HullShader** ppPS, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT LoadDomainShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11DomainShader** ppPS, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT LoadGeometryShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11GeometryShader** ppGS, _In_opt_ const D3D11_SO_DECLARATION_ENTRY* pSODesc = NULL, _In_opt_ UINT SODescCount = 0, _In_opt_ const UINT* pSOStrides = NULL, _In_opt_ UINT SOStridesCount = 0, _In_opt_ UINT SOFlags = 0, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT LoadComputeShader( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* fileName, _COM_Outptr_ ID3D11ComputeShader** ppCS, _In_opt_ std::vector< BYTE >* pData = nullptr );
    _Check_return_
    HRESULT CreateTextureFromFile( _In_ D3DDevice* pDevice, _In_z_ const wchar_t* pFilename, _COM_Outptr_ ID3D11ShaderResourceView** ppSRV );
	_Check_return_
    HRESULT CreateTextureFromFile( _In_ D3DDevice* pDevice, _In_ D3DDeviceContext* pDeviceContext, _In_z_ const wchar_t* pFilename, _COM_Outptr_ ID3D11ShaderResourceView** ppSRV );
    _Check_return_
    HRESULT CreateConstantBuffer( _In_ D3DDevice* pDev, UINT uSize, _In_opt_ const void* pData, _COM_Outptr_ ID3D11Buffer** ppCB, D3D11_USAGE usage = D3D11_USAGE_DYNAMIC );
    HRESULT ReplaceDynamicConstantBufferContents( _In_ D3DDeviceContext* pCtx, _In_ ID3D11Buffer* pBuffer, UINT uSize, _In_ const void* pData, BOOL flushGpuCache = FALSE );
    _Check_return_
    HRESULT CreateColorTextureAndViews( _In_ D3DDevice* pDev, UINT width, UINT height, DXGI_FORMAT fmt,
                                        _COM_Outptr_ ID3D11Texture2D** ppTexture, _COM_Outptr_opt_ ID3D11RenderTargetView** ppRTV, _COM_Outptr_opt_ ID3D11ShaderResourceView** ppSRV,
                                        D3D11_USAGE usage = D3D11_USAGE_DEFAULT );
    inline HRESULT SetResourceName( _In_ ID3D11DeviceChild* resource, _In_z_ const wchar_t* pName )
    {
    #if defined(_XBOX_ONE) && defined(_TITLE)
        return resource->SetName( pName );
    #else
        UNREFERENCED_PARAMETER( resource );
        UNREFERENCED_PARAMETER( pName );
        return S_OK;
    #endif
    }

#if defined(_XBOX_ONE) && defined(_TITLE)
    _Check_return_
    HRESULT MapResourceDescToESRAM( _Inout_  D3D11_TEXTURE2D_DESC* pDesc, _Inout_ UINT* pUsedEsramBytes );
    _Check_return_
    HRESULT MapResourceDescToESRAM( _Inout_ D3D11_BUFFER_DESC* pDesc, _Inout_ UINT* pUsedEsramBytes );

    void D3d11DescToXgDesc( D3D11_BUFFER_DESC* pD3d11, XG_BUFFER_DESC* pXgDesc );
    void D3d11DescToXgDesc( D3D11_TEXTURE2D_DESC* pD3d11, XG_TEXTURE2D_DESC* pXgDesc );
#endif

#endif // XSF_USE_DX_12_0

    // Internal checkpoints for automated testing
    enum CheckPoint
    {
        CP_MAIN,
        CP_BEFORE_D3D_INIT,
        CP_IN_FRAMEWORK_INIT,
        CP_IN_TICK,
        CP_BEFORE_FRAME_UPDATE,
        CP_AFTER_FRAME_UPDATE,
        CP_BEFORE_FRAME_RENDER,
        CP_AFTER_FRAME_RENDER,
        CP_IN_FRAMEWORK_SHUTDOWN,
        CP_IN_FRAMEWORK_SUSPEND,
        CP_IN_FRAMEWORK_RESUME,
        CP_CUSTOM,
        CP_NUM
    };

#if defined(_XBOX_ONE)
    void InstallExceptionHandler();
#if defined(_TITLE)
    LONG WINAPI ExceptionHandler( EXCEPTION_POINTERS *pExceptionInfo );
#endif
#endif

    void SetTestOptions( _In_opt_ void (*pfnLog)( const char* pText ), _In_opt_ void (*pfnOnError)(), BOOL enableBreakOnError );
    void SetEnableLogging( BOOL enable, CheckPoint checkpointMin = CP_MAIN );
    void SetExitOnError( BOOL exitOnError );
    void SetCheckpointMin( CheckPoint checkpointMin );
    void CloseLoggingFile();
    void CrossCheckpoint( CheckPoint cp, _In_opt_z_ const char* customCheckpoint = nullptr, _In_opt_ float customValue = 0.0f );

    //--------------------------------------------------------------------------------------
    // Aligns a value to a given pow2 boundary
    //--------------------------------------------------------------------------------------
    template< typename t, typename k >
    t AlignToPow2( t a, k b )
    {
        return (a + b - 1) & (~(b - 1));
    }

    //--------------------------------------------------------------------------------------
    // Swaps by creating a temporary
    //--------------------------------------------------------------------------------------
    template<class Num> void Swap(Num& x, Num& y)
    {
	    Num t = y;
	    y = x;
	    x = t;
    }
	
    //--------------------------------------------------------------------------------------
    // Returns x clamped to [a, b]
    //--------------------------------------------------------------------------------------
    template<class Num> Num Clamp(Num x, Num a, Num b)
    {
	    if (a > b)
	    {
		    Swap(a, b);
	    }
		
	    if (x < a) x = a;
	    if (x > b) x = b;
		
	    return x;
    }
	
    //--------------------------------------------------------------------------------------
    // Returns smoothstep(x), where x is clamped to [0,1]
    //--------------------------------------------------------------------------------------
    inline FLOAT Smoothstep(FLOAT x)
    {
	    if (x < 0.0f) x = 0.0f;
	    if (x > 1.0f) x = 1.0f;
	    return ( 3 - 2 * x ) * x * x;
    }

    //--------------------------------------------------------------------------------------
    // Float modulus (x % y)
    //--------------------------------------------------------------------------------------
    inline float ModF(float x, float y)
    {
	    if (y == 0) { return x; }
	    return x - y * floor(x / y);
    }

    //--------------------------------------------------------------------------------------
    // Name: GpuMemory
    // Desc: Helper for managing the lifetime of graphics memory
    //-------------------------------------------------------------------------------------
#if (WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE)
    class GpuMemory
    {
    public:
        GpuMemory()
             : m_pGpuMemory( nullptr ), m_sizeBytes( 0 ) {}

        GpuMemory( _In_ size_t SizeBytes, _In_ UINT Flags = D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT, _In_ SIZE_T AlignmentBytes = 4096, _In_ UINT64 DesiredGpuVirtualAddress = 0 )
             : m_pGpuMemory( nullptr ), m_sizeBytes( 0 ) { Allocate( SizeBytes, Flags, AlignmentBytes, DesiredGpuVirtualAddress ); }

        ~GpuMemory() { Free(); }
    
        void Allocate(_In_ size_t SizeBytes, _In_ UINT Flags = D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT, _In_ SIZE_T AlignmentBytes = 4096, _In_ UINT64 DesiredGpuVirtualAddress = 0);

        void Free();
    
        inline operator void* () { return m_pGpuMemory; }
#ifdef XSF_USE_DX_12_0
        inline operator D3D12_GPU_VIRTUAL_ADDRESS () { return reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_pGpuMemory); }
#endif
        inline operator BYTE* () { return static_cast< BYTE* >( m_pGpuMemory ); }
        inline size_t Size() const { return m_sizeBytes; }

    protected:
        GpuMemory( const GpuMemory& );// = delete;
        GpuMemory& operator = ( const GpuMemory& );// = delete;

        void *m_pGpuMemory;
        size_t m_sizeBytes;
    };
#endif

#ifdef XSF_USE_DX_12_0
#else
    //--------------------------------------------------------------------------------------
    // Name: ImmutableBuffer
    // Desc: Implements access to an immutable buffer
    //-------------------------------------------------------------------------------------
    class ImmutableBuffer
    {
#if defined(_XBOX_ONE) && defined(_TITLE)
        BYTE*        m_pPlacementBuffer;
#endif
        D3DBufferPtr m_buffer;
        
    public:
#if defined(_XBOX_ONE) && defined(_TITLE)
        ImmutableBuffer() : m_pPlacementBuffer( nullptr ) {}
#else
        ImmutableBuffer() {}
#endif
        ~ImmutableBuffer() { Destroy(); }

        void Create( _In_ D3DDevice* pDev, D3D11_BIND_FLAG bindFlags, UINT size, _In_ void* pInitialData );
 
#if defined(_XBOX_ONE) && defined(_TITLE)
        BYTE*         data()      { return m_pPlacementBuffer; }
#endif
        ID3D11Buffer* pd3dbuffer() { return m_buffer.Get(); }
        ID3D11Buffer** ppd3dbuffer() { return m_buffer.GetAddressOf(); }

    private:
        void Destroy();
        ImmutableBuffer( const ImmutableBuffer& );// = delete;
        ImmutableBuffer& operator = ( const ImmutableBuffer& );// = delete;
    };


#if defined(_XBOX_ONE) && defined(_TITLE)
    //--------------------------------------------------------------------------------------
    // Name: SyncRTVtoSRV
    // Desc: Sync operations for fast semantics when switching a RenderTarget to a ShaderResourceView
    //-------------------------------------------------------------------------------------
    void SyncRTVtoSRV( _In_ D3DDeviceContext* const pCtx, _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT - 1) UINT rtIndex, _In_opt_ ID3D11Resource* const pResource = nullptr );

    //--------------------------------------------------------------------------------------
    // Name: SyncDBtoSRV
    // Desc: Sync operations for fast semantics when switching a DepthBuffer to a ShaderResourceView
    //-------------------------------------------------------------------------------------
    void SyncDBtoSRV( _In_ D3DDeviceContext* const pCtx, _In_opt_ ID3D11Resource* const pResource = nullptr );
#endif


    //--------------------------------------------------------------------------------------
    // Name: DynamicBuffer
    // Desc: Implements regular dynamic vertex/index/constant buffer. Uses placement memory on ERA
    //-------------------------------------------------------------------------------------
    class DynamicBuffer
    {
        D3DBufferPtr        m_spBuffer;
        D3DDeviceContext*   m_mappedOnContext;
        UINT                m_bufferTailOffset;
        UINT                m_numBytesMapped;
        UINT                m_bufferSize;
        BOOL                m_alwaysDiscard;

    public:
        DynamicBuffer();
        ~DynamicBuffer();

        _Check_return_
        HRESULT Create( _In_ D3DDevice* pDev, D3D11_BIND_FLAG bindFlags, UINT size );
        void    Destroy();

        _Check_return_
        HRESULT Map( _In_ D3DDeviceContext* pCtx, UINT numBytesToMap, _Outptr_ void** ppData, _Out_opt_ UINT* pOffset = nullptr );
        void    Unmap( UINT numBytesUsed = 0 );

        ID3D11Buffer* const&   GetBuffer() const;       // * const& is to be able to write &GetBuffer() to get a ** for passing to D3D functions
        UINT            GetNumBytesLastMapped() const;
        UINT            GetTailOffset() const;

    private:
        DynamicBuffer( const DynamicBuffer& );// = delete;
        DynamicBuffer& operator = ( const DynamicBuffer& );// = delete;
    };


    //--------------------------------------------------------------------------------------
    // Name: RoundRobinBuffer
    // Desc: Implements access to buffered resources so that frame doesn't have to block on accessing it
    //-------------------------------------------------------------------------------------
    class RoundRobinBuffer
    {
#ifdef MAP_USAGE_DEFAULT_PLACEMENT
        BYTE* m_pPlacementBuffer;
        UINT byteWidth;
#endif
#if defined(MAP_USAGE_DEFAULT) || defined(MAP_USAGE_DEFAULT_PLACEMENT)
        std::vector< UINT > m_index;
#endif
        std::vector< D3DBufferPtr > m_buffer;
        UINT m_nContexts;
        UINT m_nObjects;
        UINT m_nBuffering;
        
    public:
        RoundRobinBuffer();
        ~RoundRobinBuffer();

        void CreateBuffers( _In_ D3DDevice* pDev, _In_ D3D11_BUFFER_DESC* pDesc );
        void Destroy();

        template < class Buffer, class MappedData >
        void Map( _In_ D3DDeviceContext* pd3dContext, UINT iContext, UINT iObject,  _Outptr_ Buffer** ppBuffer, _Outptr_ MappedData** ppMappedData );
        template < class Buffer >
        void Unmap( _In_ D3DDeviceContext* pd3dContext, _In_ Buffer* pBuffer );
        void SetNumBuffers( UINT nContexts, UINT nObjects, UINT nMapsPerFrame, UINT nFramesBuffering );
    };

    //--------------------------------------------------------------------------------------
    // Name: Map
    // Desc: Maps a requested buffer for CPU access
    //-------------------------------------------------------------------------------------
    _Use_decl_annotations_
    template < class Buffer, class MappedData >
    void RoundRobinBuffer::Map( 
        D3DDeviceContext* pd3dContext, 
        UINT iContext, 
        UINT iObject, 
        Buffer** ppBuffer, 
        MappedData** ppMappedData )
    {
#if defined(MAP_USAGE_DEFAULT) || defined(MAP_USAGE_DEFAULT_PLACEMENT)
        UINT iObjectIdx = iContext * m_nObjects + iObject;
        UINT &iBufIdx = m_index[ iObjectIdx ];
    #ifdef MAP_USAGE_DEFAULT_PLACEMENT 
        UNREFERENCED_PARAMETER( pd3dContext );
        *ppMappedData = reinterpret_cast< MappedData* >( &m_pPlacementBuffer[ ( iObjectIdx * m_nBuffering + iBufIdx ) * byteWidth ] );
        *ppBuffer = m_buffer[ 0 ];
    #else
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        *ppBuffer = m_buffer[ iObjectIdx * m_nBuffering + iBufIdx ];
        XSF_ERROR_IF_FAILED( pd3dContext->Map( *ppBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 
            D3D11_MAP_FLAG_ALLOW_USAGE_DEFAULT, &mappedResource ) );
        *ppMappedData = reinterpret_cast< MappedData* >( mappedResource.pData );
    #endif
        iBufIdx++;
        if ( iBufIdx >= m_nBuffering )
        {
            iBufIdx = 0;
        }
#else
        UNREFERENCED_PARAMETER( iContext );
        UNREFERENCED_PARAMETER( iObject );
        *ppBuffer = m_buffer[ 0 ];
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        XSF_ERROR_IF_FAILED( pd3dContext->Map( *ppBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ) );
        *ppMappedData = reinterpret_cast< MappedData* >( mappedResource.pData );
#endif
    }

    //--------------------------------------------------------------------------------------
    // Name: BytePtrToUint64
    // Desc: Converts a byte ptr address to UINT64 
    //-------------------------------------------------------------------------------------
    template< class T >
    UINT64 BytePtrToUint64( _In_ T* ptr )
    {
        return static_cast< UINT64 >( reinterpret_cast< BYTE* >( ptr ) - static_cast< BYTE* >( nullptr ) );
    }
    
    //--------------------------------------------------------------------------------------
    // Name: Unmap
    // Desc: Unmaps a requested buffer
    //-------------------------------------------------------------------------------------
    _Use_decl_annotations_
    template < class Buffer >
    void RoundRobinBuffer::Unmap( D3DDeviceContext* pd3dContext, Buffer* pBuffer )
    {
#ifdef MAP_USAGE_DEFAULT_PLACEMENT
        UNREFERENCED_PARAMETER( pd3dContext );
        UNREFERENCED_PARAMETER( pBuffer );
    #ifndef MAP_PLACEMENT_BUFFERS_CACHE_LINE_ALIGNMENT_PACK
        #error use cache line aligned buffers, otherwise an explicit flush for fast semantics is required here
    #endif
#else
        pd3dContext->Unmap( pBuffer, 0 );
#endif
    }
#endif // XSF_USE_DX_12_0

    //--------------------------------------------------------------------------------------
    // Name: XsfScopedNamedEvent
    // Desc: A helper class to implement a pair of PIXBegin/PIXEndEvents
    //-------------------------------------------------------------------------------------
    struct XsfScopedNamedEvent final
    {
        // Because PIXBeginEvent infers the type of varargs using templates, 
        // we cannot pass varargs straight through. So we must resolve the message
        // string here.
#ifdef XSF_USE_PIX_EVENTS
#define SCOPED_NAMED_EVENT_CONSTRUCTOR( m_pCtx, pCtx, color, text ) \
        m_pCtx = pCtx;                                              \
                                                                    \
        wchar_t wbuf[ 1024 ];                                       \
        va_list ap;                                                 \
        va_start( ap, text );                                       \
        vswprintf_s( wbuf, _countof( wbuf ) - 1, text, ap );        \
        va_end( ap );                                               \
                                                                    \
        XSFBeginNamedEvent( m_pCtx, color, wbuf );             
#else
#define SCOPED_NAMED_EVENT_CONSTRUCTOR( m_pCtx, pCtx, color, text ) \
        UNREFERENCED_PARAMETER( pCtx );                             \
        UNREFERENCED_PARAMETER( color );                            \
        UNREFERENCED_PARAMETER( text );                             
#endif

#ifdef XSF_USE_DX_12_0        
        D3DCommandQueue* m_pCmdQueue;
        D3DCommandList* m_pCmdList;

        XsfScopedNamedEvent( _In_opt_ D3DCommandQueue* pCmdQueue, DWORD color, _In_z_ const wchar_t* text, ... ) : m_pCmdQueue( nullptr ), m_pCmdList( nullptr )
        {
            SCOPED_NAMED_EVENT_CONSTRUCTOR( m_pCmdQueue, pCmdQueue, color, text )
        }
        XsfScopedNamedEvent( _In_opt_ D3DCommandList* pCmdList, DWORD color, _In_z_ const wchar_t* text, ... ) : m_pCmdQueue( nullptr ), m_pCmdList( nullptr )
        {
            SCOPED_NAMED_EVENT_CONSTRUCTOR( m_pCmdList, pCmdList, color, text )
        }
#else
        D3DDeviceContext* m_pCtx;

        XsfScopedNamedEvent( _In_opt_ D3DDeviceContext* pCtx, DWORD color, _In_z_ const wchar_t* text, ... ) : m_pCtx( nullptr )
        {
            SCOPED_NAMED_EVENT_CONSTRUCTOR( m_pCtx, pCtx, color, text )
        }
#endif

        ~XsfScopedNamedEvent()
        {
#ifdef XSF_USE_PIX_EVENTS
#ifdef XSF_USE_DX_12_0  
            if( m_pCmdQueue )
            {
                XSFEndNamedEvent( m_pCmdQueue );
            }
            else
            {
                XSFEndNamedEvent( m_pCmdList );
            }
#else
            XSFEndNamedEvent( m_pCtx );
#endif
#endif
        }
    };

    void SetContentFileRoot();

    //--------------------------------------------------------------------------------------
    // Name: IsPowerOfTwo
    // Desc: Is n a power of two?
    //--------------------------------------------------------------------------------------
    inline BOOL IsPowerOfTwo( UINT64 n )
    {
        return ( ( n & (n-1) ) == 0 && (n) != 0 );
    }

    //--------------------------------------------------------------------------------------
    // Name: NextMultiple
    // Desc: Next multiple after value
    //--------------------------------------------------------------------------------------
    inline UINT64 NextMultiple( UINT64 value, UINT64 multiple )
    {
        XSF_ASSERT( IsPowerOfTwo(multiple) );

        return (value + multiple - 1) & ~(multiple - 1);
    }

    //--------------------------------------------------------------------------------------
    // Name: Align
    // Desc: Align a pointer to the given alignment
    //--------------------------------------------------------------------------------------
    inline char* Align(char* pPtr, UINT64 alignment)
    {
        UINT64 ptr = (UINT64)pPtr;
        if(ptr % alignment != 0)
        {
            return (char*)( ptr + (alignment - ptr % alignment) );
        }
        else
        {
            return pPtr;
        }        
    }

    //--------------------------------------------------------------------------------------
    // Name: log2
    // Desc: Log base 2
    //--------------------------------------------------------------------------------------
    inline UINT log2( UINT n )
    {
        DOUBLE d = (DOUBLE)n;
        return (UINT) ( log(d) / log(2) );
    }

    //--------------------------------------------------------------------------------------
    // Name: SwapErase
    // Desc: Fast erase from a vector
    //--------------------------------------------------------------------------------------
    template< typename T >
    void SwapErase( std::vector<T> & vec, UINT index )
    {
        std::swap( vec[index], vec.back() );
        vec.pop_back();
    }

    //--------------------------------------------------------------------------------------
    // Name: XSetThreadName
    // Desc: Set the name of the given thread so that it will show up in the Threads Window
    //       in Visual Studio and in PIX timing captures.
    //--------------------------------------------------------------------------------------
    void XSetThreadName( DWORD dwThreadID, LPCSTR strThreadName );

    inline void XSetThreadProcessor( HANDLE hThread, DWORD cpuCore )
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE)
        SetThreadAffinityMask( hThread, ( ( DWORD_PTR )( 0x1 ) << cpuCore ) );
#else
        UNREFERENCED_PARAMETER( hThread );
		UNREFERENCED_PARAMETER( cpuCore );
#endif
    }

    //----------------------------------------------------------------------------
	// Critical section RAII wrapper
	//----------------------------------------------------------------------------
    class CriticalSection
    {
    public:

        // Initializes with no spin-count by default.
        CriticalSection(
            _In_ DWORD dwSpinCount = 0 )
        {
            InitializeCriticalSectionAndSpinCount( &m_cs, dwSpinCount );
        }

        ~CriticalSection()
        {
            DeleteCriticalSection( &m_cs );
        }

        BOOL TryAcquire()
        {
            return TryEnterCriticalSection( &m_cs );
        }

        void Acquire()
        {
            EnterCriticalSection( &m_cs );
        }

        void Release()
        {
            LeaveCriticalSection( &m_cs );
        }

    private:
        CriticalSection(const CriticalSection&);
        CriticalSection& operator = (const CriticalSection&);

        CRITICAL_SECTION m_cs;
    };

    //----------------------------------------------------------------------------
	// Scoped critical section lock: locks a critical section for a given scope
	//----------------------------------------------------------------------------
    class ScopedCriticalSectionLock
    {
    public:

        ScopedCriticalSectionLock( CriticalSection& cs )
            : m_cs( cs )
        {
            m_cs.Acquire();
        }

        ~ScopedCriticalSectionLock()
        {
            m_cs.Release();
        }

    private:
        ScopedCriticalSectionLock(const ScopedCriticalSectionLock&);
        ScopedCriticalSectionLock& operator = (const ScopedCriticalSectionLock&);

        CriticalSection& m_cs;
    };
}

namespace XSF = XboxSampleFramework;

#endif      // XSF_COMMON_H
