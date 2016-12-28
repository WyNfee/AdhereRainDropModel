//--------------------------------------------------------------------------------------
// GpuPerformanceQueries.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include <pch.h>


using namespace XboxSampleFramework;


//--------------------------------------------------------------------------------------
// Name: GpuPerformanceQueries
// Desc: Constructor for GpuPerformanceQueries. Zeroes out pointers.
//--------------------------------------------------------------------------------------
GpuPerformanceQueries::GpuPerformanceQueries() :    m_uCurrentFrameIndex( 0 ),
                                                    m_uCurrentQueryIndex( 0 ),
                                                    m_uFrequency( 0 ),
                                                    m_fFrequencyInverse( 0 ),
                                                    m_pDevice( nullptr ),
                                                    m_bPrintOutMessagesWhenBlocking( FALSE )
{
    m_entireFrameTiming[0] = m_entireFrameTiming[1] = m_entireFrameStatistics = static_cast<UINT>(-1);

    for( UINT i=0; i < _countof( m_pQueryDisjoint ); ++i )
        m_pQueryDisjoint[ i ] = nullptr;
}



//--------------------------------------------------------------------------------------
// Name: ~GpuPerformanceQueries
// Desc: Destructor for GpuPerformanceQueries
//--------------------------------------------------------------------------------------
GpuPerformanceQueries::~GpuPerformanceQueries()
{
    DestroyQuery( m_entireFrameTiming[ 0 ] );
    DestroyQuery( m_entireFrameTiming[ 1 ] );

    DestroyQuery( m_entireFrameStatistics );

    // free user queries
    const UINT uNumQueries = static_cast< UINT >( m_queries.size() );
    for( UINT i=0; i < uNumQueries; ++i )
    {
        if( m_queries[ i ].m_bCreated )
        {
            DebugPrint( "Query %d hasn't been freed\n", i );
            DestroyQuery( i );
        }
    }

    // free disjoint query
    for( UINT i=0; i < _countof( m_pQueryDisjoint ); ++i )
    {
        XSF_SAFE_RELEASE( m_pQueryDisjoint[ i ] );
    }
}

//--------------------------------------------------------------------------------------
// Name: Create
// Desc: Create disjoint queries and a set of per-frame queries
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT GpuPerformanceQueries::Create( D3DDevice* const pDevice )
{
    XSF_ASSERT( pDevice );

    if( !pDevice )
        return E_INVALIDARG;

    D3D11_QUERY_DESC queryDesc;
    queryDesc.MiscFlags = 0;
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    
    for( UINT i=0; i < _countof( m_pQueryDisjoint ); ++i )
    {
        HRESULT hr = pDevice->CreateQuery( &queryDesc, &m_pQueryDisjoint[ i ] );
        if( FAILED( hr ) )
        {
            DebugPrint( "GpuPerformanceQueries::Create: failed to create a query %x\n", hr );
            return hr;
        }
    }

    m_pDevice = pDevice;
    
    XSF_RETURN_IF_FAILED( CreateTimingQuery( m_entireFrameTiming[ 0 ] ) );
    XSF_RETURN_IF_FAILED( CreateTimingQuery( m_entireFrameTiming[ 1 ] ) );
    XSF_RETURN_IF_FAILED( CreateStatisticsQuery( m_entireFrameStatistics ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: BlockUntilQueriesAreReady
// Desc: Ensures that queries are ready in the same frame
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::BlockUntilQueriesAreReady( D3DDeviceContext* const pCtx )
{
    BlockUntilQueriesReadyForFrame( pCtx, m_uCurrentQueryIndex );
}


//--------------------------------------------------------------------------------------
// Name: BlockUntilQueriesReadyForFrame
// Desc: Ensures that queries are ready in the same frame
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::BlockUntilQueriesReadyForFrame( D3DDeviceContext* const pCtx, UINT uFrame )
{
    const UINT uNumQueries = static_cast< UINT >( m_queries.size() );
    for( UINT i=0; i < uNumQueries; ++i )
    {
        // Ignore empty slots
        if( !m_queries[ i ].m_bCreated )
            continue;

        // Process only if the user called InsertTimingQuery or EndxxxxQuery this frame
        if( !m_queries[ i ].m_bEnded )
            continue;

        // We are about to use the counter for the next frame, make sure we read its data
        // this should only block if the gpu accumulated more than ALLOW_NUM_FRAMES frames
#if defined(_XBOX_ONE) && defined(_TITLE)
        while( m_pDevice->IsFencePending( m_queries[ i ].m_fence[ uFrame ] ) )
#else
        while( S_FALSE == pCtx->GetData( m_queries[ i ].m_pQuery[ uFrame ], NULL, 0, 0 ) )
#endif
        {
            if( m_bPrintOutMessagesWhenBlocking )
            {
                DebugPrint( "GpuPerformanceQueries::OnNewFrame: blocking while trying to get a query result\n" );
            }

            Sleep( 1 );
        }

        // Retrieve the data
        switch( m_queries[ i ].m_type )
        {
        case D3D11_QUERY_TIMESTAMP:
            pCtx->GetData( m_queries[ i ].m_pQuery[ uFrame ], &m_queries[ i ].m_uValue, sizeof( m_queries[ i ].m_uValue ), 0 );
            break;

        case D3D11_QUERY_PIPELINE_STATISTICS:
            pCtx->GetData( m_queries[ i ].m_pQuery[ uFrame ], &m_queries[ i ].m_queryPipeStatisticsValue, sizeof( m_queries[ i ].m_queryPipeStatisticsValue ), 0 );
            break;
        };

        m_queries[ i ].m_bEnded = FALSE;
        m_queries[ i ].m_uValueRecordedAtFrame = m_uCurrentFrameIndex;
    }
}

//--------------------------------------------------------------------------------------
// Name: OnNewFrame
// Desc: This performs the main queries logic update
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::OnNewFrame( D3DDeviceContext* const pCtx, FLOAT& fFrameTime, D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats )
{
    XSF_ASSERT( pCtx );
    XSF_ASSERT( m_pDevice );
    XSF_ASSERT( m_pQueryDisjoint[ 0 ] );

    const UINT uCurrentQueryIndex = m_uCurrentQueryIndex;
    const UINT uNextQueryIndex = (uCurrentQueryIndex + 1) % _countof( m_pQueryDisjoint );

    // finish "entire frame" query
    InsertTimingQuery( pCtx, m_entireFrameTiming[ 1 ] );

    // finish the "disjoint" query for the current frame
    // the disjoint query tells us the sampling frequency and whether the frequency
    // changed between begin and end, in which case we can't use the results
    if( m_uCurrentFrameIndex != 0 )
    {
        pCtx->End( m_pQueryDisjoint[ uCurrentQueryIndex ] );
        EndStatisticsQuery( pCtx, m_entireFrameStatistics );
    }

    // the first 4 frames don't have any issued queries
    if( m_uCurrentFrameIndex >= _countof( m_pQueryDisjoint ) )
    {
        // try getting disjoint query data using the index for the next frame
        while( S_FALSE == pCtx->GetData( m_pQueryDisjoint[ uNextQueryIndex ], NULL, 0, 0 ) )
        {
            if( m_bPrintOutMessagesWhenBlocking )
            {
                DebugPrint( "GpuPerformanceQueries::OnNewFrame: blocking while trying to get a disjoint query result\n" );
            }
            Sleep( 1 );
        }

        // "disjoint" event tells us if something happened during its recording that will result in broken timing
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = { 1, TRUE };
        HRESULT hr;
        if( ( hr = pCtx->GetData( m_pQueryDisjoint[ uNextQueryIndex ], &disjoint, sizeof( disjoint ), 0 ) ) == S_OK )
        {
            if( disjoint.Disjoint )
            {
                // Inform about disjoint frame
                if( m_bPrintOutMessagesWhenBlocking )
                {
                    DebugPrint( "GpuPerformanceQueries::OnNewFrame: frame disjoint\n" );
                }
            }
            else
            {
                m_uFrequency = disjoint.Frequency;
                m_fFrequencyInverse = 1.0 / DOUBLE( disjoint.Frequency );
            }
        }
        else
        {
            XSF::DebugPrint( "Warning: XSF couldn't query timing frequency!\n" );
        }
    }

    // process all live queries
    BlockUntilQueriesReadyForFrame( pCtx, uNextQueryIndex );

    // Frame time on the gpu
    fFrameTime = GetTimingQueryDelta( m_entireFrameTiming[ 1 ], m_entireFrameTiming[ 0 ] );

    // End statistics query and get it's previous value
    GetStatisticsQueryValue( m_entireFrameStatistics, stats );

    // Increment frame index before adding the counter for the start of the next frame
    ++m_uCurrentFrameIndex;
    m_uCurrentQueryIndex = m_uCurrentFrameIndex % _countof( m_pQueryDisjoint );

    // Start the disjoint query for the next frame
    pCtx->Begin( m_pQueryDisjoint[ m_uCurrentQueryIndex ] );

    // Start the next frame's query
    InsertTimingQuery( pCtx, m_entireFrameTiming[ 0 ] );
    BeginStatisticsQuery( pCtx, m_entireFrameStatistics );
}

//--------------------------------------------------------------------------------------
// Name: AllocateQuery
// Desc: Finds a free slot and creates a new timing query
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::AllocateQuery( UINT& idx, const D3D11_QUERY_DESC& desc )
{
    // TODO: if this search is slow, a linked list of free items should work well
    const UINT uNumQueries = static_cast< UINT >( m_queries.size() );
    for( idx=0; idx < uNumQueries; ++idx )
    {
        if( !m_queries[ idx ].m_bCreated )
            break;
    }

    if( idx == uNumQueries )
    {
        m_queries.push_back( Query() );
        idx = uNumQueries;
    }

    Query& q = m_queries[ idx ];

    q.m_bCreated = TRUE;
    q.m_bEnded = FALSE;
    q.m_bBegan = FALSE;
    q.m_uValue = 0;
    q.m_uValueRecordedAtFrame = 0;
    q.m_type = desc.Query;
   
    for( UINT i=0; i < _countof( q.m_pQuery ); ++i )
    {
#if defined(_XBOX_ONE) && defined(_TITLE)
        q.m_fence[ i ] = 0;
#endif
        HRESULT hr = m_pDevice->CreateQuery( &desc, &q.m_pQuery[ i ] );
        if( FAILED( hr ) )
        {
            DebugPrint( "GpuPerformanceQueries::CreateTimingQuery: failed to create a query %x\n", hr );
            return hr;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CreateTimingQuery
// Desc: Finds a free slot and creates a new timing query. Each query is backed up by
// ALLOW_NUM_FRAMES D3D queries to hide the latency
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::CreateTimingQuery( GpuPerformanceQueries::Id& newQueryIndex )
{
    XSF_ASSERT( m_pDevice );

    D3D11_QUERY_DESC queryDesc;
    queryDesc.MiscFlags = 0;
    queryDesc.Query = D3D11_QUERY_TIMESTAMP;

    UINT idx;
    XSF_RETURN_IF_FAILED( AllocateQuery( idx, queryDesc ) );

    newQueryIndex = idx;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: DestroyQuery
// Desc: Releases query's resources. It's alright to delete already deleted queries.
//--------------------------------------------------------------------------------------
VOID    GpuPerformanceQueries::DestroyQuery( Id id )
{
    XSF_ASSERT( m_pDevice );
    
    if( m_pDevice != nullptr && id != static_cast<UINT>(-1) )
    {
        m_queries[ id ].m_bCreated = FALSE;

        for( UINT i=0; i < _countof( m_queries[ id ].m_pQuery ); ++i )
            XSF_SAFE_RELEASE( m_queries[ id ].m_pQuery[ i ] );
    }
}


//--------------------------------------------------------------------------------------
// Name: InsertTimingQuery
// Desc: Queues up a D3D query object for the current frame
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::InsertTimingQuery( D3DDeviceContext* const pCtx, Id id )
{
    XSF_ASSERT( m_pDevice );
    XSF_ASSERT( pCtx );
    XSF_ASSERT( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );
    XSF_ASSERT( m_queries[ id ].m_type == D3D11_QUERY_TIMESTAMP );

    // The data is drained in OnNewFrame. In this function we just discard any data if it's still pending.
    // This is to avoid a D3D warning
#if defined(_XBOX_ONE) && defined(_TITLE)
    while( m_pDevice->IsFencePending( m_queries[ id ].m_fence[ m_uCurrentQueryIndex ] ) )
    {
        SwitchToThread();
    }
#endif
    pCtx->GetData( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ], NULL, 0, 0 );

    // End the Query
    pCtx->End( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_queries[ id ].m_fence[ m_uCurrentQueryIndex ] = pCtx->InsertFence( 0 );
#endif

    m_queries[ id ].m_bEnded = TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CreateStatisticsQuery
// Desc: Finds a free slot and creates a new statistics query. Each query is backed up by
// ALLOW_NUM_FRAMES D3D queries to hide the latency
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::CreateStatisticsQuery( Id& newQueryIndex )
{
    XSF_ASSERT( m_pDevice );

    D3D11_QUERY_DESC queryDesc;
    queryDesc.MiscFlags = 0;
    queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;

    UINT idx;
    XSF_RETURN_IF_FAILED( AllocateQuery( idx, queryDesc ) );

    newQueryIndex = idx;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: BeginStatisticsQuery
// Desc: Starts a statistics query
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::BeginStatisticsQuery( D3DDeviceContext* const pCtx, Id id )
{
    XSF_ASSERT( m_pDevice );
    XSF_ASSERT( pCtx );
    XSF_ASSERT( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );
    XSF_ASSERT( m_queries[ id ].m_type == D3D11_QUERY_PIPELINE_STATISTICS );
    XSF_ASSERT( !m_queries[ id ].m_bBegan );

    pCtx->Begin( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );

    m_queries[ id ].m_bBegan = TRUE;
}


//--------------------------------------------------------------------------------------
// Name: EndStatisticsQuery
// Desc: Ends a statistics query
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID    GpuPerformanceQueries::EndStatisticsQuery( D3DDeviceContext* const pCtx, Id id )
{
    XSF_ASSERT( m_pDevice );
    XSF_ASSERT( pCtx );
    XSF_ASSERT( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );
    XSF_ASSERT( m_queries[ id ].m_type == D3D11_QUERY_PIPELINE_STATISTICS );
    XSF_ASSERT( m_queries[ id ].m_bBegan );
    XSF_ASSERT( !m_queries[ id ].m_bEnded );

    pCtx->End( m_queries[ id ].m_pQuery[ m_uCurrentQueryIndex ] );
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_queries[ id ].m_fence[ m_uCurrentQueryIndex ] = pCtx->InsertFence( 0 );
#endif

    m_queries[ id ].m_bBegan = FALSE;
    m_queries[ id ].m_bEnded = TRUE;
}
