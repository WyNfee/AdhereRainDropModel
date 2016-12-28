//----------------------------------------------------------------------------------------------------------------------
// Draw12.cpp
//
// Draw class for samples. For details, see header.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#include <pch.h>
#include "Draw12.h"

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
    : m_pSample(nullptr)
    , m_pCmdList(nullptr)
    , m_iCBColorBuffer(c_iCBColorBuffer)
{
    ZeroMemory(&m_viewport, sizeof(m_viewport));
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ~Draw()
// Desc: Destructor
//----------------------------------------------------------------------------------------------------------------------
Draw::~Draw()
{
    m_PSOCache.Terminate();

    m_CBHeap.Terminate();
}

//----------------------------------------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize all shaders and allocates buffers
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Draw::Initialize(const SampleFramework* const pSample)
{
    ATGPROFILETHIS;

    XSF_ASSERT(pSample);

    m_pSample = pSample;
    XSF::D3DDevice* const pDevice = m_pSample->GetDevice();
    ID3D12Fence* const pFence = m_pSample->GetFence();

    // Fullscreen quad
    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\FullScreenQuadVS.xvs", m_spFullScreenQuadVS.ReleaseAndGetAddressOf()));
    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\FullScreenQuadGS.xgs", m_spFullScreenQuadGS.ReleaseAndGetAddressOf()));
    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\FullScreenQuadPS.xps", m_spFullScreenQuadPS.ReleaseAndGetAddressOf()));
    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\FullScreenColoredQuadPS.xps", m_spFullScreenColoredQuadPS.ReleaseAndGetAddressOf()));

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    XSF_ERROR_IF_FAILED(m_frameHeap.Initialize(pDevice, pFence, 256 * 1024, false, m_pSample->SampleSettings().m_frameLatency, L"Draw::FrameHeap"));
    m_cbColorBufferView.SizeInBytes = static_cast<UINT>(NextMultiple(sizeof(ColorBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    XSF_RETURN_IF_FAILED(m_CBHeap.Initialize(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, c_CBHeapEnd, true));

    // 2D line
    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,}, 
                                                   {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,}};
    memcpy(m_LineInputLayout, &inputElementDesc, sizeof(m_LineInputLayout));

    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\LineVS.xvs", m_spLineVS.ReleaseAndGetAddressOf()));
    XSF_ERROR_IF_FAILED(LoadShader(L"Media\\Shaders\\LinePS.xps", m_spLinePS.ReleaseAndGetAddressOf()));

    m_vbLineView.StrideInBytes = sizeof(LineVertex);
    m_vbLineView.SizeInBytes = m_vbLineView.StrideInBytes * 2;

    // Define root table layout
    CD3DX12_DESCRIPTOR_RANGE descRange[c_numRootParameters];
    descRange[c_rootSRV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
    descRange[c_rootCBV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
    descRange[c_rootSampler].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0

    CD3DX12_ROOT_PARAMETER rootParameters[c_numRootParameters];
    rootParameters[c_rootSRV].InitAsDescriptorTable(1, &descRange[c_rootSRV], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[c_rootCBV].InitAsDescriptorTable(1, &descRange[c_rootCBV], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[c_rootSampler].InitAsDescriptorTable(1, &descRange[c_rootSampler], D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignature(ARRAYSIZE(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    D3DBlobPtr spSerializedSignature;
    XSF_RETURN_IF_FAILED(D3D12SerializeRootSignature(&rootSignature, D3D_ROOT_SIGNATURE_VERSION_1, spSerializedSignature.GetAddressOf(), nullptr));

    XSF_RETURN_IF_FAILED(pDevice->CreateRootSignature(
        c_nodeMask,
        spSerializedSignature->GetBufferPointer(),
        spSerializedSignature->GetBufferSize(),
        IID_GRAPHICS_PPV_ARGS(m_spRootSignature.ReleaseAndGetAddressOf())));

    // Define PSO template
    ZeroMemory(&m_descPSO, sizeof(m_descPSO));
    m_descPSO.pRootSignature = m_spRootSignature;
    m_descPSO.InputLayout.pInputElementDescs = m_LineInputLayout;
    m_descPSO.InputLayout.NumElements = sizeof(m_LineInputLayout) / sizeof(m_LineInputLayout[0]);
    m_descPSO.SampleDesc.Count = 1;
    m_descPSO.SampleMask = UINT_MAX;
    m_descPSO.NumRenderTargets = 1;
    m_descPSO.RTVFormats[0] = m_pSample->SampleSettings().m_swapChainFormat;
    m_descPSO.DSVFormat = m_pSample->SampleSettings().m_depthStencilFormat;

    // Allow child class to override a PSO parameter
    ModifyPSO(&m_descPSO);
  
    m_PSOCache.Initialize(pDevice, m_descPSO);

    return S_OK;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Begin()
// Desc: Setup shared states for draw calls, e.g. d3d context and viewport.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Begin(const D3D12_VIEWPORT* pViewport)
{
    m_pCmdList = m_pSample->GetCommandList();
    
    XSFBeginNamedEvent(m_pCmdList, 0, L"Draw::Begin");

    if (pViewport != nullptr)
    {
        m_pCmdList->RSSetViewports(1, pViewport);
        m_viewport = *pViewport;
    }
    else
    {
        m_viewport = m_pSample->GetBackbuffer().GetViewport();
    }

    m_descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_descPSO.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    m_descPSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    
    // While we could set up render states for drawing here, this is left to the caller so that they can modify the
    // render behavior easily.
}


//----------------------------------------------------------------------------------------------------------------------
// Name: End()
// Desc: Restores states saved in Begin()
//----------------------------------------------------------------------------------------------------------------------
void Draw::End()
{
    XSF_ASSERT(m_pCmdList);

    XSFEndNamedEvent(m_pCmdList);

    m_pCmdList = nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Quad()
// Desc: Draws a colored quad the full size of the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Quad(const XMVECTOR& color, const D3D12_VIEWPORT* pViewport)
{
    XSF_ASSERT(m_pCmdList);

    D3DDevice* const pDevice = m_pSample->GetDevice();

    XSFScopedNamedEventFunc(m_pCmdList, 0);

    if (pViewport != nullptr)
    {
        m_pCmdList->RSSetViewports(1, pViewport);
    }

    // set root signature and descriptor heaps
    m_pCmdList->SetGraphicsRootSignature(m_spRootSignature);
    ID3D12DescriptorHeap* pHeaps[] = { m_CBHeap };
    m_pCmdList->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);

    // Lock the color constant buffer so it can be written to
    ColorBuffer* pColorData;
    XSF_ERROR_IF_FAILED(m_frameHeap.GetHeapPointer(m_cbColorBufferView.SizeInBytes, m_cbColorBufferView.BufferLocation, reinterpret_cast<BYTE**>(&pColorData)));

    // Copy the color data into the constant buffer
    pColorData->color = color;

    // Finalize the constant buffer into descriptor heap
    pDevice->CreateConstantBufferView(&m_cbColorBufferView, m_CBHeap.hCPU(m_iCBColorBuffer));
    m_pCmdList->SetGraphicsRootDescriptorTable(c_rootCBV, m_CBHeap.hGPU(m_iCBColorBuffer));
    if (++m_iCBColorBuffer >= c_CBHeapEnd)
    {
        m_iCBColorBuffer = c_iCBColorBuffer;
    }
    
    // Finalize the pipeline state
    m_descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    m_descPSO.InputLayout.pInputElementDescs = nullptr;
    m_descPSO.InputLayout.NumElements = 0;
    m_descPSO.VS.pShaderBytecode = m_spFullScreenQuadVS->GetBufferPointer();
    m_descPSO.VS.BytecodeLength = m_spFullScreenQuadVS->GetBufferSize();
    m_descPSO.GS.pShaderBytecode = m_spFullScreenQuadGS->GetBufferPointer();
    m_descPSO.GS.BytecodeLength = m_spFullScreenQuadGS->GetBufferSize();
    m_descPSO.PS.pShaderBytecode = m_spFullScreenColoredQuadPS->GetBufferPointer();
    m_descPSO.PS.BytecodeLength = m_spFullScreenColoredQuadPS->GetBufferSize();

    ID3D12PipelineState *pPipelineState = m_PSOCache.FindOrCreatePSO(m_descPSO);
    XSF_ASSERT(pPipelineState != nullptr);
    m_pCmdList->SetPipelineState(pPipelineState);

    m_pCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    
    m_pCmdList->DrawInstanced(1, 1, 0, 0);
}


//----------------------------------------------------------------------------------------------------------------------
// Name: TexturedQuad()
// Desc: Draws a textured quad the full size of the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::TexturedQuad(ID3D12DescriptorHeap* const pTextureHeap, D3D12_GPU_DESCRIPTOR_HANDLE hTexture, ID3D12DescriptorHeap* const pSamplerHeap, D3D12_GPU_DESCRIPTOR_HANDLE hSampler, const D3D12_VIEWPORT* pViewport, ID3DBlob* pPixelShader)
{
    XSF_ASSERT(m_pCmdList);

    XSFScopedNamedEventFunc(m_pCmdList, 0);

    if (pViewport != nullptr)
    { 
        m_pCmdList->RSSetViewports(1, pViewport);
    }

    // set root signature and descriptor heaps
    m_pCmdList->SetGraphicsRootSignature(m_spRootSignature);
    ID3D12DescriptorHeap* pHeaps[] = {pTextureHeap, pSamplerHeap};
    m_pCmdList->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);

    // Set root descriptors
    m_pCmdList->SetGraphicsRootDescriptorTable(c_rootSRV, hTexture);
    m_pCmdList->SetGraphicsRootDescriptorTable(c_rootSampler, hSampler);

    // Finalize the pipeline state
    m_descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    m_descPSO.InputLayout.pInputElementDescs = nullptr;
    m_descPSO.InputLayout.NumElements = 0;
    m_descPSO.VS.pShaderBytecode = m_spFullScreenQuadVS->GetBufferPointer();
    m_descPSO.VS.BytecodeLength = m_spFullScreenQuadVS->GetBufferSize();
    m_descPSO.GS.pShaderBytecode = m_spFullScreenQuadGS->GetBufferPointer();
    m_descPSO.GS.BytecodeLength = m_spFullScreenQuadGS->GetBufferSize();
    // If a custom shader is specified, use that, otherwise we use the default one
    m_descPSO.PS.pShaderBytecode = (pPixelShader != nullptr)? pPixelShader->GetBufferPointer() : m_spFullScreenQuadPS->GetBufferPointer();
    m_descPSO.PS.BytecodeLength = (pPixelShader != nullptr)? pPixelShader->GetBufferSize() : m_spFullScreenQuadPS->GetBufferSize();

    ID3D12PipelineState *pPipelineState = m_PSOCache.FindOrCreatePSO(m_descPSO);
    XSF_ASSERT(pPipelineState != nullptr);
    m_pCmdList->SetPipelineState(pPipelineState);

    m_pCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_pCmdList->DrawInstanced(1, 1, 0, 0);
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Line()
// Desc: Draws a colored 2D line in the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::Line(INT32 startX, INT32 startY, const XMVECTOR& startColor, INT32 endX, INT32 endY, const XMVECTOR& endColor, const D3D12_VIEWPORT* pViewport)
{
    XSF_ASSERT(m_pCmdList);

    XSFScopedNamedEventFunc(m_pCmdList, 0);

    FLOAT width  = m_viewport.Width;
    FLOAT height = m_viewport.Height;

    if (pViewport != nullptr)
    {
        m_pCmdList->RSSetViewports(1, pViewport);
        width   = pViewport->Width;
        height  = pViewport->Height;
    }

    XSF_ASSERT(width != 0 && height != 0);

    const XMVECTOR xy0 = XMVectorSet((startX / (width - 1)) * 2.0f - 1.0f, (startY / (height - 1)) * -2.0f + 1.0f, 0.0f, 1.0f);
    const XMVECTOR xy1 = XMVectorSet((endX / (width - 1)) * 2.0f - 1.0f, (endY / (height - 1)) * -2.0f + 1.0f, 0.0f, 1.0f);

    LineInNDC(xy0, startColor, xy1, endColor);
}


//----------------------------------------------------------------------------------------------------------------------
// Name: LineInNDC()
// Desc: Draws a colored 2D line in the current viewport
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void Draw::LineInNDC(const XMVECTOR& xy0, const XMVECTOR& startColor, const XMVECTOR& xy1, const XMVECTOR& endColor, const D3D12_VIEWPORT* pViewport)
{
    XSF_ASSERT(m_pCmdList);

    XSFScopedNamedEventFunc(m_pCmdList, 0);

    if (pViewport != nullptr)
    {
        m_pCmdList->RSSetViewports(1, pViewport);
    }

    LineVertex vertexData[] = {{xy0, startColor},
                               {xy1, endColor  }};
    void *pData;
    XSF_ERROR_IF_FAILED(m_frameHeap.GetHeapPointer(m_vbLineView.SizeInBytes, m_vbLineView.BufferLocation, reinterpret_cast<BYTE**>(&pData)));

    memcpy(pData, vertexData, sizeof(vertexData));

    // set root signature and descriptor heaps
    m_pCmdList->SetGraphicsRootSignature(m_spRootSignature);

    // Finalize dynamic VB
    m_pCmdList->IASetVertexBuffers(0, 1, &m_vbLineView);

    // Finalize the pipeline state
    m_descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    m_descPSO.InputLayout.pInputElementDescs = m_LineInputLayout;
    m_descPSO.InputLayout.NumElements = sizeof(m_LineInputLayout) / sizeof(m_LineInputLayout[0]);
    m_descPSO.VS.pShaderBytecode = m_spLineVS->GetBufferPointer();
    m_descPSO.VS.BytecodeLength = m_spLineVS->GetBufferSize();
    m_descPSO.GS.pShaderBytecode = nullptr;
    m_descPSO.GS.BytecodeLength = 0;
    m_descPSO.PS.pShaderBytecode = m_spLinePS->GetBufferPointer();
    m_descPSO.PS.BytecodeLength = m_spLinePS->GetBufferSize();

    ID3D12PipelineState *pPipelineState = m_PSOCache.FindOrCreatePSO(m_descPSO);
    XSF_ASSERT(pPipelineState != nullptr);
    m_pCmdList->SetPipelineState(pPipelineState);

    m_pCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    m_pCmdList->DrawInstanced(2, 1, 0, 0);
}
