//--------------------------------------------------------------------------------------
// Draw.hlsl
//
// Shaders for drawing lines/quads, etc.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#ifdef DX12
#define     RS \
[\
    RootSignature\
    (\
       "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
        DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL),\
        DescriptorTable(CBV(b0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL),\
        DescriptorTable(Sampler(s0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)"\
    )\
]
#else
#define     RS
#endif


//--------------------------------------------------------------------------------------
// Name: FullScreenQuadVS
// Desc: Just and empty/dummy vertex shader, we use the geometry shader
//--------------------------------------------------------------------------------------
RS
float4 FullScreenQuadVS() : SV_Position
{
    float4 output = 0;
    return output;
}


//--------------------------------------------------------------------------------------
// Name: FullScreenQuadGS
// Desc: Geometry shader generates four vertices that spans the full viewport (-1,1)..(1,-1)
//--------------------------------------------------------------------------------------

struct FullScreenQuadVS_OUT
{
};

struct FullScreenQuadGS_OUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;    
};

RS
[maxvertexcount(4)]
void FullScreenQuadGS( point FullScreenQuadVS_OUT inputPoint[1], inout TriangleStream<FullScreenQuadGS_OUT> outputQuad, uint primitive : SV_PrimitiveID )
{
    FullScreenQuadGS_OUT output;
    
    output.position.z = 0.5;
    output.position.w = 1.0;
    
    output.position.x = -1.0;
    output.position.y = 1.0;
    output.texCoord.xy = float2( 0.0, 0.0 );
    outputQuad.Append( output );
    
    output.position.x = 1.0;
    output.position.y = 1.0;
    output.texCoord.xy = float2( 1.0, 0.0 );
    outputQuad.Append( output );
    
    output.position.x = -1.0;
    output.position.y = -1.0;
    output.texCoord.xy = float2( 0.0, 1.0 );
    outputQuad.Append( output );
        
    output.position.x = 1.0;
    output.position.y = -1.0;
    output.texCoord.xy = float2( 1.0, 1.0 );
    outputQuad.Append( output );
    
    outputQuad.RestartStrip();
}


//--------------------------------------------------------------------------------------
// Name: FullScreenQuadPS
// Desc: Pixel shader that samples the texture for the quad
//--------------------------------------------------------------------------------------

Texture2D       t0 : register( t0 );
SamplerState    s0 : register( s0 );

RS
float4 FullScreenQuadPS( FullScreenQuadGS_OUT input ) : SV_Target
{
    return t0.Sample( s0, input.texCoord );
}


//--------------------------------------------------------------------------------------
// Name: FullScreenColoredQuadPS
// Desc: Pixel shader outputs a constant color
//--------------------------------------------------------------------------------------

cbuffer cbQuadColor : register(b0)
{
    float4 g_QuadColor;
};

RS
float4 FullScreenColoredQuadPS() : SV_Target
{
    return g_QuadColor;
}


//--------------------------------------------------------------------------------------
// Name: LineVS
// Desc: Vertex shader that assumes incoming data is already 2d data
//--------------------------------------------------------------------------------------

struct LineVS_IN
{
    float4 pos  : POSITION0;
    float4 col  : COLOR0;
};

struct LineVS_OUT
{
    float4 pos  : SV_POSITION0;
    float4 col  : COLOR0;
};

RS
LineVS_OUT LineVS( LineVS_IN input )
{
    return input;
}


//--------------------------------------------------------------------------------------
// Name: LinePS
// Desc: Outputs the interpolated color from the start to end vertex
//--------------------------------------------------------------------------------------
RS
float4 LinePS( LineVS_OUT input ) : SV_Target
{
    return input.col;
}
