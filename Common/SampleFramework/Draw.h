//--------------------------------------------------------------------------------------
// Draw.h
//
// Helper functions to draw lines/quads in samples
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_DRAW_H_INCLUDED
#define XSF_DRAW_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

namespace XboxSampleFramework
{
    //--------------------------------------------------------------------------------------
    // Name: class Draw
    // Desc: Class to draw lines/quads in samples
    //--------------------------------------------------------------------------------------
    class Draw
    {
    public:
        struct LineVertex
        {
            XMVECTOR    m_position;
            XMVECTOR    m_color;
        };

        Draw();
        ~Draw();

        HRESULT    Initialize( _In_ D3DDevice* pd3dDevice );

        void Begin( _In_ D3DDeviceContext* pd3dContext, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr );
        void End();

        void Quad( _In_ const XMVECTOR& color, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr );
        void TexturedQuad( _In_ ID3D11ShaderResourceView* pShaderResourceView, _In_ ID3D11SamplerState* pSamplerState, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr, _In_opt_ ID3D11PixelShader* pPixelShader = nullptr );
        void Line( _In_ INT32 startX, _In_ INT32 _In_ startY, _In_ const XMVECTOR& startColor, _In_ INT32 endX, _In_ INT32 endY, _In_ const XMVECTOR& endColor, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr );
        void LineInNDC( const XMVECTOR& xy0, const XMVECTOR& startColor, const XMVECTOR& xy1, const XMVECTOR& endColor, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr );
        void LinesInNDC( _In_ const LineVertex* vertexData, UINT iCount, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ const D3D11_VIEWPORT *pOldViewport = nullptr );

    private:
        D3DDeviceContext*       m_pd3dContext;              // Current d3d context
        D3D11_VIEWPORT          m_viewPort[ 1 ];            // Current viewport
        D3D11_VIEWPORT          m_oldViewPort[ 1 ];         // Old viewport that needs to be restored when End() is called
        BOOL                    m_saveViewPort;             // Save the current viewport when Begin() is called?

        // Full screen quad
        ID3D11VertexShader*     m_pFullScreenQuadVS;
        ID3D11GeometryShader*   m_pFullScreenQuadGS;
        ID3D11PixelShader*      m_pFullScreenQuadPS;
        ID3D11PixelShader*      m_pFullScreenColoredQuadPS;

        // 2D line
        static const UINT       m_maxLines = 1024;
        ID3D11InputLayout*      m_pLineInputLayout;
        ID3D11Buffer*           m_pLineVertexBuffer;
        ID3D11VertexShader*     m_pLineVS;
        ID3D11PixelShader*      m_pLinePS;

        // Constant buffer
        ID3D11Buffer*           m_pColorBuffer;

    };


} // namespace XboxSampleFramework

#endif // XSF_DRAW_H_INCLUDED
