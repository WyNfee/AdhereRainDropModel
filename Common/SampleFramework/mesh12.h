#pragma once

//--------------------------------------------------------------------------------------
// File: SDKMesh.h
//
// Disclaimer:  
//   The SDK Mesh format (.sdkmesh) is not a recommended file format for shipping titles.  
//   It was designed to meet the specific needs of the SDK samples.  Any real-world 
//   applications should avoid this file format in favor of a destination format that 
//   meets the specific needs of the application.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SampleFramework.h"

namespace XboxSampleFramework
{
    //--------------------------------------------------------------------------------------
    // Mesh class.  This class reads the .sdkmesh file format for use by the samples
    //--------------------------------------------------------------------------------------
    class Mesh
    {
        static const UINT   MAX_FRAME_NAME = 100;
        static const UINT   MAX_MESH_NAME = 100;
        static const UINT   MAX_SUBSET_NAME = 100;
        static const UINT   MAX_MATERIAL_NAME = 100;
        static const UINT   MAX_VERTEX_STREAMS = 16;
        static const UINT   MAX_TEXTURE_NAME = MAX_PATH;
        static const UINT   MAX_MATERIAL_PATH = MAX_PATH;

    public:
        // NOTE: can change to a class if all 3 are to be replaced at the same time. structure of pointers to functions allows to override them individually
        struct Callbacks
        {
            Callbacks() :   m_pCreateVertexBuffer(&DefaultCreateBuffer),
                            m_pCreateIndexBuffer(&DefaultCreateBuffer),
                            m_pCreateTextureFromFile(&DefaultCreateTextureFromFile)
            {
            }

            HRESULT (*m_pCreateTextureFromFile)(_In_ const SampleFramework* const pSample, _In_ CpuGpuHeap *pUploadHeap, _In_z_ const WCHAR* szFileName, _COM_Outptr_ ID3D12Resource** ppTexture, D3D12_CPU_DESCRIPTOR_HANDLE hTexture, _In_opt_ VOID* pContext);
            HRESULT (*m_pCreateVertexBuffer)(_In_ const SampleFramework* const pSample, _In_ CpuGpuHeap *pUploadHeap, _COM_Outptr_ ID3D12Resource** ppBuffer, const D3D12_RESOURCE_DESC& BufferDesc, UINT Stride, _In_opt_ VOID* pData, _In_opt_ VOID* pContext);
            HRESULT (*m_pCreateIndexBuffer)(_In_ const SampleFramework* const pSample, _In_ CpuGpuHeap *pUploadHeap, _COM_Outptr_ ID3D12Resource** ppBuffer, const D3D12_RESOURCE_DESC& BufferDesc, UINT Stride, _In_opt_ VOID* pData, _In_opt_ VOID* pContext);
            VOID*   m_pContext;

            static HRESULT  DefaultCreateTextureFromFile(_In_ const SampleFramework* const pSample, _In_ CpuGpuHeap *pUploadHeap, _In_z_ const WCHAR* szFileName, _COM_Outptr_ ID3D12Resource** ppTexture, D3D12_CPU_DESCRIPTOR_HANDLE hTexture, _In_opt_ VOID* pContext);
            static HRESULT  DefaultCreateBuffer(_In_ const SampleFramework* const pSample, _In_ CpuGpuHeap *pUploadHeap, _COM_Outptr_ ID3D12Resource** ppBuffer, const D3D12_RESOURCE_DESC& bufferDesc, UINT Stride, _In_opt_ VOID* pData, _In_opt_ VOID* pContext);
        };

        static const UINT c_numSlots = 3;
        _In_range_(0,c_numSlots - 1) UINT m_loadingSlot;

        struct MeshArray
        {
            CHAR    Name[MAX_MESH_NAME];
            BYTE    NumVertexBuffers;
            UINT    VertexBuffers[MAX_VERTEX_STREAMS];
            UINT    IndexBuffer;
            UINT    NumSubsets;
            UINT    NumFrameInfluences; //aka bones

            FLOAT   BoundingBoxCenter[3];
            FLOAT   BoundingBoxExtents[3];

            union
            {
                UINT64 SubsetOffset;    //Offset to list of subsets (This also forces the union to 64bits)
                UINT* pSubsets;        //Pointer to list of subsets
            };
            union
            {
                UINT64 FrameInfluenceOffset;  //Offset to list of frame influences (This also forces the union to 64bits)
                UINT* pFrameInfluences;      //Pointer to list of frame influences
            };
        };
        
        struct Subset
        {
            CHAR Name[MAX_SUBSET_NAME];
            UINT MaterialID;
            UINT PrimitiveType;
            UINT64 IndexStart;
            UINT64 IndexCount;
            UINT64 VertexStart;
            UINT64 VertexCount;
        };

        struct Frame
        {
            CHAR Name[MAX_FRAME_NAME];
            UINT Mesh;
            UINT ParentFrame;
            UINT ChildFrame;
            UINT SiblingFrame;
            XMFLOAT4X4 Matrix;
            UINT AnimationDataIndex;        //Used to index which set of keyframes transforms this frame
        };

        struct Material
        {
            CHAR    Name[MAX_MATERIAL_NAME];

            // Use MaterialInstancePath
            CHAR    MaterialInstancePath[MAX_MATERIAL_PATH];

            // Or fall back to d3d8-type materials
            CHAR    m_strDiffuseTexture[MAX_TEXTURE_NAME];
            CHAR    m_strNormalTexture[MAX_TEXTURE_NAME];
            CHAR    m_strSpecularTexture[MAX_TEXTURE_NAME];

            FLOAT   Diffuse[4];
            FLOAT   Ambient[4];
            FLOAT   Specular[4];
            FLOAT   Emissive[4];
            FLOAT   Power;

            union
            {
                UINT64 Force64_1;            //Force the union to 64bits
                ID3D12Resource*              m_pDiffuseTexture;
            };
            union
            {
                UINT64 Force64_2;            //Force the union to 64bits
                ID3D12Resource*              m_pNormalTexture;
            };
            union
            {
                UINT64 Force64_3;            //Force the union to 64bits
                ID3D12Resource*              m_pSpecularTexture;
            };

            union
            {
                UINT64 Force64_4;            //Force the union to 64bits
                D3D12_CPU_DESCRIPTOR_HANDLE  m_hCpuDiffuse;
            };
            union
            {
                UINT64 Force64_5;            //Force the union to 64bits
                D3D12_CPU_DESCRIPTOR_HANDLE  m_hCpuNormal;
            };
            union
            {
                UINT64 Force64_6;            //Force the union to 64bits
                D3D12_CPU_DESCRIPTOR_HANDLE  m_hCpuSpecular;
            };
        };

        Mesh();
        ~Mesh();

        void SetUploadHeapSize(UINT32 uploadHeapSizeMB) { m_uploadHeapSizeMB = uploadHeapSizeMB; }

        _Check_return_
        HRESULT CreateFromFile(_In_ const SampleFramework* const pSample, _In_z_ const WCHAR* szFileName,
                               _In_opt_ const Callbacks* pLoaderCallbacks = nullptr, _In_opt_ ID3D12DescriptorHeap *pSrvHeap = nullptr, _Inout_opt_ UINT *pSrvHeapIndex = nullptr);
        _Check_return_
        HRESULT CreateFromMemory(_In_ const SampleFramework* const pSample, _In_ VOID* pData, UINT DataBytes, BOOL bCopyData,
                               _In_opt_ const Callbacks* pLoaderCallbacks = nullptr, _In_opt_ ID3D12DescriptorHeap *pSrvHeap = nullptr, _Inout_opt_ UINT *pSrvHeapIndex = nullptr);
        VOID    Destroy();

        UINT    GetVBIndex(UINT uMesh, UINT uVB) const;
        UINT    GetIBIndex(UINT uMesh) const;

        UINT    GetNumMeshes() const;
        UINT    GetNumMaterials() const;
        UINT    GetNumVBs() const;
        UINT    GetNumIBs() const;
        DXGI_FORMAT GetIBFormat(UINT uIB) const;
        UINT        GetVBFormat(UINT uVB, _Out_cap_post_count_(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, return) D3D12_INPUT_ELEMENT_DESC* pDesc) const;

        ID3D12Resource* GetVBAt(UINT uVB) const;
        const D3D12_VERTEX_BUFFER_VIEW* GetVBViewAt(UINT uVB) const;
        ID3D12Resource* GetIBAt(UINT uIB) const;
        const D3D12_INDEX_BUFFER_VIEW* GetIBViewAt(UINT uIB) const;

        const BYTE*         GetRawVerticesAt(UINT iVB) const;
        const BYTE*         GetRawIndicesAt(UINT iIB) const;
        const Material*     GetMaterial(UINT iMaterial) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT iMaterial) const;
        const MeshArray*    GetMesh(UINT uMesh) const;
        UINT                GetNumSubsets(UINT uMesh) const;
        const Subset*       GetSubset(UINT uMesh, UINT uSubset) const;
        UINT                GetVertexStride(UINT uMesh, UINT uVB) const;
        UINT                GetNumFrames() const;
        Frame*              GetFrame(UINT uFrame) const;
        Frame*              FindFrame(_In_z_ const CHAR* const pszName) const;
        HRESULT             SetFrameMatrix(const CHAR* pszName, const XMFLOAT4X4& Matrix);
        UINT                GetNumVertices(UINT uMesh, UINT uVB) const;
        UINT                GetNumIndices(UINT uMesh) const;
        const FLOAT*        GetMeshBBoxCenter(UINT uMesh) const;
        const FLOAT*        GetMeshBBoxExtents(UINT uMesh) const;
        UINT                GetOutstandingResources() const;
        UINT                GetOutstandingBufferResources() const;

        // rendering
        typedef BOOL (* PreRenderFrameCallbackPtr)(_In_ D3DCommandList* pCmdList, _In_ const Frame* pFrame, _In_opt_ void* pCallbackContext);
        typedef void (* SetShaderResourcesCallbackPtr)(_In_ D3DDevice* const pDevice, _In_ D3DCommandList* const pCmdList, UINT rootParameterIndex, _In_ ID3D12DescriptorHeap* const pSrvHeap, UINT heapIndex);

        struct RenderingOptions
        {
            RenderingOptions(_In_opt_ UINT srvSlot = ~0ul)
            {
                uSrvSlot = srvSlot;
                pCallback = nullptr;
                pCallbackContext = nullptr;
                pSetShaderResourcesCallback = nullptr;
                enableTess = FALSE;
                numInstances = 1;
            }

            UINT uSrvSlot;
            BOOL enableTess;
            UINT numInstances;
            PreRenderFrameCallbackPtr pCallback;
            SetShaderResourcesCallbackPtr pSetShaderResourcesCallback;
            VOID* pCallbackContext;
        };

        VOID Render(_In_ D3DCommandList* pCmdList, _In_ const RenderingOptions& options) const;
        VOID RenderFrame(_In_ D3DCommandList* pCmdList, UINT uFrame, _In_ const RenderingOptions& options) const;
        VOID RenderMesh(_In_ D3DCommandList* pCmdList, UINT uMesh, _In_ const RenderingOptions& options) const;

        HRESULT LoadAnimation(_In_z_ const WCHAR* const szFileName);

        // Frame manipulation
        VOID XM_CALLCONV TransformBindPose(CXMMATRIX World);
        VOID XM_CALLCONV TransformMesh(CXMMATRIX World, double fTime);

        // Animation
        UINT GetNumInfluences(UINT iMesh) const;
        CXMMATRIX GetMeshInfluenceMatrix(UINT uMesh, UINT uInfluence) const;
        UINT GetAnimationKeyFromTime(double fTime) const;
        CXMMATRIX GetWorldMatrix(UINT uFrameIndex) const;
        CXMMATRIX GetInfluenceMatrix(UINT uFrameIndex) const;
        BOOL GetAnimationProperties(UINT* pNumKeys, FLOAT* pFrameTime) const;

    protected:
        const SampleFramework* m_pSample;
        UINT m_uploadHeapSizeMB;
        XSF::CpuGpuHeap m_uploadHeap;

        UINT    m_NumOutstandingResources;
        std::vector<BYTE*> m_MappedPointers;

        struct Header;
        struct VBHeader;
        struct IBHeader;
        struct AnimFileHeader;
        struct AnimFrameData;

        // These are the pointers to the two chunks of data loaded in from the mesh file
        BYTE*   m_pMeshData;
        std::vector<BYTE> m_staticMeshData;
        BYTE* m_pAnimationData;
        BYTE** m_ppVertices;
        BYTE** m_ppIndices;

        D3D12_VERTEX_BUFFER_VIEW *m_pVBView;
        D3D12_INDEX_BUFFER_VIEW *m_pIBView;

        // General mesh info
        Header*     m_pMeshHeader;
        VBHeader*   m_pVertexBufferArray;
        IBHeader*   m_pIndexBufferArray;
        MeshArray*  m_pMeshArray;
        Subset*     m_pSubsetArray;
        Frame*      m_pFrameArray;
        Material*   m_pMaterialArray;
        DescriptorHeapWrapper *m_pSRVHeap;
        UINT*       m_pSRVHeapIndex;

        // Animation (TODO: Add ability to load/track multiple animation sets)
        AnimFileHeader* m_pAnimationHeader;
        AnimFrameData*  m_pAnimationFrameData;
        XMMATRIX*   m_pBindPoseFrameMatrices;
        XMMATRIX*   m_pTransformedFrameMatrices;
        XMMATRIX*   m_pWorldPoseFrameMatrices;

        HRESULT LoadMaterials(_Out_writes_(NumMaterials) Material* pMaterials, UINT NumMaterials, _In_opt_ const Callbacks* pLoaderCallbacks=nullptr, _Inout_opt_ UINT *pSrvHeapIndex = nullptr);
        HRESULT CreateVertexBuffer(_In_ VBHeader* pHeader, _In_ VOID* pVertices, _In_opt_ const Callbacks* pLoaderCallbacks = nullptr);
        HRESULT CreateIndexBuffer(_In_ IBHeader* pHeader, _In_ VOID* pIndices, _In_opt_ const Callbacks* pLoaderCallbacks = nullptr);

        Subset* GetSubset(UINT uMesh, UINT uSubset);

        // Frame manipulation
        VOID XM_CALLCONV TransformBindPoseFrame(UINT iFrame, CXMMATRIX ParentWorld);
        VOID XM_CALLCONV TransformFrame(UINT iFrame, CXMMATRIX ParentWorld, double fTime);
        VOID TransformFrameAbsolute(UINT iFrame, double fTime);
    };
}       // XboxSampleFramework


namespace XSF = XboxSampleFramework;
