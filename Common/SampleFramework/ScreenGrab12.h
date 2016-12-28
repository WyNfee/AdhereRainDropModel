//--------------------------------------------------------------------------------------
// File: ScreenGrab.h
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D 12 Render Target).
//
// Note these functions are useful as a light-weight runtime screen grabber. For
// full-featured texture capture, DDS writer, and texture processing pipeline,
// see the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma once
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d12_x.h>
#else
#include <d3d12.h>
#endif

#include <ocidl.h>
#include <stdint.h>
#include <functional>

namespace DirectX
{
    HRESULT SaveDDSTextureToFile( _In_ XSF::D3DDevice* const pDevice,
                                  _In_ XSF::D3DCommandQueue* const pCommandQueue,
                                  _In_ ID3D12Resource* const pSource,
                                  _In_z_ LPCWSTR fileName );

    HRESULT SaveWICTextureToFile( _In_ XSF::D3DDevice* const pDevice,
                                  _In_ XSF::D3DCommandQueue* const pCommandQueue,
                                  _In_ ID3D12Resource* const pSource,
                                  _In_ REFGUID guidContainerFormat, 
                                  _In_z_ LPCWSTR fileName,
                                  _In_opt_ const GUID* targetFormat = nullptr,
                                  _In_opt_ std::function<void(IPropertyBag2*)> setCustomProps = nullptr );

}