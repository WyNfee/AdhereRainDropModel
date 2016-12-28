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
            Callbacks() :   m_pCreateVertexBuffer( &DefaultCreateBuffer ),
                            m_pCreateIndexBuffer( &DefaultCreateBuffer ),
                            m_pCreateTextureFromFile( &DefaultCreateTextureFromFile )
            {
            }

            HRESULT (*m_pCreateTextureFromFile)( _In_ D3DDevice* pDev, _In_z_ const WCHAR* szFileName, _COM_Outptr_ ID3D11ShaderResourceView** ppSRV, _In_opt_ VOID* pContext );
            HRESULT (*m_pCreateVertexBuffer)( _In_ D3DDevice* pDev, _COM_Outptr_ ID3D11Buffer** ppBuffer, const D3D11_BUFFER_DESC& BufferDesc, _In_opt_ VOID* pData, _In_opt_ VOID* pContext );
            HRESULT (*m_pCreateIndexBuffer)( _In_ D3DDevice* pDev, _COM_Outptr_ ID3D11Buffer** ppBuffer, const D3D11_BUFFER_DESC& BufferDesc, _In_opt_ VOID* pData, _In_opt_ VOID* pContext );
            VOID*   m_pContext;

            static HRESULT  DefaultCreateTextureFromFile( _In_ D3DDevice* pDevice, _In_z_ const WCHAR* szFileName, _COM_Outptr_ ID3D11ShaderResourceView** ppSRV, _In_opt_ VOID* pContext );
            static HRESULT  DefaultCreateTextureFromFile( _In_ D3DDevice* pDevice, _In_ D3DDeviceContext* pDeviceContext, _In_z_ const WCHAR* szFileName, _Deref_out_ ID3D11ShaderResourceView** ppSRV, _In_opt_ VOID* pContext );
            static HRESULT  DefaultCreateBuffer( _In_ D3DDevice* pDev, _COM_Outptr_ ID3D11Buffer** ppBuffer, const D3D11_BUFFER_DESC& bufferDesc, _In_opt_ VOID* pData, _In_opt_ VOID* pContext );
        };

        // TODO: modify for the coding standard
        UINT m_iLoadingSlot;

        struct MeshArray
        {
            CHAR    Name[ MAX_MESH_NAME ];
            BYTE    NumVertexBuffers;
            UINT    VertexBuffers[ MAX_VERTEX_STREAMS ];
            UINT    IndexBuffer;
            UINT    NumSubsets;
            UINT    NumFrameInfluences; //aka bones

            FLOAT   BoundingBoxCenter[ 3 ];
            FLOAT   BoundingBoxExtents[ 3 ];

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
        
#if defined(_XBOX_ONE) && defined(_TITLE)
        struct MeshArrayVBSetBatch
        {
            void*   pVBSetBatch;
            UINT    vbSetBatchSize;
        };
#endif

        struct Subset
        {
            CHAR Name[ MAX_SUBSET_NAME ];
            UINT MaterialID;
            UINT PrimitiveType;
            UINT64 IndexStart;
            UINT64 IndexCount;
            UINT64 VertexStart;
            UINT64 VertexCount;
        };

        struct Frame
        {
            CHAR Name[ MAX_FRAME_NAME ];
            UINT Mesh;
            UINT ParentFrame;
            UINT ChildFrame;
            UINT SiblingFrame;
            XMFLOAT4X4 Matrix;
            UINT AnimationDataIndex;        //Used to index which set of keyframes transforms this frame
        };

        struct Material
        {
            CHAR    Name[ MAX_MATERIAL_NAME ];

            // Use MaterialInstancePath
            CHAR    MaterialInstancePath[ MAX_MATERIAL_PATH ];

            // Or fall back to d3d8-type materials
            CHAR    m_strDiffuseTexture[ MAX_TEXTURE_NAME ];
            CHAR    m_strNormalTexture[ MAX_TEXTURE_NAME ];
            CHAR    m_strSpecularTexture[ MAX_TEXTURE_NAME ];

            FLOAT   Diffuse[ 4 ];
            FLOAT   Ambient[ 4 ];
            FLOAT   Specular[ 4 ];
            FLOAT   Emissive[ 4 ];
            FLOAT   Power;

            union
            {
                UINT64 Force64_1;            //Force the union to 64bits
                ID3D11Texture2D*            m_pDiffuseTexture;
            };
            union
            {
                UINT64 Force64_2;            //Force the union to 64bits
                ID3D11Texture2D*            m_pNormalTexture;
            };
            union
            {
                UINT64 Force64_3;            //Force the union to 64bits
                ID3D11Texture2D*            m_pSpecularTexture;
            };

            union
            {
                UINT64 Force64_4;            //Force the union to 64bits
                ID3D11ShaderResourceView*    m_pDiffuseSRV;
            };
            union
            {
                UINT64 Force64_5;            //Force the union to 64bits
                ID3D11ShaderResourceView*    m_pNormalSRV;
            };
            union
            {
                UINT64 Force64_6;            //Force the union to 64bits
                ID3D11ShaderResourceView*    m_pSpecularSRV;
            };
        };

        Mesh();
        ~Mesh();

        _Check_return_
        HRESULT CreateFromFile( _In_ D3DDevice* pDev, _In_z_ const WCHAR* szFileName, _In_opt_ const Callbacks* pLoaderCallbacks = nullptr, _In_opt_ BOOL fastSemanticsEnabled = FALSE );
        _Check_return_
        HRESULT CreateFromMemory( _In_ D3DDevice* pDev, _In_ VOID* pData, UINT DataBytes, BOOL bCopyData, _In_opt_ const Callbacks* pLoaderCallbacks = nullptr, _In_opt_ BOOL fastSemanticsEnabled = FALSE );
        VOID    Destroy();

        UINT    GetVBIndex( UINT uMesh, UINT uVB ) const;
        UINT    GetIBIndex( UINT uMesh ) const;

        UINT    GetNumMeshes() const;
        UINT    GetNumMaterials() const;
        UINT    GetNumVBs() const;
        UINT    GetNumIBs() const;
        DXGI_FORMAT GetIBFormat( UINT uIB ) const;
        UINT        GetVBFormat( UINT uVB, _Out_cap_post_count_( D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, return ) D3D11_INPUT_ELEMENT_DESC* pDesc ) const;

        ID3D11Buffer* GetVBAt( UINT uVB ) const;
        ID3D11Buffer* GetIBAt( UINT uIB ) const;

        const BYTE*         GetRawVerticesAt( UINT iVB ) const;
        const BYTE*         GetRawIndicesAt( UINT iIB ) const;
        const Material*     GetMaterial( UINT iMaterial ) const;
        const MeshArray*    GetMesh( UINT uMesh ) const;
        UINT                GetNumSubsets( UINT uMesh ) const;
        const Subset*       GetSubset( UINT uMesh, UINT uSubset ) const;
        UINT                GetVertexStride( UINT uMesh, UINT uVB ) const;
        UINT                GetNumFrames() const;
        Frame*              GetFrame( UINT uFrame ) const;
        Frame*              FindFrame( _In_z_ const CHAR* const pszName ) const;
        HRESULT             SetFrameMatrix( const CHAR* pszName, const XMFLOAT4X4& Matrix );
        UINT                GetNumVertices( UINT uMesh, UINT uVB ) const;
        UINT                GetNumIndices( UINT uMesh ) const;
        const FLOAT*        GetMeshBBoxCenter( UINT uMesh ) const;
        const FLOAT*        GetMeshBBoxExtents( UINT uMesh ) const;
        UINT                GetOutstandingResources() const;
        UINT                GetOutstandingBufferResources() const;

        // rendering
        typedef BOOL (* PreRenderFrameCallbackPtr)( _In_ D3DDeviceContext* pDev, _In_ const Frame* pFrame, _In_opt_ void* pCallbackContext );

        struct RenderingOptions
        {
            RenderingOptions( UINT diffuse = ~0ul, UINT normal = ~0ul, UINT specular = ~0ul )
            {
                uDiffuseSlot = diffuse;
                uNormalSlot = normal;
                uSpecularSlot = specular;
                pCallback = NULL;
                pCallbackContext = NULL;
                enableTess = FALSE;
                numInstances = 1;
#if defined(_XBOX_ONE) && defined(_TITLE)
                fastSemanticsEnabled = FALSE;
#endif
            }

            UINT uDiffuseSlot;
            UINT uNormalSlot;
            UINT uSpecularSlot;
            BOOL enableTess;
            UINT numInstances;
            PreRenderFrameCallbackPtr pCallback;
            VOID* pCallbackContext;
#if defined(_XBOX_ONE) && defined(_TITLE)
            BOOL fastSemanticsEnabled;
#endif
        };

        VOID    Render( _In_ D3DDeviceContext* pDev, _In_ const RenderingOptions& options ) const;
        VOID    RenderFrame( _In_ D3DDeviceContext* pCtx, UINT uFrame, _In_ const RenderingOptions& options ) const;
        VOID    RenderMesh( _In_ D3DDeviceContext* pCtx, UINT uMesh, _In_ const RenderingOptions& options ) const;

        HRESULT LoadAnimation( _In_z_ const WCHAR* const szFileName );

        // Frame manipulation
        VOID XM_CALLCONV TransformBindPose( CXMMATRIX World );
        VOID XM_CALLCONV TransformMesh( CXMMATRIX World, double fTime );

        // Animation
        UINT GetNumInfluences( UINT iMesh ) const;
        CXMMATRIX GetMeshInfluenceMatrix( UINT uMesh, UINT uInfluence ) const;
        UINT GetAnimationKeyFromTime( double fTime ) const;
        CXMMATRIX GetWorldMatrix( UINT uFrameIndex ) const;
        CXMMATRIX GetInfluenceMatrix( UINT uFrameIndex ) const;
        BOOL GetAnimationProperties( UINT* pNumKeys, FLOAT* pFrameTime ) const;

    protected:
        UINT    m_NumOutstandingResources;
        std::vector< BYTE* > m_MappedPointers;

        struct Header;
        struct VBHeader;
        struct IBHeader;
        struct AnimFileHeader;
        struct AnimFrameData;

        // These are the pointers to the two chunks of data loaded in from the mesh file
        BYTE*   m_pMeshData;
        std::vector< BYTE > m_staticMeshData;
        BYTE* m_pAnimationData;
        BYTE** m_ppVertices;
        BYTE** m_ppIndices;

        // General mesh info
        Header*     m_pMeshHeader;
        VBHeader*   m_pVertexBufferArray;
        IBHeader*   m_pIndexBufferArray;
        MeshArray*  m_pMeshArray;
        Subset*     m_pSubsetArray;
        Frame*      m_pFrameArray;
        Material*   m_pMaterialArray;

#if defined(_XBOX_ONE) && defined(_TITLE)
        MeshArrayVBSetBatch* m_pMeshArrayVBSetBatch;
#endif
        // Animation (TODO: Add ability to load/track multiple animation sets)
        AnimFileHeader* m_pAnimationHeader;
        AnimFrameData*  m_pAnimationFrameData;
        XMMATRIX*   m_pBindPoseFrameMatrices;
        XMMATRIX*   m_pTransformedFrameMatrices;
        XMMATRIX*   m_pWorldPoseFrameMatrices;

        HRESULT     LoadMaterials( _In_ D3DDevice* pDev, _In_ Material* pMaterials, UINT NumMaterials, _In_opt_ const Callbacks* pLoaderCallbacks=nullptr );
        HRESULT     CreateVertexBuffer( _In_ D3DDevice* pDev, _In_ VBHeader* pHeader, _In_ VOID* pVertices, _In_opt_ const Callbacks* pLoaderCallbacks=nullptr );
#if defined(_XBOX_ONE) && defined(_TITLE)
        HRESULT     CreateSetFastVBResourceBatches( _In_ D3DDevice* pDev, UINT uMesh );
#endif
        HRESULT     CreateIndexBuffer( _In_ D3DDevice* pDev, _In_ IBHeader* pHeader, _In_ VOID* pIndices, _In_opt_ const Callbacks* pLoaderCallbacks=nullptr );

        Subset*     GetSubset( UINT uMesh, UINT uSubset );

        // Frame manipulation
        VOID XM_CALLCONV TransformBindPoseFrame( UINT iFrame, CXMMATRIX ParentWorld );
        VOID XM_CALLCONV TransformFrame( UINT iFrame, CXMMATRIX ParentWorld, double fTime );
        VOID TransformFrameAbsolute( UINT iFrame, double fTime );
    };
}       // XboxSampleFramework


namespace XSF = XboxSampleFramework;

