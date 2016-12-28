//--------------------------------------------------------------------------------------
// Input.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_INPUT_H_INCLUDED
#define XSF_INPUT_H_INCLUDED


#ifndef XSF_H_INCLUDED
#error  please include SampleFramework.h before this file
#endif

#ifdef _XBOX_ONE
#include "InputXboxOne.h"
#else
#include "InputPC.h"
#endif

#endif // XSF_INPUT_H_INCLUDED
