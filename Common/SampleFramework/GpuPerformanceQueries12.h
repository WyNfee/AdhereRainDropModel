//--------------------------------------------------------------------------------------
// GpuPerformanceQueries.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_GPU_PERFORMANCE_QUERIES12
#define XSF_GPU_PERFORMANCE_QUERIES12



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
        typedef UINT Id;

        GpuPerformanceQueries();
        ~GpuPerformanceQueries();

        HRESULT Create(_In_ D3DDevice* const pDevice, _In_ D3DCommandQueue* const pCmdQueue, _In_ ID3D12Fence* const pFence);
        void OnNewFrame(_In_ D3DCommandList* const pCmdList, FLOAT& fFrameTime, D3D12_QUERY_DATA_PIPELINE_STATISTICS& pipelineStatistics);
        void OnEndFrame(_In_ D3DCommandList* const pCmdList);

        void SetVerboseFlag(BOOL bPrintOutMessagesWhenBlocking);

        // Timing queries return a GPU tick count. They don't have a Begin and End calls
        // so to get a time delta you need two timing queries

        HRESULT CreateTimingQuery(Id& newQueryIndex);
        void InsertTimingQuery(_In_ D3DCommandList* const pCmdList, Id id);

        UINT64 GetTimingQueryValue(Id id) const;
        UINT64 GetTimingQueryFrequency() const;
        DOUBLE GetInvTimingQueryFrequency() const;

        FLOAT GetTimingQueryDelta(Id end, Id start) const;

        void BlockUntilQueriesAreReady();

        // Pipeline statistics queries return D3D pipe statistics. They have Begin and End
        // so the result is the e.g. number of primitives rendered between Begin and End

        HRESULT CreateStatisticsQuery(Id& newQueryIndex);
        void BeginStatisticsQuery(_In_ D3DCommandList* const pCmdList, Id id);
        void EndStatisticsQuery(_In_ D3DCommandList* const pCmdList, Id id);
        void GetStatisticsQueryValue(Id id, D3D12_QUERY_DATA_PIPELINE_STATISTICS& value) const;

        // All queries are destroyed in the same way

        void DestroyQuery(Id id);
    private:
        static const UINT ALLOW_NUM_FRAMES = 4;

        D3DDevice*      m_pDevice;
        ID3D12Fence*    m_pFence;

        UINT64          m_uFrequency;
        DOUBLE          m_fFrequencyInverse;

        UINT            m_uCurrentFrameIndex;
        UINT            m_uCurrentQueryIndex;

        BOOL            m_bPrintOutMessagesWhenBlocking;

        Id              m_entireFrameTiming[2];
        Id              m_entireFrameStatistics;

        struct Query
        {
            XSF::D3DQueryHeapPtr m_spQueryHeap;
            XSF::D3DResourcePtr  m_spQueryBuffer;
            union
            {
                UINT64       m_uValue;
                D3D12_QUERY_DATA_PIPELINE_STATISTICS    m_queryPipeStatisticsValue;
            };
            UINT             m_uValueRecordedAtFrame;
            UINT             m_bCreated : 1;
            UINT             m_bEnded : 1;
            UINT             m_bBegan : 1;       // For debug only, not used for timing queries
            D3D12_QUERY_TYPE m_type;
            UINT64           m_fence[ALLOW_NUM_FRAMES];  // Ensure the fence has past before polling for data

            Query()
                : m_uValue(0)
                , m_uValueRecordedAtFrame(0)
                , m_bCreated(FALSE)
                , m_bEnded(FALSE)
                , m_bBegan(FALSE)
                , m_type(D3D12_QUERY_TYPE_TIMESTAMP)
            {
                ZeroMemory(&m_queryPipeStatisticsValue, sizeof(m_queryPipeStatisticsValue));
                ZeroMemory(m_fence, sizeof(m_fence));
            }
        };

        std::vector<Query>  m_queries;

        HRESULT AllocateQuery(UINT& idx, const D3D12_QUERY_TYPE& qType);
        void BlockUntilQueriesReadyForFrame(UINT uFrame);
    };

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryFrequency
    // Desc: Returns GPU tick frequency
    //--------------------------------------------------------------------------------------
    inline
    UINT64 GpuPerformanceQueries::GetTimingQueryFrequency() const
    {
        return m_uFrequency;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetInvTimingQueryFrequency
    // Desc: Returns 1/GPU tick frequency
    //--------------------------------------------------------------------------------------
    inline
    DOUBLE GpuPerformanceQueries::GetInvTimingQueryFrequency() const
    {
        return m_fFrequencyInverse;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryDelta
    // Desc: Returns time difference between two timing queries in seconds
    //--------------------------------------------------------------------------------------
    inline
    FLOAT GpuPerformanceQueries::GetTimingQueryDelta(Id end, Id start) const
    {
        const UINT64 endTime = GetTimingQueryValue(end);
        const UINT64 startTime = GetTimingQueryValue(start);

        //XSF_ASSERT(endTime >= startTime);

        return (FLOAT)((DOUBLE)(endTime - startTime) * GetInvTimingQueryFrequency());
    }

    //--------------------------------------------------------------------------------------
    // Name: GetTimingQueryValue
    // Desc: Return the value of the timing query in GPU ticks
    //--------------------------------------------------------------------------------------
    inline
    UINT64 GpuPerformanceQueries::GetTimingQueryValue(Id id) const
    {
        return m_queries[id].m_uValue;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetStatisticsQueryValue
    // Desc: Returns the value of the statistics query
    //--------------------------------------------------------------------------------------
    inline
    void GpuPerformanceQueries::GetStatisticsQueryValue(Id id, D3D12_QUERY_DATA_PIPELINE_STATISTICS& value) const
    {
        value = m_queries[id].m_queryPipeStatisticsValue;
    }

    //--------------------------------------------------------------------------------------
    // Name: SetVerboseFlag
    // Desc: If enabled, this class will start showing a message whenever it blocks
    //--------------------------------------------------------------------------------------
    inline
    void GpuPerformanceQueries::SetVerboseFlag(BOOL bPrintOutMessagesWhenBlocking)
    {
        m_bPrintOutMessagesWhenBlocking = bPrintOutMessagesWhenBlocking;
    }
};


#endif
