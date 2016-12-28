//----------------------------------------------------------------------------------------------------------------------
// Help12.cpp
// 
// Implements a help-screen which can be shown/hidden by using the Gamepad view/back button.
//
// Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#include "pch.h"
#include "Help12.h"

namespace XSF = XboxSampleFramework;


// Disable constant-expression warning.
#pragma warning(push)
#pragma warning(disable: 4127)

//----------------------------------------------------------------------------------------------------------------------
// Name: EnumClassBitwiseOr
// Desc: Merges two bit flags from an enum-class type. 
// Params:
//       type - the type of the enum class
//       a    - a value from the enum
//       b    - another value from the enum
// Returns:
//       The merged (logical OR) values.
// Notes:
//       This is necessary because VC++11 doesn't support constexpr yet.
//----------------------------------------------------------------------------------------------------------------------
template <typename T> T EnumClassBitwiseOr( T a, T b )
{
    if ( sizeof(T) == 1 )
    {
        return (T)((unsigned char)a | (unsigned char)b);
    }
    else if ( sizeof(T) == 2 )
    {
        return (T)((unsigned short)a | (unsigned short)b);
    }
    else if ( sizeof(T) == 4 )
    {
        return (T)((unsigned long)a | (unsigned long)b);
    }
    else if ( sizeof(T) == 8 )
    {
        return (T)((unsigned long long)a | (unsigned long long)b);
    }
    else
    {
        XSF_ASSERT(!"Unsupported width");
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Name: EnumClassBitwiseOr
// Desc: Merges two bit flags from an enum-class type. 
// Params:
//       type - the type of the enum class
//       a    - a value from the enum
//       b    - another value from the enum
//       c    - a third value from the enum.
// Returns:
//       The merged (logical OR) values.
// Notes:
//       This is necessary because VC++11 doesn't support constexpr yet.
//----------------------------------------------------------------------------------------------------------------------

template <typename T> T EnumClassBitwiseOr( T a, T b, T c )
{
    if ( sizeof(T) == 1 )
    {
        return (T)((unsigned char)a | (unsigned char)b | (unsigned char)c );
    }
    else if ( sizeof(T) == 2 )
    {
        return (T)((unsigned short)a | (unsigned short)b | (unsigned short)c );
    }
    else if ( sizeof(T) == 4 )
    {
        return (T)((unsigned long)a | (unsigned long)b | (unsigned long)c);
    }
    else if ( sizeof(T) == 8 )
    {
        return (T)((unsigned long long)a | (unsigned long long)b | (unsigned long long)c);
    }
    else
    {
        XSF_ASSERT(!"Unsupported width");
    }
}

#pragma warning( pop )


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants

//----------------------------------------------------------------------------------------------------------------------
// Name: DESIGN_SURFACE_WIDTH
// Desc: The width of the design surface that anchor points/locations are specified in for the help screen. The surface
//       is scaled if the display buffer is not the same size as the design surface.
//----------------------------------------------------------------------------------------------------------------------
const INT DESIGN_SURFACE_WIDTH = 1920;

//----------------------------------------------------------------------------------------------------------------------
// Name: DESIGN_SURFACE_HEIGHT
// Desc: The height of the design surface that anchor points/locations are specified in for the help screen. The surface
//       is scaled if the display buffer is not the same size as the design surface. 
//----------------------------------------------------------------------------------------------------------------------
const INT DESIGN_SURFACE_HEIGHT = 1080;

//----------------------------------------------------------------------------------------------------------------------
// Name: DEFAULT_PADDING
// Desc: The default margin in the interior of a callout box, between the text and the edge of the box.
//----------------------------------------------------------------------------------------------------------------------
const RECT DEFAULT_PADDING = { 4, 4, 4, 4 };

//----------------------------------------------------------------------------------------------------------------------
// Name: DEFAULT_MARGIN
// Desc: Default value for the margin surrounding a callout box.
//----------------------------------------------------------------------------------------------------------------------
const RECT DEFAULT_MARGIN = { 4, 4, 4, 4 };

//----------------------------------------------------------------------------------------------------------------------
// Name: MIN_STRUT_DIST
// Desc: The minimum distance for which a callout will show a "balloon tail" or a line from the callout edge to the 
//       callout line anchor point. (not currently used)
//----------------------------------------------------------------------------------------------------------------------
const INT MIN_STRUT_DIST = 8;

//----------------------------------------------------------------------------------------------------------------------
// Name: NO_MAX_SIZE
// Desc: Value used to indicate that there is no upper bounds on this object's size. (Not currently used in layout).
//----------------------------------------------------------------------------------------------------------------------
const SIZE NO_MAX_SIZE = { INT_MAX, INT_MAX };

//----------------------------------------------------------------------------------------------------------------------
// Name: NO_MIN_SIZE
// Desc: Value used to indicate that there is no lower bounds on this object's size. (Not currently used in layout).
//----------------------------------------------------------------------------------------------------------------------
const SIZE NO_MIN_SIZE = { 0, 0};

// Path to Gamepad background image.
const LPCWSTR BACKGROUND_IMAGE_FILENAME = L"Media\\Help\\Gamepad.dds";

// Paths to font assets.
const LPCWSTR ARIAL_12_FONT = L"Media\\Fonts\\Arial_12";
const LPCWSTR ARIAL_16_FONT = L"Media\\Fonts\\Arial_16";
const LPCWSTR ARIAL_20_FONT = L"Media\\Fonts\\Arial_20";

// Title safe boundaries for the design surface.
const SIZE TITLE_SAFE_MARGIN = { DESIGN_SURFACE_WIDTH / 20, DESIGN_SURFACE_HEIGHT/ 20 };
const RECT TITLE_SAFE_BOUNDS = { TITLE_SAFE_MARGIN.cx,
    TITLE_SAFE_MARGIN.cy,
    DESIGN_SURFACE_WIDTH - TITLE_SAFE_MARGIN.cx,
    DESIGN_SURFACE_HEIGHT - TITLE_SAFE_MARGIN.cy };

// Anchor points for each of the callout boxes.
const POINT ANCHOR_TITLE		  = { DESIGN_SURFACE_WIDTH / 2, TITLE_SAFE_BOUNDS.top };
const POINT ANCHOR_DESCRIPTION	  = { DESIGN_SURFACE_WIDTH / 2, TITLE_SAFE_BOUNDS.bottom };
const POINT ANCHOR_LEFT_STICK = { 513, 468 };
const POINT ANCHOR_RIGHT_STICK = { 1405, 599 };
const POINT ANCHOR_LEFT_STICK_CLICK = { 513, 526 };
const POINT ANCHOR_RIGHT_STICK_CLICK = { 1405, 657 };
const POINT ANCHOR_DPAD_UP = { 513, 647 };
const POINT ANCHOR_DPAD_DOWN = { 513, 661 };
const POINT ANCHOR_DPAD_LEFT = { 513, 718 };
const POINT ANCHOR_DPAD_RIGHT = { 513, 691 };
const POINT ANCHOR_DPAD_ALL = { 513, 605 };
const POINT ANCHOR_RIGHT_SHOULDER = { 1405, 230 };
const POINT ANCHOR_RIGHT_TRIGGER = { 1405, 300 };
const POINT ANCHOR_LEFT_SHOULDER = { 513, 230 };
const POINT ANCHOR_LEFT_TRIGGER = { 513, 300 };
const POINT ANCHOR_A_BUTTON = { 1405, 538 };
const POINT ANCHOR_B_BUTTON = { 1405, 488 };
const POINT ANCHOR_X_BUTTON = { 1405, 389 };
const POINT ANCHOR_Y_BUTTON = { 1405, 440 };
const POINT ANCHOR_MENU = { 1405, 785 };
const POINT ANCHOR_VIEW = { 513, 785 };

// Anchor points for each of the callout balloons (what each callout is referring to).
const POINT CALLOUT_LINE_LEFT_STICK = { 740, 468 };
const POINT CALLOUT_LINE_RIGHT_STICK = { 1091, 599 };
const POINT CALLOUT_LINE_LEFT_STICK_CLICK = { 774, 487 };
const POINT CALLOUT_LINE_RIGHT_STICK_CLICK = { 1054, 599 };
const POINT CALLOUT_LINE_DPAD_UP = { 864, 555 };
const POINT CALLOUT_LINE_DPAD_DOWN = { 864, 651 };
const POINT CALLOUT_LINE_DPAD_LEFT = { 815, 605 };
const POINT CALLOUT_LINE_DPAD_RIGHT = { 916, 605 };
const POINT CALLOUT_LINE_DPAD_ALL = { 815, 605 };
const POINT CALLOUT_LINE_RIGHT_SHOULDER = { 1097, 342 };
const POINT CALLOUT_LINE_RIGHT_TRIGGER = { 1171, 336 };
const POINT CALLOUT_LINE_LEFT_SHOULDER = { 815, 342 };
const POINT CALLOUT_LINE_LEFT_TRIGGER = { 751, 336 };
const POINT CALLOUT_LINE_A_BUTTON = { 1168, 538 };
const POINT CALLOUT_LINE_B_BUTTON = { 1215, 488 };
const POINT CALLOUT_LINE_X_BUTTON = { 1097, 465 };
const POINT CALLOUT_LINE_Y_BUTTON = { 1168, 440 };
const POINT CALLOUT_LINE_MENU = { 1012, 503 };
const POINT CALLOUT_LINE_VIEW = { 907, 503 };

// The default help screen settings. 
const XSF::HelpScreen DEFAULT_HELP_SCREEN = 
{
    XSF::HELP_TITLE_COLOR,
    XSF::HELP_DESCRIPTION_COLOR,
    XSF::HELP_CALLOUT_COLOR,
    XSF::HELP_CALLOUT_CONTAINER_COLOR,
    XSF::HelpFont::UseDefaultTitleFont,
    nullptr,
    XSF::HelpFont::UseDefaultDescriptionFont,
    nullptr,
    XSF::HelpFont::UseDefaultCallOutFont,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0
};

//////////////////////////////////////////////////////////////////////////
// Static Members

XSF::BitmapFont* XSF::Help::ms_DefaultTitleFont = nullptr;             // Default title font.
XSF::BitmapFont* XSF::Help::ms_DefaultDescriptionFont = nullptr;       // Default description text font.
XSF::BitmapFont* XSF::Help::ms_DefaultCalloutFont = nullptr;           // Default callout text font.


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enums & Structs

//----------------------------------------------------------------------------------------------------------------------
// Name: enum class CalloutAlignment
// Desc: Used to specify where a callout will be rendered relative to its anchor point.
//
// Callout positioning (note: included only for future implementation - they do nothing yet)
// If CALLOUT_FIXED is specified, alignment parameters specify what point on the text rectangle is anchored to the 
// callout anchor point.
// If CALLOUT_MOVEABLE is specified (or CALLOUT_FIXED is /not/ specified), alignment specifies the closest point on the
// callout rectangle to the callout anchor point, but the box can be shifted to prevent overlapping other boxes.
// If CALLOUT_DONOTRENDER is specified, the item will not be rendered.
// If CALLOUT_NO_OVERLAP is specified, the item has a boosted priority for its layout position.
// 
// The CALLOUT_FIXED | CALLOUT_DONOTRENDER flag combo is useful for ensuring that buttons are not obscured when the
// engine is re-laying out callouts so that the callout targets are not obscured.
//----------------------------------------------------------------------------------------------------------------------
enum class CalloutAlignment : unsigned short
{
    TO_LEFT= 1,                             // Anchor with callout to LEFT of the anchor point
    TO_RIGHT = 2,                           // Anchor with callout to RIGHT of the anchor point
    HORIZONTAL_CENTER = TO_LEFT | TO_RIGHT, // Horizontally center on anchor point.
    ABOVE = 4,                              // Anchor with callout ABOVE the anchor point
    BELOW = 8,                              // Anchor with callout BELOW the anchor point
    VERTICAL_CENTER = ABOVE | BELOW,        // Vertically center block with anchor
    //RESERVED: FIXED = 16,                 // Block is anchored directly onto callout.
    //RESERVED: MOVEABLE = 0,               // Block can be moved to prevent overlaps.
    //RESERVED: DO_NOT_RENDER = 32,         // Not rendered
    //RESERVED: NO_OVERLAP = 64             // Attempts to prevent overlaps by moving other boxes around this one, and giving
    // priority to this one's placement.
};

//----------------------------------------------------------------------------------------------------------------------
// Name: enum class CalloutType
// Desc: Used to specify which visual representation will be used for a callout.
//----------------------------------------------------------------------------------------------------------------------
enum class CalloutType : unsigned short
{
    NO_CONTAINER,                           // Just the text - no background, no "tails"
    LINE_TO_ANCHOR,                         // No background on the textbox, with a line to the callout target.
    //RESERVED: BLOCK,                      // Text with a background color, and no line to callout.
    //RESERVED: BALLOON_BLOCK,              // BLOCK with a "text balloon" callout to its target.
    //RESERVED: LINE_BLOCK,                 // Block with a line to its callout target.
    //RESERVED: JUNCTION_BLOCK,             // Block with a junction around its callout target, then a line back.
};

namespace XboxSampleFramework
{
    struct Help::CalloutBox
    {
        static const CalloutBox CalloutTemplates[];

        CalloutType type;           // The type of callout to render.
        CalloutAlignment align;     // Alignment relative to anchor point.
        XSF::BitmapFont* pFont;      // Font to render text in.
        COLORREF foreground;        // Foreground color
        COLORREF background;        // Background color
        INT minstrutdistance;       // Minimum anchor distance below which callout "struts" will not be rendered.
        SIZE minSize;               // Minimum size of the callout.
        SIZE maxSize;               // Maximum size of the callout.
        SIZE contentSize;           // fWidth and fHeight in pixels of content
        RECT padding;               // padding in pixels around content
        RECT margin;                // margin in pixels around box
        RECT boundsContent;         // top left point of this text block.
        RECT boundsBackground;      // final bounds including all margins, padding, content.
        RECT boundsTotal;           // total bounds, including margin.
        POINT anchor;               // the anchor point for this callout.
        LPCWSTR text;               // text for the callout.
        POINT calloutLine;			// position to draw the line to on the callout from the anchor (if shown).

        static void Create(_Out_ CalloutBox& dest, _In_z_ LPCWSTR pszText, _In_ XSF::BitmapFont* pFont, COLORREF color, HelpID helpSlot);

        void UpdateBounds();

        void Render(_In_ XSF::Draw& pDraw, XMFLOAT2 vScaling, _In_opt_ const D3D12_VIEWPORT* pViewport = nullptr);
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callout templates used as the default definitions for each button assignment.

// NOTE: Must correspond 1:1 to the contents of the HelpId list.
const XSF::Help::CalloutBox XSF::Help::CalloutBox::CalloutTemplates[] = 
{
    // TITLE_TEXT
    {   
        CalloutType::NO_CONTAINER,          // Callout type
                                            // Callout Alignment:
        EnumClassBitwiseOr(  CalloutAlignment::HORIZONTAL_CENTER, CalloutAlignment::VERTICAL_CENTER), 
        nullptr,                            // Font (not used in template)
        XSF::HELP_TITLE_COLOR,              // Text Color
        XSF::HELP_TRANSPARENT_COLOR,        // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_TITLE,                       // Anchor point to use
        nullptr,                            // Text (not used in template)
        { 0, 0 }                            // Callout target point
    },
    // DESCRIPTION_TEXT
    {   
        CalloutType::NO_CONTAINER,          // Callout type
                                            // Callout Alignment:
        EnumClassBitwiseOr( CalloutAlignment::HORIZONTAL_CENTER, CalloutAlignment::VERTICAL_CENTER),
        nullptr,                            // Font (not used in template)
        XSF::HELP_DESCRIPTION_COLOR,        // Text Color
        XSF::HELP_TRANSPARENT_COLOR,        // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DESCRIPTION,                 // Anchor point to use
        nullptr,                            // Text (not used in template)
        { 0, 0 }                            // Callout target point
    },
    // LEFT_STICK,
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
                                            // Callout alignment:
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER),
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_LEFT_STICK,                  // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_LEFT_STICK             // Callout target point
    },
    // RIGHT_STICK
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_RIGHT_STICK,                 // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_RIGHT_STICK            // Callout target point
    },
    // DPAD_UP
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DPAD_UP,                     // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_DPAD_UP                // Callout target point
    },
    // DPAD_DOWN
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DPAD_DOWN,                   // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_DPAD_DOWN              // Callout target point
    },
    // DPAD_LEFT
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DPAD_LEFT,                   // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_DPAD_LEFT              // Callout target point
    },
    // DPAD_RIGHT
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DPAD_RIGHT,                  // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_DPAD_RIGHT             // Callout target point
    },

    // DPAD_ALL
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_DPAD_ALL,                    // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_DPAD_ALL               // Callout target point
    },
    // RIGHT_SHOULDER
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_RIGHT_SHOULDER,              // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_RIGHT_SHOULDER         // Callout target point
    },
    // RIGHT_TRIGGER
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_RIGHT_TRIGGER,               // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_RIGHT_TRIGGER          // Callout target point
    },
    // LEFT_SHOULDER
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_LEFT_SHOULDER,               // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_LEFT_SHOULDER          // Callout target point
    },
    // LEFT_TRIGGER
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_LEFT_TRIGGER,                // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_LEFT_TRIGGER           // Callout target point
    },
    // A_BUTTON
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_A_BUTTON,                    // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_A_BUTTON               // Callout target point
    },
    // B_BUTTON
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_B_BUTTON,                    // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_B_BUTTON               // Callout target point
    },
    // X_BUTTON
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_X_BUTTON,                    // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_X_BUTTON               // Callout target point
    },
    // Y_BUTTON
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_Y_BUTTON,                    // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_Y_BUTTON               // Callout target point
    },
    // START_BUTTON 
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_MENU,                       // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_MENU                  // Callout target point
    },
    // MENU_BUTTON (Same as start)
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_RIGHT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_MENU,                       // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_MENU                  // Callout target point
    },
    // BACK_BUTTON
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_VIEW,                        // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_VIEW                   // Callout target point
    },
    // VIEW_BUTTON (Same as back)
    {   
        CalloutType::LINE_TO_ANCHOR,        // Callout type
        EnumClassBitwiseOr( CalloutAlignment::TO_LEFT, CalloutAlignment::VERTICAL_CENTER), // Callout alignment
        nullptr,                            // Font (not used in template)
        XSF::HELP_CALLOUT_COLOR,            // Text Color
        XSF::HELP_CALLOUT_CONTAINER_COLOR,  // Bg Color
        MIN_STRUT_DIST,                     // Min distance where strut will be drawn
        NO_MIN_SIZE,						// No minimum size
        NO_MAX_SIZE,                        // Maximum size of block (INT_MAX = infinite in that direction)
        { 0, 0 },                           // Content Size (not used in template)
        DEFAULT_PADDING,                    // Padding
        DEFAULT_MARGIN,                     // Margin
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        { 0, 0, 0, 0 },                     // Bounds (not used in template)
        ANCHOR_VIEW,                        // Anchor point to use
        nullptr,                            // Text (not used in template)
        CALLOUT_LINE_VIEW                   // Callout target point
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------------------------------------------------
// Name: UpdateBounds
// Desc: Recalculates the bounding boxes for the text, container and margins of the callouts.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::CalloutBox::UpdateBounds()
{
    // Handle horizontal alignment.

    CalloutAlignment horizAlign = (CalloutAlignment)( (unsigned short)align & (unsigned short)CalloutAlignment::HORIZONTAL_CENTER );

    if( horizAlign == CalloutAlignment::HORIZONTAL_CENTER )
    {
        INT offset = ( (contentSize.cx + 1 ) >> 1);
        boundsContent.left = anchor.x - offset;
        boundsBackground.left = boundsContent.left - padding.left;
        boundsTotal.left = boundsContent.left - margin.left;

        boundsContent.right = anchor.x + ( contentSize.cx - offset );
        boundsBackground.right = boundsContent.right + padding.right;
        boundsTotal.right = boundsContent.right + margin.right;

    }
    else if( horizAlign == CalloutAlignment::TO_RIGHT )
    {
        boundsBackground.left = anchor.x;
        boundsContent.left = anchor.x + padding.left;
        boundsTotal.left = anchor.x - margin.left;

        boundsContent.right = boundsContent.left + contentSize.cx;
        boundsBackground.right = boundsContent.right + padding.right;
        boundsTotal.right = boundsBackground.right + margin.right;
    }
    else
    {
        boundsBackground.right = anchor.x;
        boundsContent.right = anchor.x - padding.right;
        boundsTotal.right = anchor.x + margin.right;

        boundsContent.left = boundsContent.right - contentSize.cx;
        boundsBackground.left = boundsContent.left - padding.left;
        boundsTotal.left = boundsBackground.left - margin.left;
    }

    // Handle vertical alignment.

    CalloutAlignment vertAlign = (CalloutAlignment)( (unsigned short)align & (unsigned short)CalloutAlignment::VERTICAL_CENTER );

    if( vertAlign == CalloutAlignment::VERTICAL_CENTER )
    {
        INT offset = ( (contentSize.cy + 1 ) >> 1);
        boundsContent.top = anchor.y - offset;
        boundsBackground.top = boundsContent.top - padding.top;
        boundsTotal.top = boundsContent.top - margin.top;

        boundsContent.bottom = anchor.y + ( contentSize.cy - offset );
        boundsBackground.bottom = boundsContent.bottom + padding.bottom;
        boundsTotal.bottom = boundsContent.bottom + margin.bottom;

    }
    else if( vertAlign == CalloutAlignment::BELOW )
    {
        boundsBackground.top = anchor.y;
        boundsContent.top = anchor.y + padding.top;
        boundsTotal.top = anchor.y - margin.top;

        boundsContent.bottom = boundsContent.top + contentSize.cy;
        boundsBackground.bottom = boundsContent.bottom + padding.bottom;
        boundsTotal.bottom = boundsBackground.bottom + margin.bottom;
    }
    else
    {
        boundsBackground.bottom = anchor.y;
        boundsContent.bottom = anchor.y - padding.bottom;
        boundsTotal.bottom = anchor.y + margin.bottom;

        boundsContent.top = boundsContent.bottom - contentSize.cy;
        boundsBackground.top = boundsContent.top - padding.top;
        boundsTotal.top = boundsBackground.top - margin.top;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Name: Render
// Desc: Renders a callout to the display.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::CalloutBox::Render(XSF::Draw& draw, XMFLOAT2 vScaling, const D3D12_VIEWPORT* pViewport)
{
    D3D12_RECT window = pFont->GetWindow();
    pFont->ResetWindow(0);	// we handle our title safe area ourselves, so we need absolute coords
    XMFLOAT2 fontScale = pFont->GetScaleFactors();
    pFont->SetScaleFactors(vScaling.x, vScaling.y);

    if (type == CalloutType::LINE_TO_ANCHOR)
    {
        XMVECTOR bgColor = XMLoadColor((XMCOLOR*)&background);

        CalloutAlignment horizAlign = (CalloutAlignment)((unsigned short)align & (unsigned short)CalloutAlignment::HORIZONTAL_CENTER);

        INT targetX = (horizAlign == CalloutAlignment::TO_LEFT) ? boundsContent.right : boundsContent.left;
        INT boxCenterY = (boundsContent.top + boundsContent.bottom) >> 1;

        draw.Line(static_cast<INT>(vScaling.x * calloutLine.x), static_cast<INT>(vScaling.y * calloutLine.y), bgColor,
            static_cast<INT>(vScaling.x * targetX), static_cast<INT>(vScaling.y * boxCenterY), bgColor);
    }

    // TODO: Draw background for callout.
    pFont->Begin(pViewport);
    pFont->DrawText(vScaling.x * boundsContent.left, vScaling.y * boundsContent.top, foreground, text, 0, vScaling.x * contentSize.cx);
    pFont->End();

    // Restore the font window setting.
    pFont->SetWindow(window);
    // Restore the font scale.
    pFont->SetScaleFactors(fontScale.x, fontScale.y);
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Create
// Desc: Creates a new callout instance in-place.
// Params:
//  dest     - The callout instance to overwrite based on the template and provided data.
//  pText    - The text for the callout.
//  pFont    - The font to render the callout text with.
//  color    - The foreground color for the callout text.
//  helpSlot - The template instance to base this callout on.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::CalloutBox::Create(Help::CalloutBox& dest, LPCWSTR pText, XSF::BitmapFont* pFont, COLORREF color, HelpID helpSlot)
{
    memcpy(&dest, &CalloutTemplates[static_cast<size_t>(helpSlot)], sizeof(CalloutBox));
    dest.text = pText;
    dest.foreground = color;
    dest.pFont = pFont;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Help
// Desc: Constructor for the Help object instance.
//----------------------------------------------------------------------------------------------------------------------
XSF::Help::Help()
    : m_pSample(nullptr),
    m_IsInitialized(FALSE),
    m_WantsInput(FALSE),
    m_IgnoreViewButtonThisFrame(FALSE),
    m_IsVisible(FALSE),
    m_pCalloutBoxes(nullptr),
    m_CalloutCount(0),
    m_pCanvas(nullptr)
{
}

//----------------------------------------------------------------------------------------------------------------------
// Name: ~Help
// Desc: Destructor for the Help object instance.
//----------------------------------------------------------------------------------------------------------------------
XSF::Help::~Help()
{
    Shutdown();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Hide
// Desc: Hides the help screen.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::Hide()
{
    m_WantsInput = FALSE;
    m_IsVisible = FALSE;
    m_IgnoreViewButtonThisFrame = FALSE;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Show
// Desc: Displays the help screen.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::Show()
{
    m_WantsInput = TRUE;
    m_IsVisible = TRUE;
    m_IgnoreViewButtonThisFrame = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates the help screen. Used for multi-page help/transitions/animations. Mainly here for future expansion.
//----------------------------------------------------------------------------------------------------------------------
BOOL XSF::Help::Update(const XSF::GamepadReading& input, FLOAT /*deltaTime*/)
{
    XSFScopedNamedEventFunc( static_cast<XSF::D3DCommandList*>(nullptr), 0 );

    if (!IsVisible()
#ifdef _XBOX_ONE
        && input.WasViewPressed()
#else
        && input.WasBackPressed()
#endif
        )
    {
        Show();
    }

    if (m_WantsInput)
    {
        if (!m_IgnoreViewButtonThisFrame
#ifdef _XBOX_ONE
            && input.WasViewPressed()
#else
            && input.WasBackPressed()
#endif
            )
        {
            Hide();
        }
    }

    m_IgnoreViewButtonThisFrame = FALSE;

    return IsVisible();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Render
// Desc: Renders the help screen to the display.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::Render(const D3D12_VIEWPORT *pViewport)
{
    XSF_ASSERT(m_IsVisible && "Only call if IsVisible is TRUE");
    
    XSFScopedNamedEventFunc(m_pSample->GetCommandList(), 0);

    m_pCanvas->Begin(pViewport);

    const XSF::StockRenderStates& stockStates = XSF::StockRenderStates::GetInstance();

    m_pCanvas->ApplyRasterizerState(stockStates.GetRasterizerDesc(StockRasterizerStates::Solid));
    m_pCanvas->ApplyBlendState(stockStates.GetBlendDesc(XSF::StockBlendStates::AlphaBlend));

    const D3D12_GPU_DESCRIPTOR_HANDLE hSampler = stockStates.GetSamplerGPUHandle(StockSamplerStates::MinMagLinearMipPointUVWClamp);

    // Render the background texture.
    m_pCanvas->TexturedQuad(m_srvHeap, m_srvHeap.hGPU(0), stockStates.GetSamplerHeap(), hSampler, pViewport);

    // Render the call-outs in reverse order.
    for (int i = m_CalloutCount - 1; i >= 0; --i)
    {
        m_pCalloutBoxes[i].Render(*m_pCanvas, m_DisplayScaleFactor, pViewport);
    }

    m_pCanvas->End();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes the help system, tying it to a specific display size.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::Initialize(const SampleFramework* const pSample, UINT frameWidth, UINT frameHeight)
{
    XSF_ASSERT(!m_IsInitialized && "Already initialized...");

    m_pSample = pSample;
    XSF::D3DDevice* const pDevice = m_pSample->GetDevice();
    ID3D12Fence* const pFence = m_pSample->GetFence();
    XSF::D3DCommandList* const pCmdList = m_pSample->GetCommandList();
    XSF::D3DCommandAllocator* const pCmdAllocator = m_pSample->GetCommandAllocator();

    m_DisplayBounds.cx = static_cast<LONG>(frameWidth);
    m_DisplayBounds.cy = static_cast<LONG>(frameHeight);
    
    XSF_ERROR_IF_FAILED(m_uploadHeap.Initialize(pDevice, pFence, 9 * 1024 * 1024, false, 1, L"Help::UploadHeap"));
    (const_cast<SampleFramework*>(pSample))->ManageUploadHeap(&m_uploadHeap);
    XSF_ERROR_IF_FAILED(m_srvHeap.Initialize(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true));

    // Load the background texture...
    DDSLoader12 TexLoader;
    XSF_ERROR_IF_FAILED(TexLoader.Initialize(pDevice, pSample->GetCommandQueue(), pCmdAllocator, &m_uploadHeap));
    TexLoader.BeginLoading(pCmdList);

    XSF_ERROR_IF_FAILED(TexLoader.LoadDDSFile(BACKGROUND_IMAGE_FILENAME, m_srvHeap.hCPU(0), m_spBackgroundTexture.ReleaseAndGetAddressOf()));

    TexLoader.FinishLoading(false);
    TexLoader.Terminate();

    LoadFonts(pSample);

    CalcDisplayScaling();

    m_pCanvas = new Draw;
    
    XSF_ERROR_IF_FAILED(m_pCanvas->Initialize(pSample));

    m_IsInitialized = TRUE;
}

//----------------------------------------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes the help system directly from the SampleFramework class
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::Initialize(const SampleFramework* const pSample)
{
    const SampleFramework::Settings& settings = pSample->SampleSettings();
    Initialize(pSample, settings.m_frameBufferWidth, settings.m_frameBufferHeight);
}

//----------------------------------------------------------------------------------------------------------------------
// Name: DestroyFonts
// Desc: Destroys all of the fonts we've allocated.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::DestroyFonts()
{
    XSF_SAFE_DELETE(ms_DefaultDescriptionFont);
    XSF_SAFE_DELETE(ms_DefaultTitleFont);
    XSF_SAFE_DELETE(ms_DefaultCalloutFont);
}

//----------------------------------------------------------------------------------------------------------------------
// Name: LoadFonts
// Desc: Loads all of the fonts used by the help system.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::LoadFonts(const SampleFramework* const pSample)
{
    XSF_ASSERT(!ms_DefaultTitleFont && !ms_DefaultCalloutFont && !ms_DefaultDescriptionFont && "Fonts already loaded?");

    ms_DefaultCalloutFont = new BitmapFont();
    ms_DefaultTitleFont = new BitmapFont();
    ms_DefaultDescriptionFont = new BitmapFont();

    const D3D12_RECT rc = { 0, 0, m_DisplayBounds.cx, m_DisplayBounds.cy };
    XSF_ERROR_IF_FAILED(ms_DefaultCalloutFont->Create(pSample, ARIAL_16_FONT, &rc));
    XSF_ERROR_IF_FAILED(ms_DefaultDescriptionFont->Create(pSample, ARIAL_12_FONT, &rc));
    XSF_ERROR_IF_FAILED(ms_DefaultTitleFont->Create(pSample, ARIAL_20_FONT, &rc));
}


//----------------------------------------------------------------------------------------------------------------------
// Name: Shutdown
// Desc: Shuts down this instance of the help system, releasing all of its data.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::Shutdown()
{
    if (m_IsInitialized)
    {
        DestroyCallouts();
        DestroyFonts();
        m_srvHeap.Terminate();
        XSF_SAFE_DELETE(m_pCanvas);
        m_IsInitialized = FALSE;
    }
}


//----------------------------------------------------------------------------------------------------------------------
// Name: SetHelpText
// Desc: Builds the help screen based on the provided parameters, and a default template.
// Params:
//  pTitle       - The title for the help screen
//  pDescription - The description of the help screen
//  pButton      - Pointer to an array of HelpButtonAssignment structs, one per button assignment.
//  buttonCount  - The number of elements in the pButton array.
//----------------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
void XSF::Help::SetHelpText(LPCWSTR pTitle, LPCWSTR pDescription, const HelpButtonAssignment * pButtons, int buttonCount)
{
    HelpScreen help = DEFAULT_HELP_SCREEN;
    help.titleText = pTitle;
    help.descriptionText = pDescription;
    help.buttonAssignments = pButtons;
    help.buttonCount = buttonCount;
    BuildHelpScreen(help);
}


//----------------------------------------------------------------------------------------------------------------------
// Name: BuildHelpScreen
// Desc: Builds a custom help screen based on the parameters specified in the helpText object provided.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::BuildHelpScreen(const HelpScreen& helpText)
{
    int calloutCount = helpText.buttonCount;

    XSF::BitmapFont* pTitleFont = nullptr;
    XSF::BitmapFont* pDescriptionFont = nullptr;
    XSF::BitmapFont* pCalloutFont = nullptr;

    pCalloutFont = MapFont(helpText.calloutFontType);

    if (pCalloutFont == nullptr)
    {
        pCalloutFont = helpText.pCalloutFont;
    }

    if (helpText.titleText != nullptr)
    {
        pTitleFont = MapFont(helpText.titleFontType);
        if (pTitleFont == nullptr)
        {
            pTitleFont = helpText.pTitleFont;
        }
        ++calloutCount;
    }

    if (helpText.descriptionText != nullptr)
    {
        pDescriptionFont = MapFont(helpText.descriptionFontType);

        if (pDescriptionFont == nullptr)
        {
            pDescriptionFont = helpText.pDescriptionTextFont;
        }

        ++calloutCount;
    }

    // Do we already have enough space? If so, just overwrite the originals. Otherwise, kill and realloc.
    if (m_CalloutCount != calloutCount)
    {
        DestroyCallouts();
        m_CalloutCount = calloutCount;
        m_pCalloutBoxes = new CalloutBox[calloutCount];
    }

    int index = 0;

    if (helpText.titleText != nullptr)
    {
        CalloutBox::Create(m_pCalloutBoxes[index++], helpText.titleText, pTitleFont, helpText.titleColor, XSF::HelpID::TITLE_TEXT);
    }

    if (helpText.descriptionText != nullptr)
    {
        CalloutBox::Create(m_pCalloutBoxes[index++], helpText.descriptionText, pDescriptionFont, helpText.descriptionColor, XSF::HelpID::DESCRIPTION_TEXT);
    }

    for (int i = 0; i < helpText.buttonCount; ++i)
    {
        CalloutBox::Create(m_pCalloutBoxes[index++], helpText.buttonAssignments[i].buttonText, pCalloutFont, helpText.calloutColor, helpText.buttonAssignments[i].id);
    }

    LayoutCallouts();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: DestroyCallouts
// Desc: Destroys any allocated callouts.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::DestroyCallouts()
{
    delete[] m_pCalloutBoxes;
    m_pCalloutBoxes = nullptr;
    m_CalloutCount = 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Name: MeasureCallouts
// Desc: Measures each of the callouts' text, and updates their bounds.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::MeasureCallouts()
{
    for (int i = 0; i < m_CalloutCount; ++i)
    {
        CalloutBox& callout = m_pCalloutBoxes[i];

        // Measure the text.
        FLOAT fWidth, fHeight;
        callout.pFont->GetTextExtent(callout.text, &fWidth, &fHeight);

        INT pixelWidth = static_cast<INT>(ceil(fWidth));
        INT pixelHeight = static_cast<INT>(ceil(fHeight));

        if (callout.maxSize.cx != INT_MAX && callout.maxSize.cx < pixelWidth)
        {
            // RewrapText(callout);
        }
        else
        {
            callout.contentSize.cx = pixelWidth;
            callout.contentSize.cy = pixelHeight;
        }

        callout.UpdateBounds();
    }
}


//----------------------------------------------------------------------------------------------------------------------
// Name: LayoutCallouts
// Desc: Lays out the callouts.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::LayoutCallouts()
{
    MeasureCallouts();
}


//----------------------------------------------------------------------------------------------------------------------
// Name: MapFont
// Desc: Maps a font enum value to a default Help system font, or returns nullptr for custom/default where the font is
//       explicitly specified elsewhere.
//----------------------------------------------------------------------------------------------------------------------
XSF::BitmapFont* XSF::Help::MapFont(HelpFont fontType) const
{
    switch (fontType)
    {
    case HelpFont::UseDefaultTitleFont:
        {
            return ms_DefaultTitleFont;
        }
    case HelpFont::UseDefaultDescriptionFont:
        {
            return ms_DefaultDescriptionFont;
        }
    case HelpFont::UseDefaultCallOutFont:
        {
            return ms_DefaultCalloutFont;
        }
    case HelpFont::UseCustomFont:
    default:
        {
            return nullptr;
        }
    }
}



//----------------------------------------------------------------------------------------------------------------------
// Name: CalcDisplayScaling
// Desc: Calculates the scale factor to apply to the design surface to get it to match the render buffer's dimensions.
//----------------------------------------------------------------------------------------------------------------------
void XSF::Help::CalcDisplayScaling()
{
    m_DisplayScaleFactor.x	 = static_cast<FLOAT>(m_DisplayBounds.cx) / static_cast<FLOAT>(DESIGN_SURFACE_WIDTH);
    m_DisplayScaleFactor.y	 = static_cast<FLOAT>(m_DisplayBounds.cy) / static_cast<FLOAT>(DESIGN_SURFACE_HEIGHT);
}
