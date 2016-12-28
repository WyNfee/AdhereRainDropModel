//--------------------------------------------------------------------------------------
// ViewProvider.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#pragma warning( disable : 4702 )

#include "ViewProvider.h"
#include "SampleFramework.h"
#include "Common.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Activation;

//--------------------------------------------------------------------------------------
// Name: ViewProvider
// Desc: This class acts as interface between the game and the OS
//--------------------------------------------------------------------------------------
ref class ViewProvider sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
    ViewProvider( _In_ UINT_PTR renderer );

    virtual void Initialize( _In_ Windows::ApplicationModel::Core::CoreApplicationView^ applicationView );
    virtual void SetWindow( _In_ Windows::UI::Core::CoreWindow^ window );
    virtual void Load( _In_ Platform::String^ entryPoint );
    virtual void Run();
    virtual void Uninitialize();

private:

    void OnCharacterReceived( _In_ CoreWindow^ window, _In_ CharacterReceivedEventArgs^ characterReceivedEventArgs );
    void OnKeyDown( _In_ CoreWindow^ window, _In_ KeyEventArgs^ keyEventArgs );
    void OnKeyUp( _In_ CoreWindow^ window, _In_ KeyEventArgs^ keyEventArgs );
    void OnSizeChanged( _In_ CoreWindow^ window, _In_ WindowSizeChangedEventArgs^ args );
    void OnVisibilityChanged( _In_ CoreWindow^ window, _In_ VisibilityChangedEventArgs^ args );
    void OnActivated( _In_ Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, _In_ Windows::ApplicationModel::Activation::IActivatedEventArgs^ args );
    void OnResuming( _In_ Platform::Object^ sender, _In_ Platform::Object^ args );
    void OnSuspending( _In_ Platform::Object^ sender, _In_ Windows::ApplicationModel::SuspendingEventArgs^ args );
#if defined(_XBOX_ONE) && defined(_TITLE)
    void OnResourceAvailabilityChanged( _In_ Platform::Object^ sender, _In_ Platform::Object^ args );
#endif
    void RunMainLoop();

    UINT_PTR m_renderer;
};

//--------------------------------------------------------------------------------------
// Name: ViewProvider
// Desc: Constructor of ViewProvider class
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
ViewProvider::ViewProvider( UINT_PTR renderer ) : m_renderer( renderer ) 
{
}

//--------------------------------------------------------------------------------------
// Name: ViewProvider
// Desc: Initializes the view
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::Initialize( Windows::ApplicationModel::Core::CoreApplicationView^ applicationView )
{
    applicationView->Activated += ref new Windows::Foundation::TypedEventHandler< CoreApplicationView^, IActivatedEventArgs^ >( this, &ViewProvider::OnActivated );
    CoreApplication::Suspending += ref new Windows::Foundation::EventHandler< Windows::ApplicationModel::SuspendingEventArgs^ >( this, &ViewProvider::OnSuspending );
    CoreApplication::Resuming += ref new Windows::Foundation::EventHandler< Platform::Object^>( this, & ViewProvider::OnResuming );
#if defined(_XBOX_ONE) && defined(_TITLE)
    CoreApplication::ResourceAvailabilityChanged += ref new Windows::Foundation::EventHandler< Platform::Object^>( this, &ViewProvider::OnResourceAvailabilityChanged );
#endif
}

//--------------------------------------------------------------------------------------
// Name: OnActivated
// Desc: Called when the application is activated.  For now, there is just one activation on launch
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnActivated( CoreApplicationView^ /*applicationView*/, IActivatedEventArgs^ args )
{
    CoreWindow::GetForCurrentThread()->Activate();

    XSF::DebugPrint( "Activation Kind: %S (%d)\n", args->Kind.ToString()->Data(), args->Kind );
    if( args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Launch )
    {
        ILaunchActivatedEventArgs^ launchArgs = (ILaunchActivatedEventArgs^) args;
        const wchar_t* pwszArguments = launchArgs->Arguments->Data();
        XSF::DebugPrint( "Activation Arguments: %S\n", pwszArguments );
        reinterpret_cast< SampleFramework* >( m_renderer )->ParseCommandLine( pwszArguments );
    }
    else if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Protocol)
    {
        IProtocolActivatedEventArgs^ protocolArgs = static_cast< IProtocolActivatedEventArgs^>(args);
        XSF::DebugPrint( "Activation URL: %S\n", protocolArgs->Uri->RawUri->ToString()->Data() );
#if defined(_XBOX_ONE) && defined(_TITLE)
        reinterpret_cast< SampleFramework* >( m_renderer )->OnProtocolActivation( protocolArgs );
#endif
    }

    // Query this to find out if the application was shut down gracefully last time
    Windows::ApplicationModel::Activation::ApplicationExecutionState lastState = args->PreviousExecutionState;
    XSF::DebugPrint( "Activation PreviousExecutionState: %d\n", lastState );
}

//--------------------------------------------------------------------------------------
// Name: OnResuming
// Desc: Called when the application is resuming
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnResuming( Platform::Object^ /*sender*/, Platform::Object^ /*args*/ )
{
    reinterpret_cast< SampleFramework* >( m_renderer )->FrameworkResume();
}

//--------------------------------------------------------------------------------------
// Name: OnSuspending
// Desc: Called when the application is suspending
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnSuspending( Platform::Object^ /*sender*/, Windows::ApplicationModel::SuspendingEventArgs^ /*args*/ )
{
    reinterpret_cast< SampleFramework* >( m_renderer )->FrameworkSuspend();
}

#if defined(_XBOX_ONE) && defined(_TITLE)
//--------------------------------------------------------------------------------------
// Name: OnResourceAvailabilityChanged
// Desc: Called when the resource avaiability changes (full, constrained, fullwithsystemreserve)
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnResourceAvailabilityChanged( Platform::Object^ sender, Platform::Object^ args )
{
    reinterpret_cast< SampleFramework* >( m_renderer )->OnResourceAvailabilityChanged( CoreApplication::ResourceAvailability );
}
#endif

//--------------------------------------------------------------------------------------
// Name: OnCharacterReceived
// Desc: Called when a key is pressed
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnCharacterReceived( CoreWindow^ window, CharacterReceivedEventArgs^ characterReceivedEventArgs )
{
}

//--------------------------------------------------------------------------------------
// Name: OnKeyDown
// Desc: Called when a key is pressed. 
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnKeyDown( CoreWindow^ window, KeyEventArgs^ keyEventArgs )
{
    if( static_cast< UINT >( keyEventArgs->VirtualKey ) < 256 )
    {
        reinterpret_cast< SampleFramework* >( m_renderer )->SetKeyDown( static_cast< BYTE >( keyEventArgs->VirtualKey ) );
    }
}

//--------------------------------------------------------------------------------------
// Name: OnKeyUp
// Desc: Called when a key is released
//
// [Note you currently must press F12 after boot on the alpha kit, in order for 
// keyboard events to be sent to the app.]
//
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::OnKeyUp( CoreWindow^ window, KeyEventArgs^ keyEventArgs )
{
    if( static_cast< UINT >( keyEventArgs->VirtualKey ) < 256 )
    {
        reinterpret_cast< SampleFramework* >( m_renderer )->SetKeyUp( static_cast< BYTE >( keyEventArgs->VirtualKey ) );
    }
}

//--------------------------------------------------------------------------------------
// Name: OnSizeChanged
// Desc: Event handler for size change
//--------------------------------------------------------------------------------------
void ViewProvider::OnSizeChanged( Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args )
{
    reinterpret_cast< SampleFramework* >( m_renderer )->Resize( static_cast< UINT >( args->Size.Width ), static_cast< UINT >( args->Size.Height ) );
}

//--------------------------------------------------------------------------------------
// Name: OnVisibilityChanged
// Desc: Event handler for visibility change
//--------------------------------------------------------------------------------------
void ViewProvider::OnVisibilityChanged( Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args )
{
    reinterpret_cast< SampleFramework* >( m_renderer )->OnVisibilityChanged( args->Visible );
}

//--------------------------------------------------------------------------------------
// Name: SetWindow
// Desc: We're given a window. We use CoreWindow::GetForCurrentThread() instead
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::SetWindow( Windows::UI::Core::CoreWindow^ window )
{
#if !(defined(_XBOX_ONE) && defined(_TITLE))
    const UINT width = static_cast<UINT>( window->Bounds.Right - window->Bounds.Left );
    const UINT height = static_cast<UINT>( window->Bounds.Bottom - window->Bounds.Top );
    reinterpret_cast< SampleFramework* >( m_renderer )->SetBackbufferSize( width, height );
#endif

    window->CharacterReceived += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>( this, &ViewProvider::OnCharacterReceived );
    window->KeyDown += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, KeyEventArgs^>( this, &ViewProvider::OnKeyDown );
    window->KeyUp += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, KeyEventArgs^>( this, &ViewProvider::OnKeyUp );
    window->SizeChanged += ref new Windows::Foundation::TypedEventHandler< CoreWindow^, WindowSizeChangedEventArgs^ >( this, &ViewProvider::OnSizeChanged );
    window->VisibilityChanged += ref new Windows::Foundation::TypedEventHandler< CoreWindow^, VisibilityChangedEventArgs^ >(this, &ViewProvider::OnVisibilityChanged );
}

//--------------------------------------------------------------------------------------
// Name: Load
// Desc: This method is called after Initialize
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void ViewProvider::Load( Platform::String^ )
{
}

//--------------------------------------------------------------------------------------
// Name: Run
// Desc: This is our main loop. This method is called after Load
//--------------------------------------------------------------------------------------
void ViewProvider::Run()
{
#if defined (_TITLE)
    __try
    {
#endif
        XSF::CrossCheckpoint( XSF::CP_BEFORE_D3D_INIT );

#if defined(ATG_PROFILE) || defined(ATG_PROFILE_VERBOSE)
        XSF::ATGProfiler::Initialize();
        XSF::ATGProfiler::StartCapture();
#endif
        // The actual main loop is relegated to RunMainLoop because SEH can not be used in functions which require object unwinding
        RunMainLoop();
#if defined (_TITLE)
    }
    __except (XSF::ExceptionHandler(GetExceptionInformation()))
    {
        XSF::DebugPrint(L"XSF ExceptionHandler exited\n");
    }
#endif
}

//--------------------------------------------------------------------------------------
// Name: RunMainLoop
// Desc: Main loop implementation
//--------------------------------------------------------------------------------------
void ViewProvider::RunMainLoop()
{
    SampleFramework* p = reinterpret_cast< SampleFramework* >( m_renderer );
    p->ObtainSampleSettings();
    p->CreateDeviceResources();
    p->CreateWindowSizeDependentResources( reinterpret_cast< IUnknown* >( CoreWindow::GetForCurrentThread() ) );
    p->FrameworkInitialize();

    for( ;; )
    {
        CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );

        if( !p->FrameworkUpdateAndRender() )
        {
            p->FrameworkShutdown();
            Windows::ApplicationModel::Core::CoreApplication::Exit();
            break;
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: Uninitialize
// Desc: 
//--------------------------------------------------------------------------------------
void ViewProvider::Uninitialize()
{
}

//--------------------------------------------------------------------------------------
// Name: ViewProviderFactory 
// Desc: This class produces instances of ViewProvider
//--------------------------------------------------------------------------------------
Windows::ApplicationModel::Core::IFrameworkView^ ViewProviderFactory::CreateView()
{
    return ref new ViewProvider( m_renderer );
}
