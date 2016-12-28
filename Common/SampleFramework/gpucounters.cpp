//--------------------------------------------------------------------------------------
// GPUCounters.cpp
//
// Contains declarations for using Xbox One alpha GPU counters
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GPUCounters.h"
#include <iterator>
#include <algorithm>
#include <functional>
#include <set>


static const DOUBLE NUM_SIMDS = 48;
static const DOUBLE NUM_SHADER_ENGINES = 2;
static const DOUBLE SU_CLOCKS_PRIM = 1;


//--------------------------------------------------------------------------------------
// Name: DerivedCounterDesc
// Desc: Defines one derived counter
//--------------------------------------------------------------------------------------
struct DerivedCounterDesc
{
    std::function< DOUBLE ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters ) >  m_func;
    GPUCounters::Counter        m_hwCounters[ 16 ];
    const WCHAR*                m_description;
    const WCHAR*                m_name;
};


//--------------------------------------------------------------------------------------
// Name: GPUCounters
// Desc: Ctor
//--------------------------------------------------------------------------------------
GPUCounters::GPUCounters()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_pPerfDevice = nullptr;
    m_pPerfContext = nullptr;
#endif
}


//--------------------------------------------------------------------------------------
// Name: ~GPUCounters
// Desc: Dtor, make sure all counter sets are deleted
//--------------------------------------------------------------------------------------
GPUCounters::~GPUCounters()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    std::for_each( m_counterSetPasses.begin(), m_counterSetPasses.end(), [] (CounterSet* p) { delete p; } );
    m_counterSetPasses.clear();
#endif
}


//--------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Obtains and keeps XboxPerformanceDevice and XboxPerformanceContext interfaces
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT GPUCounters::Initialize( XSF::D3DDevice* pDev, XSF::D3DDeviceContext* pCtx )
{
    (void)pDev;
    (void)pCtx;

#if defined(_XBOX_ONE) && defined(_TITLE)
    XSF_RETURN_IF_FAILED( pDev->QueryInterface( __uuidof(m_pPerfDevice), reinterpret_cast<void**>(&m_pPerfDevice) ) );
    XSF_SAFE_RELEASE( pDev );

    XSF_RETURN_IF_FAILED( pCtx->QueryInterface( __uuidof(m_pPerfContext), reinterpret_cast<void**>(&m_pPerfContext) ) );
    XSF_SAFE_RELEASE( pCtx );

    XSF::DebugPrint( "Got the performance device and context pointers\n" );
#endif

    m_lfGpuFrequency = GPU_FREQUENCY;
    m_lfGpuBandwidth = GPU_RAM_BANDWIDTH;

    // Always return S_OK on PC
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: StartPasses
// Desc: Given a set of hardware counters, start multiple passes over the rendering
// to capture them all.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT GPUCounters::StartPasses( UINT& count, const Counter* pCounters, UINT numCounters, std::vector< GPUCounters::CounterOutput >& dataStorage )
{
    (void)pCounters;
    count = 1;

    dataStorage.clear();
    dataStorage.resize( numCounters );

#if defined(_XBOX_ONE) && defined(_TITLE)

    std::for_each( m_counterSetPasses.begin(), m_counterSetPasses.end(), [] (CounterSet* p) { delete p; } );

    m_counters.clear();
    m_counterSetPasses.clear();
    m_currentPass = 0;

    if( !numCounters )
        return S_OK;

    // Add all as single pass counters
    // This means maximum number of passes, but also no danger that some counters can't be
    // enabled in the same pass as some other counters
    for( UINT i=0; i < numCounters; ++i )
    {
        CounterSet* pSet = new CounterSet;

        // Clear with INVALID_INDEX
        memset( &pSet->m_desc, 0xff, sizeof( pSet->m_desc ) );
        pSet->m_desc.Size    = sizeof(pSet->m_desc);
        pSet->m_desc.Version = D3D11X_COUNTER_SET_DESC_VERSION;

        // Find where the counter should be placed in the set description
        UINT* pElement = reinterpret_cast< UINT* >( &pSet->m_desc ) + pCounters[ i ].indexOfFirstElementInDescription;

        *pElement = pCounters[ i ].counterId;

        // Create
        XSF_RETURN_IF_FAILED( m_pPerfDevice->CreateCounterSet( &pSet->m_desc, &pSet->m_pSet ) );
        XSF_RETURN_IF_FAILED( m_pPerfDevice->CreateCounterSample( &pSet->m_pSample[ 0 ] ) );
        XSF_RETURN_IF_FAILED( m_pPerfDevice->CreateCounterSample( &pSet->m_pSample[ 1 ] ) );

        pSet->m_counters.push_back( i );

        m_counterSetPasses.push_back( pSet );
    }

    // Created the bunch of counter sets, now start first pass
    m_pPerfContext->StartCounters( m_counterSetPasses[ 0 ]->m_pSet );
    m_pPerfContext->SampleCounters( m_counterSetPasses[ 0 ]->m_pSample[ 0 ] );

    // Store the originals
    std::copy( pCounters, pCounters + numCounters, std::back_inserter( m_counters ) );

    count = static_cast< UINT >( m_counterSetPasses.size() );
#endif

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: EndOfPass
// Desc: Finishes the current rendering pass, captures the value of the hw counters
//--------------------------------------------------------------------------------------
HRESULT GPUCounters::EndOfPass( UINT passIndex, std::vector< GPUCounters::CounterOutput >& countersOutput )
{
    (void)passIndex;
    (void)countersOutput;

#if defined(_XBOX_ONE) && defined(_TITLE)
    CounterSet* pCs = m_counterSetPasses[ m_currentPass ];

    // Capture and stop counters reading
    m_pPerfContext->SampleCounters( pCs->m_pSample[ 1 ] );
    m_pPerfContext->StopCounters();

    D3D11X_COUNTER_DATA before, after;
    HRESULT hr;
    while( S_FALSE == ( hr = m_pPerfContext->GetCounterData( pCs->m_pSample[ 0 ], &before, 0 ) ) )
    {
    }
    XSF_ASSERT( S_OK == hr );

    while( S_FALSE == ( hr = m_pPerfContext->GetCounterData( pCs->m_pSample[ 1 ], &after, 0 ) ) )
    {
    }
    XSF_ASSERT( S_OK == hr );

    const UINT numCounters = static_cast< UINT >( pCs->m_counters.size() );

    // Retrieve the information and store it in the data array
    for( UINT i=0; i < numCounters; ++i )
    {
        const auto& counter = m_counters[ pCs->m_counters[ i ] ];
        const UINT numBlocks = counter.numberOfElementsInData / counter.numberInnerElementsInData;

        for( UINT j=0; j < numBlocks; ++j )
        {
            const UINT64 valueAfter  = (reinterpret_cast< const UINT64* >( &after  ))[ counter.indexOfFirstElementInData + j ];
            const UINT64 valueBefore = (reinterpret_cast< const UINT64* >( &before ))[ counter.indexOfFirstElementInData + j ];

            countersOutput[ m_currentPass ].m_dataAfter.push_back( valueAfter );
            countersOutput[ m_currentPass ].m_dataBefore.push_back( valueBefore );
        }
    }
#endif

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: StartNextPass
// Desc: Pass the same indices as into EndOfPass
//--------------------------------------------------------------------------------------
HRESULT GPUCounters::StartNextPass( UINT passIndex, std::vector< CounterOutput >& countersOutput )
{
    (void)passIndex;
    (void)countersOutput;

#if defined(_XBOX_ONE) && defined(_TITLE)
    if( passIndex + 1 < m_counterSetPasses.size() )
    {
        m_currentPass = passIndex + 1;
        m_pPerfContext->StartCounters( m_counterSetPasses[ m_currentPass ]->m_pSet );
        m_pPerfContext->SampleCounters( m_counterSetPasses[ m_currentPass ]->m_pSample[ 0 ] );
    }
#endif

    return S_OK;
}


#if defined(_XBOX_ONE) && defined(_TITLE)


//--------------------------------------------------------------------------------------
// Name: GetCounterIndex
// Desc: Searches for a given hardware counter in the list
//--------------------------------------------------------------------------------------
static
UINT GetCounterIndex( const GPUCounters::Counter& c, const GPUCounters::Counter* pCounters, UINT numCounters )
{
    for( UINT i=0; i < numCounters; ++i )
    {
        if( pCounters[ i ] == c )
        {
            return i;
        }
    }

    XSF_ASSERT( !"A derivative counter needs a GPU counter value -- add it to the list of counters" );

    return 0;
}


//--------------------------------------------------------------------------------------
// Name: GetCounterValue
// Desc: Finds the hardware counter in the list, returns the cpatured data
//--------------------------------------------------------------------------------------
template< typename T >
T GetCounterValue( const GPUCounters::Counter& c,
                   const GPUCounters::Counter* pCounters,
                   UINT numCounters,
                   const std::vector< GPUCounters::CounterOutput >& countersOutput,
                   UINT blockInstanceIndex )
{
    const UINT i = GetCounterIndex( c, pCounters, numCounters );

    const UINT64 v = countersOutput[ i ].m_dataAfter[ blockInstanceIndex ];

    return static_cast< T >( v );
}

//--------------------------------------------------------------------------------------
// Name: GetCounterValueSum
// Desc: Sums over the counters for N identical blocks
//--------------------------------------------------------------------------------------
static
DOUBLE GetCounterValueSum(  const GPUCounters::Counter& c,
                            const GPUCounters::Counter* pCounters,
                            UINT numCounters,
                            const std::vector< GPUCounters::CounterOutput >& countersOutput,
                            UINT i0,
                            UINT num )
{
    DOUBLE sum = 0;

    for( UINT i=i0; i < num; ++i )
    {
        sum += GetCounterValue< DOUBLE >( c, pCounters, numCounters, countersOutput, i );
    }

    return sum;
}


//--------------------------------------------------------------------------------------
// Name: GetCounterValueMax
// Desc: Sums over the counters for N identical blocks
//--------------------------------------------------------------------------------------
static
DOUBLE GetCounterValueMax(  const GPUCounters::Counter& c,
                            const GPUCounters::Counter* pCounters,
                            UINT numCounters,
                            const std::vector< GPUCounters::CounterOutput >& countersOutput,
                            UINT i0,
                            UINT num )
{
    DOUBLE max = GetCounterValue< DOUBLE >( c, pCounters, numCounters, countersOutput, i0 );

    for( UINT i=i0 + 1; i < num; ++i )
    {
        max = std::max( max, GetCounterValue< DOUBLE >( c, pCounters, numCounters, countersOutput, i ) );
    }

    return max;
}

//--------------------------------------------------------------------------------------
// Name: GetDeltaCounterValue
// Desc: Finds the hardware counter in the list, returns the delta of the data value
//--------------------------------------------------------------------------------------
static
DOUBLE GetDeltaCounterValue(    const GPUCounters::Counter& c,
                                const GPUCounters::Counter* pCounters,
                                UINT numCounters,
                                const std::vector< GPUCounters::CounterOutput >& countersOutput,
                                UINT blockInstanceIndex )
{
    const UINT i = GetCounterIndex( c, pCounters, numCounters );

    const DOUBLE v = static_cast< DOUBLE >( countersOutput[ i ].m_dataAfter[ blockInstanceIndex ] ) -
                     static_cast< DOUBLE >( countersOutput[ i ].m_dataBefore[ blockInstanceIndex ] );

    return v;
}


//--------------------------------------------------------------------------------------
// Name: GetDeltaCounterValueSum
// Desc: Sums over the counters for N identical blocks
//--------------------------------------------------------------------------------------
static
DOUBLE GetDeltaCounterValueSum( const GPUCounters::Counter& c,
                                const GPUCounters::Counter* pCounters,
                                UINT numCounters,
                                const std::vector< GPUCounters::CounterOutput >& countersOutput,
                                UINT i0,
                                UINT num )
{
    DOUBLE sum = 0;

    for( UINT i=i0; i < num; ++i )
    {
        sum += GetDeltaCounterValue( c, pCounters, numCounters, countersOutput, i );
    }

    return sum;
}

//--------------------------------------------------------------------------------------
// Name: GetDeltaCounterValueMax
// Desc: Sums over the counters for N identical blocks
//--------------------------------------------------------------------------------------
static
DOUBLE GetDeltaCounterValueMax( const GPUCounters::Counter& c,
                                const GPUCounters::Counter* pCounters,
                                UINT numCounters,
                                const std::vector< GPUCounters::CounterOutput >& countersOutput,
                                UINT i0,
                                UINT num )
{
    DOUBLE max = GetDeltaCounterValue( c, pCounters, numCounters, countersOutput, i0 );

    for( UINT i=i0 + 1; i < num; ++i )
    {
        max = std::max( max, GetDeltaCounterValue( c, pCounters, numCounters, countersOutput, i ) );
    }

    return max;
}




// Helpful macros to save typing
#define V( domain, name )      GetCounterValue< DOUBLE >( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, 0 )
#define Vi( domain, name, i )  GetCounterValue< DOUBLE >( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, i )
#define ViSum( domain, name )  GetCounterValueSum( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, 0, _countof( ((D3D11X_COUNTER_DATA*)0)->##domain[ 0 ] ) )
#define ViMax( domain, name )  GetCounterValueMax( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, 0, _countof( ((D3D11X_COUNTER_DATA*)0)->##domain[ 0 ] ) )
#define Vdi( domain, name, i ) GetDeltaCounterValue( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, i )
#define VdiSum( domain, name ) GetDeltaCounterValueSum( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, 0, _countof( ((D3D11X_COUNTER_DATA*)0)->##domain[ 0 ] ) )
#define VdiMax( domain, name ) GetDeltaCounterValueMax( GPU_COUNTER( domain, name ), phwCounters, numCounters, countersOutput, 0, _countof( ((D3D11X_COUNTER_DATA*)0)->##domain[ 0 ] ) )


//--------------------------------------------------------------------------------------
// Descriptions of derived counters. They must follow in the order enums are defined.
// Each entry defines a calculation function, a set of hardware counters needed to perform the calculation
// and text description
//--------------------------------------------------------------------------------------
static const DerivedCounterDesc g_derivedCountersDescriptions[] =
{
// Turn off code analysis for this file while we're investigating a problem
#ifndef CODE_ANALYSIS
    // DC_time = 0 in microseconds
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );

            return 1000. * 1000. * GRBM_GRBM_PERF_SEL_GUI_ACTIVE / GPUCounters::GetGpuFrequency();
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
        },
        L"GPU busy time in microseconds",
        L"GPUTime",
    },
    // DC_TessellatorBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );

            const DOUBLE VGT_PERF_VGT_TE11_BUSY = ViMax( VGT, VGT_PERF_VGT_TE11_BUSY );

            return VGT_PERF_VGT_TE11_BUSY / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_TE11_BUSY ),
        },
        L"The percentage of time the tessellation engine is busy",
        L"TessellatorBusy",
    },
    // DC_VSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_LS_WAVE0 = Vi( SPI, SPI_PERF_LS_WAVE, 0 );
            const DOUBLE SPI_PERF_LS_WAVE1 = Vi( SPI, SPI_PERF_LS_WAVE, 1 );
            const DOUBLE SPI_PERF_LS_BUSY0 = Vi( SPI, SPI_PERF_LS_BUSY, 0 );
            const DOUBLE SPI_PERF_LS_BUSY1 = Vi( SPI, SPI_PERF_LS_BUSY, 1 );
            const DOUBLE SPI_PERF_ES_WAVE0 = Vi( SPI, SPI_PERF_ES_WAVE, 0 );
            const DOUBLE SPI_PERF_ES_WAVE1 = Vi( SPI, SPI_PERF_ES_WAVE, 1 );
            const DOUBLE SPI_PERF_ES_BUSY0 = Vi( SPI, SPI_PERF_ES_BUSY, 0 );
            const DOUBLE SPI_PERF_ES_BUSY1 = Vi( SPI, SPI_PERF_ES_BUSY, 1 );
            const DOUBLE SPI_PERF_VS_WAVE0 = Vi( SPI, SPI_PERF_VS_WAVE, 0 );
            const DOUBLE SPI_PERF_VS_WAVE1 = Vi( SPI, SPI_PERF_VS_WAVE, 1 );
            const DOUBLE SPI_PERF_VS_BUSY0 = Vi( SPI, SPI_PERF_VS_BUSY, 0 );
            const DOUBLE SPI_PERF_VS_BUSY1 = Vi( SPI, SPI_PERF_VS_BUSY, 1 );

            return std::min(100., std::max( ( SPI_PERF_LS_WAVE1 != 0 ? SPI_PERF_LS_BUSY1 :
                                                ( SPI_PERF_ES_WAVE1 != 0 ? SPI_PERF_ES_BUSY1 :
                                                    ( SPI_PERF_VS_WAVE1 != 0 ? SPI_PERF_VS_BUSY1 : 0 ) ) ),
                                            ( SPI_PERF_LS_WAVE0 != 0 ? SPI_PERF_LS_BUSY0 :
                                                ( SPI_PERF_ES_WAVE0 != 0 ? SPI_PERF_ES_BUSY0 :
                                                    ( SPI_PERF_VS_WAVE0 != 0 ? SPI_PERF_VS_BUSY0 : 0 ) ) ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_LS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_LS_BUSY ),
            GPU_COUNTER( SPI, SPI_PERF_ES_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_ES_BUSY ),
            GPU_COUNTER( SPI, SPI_PERF_VS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_VS_BUSY ),
        },
        L"The percentage of time the GPU has vertex shader work to do",
        L"VSBusy",
    },
    // DC_HSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_HS_WAVE0 = Vi( SPI, SPI_PERF_HS_WAVE, 0 );
            const DOUBLE SPI_PERF_HS_WAVE1 = Vi( SPI, SPI_PERF_HS_WAVE, 1 );
            const DOUBLE SPI_PERF_HS_BUSY0 = Vi( SPI, SPI_PERF_HS_BUSY, 0 );
            const DOUBLE SPI_PERF_HS_BUSY1 = Vi( SPI, SPI_PERF_HS_BUSY, 1 );

            return std::min( 100., std::max( ( SPI_PERF_HS_WAVE1 != 0 ? SPI_PERF_HS_BUSY1 : 0 ),
                                             ( SPI_PERF_HS_WAVE0 != 0 ? SPI_PERF_HS_BUSY0 : 0 ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_HS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_HS_BUSY ),
        },
        L"The percentage of time the GPU has hull shader work to do",
        L"HSBusy",
    },
    // DC_DSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_LS_WAVE0 = Vi( SPI, SPI_PERF_LS_WAVE, 0 );
            const DOUBLE SPI_PERF_LS_WAVE1 = Vi( SPI, SPI_PERF_LS_WAVE, 1 );
            const DOUBLE SPI_PERF_ES_WAVE0 = Vi( SPI, SPI_PERF_ES_WAVE, 0 );
            const DOUBLE SPI_PERF_ES_WAVE1 = Vi( SPI, SPI_PERF_ES_WAVE, 1 );
            const DOUBLE SPI_PERF_ES_BUSY0 = Vi( SPI, SPI_PERF_ES_BUSY, 0 );
            const DOUBLE SPI_PERF_ES_BUSY1 = Vi( SPI, SPI_PERF_ES_BUSY, 1 );
            const DOUBLE SPI_PERF_VS_BUSY0 = Vi( SPI, SPI_PERF_VS_BUSY, 0 );
            const DOUBLE SPI_PERF_VS_BUSY1 = Vi( SPI, SPI_PERF_VS_BUSY, 1 );

            return std::min( 100., std::max( ( SPI_PERF_LS_WAVE1 != 0 ? ( SPI_PERF_ES_WAVE1 != 0 ? SPI_PERF_ES_BUSY1 : SPI_PERF_VS_BUSY1 ) : 0 ),
                                             ( SPI_PERF_LS_WAVE0 != 0 ? ( SPI_PERF_ES_WAVE0 != 0 ? SPI_PERF_ES_BUSY0 : SPI_PERF_VS_BUSY0 ) : 0 ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_LS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_LS_BUSY ),
            GPU_COUNTER( SPI, SPI_PERF_ES_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_ES_BUSY ),
            GPU_COUNTER( SPI, SPI_PERF_VS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_VS_BUSY ),
        },
        L"The percentage of time the GPU has domain shader work to do",
        L"DSBusy",
    },
    // DC_GSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_GS_WAVE0 = Vi( SPI, SPI_PERF_GS_WAVE, 0 );
            const DOUBLE SPI_PERF_GS_WAVE1 = Vi( SPI, SPI_PERF_GS_WAVE, 1 );
            const DOUBLE SPI_PERF_GS_BUSY0 = Vi( SPI, SPI_PERF_GS_BUSY, 0 );
            const DOUBLE SPI_PERF_GS_BUSY1 = Vi( SPI, SPI_PERF_GS_BUSY, 1 );

            return std::min( 100., std::max( ( SPI_PERF_GS_WAVE1 != 0 ? SPI_PERF_GS_BUSY1 : 0 ), ( SPI_PERF_GS_WAVE0 != 0 ? SPI_PERF_GS_BUSY0 : 0 ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_GS_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_GS_BUSY ),
        },
        L"The percentage of time the GPU has geometry shader work to do",
        L"GSBusy",
    },
    // DC_PSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_PS_CTL_WAVE0 = Vi( SPI, SPI_PERF_PS_CTL_WAVE, 0 );
            const DOUBLE SPI_PERF_PS_CTL_WAVE1 = Vi( SPI, SPI_PERF_PS_CTL_WAVE, 1 );
            const DOUBLE SPI_PERF_PS_CTL_BUSY0 = Vi( SPI, SPI_PERF_PS_CTL_BUSY, 0 );
            const DOUBLE SPI_PERF_PS_CTL_BUSY1 = Vi( SPI, SPI_PERF_PS_CTL_BUSY, 1 );

            return std::max( ( SPI_PERF_PS_CTL_WAVE1 != 0 ? SPI_PERF_PS_CTL_BUSY1 : 0 ),
                             ( SPI_PERF_PS_CTL_WAVE0 != 0 ? SPI_PERF_PS_CTL_BUSY0 : 0 ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_PS_CTL_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_PS_CTL_BUSY ),
        },
        L"The percentage of time the GPU has pixel shader work to do",
        L"PSBusy",
    },
    // DC_CSBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SPI_PERF_CSG_WAVE0 = Vi( SPI, SPI_PERF_CSG_WAVE, 0 );
            const DOUBLE SPI_PERF_CSG_WAVE1 = Vi( SPI, SPI_PERF_CSG_WAVE, 1 );
            const DOUBLE SPI_PERF_CSG_BUSY0 = Vi( SPI, SPI_PERF_CSG_BUSY, 0 );
            const DOUBLE SPI_PERF_CSG_BUSY1 = Vi( SPI, SPI_PERF_CSG_BUSY, 1 );

            return std::min( 100., std::max( ( SPI_PERF_CSG_WAVE1 != 0 ? SPI_PERF_CSG_BUSY1 : 0 ),
                                             ( SPI_PERF_CSG_WAVE0 != 0 ? SPI_PERF_CSG_BUSY0 : 0 ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_CSG_WAVE ),
            GPU_COUNTER( SPI, SPI_PERF_CSG_BUSY ),
        },
        L"The percentage of time the GPU has compute shader work to do",
        L"CSBusy",
    },
    // DC_VSVerticesIn,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE VGT_PERF_VGT_SPI_LSVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_LSVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID, 1 );
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 1 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND0 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND1 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 1 );

            const DOUBLE lsValid = VGT_PERF_VGT_SPI_LSVERT_VALID0 + VGT_PERF_VGT_SPI_LSVERT_VALID1;
            const DOUBLE esValid = VGT_PERF_VGT_SPI_ESVERT_VALID0 + VGT_PERF_VGT_SPI_ESVERT_VALID1;

            return ( lsValid != 0 ? lsValid : ( esValid != 0 ? esValid : ( VGT_PERF_VGT_SPI_VSVERT_SEND0 + VGT_PERF_VGT_SPI_VSVERT_SEND1 ) ) );
        },
        {
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND ),
        },
        L"The number of vertices processed by the VS",
        L"VSVerticesIn",
    },
    // DC_HSPatches,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE VGT_PERF_VGT_SPI_HSVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_HSVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_HSVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_HSVERT_VALID, 1 );

            return ( VGT_PERF_VGT_SPI_HSVERT_VALID0 + VGT_PERF_VGT_SPI_HSVERT_VALID1 );
        },
        {
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_HSVERT_VALID ),
        },
        L"The number of patches processed by the HS",
        L"HSPatches",
    },
    // DC_DSVerticesIn,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE VGT_PERF_VGT_SPI_LSVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_LSVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID, 1 );
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 1 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND0 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND1 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 1 );

            const DOUBLE lsValid = VGT_PERF_VGT_SPI_LSVERT_VALID0 + VGT_PERF_VGT_SPI_LSVERT_VALID1;
            const DOUBLE esValid = VGT_PERF_VGT_SPI_ESVERT_VALID0 + VGT_PERF_VGT_SPI_ESVERT_VALID1;

            return ( lsValid != 0 ? ( esValid != 0 ? esValid : ( VGT_PERF_VGT_SPI_VSVERT_SEND0 + VGT_PERF_VGT_SPI_VSVERT_SEND1 ) ) : 0 );
        },
        {
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_LSVERT_VALID ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND ),
        },
        L"The number of vertices processed by the DS",
        L"DSVerticesIn",
    },
    // DC_GSPrimsIn,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE VGT_PERF_VGT_SPI_GSPRIM_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_GSPRIM_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_GSPRIM_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_GSPRIM_VALID, 1 );

            return ( VGT_PERF_VGT_SPI_GSPRIM_VALID0 + VGT_PERF_VGT_SPI_GSPRIM_VALID1 );
        },
        {
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_GSPRIM_VALID ),
        },
        L"The number of primitives passed into the GS",
        L"GSPrimsIn",
    },
    // DC_GSVerticesOut,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID0 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_ESVERT_VALID1 = Vi( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID, 1 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND0 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 0 );
            const DOUBLE VGT_PERF_VGT_SPI_VSVERT_SEND1 = Vi( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND, 1 );

            return ( VGT_PERF_VGT_SPI_ESVERT_VALID0 + VGT_PERF_VGT_SPI_ESVERT_VALID1 ) != 0 ? ( VGT_PERF_VGT_SPI_VSVERT_SEND0 + VGT_PERF_VGT_SPI_VSVERT_SEND1 ) : 0;
        },
        {
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_ESVERT_VALID ),
            GPU_COUNTER( VGT, VGT_PERF_VGT_SPI_VSVERT_SEND ),
        },
        L"The number of vertices output by the GS",
        L"GSVerticesOut",
    },
    // DC_PrimitiveAssemblyBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE PAPC_PERF_CLIP_BUSY0 = Vi( SU, PAPC_PERF_CLIP_BUSY, 0 );
            const DOUBLE PAPC_PERF_CLIP_BUSY1 = Vi( SU, PAPC_PERF_CLIP_BUSY, 1 );
            const DOUBLE PAPC_PERF_SU_STALLED_SC0 = Vi( SU, PAPC_PERF_SU_STALLED_SC, 0 );
            const DOUBLE PAPC_PERF_SU_STALLED_SC1 = Vi( SU, PAPC_PERF_SU_STALLED_SC, 1 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_PRIM1 = Vi( SU, PAPC_PERF_SU_OUTPUT_PRIM, 0 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_PRIM0 = Vi( SU, PAPC_PERF_SU_OUTPUT_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_PRIM_DUAL1 = Vi( SU, PAPC_PERF_SU_OUTPUT_PRIM_DUAL, 0 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_PRIM_DUAL0 = Vi( SU, PAPC_PERF_SU_OUTPUT_PRIM_DUAL, 1 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_CLIP_PRIM1 = Vi( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM, 0 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_CLIP_PRIM0 = Vi( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL1 = Vi( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL, 0 );
            const DOUBLE PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL0 = Vi( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL, 1 );
            
            return std::min( 100.,
                            std::max( 0.,
                                std::max( ( ( PAPC_PERF_CLIP_BUSY1 - PAPC_PERF_SU_STALLED_SC1 ) - ( PAPC_PERF_SU_OUTPUT_PRIM1 + PAPC_PERF_SU_OUTPUT_PRIM_DUAL1 + PAPC_PERF_SU_OUTPUT_CLIP_PRIM1 + PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL1 * 2 ) * SU_CLOCKS_PRIM ),
                                          ( ( PAPC_PERF_CLIP_BUSY0 - PAPC_PERF_SU_STALLED_SC0 ) - ( PAPC_PERF_SU_OUTPUT_PRIM0 + PAPC_PERF_SU_OUTPUT_PRIM_DUAL0 + PAPC_PERF_SU_OUTPUT_CLIP_PRIM0 + PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL0 * 2 ) * SU_CLOCKS_PRIM ) ) ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SU, PAPC_PERF_CLIP_BUSY ),
            GPU_COUNTER( SU, PAPC_PERF_SU_STALLED_SC ),
            GPU_COUNTER( SU, PAPC_PERF_SU_OUTPUT_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_OUTPUT_PRIM_DUAL ),
            GPU_COUNTER( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_OUTPUT_CLIP_PRIM_DUAL ),
        },
        L"The percentage of time that primitive assembly (clipping and culling) is busy. High values may be caused by having many small primitives; mid to low values may indicate pixel shader or output buffer bottleneck",
        L"PrimitiveAssemblyBusy",
    },
    // DC_PrimitivesIn,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE PAPC_PERF_PA_INPUT_PRIM0 = Vi( SU, PAPC_PERF_PA_INPUT_PRIM, 0 );
            const DOUBLE PAPC_PERF_PA_INPUT_PRIM1 = Vi( SU, PAPC_PERF_PA_INPUT_PRIM, 1 );

            return ( PAPC_PERF_PA_INPUT_PRIM0 + PAPC_PERF_PA_INPUT_PRIM1 );
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SU, PAPC_PERF_PA_INPUT_PRIM ),
        },
        L"The number of primitives received by the hardware, including primitives generated by tessellation",
        L"PrimitivesIn",
    },
    // DC_CulledPrims,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE PAPC_PERF_CLPR_CULL_PRIM0 = Vi( SU, PAPC_PERF_CLPR_CULL_PRIM, 0 );
            const DOUBLE PAPC_PERF_CLPR_CULL_PRIM1 = Vi( SU, PAPC_PERF_CLPR_CULL_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_ZERO_AREA_CULL_PRIM0 = Vi( SU, PAPC_PERF_SU_ZERO_AREA_CULL_PRIM, 0 );
            const DOUBLE PAPC_PERF_SU_ZERO_AREA_CULL_PRIM1 = Vi( SU, PAPC_PERF_SU_ZERO_AREA_CULL_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_BACK_FACE_CULL_PRIM0 = Vi( SU, PAPC_PERF_SU_BACK_FACE_CULL_PRIM, 0 );
            const DOUBLE PAPC_PERF_SU_BACK_FACE_CULL_PRIM1 = Vi( SU, PAPC_PERF_SU_BACK_FACE_CULL_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_FRONT_FACE_CULL_PRIM0 = Vi( SU, PAPC_PERF_SU_FRONT_FACE_CULL_PRIM, 0 );
            const DOUBLE PAPC_PERF_SU_FRONT_FACE_CULL_PRIM1 = Vi( SU, PAPC_PERF_SU_FRONT_FACE_CULL_PRIM, 1 );
            const DOUBLE PAPC_PERF_SU_POLYMODE_FACE_CULL0 = Vi( SU, PAPC_PERF_SU_POLYMODE_FACE_CULL, 0 );
            const DOUBLE PAPC_PERF_SU_POLYMODE_FACE_CULL1 = Vi( SU, PAPC_PERF_SU_POLYMODE_FACE_CULL, 1 );

            return  PAPC_PERF_CLPR_CULL_PRIM0 + PAPC_PERF_CLPR_CULL_PRIM1 +
                    PAPC_PERF_SU_ZERO_AREA_CULL_PRIM0 + PAPC_PERF_SU_ZERO_AREA_CULL_PRIM1 +
                    PAPC_PERF_SU_BACK_FACE_CULL_PRIM0 + PAPC_PERF_SU_BACK_FACE_CULL_PRIM1 +
                    PAPC_PERF_SU_FRONT_FACE_CULL_PRIM0 + PAPC_PERF_SU_FRONT_FACE_CULL_PRIM1 +
                    PAPC_PERF_SU_POLYMODE_FACE_CULL0 + PAPC_PERF_SU_POLYMODE_FACE_CULL1;
        },
        {
            GPU_COUNTER( SU, PAPC_PERF_CLPR_CULL_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_ZERO_AREA_CULL_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_BACK_FACE_CULL_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_FRONT_FACE_CULL_PRIM ),
            GPU_COUNTER( SU, PAPC_PERF_SU_POLYMODE_FACE_CULL ),
        },
        L"The number of culled primitives. Typical reasons include scissor, the primitive having zero area, and back or front face culling",
        L"CulledPrims",
    },
    // DC_ClippedPrims,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE PAPC_PERF_CLPR_VVUCP_CLIP_PRIM0 = Vi( SU, PAPC_PERF_CLPR_VVUCP_CLIP_PRIM, 0 );
            const DOUBLE PAPC_PERF_CLPR_VVUCP_CLIP_PRIM1 = Vi( SU, PAPC_PERF_CLPR_VVUCP_CLIP_PRIM, 1 );

            return PAPC_PERF_CLPR_VVUCP_CLIP_PRIM0 + PAPC_PERF_CLPR_VVUCP_CLIP_PRIM1;
        },
        {
            GPU_COUNTER( SU, PAPC_PERF_CLPR_VVUCP_CLIP_PRIM ),
        },
        L"The number of primitives that required one or more clipping operations due to intersecting the view volume or user clip planes",
        L"ClippedPrims",
    },
    // DC_PAStalledOnRasterizer,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE PAPC_PERF_SU_STALLED_SC0 = Vi( SU, PAPC_PERF_SU_STALLED_SC, 0 );
            const DOUBLE PAPC_PERF_SU_STALLED_SC1 = Vi( SU, PAPC_PERF_SU_STALLED_SC, 1 );

            return std::max( PAPC_PERF_SU_STALLED_SC1, PAPC_PERF_SU_STALLED_SC0 ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SU, PAPC_PERF_SU_STALLED_SC ),
        },
        L"The percentage of time that primitive assembly waits for rasterization to be ready to accept data. This roughly indicates for what percentage of time the pipeline is bottlenecked by pixel operations",
        L"PAStalledOnRasterizer",
    },
    // DC_PSPixelsOut,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SX_PERF_SEL_DB0_PIXELS = ViSum( SX, SX_PERF_SEL_DB0_PIXELS );
            const DOUBLE SX_PERF_SEL_DB1_PIXELS = ViSum( SX, SX_PERF_SEL_DB1_PIXELS );

            return  SX_PERF_SEL_DB0_PIXELS +
                    SX_PERF_SEL_DB1_PIXELS;
                    
        },
        {
            GPU_COUNTER( SX, SX_PERF_SEL_DB0_PIXELS ),
            GPU_COUNTER( SX, SX_PERF_SEL_DB1_PIXELS ),
        },
        L"Pixels exported from shader to colour buffers. Does not include killed or alpha tested pixels; if there are multiple rendertargets, each rendertarget receives one export, so this will be 2 for 1 pixel written to two RTs",
        L"PSPixelsOut",
    },
    // DC_VSExportStalls,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );

            const DOUBLE SPI_PERF_VS_POS0_STALL = ViMax( SPI, SPI_PERF_VS_POS0_STALL );
            const DOUBLE SPI_PERF_VS_POS1_STALL = ViMax( SPI, SPI_PERF_VS_POS1_STALL );

            return std::max( SPI_PERF_VS_POS0_STALL, SPI_PERF_VS_POS1_STALL ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SPI, SPI_PERF_VS_POS0_STALL ),
            GPU_COUNTER( SPI, SPI_PERF_VS_POS1_STALL ),
        },
        L"The percentage of time vertex shader position export stalls. Indicates a bottleneck after the VS, possibly CULL/CLIP",
        L"VSExportStalls",
    },
    // DC_PSExportStalls,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );

            const DOUBLE SX_PERF_SEL_DB0_PIXEL_STALL = ViMax( SX, SX_PERF_SEL_DB0_PIXEL_STALL );
            const DOUBLE SX_PERF_SEL_DB1_PIXEL_STALL = ViMax( SX, SX_PERF_SEL_DB1_PIXEL_STALL );
            

            return std::max( SX_PERF_SEL_DB1_PIXEL_STALL, SX_PERF_SEL_DB0_PIXEL_STALL ) / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SX, SX_PERF_SEL_DB0_PIXEL_STALL ),
            GPU_COUNTER( SX, SX_PERF_SEL_DB1_PIXEL_STALL ),
        },
        L"The percentage of time pixel shader output stalls. Should be zero for PS or further upstream limited cases; if not zero, indicates a bottleneck in late Z testing or in the colour buffer",
        L"PSExportStalls",
    },
    // DC_CSThreadGroups,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SPI_PERF_CSG_NUM_THREADGROUPS = ViSum( SPI, SPI_PERF_CSG_NUM_THREADGROUPS );

            return SPI_PERF_CSG_NUM_THREADGROUPS;
        },
        {
            GPU_COUNTER( SPI, SPI_PERF_CSG_NUM_THREADGROUPS ),
        },
        L"Total number of thread groups",
        L"CSThreadGroups",
    },
    // DC_CSWavefronts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SPI_PERF_CSG_WAVE = ViSum( SPI, SPI_PERF_CSG_WAVE );

            return SPI_PERF_CSG_WAVE;
        },
        {
            GPU_COUNTER( SPI, SPI_PERF_CSG_WAVE  ),
        },
        L"The total number of shader vectors used for the CS",
        L"CSWavefronts",
    },
    // DC_CSThreads,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SPI_PERF_CSG_NUM_THREADGROUPS = ViSum( SPI, SPI_PERF_CSG_NUM_THREADGROUPS );
            const DOUBLE SQ_PERF_SEL_ITEMS = ViSum( SQ, SQ_PERF_SEL_ITEMS );

            return SPI_PERF_CSG_NUM_THREADGROUPS != 0 ? SQ_PERF_SEL_ITEMS : 0;
        },
        {
            GPU_COUNTER( SPI, SPI_PERF_CSG_NUM_THREADGROUPS ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_ITEMS )
        },
        L"The number of CS threads processed by the hardware",
        L"CSThreads"
    },
    // DC_CSVALUInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );
            const DOUBLE SQ_PERF_SEL_INSTS_VALU = ViSum( SQ, SQ_PERF_SEL_INSTS_VALU );

            return SQ_PERF_SEL_INSTS_VALU / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_WAVES ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INSTS_VALU ),
        },
        L"The average number of vector ALU instructions executed per work-item, affected by flow control",
        L"CSVALUInsts"
    },
    // DC_CSVALUUtilization,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_THREAD_CYCLES_VALU = ViSum( SQ, SQ_PERF_SEL_THREAD_CYCLES_VALU );
            const DOUBLE SQ_PERF_SEL_INST_CYCLES_VALU = ViSum( SQ, SQ_PERF_SEL_INST_CYCLES_VALU );
            
            return SQ_PERF_SEL_THREAD_CYCLES_VALU / SQ_PERF_SEL_INST_CYCLES_VALU / 64 * 100;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_THREAD_CYCLES_VALU ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INST_CYCLES_VALU ),
        },
        L"The percentage of active vector ALU threads in a shader vector. A lower number can mean either more thread divergence in a shader vector or that the work-group size is not a multiple of 64. Value range: 0% (bad), 100% (ideal - no thread divergence)",
        L"CSVALUUtilization"
    },
    // DC_CSSALUInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );
            const DOUBLE SQ_PERF_SEL_INSTS_SALU = ViSum( SQ, SQ_PERF_SEL_INSTS_SALU );

            return SQ_PERF_SEL_INSTS_SALU / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_WAVES ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INSTS_SALU ),
        },
        L"The average number of scalar ALU instructions executed per work-item",
        L"CSSALUInsts"
    },
    // DC_CSVFetchInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );
            const DOUBLE SQ_PERF_SEL_INSTS_VMEM_RD = ViSum( SQ, SQ_PERF_SEL_INSTS_VMEM_RD );

            return SQ_PERF_SEL_INSTS_VMEM_RD / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_WAVES ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INSTS_VMEM_RD ),
        },
        L"The average number of vector fetch instructions from the video memory executed per work-item",
        L"CSVFetchInsts"
    },
    // DC_CSSFetchInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );
            const DOUBLE SQ_PERF_SEL_INSTS_SMEM = ViSum( SQ, SQ_PERF_SEL_INSTS_SMEM );

            return SQ_PERF_SEL_INSTS_SMEM / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_WAVES ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INSTS_SMEM ),
        },
        L"The average number of scalar fetch instructions from the video memory executed per work-item",
        L"CSSFetchInsts"
    },
    // DC_CSVWriteInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );
            const DOUBLE SQ_PERF_SEL_INSTS_VMEM_WR = ViSum( SQ, SQ_PERF_SEL_INSTS_VMEM_WR );

            return SQ_PERF_SEL_INSTS_VMEM_WR / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ,  SQ_PERF_SEL_WAVES ),
            GPU_COUNTER( SQ,  SQ_PERF_SEL_INSTS_VMEM_WR ),
        },
        L"The average number of vector write instructions to the video memory executed per work-item",
        L"CSVWriteInsts"
    },
    // DC_CSVALUBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SQ_PERF_SEL_INST_CYCLES_VALU = ViSum( SQ, SQ_PERF_SEL_INST_CYCLES_VALU );

            return SQ_PERF_SEL_INST_CYCLES_VALU * 4 / NUM_SIMDS / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_INST_CYCLES_VALU )
        },
        L"The percentage of time vector ALU instructions are processed. Value range: 0% (bad) to 100% (optimal)",
        L"CSVALUBusy"
    },
    // DC_CSSALUBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SQ_PERF_SEL_INSTS_SALU = ViSum( SQ, SQ_PERF_SEL_INSTS_SALU );

            return SQ_PERF_SEL_INSTS_SALU * 4 / NUM_SIMDS / NUM_SHADER_ENGINES / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_INSTS_SALU )
        },
        L"The percentage of time scalar ALU instructions are processed. Value range: 0% (bad) to 100% (optimal)",
        L"CSALUBusy"
    },
    // DC_CSMemUnitBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE TA_PERF_SEL_TA_BUSY = ViMax( TA, TA_PERF_SEL_TA_BUSY );

            return TA_PERF_SEL_TA_BUSY / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( TA, TA_PERF_SEL_TA_BUSY )
        },
        L"The percentage of time the memory unit is active. The result includes the stall time (MemUnitStalled). This is measured with all extra fetches and writes and any cache or memory effects taken into account. Value range: 0% to 100% (fetch-bound)",
        L"CSMemUnitBusy"
    },
    // DC_CSMemUnitStalled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE TCP_PERF_SEL_TCP_TA_DATA_STALL_CYCLES = VdiMax( TCP, TCP_PERF_SEL_TCP_TA_DATA_STALL_CYCLES );

            return TCP_PERF_SEL_TCP_TA_DATA_STALL_CYCLES / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( TCP, TCP_PERF_SEL_TCP_TA_DATA_STALL_CYCLES )
        },
        L"The percentage of time the memory unit is stalled. Try reduce the number or size of fetches and writes if possible. Value range: 0% (optimal) to 100% (bad)",
        L"CSMemUnitStalled"
    },
    // DC_CSFetchSize,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TCC_PERF_SEL_MC_RDREQ = ViSum( TCC, TCC_PERF_SEL_MC_RDREQ );

            return TCC_PERF_SEL_MC_RDREQ * 32;
        },
        {
            GPU_COUNTER( TCC, TCC_PERF_SEL_MC_RDREQ )
        },
        L"The total bytes fetched from the video memory. This is measured with all extra fetches and any cache or memory effects taken into account",
        L"CSFetchSize"
    },
    // DC_CSWriteSize,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TCC_PERF_SEL_MC_WRREQ = ViSum( TCC, TCC_PERF_SEL_MC_WRREQ );

            return TCC_PERF_SEL_MC_WRREQ * 32;
        },
        {
            GPU_COUNTER( TCC, TCC_PERF_SEL_MC_WRREQ )
        },
        L"The total bytes written to the video memory. This is measured with all extra fetches and any cache or memory effects taken into account",
        L"CSWriteSize"
    },
    // DC_CSCacheHit,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TCC_PERF_SEL_HIT = ViSum( TCC, TCC_PERF_SEL_HIT );
            const DOUBLE TCC_PERF_SEL_MISS = ViSum( TCC, TCC_PERF_SEL_MISS );

            return TCC_PERF_SEL_HIT / (TCC_PERF_SEL_HIT + TCC_PERF_SEL_MISS) * 100;
        },
        {
            GPU_COUNTER( TCC, TCC_PERF_SEL_HIT ),
            GPU_COUNTER( TCC, TCC_PERF_SEL_MISS ),
        },
        L"The percentage of fetch, write, atomic, and other instructions that hit the data cache. Value range: 0% (no hit) to 100% (optimal)",
        L"CSCacheHit"
    },
    // DC_CSWriteUnitStalled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE TCC_PERF_SEL_MC_WRREQ_STALL = ViMax( TCC, TCC_PERF_SEL_MC_WRREQ_STALL );

            return TCC_PERF_SEL_MC_WRREQ_STALL / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( TCC, TCC_PERF_SEL_MC_WRREQ_STALL ),
        },
        L"The percentage of time the Write unit is stalled. Value range: 0% to 100% (bad)",
        L"CSWriteUnitStalled"
    },
    // DC_CSGDSInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_INSTS_GDS = ViSum( SQ, SQ_PERF_SEL_INSTS_GDS );
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );

            return SQ_PERF_SEL_INSTS_GDS / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ, SQ_PERF_SEL_INSTS_GDS ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_WAVES ),
        },
        L"The average number of instructions to/from the GDS executed per work-item . This counter is a subset of the VALUInsts counter",
        L"CSGDSInsts"
    },
    // DC_CSLDSInsts,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SQ_PERF_SEL_INSTS_LDS = ViSum( SQ, SQ_PERF_SEL_INSTS_LDS );
            const DOUBLE SQ_PERF_SEL_WAVES= ViSum( SQ, SQ_PERF_SEL_WAVES );

            return SQ_PERF_SEL_INSTS_LDS / SQ_PERF_SEL_WAVES;
        },
        {
            GPU_COUNTER( SQ, SQ_PERF_SEL_INSTS_LDS ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_WAVES ),
        },
        L"The average number of LDS write instructions executed per work-item ",
        L"CSLDSInsts"
    },
    // DC_CSALUStalledByLDS,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SQ_PERF_SEL_WAIT_INST_LDS = ViSum( SQ, SQ_PERF_SEL_WAIT_INST_LDS );
            const DOUBLE SQ_PERF_SEL_WAVES = ViSum( SQ, SQ_PERF_SEL_WAVES );

            return SQ_PERF_SEL_WAIT_INST_LDS / SQ_PERF_SEL_WAVES / GRBM_GRBM_PERF_SEL_GUI_ACTIVE / NUM_SHADER_ENGINES * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_WAIT_INST_LDS ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_WAVES ),
        },
        L"The percentage of time ALU units are stalled by the LDS input queue being full or the output queue being not ready. If there are LDS bank conflicts, reduce them. Otherwise, try reducing the number of LDS accesses if possible. Value range: 0% (optimal) to 100% (bad)",
        L"CSALUStalledByLDS"
    },
    // DC_CSLDSBankConflict,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE SQ_PERF_SEL_LDS_BANK_CONFLICT = ViSum( SQ, SQ_PERF_SEL_LDS_BANK_CONFLICT );

            return SQ_PERF_SEL_LDS_BANK_CONFLICT / GRBM_GRBM_PERF_SEL_GUI_ACTIVE / NUM_SIMDS * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( SQ, SQ_PERF_SEL_LDS_BANK_CONFLICT ),
        },
        L"The percentage of time LDS is stalled by bank conflicts. Value range: 0% (optimal) to 100% (bad)",
        L"CSLDSBankConflict"
    },
    // DC_TexUnitBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE TA_PERF_SEL_TA_BUSY = ViMax( TA, TA_PERF_SEL_TA_BUSY );

            return TA_PERF_SEL_TA_BUSY / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( TA, TA_PERF_SEL_TA_BUSY ),
        },
        L"The percentage of time the texture unit is active. This is measured with all extra fetches and any cache or memory effects taken into account",
        L"TexUnitBusy"
    },
    // DC_TexTriFilteringPct,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TA_PERF_SEL_MIP_2_CYCLE_PIXELS = ViSum( TA, TA_PERF_SEL_MIP_2_CYCLE_PIXELS );
            const DOUBLE TA_PERF_SEL_MIP_1_CYCLE_PIXELS = ViSum( TA, TA_PERF_SEL_MIP_1_CYCLE_PIXELS );

            const DOUBLE denom = TA_PERF_SEL_MIP_2_CYCLE_PIXELS + TA_PERF_SEL_MIP_1_CYCLE_PIXELS;
            return denom ? TA_PERF_SEL_MIP_2_CYCLE_PIXELS / denom * 100 : 0;
        },
        {
            GPU_COUNTER( TA, TA_PERF_SEL_MIP_2_CYCLE_PIXELS ),
            GPU_COUNTER( TA, TA_PERF_SEL_MIP_1_CYCLE_PIXELS ),
        },
        L"Percentage of pixels that received trilinear filtering. Note that not all pixels for which trilinear filtering is enabled will receive it (e.g. if the texture is magnified)",
        L"TexTriFilteringPct"
    },
    // DC_TexVolFilteringPct,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TA_PERF_SEL_VOL_1_CYCLE_PIXELS = ViSum( TA, TA_PERF_SEL_VOL_1_CYCLE_PIXELS );
            const DOUBLE TA_PERF_SEL_VOL_2_CYCLE_PIXELS = ViSum( TA, TA_PERF_SEL_VOL_2_CYCLE_PIXELS );

            const DOUBLE denom = TA_PERF_SEL_VOL_1_CYCLE_PIXELS + TA_PERF_SEL_VOL_2_CYCLE_PIXELS;
            return denom ? TA_PERF_SEL_VOL_2_CYCLE_PIXELS / denom * 100 : 0;
        },
        {
            GPU_COUNTER( TA, TA_PERF_SEL_VOL_2_CYCLE_PIXELS ),
            GPU_COUNTER( TA, TA_PERF_SEL_VOL_1_CYCLE_PIXELS ),
        },
        L"Percentage of pixels that received volume filtering",
        L"TexVolFilteringPct"
    },
    // DC_TexAveAnisotropy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE TA_PERF_SEL_ANISO_1_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_1_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_2_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_2_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_4_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_4_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_6_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_6_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_8_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_8_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_10_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_10_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_12_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_12_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_14_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_14_CYCLE_QUADS );
            const DOUBLE TA_PERF_SEL_ANISO_16_CYCLE_QUADS = ViSum( TA, TA_PERF_SEL_ANISO_16_CYCLE_QUADS );

            const DOUBLE denom = TA_PERF_SEL_ANISO_1_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_2_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_4_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_6_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_8_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_10_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_12_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_14_CYCLE_QUADS +
                                 TA_PERF_SEL_ANISO_16_CYCLE_QUADS;


            return denom ? (     TA_PERF_SEL_ANISO_1_CYCLE_QUADS +
                             2 * TA_PERF_SEL_ANISO_2_CYCLE_QUADS +
                             4 * TA_PERF_SEL_ANISO_4_CYCLE_QUADS +
                             6 * TA_PERF_SEL_ANISO_6_CYCLE_QUADS +
                             8 * TA_PERF_SEL_ANISO_8_CYCLE_QUADS +
                            10 * TA_PERF_SEL_ANISO_10_CYCLE_QUADS +
                            12 * TA_PERF_SEL_ANISO_12_CYCLE_QUADS +
                            14 * TA_PERF_SEL_ANISO_14_CYCLE_QUADS +
                            16 * TA_PERF_SEL_ANISO_16_CYCLE_QUADS ) / denom : 0;
        },
        {
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_1_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_2_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_4_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_6_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_8_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_10_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_12_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_14_CYCLE_QUADS ),
            GPU_COUNTER( TA, TA_PERF_SEL_ANISO_16_CYCLE_QUADS ),
        },
        L"The average degree of anisotropy applied. A number between 1 and 16. The anisotropic filtering algorithm only applies samples where they are required (e.g. there will be no extra anisotropic samples if the view vector is perpendicular to the surface) so this can be much lower than the requested anisotropy",
        L"TexAveAnisotropy"
    },
    // DC_DepthStencilTestBusy,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE DB_PERF_SEL_OP_PIPE_BUSY = ViMax( DB, DB_PERF_SEL_OP_PIPE_BUSY );

            return DB_PERF_SEL_OP_PIPE_BUSY / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( DB, DB_PERF_SEL_OP_PIPE_BUSY ),
        },
        L"Percentage of time GPU spent performing depth and stencil tests relative to GPUBusy",
        L"DepthStencilTestBusy"
    },
    // DC_HiZTilesAccepted,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_DB_SC_TILE_CULLED = ViSum( DB, DB_PERF_SEL_DB_SC_TILE_CULLED );
            const DOUBLE DB_PERF_SEL_SC_DB_TILE_TILES = ViSum( DB, DB_PERF_SEL_SC_DB_TILE_TILES );

            return DB_PERF_SEL_SC_DB_TILE_TILES ? DB_PERF_SEL_DB_SC_TILE_CULLED / DB_PERF_SEL_SC_DB_TILE_TILES * 100 : 0;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_DB_SC_TILE_CULLED ),
            GPU_COUNTER( DB, DB_PERF_SEL_SC_DB_TILE_TILES ),
        },
        L"Percentage of tiles accepted by HiZ and will be rendered to the depth or color buffers",
        L"HiZTilesAccepts"
    },
    // DC_PreZTilesDetailCulled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_SC_DB_QUAD_KILLED_TILES = ViSum( DB, DB_PERF_SEL_SC_DB_QUAD_KILLED_TILES );
            const DOUBLE DB_PERF_SEL_SC_DB_TILE_TILES = ViSum( DB, DB_PERF_SEL_SC_DB_TILE_TILES );

            return DB_PERF_SEL_SC_DB_TILE_TILES ? DB_PERF_SEL_SC_DB_QUAD_KILLED_TILES / DB_PERF_SEL_SC_DB_TILE_TILES * 100 : 0;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_SC_DB_QUAD_KILLED_TILES ),
            GPU_COUNTER( DB, DB_PERF_SEL_SC_DB_TILE_TILES ),
        },
        L"Percentage of tiles rejected because the associated prim had no contributing area",
        L"PreZTilesDetailCulled"
    },
    // DC_HiZQuadsCulled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SC_QZ0_QUAD_COUNT = ViSum( SC, SC_QZ0_QUAD_COUNT );
            const DOUBLE SC_QZ1_QUAD_COUNT = ViSum( SC, SC_QZ1_QUAD_COUNT );
            

            const DOUBLE SC_P0_HIZ_QUAD_COUNT = ViSum( SC, SC_P0_HIZ_QUAD_COUNT );
            const DOUBLE SC_P1_HIZ_QUAD_COUNT = ViSum( SC, SC_P1_HIZ_QUAD_COUNT );
            
            const DOUBLE qzQuadCount = SC_QZ0_QUAD_COUNT +
                                       SC_QZ1_QUAD_COUNT;
                                       

            const DOUBLE hizQuadCount = SC_P0_HIZ_QUAD_COUNT +
                                        SC_P1_HIZ_QUAD_COUNT;
                                        

            return hizQuadCount ? ( qzQuadCount - hizQuadCount ) / qzQuadCount * 100 : 0;
        },
        {
            GPU_COUNTER( SC, SC_QZ0_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_QZ1_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_P0_HIZ_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_P1_HIZ_QUAD_COUNT ),            
        },
        L"Percentage of quads that did not have to continue on in the pipeline after HiZ. They may be written directly to the depth buffer, or culled completely. Consistently low values here may suggest that the Z-range is not being fully utilized",
        L"HiZQuadsCulled"
    },
    // DC_PreZQuadsCulled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SC_QZ0_QUAD_COUNT = ViSum( SC, SC_QZ0_QUAD_COUNT );
            const DOUBLE SC_QZ1_QUAD_COUNT = ViSum( SC, SC_QZ1_QUAD_COUNT );
           

            const DOUBLE SC_P0_HIZ_QUAD_COUNT = ViSum( SC, SC_P0_HIZ_QUAD_COUNT );
            const DOUBLE SC_P1_HIZ_QUAD_COUNT = ViSum( SC, SC_P1_HIZ_QUAD_COUNT );
           
            const DOUBLE SC_EARLYZ_QUAD_COUNT = ViSum( SC, SC_EARLYZ_QUAD_COUNT );

            const DOUBLE qzQuadCount = SC_QZ1_QUAD_COUNT +
                                       SC_QZ0_QUAD_COUNT;

            const DOUBLE hizQuadCount = SC_P1_HIZ_QUAD_COUNT +
                                        SC_P0_HIZ_QUAD_COUNT;

            return qzQuadCount ? ( hizQuadCount - SC_EARLYZ_QUAD_COUNT ) / qzQuadCount * 100 : 0;
        },
        {
            GPU_COUNTER( SC, SC_QZ0_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_QZ1_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_P0_HIZ_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_P1_HIZ_QUAD_COUNT ),
        },
        L"Percentage of quads rejected based on the detailZ and earlyZ tests",
        L"PreZQuadsCulled"
    },
    // DC_PostZQuads,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE SC_QZ0_QUAD_COUNT = ViSum( SC, SC_QZ0_QUAD_COUNT );
            const DOUBLE SC_QZ1_QUAD_COUNT = ViSum( SC, SC_QZ1_QUAD_COUNT );
           

            const DOUBLE SC_EARLYZ_QUAD_COUNT = ViSum( SC, SC_EARLYZ_QUAD_COUNT );

            const DOUBLE qzQuadCount = SC_QZ1_QUAD_COUNT +
                                       SC_QZ0_QUAD_COUNT;

            return qzQuadCount ? SC_EARLYZ_QUAD_COUNT / qzQuadCount * 100 : 0;
        },
        {
            GPU_COUNTER( SC, SC_QZ0_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_QZ1_QUAD_COUNT ),
            GPU_COUNTER( SC, SC_EARLYZ_QUAD_COUNT ),
        },
        L"Percentage of quads for which the pixel shader will run and may be postZ tested",
        L"PostZQuads"
    },
    // DC_PreZSamplesPassing,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z = ViSum( DB, DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z );

            return  DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z ),
        },
        L"Number of samples tested for Z before shading and passed",
        L"PreZSamplesPassing"
    },
    // DC_PreZSamplesFailingS,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_PREZ_SAMPLES_FAILING_S = ViSum( DB, DB_PERF_SEL_PREZ_SAMPLES_FAILING_S );

            return  DB_PERF_SEL_PREZ_SAMPLES_FAILING_S;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_PREZ_SAMPLES_FAILING_S ),
        },
        L"Number of samples tested for Z before shading and failed stencil test",
        L"PreZSamplesFailingS"
    },
    // DC_PreZSamplesFailingZ,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z = ViSum( DB, DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z );

            return  DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z ),
        },
        L"Number of samples tested for Z before shading and failed Z test",
        L"PreZSamplesFailingZ"
    },
    // DC_PostZSamplesPassing,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z = ViSum( DB, DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z );

            return  DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z ),
        },
        L"Number of samples tested for Z after shading and passed",
        L"PostZSamplesPassing"
    },
    // DC_PostZSamplesFailingS,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_POSTZ_SAMPLES_FAILING_S = ViSum( DB, DB_PERF_SEL_POSTZ_SAMPLES_FAILING_S );

            return  DB_PERF_SEL_POSTZ_SAMPLES_FAILING_S;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_POSTZ_SAMPLES_FAILING_S ),
        },
        L"Number of samples tested for Z after shading and failed stencil test",
        L"PostZSamplesFailingS"
    },
    // DC_PostZSamplesFailingZ,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z = ViSum( DB, DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z );

            return  DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z;
        },
        {
            GPU_COUNTER( DB, DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z ),
        },
        L"Number of samples tested for Z after shading and failed Z test",
        L"PostZSamplesFailingZ"
    },
    // DC_ZUnitStalled,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE GRBM_GRBM_PERF_SEL_GUI_ACTIVE = V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE );
            const DOUBLE DB_PERF_SEL_DB_CB_LQUAD_STALLS = ViMax( DB, DB_PERF_SEL_DB_CB_LQUAD_STALLS );

            return DB_PERF_SEL_DB_CB_LQUAD_STALLS / GRBM_GRBM_PERF_SEL_GUI_ACTIVE * 100;
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
            GPU_COUNTER( DB, DB_PERF_SEL_DB_CB_LQUAD_STALLS ),
        },
        L"The percentage of time the depth buffer spends waiting for the color buffer to be ready to accept data. High figures here indicate a bottleneck in color buffer operations",
        L"ZUnitStalled"
    },
    // DC_CBMemRead,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE CB_PERF_SEL_CC_MC_READ_REQUEST = VdiSum( CB, CB_PERF_SEL_CC_MC_READ_REQUEST );

            return CB_PERF_SEL_CC_MC_READ_REQUEST * 32;
        },
        {
            GPU_COUNTER( CB, CB_PERF_SEL_CC_MC_READ_REQUEST ),
        },
        L"Number of bytes read from the color buffer",
        L"CBMemRead"
    },
    // DC_CBMemWritten,
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE CB_PERF_SEL_CC_MC_WRITE_REQUEST = VdiSum( CB, CB_PERF_SEL_CC_MC_WRITE_REQUEST );

            return CB_PERF_SEL_CC_MC_WRITE_REQUEST * 32;
        },
        {
            GPU_COUNTER( CB, CB_PERF_SEL_CC_MC_WRITE_REQUEST ),
        },
        L"Number of bytes written to the color buffer",
        L"CBMemWritten"
    },
    // DC_CBSlowPixelPct,
        {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            const DOUBLE CB_PERF_SEL_EXPORT_32_ABGR_QUAD_FRAGMENT = VdiSum( CB, CB_PERF_SEL_EXPORT_32_ABGR_QUAD_FRAGMENT );
            const DOUBLE CB_PERF_SEL_DRAWN_QUAD_FRAGMENT = VdiSum( CB, CB_PERF_SEL_DRAWN_QUAD_FRAGMENT );

            return CB_PERF_SEL_DRAWN_QUAD_FRAGMENT ? std::min( 100., CB_PERF_SEL_EXPORT_32_ABGR_QUAD_FRAGMENT / CB_PERF_SEL_DRAWN_QUAD_FRAGMENT * 100 ) : 0;
        },
        {
            GPU_COUNTER( CB, CB_PERF_SEL_DRAWN_QUAD_FRAGMENT ),
            GPU_COUNTER( CB, CB_PERF_SEL_EXPORT_32_ABGR_QUAD_FRAGMENT ),
        },
        L"Percentage of pixels written to the color buffer using a half-rate or quarter-rate format",
        L"CBSlowPixelPct"
    },
    // DC_NUM_COUNTERS
#else      // CODE_ANALYSIS
    // DC_time = 0 in microseconds
    {   [] ( const GPUCounters::Counter* phwCounters, const std::vector< GPUCounters::CounterOutput >& countersOutput, UINT numCounters )
        {
            return 1000. * 1000. * V( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ) / GPUCounters::GetGpuFrequency();
        },
        {
            GPU_COUNTER( GRBM, GRBM_PERF_SEL_GUI_ACTIVE ),
        },
        L"GPU busy time in microseconds",
        L"GPUTime",
    },
#endif
};



//--------------------------------------------------------------------------------------
// Name: GetAllDerivedCounters
// Desc: Adds all derived counters to the list
//--------------------------------------------------------------------------------------
void GetAllDerivedCounters( std::vector< DerivedCounter >& derived )
{
    for( UINT i=0; i < DC_NUM_COUNTERS; ++i )
    {
        derived.push_back( static_cast< DerivedCounter >( i ) );
    }
}


//--------------------------------------------------------------------------------------
// Name: GetRequiredRawCounters
// Desc: Returns the list of hardware counters required to get the derived counters
//--------------------------------------------------------------------------------------
void GetRequiredHWCounters( const std::vector< DerivedCounter >& derivedIn, std::vector< GPUCounters::Counter >& hwOut, const std::vector< GPUCounters::Counter >& hwExtra )
{
    std::set< GPUCounters::Counter >    c;

    for( auto i : hwExtra )
    {
        c.insert( i );
    }

    for( UINT idx : derivedIn )
    {
        XSF_ASSERT( idx < DC_NUM_COUNTERS );

        for( UINT j=0; j < _countof( g_derivedCountersDescriptions[ idx ].m_hwCounters ); ++j )
        {
            if( g_derivedCountersDescriptions[ idx ].m_hwCounters[ j ].pName )
            {
                c.insert( g_derivedCountersDescriptions[ idx ].m_hwCounters[ j ] );
            } else
            {
                break;
            }
        }
    }

    for( auto k : c )
    {
        hwOut.push_back( k );
    }
}




//--------------------------------------------------------------------------------------
// Name: ComputeDerived
// Desc: Compute the values of derived counters given the captured hardware counters
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ComputeDerivedCounters(    DOUBLE* pResults,
                                const std::vector< DerivedCounter >& derivedIn,
                                const GPUCounters::Counter* phwCounters,
                                const std::vector< GPUCounters::CounterOutput >& countersOutput,
                                UINT numhwCounters )
{
    XSF_ASSERT( pResults );
    XSF_ASSERT( phwCounters );
    XSF_ASSERT( countersOutput.size() == numhwCounters );

    ZeroMemory( pResults, derivedIn.size() * sizeof( pResults[ 0 ] ) );

    for( UINT i=0; i < derivedIn.size(); ++i )
    {
        const UINT idx = derivedIn[ i ];
        XSF_ASSERT( idx < DC_NUM_COUNTERS );

        if( g_derivedCountersDescriptions[ idx ].m_func )
        {
            pResults[ idx ] = g_derivedCountersDescriptions[ idx ].m_func( phwCounters, countersOutput, numhwCounters );
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetDerivedCounterDescription
// Desc: Returns text description of the derived counter
//--------------------------------------------------------------------------------------
const WCHAR* GetDerivedCounterDescription( DerivedCounter c )
{
    XSF_ASSERT( c < DC_NUM_COUNTERS );

    return g_derivedCountersDescriptions[ c ].m_description;
}

//--------------------------------------------------------------------------------------
// Name: GetRequiredRawCounters
// Desc: Returns text description of the derived counter
//--------------------------------------------------------------------------------------
const WCHAR* GetDerivedCounterName( DerivedCounter c )
{
    XSF_ASSERT( c < DC_NUM_COUNTERS );

    return g_derivedCountersDescriptions[ c ].m_name;
}









//--------------------------------------------------------------------------------------
// Name: DerivedCounters
// Desc: Ctor
//--------------------------------------------------------------------------------------
DerivedCounters::DerivedCounters()
{
    ZeroMemory( m_results, sizeof( m_results ) );
    m_pCounters = NULL;
    m_numPasses = 0;
}


//--------------------------------------------------------------------------------------
// Name: EnableAllCounters
// Desc: Enables all derived counters
//--------------------------------------------------------------------------------------
void DerivedCounters::EnableAllCounters()
{
    DisableAllCounters();

    GetAllDerivedCounters( m_derived );
    GetRequiredHWCounters( m_derived, m_hw, m_hwExtra );
}

//--------------------------------------------------------------------------------------
// Name: EnableCSCounters
// Desc: Enables Compute Shader counters
//--------------------------------------------------------------------------------------
void DerivedCounters::EnableCSCounters()
{
    DisableAllCounters();

    m_derived.push_back( DC_GPUTime );
    m_derived.push_back( DC_CSBusy );
    m_derived.push_back( DC_CSThreadGroups );
    m_derived.push_back( DC_CSWavefronts );
    m_derived.push_back( DC_CSThreads );
    m_derived.push_back( DC_CSVALUInsts );
    m_derived.push_back( DC_CSVALUUtilization );
    m_derived.push_back( DC_CSSALUInsts );
    m_derived.push_back( DC_CSVFetchInsts );
    m_derived.push_back( DC_CSSFetchInsts );
    m_derived.push_back( DC_CSVWriteInsts );
    m_derived.push_back( DC_CSVALUBusy );
    m_derived.push_back( DC_CSSALUBusy );
    m_derived.push_back( DC_CSMemUnitBusy );
    m_derived.push_back( DC_CSMemUnitStalled );
    m_derived.push_back( DC_CSFetchSize );
    m_derived.push_back( DC_CSWriteSize );
    m_derived.push_back( DC_CSCacheHit );
    m_derived.push_back( DC_CSWriteUnitStalled );
    m_derived.push_back( DC_CSGDSInsts );
    m_derived.push_back( DC_CSLDSInsts );
    m_derived.push_back( DC_CSALUStalledByLDS );
    m_derived.push_back( DC_CSLDSBankConflict );

    GetRequiredHWCounters( m_derived, m_hw, m_hwExtra );
}

//--------------------------------------------------------------------------------------
// Name: DisablesAllCounters
// Desc: Disables all derived counters
//--------------------------------------------------------------------------------------
void DerivedCounters::DisableAllCounters()
{
    m_hwExtra.clear();
    m_derived.clear();
    m_hw.clear();
    m_output.clear();
}

//--------------------------------------------------------------------------------------
// Name: FromGivenCounters
// Desc: Sets up this object to query the given hardware and derived counters
//--------------------------------------------------------------------------------------
void DerivedCounters::FromGivenCounters( const std::vector< GPUCounters::Counter >& hwCounters,
                                         const std::vector< DerivedCounter >& derivedCounters )
{
    DisableAllCounters();

    m_derived = derivedCounters;
    m_hw = hwCounters;
}

//--------------------------------------------------------------------------------------
// Name: AddDerivedCounter
// Desc: Add a single derived counter to this object
//--------------------------------------------------------------------------------------
void DerivedCounters::AddDerivedCounter( DerivedCounter c )
{
    m_hw.clear();
    m_output.clear();

    m_derived.push_back( c );
    GetRequiredHWCounters( m_derived, m_hw, m_hwExtra );
}


//--------------------------------------------------------------------------------------
// Name: AddHWCounter
// Desc: Add a single hw counter to this object
//--------------------------------------------------------------------------------------
void DerivedCounters::AddHWCounter( const GPUCounters::Counter& c )
{
    m_hwExtra.push_back( c );

    // Only add a HW counter once for speed reasons
    auto i = m_hw.begin();
    for( ; i != m_hw.end(); ++i )
    {
        if( (*i) == c )
            break;
    }

    if( i == m_hw.end() )
        m_hw.push_back( c );
}

//--------------------------------------------------------------------------------------
// Name: StartPasses
// Desc: Starts reading the hardware counters over several passes, returns the number
// of passes required
//--------------------------------------------------------------------------------------
UINT DerivedCounters::StartPasses( GPUCounters* pCounters )
{
    XSF_ASSERT( m_pCounters == NULL );
    XSF_ASSERT( m_numPasses == 0 );

    if( !m_hw.empty() )
    {
        XSF_ERROR_IF_FAILED( pCounters->StartPasses( m_numPasses, &m_hw[ 0 ], static_cast< UINT >( m_hw.size() ), m_output ) );
    } else
    {
        m_numPasses = 1;
    }

    m_pCounters = pCounters;

    return m_numPasses;
}


//--------------------------------------------------------------------------------------
// Name: EndOfPass
// Desc: Should be called at the end of each pass to correctly collect counters information
//--------------------------------------------------------------------------------------
void DerivedCounters::EndOfPass( UINT uIndex )
{
    XSF_ASSERT( m_pCounters != NULL );

    if( !m_hw.empty() )
    {
        XSF_ERROR_IF_FAILED( m_pCounters->EndOfPass( uIndex, m_output ) );
    }

    if( m_numPasses == uIndex + 1 )
    {
        m_pCounters = NULL;
        m_numPasses = 0;

        if( !m_hw.empty() )
        {
            ComputeDerivedCounters( m_results, m_derived, &m_hw[ 0 ], m_output, static_cast< UINT >( m_hw.size() ) );
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: StartNextPass
// Desc: Should be called at to start the next pass
//--------------------------------------------------------------------------------------
void DerivedCounters::StartNextPass( UINT uIndex )
{
    if( !m_pCounters )
        return;

    XSF_ERROR_IF_FAILED( m_pCounters->StartNextPass( uIndex, m_output ) );
}


//--------------------------------------------------------------------------------------
// Name: GetHWCounterValue
// Desc: Returns the value of a given hw counter
//--------------------------------------------------------------------------------------
UINT64  DerivedCounters::GetHWCounterValue( const GPUCounters::Counter& c ) const
{
    return GetCounterValue< UINT64 >( c, &m_hw[ 0 ], static_cast< UINT >( m_hw.size() ), m_output, 0 );
}


#endif	// _XBOX_ONE && _TITLE


//--------------------------------------------------------------------------------------
// Name: PrintHWCounterValues
// Desc: Displays the values of hardware counters
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void    PrintHWCounterValues( const GPUCounters::Counter* pCounters, const std::vector< GPUCounters::CounterOutput >& data, UINT numCounters )
{
    if( data.empty() )
        return;

    for( UINT i=0; i < numCounters; ++i )
    {
        XSF::DebugPrint( L"%s ", pCounters[ i ].pName );

        const UINT numBlocks = pCounters[ i ].numberOfElementsInData / pCounters[ i ].numberInnerElementsInData;
        XSF_ASSERT( numBlocks == data[ i ].m_dataAfter.size() );
        XSF_ASSERT( numBlocks == data[ i ].m_dataBefore.size() );

        for( UINT j=0; j < numBlocks; ++j )
        {
            XSF::DebugPrint( " %I64d(%I64d)", data[ i ].m_dataAfter[ j ], data[ i ].m_dataBefore[ j ] );
        }

        XSF::DebugPrint( "\n" );
    }
}


//--------------------------------------------------------------------------------------
// Name: PrintDerivedCounterValues
// Desc: Displays the values of hardware counters
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void    PrintDerivedCounterValues( const DOUBLE* pResults, const DerivedCounter* pCounters, UINT numCounters )
{
    for( UINT i=0; i < numCounters; ++i )
    {
        XSF::DebugPrint( L"%20s %.2f\n", GetDerivedCounterName( pCounters[ i ] ), pResults[ pCounters[ i ] ] );
    }
}

