//--------------------------------------------------------------------------------------
// ParameterManager.h
//
// Helper class for managing controller adjustment of multiple float or int parameters.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_PARAMETER_MANAGER_H
#define XSF_PARAMETER_MANAGER_H

namespace XboxSampleFramework
{

    //--------------------------------------------------------------------------------------
    // Parameter manager class.
    // Manages controller input for adjusting multiple float or int parameters used in samples.
    // Also manages rendering of parameter names and values to screen.
    //--------------------------------------------------------------------------------------

    class ParameterManager
    {
    public:

        //--------------------------------------------------------------------------------------
        // Struct containing the definition for float parameters.
        //--------------------------------------------------------------------------------------

        struct FloatParameter
        {
            float* pValue;      // Pointer to actual used parameter (this is adjusted by this class)
            float  Default;     // Default value for this parameter.
            float  Min;         // Minimum allowed value for this parameter.
            float  Max;         // Maximum allowed value for this parameter.
            float  Step;        // Adjustment speed for this parameter.
            WCHAR* Name;        // Display name for this parameter.
        };

        //--------------------------------------------------------------------------------------
        // Struct containing the definition for integer parameters.
        //--------------------------------------------------------------------------------------

        struct IntParameter
        {
            int* pValue;        // Pointer to actual used parameter.
            int  Default;       // Default value for this parameter.
            int  Min;           // Minimum allowed value for this parameter.
            int  Max;           // Maximum allowed value for this parameter.
            int  Step;          // Adjustment step-size for this parameter (usually 1...)
            WCHAR* Name;        // Display name for this parameter.
        };

        //--------------------------------------------------------------------------------------
        // Constructor
        //--------------------------------------------------------------------------------------

        ParameterManager()
            : m_SelectedIndex( 0 )
        {
        }
    
        //--------------------------------------------------------------------------------------
        // Name: Init
        // Desc: Initialize the data structures used to manage the parameters. Passing in 0
        //       for the ResetButton parameter disables the reset functionality of this class.
        //       The client code can still call Reset directly to reset parameters though.
        //--------------------------------------------------------------------------------------

        void Init( FloatParameter * pFloatParams, int FloatParamCount, IntParameter * pIntParams, int IntParamCount/*, GamepadButtons ResetButton = GamepadButtons::B*/ )
        {
            m_pFloats = new FloatParameter[ FloatParamCount ];
            m_pInts = new IntParameter[ IntParamCount ];

            memcpy( m_pFloats, pFloatParams, sizeof( FloatParameter ) * FloatParamCount );
            memcpy( m_pInts, pIntParams, sizeof( IntParameter ) * IntParamCount );

            m_FloatCount = FloatParamCount;
            m_IntCount = IntParamCount;

//            m_ResetButton = ResetButton;

            Reset();
        }

        //--------------------------------------------------------------------------------------
        // Name: Reset
        // Desc: Reset all controlled parameters to default value.
        //--------------------------------------------------------------------------------------

        void Reset()
        {
            // Reset float parameters...
            for( int i = 0; i < m_FloatCount; ++i )
            {
                *( m_pFloats[ i ].pValue ) = m_pFloats[ i ].Default;
            }

            // Reset int parameters...
            for( int i = 0; i < m_IntCount; ++i )
            {
                *( m_pInts[ i ].pValue ) = m_pInts[ i ].Default;
            }

            m_SelectedIndex = 0;
        }

        //--------------------------------------------------------------------------------------
        // Name: Update
        // Desc: Called from sample update method, parses control input and adjusts parameters
        //       based on defined metrics. Returns true if the parameters have changed (or 
        //       are changing).
        //--------------------------------------------------------------------------------------

        bool Update( const XSF::GamepadReading& input, float timeDelta )
        {
            if( input.IsBPressed() )// m_ResetButton != GamepadButtons::None && ( input.PressedButtons & (UINT)m_ResetButton ) != 0 )
            {
                Reset();
                return true;
            }

            bool Change = false;
            if( input.WasDPadDownPressed() )
            {
                m_SelectedIndex += 1;
                if( m_SelectedIndex >= m_FloatCount + m_IntCount )
                {
                    m_SelectedIndex = 0;
                }
            }

            if( input.WasDPadUpPressed() )
            {
                m_SelectedIndex -= 1;
                if( m_SelectedIndex < 0 )
                {
                    m_SelectedIndex = m_FloatCount + m_IntCount - 1;
                }
            }

            // For integer values, the buffers increment or decrement.
            if( m_SelectedIndex >= m_FloatCount )
            {
                int index = m_SelectedIndex - m_FloatCount;
                if( input.WasRightShoulderPressed() )
                {
                    *(m_pInts[ index ].pValue) = std::min( *(m_pInts[ index ].pValue) + m_pInts[ index ].Step, m_pInts[ index ].Max );
                    Change = true;
                }
                else if( input.WasLeftShoulderPressed() )
                {
                    *(m_pInts[ index ].pValue) = std::max( *(m_pInts[ index ].pValue) - m_pInts[ index ].Step, m_pInts[ index ].Min );
                    Change = true;
                }
            }
            else
            {
                if( input.RightTrigger() > 0.0f )
                {
                    float deltaUp = input.RightTrigger() * 255.0f * timeDelta * m_pFloats[ m_SelectedIndex ].Step;
                    *m_pFloats[ m_SelectedIndex ].pValue = std::min( *( m_pFloats[ m_SelectedIndex ].pValue ) + deltaUp, m_pFloats[ m_SelectedIndex ].Max );
                    Change = true;
                }
                else if( input.LeftTrigger() > 0.0f )
                {
                    float deltaDown = input.LeftTrigger() * 255.0f * timeDelta * m_pFloats[ m_SelectedIndex ].Step;
                    *m_pFloats[ m_SelectedIndex ].pValue = std::max( *( m_pFloats[ m_SelectedIndex ].pValue ) - deltaDown, m_pFloats[ m_SelectedIndex ].Min );
                    Change = true;
                }
            }

            return Change;
        }

        //--------------------------------------------------------------------------------------
        // Name: Render
        // Desc: Display UI text showing the parameter values. Also, highlight the selected 
        //       parameter in yellow.
        //--------------------------------------------------------------------------------------

        void Render( XSF::BitmapFont & font, float x, float yStart, float yStep, bool DisplayPrompt = true )
        {
            // Render the "instructions" prompt.
            if( DisplayPrompt )
            {
                font.DrawText( x, yStart, 0xffffffff, L"Select the parameter to adjust using DPAD up/down." );
                yStart += yStep;
                // Inset parameters slightly.
                x += 10;
            }

            for( int i = 0; i < m_FloatCount; ++i )
            {
                DWORD Color = m_SelectedIndex == i ? 0xffffff00 : 0xffffffff;
                font.DrawTextF( x, yStart, Color, 0, 1, L"%s: %f", m_pFloats[ i ].Name, *m_pFloats[ i ].pValue );
                yStart += yStep;
            }

            for( int i = 0; i < m_IntCount; ++i )
            {
                DWORD Color = m_SelectedIndex == ( i + m_FloatCount ) ? 0xffffff00 : 0xffffffff;
                font.DrawTextF( x, yStart, Color, 0, 1, L"%s: %d", m_pInts[ i ].Name, *m_pInts[ i ].pValue );
                yStart += yStep;
            }
        }

    private:
        FloatParameter* m_pFloats;
        IntParameter*   m_pInts;
        int             m_FloatCount;
        int             m_IntCount;
        int             m_SelectedIndex;
    };

}   // namespace XboxSampleFramework

#endif
