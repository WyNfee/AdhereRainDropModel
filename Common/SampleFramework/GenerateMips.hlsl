//--------------------------------------------------------------------------------------
// GenerateMips.hlsl
//
// Implements mip generation on the gpu for D3D12
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef DX12
#define RS\
[\
    RootSignature\
    (\
       "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),\
        DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL),\
        RootConstants(b0, num32BitConstants=2, visibility=SHADER_VISIBILITY_PIXEL)"\
    )\
]
#else
#define RS
#endif

// Fullscreen quad
static const float4 positions[] = {
    float4(1, 1, 1, 1),
    float4(1, -1, 1, 1),
    float4(-1, 1, 1, 1),
    float4(-1, -1, 1, 1)
};

cbuffer GenMipsConstants : register(b0)
{
    uint mipLevel;
    uint slice;
};

// Should be called with no VB to draw 4 vertices for a fullscreen quad
RS
float4 VSMain(dword input : SV_VertexID) : SV_Position
{
    return positions[input];
}

#if Tex1D
Texture1DArray inputTexture : register(t0);

RS
float4 PSMain(float4 input : SV_Position) : SV_Target
{
    uint3 position = uint3(uint(input.x) * 2, slice, mipLevel);
    
    float4 color = inputTexture.Load(position) * 0.5;
    return color + (inputTexture.Load(position, 1) * 0.5);
}
#endif
#if Tex2D
Texture2DArray inputTexture : register(t0);

RS
float4 PSMain(float4 input : SV_Position) : SV_Target
{
    uint4 basePosition = uint4(uint2(input.xy) * 2, slice, mipLevel);
    uint4 dimensions = uint4(0, 0, slice, mipLevel);
    uint numLevels; uint numSlices;
    inputTexture.GetDimensions(mipLevel, dimensions.x, dimensions.y, numSlices, numLevels);
    dimensions.xy -= 1;
    
    float4 color = inputTexture.Load(basePosition) * 0.25;
    
    uint4 position = min(basePosition + uint4(1, 0, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.25;
    position = min(basePosition + uint4(0, 1, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.25;
    position = min(basePosition + uint4(1, 1, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.25;
    return color;
}
#endif
#if Tex3D
Texture3D inputTexture : register(t0);

RS
float4 PSMain(float4 input : SV_Position) : SV_Target
{
    uint4 basePosition = uint4(uint2(input.xy) * 2, slice, mipLevel);
    uint4 dimensions = uint4(0, 0, 0, mipLevel);
    uint numLevels;
    inputTexture.GetDimensions(mipLevel, dimensions.x, dimensions.y, dimensions.z, numLevels);
    dimensions.xyz -= 1;
    
    float4 color = inputTexture.Load(basePosition) * 0.125;
    
    uint4 position = min(basePosition + uint4(1, 0, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    position = min(basePosition + uint4(0, 1, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    position = min(basePosition + uint4(1, 1, 0, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    
    position = min(basePosition + uint4(0, 0, 1, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    position = min(basePosition + uint4(1, 0, 1, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    position = min(basePosition + uint4(0, 1, 1, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    position = min(basePosition + uint4(1, 1, 1, 0), dimensions);
    color += inputTexture.Load(position) * 0.125;
    return color;
}
#endif