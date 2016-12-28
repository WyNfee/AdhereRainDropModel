#ifndef HLSL_TO_C_H_INCLUDED
#define HLSL_TO_C_H_INCLUDED

// Crutches to make HLSL compile as CPP

#ifdef FXC

#define HLSL_INOUT( t, v )  inout t v
#define HLSL_OUT( t, v )    out t v
#define HLSL_CREF( t, v )   const t v
#define FLOAT3( v )         float3( (v).xxx )
#define FLOAT4( a, b )      float4( a, b )
#define CPP_EXTRA_PARAM( a )
#define HLSL_ONLY( v )      v

// To be able to paste two words together in a macro. This already exists for C
#define XSF_PASTE_( a, b )   a ## b
#define XSF_PASTE( a, b )    XSF_PASTE_( a, b )

#else

#define HLSL_INOUT( t, v )      t& v
#define HLSL_OUT( t, v )        t& v
#define HLSL_CREF( t, v )       const t& v
//#define max( a, b )        std::max( a, b )
//#define min( a, b )        std::min( a, b )
using std::min;
using std::max;
#define LOAD( a, b )            a = b
#define CPP_EXTRA_PARAM( a )    , a
#define HLSL_ONLY( v )

#include <functional>

#define     float2      XMFLOAT2
#define     float3      XMFLOAT3
#define     float4      XMFLOAT4
#define     float4x4    XMMATRIX

typedef     UINT        uint;
typedef     UINT        uint2[ 2 ];
typedef     UINT        uint3[ 3 ];
typedef     UINT        uint4[ 4 ];

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator - ( const float3& a, const float3& b )
{
    return float3( a.x - b.x, a.y - b.y, a.z - b.z );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator - ( const float3& a )
{
    return float3( -a.x, -a.y, -a.z );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator + ( const float3& a, const float3& b )
{
    return float3( a.x + b.x, a.y + b.y, a.z + b.z );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator + ( const float3& a, float b )
{
    return float3( a.x + b, a.y + b, a.z + b );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3& operator += ( float3& a, const float3& b )
{
    a = a + b;
    return a;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator / ( const float3& a, const float3& b )
{
    return float3( a.x / b.x, a.y / b.y, a.z / b.z );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator * ( const float3& a, const float3& b )
{
    return float3( a.x * b.x, a.y * b.y, a.z * b.z );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator / ( const float3& a, float b )
{
    float ib = 1.f / b;
    return float3( a.x * ib, a.y * ib, a.z * ib );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator * ( const float3& a, const float b )
{
    return float3( a.x * b, a.y * b, a.z * b );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3 operator * ( const float b, const float3& a )
{
    return float3( a.x * b, a.y * b, a.z * b );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3& operator /= ( float3& a, float b )
{
    float ib = 1.f / b;
    a.x *= ib;
    a.y *= ib;
    a.z *= ib;
    return a;
}


//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
struct FLOAT3 : float3
{
    FLOAT3( float v ) : float3( v, v, v )
    {
    }
};

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
struct FLOAT4 : float4
{
    FLOAT4( float3 a, float v ) : float4( a.x, a.y, a.z, v )
    {
    }
};

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float   saturate( float v )
{
    return std::max( 0.f, std::min( 1.f, v ) );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3   saturate( const float3& v )
{
    return float3( saturate( v.x ), saturate( v.y ), saturate( v.z ) );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float   dot( const float3& a, const float3& b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float   clamp( float v, float a, float b )
{
    return std::max( a, std::min( b, v ) );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float   length( const float3& a )
{
    return sqrtf( dot( a, a ) );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3   normalize( const float3& a )
{
    return a / length( a );
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
inline
float3   exp( const float3& a )
{
    return float3( expf( a.x ), expf( a.y ), expf( a.z ) );
}

#endif      // HLSL in C++


#endif  // HLSL_TO_C_H_INCLUDED include guard
