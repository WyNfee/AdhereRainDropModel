//--------------------------------------------------------------------------------------
// Audio.cpp
//
// Sound class for samples. For details, see header.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "Audio.h"

using namespace XboxSampleFramework;

#ifdef _XBOX_ONE                              
IXAudio2*               Sound::s_pXAudio2        = NULL;
IXAudio2MasteringVoice* Sound::s_pMasteringVoice = NULL;

//--------------------------------------------------------------------------------------
// Name: class AudioVoiceCallback
// Desc: Calls the client's OnPlayEnd
//--------------------------------------------------------------------------------------
class AudioVoiceCallback : public IXAudio2VoiceCallback
{
public:
    virtual void WINAPI OnVoiceProcessingPassStart( UINT32 ) override {}
    virtual void WINAPI OnVoiceProcessingPassEnd() override {}
    virtual void WINAPI OnStreamEnd()
    {
        if (m_pSoundCallback)
        {
            m_pSoundCallback->OnPlaybackEnd( m_pSound );
        }
    }
    virtual void WINAPI OnBufferStart( void* ) override {}
    virtual void WINAPI OnBufferEnd( void* ) override {}
    virtual void WINAPI OnLoopEnd( void* ) override {}
    virtual void WINAPI OnVoiceError( void*, HRESULT ) override {}

    AudioVoiceCallback( _In_ Sound* pSound, _In_ ISoundCallback* pSoundCallback )
        : m_pSound( pSound ), m_pSoundCallback( pSoundCallback )
    {
    }

    virtual ~AudioVoiceCallback() {}

private:
    Sound*         m_pSound;
    ISoundCallback* m_pSoundCallback;
};
#endif

//--------------------------------------------------------------------------------------
// Name: Sound()
// Desc: Constructor
//--------------------------------------------------------------------------------------

Sound::Sound()
#ifdef _XBOX_ONE
    : m_pSourceVoice( NULL ), m_pSourceVoiceCallback( NULL ), m_pbWaveData( NULL ), m_cbWaveSize( 0 ), m_bInitialized( FALSE ), m_bLoop( FALSE )
#endif
{
}


//--------------------------------------------------------------------------------------
// Name: ~Sound()
// Desc: Destructor
//--------------------------------------------------------------------------------------

Sound::~Sound()
{
#ifdef _XBOX_ONE
    if ( m_pSourceVoice )
    {
        m_pSourceVoice->DestroyVoice();
    }
    delete[] m_pbWaveData;
    delete m_pSourceVoiceCallback;
#endif
}


//--------------------------------------------------------------------------------------
// Name: InitializeXAudio2()
// Desc: Initialize XAudio2
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sound::InitializeXAudio2()
{
    HRESULT hr = S_OK;
#ifdef _XBOX_ONE
    if ( s_pXAudio2 == NULL)
    {
        hr = XAudio2Create( &s_pXAudio2, 0 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    if ( s_pMasteringVoice == NULL )
    {
        hr = s_pXAudio2->CreateMasteringVoice( &s_pMasteringVoice );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
#endif
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize source voice structures and load the wave data
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
#ifdef _XBOX_ONE
HRESULT Sound::Initialize( const WCHAR* fileName, BOOL bLoop, ISoundCallback* pSoundCallback )
{
    if ( s_pXAudio2 == NULL )
    {
        return ERROR_INVALID_FUNCTION;
    }

    if ( m_bInitialized )
    {
        return ERROR_ALREADY_INITIALIZED;
    }

    if ( pSoundCallback == NULL )
    {
        m_pSourceVoiceCallback = NULL;
    }
    else
    {
        m_pSourceVoiceCallback = new AudioVoiceCallback( this, pSoundCallback );
    }

    //
    // Read the wave file
    //
    WaveFile WaveFile;
    XSF_ERROR_IF_FAILED( WaveFile.Open( fileName ) );

    // Read the format header
    BYTE header[64];
    WAVEFORMATEX* wfx = reinterpret_cast<WAVEFORMATEX*>( &header );

    XSF_ERROR_IF_FAILED( WaveFile.GetFormat( wfx, sizeof(header) ) );

    // Calculate how many bytes and samples are in the wave
    m_cbWaveSize = WaveFile.GetDuration();

    // Read the sample data into memory
    m_pbWaveData = new BYTE[ m_cbWaveSize ];
    XSF_ERROR_IF_FAILED( WaveFile.ReadSample( 0, m_pbWaveData, m_cbWaveSize, &m_cbWaveSize ) );

    // Create the source voice
    if (FAILED( s_pXAudio2->CreateSourceVoice( &m_pSourceVoice, wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, m_pSourceVoiceCallback ) ) )
    {
        XSF_SAFE_DELETE_ARRAY( m_pbWaveData );
        return E_FAIL;
    }

    m_bLoop = bLoop;

    XSF_ERROR_IF_FAILED( SubmitBuffer() );

    m_bInitialized = TRUE;
#else
HRESULT Sound::Initialize( const WCHAR*, BOOL, ISoundCallback* )
{
#endif
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Play()
// Desc: Start playback of the loaded sound
//--------------------------------------------------------------------------------------
HRESULT Sound::Play()
{
#ifdef _XBOX_ONE
    if ( !m_bInitialized )
    {
        return ERROR_INVALID_FUNCTION;
    }

    //
    // This call is a no-op if the voice is already in started state.
    //
    XSF_ERROR_IF_FAILED( m_pSourceVoice->Start( 0 ) );
#endif
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Pause()
// Desc: Pause playback of the loaded sound
//--------------------------------------------------------------------------------------
HRESULT Sound::Pause()
{
#ifdef _XBOX_ONE
    if ( !m_bInitialized )
    {
        return ERROR_INVALID_FUNCTION;
    }

    //
    // This call is a no-op if the voice is already in stopped state.
    // 'Stop' effectively pauses the source voice until a 'Start' is issued.
    //
    XSF_ERROR_IF_FAILED( m_pSourceVoice->Stop( 0 ) );
#endif
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: Stop()
// Desc: Stop playback of the loaded sound
//--------------------------------------------------------------------------------------
HRESULT Sound::Stop()
{
#ifdef _XBOX_ONE
    if ( !m_bInitialized )
    {
        return ERROR_INVALID_FUNCTION;
    }

    XSF_ERROR_IF_FAILED( Pause() );

    XSF_ERROR_IF_FAILED( m_pSourceVoice->FlushSourceBuffers() );

    XSF_ERROR_IF_FAILED( SubmitBuffer() );
#endif
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SubmitBuffer()
// Desc: Submits the buffer to XAudio2
//--------------------------------------------------------------------------------------
HRESULT Sound::SubmitBuffer()
{
#ifdef _XBOX_ONE
    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = m_pbWaveData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = m_cbWaveSize;
    if ( m_bLoop )
    {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    XSF_ERROR_IF_FAILED( m_pSourceVoice->SubmitSourceBuffer( &buffer ) );
#endif
    return S_OK;
}

#ifdef _XBOX_ONE
//--------------------------------------------------------------------------------------
// Name: RiffChunk()
// Desc: Constructor
//--------------------------------------------------------------------------------------
RiffChunk::RiffChunk()
{
    // Initialize defaults
    m_fccChunkId = 0;
    m_pParentChunk = NULL;
    m_hFile = INVALID_HANDLE_VALUE;
    m_dwDataOffset = 0;
    m_dwDataSize = 0;
    m_dwFlags = 0;
}


//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes the Riff chunk for use
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID RiffChunk::Initialize( FOURCC fccChunkId, const RiffChunk* pParentChunk,
                            HANDLE hFile )
{
    m_fccChunkId = fccChunkId;
    m_pParentChunk = pParentChunk;
    m_hFile = hFile;
}


//--------------------------------------------------------------------------------------
// Name: Open()
// Desc: Opens an existing chunk
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT RiffChunk::Open()
{
    LONG lOffset = 0;

    // Seek to the first byte of the parent chunk's data section
    if( m_pParentChunk )
    {
        lOffset = m_pParentChunk->m_dwDataOffset;

        // Special case the RIFF chunk
        if( ATG_FOURCC_RIFF == m_pParentChunk->m_fccChunkId )
        {
            lOffset += sizeof( FOURCC );
        }
    }

    // Read each child chunk header until we find the one we're looking for
    for( ; ; )
    {
        LARGE_INTEGER Offset = { static_cast<DWORD>(lOffset), 0 };
        if( INVALID_SET_FILE_POINTER == SetFilePointerEx( m_hFile, Offset, NULL, FILE_BEGIN ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    
        RIFFHEADER rhRiffHeader;
        DWORD dwRead;
        if( 0 == ReadFile( m_hFile, &rhRiffHeader, sizeof( rhRiffHeader ), &dwRead, NULL ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        // Hit EOF without finding it
        if( 0 == dwRead )
        {
            return ERROR_FILE_CORRUPT;
        }

        // Check if we found the one we're looking for
        if( m_fccChunkId == rhRiffHeader.fccChunkId )
        {
            // Save the chunk size and data offset
            m_dwDataOffset = lOffset + sizeof( rhRiffHeader );
            m_dwDataSize = rhRiffHeader.dwDataSize;

            // Success
            m_dwFlags |= RIFFCHUNK_FLAGS_VALID;

            return S_OK;
        }

        lOffset += sizeof( rhRiffHeader ) + rhRiffHeader.dwDataSize;
    }
}


//--------------------------------------------------------------------------------------
// Name: ReadData()
// Desc: Reads from the file
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT RiffChunk::ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize, OVERLAPPED* pOL ) const
{
    HRESULT hr = S_OK;

    OVERLAPPED defaultOL = {0};
    OVERLAPPED* pOverlapped = pOL;
    if( !pOL )
    {
        pOverlapped = &defaultOL;
    }

    // Seek to the offset
    pOverlapped->Offset = m_dwDataOffset + lOffset;

    // Read from the file
    DWORD dwRead;
    if( 0 == ReadFile( m_hFile, pData, dwDataSize, &dwRead, pOverlapped ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    if( SUCCEEDED( hr ) && !pOL )
    {
        // we're using the default overlapped structure, which means that even if the
        // read was async, we need to act like it was synchronous.
        if( !GetOverlappedResultEx( m_hFile, pOverlapped, &dwRead, INFINITE, FALSE) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    return hr;
}


//--------------------------------------------------------------------------------------
// Name: WaveFile()
// Desc: Constructor
//--------------------------------------------------------------------------------------
WaveFile::WaveFile()
{
    m_hFile = INVALID_HANDLE_VALUE;
}


//--------------------------------------------------------------------------------------
// Name: ~WaveFile()
// Desc: Denstructor
//--------------------------------------------------------------------------------------
WaveFile::~WaveFile()
{
    Close();
}


//--------------------------------------------------------------------------------------
// Name: Open()
// Desc: Initializes the object
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT WaveFile::Open( LPCWSTR strFileName )
{
    // If we're already open, close
    Close();

	WCHAR tmp[ 1024 ];
	_snwprintf_s( tmp, _countof( tmp ), _TRUNCATE, L"%s%s", XSF::GetContentFileRoot(), strFileName );

    // Open the file
    m_hFile = CreateFile2(
        tmp,
        GENERIC_READ,
        FILE_SHARE_READ,
        OPEN_EXISTING,
        NULL );

    if( INVALID_HANDLE_VALUE == m_hFile )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Initialize the chunk objects
    m_RiffChunk.Initialize( ATG_FOURCC_RIFF, NULL, m_hFile );
    m_FormatChunk.Initialize( ATG_FOURCC_FORMAT, &m_RiffChunk, m_hFile );
    m_DataChunk.Initialize( ATG_FOURCC_DATA, &m_RiffChunk, m_hFile );

    XSF_RETURN_IF_FAILED( m_RiffChunk.Open() );

    XSF_RETURN_IF_FAILED( m_FormatChunk.Open() );

    XSF_RETURN_IF_FAILED( m_DataChunk.Open() );

    // Validate the file type
    FOURCC fccType;
    XSF_RETURN_IF_FAILED( m_RiffChunk.ReadData( 0, &fccType, sizeof( fccType ), NULL ) );

    if( ATG_FOURCC_WAVE != fccType )
    {
        // Note this code does not support loading xWMA files (which use 'XWMA' instead of 'WAVE')
        return HRESULT_FROM_WIN32( ERROR_UNSUPPORTED_TYPE );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: GetFormat()
// Desc: Gets the wave file format.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT WaveFile::GetFormat( WAVEFORMATEX* pwfxFormat, size_t maxsize ) const
{
    if ( !pwfxFormat || ( maxsize < sizeof(WAVEFORMATEX) ) )
    {
        return E_INVALIDARG;
    }

    DWORD dwValidSize = m_FormatChunk.GetDataSize();

    // Must be at least as large as a WAVEFORMAT to be valid
    if( dwValidSize < sizeof( WAVEFORMAT ) )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    // Need enough space to load format
    if ( dwValidSize > maxsize )
    {
        return E_FAIL;
    }

    // Read the format chunk into the buffer
    HRESULT hr = m_FormatChunk.ReadData( 0, pwfxFormat, dwValidSize, NULL );
    if( FAILED( hr ) )
    {
        return hr;
    }

    switch( pwfxFormat->wFormatTag )
    {
        case WAVE_FORMAT_PCM:
        case WAVE_FORMAT_IEEE_FLOAT:
            // PCMWAVEFORMAT (16 bytes) or WAVEFORMATEX (18 bytes)
            if( dwValidSize < sizeof( PCMWAVEFORMAT ) )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }
            break;

        case WAVE_FORMAT_ADPCM:
            if( dwValidSize < sizeof( ADPCMWAVEFORMAT ) )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }
            break;

        case WAVE_FORMAT_EXTENSIBLE:
            if( dwValidSize < sizeof( WAVEFORMATEXTENSIBLE ) )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }
            else
            {
                static const GUID s_wfexBase = {0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(pwfxFormat);

                if ( memcmp( reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                             reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD) ) != 0 )
                {
                    // Unsupported!
                    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
                }

                switch( wfex->SubFormat.Data1 )
                {
                case WAVE_FORMAT_PCM:
                case WAVE_FORMAT_IEEE_FLOAT:
                case WAVE_FORMAT_ADPCM:
                case WAVE_FORMAT_XMA2:
                    break;

                default:
                    // Unsupported!
                    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
                }
            }
            break;

        case WAVE_FORMAT_XMA2:
            if ( dwValidSize < sizeof( XMA2WAVEFORMATEX ) )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }
            break;

        default:
            // Unsupported!
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    // Zero out remaining bytes, in case enough bytes were not read
    if( dwValidSize < maxsize )
    {
        ZeroMemory( ( BYTE* )pwfxFormat + dwValidSize, maxsize - dwValidSize );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: ReadSample()
// Desc: Reads data from the audio file.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT WaveFile::ReadSample( DWORD dwPosition, VOID* pBuffer,
                              DWORD dwBufferSize, DWORD* pdwRead ) const
{
    // Don't read past the end of the data chunk
    DWORD dwDuration = GetDuration();

    if( dwPosition + dwBufferSize > dwDuration )
    {
        dwBufferSize = dwDuration - dwPosition;
    }

    HRESULT hr = S_OK;
    if( dwBufferSize )
    {
        hr = m_DataChunk.ReadData( ( LONG )dwPosition, pBuffer, dwBufferSize, NULL );
    }

    if( pdwRead )
    {
        *pdwRead = dwBufferSize;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: Close()
// Desc: Closes the object
//--------------------------------------------------------------------------------------
VOID WaveFile::Close()
{
    if( m_hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hFile );
        m_hFile = INVALID_HANDLE_VALUE;
    }
}
#endif
