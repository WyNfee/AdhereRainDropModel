//--------------------------------------------------------------------------------------
// Console.h
//
// Renders a simple on screen console where you can output text information
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_CONSOLE_H
#define XSF_CONSOLE_H

#include "Draw.h"

namespace XboxSampleFramework
{

//--------------------------------------------------------------------------------------
// Name: class Console
// Desc: Class to implement the console.
//--------------------------------------------------------------------------------------
class Console
{
public:
                    Console();
                    ~Console();

    // Initialization
    // textColor and backColor is in format ARGB
    HRESULT         Create( D3DDevice* pDevice, 
                            BitmapFont *pFont, 
                            UINT Width, UINT Height, 
                            DWORD textColor, DWORD backColor,  
                            UINT nLines = 0, Draw* pDraw = NULL );

    VOID            Destroy();

    // Clear all text in the console
    VOID            Clear();

    // Write text to the console
    virtual VOID    Format( _In_z_ _Printf_format_string_ LPCSTR strFormat, ... );
    virtual VOID    Format( _In_z_ _Printf_format_string_ LPCWSTR wstrFormat, ... );
    virtual VOID    FormatV( _In_z_ _Printf_format_string_ LPCSTR strFormat, va_list pArgList );
    virtual VOID    FormatV( _In_z_ _Printf_format_string_ LPCWSTR wstrFormat, va_list pArgList );    

    // Render the console texture onto the screen using current view port
    VOID            RenderToScreen( D3DDeviceContext* pCtx );

    // Render the console texture, it is safe to call this every frame
    VOID            RenderToTexture( D3DDeviceContext* pCtx );

    // Get the shader resource view of the console texture, this is useful if you want to composite the console in your own UI
    ID3D11ShaderResourceView* GetResourceView() { return m_pRV; }    

    // method for scrolling the text window up/down
    VOID            ScrollUp( INT nLines );    

    // Send text written to the console also to the debug channel
    VOID            SendOutputToDebugChannel( BOOL bOutputToDebugChannel )
    {
        m_bOutputToDebugChannel = bOutputToDebugChannel;
    }

    static const INT		PAGE_UP    = +255;
    static const INT		PAGE_DOWN  = -255;


private:
    Draw*                   m_pDraw;
    BOOL                    m_bNeedReleaseDraw;
    
    static const UINT		SAFE_AREA_PCT_4x3        = 85;
    static const UINT		SAFE_AREA_PCT_HDTV       = 90;

    // Console texture dimensions
    UINT                    m_width;
    UINT                    m_height;

    // Safe area dimensions
    UINT					m_xSafeArea;
    UINT					m_ySafeArea;

    UINT					m_xSafeAreaOffset;
    UINT					m_ySafeAreaOffset;

    // Send console output to debug channel
    BOOL					m_bOutputToDebugChannel;        

    // Font for rendering text
    BitmapFont*				m_pFont;

    // Colors
	DWORD					m_backColor;
    DWORD					m_textColor;

    // Text Buffers
    UINT					m_ScreenHeight;         // height in lines of screen area
    UINT					m_ScreenHeightVirtual;  // height in lines of text storage buffer
    UINT					m_ScreenWidth;          // width in characters
    FLOAT					m_lineHeight;           // height of a single line in pixels

    WCHAR*					m_Buffer;               // buffer big enough to hold a full screen
    WCHAR**					m_Lines;                // pointers to individual lines
    UINT					m_CurLine;              // index of current line being written to
    UINT					m_CurLineLength;        // length of the current line
    INT						m_ScrollOffset;         // offset to display text (in lines)   

    BOOL					m_bDirty;               // new text has arrived since last RenderToTexture    

    CRITICAL_SECTION		m_CsConsoleBuf;

    ID3D11SamplerState*     m_pSamplerState;

    ID3D11Texture2D*          m_pTexture;
    ID3D11ShaderResourceView* m_pRV;
    ID3D11RenderTargetView*   m_pRT;
    ID3D11DepthStencilState*  m_pDSState;

    // Add a character to the current line
    VOID            Add( CHAR ch );
    VOID            Add( WCHAR wch );

    // Increment to the next line
    VOID            IncrementLine();    
};

}  // namespace XboxSampleFramework


#endif
