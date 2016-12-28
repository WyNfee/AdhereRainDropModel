//--------------------------------------------------------------------------------------
// VisualizeKinect.cpp
//
// Defines functions used to visualize Kinect data
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include <wrl.h>
#include <robuffer.h>
#include <collection.h>
#include "VisualizeKinect.h"
#include "StockRenderStates.h"

using namespace XboxSampleFramework;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace Windows::Foundation::Collections;
using namespace Platform::Collections;


//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------

#define COLOR_RGBA(r,g,b,a) ((UINT32)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define COLOR_GETRED(c)     ((BYTE)(((c)>>16)&0xff))
#define COLOR_GETGREEN(c)   ((BYTE)(((c)>>8)&0xff))
#define COLOR_GETBLUE(c)    ((BYTE)((c)&0xff))

#define GREEN   COLOR_RGBA(0,255,0,255)
#define RED     COLOR_RGBA(255,0,0,255)
#define BLUE    COLOR_RGBA(0,128,255,255)
#define PURPLE  COLOR_RGBA(128,0,255,255)
#define YELLOW  COLOR_RGBA(255,255,0,255)
#define BROWN   COLOR_RGBA(128,64,0,255)
#define WHITE   COLOR_RGBA(255,255,255,255)
#define GREY    COLOR_RGBA(64,64,64,255)

#define PLAYER_0    GREEN
#define PLAYER_1    BLUE
#define PLAYER_2    YELLOW
#define PLAYER_3    PURPLE
#define PLAYER_4    RED
#define PLAYER_5    BROWN
#define BACKGROUND  GREY


//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

const XMVECTOR g_Green  = XMVectorSet( 0.0f, 1.0f, 0.0f, 1.0f );
const XMVECTOR g_Blue   = XMVectorSet( 0.0f, 0.5f, 1.0f, 1.0f );
const XMVECTOR g_Yellow = XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f );
const XMVECTOR g_Purple = XMVectorSet( 0.5f, 0.0f, 1.0f, 1.0f );
const XMVECTOR g_Red    = XMVectorSet( 1.0f, 0.0f, 0.0f, 1.0f );
const XMVECTOR g_Brown  = XMVectorSet( 0.5f, 0.25f, 0.0f, 1.0f );
const XMVECTOR g_Grey   = XMVectorSet( 0.25f, 0.25f, 0.25, 1.0f );
const XMVECTOR g_White  = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
const XMVECTOR g_Black  = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f );

const wchar_t g_EmptyString[] = L"";
const wchar_t g_DetectionResult[][256] = { L"UNKOWN", L"NO", L"MAYBE", L"YES" };
const wchar_t g_Expression[][256] = { L"NEUTRAL", L"HAPPY" };
const wchar_t g_Activity[][256] = { L"LEFT EYE CLOSED", L"RIGHT EYE CLOSED", L"MOUTH OPEN", L"MOUTH MOVED", L"LOOKING AWAY" };
const wchar_t g_HandStates[][256] = { L"UNKNOWN", L"NOT TRACKED", L"OPEN", L"CLOSED", L"LASSO",  };

bool g_Verbose = false;


//--------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------

// Each skeleton bone has two joints
struct BoneJoints
{
    JointType StartJoint;
    JointType EndJoint;
};

// Define the bones in the skeleton using joint indices
static const BoneJoints g_Bones[] =
{
    // Head
    { JointType::Head, JointType::Neck },                       // Top of head to top of neck
    { JointType::Neck, JointType::SpineShoulder },              // Neck to top of spine

    // Spine
    { JointType::SpineShoulder, JointType::SpineMid },          // Top of spine to mid spine
    { JointType::SpineMid, JointType::SpineBase },              // Mid spine to base spine

    // Right arm
    { JointType::SpineShoulder, JointType::ShoulderRight },     // Neck bottom to right shoulder internal
    { JointType::ShoulderRight, JointType::ElbowRight },        // Right shoulder internal to right elbow
    { JointType::ElbowRight, JointType::WristRight },           // Right elbow to right wrist
    { JointType::WristRight, JointType::HandRight },            // Right wrist to right hand
    { JointType::HandRight, JointType::HandTipRight },          // Right hand to tip of hand
    { JointType::HandRight, JointType::ThumbRight },            // Right hand to tip of thumb

    // Left arm
    { JointType::SpineShoulder, JointType::ShoulderLeft },      // Neck bottom to left shoulder internal
    { JointType::ShoulderLeft, JointType::ElbowLeft },          // Left shoulder internal to left elbow
    { JointType::ElbowLeft, JointType::WristLeft },             // Left elbow to left wrist
    { JointType::WristLeft, JointType::HandLeft },              // Left wrist to left hand
    { JointType::HandLeft, JointType::HandTipLeft },            // Left hand to tip of hand
    { JointType::HandLeft, JointType::ThumbLeft },              // Left hand to tip of thumb

    // Right leg and foot
    { JointType::SpineBase, JointType::HipRight },              // Base spine to right hip
    { JointType::HipRight, JointType::KneeRight },              // Right hip to right knee
    { JointType::KneeRight, JointType::AnkleRight },            // Right knee to right ankle
    { JointType::AnkleRight, JointType::FootRight },            // Right ankle to right foot

    // Left leg and foot
    { JointType::SpineBase, JointType::HipLeft },               // Base spine to left hip
    { JointType::HipLeft, JointType::KneeLeft },                // Left hip to left knee
    { JointType::KneeLeft, JointType::AnkleLeft },              // Left knee to left ankle
    { JointType::AnkleLeft, JointType::FootLeft },              // Left ankle to left foot
};

const UINT g_NumBones = ARRAYSIZE( g_Bones );


//--------------------------------------------------------------------------------------
// Name: VisualizeKinect()
// Desc: Constructor
//--------------------------------------------------------------------------------------

VisualizeKinect::VisualizeKinect()
{
    m_pColorPS              = nullptr;
    m_pIrPS                 = nullptr;
    m_pSamplerState         = nullptr;
    m_pColorTexture         = nullptr;
    m_pColorTextureView     = nullptr;
    m_pIRTexture            = nullptr;
    m_pIRTextureView        = nullptr;
    m_pDepthTexture         = nullptr;
    m_pDepthTextureView     = nullptr;
    m_pBodyIndexTexture     = nullptr;
    m_pBodyIndexTextureView = nullptr;
    m_pKinectSensor         = nullptr;

    ZeroMemory( m_depthColorTable, sizeof( m_depthColorTable ) );
}


//--------------------------------------------------------------------------------------
// Name: ~VisualizeKinect()
// Desc: Destructor
//--------------------------------------------------------------------------------------

VisualizeKinect::~VisualizeKinect()
{
    Shutdown();
}


//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize and allocate memory
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::Initialize( D3DDevice* pd3dDevice, KinectSensor^ pKinectSensor )
{
    XSF_ERROR_IF_FAILED( LoadPixelShader( pd3dDevice, L"Media\\Shaders\\VisualizeKinectColorPS.xps", &m_pColorPS ) );
    XSF_ERROR_IF_FAILED( LoadPixelShader( pd3dDevice, L"Media\\Shaders\\VisualizeKinectIrPS.xps", &m_pIrPS ) );

    XSF_ASSERT( pd3dDevice );
    XSF_ERROR_IF_FAILED( m_draw.Initialize( pd3dDevice ) );

    // Retrieve info from NUI frames for creating textures
    m_pKinectSensor         = pKinectSensor;
    m_pColorFrameDesc       = pKinectSensor->ColorFrameSource->CreateFrameDescription( ColorImageFormat::Yuy2 );
    m_pIRFrameDesc          = pKinectSensor->InfraredFrameSource->FrameDescription;
    m_pDepthFrameDesc       = pKinectSensor->DepthFrameSource->FrameDescription;
    m_pBodyIndexFrameDesc   = pKinectSensor->BodyIndexFrameSource->FrameDescription;

    // Allocate a list to keep the latest body data
    m_bodyCount = pKinectSensor->BodyFrameSource->BodyCount;
    m_bodies = ref new Vector<Body^>( m_bodyCount );

    // Create a sampler state
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
    samplerDesc.AddressU        = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV        = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW        = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc  = D3D11_COMPARISON_NEVER;
    samplerDesc.Filter          = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MaxLOD          = D3D11_FLOAT32_MAX;
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateSamplerState( &samplerDesc, &m_pSamplerState ) );

    // Create texture and texture view for color stream. The color stream has a YUV4:2:2 data which can
    // be presented as either YUY2 or G8R8_G8B8
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory( &textureDesc, sizeof( textureDesc ) );
    textureDesc.Width           = m_pColorFrameDesc->Width;
    textureDesc.Height          = m_pColorFrameDesc->Height;
    textureDesc.MipLevels       = 1;
    textureDesc.ArraySize       = 1;
    textureDesc.Format          = DXGI_FORMAT_G8R8_G8B8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage           = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags       = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags       = 0;
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateTexture2D( &textureDesc, NULL, &m_pColorTexture ) );
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateShaderResourceView( m_pColorTexture, NULL, &m_pColorTextureView ) );

    // Create texture and texture view for IR stream
    ZeroMemory( &textureDesc, sizeof( textureDesc ) );
    textureDesc.Width           = m_pIRFrameDesc->Width;
    textureDesc.Height          = m_pIRFrameDesc->Height;
    textureDesc.MipLevels       = 1;
    textureDesc.ArraySize       = 1;
    textureDesc.Format          = DXGI_FORMAT_R16_UNORM ;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage           = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags       = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags       = 0;
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateTexture2D( &textureDesc, NULL, &m_pIRTexture ) );
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateShaderResourceView( m_pIRTexture, NULL, &m_pIRTextureView ) );

    // Create texture and texture view for depth stream
    ZeroMemory( &textureDesc, sizeof( textureDesc ) );
    textureDesc.Width           = m_pDepthFrameDesc->Width;
    textureDesc.Height          = m_pDepthFrameDesc->Height;
    textureDesc.MipLevels       = 1;
    textureDesc.ArraySize       = 1;
    textureDesc.Format          = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage           = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags       = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags       = 0;
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateTexture2D( &textureDesc, NULL, &m_pDepthTexture ) );
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateShaderResourceView( m_pDepthTexture, NULL, &m_pDepthTextureView ) );

    // Create texture and texture view for body index stream
    ZeroMemory( &textureDesc, sizeof( textureDesc ) );
    textureDesc.Width           = m_pBodyIndexFrameDesc->Width;
    textureDesc.Height          = m_pBodyIndexFrameDesc->Height;
    textureDesc.MipLevels       = 1;
    textureDesc.ArraySize       = 1;
    textureDesc.Format          = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage           = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags       = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags       = 0;
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateTexture2D( &textureDesc, NULL, &m_pBodyIndexTexture ) );
    XSF_ERROR_IF_FAILED( pd3dDevice->CreateShaderResourceView( m_pBodyIndexTexture, NULL, &m_pBodyIndexTextureView ) );

    // Initialze depth table to colorize depth
    const INT32 halfTableSize = _countof( m_depthColorTable ) / 2;
    FLOAT gutter              = 0.2f;
    INT32 tableIndex          = halfTableSize;
    FLOAT step                = ( 1.0f - ( gutter * 2.0f ) ) / halfTableSize;

    for ( FLOAT t = gutter; t < ( 1.0f - gutter ); t += step )
    {
        FLOAT color[3]         = { 0.0f, 0.0f, 0.0f };
        FLOAT band             = 0.7f;
        const FLOAT curveExp   = 2.0f;
        const FLOAT bandGap    = 1.0f - band;

        for ( INT32 i = 0; i < 3; ++i )
        {
            FLOAT s = ( t - bandGap * 0.5f * i ) / band;
            if ( ( s >= 0.0f ) && ( s <= 1.0f ) )
            {
                color[i] = powf( sinf( s * XM_PI * 2.0f - XM_PI * 0.5f ) * 0.5f + 0.5f, curveExp );
            }
        }

        m_depthColorTable[ tableIndex++ ] = COLOR_RGBA( (BYTE)(color[0] * 255.0f), (BYTE)(color[1] * 255.0f), (BYTE)(color[2] * 255.0f), 0xff);
    }

    for ( INT32 i = 0; i < halfTableSize; ++i )
    {
        COLORREF s = m_depthColorTable[ _countof(m_depthColorTable) - 1 - i ];
        FLOAT dim = (FLOAT)i / (FLOAT)halfTableSize;

        m_depthColorTable[i] = COLOR_RGBA( (BYTE)( (FLOAT)COLOR_GETRED  (s) * (0.25f + (dim * 0.75f)) ),
                                           (BYTE)( (FLOAT)COLOR_GETGREEN(s) * (0.25f + (dim * 0.75f)) ),
                                           (BYTE)( (FLOAT)COLOR_GETBLUE (s) * (0.25f + (dim * 0.75f)) ), 0xff);
    }
}


//--------------------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Delete memory
//--------------------------------------------------------------------------------------

void VisualizeKinect::Shutdown()
{
    XSF_SAFE_RELEASE( m_pColorPS );
    XSF_SAFE_RELEASE( m_pIrPS );
    XSF_SAFE_RELEASE( m_pSamplerState );
    XSF_SAFE_RELEASE( m_pColorTexture );
    XSF_SAFE_RELEASE( m_pColorTextureView );
    XSF_SAFE_RELEASE( m_pIRTexture );
    XSF_SAFE_RELEASE( m_pIRTextureView );
    XSF_SAFE_RELEASE( m_pDepthTexture );
    XSF_SAFE_RELEASE( m_pDepthTextureView );
    XSF_SAFE_RELEASE( m_pBodyIndexTexture );
    XSF_SAFE_RELEASE( m_pBodyIndexTextureView );
}


//--------------------------------------------------------------------------------------
// Name: GetPointerToBufferData()
// Desc: Returns the pointer to the IBuffer data.
// NOTE: The pointer is only valid for the lifetime of the governing IBuffer^
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void* VisualizeKinect::GetPointerToBufferData( IBuffer^ buffer ) const
{
#ifdef XSF_USE_PIX_EVENTS
    PIXBeginEvent( 0, L"GetPointerToBufferData" );
#endif

    // Obtain IBufferByteAccess from IBuffer
    ComPtr<IUnknown> pBuffer( (IUnknown*)buffer );
    ComPtr<IBufferByteAccess> pBufferByteAccess;
    pBuffer.As( &pBufferByteAccess );

    // Get pointer to data
    byte* pData = nullptr;
    if ( FAILED( pBufferByteAccess->Buffer( &pData ) ) )
    {
        // Buffer is not annotated with _COM_Outpr, so if it were to fail, then the value of pData is undefined
        pData = nullptr;
    }

#ifdef XSF_USE_PIX_EVENTS
    PIXEndEvent();
#endif
    return pData;
}


//--------------------------------------------------------------------------------------
// Name: GetPointerToBodyIndexFrame()
// Desc: Returns the pointer to a body index frame
// NOTE: The pointer is only valid for the lifetime of the governing BodyIndexFrame^
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void* VisualizeKinect::GetPointerToBodyIndexFrame( BodyIndexFrame^ pBodyIndexFrame ) const
{
#ifdef XSF_USE_PIX_EVENTS
    PIXBeginEvent( 0, L"GetPointerToBodyIndexFrame" );
#endif

    void* pData = nullptr;

    if ( pBodyIndexFrame != nullptr )
    {
        IBuffer^ pBuffer = pBodyIndexFrame->LockImageBuffer();
        pData = GetPointerToBufferData( pBuffer );
    }

#ifdef XSF_USE_PIX_EVENTS
    PIXEndEvent();
#endif

    return pData;
}


//--------------------------------------------------------------------------------------
// Name: GetPointerToColorFrame()
// Desc: Returns the pointer to a color frame
// NOTE: The pointer is only valid for the lifetime of the governing ColorFrame^
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void* VisualizeKinect::GetPointerToColorFrame( ColorFrame^ pColorFrame ) const
{
#ifdef XSF_USE_PIX_EVENTS
    PIXBeginEvent( 0, L"GetPointerToColorFrame" );
#endif

    void* pData = nullptr;

    if ( pColorFrame != nullptr )
    {
        IBuffer^ pBuffer = pColorFrame->LockRawImageBuffer();
        pData = GetPointerToBufferData( pBuffer );
    }

#ifdef XSF_USE_PIX_EVENTS
    PIXEndEvent();
#endif

    return pData;
}


//--------------------------------------------------------------------------------------
// Name: UpdateColor()
// Desc: Update data for color texture
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::UpdateColor( XSF::D3DDeviceContext* pd3dContext, ColorFrame^ pColorFrame )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"UpdateColor" );

    if ( pColorFrame != nullptr )
    {
        const size_t size = m_pColorFrameDesc->Width * m_pColorFrameDesc->Height * m_pColorFrameDesc->BytesPerPixel;
        IBuffer^ buffer = pColorFrame->LockRawImageBuffer();
        void* pColorData = GetPointerToBufferData( buffer );

        D3D11_MAPPED_SUBRESOURCE streamData;
        XSF_ERROR_IF_FAILED( pd3dContext->Map( m_pColorTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamData ) );
        memcpy( streamData.pData, pColorData, size );
        pd3dContext->Unmap( m_pColorTexture, 0 );
    }
    else if ( g_Verbose )
    {
        XSF::DebugPrint( "Failed to update color\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: UpdateDepth()
// Desc: Update data for depth texture
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::UpdateDepth( XSF::D3DDeviceContext* pd3dContext, DepthFrame^ pDepthFrame )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"UpdateDepth" );

    if ( pDepthFrame != nullptr )
    {
        IBuffer^ buffer = pDepthFrame->LockImageBuffer();
        void* pDepthData = GetPointerToBufferData( buffer );

        D3D11_MAPPED_SUBRESOURCE streamData;
        XSF_ERROR_IF_FAILED( pd3dContext->Map( m_pDepthTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamData ) );

        const static FLOAT maxDepth = 3500.0f;
        const static UINT32 normalizeMaxDepth = (UINT32)ceil(maxDepth / 511.0f);
        const UINT width    = m_pDepthFrameDesc->Width;
        const UINT height   = m_pDepthFrameDesc->Height;
        const UINT16* pSrcBuffer = (UINT16*)pDepthData;
        UINT32* pTargetBuffer = reinterpret_cast<UINT32*>(streamData.pData);

        for ( size_t row = 0; row < height; ++row )
        {
            for ( size_t col = 0; col < width; ++col )
            {
                const UINT16* pSrc = ( pSrcBuffer + col + row * width );
                UINT32* pDst = ( pTargetBuffer + col + row * width );
                UINT32 index = XMMin( (UINT32)((USHORT)(*pSrc) / normalizeMaxDepth), (UINT32)511 );

                (*pDst) = m_depthColorTable[ index ];
            }
        }

        pd3dContext->Unmap( m_pDepthTexture, 0 );
    }
    else if ( g_Verbose )
    {
        XSF::DebugPrint( "Failed to update depth\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: GetPointerToColorFrame()
// Desc: Returns the pointer to a color frame
// NOTE: The pointer is only valid for the lifetime of the governing ColorFrame^
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void* VisualizeKinect::GetPointerToIrFrame( InfraredFrame^ pIrFrame ) const
{
#ifdef XSF_USE_PIX_EVENTS
    PIXBeginEvent( 0, L"GetPointerToIrFrame" );
#endif

    void* pData = nullptr;

    if ( pIrFrame != nullptr )
    {
        IBuffer^ buffer = pIrFrame->LockImageBuffer();
        pData = GetPointerToBufferData( buffer );
    }

#ifdef XSF_USE_PIX_EVENTS
    PIXEndEvent();
#endif

    return pData;
}


//--------------------------------------------------------------------------------------
// Name: UpdateIR()
// Desc: Update data for infrared texture
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::UpdateIR( XSF::D3DDeviceContext* pd3dContext, InfraredFrame^ pInfraredFrame )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"UpdateIR" );

    if ( pInfraredFrame != nullptr )
    {
        const size_t size = m_pIRFrameDesc->Width * m_pIRFrameDesc->Height * m_pIRFrameDesc->BytesPerPixel;
        IBuffer^ buffer = pInfraredFrame->LockImageBuffer();
        void* pInfraredData = GetPointerToBufferData( buffer );

        D3D11_MAPPED_SUBRESOURCE streamData;
        XSF_ERROR_IF_FAILED( pd3dContext->Map( m_pIRTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamData ) );
        memcpy( streamData.pData, pInfraredData, size );
        pd3dContext->Unmap( m_pIRTexture, 0 );
    }
    else if ( g_Verbose )
    {
        XSF::DebugPrint( "Failed to update IR\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: UpdateBody()
// Desc: Updates body data, e.g. skeletal tracking, hand states, etc.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::UpdateBody( XSF::D3DDeviceContext* pd3dContext, BodyFrame^ pBodyFrame )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"UpdateBody" );

    if ( pBodyFrame != nullptr )
    {
        pBodyFrame->GetAndRefreshBodyData( m_bodies );
    }
    else if ( g_Verbose )
    {
        XSF::DebugPrint( "Failed to update body\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: UpdateBodyIndex()
// Desc: Update data for body index texture
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::UpdateBodyIndex( XSF::D3DDeviceContext* pd3dContext, BodyIndexFrame^ pBodyIndexFrame )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"UpdateBodyIndex" );

    if ( pBodyIndexFrame != nullptr )
    {
        const UINT width = m_pBodyIndexFrameDesc->Width;
        const UINT height = m_pBodyIndexFrameDesc->Height;
        IBuffer^ buffer = pBodyIndexFrame->LockImageBuffer();
        void* pBodyIndexData = GetPointerToBufferData( buffer );

        D3D11_MAPPED_SUBRESOURCE streamData;
        XSF_ERROR_IF_FAILED( pd3dContext->Map( m_pBodyIndexTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamData ) );

        const BYTE* pSrcBuffer = (BYTE*)pBodyIndexData;
        UINT32* pTargetBuffer = reinterpret_cast<UINT32*>(streamData.pData);

        for ( size_t row = 0; row < height; ++row )
        {
            for ( size_t col = 0; col < width; ++col )
            {
                const BYTE* pSrc = ( pSrcBuffer + col + row * width );
                UINT32* pDst = ( pTargetBuffer + col + row * width );
                UINT player = *pSrc;
                UINT32 color;
                GetPlayerColor( player, color );
                (*pDst) = color;
            }
        }

        pd3dContext->Unmap( m_pBodyIndexTexture, 0 );
    }
    else if ( g_Verbose )
    {
        XSF::DebugPrint( "Failed to update body index\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: RenderColor()
// Desc: Renders color stream
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::RenderColor( XSF::D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewPort )
{
    XSF_ASSERT( pd3dContext );
    XSF_ASSERT( pViewPort );

    XSFScopedNamedEvent( pd3dContext, 0, L"RenderColor" );

    const StockRenderStates& stockStates = StockRenderStates::GetStates();
    stockStates.ApplyDepthStencilState( pd3dContext, StockDepthStencilStates::AlwaysSucceedNoZWriteNoStencil );
    stockStates.ApplyBlendState( pd3dContext, StockBlendStates::Overwrite );

    m_draw.Begin( pd3dContext, pViewPort );
    m_draw.TexturedQuad( m_pColorTextureView, m_pSamplerState, nullptr, nullptr, m_pColorPS );
    m_draw.End();
}


//--------------------------------------------------------------------------------------
// Name: RenderIR()
// Desc: Renders ir stream
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::RenderIR( XSF::D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewPort, ID3D11ShaderResourceView* pIRTextureView )
{
    XSF_ASSERT( pd3dContext );
    XSF_ASSERT( pViewPort );

    XSFScopedNamedEvent( pd3dContext, 0, L"RenderIR" );

    const StockRenderStates& stockStates = StockRenderStates::GetStates();
    stockStates.ApplyDepthStencilState( pd3dContext, StockDepthStencilStates::AlwaysSucceedNoZWriteNoStencil );
    stockStates.ApplyBlendState( pd3dContext, StockBlendStates::Overwrite );

    m_draw.Begin( pd3dContext, pViewPort );
    m_draw.TexturedQuad( pIRTextureView ? pIRTextureView : m_pIRTextureView, m_pSamplerState, nullptr, nullptr, m_pIrPS );
    m_draw.End();
}


//--------------------------------------------------------------------------------------
// Name: RenderDepth()
// Desc: Renders the depth stream
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::RenderDepth( XSF::D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewPort )
{
    XSF_ASSERT( pd3dContext );
    XSF_ASSERT( pViewPort );

    XSFScopedNamedEvent( pd3dContext, 0, L"RenderDepth" );

    const StockRenderStates& stockStates = StockRenderStates::GetStates();
    stockStates.ApplyDepthStencilState( pd3dContext, StockDepthStencilStates::AlwaysSucceedNoZWriteNoStencil );
    stockStates.ApplyBlendState( pd3dContext, StockBlendStates::Overwrite );

    m_draw.Begin( pd3dContext, pViewPort );
    m_draw.TexturedQuad( m_pDepthTextureView, m_pSamplerState );
    m_draw.End();
}


//--------------------------------------------------------------------------------------
// Name: RenderBody()
// Desc: Renders all tracked skeletons
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::RenderBody( XSF::D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewPort, BitmapFont* pFont, const RenderSkeleton renderSkeleton )
{
    XSF_ASSERT( pd3dContext );
    XSFScopedNamedEvent( pd3dContext, 0, L"RenderBody" );

    if ( renderSkeleton != RENDER_SKELETON_NONE )
    {
        const float halfWindowWidth = pViewPort->Width * 0.5f ;
        const float halfWindowHeight = pViewPort->Height * 0.5f;
        const float depthWindowRatioX = ( pViewPort->Width / (FLOAT)m_pDepthFrameDesc->Width );
        const float depthWindowRatioY = ( pViewPort->Height / (FLOAT)m_pDepthFrameDesc->Height );

        const StockRenderStates& stockStates = StockRenderStates::GetStates();
        stockStates.ApplyDepthStencilState( pd3dContext, StockDepthStencilStates::AlwaysSucceedNoZWriteNoStencil );
        stockStates.ApplyBlendState( pd3dContext, StockBlendStates::Overwrite );

        const UINT32 numJoints = Body::JointCount;
        XMFLOAT2* pScreenSpaceJoints = new XMFLOAT2[ numJoints ];
        XSF_ASSERT( pScreenSpaceJoints );

        INT32 startIndex = 0;
        INT32 stopIndex = m_bodyCount - 1;
        if ( renderSkeleton == RENDER_SKELETON_CLOSEST )
        {
            startIndex = FindClosestBody( m_bodies );
            stopIndex = ( startIndex >= 0 ) ? startIndex : ( startIndex - 1 );  // if we couldn't find a skeleton, force the loop to exit early
        }

        // Project each of the tracked skeletons.
        for ( INT32 j = startIndex; j <= stopIndex; ++j )
        {
            auto pBody = m_bodies->GetAt( j );
            if ( pBody == nullptr )
            {
                continue;
            }

            // If the player isn't tracked, don't bother
            if ( !pBody->IsTracked )
            {
                continue;
            }

            // If we only want to render players for whose hands are tracked and the hands are not tracked, skip this player
            if ( renderSkeleton == RENDER_SKELETON_WITH_TRACKED_HANDS &&
                 !PlayerHasValidHandStates( j ) )
            {
                continue;
            }

            // Project the world space joints into screen space
            auto pCoordinateMapper = m_pKinectSensor->CoordinateMapper;

            for ( UINT i = 0; i < numJoints; ++i )
            {
                Joint joint = pBody->Joints->Lookup( static_cast<JointType>( i ) );

                // Check for divide by zero
                if ( fabs( joint.Position.Z ) > FLT_EPSILON  )
                {
                    auto projectedPosition = pCoordinateMapper->MapCameraPointToDepthSpace( joint.Position );

                    pScreenSpaceJoints[ i ].x = projectedPosition.X;
                    pScreenSpaceJoints[ i ].y = projectedPosition.Y;

                    pScreenSpaceJoints[ i ].x *= depthWindowRatioX;
                    pScreenSpaceJoints[ i ].y *= depthWindowRatioY;
                }
                else
                {
                    // A joint that is so close to the camera that its Z value is 0 can simply be drawn directly at the center of the 2D plane.
                    pScreenSpaceJoints[ i ].x = halfWindowWidth;
                    pScreenSpaceJoints[ i ].y = halfWindowHeight;
                }
            }

            // Draw each joint
            for ( UINT i = 0; i < numJoints; i++ )
            {
                XMVECTOR color;
                Joint joint = pBody->Joints->Lookup( static_cast<JointType>( i ) );
                TrackingState jointTrackingState = joint.TrackingState;
                if ( jointTrackingState == TrackingState::Tracked )
                {
                    color = g_Green;
                }
                else if ( jointTrackingState == TrackingState::Inferred )
                {
                    color = g_Red;
                }
                else
                {
                    // A joint in the bone wasn't tracked during skeleton tracking...
                    continue;
                }

                // Draw a box for the joint
                D3D11_VIEWPORT viewPort;
                viewPort.TopLeftX = pViewPort->TopLeftX + pScreenSpaceJoints[ i ].x - 4;
                viewPort.TopLeftY = pViewPort->TopLeftY + pScreenSpaceJoints[ i ].y - 4;
                viewPort.Width = 8;
                viewPort.Height = 8;
                viewPort.MinDepth = 0;
                viewPort.MaxDepth = 1;

                // Check if inside screen space
                if ( viewPort.TopLeftX < 0 ||
                     viewPort.TopLeftX >= 1920 ||
                     viewPort.TopLeftY < 0 ||
                     viewPort.TopLeftY >= 1080 ||
                     ( viewPort.TopLeftX + viewPort.Width ) >= 1920 ||
                     ( viewPort.TopLeftY + viewPort.Height ) >= 1080 )
                {
                    continue;
                }

                m_draw.Begin( pd3dContext, &viewPort );
                m_draw.Quad( color );
                m_draw.End();
            }

            m_draw.Begin( pd3dContext, pViewPort );

            // Draw each bone in the skeleton using the screen space joints
            for ( UINT i = 0; i < g_NumBones; i++ )
            {
                // Assign a color to each joint based on the confidence level. Don't draw a bone if one of its joints has no confidence.
                XMVECTOR startColor;
                Joint joint = pBody->Joints->Lookup( g_Bones[ i ].StartJoint );
                TrackingState jointTrackingState = joint.TrackingState;

                if ( jointTrackingState == TrackingState::Tracked )
                {
                    startColor = g_Green;
                }
                else if ( jointTrackingState == TrackingState::Inferred )
                {
                    startColor = g_Red;
                }
                else
                {
                    // A joint in the bone wasn't tracked during skeleton tracking...
                    continue;
                }

                XMVECTOR endColor;
                joint = pBody->Joints->Lookup( g_Bones[ i ].EndJoint );
                jointTrackingState = joint.TrackingState;

                if ( jointTrackingState == TrackingState::Tracked )
                {
                    endColor = g_Green;
                }
                else if ( jointTrackingState == TrackingState::Inferred )
                {
                    endColor = g_Red;
                }
                else
                {
                    // A joint in the bone wasn't tracked during skeleton tracking...
                    continue;
                }

                // Draw the bone
                INT32 startX    = (INT32)( pScreenSpaceJoints[ static_cast<int>( g_Bones[ i ].StartJoint ) ].x );
                INT32 startY    = (INT32)( pScreenSpaceJoints[ static_cast<int>( g_Bones[ i ].StartJoint ) ].y );
                INT32 endX      = (INT32)( pScreenSpaceJoints[ static_cast<int>( g_Bones[ i ].EndJoint ) ].x );
                INT32 endY      = (INT32)( pScreenSpaceJoints[ static_cast<int>( g_Bones[ i ].EndJoint ) ].y );

                // crop to screen space
                startX = std::max( std::min( startX, 1920 ), 0 );
                startY = std::max( std::min( startY, 1080 ), 0 );
                endX = std::max( std::min( endX, 1920 ), 0 );
                endY = std::max( std::min( endY, 1080 ), 0 );

                m_draw.Line( startX, startY, startColor, endX, endY, endColor );
            }

            m_draw.End();
        }

        delete[] pScreenSpaceJoints;
    }
    else
    {
        m_draw.Begin( pd3dContext, pViewPort );
        m_draw.Quad( g_Black, pViewPort );
        m_draw.End();

        float x = pViewPort->TopLeftX + 20.0f;
        float y = pViewPort->TopLeftY - 40.0f;

        pFont->Begin( pd3dContext );
        pFont->SetScaleFactors( 1.0f, 1.0f );

        wchar_t buffer[ 1024 ];
        for ( INT i = 0; i < m_bodyCount; ++i )
        {
            auto pBodyData = m_bodies->GetAt( i );
            if ( pBodyData == nullptr )
            {
                continue;
            }

            swprintf_s( buffer, 1024, L"Player %d: %s", i, pBodyData->IsTracked ? L"TRACKED" : L"NOT TRACKED" );
            UINT32 color;
            GetPlayerColor( i, color );
            pFont->DrawText( x, y, color, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Engaged: %s", g_DetectionResult[ static_cast<UINT>( pBodyData->Engaged ) ] );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Expression: %s", GetBestDetectedFacialExpression( pBodyData->Expressions ) );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Activity: %s", GetBestDetectedActivity( pBodyData->Activities ) );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Hand Left: %s", g_HandStates[ static_cast<UINT>( pBodyData->HandLeftState ) ] );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Hand Right: %s", g_HandStates[ static_cast<UINT>( pBodyData->HandRightState ) ] );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            swprintf_s( buffer, 1024, L"Lean: (%2.2f, %2.2f)", pBodyData->Lean.X, pBodyData->Lean.Y );
            pFont->DrawText( x, y, 0xffffffff, buffer );
            y += 20;

            y += 20;
        }

        pFont->End();

    }
}

//--------------------------------------------------------------------------------------
// Name: GetBestDetectedActivity()
// Desc: Returns the first activity with the highest confidence otherwise returns
//       empty string
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
const wchar_t* VisualizeKinect::GetBestDetectedActivity( IMapView< Activity, DetectionResult >^ pActivities ) const
{

    bool foundActivity = false;
    auto bestActivity = Activity::EyeLeftClosed;
    auto bestResult = DetectionResult::Unknown;

    for ( auto pActivity : pActivities )
    {
        auto result = pActivity->Value;

        if ( result > bestResult )
        {
            bestResult = result;
            bestActivity = pActivity->Key;
            foundActivity = true;
        }
    }

    if ( foundActivity )
    {
        return g_Activity[ static_cast<int>( bestActivity ) ];
    }

    return g_EmptyString;
}


//--------------------------------------------------------------------------------------
// Name: GetBestDetectedFacialExpression()
// Desc: Returns the first facial expression with the highest confidence otherwise
//       returns empty string
//--------------------------------------------------------------------------------------
const wchar_t* VisualizeKinect::GetBestDetectedFacialExpression( _In_ IMapView< Expression, DetectionResult >^ pExpressions ) const
{
    bool foundExpression = false;
    auto bestExpression = Expression::Neutral;
    auto bestResult = DetectionResult::Unknown;

    for ( auto pExpression : pExpressions )
    {
        auto result = pExpression->Value;

        if ( result > bestResult )
        {
            bestResult = result;
            bestExpression = pExpression->Key;
            foundExpression = true;
        }
    }

    if ( foundExpression )
    {
        return g_Expression[ static_cast<int>( bestExpression ) ];
    }

    return g_EmptyString;
}


//--------------------------------------------------------------------------------------
// Name: RenderBodyIndex()
// Desc: Renders body index stream
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::RenderBodyIndex( XSF::D3DDeviceContext* pd3dContext, const D3D11_VIEWPORT* pViewPort )
{
    XSF_ASSERT( pd3dContext );
    XSF_ASSERT( pViewPort );

    XSFScopedNamedEventFunc( pd3dContext, 0 );

    m_draw.Begin( pd3dContext, pViewPort );
    m_draw.TexturedQuad( m_pBodyIndexTextureView, m_pSamplerState );
    m_draw.End();
}


//--------------------------------------------------------------------------------------
// Name: FindClosestBody()
// Desc: Find the Skeleton Index closest to the camera
//--------------------------------------------------------------------------------------
INT32 VisualizeKinect::FindClosestBody( _In_opt_ IVector<Body^>^ bodies ) const
{
    INT32 userIndex = -1;

    float fMinDistance = FLT_MAX;

    for ( INT iBody = 0; iBody < m_bodyCount; iBody++ )
    {
        Body^ pBody = nullptr;

        if ( bodies == nullptr )
        {
            if ( (LONG)iBody < (LONG)m_bodies->Size )
            {
                pBody = m_bodies->GetAt( iBody );
            }
        }
        else
        {
            if ( (LONG)iBody < (LONG)bodies->Size )
            {
                pBody = bodies->GetAt( iBody );
            }
        }

        if ( pBody == nullptr )
        {
            continue;
        }

        // If not tracked, then ignore
        if ( !pBody->IsTracked )
        {
            continue;
        }

        auto pJoints = pBody->Joints;
        auto headJoint = pJoints->Lookup( JointType::Head ).Position;

        XMVECTOR headPosition = XMVectorSet( headJoint.X, headJoint.Y, headJoint.Z, 1.0f );
        XMVECTOR vDistance = XMVector3LengthSq( headPosition );
        float fDistance = XMVectorGetX( vDistance );

        // sometimes xed file will have tracked skeletons with position being zero, which is not valid
        if ( fDistance < FLT_EPSILON )
        {
             continue;
        }

        if ( fDistance < fMinDistance )
        {
            userIndex = iBody;
            fMinDistance = fDistance;
        }
    }

    return userIndex;
}


//--------------------------------------------------------------------------------------
// Name: GetPlayerColor()
// Desc: Return the color used to render the player index
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::GetPlayerColor( const UINT player, UINT32& color ) const
{
    switch ( player )
    {
    case 0:
        color = PLAYER_0;
        break;

    case 1:
        color = PLAYER_1;
        break;

    case 2:
        color = PLAYER_2;
        break;

    case 3:
        color = PLAYER_3;
        break;

    case 4:
        color = PLAYER_4;
        break;

    case 5:
        color = PLAYER_5;
        break;

    default:
        color = BACKGROUND;
    }
}


//--------------------------------------------------------------------------------------
// Name: GetPlayerColor()
// Desc: Return the color used to render the player index
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void VisualizeKinect::GetPlayerColor( const UINT player, XMVECTOR& color ) const
{
    switch ( player )
    {
    case 0:
        color = g_Green;
        break;

    case 1:
        color = g_Blue;
        break;

    case 2:
        color = g_Yellow;
        break;

    case 3:
        color = g_Purple;
        break;

    case 4:
        color = g_Red;
        break;

    case 5:
        color = g_Brown;
        break;

    default:
        color = g_Grey;
    }
}


//--------------------------------------------------------------------------------------
// Name: PlayerHasValidHandStates()
// Desc: Return the color used to render the player index
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
BOOL VisualizeKinect::PlayerHasValidHandStates( const UINT player ) const
{
    XSF_ASSERT( (INT)player < m_bodyCount );

    auto pBody = m_bodies->GetAt( player );

    if ( pBody->HandLeftState != HandState::NotTracked ||
         pBody->HandRightState != HandState::NotTracked )
    {
        return TRUE;
    }

    return FALSE;
}
