//--------------------------------------------------------------------------------------
// XMemAllocHooks.cpp
//
// Implementation for XMem Hooks. For details, see header.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "XMemAllocHooks.h"
#include <unordered_map>

#ifdef _XBOX_ONE

#define SENTINEL_WORD( lpAddress, dwSize ) ( *( UINT* )( UINT64( lpAddress ) + dwSize ) )
#define SENTINEL_WORD_SIZE 4
#define SENTINEL_VALUE 0xDEADBEEF

// Use this struct to add more debug data to allocations
struct AddressDebugData
{
    AddressDebugData( SIZE_T dwAllocSize, ULONGLONG dwAllocAttribs )
        : dwSize( dwAllocSize ),
          dwAttribs( dwAllocAttribs )
    {
    }

    SIZE_T dwSize;
    ULONGLONG dwAttribs;
};

static std::unordered_map< PVOID, AddressDebugData > g_mapAddressToDebugData;
static SIZE_T g_dwTotalAllocatedSize;
static SIZE_T g_dwRealTotalAllocatedSize; // This includes the debug check extra data
static SIZE_T g_dwMaxAllocatedSize;

SIZE_T ComputeRealAllocationSize( SIZE_T dwSize, ULONGLONG dwAttributes )
{
    UNREFERENCED_PARAMETER( dwAttributes );

    return dwSize + SENTINEL_WORD_SIZE;
}

PVOID WINAPI XboxSampleFramework::XMemAllocHook( _In_ SIZE_T dwSize, _In_ ULONGLONG dwAttributes )
{
    static bool storingDataGuard = false; // Prevent recursive calls in case operator new calls XMemAllocHook

    SIZE_T dwRealAllocationSize = ComputeRealAllocationSize(dwSize, dwAttributes);

    PVOID lpAddress = XMemAllocDefault( dwRealAllocationSize, dwAttributes );
    if ( !storingDataGuard && nullptr != lpAddress )
    {
        storingDataGuard = true;
        g_dwTotalAllocatedSize += dwSize;
        g_dwRealTotalAllocatedSize += dwRealAllocationSize;

        // Keep track of the maximum amount of memory used
        if ( g_dwTotalAllocatedSize > g_dwMaxAllocatedSize )
        {
            g_dwMaxAllocatedSize = g_dwTotalAllocatedSize;
        }

        // Check if address is already in the map (XMemFreeDefault was called manually or there was a collision)
        auto mapIt = g_mapAddressToDebugData.find( lpAddress );
        XSF_ASSERT( mapIt == g_mapAddressToDebugData.end() );
        g_mapAddressToDebugData.emplace( lpAddress, AddressDebugData( dwSize, dwAttributes ) ); // Move Semantics

        SENTINEL_WORD( lpAddress, dwSize ) = SENTINEL_VALUE;

        storingDataGuard = false;
    }

    return lpAddress;
}

void WINAPI XboxSampleFramework::XMemFreeHook( _In_ PVOID lpAddress, _In_ ULONGLONG dwAttributes )
{
    if (nullptr != lpAddress)
    {
        auto mapIt = g_mapAddressToDebugData.find( lpAddress );
        if ( mapIt != g_mapAddressToDebugData.end() )
        {
            const AddressDebugData& data = mapIt->second;
            UINT64 dwAllocSize = data.dwSize;
            UINT64 dwRealAllocSize = ComputeRealAllocationSize( dwAllocSize, dwAttributes );

            XSF_ASSERT( SENTINEL_VALUE == SENTINEL_WORD( lpAddress, dwAllocSize ) );
            g_dwTotalAllocatedSize -= dwAllocSize;
            g_dwRealTotalAllocatedSize -= dwRealAllocSize;
            g_mapAddressToDebugData.erase( mapIt );
        }
        else
        {
            XSF::DebugPrint( L"XMemFreeHook called without corresponding XMemAllocHook call (This is not necesarrily \
                              an error as a call to XMemAlloc could have occurred before the hooks were set up or a \
                              recursive call occurred to XMemAllocHook)" );
        }  

        XMemFreeDefault( lpAddress, dwAttributes );
    }
}

SIZE_T XMemGetTotalAllocatedMemory()
{
    return g_dwTotalAllocatedSize;
}

#endif
