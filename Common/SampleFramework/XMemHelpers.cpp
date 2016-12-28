//--------------------------------------------------------------------------------------
// XMemHelpers.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "XMemHelpers.h"

#if defined(_XBOX_ONE) && defined(_TITLE)

#include <xmem.h>

namespace XboxSampleFramework
{
    //----------------------------------------------------------------------------
    // XMem helpers
    //----------------------------------------------------------------------------
    static __forceinline XALLOC_ATTRIBUTES AsAttributes( ULONGLONG i )
    {
        XALLOC_ATTRIBUTES d = {i};
        return d; 
    }

    // Returns the page size in bytes
    MEM_PAGE_SIZE GetXMemPageSize( 
        _In_ ULONGLONG qwAttrs )
    {
        switch ( AsAttributes( qwAttrs ).s.dwPageSize )
        {
        case XALLOC_PAGESIZE_4KB: return MEM_PAGE_SIZE_4KB;
        case XALLOC_PAGESIZE_64KB: return MEM_PAGE_SIZE_64KB;
        case XALLOC_PAGESIZE_4MB: return MEM_PAGE_SIZE_4MB;
        default: return MEM_PAGE_SIZE_INVALID;
        }
    }

    // Returns the page size in as MEM_* flags (for use below)
    // (Internal only: used by GetXMemAllocationType)
    static DWORD GetMemPageSizeFlags( 
        _In_ XALLOC_ATTRIBUTES qwAttrs )
    {
        switch ( qwAttrs.s.dwPageSize )
        {
        case XALLOC_PAGESIZE_64KB: return MEM_LARGE_PAGES;
        case XALLOC_PAGESIZE_4MB: return MEM_4MB_PAGES;
        case XALLOC_PAGESIZE_4KB: return 0;
        default: return 0;
        }
    }

    // Returns the allocation type (for use with VirtualAlloc and similar)
    // (Internal only: used by GetXMemAllocationType)
    static DWORD GetMemTypeFlags(
        _In_ XALLOC_ATTRIBUTES qwAttrs )
    {
        switch ( qwAttrs.s.dwMemoryType )
        {
        case XALLOC_MEMTYPE_HEAP_CACHEABLE: 
            return 0;

        case XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE: 
        case XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE_GPU_READONLY:
        case XALLOC_MEMTYPE_GRAPHICS_CACHEABLE:
        case XALLOC_MEMTYPE_GRAPHICS_CACHEABLE_NONCOHERENT_GPU_READONLY:
        case XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_WRITECOMBINE:
        case XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_CACHEABLE:
            return MEM_GRAPHICS;

        case XALLOC_MEMTYPE_PHYSICAL_CACHEABLE:
        case XALLOC_MEMTYPE_PHYSICAL_WRITECOMBINE:
        case XALLOC_MEMTYPE_PHYSICAL_UNCACHED: 
            return MEM_PHYSICAL;

        default:
            return 0;
        };
    };

    // Returns the allocation type (for use with VirtualAlloc and similar)
    ULONG GetXMemAllocationType(
        _In_ ULONGLONG qwAttrs )
    {
        return GetMemTypeFlags( AsAttributes( qwAttrs ) )
             | GetMemPageSizeFlags( AsAttributes( qwAttrs ) );
    }

    // Returns the page protection flags (for use with VirtualAlloc and similar)
    ULONG GetXMemPageProtectFlags( 
        _In_ ULONGLONG qwAttrs )
    {
        switch ( AsAttributes( qwAttrs ).s.dwMemoryType )
        {
        case XALLOC_MEMTYPE_HEAP_CACHEABLE: 
            return PAGE_READWRITE;
        case XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE: 
            return PAGE_READWRITE | PAGE_WRITECOMBINE;
        case XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE_GPU_READONLY:
            return PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GPU_READONLY;
        case XALLOC_MEMTYPE_GRAPHICS_CACHEABLE:
            return PAGE_READWRITE | PAGE_GPU_COHERENT;
        case XALLOC_MEMTYPE_GRAPHICS_CACHEABLE_NONCOHERENT_GPU_READONLY:
            return PAGE_READWRITE | PAGE_GPU_READONLY;
        case XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_WRITECOMBINE:
            return PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GPU_EXECUTE;
        case XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_CACHEABLE:
            return PAGE_READWRITE | PAGE_GPU_EXECUTE | PAGE_GPU_COHERENT;
        case XALLOC_MEMTYPE_PHYSICAL_CACHEABLE:
            return PAGE_READWRITE;
        case XALLOC_MEMTYPE_PHYSICAL_WRITECOMBINE:
            return PAGE_READWRITE | PAGE_WRITECOMBINE;
        case XALLOC_MEMTYPE_PHYSICAL_UNCACHED: 
            return PAGE_READWRITE | PAGE_NOCACHE;

        default: 
            return 0;
        };
    }
}

#endif