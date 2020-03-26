/*
-----------------------------------------------------------------------------
This source file is part of Open Game Engine 2D.
It is licensed under the terms of the MIT license.
For the latest info, see http://oge2d.sourceforge.net

Copyright (c) 2010-2012 Lin Jia Jun (Joe Lam)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "ogeMovie.h"
#include "ogeCommon.h"

#ifdef __OGE_WITH_MOVIE__
static CogeVideo* g_pMovieVideo = NULL;
#endif

/*------------------ Global functions ------------------*/

#if defined(__OGE_WITH_SDL2__) && defined(__OGE_WITH_MOVIE__)
static void UpdateMovieWindow( SDL_Surface* surface, Sint32 x, Sint32 y, Uint32 w, Uint32 h )
{
#ifdef __OGE_WITH_GLWIN__
    //OGE_Log("Calling UpdateMovieWindow(): %d ... ", (int)surface);
    if(g_pMovieVideo) return g_pMovieVideo->UpdateRenderer(surface, x, y, w, h);
#else
    SDL_UpdateRect(surface, x, y, w, h);
#endif
}
#endif

/*------------------ CogeMoviePlayer ------------------*/

CogeMoviePlayer::CogeMoviePlayer()
{
    m_pVideo = NULL;
    m_pAudio = NULL;
    m_pCurrentMovie = NULL;

    m_bVideoIsOK = false;
    m_bAudioIsOK = false;

    m_iAudioFormat = 0;
    m_iAudioFreq = 0;
    m_iAudioChannels = 0;

    m_iMovieVolume = -1; // by default ...

    m_bIsWorking = false;

    m_iState = -1;

}

CogeMoviePlayer::~CogeMoviePlayer()
{
    Finalize();
}

#ifdef __OGE_WITH_MOVIE__

int CogeMoviePlayer::Initialize(CogeVideo* pVideo, CogeAudio* pAudio)
{
    Finalize();

    m_pVideo = pVideo;
    m_pAudio = pAudio;

    g_pMovieVideo = m_pVideo;

    m_bVideoIsOK = pVideo->GetState() >= 0;
    m_bAudioIsOK = pAudio->GetState() >= 0;

    if(m_bAudioIsOK)
    {
        Mix_QuerySpec(&m_iAudioFreq, &m_iAudioFormat, &m_iAudioChannels);
        m_iMovieVolume = pAudio->GetMusicVolume();
    }

    if(m_bVideoIsOK) m_iState = 0;

    return m_iState;
}

#else

int CogeMoviePlayer::Initialize(CogeVideo* pVideo, CogeAudio* pAudio)
{
    OGE_Log("Movie player is not available. \n");

    Finalize();

    return m_iState;
}

#endif

void CogeMoviePlayer::Finalize()
{
    StopMovie();

    m_bIsWorking = false;

    m_iState = -1;

    m_pVideo = NULL;
    m_pAudio = NULL;
    m_pCurrentMovie = NULL;

    m_bVideoIsOK = false;
    m_bAudioIsOK = false;

    m_iAudioFormat = 0;
    m_iAudioFreq = 0;
    m_iAudioChannels = 0;

    m_iMovieVolume = 0;

    DelAllMovie();
}

void CogeMoviePlayer::DelAllMovie()
{
    StopMovie();
    m_pCurrentMovie = NULL;
    ogeMovieMap::iterator it;
	CogeMovie* pMatchedMovie = NULL;

	it = m_MovieMap.begin();

	while (it != m_MovieMap.end())
	{
		pMatchedMovie = it->second;
		m_MovieMap.erase(it);
		delete pMatchedMovie;
		it = m_MovieMap.begin();
	}
}

bool CogeMoviePlayer::IsVideoOK()
{
    return m_bVideoIsOK;
}

bool CogeMoviePlayer::IsAudioOK()
{
    return m_bAudioIsOK;
}

int CogeMoviePlayer::GetState()
{
    return m_iState;
}


#ifdef __OGE_WITH_MOVIE__

CogeMovie* CogeMoviePlayer::NewMovie(const std::string& sMovieName, const std::string& sFileName)
{
    if(!m_bVideoIsOK) return NULL;

	ogeMovieMap::iterator it;

    it = m_MovieMap.find(sMovieName);
	if (it != m_MovieMap.end())
	{
        OGE_Log("The Movie name '%s' is in use.\n", sMovieName.c_str());
        return NULL;
	}

	CogeMovie* pNewMovie = NULL;

	if (sFileName.length() > 0)
	{
	    SMPEG_Info mpg_info;
	    pNewMovie = new CogeMovie(sMovieName, this);
	    pNewMovie->m_pMovie = SMPEG_new(sFileName.c_str(), &mpg_info, 0);
		if(pNewMovie->m_pMovie == NULL)
        {
            delete pNewMovie;
            pNewMovie = NULL;
            OGE_Log("Unable to load movie file: %s\n", sFileName.c_str());
            return NULL;
        }
        else
        {
            pNewMovie->m_AudioSpec.format = m_iAudioFormat;
            pNewMovie->m_AudioSpec.freq = m_iAudioFreq;
            pNewMovie->m_AudioSpec.channels = m_iAudioChannels;
        }

	}
	else return NULL;

	if(pNewMovie)
	{
	    pNewMovie->m_iState = 0;
	    m_MovieMap.insert(ogeMovieMap::value_type(sMovieName, pNewMovie));
	}

	return pNewMovie;

}
CogeMovie* CogeMoviePlayer::NewMovieFromBuffer(const std::string& sMovieName, char* pBuffer, int iBufferSize)
{
    if(!m_bVideoIsOK) return NULL;

	ogeMovieMap::iterator it;

    it = m_MovieMap.find(sMovieName);
	if (it != m_MovieMap.end())
	{
        OGE_Log("The Movie name '%s' is in use.\n", sMovieName.c_str());
        return NULL;
	}

	CogeMovie* pNewMovie = NULL;

	if(pBuffer != NULL && iBufferSize > 0)
	{
	    SMPEG_Info mpg_info;
	    pNewMovie = new CogeMovie(sMovieName, this);
	    pNewMovie->m_pMovie = SMPEG_new_data(pBuffer, iBufferSize, &mpg_info, 0);
		if(pNewMovie->m_pMovie == NULL)
        {
            delete pNewMovie;
            pNewMovie = NULL;
            OGE_Log("Unable to load movie from buffer: %s\n", sMovieName.c_str());
            return NULL;
        }
        else
        {
            pNewMovie->m_AudioSpec.format = m_iAudioFormat;
            pNewMovie->m_AudioSpec.freq = m_iAudioFreq;
            pNewMovie->m_AudioSpec.channels = m_iAudioChannels;
        }

	}
	else return NULL;

	if(pNewMovie)
	{
	    pNewMovie->m_iState = 0;
	    m_MovieMap.insert(ogeMovieMap::value_type(sMovieName, pNewMovie));
	}

	return pNewMovie;
}

#else

CogeMovie* CogeMoviePlayer::NewMovie(const std::string& sMovieName, const std::string& sFileName)
{
    return NULL;
}
CogeMovie* CogeMoviePlayer::NewMovieFromBuffer(const std::string& sMovieName, char* pBuffer, int iBufferSize)
{
    return NULL;
}

#endif


CogeMovie* CogeMoviePlayer::FindMovie(const std::string& sMovieName)
{
    ogeMovieMap::iterator it;
    it = m_MovieMap.find(sMovieName);
	if (it != m_MovieMap.end()) return it->second;
	else return NULL;
}
CogeMovie* CogeMoviePlayer::GetMovie(const std::string& sMovieName, const std::string& sFileName)
{
    CogeMovie* pMatchedMovie = FindMovie(sMovieName);
    if (!pMatchedMovie) pMatchedMovie = NewMovie(sMovieName, sFileName);
    return pMatchedMovie;
}
CogeMovie* CogeMoviePlayer::GetMovieFromBuffer(const std::string& sMovieName, char* pBuffer, int iBufferSize)
{
    CogeMovie* pMatchedMovie = FindMovie(sMovieName);
    if (!pMatchedMovie) pMatchedMovie = NewMovieFromBuffer(sMovieName, pBuffer, iBufferSize);
    return pMatchedMovie;
}
int CogeMoviePlayer::DelMovie(const std::string& sMovieName)
{
    CogeMovie* pMatchedMovie = FindMovie(sMovieName);
	if (pMatchedMovie)
	{
	    pMatchedMovie->Stop();
		m_MovieMap.erase(sMovieName);
		delete pMatchedMovie;
		return 1;
	}
	return -1;
}

void CogeMoviePlayer::StopMovie()
{
    if(m_iState < 0) return;
    if(m_pCurrentMovie) m_pCurrentMovie->Stop();
}
void CogeMoviePlayer::PauseMovie()
{
    if(m_iState < 0) return;
    if(m_pCurrentMovie) m_pCurrentMovie->Pause();
}
void CogeMoviePlayer::ResumeMovie()
{
    if(m_iState < 0) return;
    if(m_pCurrentMovie) m_pCurrentMovie->Resume();
}

int CogeMoviePlayer::PrepareMovie(const std::string& sMovieName)
{
    CogeMovie* pMovie = FindMovie(sMovieName);
    if(pMovie == NULL) return -1;
    if(m_pCurrentMovie == pMovie) return 0;
    StopMovie();
    m_pCurrentMovie = pMovie;
    return 1;
}
int CogeMoviePlayer::PrepareMovie(CogeMovie* pTheMovie)
{
    if(pTheMovie == NULL) return -1;
    if(m_pCurrentMovie == pTheMovie) return 0;
    StopMovie();
    m_pCurrentMovie = pTheMovie;
    return 1;
}

CogeMovie* CogeMoviePlayer::GetCurrentMovie()
{
    return m_pCurrentMovie;
}

int CogeMoviePlayer::GetMovieVolume()
{
    return m_iMovieVolume;
}
void CogeMoviePlayer::SetMovieVolume(int iValue)
{
    m_iMovieVolume = iValue;
}

int CogeMoviePlayer::PlayMovie(int iLoopTimes)
{
    if(m_iState < 0) return m_iState;
    if(m_pCurrentMovie) return m_pCurrentMovie->Play(iLoopTimes);
    else return -1;
}


#ifdef __OGE_WITH_MOVIE__

bool CogeMoviePlayer::IsPlaying()
{
    if(m_iState < 0 || m_pCurrentMovie == NULL) return false;
    else return SMPEG_status(m_pCurrentMovie->m_pMovie) == SMPEG_PLAYING;
}

#else

bool CogeMoviePlayer::IsPlaying()
{
    return false;
}

#endif


/*------------------ CogeMovie ------------------*/

#ifdef __OGE_WITH_MOVIE__

CogeMovie::CogeMovie(const std::string& sName, CogeMoviePlayer* pPlayer)
{
    m_pPlayer = pPlayer;
    m_pMovie = NULL;
    m_sName = sName;
    m_iState = -1;
}
CogeMovie::~CogeMovie()
{
    if(m_pPlayer && m_pMovie)
    {
        if(m_pPlayer->m_pCurrentMovie == this)
        {
            if(SMPEG_status(m_pMovie) == SMPEG_PLAYING) Stop();
            m_pPlayer->m_pCurrentMovie = NULL;
        }
    }

    if(m_pMovie)
    {
        SMPEG_delete(m_pMovie);
        m_pMovie = NULL;
    }
}

int CogeMovie::GetState()
{
    return m_iState;
}
const std::string& CogeMovie::GetName()
{
    return m_sName;
}

int CogeMovie::Play(int iLoopTimes)
{
    if(m_pPlayer == NULL || m_pMovie == NULL) return -1;

    if(m_pPlayer->IsVideoOK() == false) return -1;

    if(SMPEG_status(m_pMovie) == SMPEG_PLAYING) return 0;

    m_pPlayer->m_pVideo->Pause();

    SDL_Surface* pDisplaySurface = (SDL_Surface*) m_pPlayer->m_pVideo->GetRawSurface();

    if(pDisplaySurface == NULL) return -1;

#ifdef __OGE_WITH_SDL2__
    //OGE_Log("Setting callback ...");
    SMPEG_setdisplay(m_pMovie, pDisplaySurface, NULL, UpdateMovieWindow);
#else
    SMPEG_setdisplay(m_pMovie, pDisplaySurface, NULL, NULL);
#endif

    if(m_pPlayer->IsAudioOK())
    {
        m_pPlayer->m_pAudio->StopAllMusic();

        SMPEG_actualSpec(m_pMovie, &m_AudioSpec);

        Mix_HookMusic(SMPEG_playAudioSDL, m_pMovie);

        SMPEG_enableaudio(m_pMovie, 1);

        if(m_pPlayer->m_iMovieVolume >= 0) SMPEG_setvolume(m_pMovie, m_pPlayer->m_iMovieVolume);
    }

    if(iLoopTimes > 0) SMPEG_loop(m_pMovie, iLoopTimes);
    else SMPEG_loop(m_pMovie, 0);

    SMPEG_play(m_pMovie);

    return 0;
}
int CogeMovie::Pause()
{
    if(m_pMovie) SMPEG_pause(m_pMovie);
    return 0;
}
int CogeMovie::Resume()
{
    if(m_pMovie) SMPEG_play(m_pMovie);
    return 0;
}
int CogeMovie::Stop()
{
    if(m_pMovie) SMPEG_stop(m_pMovie);
    if(m_pPlayer->IsAudioOK())
    {
        Mix_HookMusic(NULL, NULL);
        m_pPlayer->m_pAudio->StopAllMusic();
    }
    if(m_pPlayer->IsVideoOK()) m_pPlayer->m_pVideo->Resume();
    return 0;
}

int CogeMovie::Rewind()
{
    if(m_pMovie) SMPEG_rewind(m_pMovie);
    return 0;
}

#else

CogeMovie::CogeMovie(const std::string& sName, CogeMoviePlayer* pPlayer)
{
    m_pPlayer = NULL;
    m_pMovie = NULL;
    m_sName = sName;
    m_iState = -1;
}
CogeMovie::~CogeMovie()
{
    m_pPlayer = NULL;
    m_pMovie = NULL;
    m_iState = -1;
}

int CogeMovie::GetState()
{
    return m_iState;
}
const std::string& CogeMovie::GetName()
{
    return m_sName;
}

int CogeMovie::Play(int iLoopTimes)
{
    return -1;
}
int CogeMovie::Pause()
{
    return 0;
}
int CogeMovie::Resume()
{
    return 0;
}
int CogeMovie::Stop()
{
    return 0;
}

int CogeMovie::Rewind()
{
    return 0;
}


#endif


