//--------------------------------------------------------------------------------------
// Mesh.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct VSSceneIn
{
    float3 pos    : POSITION;            
    float3 norm : NORMAL;            
    float2 tex    : TEXCOORD0;            
};

struct PSSceneIn
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD0;
};

cbuffer cb0 : register( b0 )
{
    float4x4    g_mWorldViewProj;
};

Texture2D        g_txDiffuse : register( t0 );

SamplerState    g_sampler : register( s0 );

#ifdef DX12
#define     RS \
[\
    RootSignature\
    (\
       "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
        DescriptorTable(SRV(t0, numDescriptors = 3), visibility = SHADER_VISIBILITY_PIXEL),\
        CBV(b0, visibility = SHADER_VISIBILITY_VERTEX),\
        StaticSampler(\
            s0,\
            addressU = TEXTURE_ADDRESS_CLAMP,\
            addressV = TEXTURE_ADDRESS_CLAMP,\
            addressW = TEXTURE_ADDRESS_CLAMP,\
            comparisonFunc = COMPARISON_ALWAYS,\
            borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,\
            filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,\
            visibility = SHADER_VISIBILITY_PIXEL)"\
    )\
]
#else
#define     RS
#endif

RS
PSSceneIn VSSceneMain( VSSceneIn input )
{
    PSSceneIn output;
    
    output.pos = mul( float4( input.pos, 1.0 ), g_mWorldViewProj );
    output.tex = input.tex;
    
    return output;
}

RS
float4 PSSceneMain( PSSceneIn input ) : SV_Target
{    
    return g_txDiffuse.Sample( g_sampler, input.tex );
}



