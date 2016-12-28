#pragma once

#include <xinput.h>

namespace XboxSampleFramework
{
    struct GamepadReading : public XINPUT_GAMEPAD
    {
        // The following members are inherited from XINPUT_GAMEPAD:
        //    WORD    wButtons;
        //    BYTE    bLeftTrigger;
        //    BYTE    bRightTrigger;
        //    SHORT   sThumbLX;
        //    SHORT   sThumbLY;
        //    SHORT   sThumbRX;
        //    SHORT   sThumbRY;

        // Thumb stick values converted to range [-1,+1]
        FLOAT      fX1;
        FLOAT      fY1;
        FLOAT      fX2;
        FLOAT      fY2;

        // Records the state (when last updated) of the buttons.
        // These remain set as long as the button is pressed.
        WORD       wLastButtons;

        // Records which buttons were pressed this frame - only set on
        // the frame that the button is first pressed.
        WORD       wPressedButtons;

        // Records which buttons were released this frame - only set on
        // the frame that the button is released.
        WORD       wReleasedButtons;

        // Controllers can be set to be checked every N frames
        WORD       nextCheckCountdown;

        BYTE       bLastLeftTrigger : 1;
        BYTE       bLastRightTrigger : 1;
        BYTE       bPressedLeftTrigger : 1;
        BYTE       bPressedRightTrigger : 1;
        BYTE       bReleasedLeftTrigger : 1;
        BYTE       bReleasedRightTrigger : 1;
        
        // Device properties
        XINPUT_CAPABILITIES caps;
        bool       bConnected : 1;

        // Flags for whether game pad was just inserted or removed
        bool       bInserted : 1;
        bool       bRemoved : 1;

        // The user index associated with this Gamepad
        DWORD      dwUserIndex;

        // Deadzone pseudo-constants for the thumbsticks
        static SHORT LEFT_THUMB_DEADZONE;
        static SHORT RIGHT_THUMB_DEADZONE;
    public:
        // Something like that
        float LeftThumbstickX() const           { return IsValid() ? sThumbLX : 0.0f; }
        float LeftThumbstickY() const           { return IsValid() ? sThumbLY : 0.0f; }
        float RightThumbstickX() const          { return IsValid() ? sThumbRX : 0.0f; }
        float RightThumbstickY() const          { return IsValid() ? sThumbRY : 0.0f; }
        bool IsAPressed() const                 { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_A)         : false; }
        bool IsBackPressed() const              { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_BACK)      : false; }
        bool IsBPressed() const                 { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_B)         : false; }
        bool IsDPadDownPressed() const          { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN) : false; }
        bool IsDPadLeftPressed() const          { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT) : false; }
        bool IsDPadRightPressed() const         { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT): false; }
        bool IsDPadUpPressed() const            { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_UP)   : false; }
        bool IsLeftShoulderPressed() const      { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) : false; }
        bool IsLeftThumbstickPressed() const    { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)    : false; }
        bool IsRightShoulderPressed() const     { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER): false; }
        bool IsRightThumbstickPressed() const   { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)   : false; }
        bool IsStartPressed() const             { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_START)     : false; }
        bool IsXPressed() const                 { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_X)   : false; }
        bool IsYPressed() const                 { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_Y)   : false; }
        float LeftTrigger() const               { return IsValid() ? (float)bLeftTrigger / 255.f   : 0; }
        float RightTrigger() const              { return IsValid() ? (float)bRightTrigger / 255.f  : 0; }

        // Extra properties to our class
        bool IsValid() const                        { return !!bConnected; }
        float NormalizedLeftThumbstickX() const     { return IsValid() ? fX1 : 0.0f; }
        float NormalizedLeftThumbstickY() const     { return IsValid() ? fY1 : 0.0f; }
        float NormalizedRightThumbstickX() const    { return IsValid() ? fX2 : 0.0f; }
        float NormalizedRightThumbstickY() const    { return IsValid() ? fY2 : 0.0f; }
        float LastLeftTrigger() const               { return IsValid() ? (float)bLeftTrigger / 255.f   : 0; }
        float LastRightTrigger() const              { return IsValid() ? (float)bRightTrigger / 255.f  : 0; }
        bool IsLeftTriggerPressed() const           { return IsValid() ? bPressedLeftTrigger : false; }
        bool IsRightTriggerPressed() const          { return IsValid() ? bPressedRightTrigger : false; }

        bool APreviouslyPressed() const             { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_A)         : false; }
        bool BackPreviouslyPressed() const          { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_BACK)      : false; }
        bool BPreviouslyPressed() const             { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_B)         : false; }
        bool DPadDownPreviouslyPressed() const      { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_DPAD_DOWN) : false; }
        bool DPadLeftPreviouslyPressed() const      { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_DPAD_LEFT) : false; }
        bool DPadRightPreviouslyPressed() const     { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_DPAD_RIGHT): false; }
        bool DPadUpPreviouslyPressed() const        { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_DPAD_UP)   : false; }
        bool LeftShoulderPreviouslyPressed() const  { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) : false; }
        bool LeftThumbstickPreviouslyPressed() const{ return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_LEFT_THUMB)    : false; }
        bool RightShoulderPreviouslyPressed() const { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER): false; }
        bool RightThumbstickPreviouslyPressed() const{return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_RIGHT_THUMB)   : false; }
        bool StartPreviouslyPressed() const         { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_START)     : false; }
        bool XPreviouslyPressed() const             { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_X)   : false; }
        bool YPreviouslyPressed() const             { return IsValid() ? !!(wLastButtons & XINPUT_GAMEPAD_Y)   : false; }
        
        // The difference between IsPressed and WasPressed is that IsPressed simply returns with every call the immediate state of the button,
        // but WasPressed indicates the state change of the button. IsPressed is usefull for incrementing/decrementing values, i.e. the increment will
        // keep going while you hold the button. WasPressed is useful for mode changes, i.e. it will only be valid on the call that there is a state change
        // even if you hold the button down.
        bool WasAPressed() const                    { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_A)         : false; }
        bool WasBackPressed() const                 { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_BACK)      : false; }
        bool WasBPressed() const                    { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_B)         : false; }
        bool WasDPadDownPressed() const             { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN) : false; }
        bool WasDPadLeftPressed() const             { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT) : false; }
        bool WasDPadRightPressed() const            { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT): false; }
        bool WasDPadUpPressed() const               { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_DPAD_UP)   : false; }
        bool WasLeftShoulderPressed() const         { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) : false; }
        bool WasLeftThumbstickPressed() const       { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)    : false; }
        bool WasRightShoulderPressed() const        { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER): false; }
        bool WasRightThumbstickPressed() const      { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)   : false; }
        bool WasStartPressed() const                { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_START)     : false; }
        bool WasXPressed() const                    { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_X)   : false; }
        bool WasYPressed() const                    { return IsValid() ? !!(wPressedButtons & XINPUT_GAMEPAD_Y)   : false; }

        bool WasAReleased() const                   { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_A)         : false; }
        bool WasBackReleased() const                { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_BACK)      : false; }
        bool WasBReleased() const                   { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_B)         : false; }
        bool WasDPadDownReleased() const            { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_DPAD_DOWN) : false; }
        bool WasDPadLeftReleased() const            { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_DPAD_LEFT) : false; }
        bool WasDPadRightReleased() const           { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_DPAD_RIGHT): false; }
        bool WasDPadUpReleased() const              { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_DPAD_UP)   : false; }
        bool WasLeftShoulderReleased() const        { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) : false; }
        bool WasLeftThumbstickReleased() const      { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_LEFT_THUMB)    : false; }
        bool WasRightShoulderReleased() const       { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER): false; }
        bool WasRightThumbstickReleased() const     { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)   : false; }
        bool WasStartReleased() const               { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_START)     : false; }
        bool WasXReleased() const                   { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_X)   : false; }
        bool WasYReleased() const                   { return IsValid() ? !!(wReleasedButtons & XINPUT_GAMEPAD_Y)   : false; }

        UINT PressedButtons() const                 { return wPressedButtons; }
        UINT ReleasedButtons() const                { return wReleasedButtons; }

        bool WasAnythingPressedOrReleased() const   { return (0 != wReleasedButtons) || (0 != wPressedButtons); }
    };


    //--------------------------------------------------------------------------------------
    // Name: class Input
    // Desc: Class to manage input devices
    //--------------------------------------------------------------------------------------
    class Input
    {
    public:
        HRESULT    Initialize();
        VOID Update();
        const XSF::GamepadReading& GetCurrentGamepadReading() const { return m_mergedPad; }

    private:
        // Processes input from all 4 gamepads and merge it into one input
        VOID GetMergedInput( GamepadReading& destPad, DWORD dwMask = 0, DWORD* pdwActiveGamePadsMask = NULL );

        GamepadReading   m_Gamepads[ XUSER_MAX_COUNT ];
        GamepadReading   m_mergedPad;

        // Processes input from the game pad
        VOID GetInput( GamepadReading* pGamepads = NULL );
    };

} // namespace XboxSampleFramework
