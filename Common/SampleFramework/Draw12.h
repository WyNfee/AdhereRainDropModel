//--------------------------------------------------------------------------------------
// Draw12.h
//
// Helper functions to draw lines/quads in samples
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_DRAW12_H_INCLUDED
#define XSF_DRAW12_H_INCLUDED

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
    private:
        struct LineVertex
        {
            XMVECTOR    m_position;
            XMVECTOR    m_color;
        };

    public:
        Draw();
        ~Draw();

        HRESULT Initialize(_In_ const SampleFramework* const pSample);

        void Begin(_In_opt_ const D3D12_VIEWPORT* pViewport = nullptr);
        void End();

        inline void ApplyRasterizerState(const D3D12_RASTERIZER_DESC* const pRasterizerDesc)
        {
            memcpy(&m_descPSO.RasterizerState, pRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
        }

        inline void ApplyBlendState(const D3D12_BLEND_DESC* const pBlendDesc)
        {
            memcpy(&m_descPSO.BlendState, pBlendDesc, sizeof(D3D12_BLEND_DESC));
        }

        inline void ApplyDepthStencilState(const D3D12_DEPTH_STENCIL_DESC* const pDepthStencilDesc)
        {
            memcpy(&m_descPSO.DepthStencilState, pDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
        }

        virtual void ModifyPSO(_Inout_ D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDescPSO)
        {
            //  pDescPSO parameter contains state set by Draw class. 
            //  Override this method to modify parameters just before PSO creation (in Initialize() method).
            UNREFERENCED_PARAMETER(pDescPSO);
        }

        void Quad(_In_ const XMVECTOR& color, _In_opt_ const D3D12_VIEWPORT* pViewport = nullptr);
        void TexturedQuad(_In_ ID3D12DescriptorHeap* const pTextureHeap, D3D12_GPU_DESCRIPTOR_HANDLE hTexture, _In_ ID3D12DescriptorHeap* const pSamplerHeap, D3D12_GPU_DESCRIPTOR_HANDLE hSampler, _In_opt_ const D3D12_VIEWPORT* pViewport = nullptr, _In_opt_ ID3DBlob* pPixelShader = nullptr);
        void Line(_In_ INT32 startX, _In_ INT32 _In_ startY, _In_ const XMVECTOR& startColor, _In_ INT32 endX, _In_ INT32 endY, _In_ const XMVECTOR& endColor, _In_opt_ const D3D12_VIEWPORT* pViewport = nullptr);
        void LineInNDC(const XMVECTOR& xy0, const XMVECTOR& startColor, const XMVECTOR& xy1, const XMVECTOR& endColor, _In_opt_ const D3D12_VIEWPORT* pViewport = nullptr);

    private:
        const SampleFramework*              m_pSample;
        D3DCommandList*                     m_pCmdList;
        D3D12_VIEWPORT                      m_viewport;

        // Full screen quad
        D3DBlobPtr                          m_spFullScreenQuadVS;
        D3DBlobPtr                          m_spFullScreenQuadGS;
        D3DBlobPtr                          m_spFullScreenQuadPS;
        D3DBlobPtr                          m_spFullScreenColoredQuadPS;

        // 2D line
        D3D12_INPUT_ELEMENT_DESC            m_LineInputLayout[2];
        D3DBlobPtr                          m_spLineVS;
        D3DBlobPtr                          m_spLinePS;

        // Constant buffer
        CpuGpuHeap                          m_frameHeap;
        DescriptorHeapWrapper               m_CBHeap;
        D3D12_CONSTANT_BUFFER_VIEW_DESC     m_cbColorBufferView;

        // Vertex buffer
        D3D12_VERTEX_BUFFER_VIEW            m_vbLineView;

        D3DRootSignaturePtr                 m_spRootSignature;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC  m_descPSO;
        PSOCache                            m_PSOCache;

        // indexes of root parameters
        static const UINT                   c_rootSRV = 0;
        static const UINT                   c_rootCBV = c_rootSRV + 1;
        static const UINT                   c_rootSampler = c_rootCBV + 1;
        static const UINT                   c_numRootParameters = c_rootSampler + 1;

        // indexes in the descriptor heap
        static const UINT                   c_iCBColorBuffer = 0;
        static const UINT                   c_CBHeapEnd = 1000;
        UINT                                m_iCBColorBuffer;
    };

} // namespace XboxSampleFramework

#endif // XSF_DRAW12_H_INCLUDED
