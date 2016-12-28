//--------------------------------------------------------------------------------------
// GpuPerformanceQueries12.cpp
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
GpuPerformanceQueries::GpuPerformanceQueries() :    m_uCurrentFrameIndex(0),
                                                    m_uCurrentQueryIndex(0),
                                                    m_uFrequency(0),
                                                    m_fFrequencyInverse(0),
                                                    m_pDevice(nullptr),
                                                    m_pFence(nullptr),
                                                    m_bPrintOutMessagesWhenBlocking(FALSE)
{
    m_entireFrameTiming[0] = m_entireFrameTiming[1] = m_entireFrameStatistics = static_cast<UINT>(-1);
}

//--------------------------------------------------------------------------------------
// Name: ~GpuPerformanceQueries
// Desc: Destructor for GpuPerformanceQueries
//--------------------------------------------------------------------------------------
GpuPerformanceQueries::~GpuPerformanceQueries()
{
    DestroyQuery(m_entireFrameTiming[0]);
    DestroyQuery(m_entireFrameTiming[1]);

    DestroyQuery(m_entireFrameStatistics);

    // free user queries
    const UINT uNumQueries = static_cast<UINT>(m_queries.size());
    for (UINT i=0; i < uNumQueries; ++i)
    {
        if (m_queries[i].m_bCreated)
        {
            DebugPrint("Query %d hasn't been freed\n", i);
            DestroyQuery(i);
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: Create
// Desc: Create disjoint queries and a set of per-frame queries
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT GpuPerformanceQueries::Create(D3DDevice* const pDevice, D3DCommandQueue* const pCmdQueue, ID3D12Fence* const pFence)
{
    XSF_ASSERT(pDevice);
    XSF_ASSERT(pFence);

    if (!pDevice || !pFence)
        return E_INVALIDARG;

    m_pDevice = pDevice;
    m_pFence = pFence;
    
    XSF_RETURN_IF_FAILED(CreateTimingQuery(m_entireFrameTiming[0]));
    XSF_RETURN_IF_FAILED(CreateTimingQuery(m_entireFrameTiming[1]));
    XSF_RETURN_IF_FAILED(CreateStatisticsQuery(m_entireFrameStatistics));

    // Present the GPU from going into idle states
#if defined(_DEBUG) || defined(_DEBUG)
    XSF_RETURN_IF_FAILED(m_pDevice->SetStablePowerState(TRUE));
#endif

    // Get GPU clock frequency
    XSF_RETURN_IF_FAILED(pCmdQueue->GetTimestampFrequency(&m_uFrequency));
    m_fFrequencyInverse = 1.0 / static_cast<DOUBLE>(m_uFrequency);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: BlockUntilQueriesAreReady
// Desc: Ensures that queries are ready in the same frame
//--------------------------------------------------------------------------------------
void GpuPerformanceQueries::BlockUntilQueriesAreReady()
{
    BlockUntilQueriesReadyForFrame(m_uCurrentQueryIndex);
}


//--------------------------------------------------------------------------------------
// Name: BlockUntilQueriesReadyForFrame
// Desc: Ensures that queries are ready in the same frame
//--------------------------------------------------------------------------------------
void GpuPerformanceQueries::BlockUntilQueriesReadyForFrame(UINT uFrame)
{
    const UINT uNumQueries = static_cast<UINT>(m_queries.size());
    for (UINT i = 0; i < uNumQueries; ++i)
    {
        // Ignore empty slots
        if (!m_queries[i].m_bCreated)
            continue;

        // Process only if the user called InsertTimingQuery or EndxxxxQuery this frame
        if (!m_queries[i].m_bEnded)
            continue;

        // If the fence is zero, the query hasn't passed
        if (m_queries[i].m_fence[uFrame] == 0)
        {
            m_queries[i].m_bEnded = FALSE;
            continue;
        }

        // We are about to use the counter for the next frame, make sure we read its data
        // this should only block if the gpu accumulated more than ALLOW_NUM_FRAMES frames
        while (m_pFence->GetCompletedValue() < m_queries[i].m_fence[uFrame])
        {
            if (m_bPrintOutMessagesWhenBlocking)
            {
                DebugPrint("GpuPerformanceQueries::OnNewFrame: blocking while trying to get a query result\n");
            }

            SwitchToThread();
        }

        // Retrieve the data
        void* pData;
        const D3D12_RANGE mapRange = CD3DX12_RANGE(uFrame * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, uFrame * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT + sizeof(D3D12_QUERY_TYPE_PIPELINE_STATISTICS));
        XSF_ERROR_IF_FAILED(m_queries[i].m_spQueryBuffer->Map(0, &mapRange, &pData));
        const BYTE *pOffsetData = reinterpret_cast<BYTE*>(pData) + uFrame * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        switch(m_queries[i].m_type)
        {
        case D3D12_QUERY_TYPE_TIMESTAMP:
            m_queries[i].m_uValue = *reinterpret_cast<const UINT64*>(pOffsetData);
            break;

        case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
            m_queries[i].m_queryPipeStatisticsValue = *reinterpret_cast<const D3D12_QUERY_DATA_PIPELINE_STATISTICS*>(pOffsetData);
            break;
        };
        m_queries[i].m_spQueryBuffer->Unmap(0, nullptr);

        m_queries[i].m_bEnded = FALSE;
        m_queries[i].m_uValueRecordedAtFrame = m_uCurrentFrameIndex;
    }
}

//--------------------------------------------------------------------------------------
// Name: OnNewFrame
// Desc: This performs the main queries logic update
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void GpuPerformanceQueries::OnNewFrame(D3DCommandList* const pCmdList, FLOAT& fFrameTime, D3D12_QUERY_DATA_PIPELINE_STATISTICS& stats)
{
    XSF_ASSERT(pCmdList);

    const UINT uCurrentQueryIndex = m_uCurrentQueryIndex;
    const UINT uNextQueryIndex = (uCurrentQueryIndex + 1) % ALLOW_NUM_FRAMES;

    // finish "entire frame" query
    InsertTimingQuery(pCmdList, m_entireFrameTiming[1]);

    // insert the beginning of the statistics query
    BeginStatisticsQuery(pCmdList, m_entireFrameStatistics);

    // process all live queries
    BlockUntilQueriesReadyForFrame(uNextQueryIndex);

    // Frame time on the gpu
    fFrameTime = GetTimingQueryDelta(m_entireFrameTiming[1], m_entireFrameTiming[0]);

    // statistics query
    GetStatisticsQueryValue(m_entireFrameStatistics, stats);

    // Increment frame index before adding the counter for the start of the next frame
    m_uCurrentQueryIndex = (m_uCurrentFrameIndex + 1) % ALLOW_NUM_FRAMES;

    // Start the next frame's query
    InsertTimingQuery(pCmdList, m_entireFrameTiming[0]);
}

//--------------------------------------------------------------------------------------
// Name: OnEndFrame
// Desc: This performs the main queries logic update
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void GpuPerformanceQueries::OnEndFrame(D3DCommandList* const pCmdList)
{
    XSF_ASSERT(pCmdList);

    m_uCurrentQueryIndex = m_uCurrentFrameIndex % ALLOW_NUM_FRAMES;

    // End statistics query
    EndStatisticsQuery(pCmdList, m_entireFrameStatistics);

    // Increment frame index before adding the counter for the start of the next frame
    ++m_uCurrentFrameIndex;
    m_uCurrentQueryIndex = m_uCurrentFrameIndex % ALLOW_NUM_FRAMES;
}

//--------------------------------------------------------------------------------------
// Name: AllocateQuery
// Desc: Finds a free slot and creates a new timing query
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::AllocateQuery(UINT& idx, const D3D12_QUERY_TYPE& qType)
{
    // TODO: if this search is slow, a linked list of free items should work well
    const UINT uNumQueries = static_cast<UINT>(m_queries.size());
    for (idx=0; idx < uNumQueries; ++idx)
    {
        if (!m_queries[idx].m_bCreated)
            break;
    }

    if (idx == uNumQueries)
    {
        m_queries.push_back(Query());
        idx = uNumQueries;
    }

    Query& q = m_queries[idx];

    q.m_bCreated = TRUE;
    q.m_bEnded = FALSE;
    q.m_bBegan = FALSE;
    q.m_uValue = 0;
    q.m_uValueRecordedAtFrame = 0;
    q.m_type = qType;
    ZeroMemory(q.m_fence, sizeof(q.m_fence));

    // create the query heap
    D3D12_QUERY_HEAP_DESC descHeap = {};
    descHeap.Count = ALLOW_NUM_FRAMES;
    switch(q.m_type)
    {
    case D3D12_QUERY_TYPE_OCCLUSION:
    case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
        descHeap.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        break;
    case D3D12_QUERY_TYPE_TIMESTAMP:
        descHeap.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        break;
    case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
        descHeap.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        break;
    default:
        descHeap.Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
        break;
    }
    XSF_RETURN_IF_FAILED(m_pDevice->CreateQueryHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(q.m_spQueryHeap.ReleaseAndGetAddressOf())));

    // create a buffer to hold query data
    const D3D12_HEAP_PROPERTIES readBackHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ALLOW_NUM_FRAMES * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    XSF_ERROR_IF_FAILED(m_pDevice->CreateCommittedResource(
        &readBackHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(q.m_spQueryBuffer.ReleaseAndGetAddressOf())));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CreateTimingQuery
// Desc: Finds a free slot and creates a new timing query. Each query is backed up by
// ALLOW_NUM_FRAMES D3D queries to hide the latency
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::CreateTimingQuery(GpuPerformanceQueries::Id& newQueryIndex)
{
    UINT idx;
    XSF_RETURN_IF_FAILED(AllocateQuery(idx, D3D12_QUERY_TYPE_TIMESTAMP));

    newQueryIndex = idx;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: DestroyQuery
// Desc: Releases query's resources. It's alright to delete already deleted queries.
//--------------------------------------------------------------------------------------
void GpuPerformanceQueries::DestroyQuery(Id id)
{    
    if (id != static_cast<UINT>(-1))
    {
        m_queries[id].m_bCreated = FALSE;
        m_queries[id].m_spQueryHeap.Reset();
        m_queries[id].m_spQueryBuffer.Reset();
    }
}

//--------------------------------------------------------------------------------------
// Name: InsertTimingQuery
// Desc: Queues up a D3D query object for the current frame
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void GpuPerformanceQueries::InsertTimingQuery(D3DCommandList* const pCmdList, Id id)
{
    XSF_ASSERT(pCmdList);
    XSF_ASSERT(m_queries[id].m_spQueryHeap.Get());
    XSF_ASSERT(m_uCurrentQueryIndex < ALLOW_NUM_FRAMES);
    XSF_ASSERT(m_queries[id].m_type == D3D12_QUERY_TYPE_TIMESTAMP);

    // The data is drained in OnNewFrame. In this function we just discard any data if it's still pending.
    // This is to avoid a D3D warning
    while(m_pFence->GetCompletedValue() < m_queries[id].m_fence[m_uCurrentQueryIndex])
    {
        SwitchToThread();
    }

    // End the Query
    pCmdList->EndQuery(m_queries[id].m_spQueryHeap, m_queries[id].m_type, m_uCurrentQueryIndex);
    m_queries[id].m_fence[m_uCurrentQueryIndex] = m_pFence->GetCompletedValue() + 1;

    // insert a ResolveQueryData command in the command list
    pCmdList->ResolveQueryData(
            m_queries[id].m_spQueryHeap,
            m_queries[id].m_type,
            m_uCurrentQueryIndex,
            1,
            m_queries[id].m_spQueryBuffer,
            m_uCurrentQueryIndex * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    m_queries[id].m_bEnded = TRUE;
}

//--------------------------------------------------------------------------------------
// Name: CreateStatisticsQuery
// Desc: Finds a free slot and creates a new statistics query. Each query is backed up by
// ALLOW_NUM_FRAMES D3D queries to hide the latency
//--------------------------------------------------------------------------------------
HRESULT GpuPerformanceQueries::CreateStatisticsQuery(Id& newQueryIndex)
{
    UINT idx;
    XSF_RETURN_IF_FAILED(AllocateQuery(idx, D3D12_QUERY_TYPE_PIPELINE_STATISTICS));

    newQueryIndex = idx;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: BeginStatisticsQuery
// Desc: Starts a statistics query
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void GpuPerformanceQueries::BeginStatisticsQuery(D3DCommandList* const pCmdList, Id id)
{
    XSF_ASSERT(pCmdList);
    XSF_ASSERT(m_queries[id].m_spQueryHeap.Get());
    XSF_ASSERT(m_uCurrentQueryIndex < ALLOW_NUM_FRAMES);
    XSF_ASSERT(m_queries[id].m_type == D3D12_QUERY_TYPE_PIPELINE_STATISTICS);
    XSF_ASSERT(!m_queries[id].m_bBegan);

    pCmdList->BeginQuery(m_queries[id].m_spQueryHeap, m_queries[ id ].m_type, m_uCurrentQueryIndex);

    m_queries[id].m_bBegan = TRUE;
}

//--------------------------------------------------------------------------------------
// Name: EndStatisticsQuery
// Desc: Ends a statistics query
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void GpuPerformanceQueries::EndStatisticsQuery(D3DCommandList* const pCmdList, Id id)
{
    XSF_ASSERT(pCmdList);
    XSF_ASSERT(m_queries[id].m_spQueryHeap.Get());
    XSF_ASSERT(m_uCurrentQueryIndex < ALLOW_NUM_FRAMES);
    XSF_ASSERT(m_queries[id].m_type == D3D12_QUERY_TYPE_PIPELINE_STATISTICS);
    XSF_ASSERT(m_queries[id].m_bBegan);
    XSF_ASSERT(!m_queries[id].m_bEnded);

    pCmdList->EndQuery(m_queries[id].m_spQueryHeap, m_queries[id].m_type, m_uCurrentQueryIndex);
    m_queries[id].m_fence[m_uCurrentQueryIndex] = m_pFence->GetCompletedValue() + 1;

    // insert a ResolveQueryData command in the command list
    pCmdList->ResolveQueryData(
            m_queries[id].m_spQueryHeap,
            m_queries[id].m_type,
            m_uCurrentQueryIndex,
            1,
            m_queries[id].m_spQueryBuffer,
            m_uCurrentQueryIndex * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    m_queries[id].m_bBegan = FALSE;
    m_queries[id].m_bEnded = TRUE;
}
