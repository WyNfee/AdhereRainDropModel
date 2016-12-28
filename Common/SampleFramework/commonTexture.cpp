//--------------------------------------------------------------------------------------
// CommonTexture.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "DDSTextureLoader.h"

//--------------------------------------------------------------------------------------
// Name: CreateTextureFromFile
// Desc: Loads texture data and creates a texture from a file
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT XSF::CreateTextureFromFile( D3DDevice* pDevice, const WCHAR* pFilename, ID3D11ShaderResourceView** ppSRV )
{
    std::vector< BYTE > file;
    HRESULT hr = XSF::LoadBlob( pFilename, file );
    if( SUCCEEDED( hr ) )
    {
        hr = CreateDDSTextureFromMemory( pDevice, &file[ 0 ], file.size(), NULL, ppSRV, 0 );
    }

    return hr;
}

_Use_decl_annotations_
HRESULT XSF::CreateTextureFromFile( D3DDevice* pDevice, D3DDeviceContext* pDeviceContext, const WCHAR* pFilename, ID3D11ShaderResourceView** ppSRV )
{
    std::vector< BYTE > file;
    HRESULT hr = XSF::LoadBlob( pFilename, file );
    if( SUCCEEDED( hr ) )
    {
        hr = CreateDDSTextureFromMemory( pDevice, pDeviceContext, &file[ 0 ], file.size(), NULL, ppSRV, 0 );
    }

    return hr;
}
