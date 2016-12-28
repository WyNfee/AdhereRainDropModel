#include "pch.h"

#if defined(ATG_PROFILE_VERBOSE) || defined(ATG_PROFILE)

#include "Perf.h"
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations

struct PerfCounter;
struct ActivityMonitor;
PerfCounter* AllocRootNode();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants

// Performance instrumentation configuration

//----------------------------------------------------------------------------------------------------------------------
// Name: MAXFNNAMELEN
// Desc: Maximum length for a function name.
//----------------------------------------------------------------------------------------------------------------------
const size_t MAXFNNAMELEN = 1024;

//----------------------------------------------------------------------------------------------------------------------
// Name: MAXTHREADS
// Desc: Maximum number of unique threads that the collection system can handle. 
//----------------------------------------------------------------------------------------------------------------------
const size_t MAXTHREADS = 128;

//----------------------------------------------------------------------------------------------------------------------
// Name: MAXTHREADNAMELEN
// Desc: Space allocated for the description of a thread.
//----------------------------------------------------------------------------------------------------------------------
const size_t MAXTHREADNAMELEN = 64;

//----------------------------------------------------------------------------------------------------------------------
// Name: SCRATCHTEXTBUFFERLEN
// Desc: Scratch buffer size used for sprintf-style string building.
//----------------------------------------------------------------------------------------------------------------------
const size_t SCRATCHTEXTBUFFERLEN = 1024;

#ifdef _XBOX_ONE

//----------------------------------------------------------------------------------------------------------------------
// Name: CYCLES_PER_SECOND_RDTSC_XBOX_ONE
// Desc: The number of cycles per second for the RDTSC counter on Xbox One.
//----------------------------------------------------------------------------------------------------------------------

static const double CYCLES_PER_SECOND_RDTSC_XBOX_ONE = 2.1E9;

#endif // _XBOX_ONE

#if defined(_XBOX_ONE) && !defined( ATGPROFILE_USE_QPC )

//----------------------------------------------------------------------------------------------------------------------
// Name: TIMER_CALL_CYCLE_COST
// Desc: Each enter/exit makes 4 RDTSCP calls, total, at 46 cycles per call on Xbox One.
//       We can factor these into the overhead.
//----------------------------------------------------------------------------------------------------------------------

static const ULONGLONG TIMER_CALL_CYCLE_COST_X2 = 2ULL * 46ULL;
static const ULONGLONG TIMER_CALL_CYCLE_COST = 46ULL;

#elif defined( ATGPROFILE_USE_QPC )

// We estimate these costs by calling QueryPerformanceCounter in a loop at initialization time...
// On Xbox One, this is actually not useful, as the cost of calling QPC is less than the resolution of the QPC timer.
// On Windows, or in future, it may be useful though, so we keep it around for future proofing purposes.
static ULONGLONG TIMER_CALL_CYCLE_COST_X2= 0ULL;
static ULONGLONG TIMER_CALL_CYCLE_COST = 0ULL;

#endif

// Strings used by the reporting system.

//----------------------------------------------------------------------------------------------------------------------
// Name: BYTE_ORDER_MARK
// Desc: Used to designate endianness of a UTF-16 file.
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* BYTE_ORDER_MARK = L"\xFEFF";

//----------------------------------------------------------------------------------------------------------------------
// Name: HEADER_CSV_FORMAT
// Desc: Header for the report when emitted in CSV format
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* HEADER_CSV_FORMAT = L"Function,Calls,Exc Avg Counts,Exc Avg Time (ms),Inc Avg Counts,Inc Avg Time (ms),Exc Total Counts,Exc Total Time (ms),Inc Total Counts,Inc Total Time (ms)\r\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: HEADER_SCREEN_FORMAT
// Desc: Header for the report when emitted in raw text format (on-screen viewing)
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* HEADER_SCREEN_FORMAT = L"Function                                                         |      Calls |        Exc Avg Counts | Exc Avg Time |        Inc Avg Counts | Inc Avg Time | Exc Total Counts    |Exc Total Time|     Inc Total Counts| Inc Total Time (ms)\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: THREADHEADER_SCREEN_FORMAT
// Desc: Header each unique thread in the report when emitted in raw text format.
//       A single ANSI string (the thread's name) will be provided as a parameter
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* THREADHEADER_SCREEN_FORMAT = L"-%hs-\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: THREADHEADER_CSV_FORMAT
// Desc: Header each unique thread in the report when emitted in CSV format
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* THREADHEADER_CSV_FORMAT = L"-%hs-\r\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: FOOTER_CSV_FORMAT
// Desc: Footer for the report when emitted in CSV format
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* FOOTER_CSV_FORMAT = L"== End of Report ==\r\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: FOOTER_SCREEN_FORMAT
// Desc: Header for the report when emitted in raw text format
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* FOOTER_SCREEN_FORMAT = L"== End of Report ==\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: THREADFOOTER_CSV_FORMAT
// Desc: Header for the report when emitted in CSV format. A single ANSI string can be provided (thread name)
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* THREADFOOTER_CSV_FORMAT = L"-- End of Thread --\r\n\r\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: THREADFOOTER_SCREEN_FORMAT
// Desc: Header for the report when emitted in raw text format
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* THREADFOOTER_SCREEN_FORMAT = L"-- End of Thread --\n\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: TIMINGENTRY_CSV_FORMAT
// Desc: A single line in the report for a function entry, for CSV reports.
// Params:
//       0 - Function name + indent
//       1 - Call count
//       2 - Average exclusive cycle count
//       3 - Average exclusive time ms
//       4 - Average total cycle count
//       5 - Average total time
//       6 - Total exclusive cycle count
//       7 - Total exclusive time
//       8 - Total inclusive cycle count
//       9 - Total inclusive time
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* TIMINGENTRY_CSV_FORMAT = L"%hs,%lu,%.2f,%.6f,%.2f,%.6f,%I64d,%.6f,%I64d,%.6f\r\n";

//----------------------------------------------------------------------------------------------------------------------
// Name: TIMINGENTRY_SCREEN_FORMAT
// Desc: A single line in the report for a function entry, for on-screen reports.
// Params:
//       0 - Function name + indent
//       1 - Call count
//       2 - Average exclusive cycle count
//       3 - Average exclusive time ms
//       4 - Average total cycle count
//       5 - Average total time
//       6 - Total exclusive cycle count
//       7 - Total exclusive time
//       8 - Total inclusive cycle count
//       9 - Total inclusive time//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* TIMINGENTRY_SCREEN_FORMAT = L"%-64.64hs | %10lu | %21.2f | %12.6f | %21.2f | %12.6f | %19I64d | %12.6f | %19I64d | %12.6f\n";;

//----------------------------------------------------------------------------------------------------------------------
// Name: LOGFILE_STORAGE_ROOT
// Desc: This is currently in flux, so you can change it here if it moves.
//----------------------------------------------------------------------------------------------------------------------
#define LOGFILE_STORAGE_ROOT L"G:\\"

//----------------------------------------------------------------------------------------------------------------------
// Name: LOGFILE_PATH
// Desc: The folder in which log files will be stored.
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* LOGFILE_FOLDER_NAME = LOGFILE_STORAGE_ROOT L"PerfLogs";

//----------------------------------------------------------------------------------------------------------------------
// Name: LOGFILE_NAME
// Desc: The format used to create a logfile filename.
// Params: 0 - Path root
//         1 - Year
//         2 - Month
//         3 - Day of the Month
//         4 - Hours
//         5 - Minutes
//         6 - Seconds
//         7 - Microseconds
//         8 - File extension
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* LOGFILE_NAME = L"%s\\%04d%02d%02d %02d%02d%02d.%03d.%s";

//----------------------------------------------------------------------------------------------------------------------
// Name: SPECIFIC_LOGFILE_NAME
// Desc: The format string used when a specific logfile is specified. The storage folder will be prefixed to it.
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* SPECIFIC_LOGFILE_NAME = L"%s\\%s";

//----------------------------------------------------------------------------------------------------------------------
// Name: LOGFILE_CSV_EXT
// Desc: Filename extension for CSV report
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* LOGFILE_CSV_EXT = L"csv";

//----------------------------------------------------------------------------------------------------------------------
// Name: LOGFILE_CSV_EXT
// Desc: Filename extension for on-screen report
//----------------------------------------------------------------------------------------------------------------------
static const WCHAR* LOGFILE_SCREEN_EXT = L"txt";

// Number of milliseconds in a second.
const double MILLISECONDS_PER_SECOND = 1000.0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread-local variables

// Per-thread current head
__declspec(thread) static PerfCounter* s_tl_pCurrentHead = nullptr;
__declspec(thread) static BOOL* s_tl_pActivity = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static variables

// Number of slots allocated for function timing recording
static DWORD s_dwMaxFunctionCount = 0;

// Next free slot # for the allocator
static volatile DWORD dwLastUsedFunction = 0;

// Function entry point buffer (the slots we'll allocate for timing info).
static PerfCounter* s_pBuffer;

// List of threads in use by this process which have been marked for measurement.
static PerfCounter* s_ThreadListHeads[ MAXTHREADS ];

// The Thread IDs for each of the threads.
static DWORD s_ThreadIDs[ MAXTHREADS ];

// Next free index in thread list.
static volatile DWORD s_dwThreadNextFreeIndex = 0;

//----------------------------------------------------------------------------------------------------------------------
// Name: s_CaptureEnabled
// Desc: If TRUE, functions will contribute to timing information. If FALSE, functions will still be added to the 
//       call graph, but will timings will not be kept.
//----------------------------------------------------------------------------------------------------------------------
static volatile BOOL s_CaptureEnabled = FALSE;

#if !defined(_XBOX_ONE) || defined(ATGPROFILE_USE_QPC)

//----------------------------------------------------------------------------------------------------------------------
// Name: s_PerfCounterFrequency
// Desc: The frequency for the performance counter. Used by the reporting system for calibrating RDTSC calls on Windows
//       if RDTSC is used.
//----------------------------------------------------------------------------------------------------------------------
static LARGE_INTEGER s_PerfCounterFrequency;

#endif //!_XBOX_ONE

#if !defined(ATGPROFILE_USE_QPC)

// Number of cycles per ms for __rdtscp counter.
static double s_CyclesPerSecond;

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structs

#define CACHEALIGN __declspec(align(64))

//----------------------------------------------------------------------------------------------------------------------
// Name: PerfCounter
// Desc: Wraps all of the information in the call graph
//----------------------------------------------------------------------------------------------------------------------
CACHEALIGN struct PerfCounter
{
    const char*   pszName;            // Name of this function.
    PerfCounter*  pNextSibling;       // Next sibling in the chain
    PerfCounter*  pParent;            // Parent function in the graph
    PerfCounter*  pFirstChild;        // First child of this function in the graph
    DWORD         dwCallCount;        // Number of times this function was successfully entered and exited
    __int64       inclusiveCycles;    // Total count of cycles spent in this function.
    __int64       exclusiveCycles;    // The exclusive cycle count for this function.
    __int64       overheadCycles;     // Cycles spent in child functions which are measurement overhead.
};

//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat
// Desc: Contains all the necessary info to format a report for screen or CSV
//----------------------------------------------------------------------------------------------------------------------
class ReportFormat
{
    HANDLE m_File;
    XboxSampleFramework::ATGProfiler::OUTPUTFORMAT m_format;
    const WCHAR* m_ReportHeader;
    const WCHAR* m_ReportFooter;
    const WCHAR* m_ThreadHeader;
    const WCHAR* m_ThreadFooter;
    const WCHAR* m_EntryFormat;

    WCHAR* m_pScratch;
    WCHAR m_StorageRoot[ MAX_PATH ];
    WCHAR m_ApplicationDataPath[ MAX_PATH ];

public:
    ReportFormat();
    ~ReportFormat();

    HRESULT Initialize( XboxSampleFramework::ATGProfiler::OUTPUTFORMAT format, LPCWSTR pszLogFileName );
    HRESULT WriteReportHeader() const;
    HRESULT WriteThreadHeader( _In_z_ const char* pszThreadName ) const;
    HRESULT WriteEntry( _In_ const PerfCounter* pNode, int indent) const;
    HRESULT WriteThreadFooter( _In_z_ const char* pszThreadName ) const;
    HRESULT WriteReportFooter() const;
    void Shutdown();
private:
    HRESULT WriteOutput( _In_z_ const WCHAR* pText ) const;
    const WCHAR* GetStorageRoot();
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method Implementations



//----------------------------------------------------------------------------------------------------------------------
// Name: ToMsec
// Desc: Converts from perf counter cycles to time in milliseconds.
//----------------------------------------------------------------------------------------------------------------------
#ifdef ATGPROFILE_USE_QPC

static double ToMsec( __int64 cycles )
{
    double countsPerSecond = static_cast<double>(s_PerfCounterFrequency.QuadPart);
    double cyclesReal = static_cast<double>(cycles);
    return ( cyclesReal * MILLISECONDS_PER_SECOND ) / countsPerSecond ;
}

static double ToMsec( double cycles )
{
    double countsPerSecond = static_cast<double>(s_PerfCounterFrequency.QuadPart);
    return ( cycles * MILLISECONDS_PER_SECOND ) / countsPerSecond ;
}
#else // !ATGPROFILE_USE_QPC

static double ToMsec( __int64 cycles )
{
    double cyclesReal = static_cast<double>(cycles);
    return ( cyclesReal * MILLISECONDS_PER_SECOND ) / s_CyclesPerSecond ;
}

static double ToMsec( double cycles )
{
    return ( cycles * MILLISECONDS_PER_SECOND ) / s_CyclesPerSecond ;
}

#endif // ATGPROFILE_USE_QPC

//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::ReportFormat
// Desc: Constructor for the ReportFormat object. Does not perform initialization; see ReportFormat::Initialize
//----------------------------------------------------------------------------------------------------------------------
ReportFormat::ReportFormat()
    : m_File( INVALID_HANDLE_VALUE ),
    m_ReportFooter( nullptr ),
    m_ReportHeader( nullptr ),
    m_ThreadFooter( nullptr ),
    m_ThreadHeader( nullptr ),
    m_EntryFormat( nullptr ),
    m_pScratch( nullptr )
{

}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::~ReportFormat
// Desc: Destructor; calls Shutdown if Shutdown wasn't called already.
//----------------------------------------------------------------------------------------------------------------------
ReportFormat::~ReportFormat()
{
    Shutdown();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::Shutdown
// Desc: Closes out the report (closing the file, if writing to file was enabled).
//----------------------------------------------------------------------------------------------------------------------
void ReportFormat::Shutdown()
{
    if ( m_File != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_File );
        m_File = INVALID_HANDLE_VALUE;
    }

    delete[] m_pScratch;
    m_pScratch = nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::Initialize
// Desc: 
// Parameters:
// Returns: HRESULT - 
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::Initialize( XboxSampleFramework::ATGProfiler::OUTPUTFORMAT format, LPCWSTR pszLogFileName )
{
    m_pScratch = new WCHAR[ SCRATCHTEXTBUFFERLEN ];
    m_format = format;
    const WCHAR* pFileExtension;


    switch ( format & XboxSampleFramework::ATGProfiler::FORMAT_MASK )
    {
    case XboxSampleFramework::ATGProfiler::CSV_COMPATIBLE:
        {
            m_ReportHeader = HEADER_CSV_FORMAT;
            m_ReportFooter = FOOTER_CSV_FORMAT;
            m_ThreadHeader = THREADHEADER_CSV_FORMAT;
            m_ThreadFooter = THREADFOOTER_CSV_FORMAT;
            m_EntryFormat = TIMINGENTRY_CSV_FORMAT;

            // Used in this function if we're writing to a file.
            pFileExtension = LOGFILE_CSV_EXT;
            break;
        }
    case XboxSampleFramework::ATGProfiler::SCREEN_FRIENDLY:
    default:
        {
            m_ReportHeader = HEADER_SCREEN_FORMAT;
            m_ReportFooter = FOOTER_SCREEN_FORMAT;
            m_ThreadHeader = THREADHEADER_SCREEN_FORMAT;
            m_ThreadFooter = THREADFOOTER_SCREEN_FORMAT;
            m_EntryFormat = TIMINGENTRY_SCREEN_FORMAT;
            
            // Used in this function if we're writing to a file.
            pFileExtension = LOGFILE_SCREEN_EXT;
            break;
        }
    }

    if ( ( m_format & XboxSampleFramework::ATGProfiler::FLAG_TO_FILE ) == XboxSampleFramework::ATGProfiler::FLAG_TO_FILE )
    {
        WCHAR temp[MAX_PATH] = {0};

        wcscpy_s( temp, LOGFILE_FOLDER_NAME );


        // Check that folder exists...
        DWORD dwAttribs = GetFileAttributes( temp );
        if ( dwAttribs == INVALID_FILE_ATTRIBUTES || !( dwAttribs & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            // Try to create the folder
            if ( !CreateDirectory( temp, nullptr ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }

        if ( pszLogFileName == nullptr )
        {
            // Get current date/time
            FILETIME time, localTime;
            SYSTEMTIME sysTime;

            // GetLocalTime has gone away, so we work around it like this:
            ::GetSystemTimeAsFileTime( &time );
            ::FileTimeToLocalFileTime( &time, &localTime );
            ::FileTimeToSystemTime( &localTime, &sysTime );

            _snwprintf_s( m_pScratch, SCRATCHTEXTBUFFERLEN, SCRATCHTEXTBUFFERLEN - 1, LOGFILE_NAME, temp,
                sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds,
                pFileExtension );
        }
        else
        {
            _snwprintf_s( m_pScratch, SCRATCHTEXTBUFFERLEN, SCRATCHTEXTBUFFERLEN - 1, SPECIFIC_LOGFILE_NAME, temp, pszLogFileName );
        }

        // Create log file.

        CREATEFILE2_EXTENDED_PARAMETERS params;
        ZeroMemory( &params, sizeof(params) );

        params.dwSize = sizeof( params );
        params.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;

        m_File = CreateFile2( m_pScratch, GENERIC_WRITE, 0, CREATE_ALWAYS, &params );
        if ( m_File == INVALID_HANDLE_VALUE )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        // Write out Byte-order mark
        if ( !WriteFile( m_File, BYTE_ORDER_MARK, sizeof(WCHAR), nullptr, nullptr ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    return S_OK;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteOutput
// Desc: Writes a block of text out to the outputs that were specified when this object was initialized.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteOutput( _In_z_ const WCHAR* pText ) const
{
    if ( ( m_format & XboxSampleFramework::ATGProfiler::FLAG_TO_DEBUG_CHANNEL ) == XboxSampleFramework::ATGProfiler::FLAG_TO_DEBUG_CHANNEL )
    {
        OutputDebugStringW( pText );
    }

    if ( ( m_format & XboxSampleFramework::ATGProfiler::FLAG_TO_STD_OUT ) == XboxSampleFramework::ATGProfiler::FLAG_TO_STD_OUT )
    {
        _putws( pText );
    }

    if ( ( m_format & XboxSampleFramework::ATGProfiler::FLAG_TO_FILE ) == XboxSampleFramework::ATGProfiler::FLAG_TO_FILE )
    {
        // Intentionally ignore errors...
        if ( !WriteFile( m_File, pText, (DWORD)wcslen(pText) * sizeof(WCHAR), nullptr, nullptr ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    return S_OK;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteReportHeader
// Desc: Writes out the header for the report.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteReportHeader() const
{
    return WriteOutput( m_ReportHeader );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteReportFooter
// Desc: Writes out the footer for the report.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteReportFooter() const
{
    return WriteOutput( m_ReportFooter );
}



//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteThreadHeader
// Desc: Writes out a new section of the report, detailing which thread it corresponds to.
// Parameters:
//   pszThreadName - The name of the thread this section applies to.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteThreadHeader( _In_z_ const char* pszThreadName ) const
{
    _snwprintf_s( m_pScratch, SCRATCHTEXTBUFFERLEN, _TRUNCATE, m_ThreadHeader, pszThreadName );
    return WriteOutput( m_pScratch );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteThreadFooter
// Desc: Writes out the end of this thread's section of the report/
// Parameters:
//   pszThreadName - The name of the thread that this section applies to.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteThreadFooter( _In_z_ const char* pszThreadName ) const
{
    _snwprintf_s( m_pScratch, SCRATCHTEXTBUFFERLEN, _TRUNCATE, m_ThreadFooter, pszThreadName );
    return WriteOutput( m_pScratch );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::WriteEntry
// Desc: Writes out a report line for the specified node
// Parameters:
//   pNode - The node to emit
//   indent - The distance of this node from the root.
//----------------------------------------------------------------------------------------------------------------------
HRESULT ReportFormat::WriteEntry( _In_ const PerfCounter* pNode, int indent ) const
{
    // Emit indent

    std::string tempFunctionName;
    tempFunctionName.append( indent, '.' );
    tempFunctionName.append( pNode->pszName );

    // If we're in CSV format, replace commas in function names with ` chars.

    if ( ( m_format & XboxSampleFramework::ATGProfiler::FORMAT_MASK ) == XboxSampleFramework::ATGProfiler::CSV_COMPATIBLE )
    {
        for ( size_t i = 0; i < tempFunctionName.length(); ++i )
        {
            if ( tempFunctionName[ i ] == ',' )
            {
                tempFunctionName[ i ] = '`';
            }
        }
    }

    double avgExclusiveCycles = ( (double) pNode->exclusiveCycles / (double)pNode->dwCallCount );
    double avgInclusiveCycles = ( (double) pNode->inclusiveCycles / (double) pNode->dwCallCount );
    double avgExclusiveTimeMs = ToMsec( avgExclusiveCycles );
    double avgInclusiveTimeMs = ToMsec( avgInclusiveCycles );
    double inclusiveTimeMs = ToMsec( pNode->inclusiveCycles );
    double exclusiveTimeMs = ToMsec( pNode->exclusiveCycles );

    _snwprintf_s( m_pScratch,                   // (Text buffer)
        SCRATCHTEXTBUFFERLEN,                   // (Buffer length)
        _TRUNCATE,                              // Allow truncation of report line...
        m_EntryFormat,                          // (Format specifier)
        tempFunctionName.c_str(),               // Function name + indent
        pNode->dwCallCount,                     // Call count
        avgExclusiveCycles,                     // Average exclusive cycle count
        avgExclusiveTimeMs,                     // Average exclusive time ms
        avgInclusiveCycles,                     // Average total cycle count
        avgInclusiveTimeMs,                     // Average total time
        pNode->exclusiveCycles,                 // Total exclusive cycle count
        exclusiveTimeMs,                        // Total exclusive time
        pNode->inclusiveCycles,                 // Total inclusive cycle count
        inclusiveTimeMs                         // Total inclusive time
        );    

   return WriteOutput( m_pScratch );
}


//----------------------------------------------------------------------------------------------------------------------
// Name: ReportFormat::GetStorageRoot
// Returns: const WCHAR* - the application data storage path (ends in \)
//----------------------------------------------------------------------------------------------------------------------
const WCHAR* ReportFormat::GetStorageRoot()
{
#ifdef _XBOX_ONE
    using namespace Windows::Storage;

    StorageFolder^ installFolder = Windows::ApplicationModel::Package::Current->InstalledLocation;
    const WCHAR* installPath = installFolder->Path->Data();

    _snwprintf_s( m_StorageRoot, _countof( m_StorageRoot ), _TRUNCATE, L"%s\\", installPath );

    // The d: drive designator is "developer scratch space"
    std::wstring  writeableFolder = L"d:\\";

    _snwprintf_s( m_ApplicationDataPath, _countof( m_ApplicationDataPath ), _TRUNCATE, L"%s\\", writeableFolder );
#else
    WCHAR temp[ 1024 ];
    GetCurrentDirectoryW( _countof( temp ), temp );

    swprintf_s( m_StorageRoot, L"%s\\", temp );
    swprintf_s( m_ApplicationDataPath, L"%s\\", temp );
#endif
    return m_ApplicationDataPath;
}

//----------------------------------------------------------------------------------------------------------------------
// Name: AllocCounter
// Desc: Allocates a new performance counter from the pool.
// Params:
//       pszName      - the name of the function/text for the label that this item counts towards
//       pParent      - the parent node
//       pPrevSibling - the previous node in the chain (or nullptr)
//----------------------------------------------------------------------------------------------------------------------

FORCEINLINE PerfCounter* AllocCounter( _In_z_ const char* pszName, _In_opt_ PerfCounter* pParent, _In_opt_ PerfCounter* pPrevSibling )
{
    DWORD index = (DWORD)InterlockedIncrement( &dwLastUsedFunction );

    if ( index > s_dwMaxFunctionCount )
    {
        OutputDebugStringW( L"Out of function tracking memory - increase max function count and run again" );
        DebugBreak();
    }

    PerfCounter* pCounter = &s_pBuffer[ index-1 ];
    pCounter->pszName = pszName;
    pCounter->pParent = pParent;

    if ( pPrevSibling )
    {
        pPrevSibling->pNextSibling = pCounter;
    }

    return pCounter;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: AllocRootNode
// Desc: Allocates a root node for a given thread, from the pool of root nodes.
// Parameters:
//   pszThreadName - The name of the thread that this root is being allocated for.
// Returns: A pointer to the root PerfCounter node for this thread.
//----------------------------------------------------------------------------------------------------------------------
FORCEINLINE HRESULT AllocRootNode( _Out_ PerfCounter** pNodeOut, _In_z_ const char* pszThreadName )
{
    LONG index = InterlockedIncrement( &s_dwThreadNextFreeIndex );
    if ( index > MAXTHREADS )
    {
        OutputDebugStringW( L"Too many threads - increase MAXTHREADS" );
        return HRESULT_FROM_WIN32( ERROR_TOO_MANY_THREADS );
    }

    DWORD dwThreadId = GetCurrentThreadId();

    char* pThreadName = new char[ MAXTHREADNAMELEN ];
    sprintf_s( pThreadName, MAXTHREADNAMELEN, "[%s] TID: 0x%lX", pszThreadName, dwThreadId );

    PerfCounter* pCounter = AllocCounter( pThreadName, nullptr, nullptr );

    s_ThreadListHeads[ index - 1 ] = pCounter;
    s_ThreadIDs[ index - 1 ] = dwThreadId;
    s_tl_pCurrentHead = pCounter;

    *pNodeOut = pCounter;

    return S_OK;
}

#if defined( ATGPROFILE_USE_QPC )

#pragma optimize("",off)

//----------------------------------------------------------------------------------------------------------------------
// Name: MeasureQPCCost
// Desc: Estimates the cost of calling QueryPerformanceCounter (in QPC cycles)
//----------------------------------------------------------------------------------------------------------------------
ULONGLONG MeasureQPCCost()
{
    // Measure estimate cost of a call to QPC so we can try to eliminate overhead from the measurement.

    LARGE_INTEGER qpcStart, qpcEnd;
    ULONGLONG qpcTotalCycles = 0ULL;

    const INT QPC_TEST_COUNT = 100;

    QueryPerformanceCounter( &qpcStart );
    for( int i= 0; i < QPC_TEST_COUNT; ++i )
    {
        QueryPerformanceCounter( &qpcEnd );
    }
    QueryPerformanceCounter( &qpcEnd );
    return ( (ULONGLONG)(qpcEnd.QuadPart) - (ULONGLONG)(qpcStart.QuadPart) ) / QPC_TEST_COUNT;
}
#pragma optimize("",on)

#endif // defined( ATGPROFILE_USE_QPC 


//----------------------------------------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Allocates space for the call graph
//----------------------------------------------------------------------------------------------------------------------
HRESULT XboxSampleFramework::ATGProfiler::Initialize( DWORD dwMaxFunctionCount /*= 4096 */ )
{
    if ( s_pBuffer != nullptr )
    {
        OutputDebugStringW( L"Already initialized perf counter system");
        return HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED );
    }

    s_dwThreadNextFreeIndex = 0;
    ::ZeroMemory( s_ThreadListHeads, ARRAYSIZE( s_ThreadListHeads ) );

#if !defined(ATGPROFILE_USE_QPC)

#ifndef _XBOX_ONE

    QueryPerformanceFrequency( &s_PerfCounterFrequency );
    LARGE_INTEGER qpcStart, qpcEnd;


#ifdef ATGPROFILE_USE_RDTSCP
    unsigned int tsc_aux;

    // If we're using  __rdtscp, we need to measure the CPU clock speed.
    
    MemoryBarrier();
    
    unsigned __int64 rdStart = __rdtscp( &tsc_aux );
    QueryPerformanceCounter( &qpcStart );
    
    MemoryBarrier();

    Sleep(1000);
    
    MemoryBarrier();
    
    unsigned __int64 rdEnd = __rdtscp( &tsc_aux );
#else
    MemoryBarrier();

    unsigned __int64 rdStart = __rdtsc();
    QueryPerformanceCounter( &qpcStart );

    MemoryBarrier();

    Sleep(1000);

    MemoryBarrier();

    unsigned __int64 rdEnd = __rdtsc();
#endif

    QueryPerformanceCounter( &qpcEnd );
    
    MemoryBarrier();

    unsigned __int64 rdTime = rdEnd - rdStart;
    unsigned __int64 qpcTime = ((unsigned __int64)qpcEnd.QuadPart) - ((unsigned __int64)qpcStart.QuadPart);
    s_CyclesPerSecond = ( static_cast<double>( rdTime ) / static_cast<double>( qpcTime ) ) *
                            static_cast<double>( s_PerfCounterFrequency.QuadPart );
#else // == _XBOX_ONE

    s_CyclesPerSecond = CYCLES_PER_SECOND_RDTSC_XBOX_ONE;

#endif //!_XBOX_ONE

#else // !defined(ATGPROFILE_USE_QPC)

    QueryPerformanceFrequency( &s_PerfCounterFrequency );

    ULONGLONG qpcTotalCycles = MeasureQPCCost();

    TIMER_CALL_CYCLE_COST = qpcTotalCycles;
    TIMER_CALL_CYCLE_COST_X2 = TIMER_CALL_CYCLE_COST * 2;

#endif


    s_pBuffer = new PerfCounter[ dwMaxFunctionCount ]();
    dwLastUsedFunction = 0;
    s_dwMaxFunctionCount = dwMaxFunctionCount;

    InitializeThread( "Main Thread" );

    return S_OK;
}



//----------------------------------------------------------------------------------------------------------------------
// Name: InitializeThread
// Desc: Initializes the timer system for the current thread (that the main system wasn't created on).
//----------------------------------------------------------------------------------------------------------------------
HRESULT XboxSampleFramework::ATGProfiler::InitializeThread( _In_z_ const char* pszThreadName )
{
    if ( s_tl_pCurrentHead == nullptr )
    {
        //NOTE: Special case; label all your thread starts to swallow this cost in a deterministic fashion.
        return AllocRootNode( &s_tl_pCurrentHead, pszThreadName );
    }

    return S_OK;
}

//----------------------------------------------------------------------------------------------------------------------
// Name: ShutdownTimerSystem
// Desc: Deallocates the memory used by the system.
//----------------------------------------------------------------------------------------------------------------------
void XboxSampleFramework::ATGProfiler::ShutdownTimerSystem()
{
    delete[] s_pBuffer;
    s_pBuffer = nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Enter
// Desc: Marks the start of execution of a function
// Params:
//       pszFunctionName - the name of the function/label that is starting execution.
//----------------------------------------------------------------------------------------------------------------------
void XboxSampleFramework::ATGProfiler::Enter( _In_z_ const char* pszFunctionName )
{
    PerfCounter* pCurrent = s_tl_pCurrentHead;
    pCurrent->overheadCycles = 0ULL;

    // Do we have any potential siblings?

    if ( pCurrent->pFirstChild == nullptr )
    {
        // We're the first, so allocate one for ourselves.

        PerfCounter* pCounter = AllocCounter( pszFunctionName, pCurrent, nullptr );
        pCurrent->pFirstChild = pCounter;
        pCurrent = pCounter;
    }
    else
    {
        // Find matching child or create one.
        pCurrent = pCurrent->pFirstChild;

        for( ;; )
        {
            if ( pCurrent->pszName == pszFunctionName )
            {
                // Match found.
                break;
            }

            if ( pCurrent->pNextSibling == nullptr )
            {
                // No more in the list, so create one.
                pCurrent = AllocCounter( pszFunctionName, pCurrent->pParent, pCurrent );
                break;
            }

            pCurrent = pCurrent->pNextSibling;
        }
    }


    s_tl_pCurrentHead = pCurrent;
}

//----------------------------------------------------------------------------------------------------------------------
// Name: Exit
// Desc: Marks that a function/label has been exited from, accumulating its time.
//----------------------------------------------------------------------------------------------------------------------

#ifdef ATGPROFILE_USE_QPC

void XboxSampleFramework::ATGProfiler::Exit( __int64 end, __int64 start, __int64 overheadStart )
{
    PerfCounter* pCurrent = s_tl_pCurrentHead;
    s_tl_pCurrentHead = pCurrent->pParent;

    if ( s_CaptureEnabled )
    {
        pCurrent->dwCallCount++;
        pCurrent->inclusiveCycles += end - start - pCurrent->overheadCycles - TIMER_CALL_CYCLE_COST;
        if ( s_tl_pCurrentHead != nullptr )
        {
            LARGE_INTEGER overheadEnd;
            QueryPerformanceCounter( &overheadEnd );
            s_tl_pCurrentHead->overheadCycles += ( (__int64)overheadEnd.QuadPart - end ) + ( start - overheadStart ) + TIMER_CALL_CYCLE_COST_X2;
        }
    }
}

#else // use __RDTSCP

void XboxSampleFramework::ATGProfiler::Exit( unsigned __int64 end, unsigned __int64 start, unsigned __int64 overheadStart )
{
#ifdef ATGPROFILE_USE_RDTSCP
    unsigned int tsc_aux;
#endif

    PerfCounter* pCurrent = s_tl_pCurrentHead;
    s_tl_pCurrentHead = pCurrent->pParent;

    if ( s_CaptureEnabled )
    {
        pCurrent->dwCallCount++;
        pCurrent->inclusiveCycles += end - start - pCurrent->overheadCycles - TIMER_CALL_CYCLE_COST;
        if ( s_tl_pCurrentHead != nullptr )
        {
#ifdef ATGPROFILE_USE_RDTSCP
            unsigned __int64 overheadEnd = __rdtscp( &tsc_aux );
#else
            unsigned __int64 overheadEnd = __rdtsc();
#endif
            s_tl_pCurrentHead->overheadCycles += ( overheadEnd - end ) + ( start - overheadStart ) + TIMER_CALL_CYCLE_COST_X2;
        }
    }

    // Note: if s_tl_pCurrentHead is nullptr, we're at the root, and don't care about passing overhead up the chain.
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// Name: EmitTree
// Desc: Walks the timing tree and emits information for this node and its children, depth-first.
//----------------------------------------------------------------------------------------------------------------------
HRESULT EmitTree( int indent, _In_ const PerfCounter* pNode, const ReportFormat& formatInfo )
{
    HRESULT hr = S_OK;

    while ( pNode != nullptr )
    {
        hr = formatInfo.WriteEntry( pNode, indent );
        if ( FAILED( hr ) )
            return hr;

        if ( pNode->pFirstChild != nullptr )
        {
            hr = EmitTree( indent + 1, pNode->pFirstChild, formatInfo );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        pNode = pNode->pNextSibling;
    }

    return hr;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: CalculateExclusiveCount
// Desc: Calculates the exclusive time spent in each node in the tree.
//----------------------------------------------------------------------------------------------------------------------
__int64 CalculateExclusiveCount( _In_ PerfCounter* pNode )
{
    __int64 totalChildCount = 0LL;

    while ( pNode )
    {
        if ( pNode->pFirstChild != nullptr )
        {
            __int64 childCount = CalculateExclusiveCount( pNode->pFirstChild );
            pNode->exclusiveCycles = pNode->inclusiveCycles - childCount;
        }
        else
        {
            pNode->exclusiveCycles = pNode->inclusiveCycles;
        }

        totalChildCount += pNode->exclusiveCycles;

        pNode = pNode->pNextSibling;
    }

    return totalChildCount;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: __Perf::EmitResults
// Desc: Emits the results of the timing capture.
//----------------------------------------------------------------------------------------------------------------------
HRESULT XboxSampleFramework::ATGProfiler::EmitResults( OUTPUTFORMAT format /* = CSV_COMPATIBLE */, LPCWSTR pszLogFileName /*= nullptr */ )
{
    BOOL enabled = s_CaptureEnabled;
    s_CaptureEnabled = FALSE;


    // Walks through the tree, emitting the results.
    //WARNING: Not yet multithreaded; only outputs for calling thread. ENSURE THAT ALL OTHER THREADS ARE STOPPED.

    ReportFormat formatInfo;
    HRESULT hr = formatInfo.Initialize( format, pszLogFileName );

    if ( FAILED(hr) )
    {
        OutputDebugStringW(L"Failed to initialize output");
        if ( IsDebuggerPresent() )
        {
            DebugBreak();
        }

        s_CaptureEnabled = enabled;
        return hr;
    }
    
    hr = formatInfo.WriteReportHeader();
    if ( FAILED(hr) )
    {
        s_CaptureEnabled = enabled;
        return hr;
    }

    for( DWORD index = 0; index < s_dwThreadNextFreeIndex; ++index )
    {
        PerfCounter* pCurrent = s_ThreadListHeads[ index ];

        if ( pCurrent )
        {
            CalculateExclusiveCount( pCurrent );

            hr = formatInfo.WriteThreadHeader( pCurrent->pszName );
            if ( FAILED(hr) )
            {
                s_CaptureEnabled = enabled;
                return hr;
            }

            if ( pCurrent->pFirstChild )
            {
                hr = EmitTree( 0, pCurrent->pFirstChild, formatInfo );
                if ( FAILED(hr) )
                {
                    s_CaptureEnabled = enabled;
                    return hr;
                }
            }

            hr = formatInfo.WriteThreadFooter( pCurrent->pszName );
            if ( FAILED(hr) )
            {
                s_CaptureEnabled = enabled;
                return hr;
            }
        }
    }

    hr = formatInfo.WriteReportFooter();
    if ( FAILED(hr) )
    {
        s_CaptureEnabled = enabled;
        return hr;
    }

    formatInfo.Shutdown();
    s_CaptureEnabled = enabled;

    return S_OK;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: XboxSampleFramework::ATGProfiler::StartCapture
// Desc: Allows data to be captured by the system.
//----------------------------------------------------------------------------------------------------------------------
void XboxSampleFramework::ATGProfiler::StartCapture()
{
    s_CaptureEnabled = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: XboxSampleFramework::ATGProfiler::StopCapture
// Desc: Stops capturing data. 
//----------------------------------------------------------------------------------------------------------------------
void XboxSampleFramework::ATGProfiler::StopCapture()
{
    s_CaptureEnabled = FALSE;
}
#else

// Fix for LNK4221 warning when profiling disabled.
int dummy = 0;

#endif