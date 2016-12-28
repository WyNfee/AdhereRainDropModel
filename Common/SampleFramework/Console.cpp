//--------------------------------------------------------------------------------------
// Console.cpp
//
// Renders a simple on screen console where you can output text information
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "BitmapFont.h"
#include "Console.h"

using namespace XboxSampleFramework;

//--------------------------------------------------------------------------------------
// Name: Console()
// Desc: Initialize variables
//--------------------------------------------------------------------------------------
Console::Console()
{
    m_Buffer = NULL;
    m_Lines = NULL;
    m_bOutputToDebugChannel = FALSE;
	m_ScrollOffset = 0;
    m_pTexture = NULL;
    m_pRV = NULL;
    m_pRT = NULL;
    m_pSamplerState = NULL;
    m_bDirty = TRUE;        
    m_bNeedReleaseDraw = FALSE;

    InitializeCriticalSection( &m_CsConsoleBuf );
}

//--------------------------------------------------------------------------------------
// Name: ~Console()
// Desc: Destroy object
//--------------------------------------------------------------------------------------
Console::~Console()
{
    Destroy();

    DeleteCriticalSection( &m_CsConsoleBuf );
}

//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Initialize the class, create necessary resources
//--------------------------------------------------------------------------------------
HRESULT Console::Create( D3DDevice* pDevice, 
                         BitmapFont *pFont, 
                         UINT Width, UINT Height, 
                         DWORD textColor, DWORD backColor,
                         UINT nLines, Draw* pDraw )
{
	D3D11_TEXTURE2D_DESC descTex;
    ZeroMemory( &descTex, sizeof(descTex) );
    descTex.Width = m_width = Width;
    descTex.Height = m_height = Height;
    descTex.MipLevels = 1;      
    descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  
	descTex.SampleDesc.Count = 1;
    descTex.SampleDesc.Quality = 0;
	descTex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    descTex.ArraySize = 1;
    XSF_ERROR_IF_FAILED( pDevice->CreateTexture2D( &descTex, NULL, &m_pTexture ) );

    D3D11_SHADER_RESOURCE_VIEW_DESC descRV;
    ZeroMemory( &descRV, sizeof(descRV) );
    descRV.Format = descTex.Format;
    descRV.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    descRV.Texture2D.MipLevels = descTex.MipLevels;
    descRV.Texture2D.MostDetailedMip = 0;
    XSF_ERROR_IF_FAILED( pDevice->CreateShaderResourceView( m_pTexture, &descRV, &m_pRV ) )

    D3D11_RENDER_TARGET_VIEW_DESC descRTV;
    ZeroMemory( &descRTV, sizeof(descRTV) );
    descRTV.Format = descTex.Format;
    descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    XSF_ERROR_IF_FAILED( pDevice->CreateRenderTargetView( m_pTexture, &descRTV, &m_pRT ) )
    
    D3D11_SAMPLER_DESC descSam;
    ZeroMemory( &descSam, sizeof(descSam) );
    descSam.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    descSam.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    descSam.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    descSam.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;        
    descSam.MipLODBias = 0;
    descSam.MaxAnisotropy = 1;
    descSam.ComparisonFunc = D3D11_COMPARISON_NEVER;
    descSam.BorderColor[ 0 ] = 0.0f;
    descSam.BorderColor[ 1 ] = 0.0f;
    descSam.BorderColor[ 2 ] = 0.0f;
    descSam.BorderColor[ 3 ] = 0.0f;
    descSam.MinLOD = -D3D11_FLOAT32_MAX;
    descSam.MaxLOD = D3D11_FLOAT32_MAX;
    XSF_ERROR_IF_FAILED( pDevice->CreateSamplerState( (D3D11_SAMPLER_DESC*)&descSam, &m_pSamplerState ) );

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;        
    XSF_ERROR_IF_FAILED( pDevice->CreateDepthStencilState( &dsDesc, &m_pDSState ) );

    if ( pDraw == NULL )
    {
        pDraw = new Draw();
        pDraw->Initialize( pDevice );
        m_bNeedReleaseDraw = TRUE;
    }
	m_pDraw = pDraw;

    // Calculate the safe area
    UINT uiSafeAreaPct = SAFE_AREA_PCT_HDTV;

    m_xSafeArea = ( Width * uiSafeAreaPct ) / 100;
    m_ySafeArea = ( Height * uiSafeAreaPct ) / 100;

    m_xSafeAreaOffset = ( Width - m_xSafeArea ) / 2;
    m_ySafeAreaOffset = ( Height - m_ySafeArea ) / 2;       
    
    // Save the font
    m_pFont = pFont;    

    // Save the colors
    m_backColor = backColor;
    m_textColor = textColor;

    // Calculate the number of lines on the screen
    FLOAT fCharWidth, fCharHeight;
    m_pFont->GetTextExtent( L"i", &fCharWidth, &fCharHeight, FALSE );

    m_ScreenHeight = ( UINT )( m_ySafeArea / fCharHeight );
    m_ScreenWidth = ( UINT )( m_xSafeArea / fCharWidth );

    m_ScreenHeightVirtual = __max( m_ScreenHeight, nLines );

    m_lineHeight = fCharHeight;

    // Allocate memory to hold the lines
    m_Buffer = new WCHAR[ m_ScreenHeightVirtual * ( m_ScreenWidth + 1 ) ];
    m_Lines = new WCHAR*[ m_ScreenHeightVirtual ];

    // Set the line pointers as indexes into the buffer
    for( UINT i = 0; i < m_ScreenHeightVirtual; i++ )
    {
        m_Lines[ i ] = m_Buffer + ( m_ScreenWidth + 1 ) * i;
    }   

    // Clear the screen
    Clear();

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Clear()
// Desc: Clear all text in the console
//--------------------------------------------------------------------------------------
VOID Console::Clear()
{
    m_CurLine = 0;
    m_CurLineLength = 0;
    ZeroMemory( m_Buffer, m_ScreenHeightVirtual * ( m_ScreenWidth + 1 ) * sizeof( WCHAR ) );
    
    m_bDirty = TRUE;
}

//--------------------------------------------------------------------------------------
// Name: IncrementLine()
// Desc: Skip to the next line
//--------------------------------------------------------------------------------------
VOID Console::IncrementLine()
{
    m_CurLine = ( m_CurLine + 1 ) % m_ScreenHeightVirtual;
    m_CurLineLength = 0;
    ZeroMemory( m_Lines[m_CurLine], ( m_ScreenWidth + 1 ) * sizeof( WCHAR ) );
}

//--------------------------------------------------------------------------------------
// Name: ScrollUp()
// Desc: Scroll the text window upwards
//--------------------------------------------------------------------------------------
VOID Console::ScrollUp( INT nLines )
{
    // Scroll exactly one page height if requested
    if( nLines >= PAGE_UP )
    {
        nLines = ( INT )m_ScreenHeight;
    }
    else if( nLines <= PAGE_DOWN )
    {
        nLines = -( INT )m_ScreenHeight;
    }

    m_ScrollOffset += nLines;

    m_ScrollOffset %= m_ScreenHeightVirtual;

    m_bDirty = TRUE;
}

//--------------------------------------------------------------------------------------
// Name: Destroy()
// Desc: Tear everything down
//--------------------------------------------------------------------------------------
VOID Console::Destroy()
{
    // Delete the memory we've allocated
    XSF_SAFE_DELETE_ARRAY( m_Lines );
    XSF_SAFE_DELETE_ARRAY( m_Buffer );    

    if( m_bNeedReleaseDraw )
        XSF_SAFE_DELETE( m_pDraw );

    XSF_SAFE_RELEASE( m_pTexture );
    XSF_SAFE_RELEASE( m_pRV );
    XSF_SAFE_RELEASE( m_pRT );
    XSF_SAFE_RELEASE( m_pSamplerState );
    XSF_SAFE_RELEASE( m_pDSState );
}

//--------------------------------------------------------------------------------------
// Name: RenderToScreen()
// Desc: Render the console texture onto the screen using current view port
//--------------------------------------------------------------------------------------
VOID Console::RenderToScreen( D3DDeviceContext* pCtx )
{
	RenderToTexture( pCtx );        

    ID3D11DepthStencilState* pDepthStencilState;
    UINT stencilRef;
    pCtx->OMGetDepthStencilState( &pDepthStencilState, &stencilRef );   // save depth stencil state
    pCtx->OMSetDepthStencilState( m_pDSState, 0 );                      // disable depth writing
	
	m_pDraw->Begin( pCtx );
	m_pDraw->TexturedQuad( m_pRV, m_pSamplerState );
	m_pDraw->End();

    pCtx->OMSetDepthStencilState( pDepthStencilState, stencilRef );     // restore depth stencil state
    XSF_SAFE_RELEASE( pDepthStencilState );    
}

//--------------------------------------------------------------------------------------
// Name: RenderToTexture()
// Desc: Render the console texture, it is safe to call this every frame
//--------------------------------------------------------------------------------------
VOID Console::RenderToTexture( D3DDeviceContext* pCtx )
{
    // Only update the console texture if there is any updated text
    if ( !m_bDirty )
        return;
    
    // Save the render target
    ID3D11RenderTargetView* pRT = NULL;
    ID3D11DepthStencilView* pDS = NULL;
    pCtx->OMGetRenderTargets( 1, &pRT, &pDS );    

    // Save the view port
    UINT numViewports = 1;
    D3D11_VIEWPORT viewport;
    pCtx->RSGetViewports( &numViewports, &viewport );

    // Set the view port for rendering our console texture
    D3D11_VIEWPORT newViewport;
    newViewport.TopLeftX = 0;
    newViewport.TopLeftY = 0;
    newViewport.Width = (FLOAT)m_width;
    newViewport.Height = (FLOAT)m_height;
    newViewport.MinDepth = 0;
    newViewport.MaxDepth = 1;
    pCtx->RSSetViewports( 1, &newViewport );    

    // Set the render target for rendering our console texture and clear it with background color
    const FLOAT color[4] = {  ( ( m_backColor & 0x00ff0000 ) >> 16L ) / 255.0F, 
                              ( ( m_backColor & 0x0000ff00 ) >> 8L ) / 255.0F,
                              ( ( m_backColor & 0x000000ff ) >> 0L ) / 255.0F,
                              ( ( m_backColor & 0xff000000 ) >> 24L ) / 255.0F };
    pCtx->OMSetRenderTargets( 1, &m_pRT, NULL );
    pCtx->ClearRenderTargetView( m_pRT, color );    

    EnterCriticalSection( &m_CsConsoleBuf );

    // The top line
    UINT nTextLine = ( m_CurLine - m_ScreenHeight + m_ScreenHeightVirtual - m_ScrollOffset + 1 ) % m_ScreenHeightVirtual;

    UINT safeArea = m_pFont->GetSafeAreaInPixels();
    m_pFont->ResetWindow( 0 ); // we handle screen safe area ourselves, so we disable the screen safe area setting in the font itself    
    m_pFont->Begin( pCtx );

    for( UINT nScreenLine = 0; nScreenLine < m_ScreenHeight; nScreenLine++ )
    {
        m_pFont->DrawText( ( FLOAT )( m_xSafeAreaOffset ),
                         ( FLOAT )( m_ySafeAreaOffset + m_lineHeight * nScreenLine ),
                         m_textColor, m_Lines[nTextLine] );

        nTextLine = ( nTextLine + 1 ) % m_ScreenHeightVirtual;
    }

    m_pFont->End();
    m_pFont->ResetWindow( safeArea ); // since we share the font that's passed in, we restore the screen safe area setting for the font

    LeaveCriticalSection( &m_CsConsoleBuf );

    // Set the render target back
    pCtx->OMSetRenderTargets( 1, &pRT, pDS );

    // Set the view port back
    pCtx->RSSetViewports( 1, &viewport );

    XSF_SAFE_RELEASE( pRT );
    XSF_SAFE_RELEASE( pDS );

    m_bDirty = FALSE;
}

//--------------------------------------------------------------------------------------
// Name: Add( CHAR )
// Desc: Convert ANSI to WCHAR and add to the current line
//--------------------------------------------------------------------------------------
VOID Console::Add( CHAR ch )
{
    WCHAR wch;

    INT ret = MultiByteToWideChar( CP_ACP,        // ANSI code page
                                   0,             // No flags
                                   &ch,           // Character to convert
                                   1,             // Convert one byte
                                   &wch,          // Target wide character buffer
                                   1 );           // One wide character

    XSF_ASSERT( ret == 1 );
    (void)ret;

    Add( wch );
}



//--------------------------------------------------------------------------------------
// Name: Add( WCHAR )
// Desc: Add a wide character to the current line
//--------------------------------------------------------------------------------------
VOID Console::Add( WCHAR wch )
{
    // If this is a newline, just increment lines and move on
    if( wch == L'\n' )
    {
        IncrementLine();
        return;
    }

    BOOL bIncrementLine = FALSE;  // Whether to wrap to the next line

    if( m_CurLineLength == m_ScreenWidth )
    {
        bIncrementLine = TRUE;
    }
    else
    {
        // Try to append the character to the line
        m_Lines[ m_CurLine ][ m_CurLineLength ] = wch;

        if( m_pFont->GetTextWidth( m_Lines[ m_CurLine ] ) > m_xSafeArea )
        {
            // The line is too long, we need to wrap the character to the next line
            m_Lines[ m_CurLine][ m_CurLineLength ] = L'\0';
            bIncrementLine = TRUE;
        }
    }

    // If we need to skip to the next line, do so
    if( bIncrementLine )
    {
        IncrementLine();
        m_Lines[ m_CurLine ][0] = wch;
    }

    m_CurLineLength++;
}

//--------------------------------------------------------------------------------------
// Name: Format()
// Desc: Output a variable argument list using a format string
//--------------------------------------------------------------------------------------
VOID Console::Format( _In_z_ _Printf_format_string_ LPCSTR strFormat, ... )
{
    va_list pArgList;
    va_start( pArgList, strFormat );    
    FormatV( strFormat, pArgList );    
    va_end( pArgList );
}

VOID Console::Format( _In_z_ _Printf_format_string_ LPCWSTR wstrFormat, ... )
{
    va_list pArgList;
    va_start( pArgList, wstrFormat );
    FormatV( wstrFormat, pArgList );
    va_end( pArgList );
}

//--------------------------------------------------------------------------------------
// Name: FormatV()
// Desc: Output a va_list using a format string
//--------------------------------------------------------------------------------------
VOID Console::FormatV( _In_z_ _Printf_format_string_ LPCSTR strFormat, va_list pArgList )
{
    EnterCriticalSection( &m_CsConsoleBuf );
    
    // Count the required length of the string
    DWORD dwStrLen = _vscprintf( strFormat, pArgList ) + 1;    // +1 = null terminator
    CHAR* strMessage = ( CHAR* )_malloca( dwStrLen );
    if ( strMessage )
    {
        vsprintf_s( strMessage, dwStrLen, strFormat, pArgList );

        // Output the string to the console
        DWORD uStringLength = (DWORD)strlen( strMessage );
        for( DWORD i = 0; i < uStringLength; i++ )
        {
            Add( strMessage[i] );
        }

        // Output the string to the debug channel, if requested
        if( m_bOutputToDebugChannel )
        {
            OutputDebugStringA( strMessage );
        }   

        m_bDirty = TRUE;

        _freea( strMessage );
    }

    LeaveCriticalSection( &m_CsConsoleBuf );
}

VOID Console::FormatV( _In_z_ _Printf_format_string_ LPCWSTR wstrFormat, va_list pArgList )
{
    EnterCriticalSection( &m_CsConsoleBuf );
    
    // Count the required length of the string
    DWORD dwStrLen = _vscwprintf( wstrFormat, pArgList ) + 1;    // +1 = null terminator
    WCHAR* strMessage = ( WCHAR* )_malloca( dwStrLen * sizeof( WCHAR ) );
    if ( strMessage )
    {
        vswprintf_s( strMessage, dwStrLen, wstrFormat, pArgList );

        // Output the string to the console
        DWORD uStringLength = (DWORD)wcslen( strMessage );
        for( DWORD i = 0; i < uStringLength; i++ )
        {
            Add( strMessage[i] );
        }

        // Output the string to the debug channel, if requested
        if( m_bOutputToDebugChannel )
        {
            OutputDebugStringW( strMessage );
        }    

        m_bDirty = TRUE;

        _freea( strMessage );
    }

    LeaveCriticalSection( &m_CsConsoleBuf );
}

