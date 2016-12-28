//----------------------------------------------------------------------------------------------------------------------
// Perf.h
// 
// Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#ifndef PERF_H_GUARD
#define PERF_H_GUARD

#ifndef _XBOX_ONE

// Comment this out to use rdtscp to measure cycles on Windows 8, which may give more accurate cycle counts, but could
// have other stability issues that are normally accounted for by QueryPerformanceCounter.

#define ATGPROFILE_USE_QPC

#endif //!_XBOX_ONE

#if defined(ATG_PROFILE_VERBOSE) || defined(ATG_PROFILE)

#include <intrin.h>

namespace XboxSampleFramework
{
    namespace ATGProfiler
    {
        //----------------------------------------------------------------------------------------------------------------------
        // Name: enum OUTPUTFORMAT
        // Desc: The reporting formats we support.
        //----------------------------------------------------------------------------------------------------------------------
        enum OUTPUTFORMAT
        {
            CSV_COMPATIBLE = 0,          // Output suitable for import into a spreadsheet
            SCREEN_FRIENDLY = 1,         // Output suitable for reading on-screen.
            FORMAT_MASK = 1,             // Mask used to mask-off output formats
            FLAG_TO_DEBUG_CHANNEL = 4,   // Flag that specifies output to debug channel
            FLAG_TO_STD_OUT = 8,         // Flag that specifies output to stdout
            FLAG_TO_FILE = 16            // Flag that specifies output to file.
        };

        //----------------------------------------------------------------------------------------------------------------------
        // Name: Initialize
        // Desc: Starts the perf mgmt system, and allocates buffers.
        //----------------------------------------------------------------------------------------------------------------------
        HRESULT Initialize( DWORD dwMaxFunctionCount = 4096 );

        //----------------------------------------------------------------------------------------------------------------------
        // Name: ShutdownTimerSystem
        // Desc: Releases the buffers used by the system.
        //----------------------------------------------------------------------------------------------------------------------
        void ShutdownTimerSystem();

        //----------------------------------------------------------------------------------------------------------------------
        // Name: InitializeForThread
        // Desc: Initializes the timing system for this thread.
        //----------------------------------------------------------------------------------------------------------------------
        HRESULT InitializeThread( _In_z_ const char* pszThreadName = "Unnamed Thread" );

        //----------------------------------------------------------------------------------------------------------------------
        // Name: Enter
        // Desc: Marks the start of a function. The parameter is the name to use in the report.
        //----------------------------------------------------------------------------------------------------------------------
        void Enter( _In_z_ const char* pszFunctionName );

        //----------------------------------------------------------------------------------------------------------------------
        // Name: Exit
        // Desc: Marks the exit of a function. The parameter is the amount of time spent inclusively in the function.
        //----------------------------------------------------------------------------------------------------------------------
    #ifdef ATGPROFILE_USE_QPC
        void Exit( __int64 end, __int64 start, __int64 overheadStart );
    #else // use __RDTSCP
        void Exit( unsigned __int64 end, unsigned __int64 start, unsigned __int64 overheadStart );
    #endif

        //----------------------------------------------------------------------------------------------------------------------
        // Name: XboxSampleFramework::ATGProfiler::StartCapture
        // Desc: Allows data to be captured by the system.
        //----------------------------------------------------------------------------------------------------------------------
        void StartCapture();

        //----------------------------------------------------------------------------------------------------------------------
        // Name: XboxSampleFramework::ATGProfiler::StopCapture
        // Desc: Stops capturing data. 
        //----------------------------------------------------------------------------------------------------------------------
        void StopCapture();

        //----------------------------------------------------------------------------------------------------------------------
        // Name: XboxSampleFramework::ATGProfiler::HaltThreadsOnNextFunctionExit
        // Desc: Halts the measured threads at the next exit from a function 
        //----------------------------------------------------------------------------------------------------------------------
        void HaltThreadsOnNextFunctionExit();
        
        //----------------------------------------------------------------------------------------------------------------------
        // Name: XboxSampleFramework::ATGProfiler::ResetCounters
        // Desc: Resets all of the cycle counts to zero. (Implicitly calls Start/StopCapture, but leaves the system in the same
        // state as on entry).
        //----------------------------------------------------------------------------------------------------------------------
        void ResetCounters();

        //----------------------------------------------------------------------------------------------------------------------
        // Name: XboxSampleFramework::ATGProfiler::EmitResults
        // Desc: Writes the results to stdout.
        //----------------------------------------------------------------------------------------------------------------------
        HRESULT EmitResults( OUTPUTFORMAT format = CSV_COMPATIBLE, LPCWSTR pszLogFileName = nullptr );

        //----------------------------------------------------------------------------------------------------------------------
        // Name: struct XboxSampleFramework::ATGProfiler::Timer
        // Desc: Resource-Initialization is Acquisition pattern based scoped timer. Identified by a const single-byte string.
        //----------------------------------------------------------------------------------------------------------------------
    #ifdef ATGPROFILE_USE_QPC

        struct Timer
        {
            LARGE_INTEGER m_start;
            LARGE_INTEGER m_end;
            LARGE_INTEGER m_overhead;

            FORCEINLINE Timer( LPCSTR pszFuncName )
            {
                QueryPerformanceCounter( &m_overhead );
                XboxSampleFramework::ATGProfiler::Enter( pszFuncName );
                QueryPerformanceCounter( &m_start );
            }

            FORCEINLINE ~Timer()
            {
                QueryPerformanceCounter( &m_end );
                XboxSampleFramework::ATGProfiler::Exit( (__int64)m_end.QuadPart, (__int64)m_start.QuadPart, (__int64)m_overhead.QuadPart );
            }
        };

    #else // use RDTSCP

        struct Timer
        {
            unsigned __int64 m_overhead;
            unsigned __int64 m_start;
            unsigned __int64 m_end;

            FORCEINLINE Timer( LPCSTR pszFuncName )
            {
    #ifdef ATGPROFILE_USE_RDTSCP
                unsigned int tsc_aux;
                m_overhead = __rdtscp( &tsc_aux );
                XboxSampleFramework::ATGProfiler::Enter( pszFuncName );
                m_start = __rdtscp( &tsc_aux );
    #else
                m_overhead = __rdtsc();
                XboxSampleFramework::ATGProfiler::Enter( pszFuncName ); 
                m_start = __rdtsc();
    #endif
            }

            FORCEINLINE ~Timer()
            {
    #ifdef ATGPROFILE_USE_RDTSCP
                unsigned int tsc_aux;
                m_end = __rdtscp(&tsc_aux);
    #else
                m_end = __rdtsc();
    #endif
                XboxSampleFramework::ATGProfiler::Exit( m_end, m_start, m_overhead );
            }
        };

    #endif // ATGPROFILE_USE_QPC

    };
};

#endif

#if defined(ATG_PROFILE_VERBOSE)

#define ATGPROFILETHIS XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define STARTATGPROFILETHIS { XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define ENDATGPROFILETHIS }
#define STARTATGPROFILELABEL( a ) { XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define ENDATGPROFILELABEL }
#define ATGPROFILELABEL( a ) XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define VERBOSEATGPROFILETHIS XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define VERBOSESTARTATGPROFILETHIS { XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define VERBOSEENDATGPROFILETHIS }
#define VERBOSESTARTATGPROFILELABEL( a ) { XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define VERBOSEATGPROFILELABEL XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define VERBOSEENDATGPROFILELABEL }

#elif defined(ATG_PROFILE)

#define ATGPROFILETHIS XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define STARTATGPROFILETHIS { XboxSampleFramework::ATGProfiler::Timer __perf_timer( __FUNCSIG__ )
#define ENDATGPROFILETHIS }
#define STARTATGPROFILELABEL( a ) { XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define ATGPROFILELABEL( a ) XboxSampleFramework::ATGProfiler::Timer __perf_timer( a )
#define ENDATGPROFILELABEL }
#define VERBOSEATGPROFILETHIS
#define VERBOSESTARTATGPROFILETHIS
#define VERBOSEENDATGPROFILETHIS
#define VERBOSESTARTATGPROFILELABEL( a )
#define VERBOSEATGPROFILELABEL( a )
#define VERBOSEENDATGPROFILELABEL

#else

// Null versions
#define ATGPROFILETHIS
#define STARTATGPROFILETHIS
#define ENDATGPROFILETHIS
#define STARTATGPROFILELABEL( a )
#define ATGPROFILELABEL( a )
#define ENDATGPROFILELABEL
#define VERBOSEATGPROFILETHIS
#define VERBOSESTARTATGPROFILETHIS
#define VERBOSEENDATGPROFILETHIS
#define VERBOSESTARTATGPROFILELABEL( a )
#define VERBOSEATGPROFILELABEL( a )
#define VERBOSEENDATGPROFILELABEL

#endif


#endif //PERF_H_GUARD
