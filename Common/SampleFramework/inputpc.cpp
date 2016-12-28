//--------------------------------------------------------------------------------------
// Input.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"


//--------------------------------------------------------------------------------------
SHORT XSF::GamepadReading::LEFT_THUMB_DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
SHORT XSF::GamepadReading::RIGHT_THUMB_DEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

static const UINT DEFAULT_CHECK_COUNTDOWN = 200;

//--------------------------------------------------------------------------------------
// Name: ConvertThumbstickValue()
// Desc: Converts SHORT thumbstick values to FLOAT, while enforcing a deadzone
//--------------------------------------------------------------------------------------
inline static
FLOAT ConvertThumbstickValue( SHORT sThumbstickValue, SHORT sDeadZone )
{
    if( sThumbstickValue > +sDeadZone )
    {
        return (sThumbstickValue-sDeadZone) / (32767.0f-sDeadZone);
    }
    if( sThumbstickValue < -sDeadZone )
    {
        return (sThumbstickValue+sDeadZone+1.0f) / (32767.0f-sDeadZone);
    }
    return 0.0f;
}

//--------------------------------------------------------------------------------------
// Name: ClampToShort()
// Desc: Takes an integer input and clamps it to fit in a short, to avoid wrapping.
//--------------------------------------------------------------------------------------
inline static
SHORT ClampToShort(INT input)
{
    // Use min/max from windef.h. Define NOMINMAX if you don't want these
    // macros defined.
    input = std::min< INT >( input, SHRT_MAX );
    input = std::max< INT >( input, SHRT_MIN );
    return (SHORT)input;
}



//--------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes the gamepad input system
//--------------------------------------------------------------------------------------
HRESULT XSF::Input::Initialize()
{
    ZeroMemory( m_Gamepads, sizeof( m_Gamepads ) );
    ZeroMemory( &m_mergedPad, sizeof( m_mergedPad ) );

    // Stagger the updates
    for( UINT i=0; i < XUSER_MAX_COUNT; ++i )
    {
        m_Gamepads[ i ].nextCheckCountdown = static_cast< WORD >( i * DEFAULT_CHECK_COUNTDOWN / XUSER_MAX_COUNT );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: GetInput()
// Desc: Processes input from the gamepads
//--------------------------------------------------------------------------------------
VOID XSF::Input::GetInput( GamepadReading* pGamepads )
{
    // If the user did not specify a list of gamepads, use the global list
    if( NULL == pGamepads )
        pGamepads = m_Gamepads;

    // Loop through all gamepads
    for( DWORD i=0; i < XUSER_MAX_COUNT; i++ )
    {
        // Don't query disconnected controllers each frame
        if( !pGamepads[ i ].bConnected &&
            pGamepads[ i ].nextCheckCountdown > 0 )
        {
            --pGamepads[ i ].nextCheckCountdown;
            continue;
        }

        // Read the input state
        XINPUT_STATE InputState;
        BOOL bWasConnected = pGamepads[i].bConnected;
        pGamepads[i].bConnected = ( XInputGetState( i, &InputState ) == ERROR_SUCCESS ) ? true : false;

        // Track insertion and removals
        pGamepads[i].bRemoved  = (  bWasConnected && !pGamepads[i].bConnected ) ? true : false;
        pGamepads[i].bInserted = ( !bWasConnected &&  pGamepads[i].bConnected ) ? true : false;

        if( !pGamepads[i].bConnected )
        {
            pGamepads[ i ].nextCheckCountdown = DEFAULT_CHECK_COUNTDOWN;
            continue;
        }

        // Store the capabilities of the device
        if( pGamepads[i].bInserted )
        {
            ZeroMemory( &pGamepads[i], sizeof(m_Gamepads[i]) );
            pGamepads[i].bConnected = true;
            pGamepads[i].bInserted  = true;
            XInputGetCapabilities( i, XINPUT_FLAG_GAMEPAD, &pGamepads[i].caps );
        }

        // Copy GamepadReading to local structure
        memcpy( &pGamepads[i], &InputState.Gamepad, sizeof(XINPUT_GAMEPAD) );

        // Put Xbox device input for the GamepadReading into our custom format
        pGamepads[i].fX1 = ConvertThumbstickValue( pGamepads[i].sThumbLX, GamepadReading::LEFT_THUMB_DEADZONE );
        pGamepads[i].fY1 = ConvertThumbstickValue( pGamepads[i].sThumbLY, GamepadReading::LEFT_THUMB_DEADZONE );
        pGamepads[i].fX2 = ConvertThumbstickValue( pGamepads[i].sThumbRX, GamepadReading::RIGHT_THUMB_DEADZONE );
        pGamepads[i].fY2 = ConvertThumbstickValue( pGamepads[i].sThumbRY, GamepadReading::RIGHT_THUMB_DEADZONE );

        // Get the boolean buttons that have been pressed or released since the last
        // call. Each button is represented by one bit.
        pGamepads[i].wPressedButtons  = ( pGamepads[i].wLastButtons ^ pGamepads[i].wButtons ) & pGamepads[i].wButtons;
        pGamepads[i].wReleasedButtons = ( pGamepads[i].wLastButtons ^ pGamepads[i].wButtons ) & ~pGamepads[i].wButtons;
        pGamepads[i].wLastButtons     = pGamepads[i].wButtons;

        // Figure out if the left trigger has been pressed or released
        BOOL bPressed = ( pGamepads[i].bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD );
        if( bPressed )
        {
            pGamepads[i].bPressedLeftTrigger  = !pGamepads[i].bLastLeftTrigger;
            pGamepads[i].bReleasedLeftTrigger = FALSE;
        }
        else
        {
            pGamepads[i].bPressedLeftTrigger  = FALSE;
            pGamepads[i].bReleasedLeftTrigger = pGamepads[i].bLastLeftTrigger;
        }

        // Store the state for next time
        pGamepads[i].bLastLeftTrigger = bPressed;

        // Figure out if the right trigger has been pressed or released
        bPressed = ( pGamepads[i].bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

        if( bPressed )
        {
            pGamepads[i].bPressedRightTrigger  = !pGamepads[i].bLastRightTrigger;
            pGamepads[i].bReleasedRightTrigger = FALSE;
        }
        else
        {
            pGamepads[i].bPressedRightTrigger  = FALSE;
            pGamepads[i].bReleasedRightTrigger = pGamepads[i].bLastRightTrigger;
        }

        // Store the state for next time
        pGamepads[i].bLastRightTrigger = bPressed;

        // Set the user index for this GamepadReading
        pGamepads[i].dwUserIndex = i;
    }
}


//--------------------------------------------------------------------------------------
// Name: GetMergedInput()
// Desc: Processes input from all gamepads and merge it into one input. This is done for
//       expediency in samples and is not typically useful or advised for games.
//       If the pdwActiveGamePadsMask is non-NULL, then returns active gamepads.
//       
//--------------------------------------------------------------------------------------
VOID XSF::Input::GetMergedInput( GamepadReading& destPad, DWORD dwMask, DWORD* pdwActiveGamePadsMask )
{
    // Get input for ALL the gamepads
    GetInput();

    // Sum input across ALL gamepads into one default structure.
    ZeroMemory( &destPad, sizeof( destPad ) );
    INT  iThumbLX = 0;
    INT  iThumbLY = 0;
    INT  iThumbRX = 0;
    INT  iThumbRY = 0;
    BOOL bActiveThumbs[XUSER_MAX_COUNT]  = { 0 };
    BOOL bActiveButtons[XUSER_MAX_COUNT] = { 0 };

    for( DWORD i=0; i < XUSER_MAX_COUNT; i++ )
    {
        if( m_Gamepads[i].bConnected && ( !dwMask || ( dwMask & ( 1 << i ) ) ) )
        {
            destPad.bConnected = true;

            SHORT sThumbLX = 0;
            SHORT sThumbLY = 0;
            SHORT sThumbRX = 0;
            SHORT sThumbRY = 0;

            // Only account for thumbstick info beyond the deadzone
            // Note that this is a simplification of the deadzone. The dead
            // zone is when the thumb stick is at or near its 'home' position.
            // If the thumb stick is pushed all the way to the left then, even
            // though the y-coordinates are still in the dead-zone range, they
            // are actually considered significant. This dead-zone ignoring
            // code will ignore the y-coordinate in a horizontal block all the
            // way across, and will ignore the x-coordinate in a vertical block
            // all the way up/down. This simplification lets us normalize the
            // coordinates into a simple +-1.0 range, but for delicate control
            // (subtle steering when pushing forward) this technique will not
            // be sufficient.
            if( m_Gamepads[i].sThumbLX > GamepadReading::LEFT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbLX < -GamepadReading::LEFT_THUMB_DEADZONE )
                sThumbLX = m_Gamepads[i].sThumbLX;
            if( m_Gamepads[i].sThumbLY > GamepadReading::LEFT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbLY < -GamepadReading::LEFT_THUMB_DEADZONE )
                sThumbLY = m_Gamepads[i].sThumbLY;
            if( m_Gamepads[i].sThumbRX > GamepadReading::RIGHT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbRX < -GamepadReading::RIGHT_THUMB_DEADZONE )
                sThumbRX = m_Gamepads[i].sThumbRX;
            if( m_Gamepads[i].sThumbRY > GamepadReading::RIGHT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbRY < -GamepadReading::RIGHT_THUMB_DEADZONE )
                sThumbRY = m_Gamepads[i].sThumbRY;

            // Sum up the raw thumbstick inputs, as long as the thumbstick
            // is outside of the dead-zone. This is different from the
            // summing above where the x and y values are accepted or
            // discarded separately.
            if( m_Gamepads[i].sThumbRX > GamepadReading::RIGHT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbRX < -GamepadReading::RIGHT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbRY > GamepadReading::RIGHT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbRY < -GamepadReading::RIGHT_THUMB_DEADZONE )
            {
                iThumbRX += m_Gamepads[i].sThumbRX;
                iThumbRY += m_Gamepads[i].sThumbRY;
            }

            if( m_Gamepads[i].sThumbLX > GamepadReading::LEFT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbLX < -GamepadReading::LEFT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbLY > GamepadReading::LEFT_THUMB_DEADZONE ||
                m_Gamepads[i].sThumbLY < -GamepadReading::LEFT_THUMB_DEADZONE )
            {
                iThumbLX += m_Gamepads[i].sThumbLX;
                iThumbLY += m_Gamepads[i].sThumbLY;
            }

            // Keep track of which gamepads are active
            if( sThumbLX!=0 || sThumbLY!=0 || sThumbRX!=0 || sThumbRY!=0 )
                bActiveThumbs[i] = TRUE;

            destPad.fX1 += m_Gamepads[i].fX1;
            destPad.fY1 += m_Gamepads[i].fY1;
            destPad.fX2 += m_Gamepads[i].fX2;
            destPad.fY2 += m_Gamepads[i].fY2;

            destPad.wButtons         |= m_Gamepads[i].wButtons;
            destPad.wPressedButtons  |= m_Gamepads[i].wPressedButtons;
            destPad.wReleasedButtons |= m_Gamepads[i].wReleasedButtons;
            destPad.wLastButtons     |= m_Gamepads[i].wLastButtons;

            if( m_Gamepads[i].wButtons != m_Gamepads[i].wLastButtons )
                bActiveButtons[i] = TRUE;

            destPad.bLeftTrigger          |= m_Gamepads[i].bLeftTrigger;
            destPad.bPressedLeftTrigger   |= m_Gamepads[i].bPressedLeftTrigger;
            destPad.bReleasedLeftTrigger  |= m_Gamepads[i].bReleasedLeftTrigger;
            destPad.bLastLeftTrigger      |= m_Gamepads[i].bLastLeftTrigger;

            destPad.bRightTrigger         |= m_Gamepads[i].bRightTrigger;
            destPad.bPressedRightTrigger  |= m_Gamepads[i].bPressedRightTrigger;
            destPad.bReleasedRightTrigger |= m_Gamepads[i].bReleasedRightTrigger;
            destPad.bLastRightTrigger     |= m_Gamepads[i].bLastRightTrigger;

            if( m_Gamepads[i].bLeftTrigger != m_Gamepads[i].bLastLeftTrigger )
                bActiveButtons[i] = TRUE;
            if( m_Gamepads[i].bRightTrigger != m_Gamepads[i].bLastRightTrigger )
                bActiveButtons[i] = TRUE;
        }
    }

    // Clamp summed thumbstick values to proper range
    destPad.sThumbLX = ClampToShort(iThumbLX);
    destPad.sThumbLY = ClampToShort(iThumbLY);
    destPad.sThumbRX = ClampToShort(iThumbRX);
    destPad.sThumbRY = ClampToShort(iThumbRY);

    // Fill pActiveGamePadsMask
    if ( pdwActiveGamePadsMask != NULL )
    {
        DWORD dwActiveGamePadsMask = 0;
        for( DWORD i=0; i<XUSER_MAX_COUNT; i++ )
        {
            if( bActiveButtons[i] || bActiveThumbs[i] || m_Gamepads[i].wPressedButtons )
            {
                dwActiveGamePadsMask |= 1 << i;
            }
        }

        *pdwActiveGamePadsMask = dwActiveGamePadsMask;
    }

    // Assign an active GamepadReading
    for( DWORD i=0; i<XUSER_MAX_COUNT; i++ )
    {
        if( bActiveButtons[i] )
        {
            destPad.dwUserIndex = i;
            break;
        }

        if( bActiveThumbs[i] )
        {
            destPad.dwUserIndex = i;
            break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: Update
// Desc: Update the input each frame
//--------------------------------------------------------------------------------------
VOID XSF::Input::Update()
{
    GetMergedInput( m_mergedPad );
}


