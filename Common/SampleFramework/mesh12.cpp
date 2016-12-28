//--------------------------------------------------------------------------------------
// File: Mesh12.cpp
//
// The SDK Mesh format (.sdkmesh) is not a recommended file format for games.  
// It was designed to meet the specific needs of the SDK samples.  Any real-world 
// applications should avoid this file format in favor of a destination format that 
// meets the specific needs of the application.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <pch.h>

#include "common.h"
#include "Mesh12.h"


//--------------------------------------------------------------------------------------
// Hard Defines for the various structures
//--------------------------------------------------------------------------------------
#define SDKMESH_FILE_VERSION 101

#define INVALID_FRAME ((UINT)-1)
#define INVALID_MESH ((UINT)-1)
#define INVALID_MATERIAL ((UINT)-1)
#define INVALID_SUBSET ((UINT)-1)
#define INVALID_ANIMATION_DATA ((UINT)-1)
#define ERROR_RESOURCE_VALUE 0
#define INVALID_SAMPLER_SLOT ((UINT)-1)



namespace XboxSampleFramework
{
    namespace Detail
    {
        BOOL IsErrorResource(ID3D12Resource* pResource)
        {
            return (pResource == ERROR_RESOURCE_VALUE);
        }
        BOOL IsErrorResource(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            return (handle.ptr == ERROR_RESOURCE_VALUE);
        }

        //--------------------------------------------------------------------------------------
        // Enumerated Types.  These will have mirrors in both D3D9 and D3D10
        //--------------------------------------------------------------------------------------
        enum SDKMESH_PRIMITIVE_TYPE
        {
            PT_TRIANGLE_LIST = 0,
            PT_TRIANGLE_STRIP,
            PT_LINE_LIST,
            PT_LINE_STRIP,
            PT_POINT_LIST,
            PT_TRIANGLE_LIST_ADJ,
            PT_TRIANGLE_STRIP_ADJ,
            PT_LINE_LIST_ADJ,
            PT_LINE_STRIP_ADJ,
            PT_QUAD_PATCH_LIST,
            PT_TRIANGLE_PATCH_LIST,
        };

        enum SDKMESH_INDEX_TYPE
        {
            IT_16BIT = 0,
            IT_32BIT,
        };

        enum FRAME_TRANSFORM_TYPE
        {
            FTT_RELATIVE = 0,
            FTT_ABSOLUTE,        //This is not currently used but is here to support absolute transformations in the future
        };


        struct LEGACY_D3DVERTEXELEMENT9
        {
            WORD    Stream;     // Stream index
            WORD    Offset;     // Offset in the stream in bytes
            BYTE    Type;       // Data type
            BYTE    Method;     // Processing method
            BYTE    Usage;      // Semantics
            BYTE    UsageIndex; // Semantic index
        };

        //--------------------------------------------------------------------------------------
        D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveType(SDKMESH_PRIMITIVE_TYPE PrimType)
        {
            D3D12_PRIMITIVE_TOPOLOGY retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            switch (PrimType)
            {
                case PT_TRIANGLE_LIST:
                    retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                    break;
                case PT_TRIANGLE_STRIP:
                    retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
                    break;
                case PT_LINE_LIST:
                    retType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
                    break;
                case PT_LINE_STRIP:
                    retType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
                    break;
                case PT_POINT_LIST:
                    retType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
                    break;
                case PT_TRIANGLE_LIST_ADJ:
                    retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
                    break;
                case PT_TRIANGLE_STRIP_ADJ:
                    retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
                    break;
                case PT_LINE_LIST_ADJ:
                    retType = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
                    break;
                case PT_LINE_STRIP_ADJ:
                    retType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
                    break;
                case PT_QUAD_PATCH_LIST:
                    retType = D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
                    break;
                case PT_TRIANGLE_PATCH_LIST:
                    retType = D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
                    break;
            };

            return retType;
        }
    }
}

//--------------------------------------------------------------------------------------
// Structures.  Unions with pointers are forced to 64bit.
//--------------------------------------------------------------------------------------
struct XSF::Mesh::Header
{
    //Basic Info and sizes
    UINT Version;
    BYTE IsBigEndian;
    UINT64 HeaderSize;
    UINT64 NonBufferDataSize;
    UINT64 BufferDataSize;

    //Stats
    UINT NumVertexBuffers;
    UINT NumIndexBuffers;
    UINT NumMeshes;
    UINT NumTotalSubsets;
    UINT NumFrames;
    UINT NumMaterials;

    //Offsets to Data
    UINT64 VertexStreamHeadersOffset;
    UINT64 IndexStreamHeadersOffset;
    UINT64 MeshDataOffset;
    UINT64 SubsetDataOffset;
    UINT64 FrameDataOffset;
    UINT64 MaterialDataOffset;
};


struct XSF::Mesh::VBHeader
{
    UINT64 NumVertices;
    UINT64 SizeBytes;
    UINT64 StrideBytes;
    XSF::Detail::LEGACY_D3DVERTEXELEMENT9 Decl[32];
    union
    {
        UINT64 DataOffset;                //(This also forces the union to 64bits)
        ID3D12Resource*    pVB;
    };
};

struct XSF::Mesh::IBHeader
{
    UINT64 NumIndices;
    UINT64 SizeBytes;
    UINT IndexType;
    union
    {
        UINT64 DataOffset;                //(This also forces the union to 64bits)
        ID3D12Resource*   pIB;
    };
};

struct XSF::Mesh::AnimFileHeader
{
    UINT Version;
    BYTE IsBigEndian;
    UINT FrameTransformType;
    UINT NumFrames;
    UINT NumAnimationKeys;
    UINT AnimationFPS;
    UINT64 AnimationDataSize;
    UINT64 AnimationDataOffset;
};

struct XSF::Mesh::AnimFrameData
{
    struct AnimDataItem
    {
        FLOAT   Translation[3];
        FLOAT   Orientation[4];
        FLOAT   Scaling[3];
    };

    CHAR FrameName[XSF::Mesh::MAX_FRAME_NAME];
    union
    {
        UINT64 DataOffset;
        AnimDataItem* pAnimationData;
    };
};


class CpuGpuHeap;
struct MeshLoad
{
    ID3D12Device* pDeviceice;
    ID3D12CommandList* pCmdList;
    CpuGpuHeap* pUploadHeap;
};


//--------------------------------------------------------------------------------------
XSF::Mesh::Mesh() : m_NumOutstandingResources(0),
                    m_pMeshData(nullptr),
                    m_pMeshHeader(nullptr),
                    m_pAnimationData(nullptr),
                    m_ppVertices(nullptr),
                    m_ppIndices(nullptr),
                    m_pBindPoseFrameMatrices(nullptr),
                    m_pTransformedFrameMatrices(nullptr),
                    m_pWorldPoseFrameMatrices(nullptr),
                    m_pSRVHeap(nullptr),
                    m_pSRVHeapIndex(nullptr),
                    m_uploadHeapSizeMB(256)
{
}


//--------------------------------------------------------------------------------------
XSF::Mesh::~Mesh()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::LoadMaterials(Material* pMaterials, UINT numMaterials, const Callbacks* pLoaderCallbacks, UINT *pSrvHeapIndex)
{
    VERBOSEATGPROFILETHIS;

    XSF::D3DDevice* const pDevice = m_pSample->GetDevice();

    UINT currentHeapIndex = 0;
    UINT *pCurrentHeapIndex = (pSrvHeapIndex != nullptr)? pSrvHeapIndex : &currentHeapIndex;
    for (UINT m = 0; m < numMaterials; m++)
    {
        pMaterials[m].m_pDiffuseTexture = nullptr;
        pMaterials[m].m_pNormalTexture = nullptr;
        pMaterials[m].m_pSpecularTexture = nullptr;
        pMaterials[m].m_hCpuDiffuse.ptr = 0;
        pMaterials[m].m_hCpuNormal.ptr = 0;
        pMaterials[m].m_hCpuSpecular.ptr = 0;
        
        // mark the index into the heap for this material
        m_pSRVHeapIndex[m] = *pCurrentHeapIndex;
        *pCurrentHeapIndex += c_numSlots;
    }

    if (pLoaderCallbacks && pLoaderCallbacks->m_pCreateTextureFromFile)
    {
        for (UINT m = 0; m < numMaterials; m++)
        {
            WCHAR   tmp[1024];
            size_t  dummy;

            D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            
            // load textures
            // always create an empty view in case resource load fails leaving the field uninitialized
            if (m_pSRVHeap != nullptr && static_cast<ID3D12DescriptorHeap*>(*m_pSRVHeap) != nullptr)
            {
                m_loadingSlot = 0;
                pMaterials[m].m_hCpuDiffuse = m_pSRVHeap->hCPU(m_pSRVHeapIndex[m] + m_loadingSlot);
                pDevice->CreateShaderResourceView(nullptr, &descSRV, pMaterials[m].m_hCpuDiffuse);

                if (pMaterials[m].m_strDiffuseTexture[0] != 0)
                {
                    mbstowcs_s(&dummy, tmp, pMaterials[m].m_strDiffuseTexture, _countof(tmp));
                    XSF_RETURN_IF_FAILED(pLoaderCallbacks->m_pCreateTextureFromFile(m_pSample, &m_uploadHeap, tmp, &pMaterials[m].m_pDiffuseTexture, pMaterials[m].m_hCpuDiffuse, pLoaderCallbacks->m_pContext));
                }
            
                m_loadingSlot = 1;
                pMaterials[m].m_hCpuNormal = m_pSRVHeap->hCPU(m_pSRVHeapIndex[m] + m_loadingSlot);
                pDevice->CreateShaderResourceView(nullptr, &descSRV, pMaterials[m].m_hCpuNormal);
      
                if (pMaterials[m].m_strNormalTexture[0] != 0)
                {
                    mbstowcs_s(&dummy, tmp, pMaterials[m].m_strNormalTexture, _countof(tmp));
                    XSF_RETURN_IF_FAILED(pLoaderCallbacks->m_pCreateTextureFromFile(m_pSample, &m_uploadHeap, tmp, &pMaterials[m].m_pNormalTexture, pMaterials[m].m_hCpuNormal, pLoaderCallbacks->m_pContext));
                }
            
                m_loadingSlot = 2;
                pMaterials[m].m_hCpuSpecular = m_pSRVHeap->hCPU(m_pSRVHeapIndex[m] + m_loadingSlot);
                pDevice->CreateShaderResourceView(nullptr, &descSRV, pMaterials[m].m_hCpuSpecular);

                if (pMaterials[m].m_strSpecularTexture[0] != 0)
                {
                    mbstowcs_s(&dummy, tmp, pMaterials[m].m_strSpecularTexture, _countof(tmp));
                    XSF_RETURN_IF_FAILED(pLoaderCallbacks->m_pCreateTextureFromFile(m_pSample, &m_uploadHeap, tmp, &pMaterials[m].m_pSpecularTexture, pMaterials[m].m_hCpuSpecular, pLoaderCallbacks->m_pContext));
                }
            }
        }
    }
    else
    {
        XSF::DebugPrint("No texture loader specified, Use DefaultCreateTextureFromFile if that's not intended\n");
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::CreateVertexBuffer(VBHeader* pHeader, VOID* pVertices, const Callbacks* pLoaderCallbacks)
{
    VERBOSEATGPROFILETHIS;

    pHeader->DataOffset = 0;

    // Vertex Buffer
    const D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT>(pHeader->SizeBytes));

    if (pLoaderCallbacks && pLoaderCallbacks->m_pCreateVertexBuffer)
    {
        XSF_RETURN_IF_FAILED(pLoaderCallbacks->m_pCreateVertexBuffer(m_pSample, &m_uploadHeap, &pHeader->pVB, bufferDesc, static_cast<UINT>(pHeader->StrideBytes), pVertices, pLoaderCallbacks->m_pContext));
    }
    else
    {
        DebugPrint("No vertex buffer creation function specified, Use DefaultCreateBuffer if that's not intended\n");
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::CreateIndexBuffer(IBHeader* pHeader, VOID* pIndices, const Callbacks* pLoaderCallbacks)
{
    VERBOSEATGPROFILETHIS;

    pHeader->DataOffset = 0;

    //Index Buffer
    const D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT>(pHeader->SizeBytes));
    
    if (pLoaderCallbacks && pLoaderCallbacks->m_pCreateIndexBuffer)
    {
        XSF_RETURN_IF_FAILED(pLoaderCallbacks->m_pCreateIndexBuffer(m_pSample, &m_uploadHeap, &pHeader->pIB, bufferDesc, 0, pIndices, pLoaderCallbacks->m_pContext));
    }
    else
    {
        DebugPrint("No index buffer creation function specified, Use DefaultCreateBuffer if that's not intended\n");
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::CreateFromFile(const SampleFramework* const pSample, const WCHAR* szFileName,
                                  const Callbacks* pLoaderCallbacks, ID3D12DescriptorHeap *pSrvHeap, UINT *pSrvHeapIndex)
{
    VERBOSEATGPROFILETHIS;

    std::vector<BYTE> temp;
    XSF_RETURN_IF_FAILED(LoadBlob(szFileName, temp));
    
    return CreateFromMemory(pSample, &temp[0], static_cast<UINT>(temp.size()), TRUE, pLoaderCallbacks, pSrvHeap, pSrvHeapIndex);
}

//--------------------------------------------------------------------------------------
static
void GenerateNormals(const USHORT* pIndices, UINT numTris, const FLOAT* pPosBase, UINT posStride, FLOAT* pNormalBase, UINT normalStride, UINT numVerts)
{
    std::vector<XMFLOAT3> vertNormal(numVerts);

    memset(&vertNormal[0], 0, sizeof(vertNormal[0]) * numVerts);

    posStride /= 4;
    normalStride /= 4;

    for (UINT i = 0; i < numTris; ++i)
    {
        const UINT i0 = pIndices[i * 3 + 0];
        const UINT i1 = pIndices[i * 3 + 1];
        const UINT i2 = pIndices[i * 3 + 2];

        const XMVECTOR va = XMLoadFloat3((XMFLOAT3*)&pPosBase[i0 * posStride]);
        const XMVECTOR vb = XMLoadFloat3((XMFLOAT3*)&pPosBase[i1 * posStride]);
        const XMVECTOR vc = XMLoadFloat3((XMFLOAT3*)&pPosBase[i2 * posStride]);

        const XMVECTOR dc = XMVectorSubtract(vc, va);
        const XMVECTOR db = XMVectorSubtract(vb, va);

        const XMVECTOR normal = XMVector3Normalize(XMVector3Cross(db, dc));

        const XMVECTOR vn0 = XMLoadFloat3((XMFLOAT3*)&vertNormal[i0]);
        const XMVECTOR vn1 = XMLoadFloat3((XMFLOAT3*)&vertNormal[i1]);
        const XMVECTOR vn2 = XMLoadFloat3((XMFLOAT3*)&vertNormal[i2]);

        const XMVECTOR vns0 = XMVectorAdd(vn0, normal);
        const XMVECTOR vns1 = XMVectorAdd(vn1, normal);
        const XMVECTOR vns2 = XMVectorAdd(vn2, normal);

        XMStoreFloat3(&vertNormal[i0], vns0);
        XMStoreFloat3(&vertNormal[i1], vns1);
        XMStoreFloat3(&vertNormal[i2], vns2);
    }

    for (UINT i = 0; i < numVerts; ++i)
    {
        const XMVECTOR v = XMLoadFloat3((XMFLOAT3*)&vertNormal[i]);
        XMStoreFloat3((XMFLOAT3*)&pNormalBase[i * normalStride], XMVector3Normalize(v));
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::CreateFromMemory(const SampleFramework* const pSample, VOID* pData, UINT DataBytes, BOOL bCopyData, 
                                    const Callbacks* pLoaderCallbacks0, ID3D12DescriptorHeap *pSrvHeap, UINT *pSrvHeapIndex)
{
    VERBOSEATGPROFILETHIS;

    using namespace Detail;

    m_pSample = pSample;
    D3DDevice* const pDevice = pSample->GetDevice();
    ID3D12Fence* const pFence = pSample->GetFence();
    D3DCommandList* pCmdList = pSample->GetCommandList();
    D3DCommandAllocator *pCmdAllocator = pSample->GetCommandAllocator();

    XSF_ASSERT(pDevice);
    XSF_ASSERT(pFence);
    XSF_ASSERT(pCmdList);
    XSF_ASSERT(pCmdAllocator);
    XSF_ASSERT(pData);

    if (!pDevice || !pFence || !pCmdList || !pCmdAllocator || !pData)
    {
        return E_INVALIDARG;
    }

    XSF_RETURN_IF_FAILED(m_uploadHeap.Initialize(pDevice, pFence, m_uploadHeapSizeMB * 1024 * 1024, false, 1, L"Mesh::UploadHeap"));
    (const_cast<SampleFramework*>(pSample))->ManageUploadHeap(&m_uploadHeap);

    Callbacks tmp;
    const Callbacks* pLoaderCallbacks = pLoaderCallbacks0 ? pLoaderCallbacks0 : &tmp;

    // Set outstanding resources to zero
    m_NumOutstandingResources = 0;

    if (bCopyData)
    {
        m_staticMeshData.resize(DataBytes);
        memcpy(&m_staticMeshData[0], pData, DataBytes);
        m_pMeshData = &m_staticMeshData[0];
    } 
    else
    {
        m_pMeshData = reinterpret_cast<BYTE*>(pData);
    }

    // Pointer fixup
    m_pMeshHeader = reinterpret_cast<Header*>(m_pMeshData);
    m_pVertexBufferArray = reinterpret_cast<VBHeader*>(m_pMeshData + m_pMeshHeader->VertexStreamHeadersOffset);
    m_pIndexBufferArray = reinterpret_cast<IBHeader*>(m_pMeshData + m_pMeshHeader->IndexStreamHeadersOffset);
    m_pMeshArray = reinterpret_cast<MeshArray*>(m_pMeshData + m_pMeshHeader->MeshDataOffset);
    m_pSubsetArray = reinterpret_cast<Subset*>(m_pMeshData + m_pMeshHeader->SubsetDataOffset);
    m_pFrameArray = reinterpret_cast<Frame*>(m_pMeshData + m_pMeshHeader->FrameDataOffset);
    m_pMaterialArray = reinterpret_cast<Material*>(m_pMeshData + m_pMeshHeader->MaterialDataOffset);

    // Setup subsets
    for (UINT i = 0; i < m_pMeshHeader->NumMeshes; i++)
    {
        m_pMeshArray[i].pSubsets = reinterpret_cast<UINT*>(m_pMeshData + m_pMeshArray[i].SubsetOffset);
        m_pMeshArray[i].pFrameInfluences = reinterpret_cast<UINT*>(m_pMeshData + m_pMeshArray[i].FrameInfluenceOffset);
    }

    HRESULT hr;

    // error condition
    if (m_pMeshHeader->Version != SDKMESH_FILE_VERSION)
    {
        return E_NOINTERFACE;
    }

    // Setup buffer data pointer
    BYTE* pBufferData = m_pMeshData + m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Get the start of the buffer data
    UINT64 BufferDataStart = m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Create VBs
    m_ppVertices = new BYTE*[m_pMeshHeader->NumVertexBuffers];
    m_pVBView = new D3D12_VERTEX_BUFFER_VIEW[m_pMeshHeader->NumVertexBuffers];
    for (UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
    {
        m_ppVertices[i] = reinterpret_cast<BYTE*>(pBufferData + (m_pVertexBufferArray[i].DataOffset - BufferDataStart));
        ZeroMemory(&m_pVBView[i], sizeof(D3D12_VERTEX_BUFFER_VIEW));
    }

    // Create IBs
    m_ppIndices = new BYTE*[m_pMeshHeader->NumIndexBuffers];
    m_pIBView = new D3D12_INDEX_BUFFER_VIEW[m_pMeshHeader->NumIndexBuffers];
    for (UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
    {
        m_ppIndices[i] = reinterpret_cast<BYTE*>(pBufferData + (m_pIndexBufferArray[i].DataOffset - BufferDataStart));
        ZeroMemory(&m_pIBView[i], sizeof(D3D12_INDEX_BUFFER_VIEW));
    }

    // Optionally generate normals
    for (UINT i = 0; i < m_pMeshHeader->NumMeshes; ++i)
    {
        const UINT ib = m_pMeshArray[i].IndexBuffer;

        FLOAT* pNormalBase = nullptr;
        UINT    normalStride = 0;
        const FLOAT* pPosBase = nullptr;
        UINT    posStride = 0;

        // Find where to stick the normals
        for (UINT j = 0; j < m_pMeshArray[i].NumVertexBuffers; ++j)
        {
            const UINT vb = m_pMeshArray[i].VertexBuffers[j];
            const LEGACY_D3DVERTEXELEMENT9* pDecl = m_pVertexBufferArray[vb].Decl;

            for (UINT k = 0; k < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++k)
            {
                if (pDecl[k].Stream == 0xff)
                    break;

                if (pDecl[k].Usage == 3/*D3DDECLUSAGE_NORMAL*/)
                {
                    pNormalBase = reinterpret_cast<FLOAT*>(reinterpret_cast<BYTE*>(m_ppVertices[vb]) + pDecl[k].Offset);
                    normalStride = static_cast<UINT>(m_pVertexBufferArray[vb].StrideBytes);
                }

                if( pDecl[k].Usage == 0/*D3DDECLUSAGE_POSITION*/)
                {
                    pPosBase = reinterpret_cast<FLOAT*>(reinterpret_cast<BYTE*>(m_ppVertices[vb]) + pDecl[k].Offset);
                    posStride = static_cast<UINT>(m_pVertexBufferArray[vb].StrideBytes);
                }
            }
        }

        // Either no normals or positions
        if (!posStride || !normalStride)
            continue;

        // Generate normals in place
        bool generateNormals = false;
        if (generateNormals)
        {
            GenerateNormals( reinterpret_cast<USHORT*>(m_ppIndices[ib]), static_cast<UINT>(m_pIndexBufferArray[ib].NumIndices / 3),
                                pPosBase, posStride, pNormalBase, normalStride, static_cast<UINT>(m_pVertexBufferArray[0].NumVertices));
        }
    }

    // Create VBs
    for (UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
    {
        CreateVertexBuffer(&m_pVertexBufferArray[i], m_ppVertices[i], pLoaderCallbacks);

        m_pVBView[i].SizeInBytes = static_cast<UINT>(m_pVertexBufferArray[i].SizeBytes);
        m_pVBView[i].StrideInBytes = static_cast<UINT>(m_pVertexBufferArray[i].StrideBytes);
        m_pVBView[i].BufferLocation = m_pVertexBufferArray[i].pVB->GetGPUVirtualAddress();
    }

    // Create IBs
    for (UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
    {
        CreateIndexBuffer(&m_pIndexBufferArray[i], m_ppIndices[i], pLoaderCallbacks);

        m_pIBView[i].Format = (m_pIndexBufferArray[i].IndexType == Detail::IT_16BIT)? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        m_pIBView[i].SizeInBytes = static_cast<UINT>(m_pIndexBufferArray[i].SizeBytes);
        m_pIBView[i].BufferLocation = m_pIndexBufferArray[i].pIB->GetGPUVirtualAddress();
    }

    // Create D3D resources

    // Load Materials
    m_pSRVHeap = new DescriptorHeapWrapper(pSrvHeap);
    m_pSRVHeapIndex = new UINT[m_pMeshHeader->NumMaterials];
    XSF_RETURN_IF_FAILED(LoadMaterials(m_pMaterialArray, m_pMeshHeader->NumMaterials, pLoaderCallbacks, pSrvHeapIndex));
    
    hr = E_OUTOFMEMORY;
    // Create a place to store our bind pose frame matrices
    m_pBindPoseFrameMatrices = reinterpret_cast<XMMATRIX*>(_aligned_malloc(sizeof(XMMATRIX) * m_pMeshHeader->NumFrames, 16));
    if (!m_pBindPoseFrameMatrices)
        return hr;

    // Create a place to store our transformed frame matrices
    m_pTransformedFrameMatrices = reinterpret_cast<XMMATRIX*>(_aligned_malloc(sizeof(XMMATRIX) * m_pMeshHeader->NumFrames, 16));
    if (!m_pTransformedFrameMatrices)
        return hr;

    m_pWorldPoseFrameMatrices = reinterpret_cast<XMMATRIX*>(_aligned_malloc( sizeof(XMMATRIX) * m_pMeshHeader->NumFrames, 16));
    if (!m_pWorldPoseFrameMatrices)
        return hr;

    hr = S_OK;

    // update bounding volume 
    for (UINT meshi = 0; meshi < m_pMeshHeader->NumMeshes; ++meshi)
    {
        FLOAT   lower[3];
        FLOAT   upper[3];

        lower[0] = lower[1] = lower[2] = FLT_MAX;
        upper[0] = upper[1] = upper[2] = -FLT_MAX;

        MeshArray* currentMesh = &m_pMeshArray[meshi];

        const INT indsize = (m_pIndexBufferArray[currentMesh->IndexBuffer].IndexType == Detail::IT_16BIT)? 2 : 4;

        for (UINT subset = 0; subset < currentMesh->NumSubsets; subset++)
        {
            Subset* pSubset = GetSubset(meshi, subset);

            D3D12_PRIMITIVE_TOPOLOGY PrimType = GetPrimitiveType(static_cast<SDKMESH_PRIMITIVE_TYPE>(pSubset->PrimitiveType));
            XSF_ASSERT(PrimType == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST || PrimType == D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST || PrimType == D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST);   // only triangle lists and patch lists are handled.

            // Store back
            pSubset->PrimitiveType = PrimType;

            UINT IndexCount = static_cast<UINT>(pSubset->IndexCount);
            UINT IndexStart = static_cast<UINT>(pSubset->IndexStart);

            UINT *ind = reinterpret_cast<UINT*>(m_ppIndices[currentMesh->IndexBuffer]);
            FLOAT *verts = reinterpret_cast<FLOAT*>(m_ppVertices[currentMesh->VertexBuffers[0]]);
            UINT stride = static_cast<UINT>(m_pVertexBufferArray[currentMesh->VertexBuffers[0]].StrideBytes);
            XSF_ASSERT(stride % 4 == 0);
            stride /= 4;
            for (UINT vertind = IndexStart; vertind < IndexStart + IndexCount; ++vertind) //TODO: test 16 bit and 32 bit
            {
                UINT current_ind=0;
                if (indsize == 2)
                {
                    UINT ind_div2 = vertind / 2;
                    current_ind = ind[ind_div2];
                    if (vertind %2 ==0)
                    {
                        current_ind = current_ind << 16;
                        current_ind = current_ind >> 16;
                    }
                    else
                    {
                        current_ind = current_ind >> 16;
                    }
                }
                else
                {
                    current_ind = ind[vertind];
                }

                FLOAT* pt = reinterpret_cast<FLOAT*>(&(verts[stride * current_ind]));
                for (UINT i = 0; i < 3; ++i)
                {
                    if (pt[i] < lower[i])
                        lower[i] = pt[i];

                    if (pt[i] > upper[i])
                        upper[i] = pt[i];
                }
            }
        }

        for (UINT i = 0; i < 3; ++i)
        {
            const FLOAT h = 0.5f * (upper[i] - lower[i]);

            currentMesh->BoundingBoxCenter[i] = lower[i] + h;
            currentMesh->BoundingBoxExtents[i] = h;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID XSF::Mesh::RenderMesh(D3DCommandList* pCmdList, UINT uMesh, const RenderingOptions& options) const
{
    VERBOSEATGPROFILETHIS;

    using namespace Detail;

    MeshArray* pMesh = &m_pMeshArray[uMesh];
    if (pMesh->NumVertexBuffers > D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
        return;

    pCmdList->IASetIndexBuffer(&m_pIBView[pMesh->IndexBuffer]);

    D3D12_VERTEX_BUFFER_VIEW pVBView[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    for (UINT i = 0; i < pMesh->NumVertexBuffers; ++i)
    {
        pVBView[i] = m_pVBView[pMesh->VertexBuffers[i]];
    }
    pCmdList->IASetVertexBuffers(0, pMesh->NumVertexBuffers, pVBView);

    Subset* pSubset = nullptr;
    Material* pMat = nullptr;

    for (UINT subset = 0; subset < pMesh->NumSubsets; subset++)
    {
        pSubset = &m_pSubsetArray[pMesh->pSubsets[subset]];

        pCmdList->IASetPrimitiveTopology(options.enableTess? D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST : static_cast<D3D12_PRIMITIVE_TOPOLOGY>(pSubset->PrimitiveType));

        pMat = &m_pMaterialArray[pSubset->MaterialID];

        if (options.pSetShaderResourcesCallback != nullptr)
        {
            options.pSetShaderResourcesCallback(m_pSample->GetDevice(), pCmdList, options.uSrvSlot, *m_pSRVHeap, m_pSRVHeapIndex[pSubset->MaterialID]);
        }
        else
        {
            if (options.uSrvSlot != INVALID_SAMPLER_SLOT)
                pCmdList->SetGraphicsRootDescriptorTable(options.uSrvSlot, m_pSRVHeap->hGPU(m_pSRVHeapIndex[pSubset->MaterialID]));
        }

        const UINT IndexCount = static_cast<UINT>(pSubset->IndexCount);
        const UINT IndexStart = static_cast<UINT>(pSubset->IndexStart);
        const UINT VertexStart = static_cast<UINT>(pSubset->VertexStart);

        pCmdList->DrawIndexedInstanced(IndexCount, options.numInstances, IndexStart, VertexStart, 0);
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID XSF::Mesh::RenderFrame(D3DCommandList* pCmdList, UINT uFrame, const RenderingOptions& options) const
{
    VERBOSEATGPROFILETHIS;

    XSF_ASSERT(m_pMeshData);
    XSF_ASSERT(m_pFrameArray);

    if (!m_pMeshData || !m_pFrameArray)
        return;

    if (m_pFrameArray[uFrame].Mesh != INVALID_MESH)
    {
        if (options.pCallback)
        {
            if (options.pCallback(pCmdList, &m_pFrameArray[uFrame], options.pCallbackContext))
            {
                RenderMesh(pCmdList, m_pFrameArray[uFrame].Mesh, options);
            }
        }
        else
        {
            RenderMesh(pCmdList, m_pFrameArray[uFrame].Mesh, options);
        }
    }

    // Render our children
    if (m_pFrameArray[uFrame].ChildFrame != INVALID_FRAME)
    {
        RenderFrame(pCmdList, m_pFrameArray[uFrame].ChildFrame, options);
    }

    // Render our siblings
    if (m_pFrameArray[uFrame].SiblingFrame != INVALID_FRAME)
    {
        RenderFrame(pCmdList, m_pFrameArray[uFrame].SiblingFrame, options);
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID XSF::Mesh::Render(D3DCommandList* pCmdList, const RenderingOptions& options) const
{
    XSFScopedNamedEventFunc(pCmdList, XSF_COLOR_MESH_RENDER);

    RenderFrame(pCmdList, 0, options);
}

//--------------------------------------------------------------------------------------
VOID XSF::Mesh::Destroy()
{
    using namespace Detail;

    if (m_pMeshData)
    {
        if (m_pMaterialArray)
        {
            for (UINT m = 0; m < m_pMeshHeader->NumMaterials; m++)
            {
                if (!IsErrorResource(m_pMaterialArray[m].m_pDiffuseTexture))
                {
                    XSF_SAFE_RELEASE (m_pMaterialArray[m].m_pDiffuseTexture);
                }
                if (!IsErrorResource(m_pMaterialArray[m].m_pNormalTexture))
                {
                    XSF_SAFE_RELEASE(m_pMaterialArray[m].m_pNormalTexture);
                }
                if (!IsErrorResource(m_pMaterialArray[m].m_pSpecularTexture))
                {
                    XSF_SAFE_RELEASE(m_pMaterialArray[m].m_pSpecularTexture);
                }
            }

            XSF_SAFE_DELETE_ARRAY(m_pSRVHeapIndex);
        }
    }
    XSF_SAFE_DELETE(m_pSRVHeap);

    m_pMeshData = nullptr;
    m_staticMeshData.clear();
    XSF_SAFE_DELETE_ARRAY(m_pAnimationData);
    XSF_SAFE_ALIGNED_FREE(m_pBindPoseFrameMatrices);
    XSF_SAFE_ALIGNED_FREE(m_pTransformedFrameMatrices);
    XSF_SAFE_ALIGNED_FREE(m_pWorldPoseFrameMatrices);
    
    XSF_SAFE_DELETE_ARRAY(m_ppVertices);
    XSF_SAFE_DELETE_ARRAY(m_ppIndices);
    XSF_SAFE_DELETE_ARRAY(m_pVBView);
    XSF_SAFE_DELETE_ARRAY(m_pIBView);

    if (m_pMeshHeader)
    {
        for (UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
        {
            XSF_SAFE_RELEASE(m_pVertexBufferArray[i].pVB);
        }

        for (UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
        {
            XSF_SAFE_RELEASE(m_pIndexBufferArray[i].pIB);
        }
    }

    m_pMeshHeader = nullptr;
    m_pVertexBufferArray = nullptr;
    m_pIndexBufferArray = nullptr;
    m_pMeshArray = nullptr;
    m_pSubsetArray = nullptr;
    m_pFrameArray = nullptr;
    m_pMaterialArray = nullptr;

    m_pAnimationHeader = nullptr;
    m_pAnimationFrameData = nullptr;
}

//--------------------------------------------------------------------------------------
// transform bind pose frame using a recursive traversal
//--------------------------------------------------------------------------------------
VOID XM_CALLCONV XSF::Mesh::TransformBindPoseFrame(UINT uFrame, CXMMATRIX ParentWorld)
{
    if (!m_pBindPoseFrameMatrices)
        return;

    // Transform ourselves
    XMMATRIX LocalWorld = XMMatrixMultiply(XMMATRIX(&m_pFrameArray[uFrame].Matrix.m[0][0]), ParentWorld);
    m_pBindPoseFrameMatrices[uFrame] = LocalWorld;

    // Transform our siblings
    if (m_pFrameArray[uFrame].SiblingFrame != INVALID_FRAME)
        TransformBindPoseFrame(m_pFrameArray[uFrame].SiblingFrame, ParentWorld);

    // Transform our children
    if (m_pFrameArray[uFrame].ChildFrame != INVALID_FRAME)
        TransformBindPoseFrame(m_pFrameArray[uFrame].ChildFrame, LocalWorld);
}

//--------------------------------------------------------------------------------------
// transform frame using a recursive traversal
//--------------------------------------------------------------------------------------
VOID XM_CALLCONV XSF::Mesh::TransformFrame(UINT uFrame, CXMMATRIX ParentWorld, double fTime)
{
    // Get the tick data
    XMMATRIX LocalTransform;
    UINT uTick = GetAnimationKeyFromTime(fTime);

    if (INVALID_ANIMATION_DATA != m_pFrameArray[uFrame].AnimationDataIndex)
    {
        AnimFrameData* pFrameData = &m_pAnimationFrameData[m_pFrameArray[uFrame].AnimationDataIndex];
        AnimFrameData::AnimDataItem* pData = &pFrameData->pAnimationData[uTick];

        // turn it into a matrix (Ignore scaling for now)
        XMFLOAT3 parentPos = XMFLOAT3(pData->Translation);
        XMMATRIX mTranslate = XMMatrixTranslation(parentPos.x, parentPos.y, parentPos.z);

        const XMFLOAT4 cDataOrientation = XMFLOAT4(pData->Orientation);
        XMVECTOR quat = XMLoadFloat4(&cDataOrientation);
        XMMATRIX mQuat;
        if (pData->Orientation[3] == 0 && pData->Orientation[0] == 0 && pData->Orientation[1] == 0 && pData->Orientation[2] == 0)
            quat = XMQuaternionIdentity();
        quat = XMQuaternionNormalize(quat);
        mQuat = XMMatrixRotationQuaternion(quat);
        LocalTransform = (mQuat * mTranslate);
    }
    else
    {
        LocalTransform = XMMATRIX(&m_pFrameArray[uFrame].Matrix.m[0][0]);
    }

    // Transform ourselves
    XMMATRIX LocalWorld = XMMatrixMultiply(LocalTransform, ParentWorld);
    m_pTransformedFrameMatrices[uFrame] = LocalWorld;
    m_pWorldPoseFrameMatrices[uFrame] = LocalWorld;

    // Transform our siblings
    if (m_pFrameArray[uFrame].SiblingFrame != INVALID_FRAME)
        TransformFrame( m_pFrameArray[uFrame].SiblingFrame, ParentWorld, fTime);

    // Transform our children
    if (m_pFrameArray[uFrame].ChildFrame != INVALID_FRAME)
        TransformFrame( m_pFrameArray[uFrame].ChildFrame, LocalWorld, fTime);
}

//--------------------------------------------------------------------------------------
// transform frame assuming that it is an absolute transformation
//--------------------------------------------------------------------------------------
VOID XSF::Mesh::TransformFrameAbsolute(UINT uFrame, double fTime)
{
    XMMATRIX mTrans1;
    XMMATRIX mTrans2;
    XMMATRIX mRot1;
    XMMATRIX mRot2;
    XMVECTOR quat1;
    XMVECTOR quat2;
    XMMATRIX mInvTo;
    XMMATRIX mFrom;

    UINT uTick = GetAnimationKeyFromTime(fTime);

    if (INVALID_ANIMATION_DATA != m_pFrameArray[uFrame].AnimationDataIndex)
    {
        AnimFrameData* pFrameData = &m_pAnimationFrameData[m_pFrameArray[uFrame].AnimationDataIndex];
        AnimFrameData::AnimDataItem* pData = &pFrameData->pAnimationData[uTick];
        AnimFrameData::AnimDataItem* pDataOrig = &pFrameData->pAnimationData[0];

        mTrans1 = XMMatrixTranslation(-pDataOrig->Translation[0], -pDataOrig->Translation[1], -pDataOrig->Translation[2]);
        mTrans2 = XMMatrixTranslation(pData->Translation[0], pData->Translation[1], pData->Translation[2]);

        const XMFLOAT4 dataOrigOrientation = XMFLOAT4(pDataOrig->Orientation);
        quat1 = XMLoadFloat4(&dataOrigOrientation);
        quat1 = XMQuaternionInverse(quat1);
        mRot1 = XMMatrixRotationQuaternion(quat1);
        mInvTo = mTrans1 * mRot1;

        const XMFLOAT4 dataOrientation = XMFLOAT4(pData->Orientation);
        quat2 = XMLoadFloat4(&dataOrientation);
        mRot2 = XMMatrixRotationQuaternion(quat2);
        mFrom = mRot2 * mTrans2;

        XMMATRIX mOutput = mInvTo * mFrom;
        m_pTransformedFrameMatrices[uFrame] = mOutput;
    }
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::LoadAnimation(const WCHAR* const szFileName)
{
    std::vector<BYTE> animation;
    XSF_RETURN_IF_FAILED(LoadBlob(szFileName, animation));
    
    // Header
    AnimFileHeader fileheader;
    memcpy(&fileheader, &animation[0], sizeof(AnimFileHeader));

    //allocate
    UINT64 BaseOffset = sizeof(AnimFileHeader);
    m_pAnimationData = new (std::nothrow) BYTE[static_cast<size_t>(BaseOffset + fileheader.AnimationDataSize)];
    if (!m_pAnimationData)
    {
        return E_OUTOFMEMORY;
    }

    // read it all in
    memcpy(m_pAnimationData, &animation[0], static_cast<size_t>(BaseOffset + fileheader.AnimationDataSize));
    
    // pointer fixup
    m_pAnimationHeader = reinterpret_cast<AnimFileHeader*>(m_pAnimationData);
    m_pAnimationFrameData = reinterpret_cast<AnimFrameData*>(m_pAnimationData + m_pAnimationHeader->AnimationDataOffset);

    for (UINT i = 0; i < m_pAnimationHeader->NumFrames; i++)
    {
        m_pAnimationFrameData[i].pAnimationData = reinterpret_cast<AnimFrameData::AnimDataItem*>(m_pAnimationData + m_pAnimationFrameData[i].DataOffset + BaseOffset);
        Frame* pFrame = FindFrame(m_pAnimationFrameData[i].FrameName);
        if (pFrame)
        {
            pFrame->AnimationDataIndex = i;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// transform the bind pose
//--------------------------------------------------------------------------------------
VOID XM_CALLCONV XSF::Mesh::TransformBindPose(CXMMATRIX World)
{
    TransformBindPoseFrame(0, World);
}

//--------------------------------------------------------------------------------------
// transform the mesh frames according to the animation for time fTime
//--------------------------------------------------------------------------------------
VOID XM_CALLCONV XSF::Mesh::TransformMesh(CXMMATRIX World, double fTime)
{
    if (m_pAnimationHeader == nullptr || Detail::FTT_RELATIVE == m_pAnimationHeader->FrameTransformType)
    {
        TransformFrame(0, World, fTime);

        // For each frame, move the transform to the bind pose, then
        // move it to the final position
        XMMATRIX mInvBindPose;
        XMMATRIX mFinal;
        for (UINT i = 0; i < m_pMeshHeader->NumFrames; i++)
        {
            mInvBindPose = XMMatrixInverse(nullptr, m_pBindPoseFrameMatrices[i]);
            mFinal = mInvBindPose * m_pTransformedFrameMatrices[i];
            m_pTransformedFrameMatrices[i] = mFinal;
        }
    }
    else if (Detail::FTT_ABSOLUTE == m_pAnimationHeader->FrameTransformType)
    {
        for (UINT i = 0; i < m_pAnimationHeader->NumFrames; i++)
            TransformFrameAbsolute(i, fTime);
    }
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumInfluences(UINT uMesh) const
{
    return m_pMeshArray[uMesh].NumFrameInfluences;
}

//--------------------------------------------------------------------------------------
CXMMATRIX XSF::Mesh::GetMeshInfluenceMatrix(UINT uMesh, UINT uInfluence) const
{
    UINT uFrame = m_pMeshArray[uMesh].pFrameInfluences[uInfluence];
    return m_pTransformedFrameMatrices[uFrame];
}

CXMMATRIX XSF::Mesh::GetWorldMatrix(UINT uFrameIndex) const
{
    return m_pWorldPoseFrameMatrices[uFrameIndex];
}

CXMMATRIX XSF::Mesh::GetInfluenceMatrix(UINT uFrameIndex) const
{
    return m_pTransformedFrameMatrices[uFrameIndex];
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetAnimationKeyFromTime(double fTime) const
{
    if (m_pAnimationHeader == nullptr)
    {
        return 0;
    }

    UINT uTick = static_cast<UINT>(m_pAnimationHeader->AnimationFPS * fTime);

    uTick = uTick % (m_pAnimationHeader->NumAnimationKeys - 1);
    uTick ++;

    return uTick;
}

BOOL XSF::Mesh::GetAnimationProperties(UINT* pNumKeys, FLOAT* pFrameTime) const
{
    if (m_pAnimationHeader == nullptr)
    {
        return FALSE;
    }

    *pNumKeys = m_pAnimationHeader->NumAnimationKeys;
    *pFrameTime = 1.0f / static_cast<FLOAT>(m_pAnimationHeader->AnimationFPS);

    return TRUE;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT XSF::Mesh::GetIBFormat(UINT uIB) const
{
    using namespace Detail;

    switch (m_pIndexBufferArray[uIB].IndexType)
    {
        default:
        case IT_16BIT:
            return DXGI_FORMAT_R16_UINT;
        case IT_32BIT:
            return DXGI_FORMAT_R32_UINT;
    };
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
UINT  XSF::Mesh::GetVBFormat(UINT uVB, D3D12_INPUT_ELEMENT_DESC* pDesc) const
{
    using namespace Detail;

    static const CHAR* semantics[] =
    {
        "POSITION",     // D3DDECLUSAGE_POSITION = 0,
        "BLENDWEIGHT",  // D3DDECLUSAGE_BLENDWEIGHT,   // 1
        "BLENDINDICES", // D3DDECLUSAGE_BLENDINDICES,  // 2
        "NORMAL",       // D3DDECLUSAGE_NORMAL,        // 3
        "PSIZE",        // D3DDECLUSAGE_PSIZE,         // 4
        "TEXCOORD",     // D3DDECLUSAGE_TEXCOORD,      // 5
        "TANGENT",      // D3DDECLUSAGE_TANGENT,       // 6
        "BINORMAL",     // D3DDECLUSAGE_BINORMAL,      // 7
        "TESSFACTOR",   // D3DDECLUSAGE_TESSFACTOR,    // 8
        "POSITIONT",    // D3DDECLUSAGE_POSITIONT,     // 9
        "COLOR",        // D3DDECLUSAGE_COLOR,         // 10
        "FOG",          // D3DDECLUSAGE_FOG,           // 11
        "DEPTH",        // D3DDECLUSAGE_DEPTH,         // 12
        "SAMPLE",       // D3DDECLUSAGE_SAMPLE,        // 13
    };

    static const DXGI_FORMAT    formats[] =
    {
        DXGI_FORMAT_R32_FLOAT,              // D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
        DXGI_FORMAT_R32G32_FLOAT,           // D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
        DXGI_FORMAT_R32G32B32_FLOAT,        // D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
        DXGI_FORMAT_R32G32B32A32_FLOAT,     // D3DDECLTYPE_FLOAT4    =  3,  // 4D float

        // no direct equivalent in vanilla dx11 so beware
        DXGI_FORMAT_R8G8B8A8_UNORM,         // D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range
                                            // Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
        DXGI_FORMAT_R8G8B8A8_UINT,          // D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
        DXGI_FORMAT_R16G16_SINT,            // D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
        DXGI_FORMAT_R16G16B16A16_SINT,      // D3DDECLTYPE_SHORT4    =  7,  // 4D signed short

    // The following types are valid only with vertex shaders >= 2.0

        DXGI_FORMAT_R8G8B8A8_UNORM,         // D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
        DXGI_FORMAT_R16G16_SNORM,           // D3DDECLTYPE_SHORT2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
        DXGI_FORMAT_R16G16B16A16_SNORM,     // D3DDECLTYPE_SHORT4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
        DXGI_FORMAT_R16G16_UNORM,           // D3DDECLTYPE_USHORT2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
        DXGI_FORMAT_R16G16B16A16_UNORM,     // D3DDECLTYPE_USHORT4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
        DXGI_FORMAT_R10G10B10A2_UINT,       // D3DDECLTYPE_UDEC3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
        DXGI_FORMAT_R10G10B10A2_UNORM,      // D3DDECLTYPE_DEC3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
        DXGI_FORMAT_R16G16_FLOAT,           // D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
        DXGI_FORMAT_R16G16B16A16_FLOAT,     // D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
    };

    const LEGACY_D3DVERTEXELEMENT9* pDecl = m_pVertexBufferArray[uVB].Decl;

    UINT i;
    for (i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        if( pDecl[i].Stream == 0xff)
            break;

        pDesc[i].InputSlot = pDecl[i].Stream;
        pDesc[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        pDesc[i].InstanceDataStepRate = 0;

        pDesc[i].AlignedByteOffset = pDecl[i].Offset;

        XSF_ASSERT(pDecl[i].Usage < _countof(semantics));
        pDesc[i].SemanticName = semantics[ pDecl[i].Usage];
        pDesc[i].SemanticIndex = pDecl[i].UsageIndex;

        XSF_ASSERT(pDecl[i].Type < _countof(formats));
        pDesc[i].Format = formats[pDecl[i].Type];
    }

    return i;
}


//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetVBIndex(UINT uMesh, UINT uVB) const
{
    return m_pMeshArray[uMesh].VertexBuffers[uVB];
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetIBIndex(UINT uMesh) const
{
    return m_pMeshArray[uMesh].IndexBuffer;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumMeshes() const
{
    XSF_ASSERT(m_pMeshHeader);

    if (!m_pMeshHeader)
        return 0;
    return m_pMeshHeader->NumMeshes;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumMaterials() const
{
    XSF_ASSERT(m_pMeshHeader);

    if (!m_pMeshHeader)
        return 0;
    return m_pMeshHeader->NumMaterials;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumVBs() const
{
    XSF_ASSERT(m_pMeshHeader);

    if (!m_pMeshHeader)
        return 0;
    return m_pMeshHeader->NumVertexBuffers;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumIBs() const
{
    XSF_ASSERT(m_pMeshHeader);

    if (!m_pMeshHeader)
        return 0;
    return m_pMeshHeader->NumIndexBuffers;
}

//--------------------------------------------------------------------------------------
ID3D12Resource* XSF::Mesh::GetVBAt(UINT uVB) const
{
    return m_pVertexBufferArray[uVB].pVB;
}

//--------------------------------------------------------------------------------------
const D3D12_VERTEX_BUFFER_VIEW* XSF::Mesh::GetVBViewAt(UINT uVB) const
{
    return &m_pVBView[uVB];
}

//--------------------------------------------------------------------------------------
ID3D12Resource* XSF::Mesh::GetIBAt(UINT uIB) const
{
    return m_pIndexBufferArray[uIB].pIB;
}

//--------------------------------------------------------------------------------------
const D3D12_INDEX_BUFFER_VIEW* XSF::Mesh::GetIBViewAt(UINT uIB) const
{
    return &m_pIBView[uIB];
}

//--------------------------------------------------------------------------------------
const BYTE* XSF::Mesh::GetRawVerticesAt(UINT iVB) const
{
    return m_ppVertices[iVB];
}

//--------------------------------------------------------------------------------------
const BYTE* XSF::Mesh::GetRawIndicesAt(UINT iIB) const
{
    return m_ppIndices[iIB];
}

//--------------------------------------------------------------------------------------
const XSF::Mesh::Material* XSF::Mesh::GetMaterial(UINT uMaterial) const
{
    return &m_pMaterialArray[uMaterial];
}

//--------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE XSF::Mesh::GetDescriptorHandle(UINT iMaterial) const
{
    return m_pSRVHeap->hGPU(m_pSRVHeapIndex[iMaterial]);
}

//--------------------------------------------------------------------------------------
const XSF::Mesh::MeshArray* XSF::Mesh::GetMesh(UINT uMesh) const
{
    return &m_pMeshArray[uMesh];
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumSubsets(UINT uMesh) const
{
    return m_pMeshArray[uMesh].NumSubsets;
}

//--------------------------------------------------------------------------------------
const XSF::Mesh::Subset* XSF::Mesh::GetSubset(UINT uMesh, UINT uSubset) const
{
    return &m_pSubsetArray[m_pMeshArray[uMesh].pSubsets[uSubset]];
}

//--------------------------------------------------------------------------------------
XSF::Mesh::Subset* XSF::Mesh::GetSubset(UINT uMesh, UINT uSubset)
{
    return &m_pSubsetArray[m_pMeshArray[uMesh].pSubsets[uSubset]];
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetVertexStride(UINT uMesh, UINT uVB) const
{
    return static_cast<UINT>(m_pVertexBufferArray[m_pMeshArray[uMesh].VertexBuffers[uVB]].StrideBytes);
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumFrames() const
{
    return m_pMeshHeader->NumFrames;
}

//--------------------------------------------------------------------------------------
XSF::Mesh::Frame* XSF::Mesh::GetFrame(UINT uFrame) const
{
    XSF_ASSERT (uFrame < m_pMeshHeader->NumFrames);
    return &m_pFrameArray[uFrame];
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
XSF::Mesh::Frame* XSF::Mesh::FindFrame(const CHAR* const pszName) const
{
    for (UINT i = 0; i < m_pMeshHeader->NumFrames; i++)
    {
        if (_stricmp(m_pFrameArray[i].Name, pszName) == 0)
        {
            return &m_pFrameArray[i];
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------
HRESULT XSF::Mesh::SetFrameMatrix(const CHAR* pszName, const XMFLOAT4X4& Matrix)
{
    for (UINT i = 0; i < m_pMeshHeader->NumFrames; i++)
    {
        if (_stricmp( m_pFrameArray[i].Name, pszName) == 0)
        {
            m_pFrameArray[i].Matrix =  Matrix;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumVertices(UINT uMesh, UINT uVB) const
{
    return static_cast<UINT>(m_pVertexBufferArray[m_pMeshArray[uMesh].VertexBuffers[uVB]].NumVertices);
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetNumIndices(UINT uMesh) const
{
    return static_cast<UINT>(m_pIndexBufferArray[m_pMeshArray[uMesh].IndexBuffer].NumIndices);
}

//--------------------------------------------------------------------------------------
const FLOAT* XSF::Mesh::GetMeshBBoxCenter(UINT uMesh) const
{
    return m_pMeshArray[uMesh].BoundingBoxCenter;
}

//--------------------------------------------------------------------------------------
const FLOAT* XSF::Mesh::GetMeshBBoxExtents(UINT uMesh) const
{
    return m_pMeshArray[uMesh].BoundingBoxExtents;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetOutstandingResources() const
{
    UINT outstandingResources = 0;
    if (!m_pMeshHeader)
        return 1;

    outstandingResources += GetOutstandingBufferResources();

    for (UINT i = 0; i < m_pMeshHeader->NumMaterials; i++)
    {
        if (m_pMaterialArray[i].m_strDiffuseTexture[0] != 0)
        {
            if (!Detail::IsErrorResource(m_pMaterialArray[i].m_hCpuDiffuse))
                outstandingResources++;
        }

        if (m_pMaterialArray[i].m_strNormalTexture[0] != 0)
        {
            if (!Detail::IsErrorResource(m_pMaterialArray[i].m_hCpuNormal))
                outstandingResources++;
        }

        if (m_pMaterialArray[i].m_strSpecularTexture[0] != 0)
        {
            if (!Detail::IsErrorResource(m_pMaterialArray[i].m_hCpuSpecular))
                outstandingResources++;
        }
    }

    return outstandingResources;
}

//--------------------------------------------------------------------------------------
UINT XSF::Mesh::GetOutstandingBufferResources() const
{
    UINT outstandingResources = 0;
    if (!m_pMeshHeader)
        return 1;

    for (UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++)
    {
        if (!m_pVertexBufferArray[i].pVB && !Detail::IsErrorResource(m_pVertexBufferArray[i].pVB))
            outstandingResources++;
    }

    for (UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++)
    {
        if (!m_pIndexBufferArray[i].pIB && !Detail::IsErrorResource(m_pIndexBufferArray[i].pIB))
            outstandingResources++;
    }

    return outstandingResources;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::Callbacks::DefaultCreateTextureFromFile(const SampleFramework* const pSample, CpuGpuHeap *pUploadHeap, const WCHAR* szFileName, ID3D12Resource** ppTexture, D3D12_CPU_DESCRIPTOR_HANDLE hTexture, VOID* /*pContext*/)
{
    VERBOSEATGPROFILETHIS;

    if (wcsstr(szFileName, L"dds") == nullptr)
    {
        return E_NOTIMPL;
    }

    DDSLoader12 ddsLoader;
    XSF_RETURN_IF_FAILED(ddsLoader.Initialize(pSample->GetDevice(), pSample->GetCommandQueue(), pSample->GetCommandAllocator(), pUploadHeap));

    ddsLoader.BeginLoading(pSample->GetCommandList());
    HRESULT hr = ddsLoader.LoadDDSFile(szFileName, hTexture, ppTexture);
    ddsLoader.FinishLoading(false);

    return hr;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::Mesh::Callbacks::DefaultCreateBuffer(const SampleFramework* const pSample, CpuGpuHeap *pUploadHeap, ID3D12Resource** ppBuffer, const D3D12_RESOURCE_DESC& BufferDesc, UINT Stride, VOID* pData, VOID* /*pContext*/)
{
    VERBOSEATGPROFILETHIS;

    UNREFERENCED_PARAMETER(Stride);

    XSF::D3DCommandList* const pCmdList = pSample->GetCommandList();

    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    XSF_RETURN_IF_FAILED(pSample->GetDevice()->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &BufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(ppBuffer)));

    ResourceBarrier(pCmdList, *ppBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    XSF_RETURN_IF_FAILED(pUploadHeap->CopyBufferDataToDefaultBuffer((const BYTE*)pData, static_cast<SIZE_T>(BufferDesc.Width), pCmdList, *ppBuffer));
    ResourceBarrier(pCmdList, *ppBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

    return S_OK;
}
