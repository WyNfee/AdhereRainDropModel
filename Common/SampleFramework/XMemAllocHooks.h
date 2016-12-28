//--------------------------------------------------------------------------------------
// XMemAllocHooks.h
//
// Declares functions that keep track of memory allocations for debugging and
// can be used as XMem hooks
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_XMEM_ALLOC_HOOKS_H
#define XSF_XMEM_ALLOC_HOOKS_H

#ifdef _XBOX_ONE

#include "SampleFramework.h"

namespace XboxSampleFramework
{
    PVOID WINAPI XMemAllocHook( _In_ SIZE_T dwSize, _In_ ULONGLONG dwAttributes );

    void WINAPI XMemFreeHook( _In_ PVOID lpAddress, _In_ ULONGLONG dwAttributes );

    SIZE_T XMemGetTotalAllocatedMemory();
}

#endif

#endif