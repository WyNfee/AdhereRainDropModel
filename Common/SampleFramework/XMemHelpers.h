//--------------------------------------------------------------------------------------
// XMemHelpers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once
#ifndef XSF_XMEM_HELPERS_H
#define XSF_XMEM_HELPERS_H

#if defined(_XBOX_ONE) && defined(_TITLE)

namespace XboxSampleFramework
{
    enum MEM_PAGE_SIZE
    {
        MEM_PAGE_SIZE_INVALID = 0,
        MEM_PAGE_SIZE_4KB = 4*1024,
        MEM_PAGE_SIZE_64KB = 64*1024,
        MEM_PAGE_SIZE_4MB = 4*1024*1024
    };

    // Returns the page size in bytes
    MEM_PAGE_SIZE GetXMemPageSize( 
        _In_ ULONGLONG qwAttrs );

    // Returns the allocation type and page size 
    // (MEM_* flags for use with VirtualAlloc and similar)
    ULONG GetXMemAllocationType(
        _In_ ULONGLONG qwAttrs );

    // Returns the page protection flags 
    // (PAGE_* flags for use with VirtualAlloc and similar)
    ULONG GetXMemPageProtectFlags( 
        _In_ ULONGLONG qwAttrs );
}

#endif      // _XBOX_ONE

#endif      // XSF_XMEM_HELPERS_H
