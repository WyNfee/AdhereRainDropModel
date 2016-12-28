//--------------------------------------------------------------------------------------
// Audio.h
//
// Helper functions to statically load audio files into memory, parse them, and provide rudimentary playback.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_AUDIO_H_INCLUDED
#define XSF_AUDIO_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

namespace XboxSampleFramework
{
#ifdef _XBOX_ONE
//--------------------------------------------------------------------------------------
// FourCC definitions
//--------------------------------------------------------------------------------------
const DWORD ATG_FOURCC_RIFF = 'FFIR'; // 'RIFF'
const DWORD ATG_FOURCC_WAVE = 'EVAW'; // 'WAVE'
const DWORD ATG_FOURCC_FORMAT = ' tmf'; // 'fmt '
const DWORD ATG_FOURCC_DATA = 'atad'; // 'data'

//--------------------------------------------------------------------------------------
// Misc type definitions
//--------------------------------------------------------------------------------------
typedef DWORD FOURCC, *PFOURCC, *LPFOURCC;

//--------------------------------------------------------------------------------------
// For parsing WAV files
//--------------------------------------------------------------------------------------
struct RIFFHEADER
{
    FOURCC fccChunkId;
    DWORD dwDataSize;
};

#define RIFFCHUNK_FLAGS_VALID   0x00000001
#endif

struct ISoundCallback;

//--------------------------------------------------------------------------------------
// Name: class Sound
// Desc: Class to provide rudimentary playback of sounds.
//--------------------------------------------------------------------------------------
class Sound
{
public:
    Sound();
    ~Sound();

    _Check_return_
    static HRESULT InitializeXAudio2();

#ifdef _XBOX_ONE
    _Check_return_
    HRESULT Initialize( _In_z_ const WCHAR* fileName, BOOL bLoop = FALSE, _In_opt_ ISoundCallback* pSoundCallback = NULL );
#else
    _Check_return_
    HRESULT Initialize( _In_z_ const WCHAR*, BOOL, _In_opt_ ISoundCallback* );
#endif
    HRESULT Destroy();

#ifdef _XBOX_ONE
    void SetLoop( BOOL bLoop )
    {
        m_bLoop = bLoop;
#else
    void SetLoop( BOOL )
    {
#endif
    }

    // no-op if sound is already playing
    HRESULT Play();
    // no-op if sound is already paused
    HRESULT Pause();
    // no-op if sound is already stopped
    HRESULT Stop();

private:
#ifdef _XBOX_ONE
    static IXAudio2*                 s_pXAudio2;
    static IXAudio2MasteringVoice*     s_pMasteringVoice;
    IXAudio2SourceVoice*             m_pSourceVoice;
    IXAudio2VoiceCallback*             m_pSourceVoiceCallback;
    BYTE*                            m_pbWaveData;
    DWORD                            m_cbWaveSize;
    BOOL                             m_bLoop;
    BOOL                             m_bInitialized;
#endif

    HRESULT SubmitBuffer();
};

//--------------------------------------------------------------------------------------
// Name: struct ISoundCallback
// Desc: Callback definitions for sound objects
//--------------------------------------------------------------------------------------
struct ISoundCallback
{
    // This is called in the context of an XAudio2 callback.
    // If you want to call 'Destroy' you'll need to do it on another thread.
    // It follows the XAudio2 convention: this is called only when looping is disabled
    // and the sound reaches the end.
    virtual void OnPlaybackEnd(_In_ Sound* pSender) = 0;
};

#ifdef _XBOX_ONE
//--------------------------------------------------------------------------------------
// Name: class RiffChunk
// Desc: RIFF chunk utility class
//--------------------------------------------------------------------------------------
class RiffChunk
{
    const RiffChunk* m_pParentChunk;     // Parent chunk
    HANDLE m_hFile;
    FOURCC m_fccChunkId;       // Chunk identifier
    DWORD m_dwDataOffset;     // Chunk data offset
    DWORD m_dwDataSize;       // Chunk data size
    DWORD m_dwFlags;          // Chunk flags

public:
    RiffChunk();

    VOID Initialize( FOURCC fccChunkId,
                     _In_opt_ const RiffChunk* pParentChunk,
                     HANDLE hFile );

    _Check_return_
    HRESULT Open();

    BOOL IsValid() const
    {
        return !!( m_dwFlags & RIFFCHUNK_FLAGS_VALID );
    }

    _Check_return_
    HRESULT ReadData( LONG lOffset,
                      _Out_writes_(dwDataSize) VOID* pData,
                      DWORD dwDataSize,
                      _In_opt_ OVERLAPPED* pOL ) const;

    FOURCC GetChunkId() const
    {
        return m_fccChunkId;
    }

    DWORD GetDataSize() const
    {
        return m_dwDataSize;
    }

    DWORD GetDataOffset() const
    {
        return m_dwDataOffset;
    }

private:
    // prevent copying so that we don't have to duplicate file handles
    RiffChunk( const RiffChunk& );
    RiffChunk& operator =( const RiffChunk& );
};

//--------------------------------------------------------------------------------------
// Name: class WaveFile
// Desc: Wave file utility class
//--------------------------------------------------------------------------------------
class WaveFile
{
    HANDLE m_hFile;            // File handle
    RiffChunk m_RiffChunk;        // RIFF chunk
    RiffChunk m_FormatChunk;      // Format chunk
    RiffChunk m_DataChunk;        // Data chunk

public:
    WaveFile();
    ~WaveFile();

    HRESULT Open( _In_z_ LPCWSTR strFileName );

    VOID    Close();

    HRESULT GetFormat( _Out_ WAVEFORMATEX* pwfxFormat, _In_ size_t maxsize ) const;

    DWORD GetFormatSize() const
    {
        return m_FormatChunk.GetDataSize();
    }

    HRESULT ReadSample( DWORD dwPosition,
        _Out_writes_(dwBufferSize) VOID* pBuffer,
        DWORD dwBufferSize,
        _Out_opt_ DWORD* pdwRead ) const;

    DWORD GetDuration() const
    {
        return m_DataChunk.GetDataSize();
    }

    DWORD GetWaveDataOffset() const
    {
        return m_DataChunk.GetDataOffset();
    }

private:
    // prevent copying so that we don't have to duplicate file handles
    WaveFile( const WaveFile& );
    WaveFile& operator =( const WaveFile& );
};
#endif

} // namespace XboxSampleFramework

#endif // XSF_AUDIO_H_INCLUDED
