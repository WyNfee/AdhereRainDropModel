//----------------------------------------------------------------------------------------------------------------------
// Help.h
// 
// Used by samples to display on-screen help when the back button is pressed.
//
// Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#ifndef XSF_HELP_H_INCLUDED
#define XSF_HELP_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  please include SampleFramework.h before this file
#endif

#include "Draw.h"
#include "BitmapFont.h"
#include "Input.h"

namespace XSF = XboxSampleFramework;

namespace XboxSampleFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Forward declarations.

    const COLORREF HELP_TITLE_COLOR = 0xFFFFFFFF;
    const COLORREF HELP_DESCRIPTION_COLOR = 0xFFFFFFFF;
    const COLORREF HELP_CALLOUT_COLOR = 0xFFFFFFFF;
    const COLORREF HELP_CALLOUT_CONTAINER_COLOR = 0xFF00FF00;
    const COLORREF HELP_TRANSPARENT_COLOR = 0;

    //------------------------------------------------------------------------------------------------------------------
    // Name: enum class HelpID
    // Desc: The text callouts shown by the help system.
    //------------------------------------------------------------------------------------------------------------------
    enum class HelpID
    {
        TITLE_TEXT = 0,        // Title for the help screen
        DESCRIPTION_TEXT,      // Description for the help screen
        LEFT_STICK,
        RIGHT_STICK,
        DPAD_UP,
        DPAD_DOWN,
        DPAD_LEFT,
        DPAD_RIGHT,
        DPAD_ALL,
        RIGHT_SHOULDER,
        RIGHT_TRIGGER,
        LEFT_SHOULDER,
        LEFT_TRIGGER,
        A_BUTTON,
        B_BUTTON,
        X_BUTTON,
        Y_BUTTON,
        START_BUTTON, // Legacy button; do not use. Use Menu instead.
		MENU_BUTTON,
        BACK_BUTTON, // Legacy button; do not use. Use View instead.
		VIEW_BUTTON,
        MAX_COUNT
    };

    //----------------------------------------------------------------------------------------------------------------------
    // Name: enum class HelpFont
    // Desc: The font to use for this item.
    //----------------------------------------------------------------------------------------------------------------------
    enum class HelpFont
    {
        UseDefaultTitleFont,
        UseDefaultCallOutFont,
        UseDefaultDescriptionFont,
        UseCustomFont
    };

    //----------------------------------------------------------------------------------------------------------------------
    // Name: class HelpButtonAssignment
    // Desc: Mapping between a help button and the text describing it.
    //----------------------------------------------------------------------------------------------------------------------
    struct HelpButtonAssignment
    {
        HelpID id;
        LPCWSTR buttonText;
    };

    //----------------------------------------------------------------------------------------------------------------------
    // Name: class HelpScreen
    // Desc: A more customizable way of creating a help screen.
    //----------------------------------------------------------------------------------------------------------------------
    struct HelpScreen
    {
        COLORREF titleColor;                 // title text color
        COLORREF descriptionColor;           // body text color
        COLORREF calloutColor;               // Callout text color
        COLORREF calloutBgColor;             // Callout background color
        HelpFont titleFontType;              // Defaults or custom font
        BitmapFont* pTitleFont;              // The font used for the title.
        HelpFont descriptionFontType;        // Defaults or custom font
        BitmapFont* pDescriptionTextFont;    // The font used for the body.
        HelpFont calloutFontType;            // Defaults or custom font
        BitmapFont* pCalloutFont;            // The font used for callouts.
        LPCWSTR titleText;                   // The title text.
        LPCWSTR descriptionText;             // The body text.
        const HelpButtonAssignment * buttonAssignments; // Button assignment callout array
        int buttonCount;                     // The number of buttons.
    };

    //----------------------------------------------------------------------------------------------------------------------
    // Name: Help
    // Desc: An instance of the help system.
    //----------------------------------------------------------------------------------------------------------------------
    class Help 
    {
        struct CalloutBox;

        // Member variables
        SIZE m_DisplayBounds;
        XMFLOAT2 m_DisplayScaleFactor;
        ID3D11ShaderResourceView* m_pBackground;
        BOOL m_IsVisible;
        BOOL m_WantsInput;
        BOOL m_IsAnimating;
        BOOL m_IsInitialized;
        BOOL m_IgnoreViewButtonThisFrame;
        CalloutBox* m_pCalloutBoxes;
        int m_CalloutCount;
        Draw* m_pCanvas;
        
        static BitmapFont* ms_DefaultTitleFont;
        static BitmapFont* ms_DefaultDescriptionFont;
        static BitmapFont* ms_DefaultCalloutFont;

    public:

        Help();
        ~Help();

        //----------------------------------------------------------------------------------------------------------------------
        // Name: IsInitialized
        // Desc: Returns TRUE if the help system has been initialized.
        //----------------------------------------------------------------------------------------------------------------------
        BOOL IsInitialized() const
        {
            return m_IsInitialized;
        }

        //----------------------------------------------------------------------------------------------------------------------
        // Name: IsVisible
        // Desc: Returns TRUE if the help system is visible, and wants to be rendered.
        //----------------------------------------------------------------------------------------------------------------------
        BOOL IsVisible() const
        {
            return m_IsVisible;
        }

        //----------------------------------------------------------------------------------------------------------------------
        // Name: HasInputFocus
        // Desc: Returns TRUE if the help screen should be stealing user input instead of the sample.
        //----------------------------------------------------------------------------------------------------------------------
        BOOL HasInputFocus() const
        {
            return m_WantsInput;
        }

        void Initialize( const SampleFramework& sampleFramework );
        void Initialize( D3DDevice* pDevice, UINT frameWidth, UINT frameHeight );
        void Shutdown();

        void BuildHelpScreen( const HelpScreen& helpText );
        void SetHelpText( _In_z_ LPCWSTR pTitle, _In_z_ LPCWSTR pDescription,
            _In_count_(buttonCount) const HelpButtonAssignment * pButtons, int buttonCount );

        // Used to hide/show the help screen.
        void Show();
        void Hide();

        // Call these if IsVisible is true.
        BOOL Update( const XSF::GamepadReading& input, FLOAT fDeltaTime );      // returns TRUE is IsVisible
        void Render( _In_ D3DDeviceContext* pCtx, _In_opt_ const D3D11_VIEWPORT* pViewport = nullptr, _In_opt_ BOOL fastSEmanticsEnabled = FALSE );

    private:
        BitmapFont* MapFont( HelpFont fontType ) const;
        void CalcDisplayScaling();
        void LoadFonts( _In_ D3DDevice* pDevice );
        void DestroyFonts();
        void LayoutCallouts();
        void MeasureCallouts();
        void DestroyCallouts();
    };

} // namespace XboxSampleFramework

#endif //XSF_HELP_H_INCLUDED
