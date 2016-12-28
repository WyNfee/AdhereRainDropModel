//--------------------------------------------------------------------------------------
// VisualizeKinect.hlsl
//
// Shaders visualizing kinect streams
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------

// Same struct defined as in draw.hlsl
struct FullScreenQuadGS_OUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;    
};

Texture2D       t0 : register( t0 );
SamplerState    s0 : register( s0 );


//--------------------------------------------------------------------------------------
// Name: VisualizeKinectColorPS
// Desc: Color from color stream is YUV 4:2:2, or YUY2 format, which we need to sample as RGB
//--------------------------------------------------------------------------------------
static const float4 g_YuvOffset = { 0.501961, 0, 0.501961, 0 };
float4 VisualizeKinectColorPS( FullScreenQuadGS_OUT input ) : SV_Target
{
    float4 output;

    float4 yuv = t0.Sample( s0, input.texCoord );
    float4 offset = yuv - g_YuvOffset;

    output.r = clamp( offset.g + 1.568648 * offset.b, 0.0, 1.0 );
    output.g = clamp( offset.g - 0.186593 * offset.r - 0.466296 * offset.b, 0.0, 1.0 );
    output.b = clamp( offset.g + 1.848352 * offset.r, 0.0, 1.0 );
    output.a = 1.0;

    return output;
}


//--------------------------------------------------------------------------------------
// Name: VisualizeKinectIrPS
// Desc: IR from color stream is 16bit gray scale value
//--------------------------------------------------------------------------------------
static const float g_Power = 8;
float4 VisualizeKinectIrPS( FullScreenQuadGS_OUT input ) : SV_Target
{
    float4 output;

    float intensity  = 1.0 - t0.Sample( s0, input.texCoord ).r;
    intensity = 1.0 - pow( intensity, g_Power );

    output.rgb = clamp( intensity, 0.0, 1.0 );
    output.a = 1.0;

    return output;
}
