//--------------------------------------------------------------------------------------
// GPUCounters.h
//
// Contains declarations for using Xbox One alpha GPU counters
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once


#if defined(_XBOX_ONE) && defined(_TITLE)

// Real frequencies
_declspec(selectany) extern const DOUBLE GPU_FREQUENCY = (DOUBLE) D3D11X_XBOX_GPU_CLOCK_FREQUENCY;
_declspec(selectany) extern const DOUBLE GPU_RAM_BANDWIDTH = 68 * 1024 * 1024 * 1024.0;
_declspec(selectany) extern const DOUBLE GPU_ESRAM_BANDWIDTH = 102.4 * 1024 * 1024 * 1024.0;

#else

// Just a guess
_declspec(selectany) extern const DOUBLE GPU_FREQUENCY = 851200000.0;
_declspec(selectany) extern const DOUBLE GPU_RAM_BANDWIDTH = 220.f * 1024 * 1024 * 1024.0;
_declspec(selectany) extern const DOUBLE GPU_ESRAM_BANDWIDTH = 220.f * 1024 * 1024 * 1024.0;

#endif

//--------------------------------------------------------------------------------------
// Name: GPUCounters
// Desc: Helps to query GPU hardware counters, usually only one instance of this class is needed
// This will only work with the alternate driver
//--------------------------------------------------------------------------------------
class GPUCounters
{
public:
    //--------------------------------------------------------------------------------------
    // Name: Counters
    // Desc: A single hardware counter
    //--------------------------------------------------------------------------------------
    struct Counter
    {
        Counter()
        {
        }

        Counter( const Counter& c )
        {
            *this = c;
        }

        Counter( BYTE ofsDesc, WORD ofsData, BYTE numElems, BYTE numInner, DWORD counterId, const WCHAR* block, const WCHAR* name ) :
                                                    indexOfFirstElementInDescription( ofsDesc ),
                                                    indexOfFirstElementInData( ofsData ),
                                                    numberOfElementsInData( numElems ),
                                                    numberInnerElementsInData( numInner ),
                                                    counterId( counterId ),
                                                    pBlock( block ),
                                                    pName( name )
        {
        }

        bool operator == ( const Counter& c ) const
        {
            return  indexOfFirstElementInDescription == c.indexOfFirstElementInDescription &&
                    numberOfElementsInData == c.numberOfElementsInData &&
                    counterId == c.counterId;
        }

        bool operator < ( const Counter& c ) const
        {
            return  (indexOfFirstElementInDescription + (counterId << 16)) <
                        (c.indexOfFirstElementInDescription + (c.counterId << 16));
        }

        const WCHAR*    pBlock;                                 // name of the block
        const WCHAR*    pName;                                  // name of the counter
        BYTE            indexOfFirstElementInDescription;       // index of the first UINT32 in D3D11X_COUNTER_SET_DESC
        BYTE            numberOfElementsInData;                 // number of elements in D3D11X_COUNTER_DATA, for instance 16 for CB[ 2 ][ 8 ]
        BYTE            numberInnerElementsInData;              // number of inner elements in D3D11X_COUNTER_DATA, for instance 2 for CB[ 2 ][ 8 ]
        BYTE            pad0;
        WORD            indexOfFirstElementInData;              // index of the first UINT64 in D3D11X_COUNTER_DATA
        DWORD           counterId;                              // the actual counter id, for instance CB_PERF_SEL...
    };

    //--------------------------------------------------------------------------------------
    // Name: CounterOutput
    // Desc: Output from a single hardware counter captured before the draw call and after
    // Some counters don't reset to 0 and require taking a diffference between after and before
    //--------------------------------------------------------------------------------------
    struct CounterOutput
    {
        std::vector< UINT64 >   m_dataBefore;
        std::vector< UINT64 >   m_dataAfter;
    };

    GPUCounters();
    ~GPUCounters();

    // Only to be used with one context
    HRESULT Initialize( _In_ XSF::D3DDevice* pDev, _In_ XSF::D3DDeviceContext* pCtx );

    // Some counter sets need multiple passes over the draw call
    HRESULT StartPasses( UINT& numPasses, _In_ const Counter* pCounters, UINT numCounters, std::vector< CounterOutput >& countersOutput );
    HRESULT EndOfPass( UINT passIndex, std::vector< CounterOutput >& countersOutput );
    HRESULT StartNextPass( UINT passIndex, std::vector< CounterOutput >& countersOutput );

    static DOUBLE GetGpuFrequency() { XSF_ASSERT( m_lfGpuFrequency != 0.0 ); return m_lfGpuFrequency; }
    static DOUBLE GetGpuBandwidth() { XSF_ASSERT( m_lfGpuBandwidth != 0.0 ); return m_lfGpuBandwidth; }

#if defined(_XBOX_ONE) && defined(_TITLE)
    ID3DXboxPerformanceContext* GetD3DXboxPerformanceContext() { XSF_ASSERT( m_pPerfContext != nullptr ); return m_pPerfContext; }
#endif

private:

#if defined(_XBOX_ONE) && defined(_TITLE)
    ID3DXboxPerformanceDevice*   m_pPerfDevice;
    ID3DXboxPerformanceContext*  m_pPerfContext;


    //--------------------------------------------------------------------------------------
    // Name: CounterSet
    // Desc: Can contain multiple hardware counters that can be read in one pass of rendering
    //--------------------------------------------------------------------------------------
    struct CounterSet
    {
        CounterSet() : m_pSet( NULL )
        {
            m_pSample[ 0 ] = m_pSample[ 1 ] = NULL;
            ZeroMemory( &m_desc, sizeof( m_desc ) );
        }

        ~CounterSet()
        {
            XSF_SAFE_RELEASE( m_pSet );
            XSF_SAFE_RELEASE( m_pSample[ 0 ] );
            XSF_SAFE_RELEASE( m_pSample[ 1 ] );
        }

        D3D11X_COUNTER_SET_DESC m_desc;
        ID3DXboxCounterSet*     m_pSet;
        ID3DXboxCounterSample*  m_pSample[ 2 ];
        std::vector< UINT >     m_counters;
    };

    UINT                        m_currentPass;
    std::vector< CounterSet* >  m_counterSetPasses;
    std::vector< Counter >      m_counters;
#endif

    static DOUBLE               m_lfGpuFrequency;
    static DOUBLE               m_lfGpuBandwidth;
};

__declspec( selectany ) DOUBLE GPUCounters::m_lfGpuFrequency = 0.0;
__declspec( selectany ) DOUBLE GPUCounters::m_lfGpuBandwidth = 0.0;


//--------------------------------------------------------------------------------------
// Name: DerivedCounter
// Desc: In addition to hardware counters, a set of derived counters is available.
// Derived counters use one or more hardware counters to produce their values
//--------------------------------------------------------------------------------------
enum DerivedCounter
{
    DC_GPUTime = 0,
    DC_TessellatorBusy,
    DC_VSBusy,
    DC_HSBusy,
    DC_DSBusy,
    DC_GSBusy,
    DC_PSBusy,
    DC_CSBusy,
    DC_VSVerticesIn,
    DC_HSPatches,
    DC_DSVerticesIn,
    DC_GSPrimsIn,
    DC_GSVerticesOut,
    DC_PrimitiveAssemblyBusy,
    DC_PrimitivesIn,
    DC_CulledPrims,
    DC_ClippedPrims,
    DC_PAStalledOnRasterizer,
    DC_PSPixelsOut,
    DC_VSExportStalls,
    DC_PSExportStalls,
    DC_CSThreadGroups,
    DC_CSWavefronts,
    DC_CSThreads,
    DC_CSVALUInsts,
    DC_CSVALUUtilization,
    DC_CSSALUInsts,
    DC_CSVFetchInsts,
    DC_CSSFetchInsts,
    DC_CSVWriteInsts,
    DC_CSVALUBusy,
    DC_CSSALUBusy,
    DC_CSMemUnitBusy,
    DC_CSMemUnitStalled,
    DC_CSFetchSize,
    DC_CSWriteSize,
    DC_CSCacheHit,
    DC_CSWriteUnitStalled,
    DC_CSGDSInsts,
    DC_CSLDSInsts,
    DC_CSALUStalledByLDS,
    DC_CSLDSBankConflict,
    DC_TexUnitBusy,
    DC_TexTriFilteringPct,
    DC_TexVolFilteringPct,
    DC_TexAveAnisotropy,
    DC_DepthStencilTestBusy,
    DC_HiZTilesAccepted,
    DC_PreZTilesDetailCulled,
    DC_HiZQuadsCulled,
    DC_PreZQuadsCulled,
    DC_PostZQuads,
    DC_PreZSamplesPassing,
    DC_PreZSamplesFailingS,
    DC_PreZSamplesFailingZ,
    DC_PostZSamplesPassing,
    DC_PostZSamplesFailingS,
    DC_PostZSamplesFailingZ,
    DC_ZUnitStalled,
    DC_CBMemRead,
    DC_CBMemWritten,
    DC_CBSlowPixelPct,
    DC_NUM_COUNTERS
};

#if defined(_XBOX_ONE) && defined(_TITLE)


//--------------------------------------------------------------------------------------
// Name: GPU_COUNTER
// Desc: Creates an instance of GPUCounters::Counter, for example GPU_COUNTER( VGT, vgt_perf_VGT_SPI_ESVERT_VALID ) 
//--------------------------------------------------------------------------------------
#define GPU_COUNTER( type, value )      \
    GPUCounters::Counter( offsetof( D3D11X_COUNTER_SET_DESC, type ) / 4,                            \
                          offsetof( D3D11X_COUNTER_DATA, type ) / sizeof( UINT64 ),                 \
                          sizeof( ((D3D11X_COUNTER_DATA*)0)->##type ) / sizeof( UINT64 ),           \
                          _countof( ((D3D11X_COUNTER_DATA*)0)->##type ),                            \
                          GPUPerfCounters::value, L#type, L#value )


//--------------------------------------------------------------------------------------
// Name: GetAllDerivedCounters
// Desc: Adds all derived counters to the list
//--------------------------------------------------------------------------------------
void GetAllDerivedCounters( std::vector< DerivedCounter >& derived );

//--------------------------------------------------------------------------------------
// Name: GetRequiredHWCounters
// Desc: Returns the list of hardware counters required to get the derived counters
//--------------------------------------------------------------------------------------
void GetRequiredHWCounters( const std::vector< DerivedCounter >& derivedIn, std::vector< GPUCounters::Counter >& rawOut, const std::vector< GPUCounters::Counter >& hwExtra );

//--------------------------------------------------------------------------------------
// Name: GetRequiredHWCounters
// Desc: Returns text description of the derived counter
//--------------------------------------------------------------------------------------
const WCHAR* GetDerivedCounterDescription( DerivedCounter c );

//--------------------------------------------------------------------------------------
// Name: GetDerivedCounterName
// Desc: Returns short text name of the derived counter
//--------------------------------------------------------------------------------------
const WCHAR* GetDerivedCounterName( DerivedCounter c );

//--------------------------------------------------------------------------------------
// Name: ComputeDerivedCounters
// Desc: Compute the values of derived counters given the captured hardware counters
//--------------------------------------------------------------------------------------
void ComputeDerivedCounters( _Out_writes_( DC_NUM_COUNTERS ) DOUBLE* pResults,
                            const std::vector< DerivedCounter >& derivedIn,
                            _In_ const GPUCounters::Counter* pHWCounters,
                            const std::vector< GPUCounters::CounterOutput >& data,
                            UINT numHWCounters );

//--------------------------------------------------------------------------------------
// Name: DerivedCounters
// Desc: A helper class that wraps up all that is needed to grab the counters
//--------------------------------------------------------------------------------------
class DerivedCounters
{
private:
    GPUCounters*                                m_pCounters;
    UINT                                        m_numPasses;
    DOUBLE                                      m_results[ DC_NUM_COUNTERS ];
    std::vector< GPUCounters::CounterOutput >   m_output;
    std::vector< DerivedCounter >               m_derived;
    std::vector< GPUCounters::Counter >         m_hwExtra;
    std::vector< GPUCounters::Counter >         m_hw;
public:
    DerivedCounters();

    //--------------------------------------------------------------------------------------
    // Name: GetResults
    // Desc: Returns computed results of the derived counters
    //--------------------------------------------------------------------------------------
    const DOUBLE*   GetResults() const
    {
        return m_results;
    }

    //--------------------------------------------------------------------------------------
    // Name: EnableAllCounters
    // Desc: Enables all derived counters
    //--------------------------------------------------------------------------------------
    void EnableAllCounters();

    //--------------------------------------------------------------------------------------
    // Name: EnableCSCounters
    // Desc: Enables compute shader counters
    //--------------------------------------------------------------------------------------
    void EnableCSCounters();

    //--------------------------------------------------------------------------------------
    // Name: DisablesAllCounters
    // Desc: Disables all derived counters
    //--------------------------------------------------------------------------------------
    void DisableAllCounters();

    //--------------------------------------------------------------------------------------
    // Name: FromGivenCounters
    // Desc: Sets up this object to query the given hardware and derived counters
    //--------------------------------------------------------------------------------------
    void FromGivenCounters( const std::vector< GPUCounters::Counter >& HWCounters,
                            const std::vector< DerivedCounter >& derivedCounters );

    //--------------------------------------------------------------------------------------
    // Name: AddDerivedCounter
    // Desc: Add a single derived counter to this object
    //--------------------------------------------------------------------------------------
    void AddDerivedCounter( DerivedCounter c );

    //--------------------------------------------------------------------------------------
    // Name: AddHWCounter
    // Desc: Add a single hw counter to this object
    //--------------------------------------------------------------------------------------
    void AddHWCounter( const GPUCounters::Counter& c );

    //--------------------------------------------------------------------------------------
    // Name: StartPasses
    // Desc: Starts reading the hardware counters over several passes, returns the number
    // of passes required
    //--------------------------------------------------------------------------------------
    UINT StartPasses( GPUCounters* pCounters );

    //--------------------------------------------------------------------------------------
    // Name: EndOfPass
    // Desc: Should be called at the end of each pass to correctly collect counters information
    //--------------------------------------------------------------------------------------
    void EndOfPass( UINT uIndex );

    //--------------------------------------------------------------------------------------
    // Name: EndOfPass
    // Desc: Should be called to start the next pass
    //--------------------------------------------------------------------------------------
    void StartNextPass( UINT uIndex );

    //--------------------------------------------------------------------------------------
    // Name: GetHWCountersOutput
    // Desc: Returns raw data values read from the hardware counters
    //--------------------------------------------------------------------------------------
    const std::vector< GPUCounters::CounterOutput >&   GetHWCountersOutput() const
    {
        return m_output;
    }

    //--------------------------------------------------------------------------------------
    // Name: GetHWCounters
    // Desc: Returns the list of enabled hardware counters
    //--------------------------------------------------------------------------------------
    const GPUCounters::Counter* GetHWCounters() const
    {
        if( m_hw.empty() )
            return NULL;

        return &m_hw[ 0 ];
    }

    //--------------------------------------------------------------------------------------
    // Name: GetNumHWCounters
    // Desc: Returns the number of enabled hardware counters
    //--------------------------------------------------------------------------------------
    UINT GetNumHWCounters() const
    {
        return static_cast< UINT >( m_hw.size() );
    }

    //--------------------------------------------------------------------------------------
    // Name: GetDerivedCounters
    // Desc: Returns the list of enabled derived counters
    //--------------------------------------------------------------------------------------
    const DerivedCounter* GetDerivedCounters() const
    {
        if( m_derived.empty() )
            return NULL;

        return &m_derived[ 0 ];
    }

    //--------------------------------------------------------------------------------------
    // Name: GetNumDerivedCounters
    // Desc: Returns the number of enabled derived counters
    //--------------------------------------------------------------------------------------
    UINT GetNumDerivedCounters() const
    {
        return static_cast< UINT >( m_derived.size() );
    }


    //--------------------------------------------------------------------------------------
    // Name: GetHWCounterValue
    // Desc: Returns the value of a given hw counter
    //--------------------------------------------------------------------------------------
    UINT64  GetHWCounterValue( const GPUCounters::Counter& c ) const;
};

#else

#define GPU_COUNTER( type, value )      \
    GPUCounters::Counter( 0, 0, 0, 0, 0, L#value )


inline void GetAllDerivedCounters( std::vector< DerivedCounter >& /*derived*/ ) {}
inline void GetRequiredHWCounters( const std::vector< DerivedCounter >& /*derivedIn*/, std::vector< GPUCounters::Counter >& /*rawOut*/, const std::vector< GPUCounters::Counter >& /*hwExtra*/ ) {}
inline const WCHAR* GetDerivedCounterDescription( DerivedCounter /*c*/ ) { return L""; }
inline const WCHAR* GetDerivedCounterName( DerivedCounter /*c*/ ) { return L""; }
inline void ComputeDerived( DOUBLE* /*pOut*/, const std::vector< DerivedCounter >& /*derivedIn*/, const GPUCounters::Counter* /*pHWCounters*/, const std::vector< GPUCounters::CounterOutput >& /*data*/, UINT /*numCounters*/ ) {}


//--------------------------------------------------------------------------------------
// Name: DerivedCounters
// Desc: A helper class
//--------------------------------------------------------------------------------------
struct DerivedCounters
{
public:
    const DOUBLE*   GetResults() const { return NULL; }

    void EnableAllCounters() {}
    void EnableCSCounters() {}
    void DisableAllCounters() {}

    void AddDerivedCounter( DerivedCounter /*c*/ ) {}
    void AddHWCounter( const GPUCounters::Counter& /*c*/ ) {}

    UINT StartPasses( GPUCounters* /*pCounters*/ ) { return 0; }
    void EndOfPass( UINT /*uIndex*/ ) {}
    void StartNextPass( UINT /*uIndex*/ ) {}

    const std::vector< GPUCounters::CounterOutput >&   GetHWCountersOutput() const
    {
        static std::vector< GPUCounters::CounterOutput > t;
        return t;
    }

    const GPUCounters::Counter* GetHWCounters() const
    {
        return NULL;
    }

    UINT GetNumHWCounters() const
    {
        return 0;
    }

    const DerivedCounter* GetDerivedCounters() const
    {
        return NULL;
    }

    UINT GetNumDerivedCounters() const
    {
        return 0;
    }

    UINT64  GetHWCounterValue( const GPUCounters::Counter& /*c*/ ) const
    {
        return 0;
    }
};


#endif



//--------------------------------------------------------------------------------------
// Name: PrintHWCounterValues
// Desc: Displays the values of hardware counters
//--------------------------------------------------------------------------------------
void    PrintHWCounterValues( _In_ const GPUCounters::Counter* pCounters, const std::vector< GPUCounters::CounterOutput >& data, UINT numCounters );

//--------------------------------------------------------------------------------------
// Name: PrintDerivedCounterValues
// Desc: Displays the values of hardware counters
//--------------------------------------------------------------------------------------
void    PrintDerivedCounterValues( _In_ const DOUBLE* pResults, const DerivedCounter* pCounters, UINT numCounters );

// You can wrap your draw call in this to reduce the amount of typing
// Use like so
//    DerivedCounters derivedCounters;
//    derivedCounters.EnableAllCounters();
//    XSF_COLLECT_GPU_COUNTERS( derivedCounters, &m_gpuCounters,
//            
//                pCtx->Draw( NUM_PARTICLES, 0 );
//            
//    );
//    PrintDerivedCounterValues( derivedCounters.GetResults(), derivedCounters.GetDerivedCounters(), derivedCounters.GetNumDerivedCounters() );
//
#define XSF_COLLECT_GPU_COUNTERS( c, gpuCounters, body ) {\
    const UINT __np = c.StartPasses( gpuCounters );\
    for( UINT __i=0; __i < __np; ++__i ) {\
        {body};\
        c.EndOfPass( __i );\
        c.StartNextPass( __i );\
    }\
}

