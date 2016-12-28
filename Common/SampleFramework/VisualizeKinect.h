//--------------------------------------------------------------------------------------
// VisualizeKinect.h
//
// Declares functions used to visualize Kinect data
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_VISUALIZEKINECT_H_INCLUDED
#define XSF_VISUALIZEKINECT_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

#include "BitmapFont.h"
#include "Draw.h"

using namespace Windows::Kinect;
using namespace Windows::Foundation::Collections;

namespace XboxSampleFramework
{
    class VisualizeKinect
    {
    public:
        enum RenderSkeleton
        {
            RENDER_SKELETON_NONE,               // Render no skeleton
            RENDER_SKELETON_ALL,                // Render all tracked skeletons
            RENDER_SKELETON_CLOSEST,            // Render only the closest skeleton to the sensor
            RENDER_SKELETON_WITH_TRACKED_HANDS  // Render only the skeletons for which hand states are valid
        };

        VisualizeKinect();
        ~VisualizeKinect();

        void Initialize( _In_ D3DDevice* pd3dDevice, _In_ KinectSensor^ pKinectSensor );
        void Shutdown();

        void UpdateColor( XSF::D3DDeviceContext* pd3dContext, _In_ ColorFrame^ pColorFrame );
        void UpdateDepth( XSF::D3DDeviceContext* pd3dContext, _In_ DepthFrame^ pDepthFrame );
        void UpdateIR(  XSF::D3DDeviceContext* pd3dContext, _In_ InfraredFrame^ pInfraredFrame );
        void UpdateBody( XSF::D3DDeviceContext* pd3dContext, _In_ BodyFrame^ pBodyFrame );
        void UpdateBodyIndex( XSF::D3DDeviceContext* pd3dContext, _In_ BodyIndexFrame^ pBodyIndexFrame );

        void RenderColor( _In_ D3DDeviceContext* pd3dContext, _In_ const D3D11_VIEWPORT* pViewPort );
        void RenderDepth( _In_ D3DDeviceContext* pd3dContext, _In_ const D3D11_VIEWPORT* pViewPort );
        void RenderIR( _In_ D3DDeviceContext* pd3dContext, _In_ const D3D11_VIEWPORT* pViewPort, _In_opt_ ID3D11ShaderResourceView* pIRTextureView = nullptr );
        void RenderBody( _In_ D3DDeviceContext* pd3dContext, _In_ const D3D11_VIEWPORT* pViewPort, _In_opt_ BitmapFont* pFont, _In_ const RenderSkeleton renderSkeleton = RENDER_SKELETON_ALL );
        void RenderBodyIndex( _In_ D3DDeviceContext* pd3dContext, _In_ const D3D11_VIEWPORT* pViewPort );

        void GetPlayerColor( _In_ const UINT player, UINT32& color ) const;
        void GetPlayerColor( _In_ const UINT player, XMVECTOR& color ) const;

        INT32 FindClosestBody( _In_opt_ Windows::Foundation::Collections::IVector<Body^>^ bodies = nullptr ) const;
        Body^ GetBodyAt( _In_ const UINT index ) const { return m_bodies->GetAt( index ); }
        BOOL PlayerHasValidHandStates( _In_ const UINT player ) const;

        ID3D11Texture2D* GetColorTexture() const { return m_pColorTexture; }
        ID3D11Texture2D* GetIRTexture() const { return m_pIRTexture; }
        ID3D11Texture2D* GetDepthTexture() const { return m_pDepthTexture; }
        ID3D11Texture2D* GetBodyIndexTexture() const { return m_pBodyIndexTexture; }

        const wchar_t* GetBestDetectedActivity( _In_ IMapView< Activity, DetectionResult >^ pActivities ) const;
        const wchar_t* GetBestDetectedFacialExpression( _In_ IMapView< Expression, DetectionResult >^ pExpressions ) const;

        void* GetPointerToBufferData( _In_ Windows::Storage::Streams::IBuffer^ buffer ) const;
        void* GetPointerToBodyIndexFrame( _In_ BodyIndexFrame^ pBodyIndexFrame ) const;
        void* GetPointerToColorFrame( _In_ ColorFrame^ pColorFrame ) const;
        void* GetPointerToIrFrame( _In_ InfraredFrame^ pIrFrame ) const;

    private:
        KinectSensor^               m_pKinectSensor;
        Draw                        m_draw;

        UINT32                      m_depthColorTable[ 512 ];

        // Shaders
        ID3D11PixelShader*          m_pColorPS;
        ID3D11PixelShader*          m_pIrPS;
        ID3D11SamplerState*         m_pSamplerState;

        // Textures
        ID3D11Texture2D*            m_pColorTexture;
        ID3D11ShaderResourceView*   m_pColorTextureView;
        ID3D11Texture2D*            m_pIRTexture;
        ID3D11ShaderResourceView*   m_pIRTextureView;
        ID3D11Texture2D*            m_pDepthTexture;
        ID3D11ShaderResourceView*   m_pDepthTextureView;
        ID3D11Texture2D*            m_pBodyIndexTexture;
        ID3D11ShaderResourceView*   m_pBodyIndexTextureView;

        // Frame descriptor for each stream
        FrameDescription^           m_pColorFrameDesc;
        FrameDescription^           m_pIRFrameDesc;
        FrameDescription^           m_pDepthFrameDesc;
        FrameDescription^           m_pBodyIndexFrameDesc;

        // Store a local copy of the body data
        int                         m_bodyCount;
        IVector<Body^>^             m_bodies;
    };

} // namespace XboxSampleFramework

#endif // XSF_DRAW_H_INCLUDED
