//----------------------------------------------------------------------------------------------------------------------
// Draw.cpp
//
// Draw class for samples. For details, see header.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#include <pch.h>
#include "Draw.h"

using namespace XboxSampleFramework;

struct ColorBuffer
{
    XMVECTOR color;
};

//----------------------------------------------------------------------------------------------------------------------
// Name: Draw()
// Desc: Constructor
//----------------------------------------------------------------------------------------------------------------------

Draw::Draw()
    : m_saveViewPort( FALSE ),
      m_pd3dContext( nullptr ),
      m_pFullScreenQuadVS( nullptr ),
      m_pFullScreenQuadGS( nullptr ),
      m_pFullScreenQuadPS( nullptr ),
      m_pFullScreenColoredQuadPS( nullptr ),
      m_pLineInputLayout( nullptr ),
      m_pLineVertexBuffer( nullptr ),
      m_pLineVS( nullptr ),
      m_pLinePS( nullptr ),
      m_pColorBuffer( nullptr )
{
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ~Draw()
// Desc: Destructor
//----------------------------------------------------------------------------------------------------------------------

Draw::~Draw()
{
    XSF_SAFE_RELEASE( m_pFullScreenQuadVS );
    XSF_SAFE_RELEASE( m_pFullScreenQuadGS );
    XSF_SAFE_RELEASE( m_pFullScreenQuadPS );
    XSF_SAFE_RELEASE( m_pFullScreenColoredQuadPS );
    XSF_SAFE_RELEASE( m_pLineInputLayout );
    XSF_SAFE_RELEASE( m_pLineVertexBuffer );
    XSF_SAFE_RELEASE( m_pLineVS );
    XSF_SAFE_RELEASE( m_pLinePS );
    XSF_SAFE_RELEASE( m_pColorBuffer );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize all shaders and allocates buffers
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Draw::Initialize( D3DDevice* pd3dDevice )
{
    ATGPROFILETHIS;

    XSF_ASSERT( pd3dDevice );

    // Fullscreen quad
    XSF_ERROR_IF_FAILED( LoadVertexShader( pd3dDevice, L"Media\\Shaders\\FullScreenQuadVS.xvs", &m_pFullScreenQuadVS ) );
    XSF_ERROR_IF_FAILED( LoadGeometryShader( pd3dDevice, L"Media\\Shaders\\FullScreenQuadGS.xgs", &m_pFullScreenQuadGS ) );
    XSF_ERROR_IF_FAILED( LoadPixelShader( pd3dDevice, L"Media\\Shaders\\FullScreenQuadPS.xps", &m_pFullScreenQuadPS ) );
    XSF_ERROR_IF_FAILED( LoadPixelShader( pd3dDevice, L"Media\\Shaders\\FullScreenColoredQuadPS.xps", &m_pFullScreenColoredQuadPS ) );

    D3D11_BUFFER_DESC colorBufferDesc;
    colorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    colorBufferDesc.ByteWidth = sizeof(ColorBuffer);
    colorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    colorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    colorBufferDesc.MiscFlags = 0;
    colorBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateBuffer( &colorBufferDesc, NULL, &m_pColorBuffer ) );

    // 2D line
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0, }, 
                                                    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0, } };

    XSF_ERROR_IF_FAILED( LoadVertexShader( pd3dDevice, L"Media\\Shaders\\LineVS.xvs", &m_pLineVS, &inputElementDesc[0], _countof( inputElementDesc ), &m_pLineInputLayout  ) );
    XSF_ERROR_IF_FAILED( LoadPixelShader( pd3dDevice, L"Media\\Shaders\\LinePS.xps", &m_pLinePS ) );

    const D3D11_BUFFER_DESC bufferDesc = { sizeof( LineVertex ) * ( m_maxLines + 1 ), D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, sizeof( LineVertex ) };
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateBuffer( &bufferDesc, nullptr, &m_pLineVertexBuffer ) );

    return S_OK;
}



//----------------------------------------------------------------------------------------------------------------------
// Name: Begin()
// Desc: Setup shared states for draw calls, e.g. d3d context and viewport.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Begin( D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewport, const D3D11_VIEWPORT* pOldViewport )
{
    XSF_ASSERT( pd3dContext );
    XSFBeginNamedEvent( pd3dContext, 0, L"Draw::Begin" );

    UINT numViewPorts = 1;

    if( pViewport != nullptr )
    {
        if( pOldViewport != nullptr )
        {
            m_oldViewPort[0] = *pOldViewport;
        }
        else
        {
            pd3dContext->RSGetViewports( &numViewPorts, m_oldViewPort );
        }
        pd3dContext->RSSetViewports( 1, pViewport );
        m_saveViewPort = TRUE;
    }

    m_pd3dContext = pd3dContext;
    if( pOldViewport != nullptr )
    {
        *m_viewPort = *pOldViewport;
    }
    else
    {
        m_pd3dContext->RSGetViewports( &numViewPorts, m_viewPort );
    }

    // While we could set up render states for drawing here, this is left to the caller so that they can modify the
    // render behavior easily.
}


//----------------------------------------------------------------------------------------------------------------------
// Name: End()
// Desc: Restores states saved in Begin()
//----------------------------------------------------------------------------------------------------------------------
void Draw::End()
{
    XSF_ASSERT( m_pd3dContext );

    // Restore the old viewport
    if( m_saveViewPort )
    {
        m_pd3dContext->RSSetViewports( 1, m_oldViewPort );
    }
    
    m_saveViewPort  = FALSE;

    XSFEndNamedEvent( m_pd3dContext );

    m_pd3dContext   = nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Quad()
// Desc: Draws a colored quad the full size of the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Quad( const XMVECTOR& color, const D3D11_VIEWPORT* pViewport, const D3D11_VIEWPORT* pOldViewport )
{
    XSF_ASSERT( m_pd3dContext );

    XSFScopedNamedEventFunc( m_pd3dContext, 0 );

    D3D11_VIEWPORT  oldViewPort[ 1 ];
    UINT            numViewPorts = 1;

    if( pViewport != nullptr )
    {
        if( pOldViewport != nullptr )
        {
            oldViewPort[0] = *pOldViewport;
        }
        else
        {
            m_pd3dContext->RSGetViewports( &numViewPorts, oldViewPort );
        }
        m_pd3dContext->RSSetViewports( 1, pViewport );
    }

    // Lock the color constant buffer so it can be written to
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    XSF_ERROR_IF_FAILED( m_pd3dContext->Map( m_pColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ) );

    // Get a pointer to the data in the constant buffer
    ColorBuffer* pColorData = (ColorBuffer*)mappedResource.pData;

    // Copy the color data into the constant buffer
    pColorData->color = color;

    // Unlock the constant buffer
    m_pd3dContext->Unmap( m_pColorBuffer, 0 );

    // Finally set the constant buffer in the pixel shader with the updated values
    m_pd3dContext->PSSetConstantBuffers( 0, 1, &m_pColorBuffer );
    
    m_pd3dContext->VSSetShader( m_pFullScreenQuadVS, nullptr, 0 );
    m_pd3dContext->GSSetShader( m_pFullScreenQuadGS, nullptr, 0 );
    m_pd3dContext->PSSetShader( m_pFullScreenColoredQuadPS, nullptr, 0 );
    m_pd3dContext->HSSetShader( nullptr, nullptr, 0 );
    m_pd3dContext->DSSetShader( nullptr, nullptr, 0 );
    
    m_pd3dContext->IASetInputLayout( nullptr );
    m_pd3dContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, 0 );        
    m_pd3dContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
    
    m_pd3dContext->Draw( 1, 0 );

    if ( pViewport != nullptr )
    {
        // Restore the old viewport
        m_pd3dContext->RSSetViewports( numViewPorts, oldViewPort );
    }
}


//----------------------------------------------------------------------------------------------------------------------
// Name: TexturedQuad()
// Desc: Draws a textured quad the full size of the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::TexturedQuad(ID3D11ShaderResourceView* pTextureView, ID3D11SamplerState* pSamplerState, const D3D11_VIEWPORT* pViewport, const D3D11_VIEWPORT* pOldViewport, ID3D11PixelShader* pPixelShader)
{
	XSF_ASSERT(m_pd3dContext);

	XSFScopedNamedEventFunc(m_pd3dContext, 0);

	D3D11_VIEWPORT  oldViewPort[1];
	UINT            numViewPorts = 1;

	if (pViewport != nullptr)
	{
		if (pOldViewport != nullptr)
		{
			oldViewPort[0] = *pOldViewport;
		}
		else
		{
			m_pd3dContext->RSGetViewports(&numViewPorts, oldViewPort);
		}
		m_pd3dContext->RSSetViewports(1, pViewport);
	}

	m_pd3dContext->VSSetShader(m_pFullScreenQuadVS, nullptr, 0);
	m_pd3dContext->GSSetShader(m_pFullScreenQuadGS, nullptr, 0);
	m_pd3dContext->HSSetShader(nullptr, nullptr, 0);
	m_pd3dContext->DSSetShader(nullptr, nullptr, 0);

	// If a custom shader is specified, use that, otherwise we use the default one
	m_pd3dContext->PSSetShader(pPixelShader ? pPixelShader : m_pFullScreenQuadPS, nullptr, 0);

	if (pTextureView)
	{
		m_pd3dContext->PSSetShaderResources(0, 1, &pTextureView);
	}

    m_pd3dContext->PSSetSamplers( 0, 1, &pSamplerState );

    m_pd3dContext->IASetInputLayout( nullptr );
    m_pd3dContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, 0 );        
    m_pd3dContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );

    m_pd3dContext->Draw( 1, 0 );

    if( pViewport != nullptr )
    {
        // Restore the old viewport
        m_pd3dContext->RSSetViewports( numViewPorts, oldViewPort );
    }
    ID3D11ShaderResourceView* pnullView[] = { nullptr };
    m_pd3dContext->PSSetShaderResources( 0, 1, pnullView );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Line()
// Desc: Draws a colored 2D line in the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Line( INT32 startX, INT32 startY, const XMVECTOR& startColor, INT32 endX, INT32 endY, const XMVECTOR& endColor, const D3D11_VIEWPORT* pViewport, const D3D11_VIEWPORT* pOldViewport )
{
    XSF_ASSERT( m_pd3dContext );

    XSFScopedNamedEventFunc( m_pd3dContext, 0 );

    FLOAT width     = m_viewPort[ 0 ].Width;
    FLOAT height    = m_viewPort[ 0 ].Height;

    D3D11_VIEWPORT  oldViewPort[ 1 ];
    UINT            numViewPorts = 1;

    if( pViewport != nullptr )
    {
        if( pOldViewport != nullptr )
        {
            oldViewPort[0] = *pOldViewport;
        }
        else
        {
            m_pd3dContext->RSGetViewports( &numViewPorts, oldViewPort );
        }
        m_pd3dContext->RSSetViewports( 1, pViewport );
        width   = pViewport->Width;
        height  = pViewport->Height;
    }

    const XMVECTOR xy0 = XMVectorSet( ( startX / ( width - 1 ) ) * 2.0f - 1.0f, ( startY / ( height - 1 ) ) * -2.0f + 1.0f, 0.0f, 1.0f );
    const XMVECTOR xy1 = XMVectorSet( ( endX / ( width - 1 ) ) * 2.0f - 1.0f, ( endY / ( height - 1 ) ) * -2.0f + 1.0f, 0.0f, 1.0f );

    LineInNDC( xy0, startColor, xy1, endColor, nullptr );

    if( pViewport != nullptr )
    {
        // Restore the old viewport
        m_pd3dContext->RSSetViewports( numViewPorts, oldViewPort );
    }
}


//----------------------------------------------------------------------------------------------------------------------
// Name: LineInNDC()
// Desc: Draws a colored 2D line in the current viewport
//----------------------------------------------------------------------------------------------------------------------

void Draw::LineInNDC( const XMVECTOR& xy0, const XMVECTOR& startColor, const XMVECTOR& xy1, const XMVECTOR& endColor, _In_opt_ const D3D11_VIEWPORT* pViewport, const D3D11_VIEWPORT* pOldViewport )
{
    LineVertex vertexData[ 2 ];
    vertexData[ 0 ].m_position  = xy0;
    vertexData[ 0 ].m_color     = startColor;

    vertexData[ 1 ].m_position  = xy1;
    vertexData[ 1 ].m_color     = endColor;

    LinesInNDC( vertexData, 2, pViewport, pOldViewport );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: LinesInNDC()
// Desc: Draws colored 2D lines in the current viewport
//----------------------------------------------------------------------------------------------------------------------

void Draw::LinesInNDC( _In_ const LineVertex* vertexData, UINT iCount, _In_opt_ const D3D11_VIEWPORT* pViewport, _In_opt_ const D3D11_VIEWPORT *pOldViewport )
{
    XSF_ASSERT( m_pd3dContext );

    XSFScopedNamedEventFunc( m_pd3dContext, 0 );

    D3D11_VIEWPORT  oldViewPort[ 1 ];
    UINT            numViewPorts = 1;

    if( pViewport != nullptr )
    {
        if( pOldViewport != nullptr )
        {
            oldViewPort[0] = *pOldViewport;
        }
        else
        {
            m_pd3dContext->RSGetViewports( &numViewPorts, oldViewPort );
        }
        m_pd3dContext->RSSetViewports( 1, pViewport );
    }

    D3D11_MAPPED_SUBRESOURCE ms;
    XSF_ERROR_IF_FAILED( m_pd3dContext->Map( m_pLineVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms ) );
    memcpy( ms.pData, vertexData, iCount * sizeof( vertexData[ 0 ] ) );
    m_pd3dContext->Unmap( m_pLineVertexBuffer, 0 );

    m_pd3dContext->VSSetShader( m_pLineVS, nullptr, 0 );
    m_pd3dContext->GSSetShader( nullptr, nullptr, 0 );
    m_pd3dContext->HSSetShader( nullptr, nullptr, 0 );
    m_pd3dContext->DSSetShader( nullptr, nullptr, 0 );
    m_pd3dContext->PSSetShader( m_pLinePS, nullptr, 0 );

    m_pd3dContext->IASetInputLayout( m_pLineInputLayout );
    m_pd3dContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP );

    UINT Strides[1] = { sizeof( LineVertex ), };
    UINT Offsets[1] = { 0, };
    m_pd3dContext->IASetVertexBuffers( 0, 1, &m_pLineVertexBuffer, Strides, Offsets );

    m_pd3dContext->Draw( iCount, 0 );

    if( pViewport != nullptr )
    {
        // Restore the old viewport
        m_pd3dContext->RSSetViewports( numViewPorts, oldViewPort );
    }
}

