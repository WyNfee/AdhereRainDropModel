//--------------------------------------------------------------------------------------
// ViewProvider.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once


//--------------------------------------------------------------------------------------
// Name: ViewProviderFactory 
// Desc: This class produces instances of ViewProvider
//--------------------------------------------------------------------------------------
ref class ViewProviderFactory sealed : Windows::ApplicationModel::Core::IFrameworkViewSource 
{
public:
    ViewProviderFactory( UINT_PTR renderer ) : m_renderer( renderer ) {}
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();

private:
    UINT_PTR m_renderer;
};
