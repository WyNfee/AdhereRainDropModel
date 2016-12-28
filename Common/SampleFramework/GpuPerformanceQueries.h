//--------------------------------------------------------------------------------------
// GpuPerformanceQueries.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_GPU_PERFORMANCE_QUERIES
#define XSF_GPU_PERFORMANCE_QUERIES



#ifndef XSF_H_INCLUDED
#error  please include SampleFramework.h before this file
#endif

namespace XboxSampleFramework
{
    //--------------------------------------------------------------------------------------
    // Name: GpuPerformanceQueries
    // Desc: Performance measurement on the GPU
    //--------------------------------------------------------------------------------------
    class GpuPerformanceQueries
    {
    public:
        typedef UINT    Id;

        GpuPerformanceQueries();
        ~GpuPerformanceQueries();

        HRESULT Create( _In_ D3DDevice* const pDevice );
        VOID    OnNewFrame( _In_ D3DDeviceContext* const pImmediateContext, FLOAT& fFrameTime, D3D11_QUERY_DATA_PIPELINE_STATISTICS& pipelineStatistics );

        VOID    SetVerboseFlag( BOOL bPrintOutMessagesWhenBlocking );

        // Timing queries return a GPU tick count. They don't have a Begin and End calls
        // so to get a time delta you need two timing queries

        HRESULT CreateTimingQuery( Id& newQueryIndex );
        VOID    InsertTimingQuery( _In_ D3DDeviceContext* const pCtx, Id id );

        UINT64    GetTimingQueryValue( Id id ) const;
        UINT64    GetTimingQueryFrequency() const;
        DOUBLE    GetInvTimingQueryFrequency() const;

        FLOAT    GetTimingQueryDelta( Id end, Id start ) const;

        void     BlockUntilQueriesAreReady( _In_ D3DDeviceContext* const pCtx );

        // Pipeline statistics queries return D3D pipe statistics. They have Begin and End
        // so the result is the e.g. number of primitives rendered between Begin and End

        HRESULT CreateStatisticsQuery( Id& newQueryIndex );
        VOID    BeginStatisticsQuery( _In_ D3DDeviceContext* const pCtx, Id id );
        VOID    EndStatisticsQuery( _In_ D3DDeviceContext* const pCtx, Id id );
        VOID    GetStatisticsQueryValue( Id id, D3D11_QUERY_DATA_PIPELINE_STATISTICS& value ) const;

        // All queries are destroyed in the same way

        VOID    DestroyQuery( Id id );
    private:
        static const UINT   ALLOW_NUM_FRAMES = 4;

        D3DDevice*      m_pDevice;

        ID3D11Query*    m_pQueryDisjoint[ ALLOW_NUM_FRAMES ];

        UINT64          m_uFrequency;
        DOUBLE          m_fFrequencyInverse;

        UINT            m_uCurrentFrameIndex;
        UINT            m_uCurrentQueryIndex;

        BOOL            m_bPrintOutMessagesWhenBlocking;

        Id              m_entireFrameTiming[ 2 ];
        Id              m_entireFrameStatistics;

        struct Query
        {
            ID3D11Query*    m_pQuery[ ALLOW_NUM_FRAMES ];
            union
            {
                UINT64            m_uValue;
                D3D11_QUERY_DATA_PIPELINE_STATISTICS    m_queryPipeStatisticsValue;
            };
            UINT            m_uValueRecordedAtFrame;
            UINT            m_bCreated : 1;
			UINT            m_bEnded : 1;
			UINT            m_bBegan : 1;       // For debug only, not used for timing queries
            D3D11_QUERY     m_type;
#if defined(_XBOX_ONE) && defined(_TITLE)
            UINT64          m_fence[ ALLOW_NUM_FRAMES ];    // Ensure the fence has past before polling for data
#endif
        };

        std::vector< Query >    m_queries;

        HRESULT AllocateQuery( UINT& idx, const D3D11_QUERY_DESC& desc );
        VOID    BlockUntilQueriesReadyForFrame( _In_ D3DDeviceContext* const pCtx, UINT uFrame );
    };

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryFrequency
    // Desc: Returns GPU tick frequency
    //--------------------------------------------------------------------------------------
    inline
    UINT64    GpuPerformanceQueries::GetTimingQueryFrequency() const
    {
        return m_uFrequency;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetInvTimingQueryFrequency
    // Desc: Returns 1/GPU tick frequency
    //--------------------------------------------------------------------------------------
    inline
    DOUBLE    GpuPerformanceQueries::GetInvTimingQueryFrequency() const
    {
        return m_fFrequencyInverse;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryDelta
    // Desc: Returns time difference between two timing queries in seconds
    //--------------------------------------------------------------------------------------
    inline
    FLOAT    GpuPerformanceQueries::GetTimingQueryDelta( Id end, Id start ) const
    {
        const UINT64    endTime = GetTimingQueryValue( end );
        const UINT64    startTime = GetTimingQueryValue( start );

        return (FLOAT)((DOUBLE)(endTime - startTime) * GetInvTimingQueryFrequency());
    }

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryValue
    // Desc: Return the value of the timing query in GPU ticks
    //--------------------------------------------------------------------------------------
    inline
    UINT64    GpuPerformanceQueries::GetTimingQueryValue( Id id ) const
    {
        XSF_ASSERT( m_pDevice );

        return m_queries[ id ].m_uValue;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetStatisticsQueryValue
    // Desc: Returns the value of the statistics query
    //--------------------------------------------------------------------------------------
    inline
    VOID    GpuPerformanceQueries::GetStatisticsQueryValue( Id id, D3D11_QUERY_DATA_PIPELINE_STATISTICS& value ) const
    {
        XSF_ASSERT( m_pDevice );

        value = m_queries[ id ].m_queryPipeStatisticsValue;
    }

    //--------------------------------------------------------------------------------------
    // Name: SetVerboseFlag
    // Desc: If enabled, this class will start showing a message whenever it blocks
    //--------------------------------------------------------------------------------------
    inline
    VOID    GpuPerformanceQueries::SetVerboseFlag( BOOL bPrintOutMessagesWhenBlocking )
    {
        m_bPrintOutMessagesWhenBlocking = bPrintOutMessagesWhenBlocking;
    }
};


#endif
