//--------------------------------------------------------------------------------------
// Common.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "Common.h"
#ifdef _XBOX_ONE
#include <dbghelp.h>
#endif

namespace XboxSampleFramework
{
    namespace Details
    {
        wchar_t    g_strCommonFileRoot[ 1024 ];
        wchar_t    g_strApplicationDataPath[ 1024 ];

        // Default logging
        static const wchar_t* CHECKPOINT_FILE_NAME = L"XSF.log";
        CheckPoint g_checkpointMin;		// min checkpoint to log
        UINT64   g_checkpointTimings[ XSF::CP_NUM ];
        FLOAT    g_checkpointDeltaFromStart[ XSF::CP_NUM ];
        FILE*    g_pCheckpointLogFile;
        wchar_t    g_checkpointFileName[ MAX_PATH ];

        // Support for automatic testing
        void    (*g_pLoggingFunctionIntercept)( const char* pText );
        void    (*g_pStopOnErrorFunctionIntercept)();
        BOOL     g_breakOnError = TRUE;
        BOOL     g_exitOnError = FALSE;
    }

}

namespace XSF = XboxSampleFramework;

//--------------------------------------------------------------------------------------
// Name: GetExecutableName
// Desc: Retrieves the name of the currently running program
//--------------------------------------------------------------------------------------
BOOL XSF::GetExecutableName( _Out_writes_(executableNameSize) wchar_t* strExecutableName, DWORD _In_range_(1,256) executableNameSize )
{
#if defined(_XBOX_ONE) && defined(_TITLE)   // #tbb - These APIs only exist for Desktop/UAP under TH
    HMODULE hModule = nullptr;

    strExecutableName[0] = L'\0';

    if( !GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(XSF::GetExecutableName), &hModule ) )
    {
        return FALSE;
    }

    if( !GetModuleFileName( hModule, strExecutableName, executableNameSize ) )
    {
        return FALSE;
    }

    wchar_t *strLastSlash = wcsrchr( strExecutableName, L'\\' );
    if( strLastSlash )
    {
        // Chop the exe name from the exe path
        wcscpy_s( strExecutableName, MAX_PATH, &strLastSlash[1] );

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExecutableName, L'.' );
        if( strLastSlash )
        {
            *strLastSlash = 0;
        }
    }
#else
    wcscpy_s(strExecutableName, executableNameSize, L"Unknown");
#endif

    return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: SetContentFileRoot
// Desc: Retrieves a game package read-only data path and stores it for future use
//--------------------------------------------------------------------------------------
void XSF::SetContentFileRoot()
{
#ifdef _XBOX_ONE

    std::wstring installFolder = Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data();

    // Remove any trailing "\\" from the end of installFolder path
    installFolder.erase( installFolder.find_last_not_of( L"\\" ) + 1 );

    _snwprintf_s( Details::g_strCommonFileRoot, _countof( Details::g_strCommonFileRoot ), _TRUNCATE, L"%s\\", installFolder.c_str() );

    // The d: drive designator is "developer scratch space"
    std::wstring  writeableFolder = L"d:\\";

    // Remove any trailing "\\" from the end of writeableFolder path
    writeableFolder.erase( writeableFolder.find_last_not_of( L"\\" ) + 1 );

    _snwprintf_s( Details::g_strApplicationDataPath, _countof( Details::g_strApplicationDataPath ), _TRUNCATE, L"%s\\", writeableFolder.c_str() );

#else
    wchar_t installFolder[ 1024 ];
    GetModuleFileName( nullptr, installFolder, 1024 );
    installFolder[MAX_PATH - 1] = 0;
    WCHAR* strLastSlash = wcsrchr( installFolder, L'\\' );
    if (strLastSlash)
    {
        *(strLastSlash) = 0;
    }
    swprintf_s( Details::g_strCommonFileRoot, L"%s\\", installFolder );
    swprintf_s( Details::g_strApplicationDataPath, L"%s\\", installFolder );
#endif
}

//--------------------------------------------------------------------------------------
// Name: GetContentFileRoot
// Desc: Returns the read-only game data package location
//--------------------------------------------------------------------------------------
const wchar_t* XSF::GetContentFileRoot()
{
    return Details::g_strCommonFileRoot;
}

//--------------------------------------------------------------------------------------
// Name: GetStorageFileRoot
// Desc: Returns the path to data storage
//--------------------------------------------------------------------------------------
const wchar_t* XSF::GetStorageFileRoot()
{
    return Details::g_strApplicationDataPath;
}

//--------------------------------------------------------------------------------------
// Name: DebugPrint
// Desc: Outputs a message to the debugger
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::DebugPrint( const char* msg, ... )
{
    va_list ap;

    va_start( ap, msg );

    char   buf[ 4096 ];
    vsprintf_s( buf, msg, ap );
    
    va_end( ap );

#ifndef NDEBUG
    OutputDebugStringA( buf );
    printf( buf );
#endif

    // Pass to the intercept if given
    if( Details::g_pLoggingFunctionIntercept )
    {
        Details::g_pLoggingFunctionIntercept( buf );
    }
}

//--------------------------------------------------------------------------------------
// Name: DebugPrint
// Desc: Outputs a message to the debugger
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::DebugPrint( const wchar_t* msg, ... )
{
    wchar_t buf[ 4096 ];

    va_list ap;

    va_start( ap, msg );

    vswprintf_s( buf, _countof( buf ) - 1, msg, ap );
    buf[ _countof( buf ) - 1 ] = 0;

    va_end( ap );

    msg = buf;

    PrintNoVarargs( buf );
}

//--------------------------------------------------------------------------------------
// Name: PrintNoVarargs
// Desc: Outputs a message to the debugger
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::PrintNoVarargs( const wchar_t* msg )
{
    OutputDebugStringW( msg );

    // Pass to the intercept if given
    if( Details::g_pLoggingFunctionIntercept )
    {
        char buf[ 4096 ];
        size_t converted;
        wcstombs_s( &converted, buf, msg, _countof( buf ) - 1 );
        buf[ converted ] = 0;
        Details::g_pLoggingFunctionIntercept( buf );
    }
}

//--------------------------------------------------------------------------------------
// Name: ReportFailureAndStop
// Desc: Reports the error and stops
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::ReportFailureAndStop( const char* pFmt, LPCSTR strFileName, DWORD dwLineNumber, ... )
{
    // Format the message from the call site
    va_list ap;

    va_start( ap, dwLineNumber );

    char   buf[ 1024 ];
    vsprintf_s( buf, pFmt, ap );

    va_end( ap );

    // Prepend with file name and line number
    char buf2[ 1024 ];
    sprintf_s( buf2, "%s(%d): %s\n", strFileName, dwLineNumber, buf );

    // Display to log
    wchar_t   wbuf[ _countof( buf2 ) ];
    size_t converted;
    mbstowcs_s( &converted, wbuf, buf2, strlen( buf2 ) );
    PrintNoVarargs( wbuf );

    // Tell the test harness there is an error
    if( Details::g_pStopOnErrorFunctionIntercept )
    {
        Details::g_pStopOnErrorFunctionIntercept();
    }

    // Ignore the message box and debug break if asked
    if( Details::g_breakOnError )
    {
        if( !IsDebuggerPresent() )
        {
#if defined( _XBOX_ONE )
            // Thrown exception will hang the sample sometimes so using a slightly different exit mechanism
            // This behaviour may change in the future
            Windows::ApplicationModel::Core::CoreApplication::Exit();
            Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
#else
            MessageBoxA( nullptr, buf2, "Error", MB_OK );
#endif
        }

        // Stop or break into the debugger
        __debugbreak();
    }

    // exit the application if requested
    if( Details::g_exitOnError )
    {
#if defined( _XBOX_ONE )
        Windows::ApplicationModel::Core::CoreApplication::Exit();
        Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
#else
        exit(1);
#endif
    }
    
}

//--------------------------------------------------------------------------------------
// Name: LoadBlob
// Desc: Reads a file from the read only data content path
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadBlob( const wchar_t* pFilename, std::vector< BYTE >& data )
{
    VERBOSEATGPROFILETHIS;

    data.clear();

    wchar_t tmp[ 1024 ];
    _snwprintf_s( tmp, _countof( tmp ), _TRUNCATE, L"%s%s", Details::g_strCommonFileRoot, pFilename );

    FILE* fp = nullptr;

    _wfopen_s( &fp, tmp, L"rb" );

    if( fp )
    {
        fseek( fp, 0, SEEK_END );
        long sz = ftell( fp );
        fseek( fp, 0, SEEK_SET );

        try
        {
            data.resize( sz );
        }
        catch( std::bad_alloc & )
        {
            return E_OUTOFMEMORY;
        }

        if( sz > 0 )
        {
            fread( &data[ 0 ], sz, 1, fp );
        }

        fclose( fp );

#ifdef DEBUG
        DebugPrint( L"LoadBlob: Read file %s, size=%d\n", tmp, sz );
#endif

        return S_OK;
    }

    // Only output a failed message if this was not the "test" file
    if ( _wcsicmp( pFilename, L"test" ) )
    {
        DebugPrint( L"LoadBlob: Failed to open file %s\n", tmp );
    }

    return E_FAIL;
}


#ifdef XSF_USE_DX_12_0
//--------------------------------------------------------------------------------------
// Name: LoadShader()
// Desc: Load a shader blob from file
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadShader( const wchar_t* path, ID3DBlob** ppShader )
{
    VERBOSEATGPROFILETHIS;

    wchar_t tmp[ 1024 ];
    _snwprintf_s( tmp, _TRUNCATE, L"%s%s", Details::g_strCommonFileRoot, path );

    return D3DReadFileToBlob( tmp, ppShader );
}


//--------------------------------------------------------------------------------------
// Name: CreateColorTextureAndViews
// Desc: Creates the texture of a given size and all necessary views for it
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::CreateColorTextureAndViews(XSF::D3DDevice* pDevice, UINT width, UINT height, DXGI_FORMAT fmt,
                                        ID3D12Resource** ppTexture, D3D12_CPU_DESCRIPTOR_HANDLE hRTV, D3D12_CPU_DESCRIPTOR_HANDLE hSRV,
                                        D3D12_CLEAR_VALUE *pOptimizedClearValue, D3D12_HEAP_TYPE heapType)
{
    VERBOSEATGPROFILETHIS;

    D3D12_RESOURCE_DESC descTex = CD3DX12_RESOURCE_DESC::Tex2D(fmt, width, height, 1, 1);
    D3D12_HEAP_FLAGS heapMiscFlag = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON;
    switch (heapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT:
        descTex.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        break;
    case D3D12_HEAP_TYPE_UPLOAD:
        usage = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;
    case D3D12_HEAP_TYPE_READBACK:
    {
        usage = D3D12_RESOURCE_STATE_COPY_DEST;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout;
        UINT NumRows;
        UINT64 RowSize;
        UINT64 TotalBytes;
        pDevice->GetCopyableFootprints(&descTex, 0, 1, 0, &Layout, &NumRows, &RowSize, &TotalBytes);
        descTex = CD3DX12_RESOURCE_DESC::Buffer(TotalBytes);
    }
        break;
    }

    const D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(heapType);
    XSF_ERROR_IF_FAILED(pDevice->CreateCommittedResource(
        &heapProperties,
        heapMiscFlag,
        &descTex,
        usage,
        pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(ppTexture)));
    
    if (hRTV.ptr != 0)
    {
        D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
        descRTV.Format = fmt;
        descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        pDevice->CreateRenderTargetView(*ppTexture, &descRTV, hRTV);
    }

    if (hSRV.ptr != 0)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
        descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        descSRV.Format = fmt;
        descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        descSRV.Texture2D.MipLevels = 1;
        pDevice->CreateShaderResourceView(*ppTexture, &descSRV, hSRV);
    }

    return S_OK;
}
#else // XSF_USE_DX_12_0
//--------------------------------------------------------------------------------------
// Name: LoadPixelShader()
// Desc: Load a pixel shader
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadPixelShader( XSF::D3DDevice* pDev, const wchar_t* path, ID3D11PixelShader** ppPS, std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( path, *pData );
    if( FAILED( hr ) )
        return hr;

    return pDev->CreatePixelShader( &(*pData)[ 0 ], pData->size(), nullptr, ppPS );
}

//--------------------------------------------------------------------------------------
// Name: LoadVertexShader()
// Desc: Load a vertex shader
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadVertexShader( XSF::D3DDevice* pDev, const wchar_t* path, ID3D11VertexShader** ppVS, 
                               const D3D11_INPUT_ELEMENT_DESC* pInputElementDesc, UINT numElements, ID3D11InputLayout** ppInputLayout,
                               std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    if( ppInputLayout )
        *ppInputLayout = nullptr;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( path, *pData );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = pDev->CreateVertexShader( &(*pData)[ 0 ], pData->size(), nullptr, ppVS );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if ( pInputElementDesc && numElements && ppInputLayout )
    {
        hr = pDev->CreateInputLayout( pInputElementDesc, numElements, &(*pData)[ 0 ], pData->size(), ppInputLayout );
        if ( FAILED( hr ) )
        {
            return hr;
        }        
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: LoadGeometryShader()
// Desc: Load a geometry shader with optional SO
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadGeometryShader( XSF::D3DDevice* pDevice,
                                 const wchar_t* fileName,
                                 ID3D11GeometryShader** ppGS,
                                 const D3D11_SO_DECLARATION_ENTRY* pSODesc,
                                 UINT SODescCount,
                                 const UINT* pSOStrides,
                                 UINT SOStridesCount,
                                 UINT SOFlags,
                                 std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( fileName, *pData );
    if( FAILED( hr ) )
        return hr;

    if( !pSODesc )
    {
        return pDevice->CreateGeometryShader( &(*pData)[ 0 ], pData->size(), nullptr, ppGS );
    }

    return pDevice->CreateGeometryShaderWithStreamOutput( &(*pData)[ 0 ], 
                                                          pData->size(), 
                                                          pSODesc,
                                                          SODescCount,
                                                          pSOStrides,
                                                          SOStridesCount,
                                                          SOFlags,
                                                          nullptr,
                                                          ppGS );
}

//--------------------------------------------------------------------------------------
// Name: LoadHullShader()
// Desc: Load a hull shader
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadHullShader( XSF::D3DDevice* pDev, const wchar_t* path, ID3D11HullShader** ppPS, std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( path, *pData );
    if( FAILED( hr ) )
        return hr;

    return pDev->CreateHullShader( &(*pData)[ 0 ], pData->size(), nullptr, ppPS );
}


//--------------------------------------------------------------------------------------
// Name: LoadDomainShader()
// Desc: Load a domain shader
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadDomainShader( XSF::D3DDevice* pDev, const wchar_t* path, ID3D11DomainShader** ppPS, std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( path, *pData );
    if( FAILED( hr ) )
        return hr;

    return pDev->CreateDomainShader( &(*pData)[ 0 ], pData->size(), nullptr, ppPS );
}

//--------------------------------------------------------------------------------------
// Name: LoadComputeShader()
// Desc: Load a compute shader
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::LoadComputeShader( D3DDevice* pDev, const wchar_t* path, ID3D11ComputeShader** ppCS, std::vector< BYTE >* pData )
{
    VERBOSEATGPROFILETHIS;

    std::vector< BYTE > data;
    if( !pData )
        pData = &data;

    HRESULT hr = XSF::LoadBlob( path, *pData );
    if( FAILED( hr ) )
        return hr;

    return pDev->CreateComputeShader( &(*pData)[ 0 ], pData->size(), nullptr, ppCS );
}

//--------------------------------------------------------------------------------------
// Name: CreateConstantBuffer
// Desc: Creates a dynamic, CPU-writable constant buffer. Optionally a default CB can be created.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::CreateConstantBuffer( XSF::D3DDevice* pDev, UINT uSize, const void* pData, ID3D11Buffer** ppCB, D3D11_USAGE usage )
{
    VERBOSEATGPROFILETHIS;

    D3D11_BUFFER_DESC bufDesc = { 0 };
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.ByteWidth = (uSize + 15) & ~15;     // has to be a multiple of 16 otherwise D3D is unhappy
    bufDesc.Usage = usage;
    if( D3D11_USAGE_DYNAMIC == usage )
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA dataDesc = { 0 };
    dataDesc.pSysMem = pData;

    return pDev->CreateBuffer( &bufDesc, pData ? &dataDesc : nullptr, ppCB );
}

//--------------------------------------------------------------------------------------
// Name: ReplaceDynamicConstantBufferContents
// Desc: Replaced contents of a CB with given data
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::ReplaceDynamicConstantBufferContents( XSF::D3DDeviceContext* pCtx, ID3D11Buffer* pBuffer, UINT uSize, const void* pData, BOOL flushGpuCache )
{
    VERBOSEATGPROFILETHIS;

    D3D11_MAPPED_SUBRESOURCE    mapped;

    HRESULT hr = pCtx->Map( pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
    if( FAILED( hr ) )
        return hr;

    memcpy( mapped.pData, pData, uSize );

    pCtx->Unmap( pBuffer, 0 );

#if defined(_XBOX_ONE) && defined(_TITLE)
    if( flushGpuCache )
    {
        pCtx->FlushGpuCacheRange( D3D11_FLUSH_KCACHE_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL );
    }
#else
    UNREFERENCED_PARAMETER( flushGpuCache );
#endif

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CreateColorTextureAndViews
// Desc: Creates the texture of a given size and all necessary views for it
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::CreateColorTextureAndViews( XSF::D3DDevice* pDev, UINT width, UINT height, DXGI_FORMAT fmt,
                                ID3D11Texture2D** ppTexture, ID3D11RenderTargetView** ppRTV, ID3D11ShaderResourceView** ppSRV,
                                D3D11_USAGE usage )
{
    VERBOSEATGPROFILETHIS;

    if( ppRTV )
        *ppRTV = nullptr;
    if( ppSRV )
        *ppSRV = nullptr;

    HRESULT hr;

    D3D11_TEXTURE2D_DESC texDesc;
    ZeroMemory( &texDesc, sizeof( texDesc ) );
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = fmt;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = usage;
    texDesc.BindFlags = (usage != D3D11_USAGE_STAGING) ? (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE) : 0;
    texDesc.CPUAccessFlags = (usage != D3D11_USAGE_STAGING) ? 0 : D3D11_CPU_ACCESS_READ;
    texDesc.MiscFlags = 0;
    hr = pDev->CreateTexture2D( &texDesc, nullptr, ppTexture );
    if( FAILED( hr ) )
        return hr;

    if( texDesc.BindFlags & D3D11_BIND_RENDER_TARGET )
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        ZeroMemory( &rtvDesc, sizeof( rtvDesc ) );
        rtvDesc.Format = fmt;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        hr = pDev->CreateRenderTargetView( *ppTexture, &rtvDesc, ppRTV );
        if( FAILED( hr ) )
            return hr;
    }

    if( texDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = fmt;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        hr = pDev->CreateShaderResourceView( *ppTexture, &srvDesc, ppSRV );
        if( FAILED( hr ) )
            return hr;
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Marks the resource descriptor to allocate the resource in esram
//--------------------------------------------------------------------------------------
#if defined(_XBOX_ONE) && defined(_TITLE)

template <class Desc>
HRESULT MapResourceDescToESRAM( 
    Desc* pDesc, UINT byteWidth, 
    UINT esramBaseAlignmentBytes, 
    UINT* pUsedEsramBytes )
{
    UINT baseEsramAddress = static_cast< UINT >( XSF::NextMultiple( *pUsedEsramBytes, esramBaseAlignmentBytes ) );

    const UINT ESRAM_SIZE = 32 * 1024 * 1024;
    if ( baseEsramAddress + byteWidth <= ESRAM_SIZE )
    {
        pDesc->MiscFlags |= D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;
        //desc->ESRAMUsageBytes = 0;
        pDesc->ESRAMOffsetBytes = baseEsramAddress;
        *pUsedEsramBytes = baseEsramAddress + byteWidth;
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

_Use_decl_annotations_
void XSF::D3d11DescToXgDesc( D3D11_BUFFER_DESC* pDesc, XG_BUFFER_DESC* pXgDesc )
{
    ZeroMemory( pXgDesc, sizeof(*pXgDesc) );
    pXgDesc->ByteWidth = pDesc->ByteWidth;
    pXgDesc->Usage = static_cast<XG_USAGE>( pDesc->Usage );
    pXgDesc->BindFlags = pDesc->BindFlags;
    pXgDesc->CPUAccessFlags = pDesc->CPUAccessFlags;
    pXgDesc->MiscFlags = pDesc->MiscFlags;
    pXgDesc->StructureByteStride = pDesc->StructureByteStride;
    pXgDesc->ESRAMOffsetBytes = pDesc->ESRAMOffsetBytes;
    pXgDesc->ESRAMUsageBytes = pDesc->ESRAMUsageBytes;
}

_Use_decl_annotations_
void XSF::D3d11DescToXgDesc( D3D11_TEXTURE2D_DESC* pDesc, XG_TEXTURE2D_DESC* pXgDesc )
{
    ZeroMemory( pXgDesc, sizeof(*pXgDesc) );
    pXgDesc->Width = pDesc->Width;
    pXgDesc->Height = pDesc->Height;
    pXgDesc->MipLevels = pDesc->MipLevels;
    pXgDesc->ArraySize = pDesc->ArraySize;
    pXgDesc->Format = static_cast<XG_FORMAT>( pDesc->Format );
    pXgDesc->SampleDesc.Count = pDesc->SampleDesc.Count;
    pXgDesc->SampleDesc.Quality = pDesc->SampleDesc.Quality;
    pXgDesc->Usage = static_cast<XG_USAGE>( pDesc->Usage );
    pXgDesc->BindFlags = static_cast<XG_BIND_FLAG>( pDesc->BindFlags );
    pXgDesc->CPUAccessFlags = pDesc->CPUAccessFlags;
    pXgDesc->MiscFlags = pDesc->MiscFlags;
    pXgDesc->ESRAMOffsetBytes = pDesc->ESRAMOffsetBytes;
    pXgDesc->ESRAMUsageBytes = pDesc->ESRAMUsageBytes;
    pXgDesc->TileMode = XGComputeOptimalTileMode( 
        XG_RESOURCE_DIMENSION_TEXTURE2D, pXgDesc->Format, pXgDesc->Width, 
        pXgDesc->Height, pXgDesc->ArraySize, pXgDesc->SampleDesc.Count, pXgDesc->BindFlags );
    pXgDesc->Pitch = 0;
}

_Use_decl_annotations_
HRESULT XSF::MapResourceDescToESRAM( D3D11_TEXTURE2D_DESC* pDesc, UINT* pUsedEsramBytes )
{
    // Calculate the size and alignment required to create the texture in ESRAM
    XG_TEXTURE2D_DESC xgDesc;
    D3d11DescToXgDesc( pDesc, &xgDesc );

    Microsoft::WRL::ComPtr<XGTextureAddressComputer> texAddressComputer = nullptr;
    XSF_ERROR_IF_FAILED( XGCreateTexture2DComputer( 
        &xgDesc, texAddressComputer.ReleaseAndGetAddressOf() ) );

    XG_RESOURCE_LAYOUT xgLayout;
    XSF_ERROR_IF_FAILED( texAddressComputer->GetResourceLayout( &xgLayout ) );
   
    return MapResourceDescToESRAM( pDesc, static_cast<UINT>( xgLayout.SizeBytes ), 
        static_cast<UINT>( xgLayout.BaseAlignmentBytes ), pUsedEsramBytes );
}

_Use_decl_annotations_
HRESULT XSF::MapResourceDescToESRAM( D3D11_BUFFER_DESC* pDesc, UINT* pUsedEsramBytes )
{
    // Calculate the size and alignment required to create the buffer in ESRAM
    XG_BUFFER_DESC xgDesc;
    D3d11DescToXgDesc( pDesc, &xgDesc );

    XG_RESOURCE_LAYOUT xgLayout;
    XSF_ERROR_IF_FAILED( XGComputeBufferLayout( &xgDesc, &xgLayout ) ); 

    return MapResourceDescToESRAM( pDesc, static_cast<UINT>( xgLayout.SizeBytes ), 
        static_cast<UINT>( xgLayout.BaseAlignmentBytes ), pUsedEsramBytes );
}
#endif
#endif // XSF_USE_DX_12_0


//--------------------------------------------------------------------------------------
// Name: InstallExceptionHandler
// Desc: Installs an exception handler to write minidumps
//--------------------------------------------------------------------------------------
#if defined(_XBOX_ONE)

_Use_decl_annotations_
void XSF::InstallExceptionHandler()
{
#if defined(_TITLE)
    ::SetUnhandledExceptionFilter( XSF::ExceptionHandler );
#endif
}

#if defined(_TITLE)
LONG WINAPI XSF::ExceptionHandler(_In_ struct _EXCEPTION_POINTERS* pException)
{
    BOOL bDebuggerAttached = ::IsDebuggerPresent(); 
    if( bDebuggerAttached )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

    BOOL bMiniDumpResult = FALSE;
    HMODULE hToolHelpModule = ::LoadLibrary(L"toolhelpx.dll");
    if( hToolHelpModule != INVALID_HANDLE_VALUE )
    {
        typedef BOOL (WINAPI* MiniDumpWriteDumpPfn) (
            _In_ HANDLE hProcess,
            _In_ DWORD ProcessId,
            _In_ HANDLE hFile,
            _In_ MINIDUMP_TYPE DumpType,
            _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
            _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
            _Reserved_ PVOID CallbackParam
            );

        MiniDumpWriteDumpPfn pMiniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpPfn>(::GetProcAddress(hToolHelpModule, "MiniDumpWriteDump"));
        if( pMiniDumpWriteDump != NULL )
        {
            WCHAR wstrExecutableName[MAX_PATH];
            XSF::GetExecutableName( wstrExecutableName, _countof(wstrExecutableName) );

            SYSTEMTIME stLocalTime;
            GetLocalTime( &stLocalTime );

            WCHAR wstrMiniDumpPath[MAX_PATH];
            swprintf_s( wstrMiniDumpPath, L"d:\\%s-%04d%02d%02d-%02d%02d%02d.dmp",
                wstrExecutableName, 
                stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond );

            HANDLE hDumpFile = CreateFile( wstrMiniDumpPath, GENERIC_READ|GENERIC_WRITE, 
                        FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

            HANDLE hProcess = ::GetCurrentProcess();
            DWORD dProcessId = ::GetCurrentProcessId();

            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;       
            ZeroMemory( &exceptionInfo, sizeof( MINIDUMP_EXCEPTION_INFORMATION ) );       
            exceptionInfo.ThreadId = GetCurrentThreadId();       
            exceptionInfo.ExceptionPointers = pException;       
            exceptionInfo.ClientPointers = FALSE;

            bMiniDumpResult = pMiniDumpWriteDump( hProcess, dProcessId, 
                hDumpFile,
                MiniDumpWithFullMemory,
                &exceptionInfo, NULL, NULL );

            ::CloseHandle( hDumpFile );
            ::FreeLibrary( hToolHelpModule );

            XSF::DebugPrint( L"XSF ExceptionHandler, MiniDumpWriteDump Result: %d\n", bMiniDumpResult );
            return EXCEPTION_EXECUTE_HANDLER;
        }

        ::FreeLibrary( hToolHelpModule );
    }

    XSF::DebugPrint( L"XSF ExceptionHandler, Unable to find MiniDumpWriteDump proceedure\n" );
    return EXCEPTION_CONTINUE_SEARCH;;
}

#endif //_TITLE
#endif //_XBOX_ONE
//--------------------------------------------------------------------------------------
// Name: SetEnableLogging
// Desc: Enables or disables file logging
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::SetTestOptions( void (*pfnLog)( const char* pText ), void (*pfnStopOnError)(), BOOL enableBreakOnError )
{
    Details::g_pLoggingFunctionIntercept = pfnLog;
    Details::g_pStopOnErrorFunctionIntercept = pfnStopOnError;
    Details::g_breakOnError = enableBreakOnError;
}

//--------------------------------------------------------------------------------------
// Name: SetExitOnError
// Desc: Sets whether the sample exits on errors
//--------------------------------------------------------------------------------------
void XSF::SetExitOnError( BOOL exitOnError )
{
   Details::g_exitOnError = exitOnError;
}

//--------------------------------------------------------------------------------------
// Name: DefaultLogPrintError
// Desc: Print message
//--------------------------------------------------------------------------------------
static
void DefaultLogPrintMessage( const char* pText )
{
    using namespace XSF::Details;

    // On the first run try opening a log file
    static BOOL bFirstTime = TRUE;
    if( bFirstTime )
    {
        bFirstTime = FALSE;

        _snwprintf_s( g_checkpointFileName, _countof( g_checkpointFileName ), _TRUNCATE, L"%s\\%s", XSF::GetStorageFileRoot(), CHECKPOINT_FILE_NAME );
        
        // Try opening a file
        g_pCheckpointLogFile = nullptr;
        _wfopen_s( &g_pCheckpointLogFile, g_checkpointFileName, L"wt" );

        if( g_pCheckpointLogFile )
        {
            XSF::DebugPrint( L"Opened log file %s\n", g_checkpointFileName );
        } else
        {
            XSF::DebugPrint( L"Failed to open log file %s\n", g_checkpointFileName );
        }
    }

    // Write into the log file if enabled
    if( g_pCheckpointLogFile )
    {
        fputs( pText, g_pCheckpointLogFile );
    }
}

//--------------------------------------------------------------------------------------
// Name: DefaultLogPrintError
// Desc: Act on an unrecoverable error
//--------------------------------------------------------------------------------------
static
void DefaultLogPrintError()
{
#if defined( _XBOX_ONE )
    // Thrown exception will hang the sample sometimes so using a slightly different exit mechanism
    // This behaviour may change in the future
    Windows::ApplicationModel::Core::CoreApplication::Exit();
    Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
#else
    exit( -1 );
#endif
}

//--------------------------------------------------------------------------------------
// Name: SetEnableLogging
// Desc: Enables or disables file logging
//--------------------------------------------------------------------------------------
void XSF::SetEnableLogging( BOOL enable, CheckPoint checkpointMin )
{
    XSF::Details::g_checkpointMin = checkpointMin;
    if( enable )
    {
        SetTestOptions( DefaultLogPrintMessage, DefaultLogPrintError, FALSE );
    } else
    {
        SetTestOptions( nullptr, nullptr, TRUE );
    }
}

//--------------------------------------------------------------------------------------
// Name: SetCheckpointMin
// Desc: Sets minimum checkpoint to log
//--------------------------------------------------------------------------------------
void XSF::SetCheckpointMin( CheckPoint checkpointMin )
{
    XSF::Details::g_checkpointMin = checkpointMin;
}

//--------------------------------------------------------------------------------------
// Name: CloseLoggingFile
// Desc: Close logging file, if it's opened
//--------------------------------------------------------------------------------------
void XSF::CloseLoggingFile()
{
    using namespace XSF::Details;
    if( g_pCheckpointLogFile )
    {
        fflush( g_pCheckpointLogFile );
        fclose( g_pCheckpointLogFile );
        g_pCheckpointLogFile = nullptr;
    }
}

//--------------------------------------------------------------------------------------
// Name: CrossCheckpoint
// Desc: Outputs a file that can be read by automated test process
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::CrossCheckpoint( CheckPoint cp, const char* customCheckpoint, float customValue )
{
    using namespace Details;

    XSF_ASSERT( cp >= CP_MAIN && cp < CP_NUM );

    // Update timings for this checkpoint
    QueryPerformanceCounter( (LARGE_INTEGER*)&g_checkpointTimings[ cp ] );

    UINT64 ticksPerSecond;
    QueryPerformanceFrequency( (LARGE_INTEGER*)&ticksPerSecond );

    const UINT64 deltaFromMain = g_checkpointTimings[ cp ] - g_checkpointTimings[ CP_MAIN ];
    g_checkpointDeltaFromStart[ cp ] = static_cast< float >( deltaFromMain ) / static_cast< float >( ticksPerSecond );

    static const char* strings[] =
    {
        "CP_MAIN",                      // CP_MAIN,
        "CP_BEFORE_D3D_INIT",           // CP_BEFORE_D3D_INIT,
        "CP_IN_FRAMEWORK_INIT",         // CP_IN_FRAMEWORK_INIT,
        "CP_IN_TICK",                   // CP_IN_TICK
        "CP_BEFORE_FRAME_UPDATE",       // CP_BEFORE_FRAME_UPDATE,
        "CP_AFTER_FRAME_UPDATE",        // CP_AFTER_FRAME_UPDATE,
        "CP_BEFORE_FRAME_RENDER",       // CP_BEFORE_FRAME_RENDER,
        "CP_AFTER_FRAME_RENDER",        // CP_AFTER_FRAME_RENDER,
        "CP_IN_FRAMEWORK_SHUTDOWN",     // CP_IN_FRAMEWORK_SHUTDOWN,
        "CP_IN_FRAMEWORK_SUSPEND",      // CP_IN_FRAMEWORK_SUSPEND,
        "CP_IN_FRAMEWORK_RESUME",       // CP_IN_FRAMEWORK_RESUME
        "CP_CUSTOM",                    // CP_CUSTOM
    };

    // Write into the log file if enabled
    if( g_pCheckpointLogFile && cp >= g_checkpointMin )
    {
        if( customCheckpoint )
        {
            XSF::DebugPrint( "%s,%f", customCheckpoint, customValue );
        }
        else
        {
            XSF::DebugPrint( "%s,%f", strings[ cp ], g_checkpointDeltaFromStart[ cp ] );
        }

        // Print extra type-specific information, e.g. full D3D pipeline stats from SampleFramework::GetStatistics()
        switch( cp )
        {
        case CP_IN_TICK:
            XSF::DebugPrint( ",%d", SampleFramework::GetStatistics().m_frameNumber );
            break;
        }

        XSF::DebugPrint( "\n" );
    }
    else
    {
        // Output lower frequency checkpoints always
        if( (cp < CP_IN_TICK || cp > CP_AFTER_FRAME_RENDER) && cp != CP_CUSTOM )
        {
            XSF::DebugPrint( "%s,%f\n", strings[ cp ], g_checkpointDeltaFromStart[ cp ] );
		}
    }
}

//--------------------------------------------------------------------------------------
// Name: Allocate
// Desc: Allocates graphics memory
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
#if (WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE)
void XSF::GpuMemory::Allocate(size_t SizeBytes, UINT Flags, SIZE_T AlignmentBytes, UINT64 DesiredGpuVirtualAddress)
{
    Free();

    m_sizeBytes = XSF::NextMultiple(static_cast<SIZE_T>(SizeBytes), AlignmentBytes);
#ifdef XSF_USE_DX_12_0
    const UINT32 AllocationType = MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT;
    UINT32 Protection = PAGE_READWRITE;
    switch (Flags)
    {
    case D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT:
        Protection |= PAGE_GPU_COHERENT;
        break;
    case D3D11_GRAPHICS_MEMORY_ACCESS_CPU_WRITECOMBINE_NONCOHERENT:
        Protection |= PAGE_WRITECOMBINE;
        break;
    case D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_NONCOHERENT_GPU_READONLY:
        Protection |= PAGE_GPU_READONLY;
        break;
    default:
        XSF_ERROR_IF_FAILED(E_INVALIDARG);
    }
    m_pGpuMemory = VirtualAlloc(
        reinterpret_cast<LPVOID>(DesiredGpuVirtualAddress),
        m_sizeBytes,
        AllocationType,
        Protection);
#else
    XSF_ERROR_IF_FAILED(D3DAllocateGraphicsMemory(m_sizeBytes, AlignmentBytes, DesiredGpuVirtualAddress, Flags, &m_pGpuMemory));
#endif
    ZeroMemory(m_pGpuMemory, m_sizeBytes);
}

void XSF::GpuMemory::Free()
{
#ifdef XSF_USE_DX_12_0
    VirtualFree(m_pGpuMemory, m_sizeBytes, MEM_RELEASE);
#else
    D3DFreeGraphicsMemory(m_pGpuMemory);
#endif
    m_pGpuMemory = nullptr;
}
#endif


#ifdef XSF_USE_DX_12_0
#else
#if defined(_XBOX_ONE) && defined(_TITLE)
//--------------------------------------------------------------------------------------
// Name: SyncRTVtoSRV
// Desc: Sync operations for fast semantics when switching a RenderTarget to a ShaderResourceView
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::SyncRTVtoSRV( XSF::D3DDeviceContext* const pCtx, UINT rtIndex, ID3D11Resource* const pResource )
{
    UNREFERENCED_PARAMETER( pResource );
    static const D3D11_FLUSH s_coherencyFlush[] =
    {
        D3D11_FLUSH_ENSURE_CB0_COHERENCY,
        D3D11_FLUSH_ENSURE_CB1_COHERENCY,
        D3D11_FLUSH_ENSURE_CB2_COHERENCY,
        D3D11_FLUSH_ENSURE_CB3_COHERENCY,
        D3D11_FLUSH_ENSURE_CB4_COHERENCY,
        D3D11_FLUSH_ENSURE_CB5_COHERENCY,
        D3D11_FLUSH_ENSURE_CB6_COHERENCY,
        D3D11_FLUSH_ENSURE_CB7_COHERENCY,
    };

    // _FLUSH_AND_INV_CB_PIXEL_DATA is an instantianous end of pipe signal to start flushing
    pCtx->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_FLUSH_AND_INV_CB_PIXEL_DATA );
    // PS Partial flush will block any next PS submissions, thus the correct order is to
    // first trigger flush and invalidation of CB data and then block until done
    pCtx->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_PS_PARTIAL_FLUSH );

    // CB coherency is required to make sure the CB caches finished 
    // writing memory of the Render Target rtIndex before its use as a SRV starts
    pCtx->FlushGpuCacheRange( s_coherencyFlush[rtIndex] | D3D11_FLUSH_DEFAULT, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL );
}

//--------------------------------------------------------------------------------------
// Name: SyncDBtoSRV
// Desc: Sync operations for fast semantics when switching a DepthBuffer to a ShaderResourceView
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::SyncDBtoSRV( XSF::D3DDeviceContext* const pCtx, ID3D11Resource* const pResource )
{
    UNREFERENCED_PARAMETER( pResource );

    pCtx->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_DB_CACHE_FLUSH_AND_INV );
    pCtx->GpuSendPipelinedEvent( D3D11X_GPU_PIPELINED_EVENT_PS_PARTIAL_FLUSH );
    pCtx->FlushGpuCacheRange( D3D11_FLUSH_ENSURE_DB_COHERENCY | D3D11_FLUSH_DEFAULT, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL );
}
#endif


//--------------------------------------------------------------------------------------
// Name: Destroy
// Desc: Releases the internal buffer object
//-------------------------------------------------------------------------------------
void  XSF::ImmutableBuffer::Destroy()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    VirtualFree( m_pPlacementBuffer, 0, MEM_RELEASE );
#endif
}

//--------------------------------------------------------------------------------------
// Name: Create
// Desc: Creates & initializes the buffer
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::ImmutableBuffer::Create( D3DDevice* pDev, D3D11_BIND_FLAG bindFlags, UINT size, void* pInitialData ) 
{
    XSF_ASSERT( pInitialData != nullptr );

    D3D11_BUFFER_DESC bufDesc = { 0 };
    bufDesc.BindFlags = bindFlags;
    bufDesc.ByteWidth = size;
    bufDesc.Usage = D3D11_USAGE_IMMUTABLE;

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_pPlacementBuffer = static_cast<BYTE*>( VirtualAlloc( nullptr, size,
        MEM_LARGE_PAGES | MEM_GRAPHICS | MEM_RESERVE | MEM_COMMIT,
        PAGE_WRITECOMBINE | PAGE_READWRITE ));

    XSF_ERROR_IF_FAILED( pDev->CreatePlacementBuffer( &bufDesc, nullptr, m_buffer.ReleaseAndGetAddressOf() ) );
    memcpy( m_pPlacementBuffer, pInitialData, size );
#else
    D3D11_SUBRESOURCE_DATA vbInitData = { pInitialData, 0, 0 };
    XSF_ERROR_IF_FAILED( pDev->CreateBuffer( &bufDesc, &vbInitData, m_buffer.ReleaseAndGetAddressOf() ) );
#endif
}

//--------------------------------------------------------------------------------------
// Name: DynamicBuffer
// Desc: Constructs an empty dynamic buffer
//-------------------------------------------------------------------------------------
XSF::DynamicBuffer::DynamicBuffer() : m_mappedOnContext( nullptr ),
                                      m_bufferTailOffset( 0 ),
                                      m_numBytesMapped( 0 ),
                                      m_bufferSize( 0 ),
                                      m_alwaysDiscard( FALSE )
{
}

//--------------------------------------------------------------------------------------
// Name: DynamicBuffer
// Desc: Constructs an empty dynamic buffer
//-------------------------------------------------------------------------------------
XSF::DynamicBuffer::~DynamicBuffer()
{
    Destroy();
}

//--------------------------------------------------------------------------------------
// Name: DynamicBuffer
// Desc: Constructs an empty dynamic buffer
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::DynamicBuffer::Create( D3DDevice* pDev, D3D11_BIND_FLAG bindFlags, UINT size )
{
    VERBOSEATGPROFILETHIS;

    D3D11_BUFFER_DESC bufDesc = { 0 };
    bufDesc.BindFlags = bindFlags;
    bufDesc.ByteWidth = size;
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    XSF_RETURN_IF_FAILED( pDev->CreateBuffer( &bufDesc, nullptr, m_spBuffer.ReleaseAndGetAddressOf() ) );
    
    m_bufferTailOffset = 0;
    m_numBytesMapped = 0;
    m_bufferSize = size;
    m_alwaysDiscard = FALSE;

#ifndef _XBOX_ONE
    // determine if hw supports map no overwrite
    if( bindFlags & ( D3D11_BIND_CONSTANT_BUFFER | D3D11_BIND_SHADER_RESOURCE ) )
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS options;
        XSF_RETURN_IF_FAILED( pDev->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS) ) );
        if( ( ( bindFlags & D3D11_BIND_CONSTANT_BUFFER ) && !options.MapNoOverwriteOnDynamicConstantBuffer ) ||
            ( ( bindFlags & D3D11_BIND_SHADER_RESOURCE ) && !options.MapNoOverwriteOnDynamicBufferSRV ) )
        {
            m_alwaysDiscard = TRUE;
        }
    }
#endif

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Destroy
// Desc: Release the internal buffer object
//-------------------------------------------------------------------------------------
void  XSF::DynamicBuffer::Destroy()
{
    m_spBuffer.Reset();
    m_bufferTailOffset = 0;
    m_numBytesMapped = 0;
    m_bufferSize = 0;
}

//--------------------------------------------------------------------------------------
// Name: Map
// Desc: Returns a pointer to mapped buffer memory
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::DynamicBuffer::Map( D3DDeviceContext* pCtx, UINT numBytesToMap, void** ppData, UINT* pOffset )
{
    VERBOSEATGPROFILETHIS;

    XSF_ASSERT( !m_mappedOnContext );
    XSF_ASSERT( pCtx );
    XSF_ASSERT( ppData );
    XSF_ASSERT( numBytesToMap <= m_bufferSize );

    m_mappedOnContext = pCtx;

    // The first Map on a deferred context should be with a DISCARD. There is no easy way to know
    // which one is the first call so just discard anyway
    const BOOL bDeferred = pCtx->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED;

    if( m_bufferTailOffset + numBytesToMap > m_bufferSize ||
        bDeferred || m_alwaysDiscard )
    {
        D3D11_MAPPED_SUBRESOURCE data;
        XSF_RETURN_IF_FAILED( pCtx->Map( m_spBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data ) );

        *ppData = data.pData;
        m_bufferTailOffset = 0;
    } else
    {
        D3D11_MAPPED_SUBRESOURCE data;
        XSF_RETURN_IF_FAILED( pCtx->Map( m_spBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &data ) );
        
        *ppData = (BYTE*)data.pData + m_bufferTailOffset;
    }

    if( pOffset )
        *pOffset = m_bufferTailOffset;

    m_numBytesMapped = numBytesToMap;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Unmap
// Desc: Unmaps buffer memory
//-------------------------------------------------------------------------------------
void XSF::DynamicBuffer::Unmap( UINT numBytesUsed )
{
    VERBOSEATGPROFILETHIS;

    XSF_ASSERT( m_mappedOnContext );

    if( !numBytesUsed )
        numBytesUsed = m_numBytesMapped;

    m_bufferTailOffset += numBytesUsed;

    m_mappedOnContext->Unmap( m_spBuffer, 0 );
    m_mappedOnContext = nullptr;
}

//--------------------------------------------------------------------------------------
// Name: GetBuffer
// Desc: Returns the buffer
//-------------------------------------------------------------------------------------
ID3D11Buffer* const&   XSF::DynamicBuffer::GetBuffer() const
{
    return *m_spBuffer.GetAddressOf();
}

//--------------------------------------------------------------------------------------
// Name: GetNumBytesLastMapped
// Desc: Returns how many bytes was mapped last
//-------------------------------------------------------------------------------------
UINT XSF::DynamicBuffer::GetNumBytesLastMapped() const
{
    return m_numBytesMapped;
}

//--------------------------------------------------------------------------------------
// Name: GetTailOffset
// Desc: Returns the current tail offset
//-------------------------------------------------------------------------------------
UINT XSF::DynamicBuffer::GetTailOffset() const
{
    return m_bufferTailOffset;
}

//--------------------------------------------------------------------------------------
// Name: RoundRobinBuffer
// Desc: Constructs an empty round robin buffer
//-------------------------------------------------------------------------------------
XSF::RoundRobinBuffer::RoundRobinBuffer() : 
#ifdef MAP_USAGE_DEFAULT_PLACEMENT
    m_pPlacementBuffer( nullptr ),
    byteWidth( 0 ),
#endif
    m_nContexts( 0 ),
    m_nObjects( 0 ),
    m_nBuffering( 0 )
{
}


//--------------------------------------------------------------------------------------
// Name: RoundRobinBuffer
// Desc: Destructor
//-------------------------------------------------------------------------------------
XSF::RoundRobinBuffer::~RoundRobinBuffer()
{
    Destroy();
}

//--------------------------------------------------------------------------------------
// Name: Destroy
// Desc: Releases the internal buffer object
//-------------------------------------------------------------------------------------
void  XSF::RoundRobinBuffer::Destroy()
{
#ifdef MAP_USAGE_DEFAULT_PLACEMENT
    VirtualFree( m_pPlacementBuffer, 0, MEM_RELEASE );
#endif
}

//--------------------------------------------------------------------------------------
// Name: CreateBuffers
// Desc: Creates buffers
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::RoundRobinBuffer::CreateBuffers( D3DDevice* pDev, D3D11_BUFFER_DESC* pDesc ) 
{
    UINT numBuffers = m_nContexts * m_nObjects * m_nBuffering;
#if defined(MAP_USAGE_DEFAULT) || defined(MAP_USAGE_DEFAULT_PLACEMENT)
    XSF_ASSERT( pDesc->Usage == D3D11_USAGE_DEFAULT );
#endif

#ifdef MAP_USAGE_DEFAULT_PLACEMENT
    byteWidth = pDesc->ByteWidth;

    #ifdef MAP_PLACEMENT_BUFFERS_CACHE_LINE_ALIGNMENT_PACK
    // Force cache line alignment and packing to avoid cache flushing on Unmap in RoundRobinBuffers
    byteWidth  = static_cast<UINT>( NextMultiple( byteWidth, BUFFER_CACHE_LINE_SIZE ) );
    #endif
    
    m_pPlacementBuffer = static_cast<BYTE*>( VirtualAlloc( nullptr, numBuffers * byteWidth, 
        MEM_LARGE_PAGES | MEM_GRAPHICS | MEM_RESERVE | MEM_COMMIT, 
        PAGE_WRITECOMBINE | PAGE_READWRITE ));

    #ifdef MAP_PLACEMENT_BUFFERS_CACHE_LINE_ALIGNMENT_PACK
    XSF_ASSERT( ( BytePtrToUint64( m_pPlacementBuffer ) & ( BUFFER_CACHE_LINE_SIZE - 1 ) ) == 0 );
    #endif

    XSF_ERROR_IF_FAILED( pDev->CreatePlacementBuffer( pDesc, nullptr, &m_buffer[0] ) );
#else
    for ( UINT i = 0; i < numBuffers; ++i )
    {
        XSF_ERROR_IF_FAILED( pDev->CreateBuffer( pDesc, nullptr, &m_buffer[i] ) );
    }
#endif
}

//--------------------------------------------------------------------------------------
// Name: SetNumBuffers
// Desc: Configures RoundRobinBuffer to handle XYZ buffers
// nContexts - number of contexts requesting a buffer per frame
// nObjects - number of objects requesting this buffer per frame
// nMapsPerFrame - number of times a buffer is mapped per frame
// nFramesBuffering - number of frames to buffer for ~ NUMBER OF BACKBUFFERS in swap chain
//-------------------------------------------------------------------------------------
void XSF::RoundRobinBuffer::SetNumBuffers( UINT nContexts, UINT nObjects, UINT nMapsPerFrame, UINT nFramesBuffering )
{
#if defined(MAP_USAGE_DEFAULT) || defined(MAP_USAGE_DEFAULT_PLACEMENT)
    m_nContexts = nContexts;
    m_nObjects = nObjects;
    m_nBuffering = nMapsPerFrame * nFramesBuffering;

    #ifdef MAP_USAGE_DEFAULT_PLACEMENT
    m_buffer.resize( 1 );
    #else
    m_buffer.resize( nContexts * nObjects * m_nBuffering );
    #endif
    m_index.resize( nContexts * nObjects, 0 );
#else
    UNREFERENCED_PARAMETER(nContexts);
    UNREFERENCED_PARAMETER(nObjects);
    UNREFERENCED_PARAMETER(nMapsPerFrame);
    UNREFERENCED_PARAMETER(nFramesBuffering);
    m_nContexts = 1;
    m_nObjects = 1;
    m_nBuffering = 1;
    m_buffer.resize( 1 );
#endif
}
#endif // XSF_USE_DX_12_0


//--------------------------------------------------------------------------------------
// Name: XSetThreadName
// Desc: Set the name of the given thread so that it will show up in the Threads Window
//       in Visual Studio and in PIX timing captures.
//--------------------------------------------------------------------------------------
void XSF::XSetThreadName( DWORD dwThreadID, LPCSTR strThreadName )
{
    // Structure used to name threads
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;     // must be 0x1000
        LPCSTR szName;    // pointer to name (in user address space)
        DWORD dwThreadID; // thread ID (-1 = caller thread)
        DWORD dwFlags;    // reserved for future use, must be zero
    } THREADNAME_INFO;


    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = strThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException( 0x406D1388, 0, sizeof( info ) / sizeof( DWORD ), (ULONG_PTR*)&info );
    }
    __except( GetExceptionCode()== 0x406D1388 ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER ) 
    {
        __noop;
    }
}
