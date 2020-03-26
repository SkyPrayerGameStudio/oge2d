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

#ifndef __OGE_MOVIE_H_INCLUDED__
#define __OGE_MOVIE_H_INCLUDED__

#include "SDL.h"
#include "SDL_mixer.h"

#ifdef __OGE_WITH_MOVIE__
#include "smpeg.h"
#endif

#include "ogeVideo.h"
#include "ogeAudio.h"

#include <string>
#include <map>

class CogeMovie;

class CogeMoviePlayer
{
private:

    typedef std::map<std::string, CogeMovie*> ogeMovieMap;

    CogeVideo*    m_pVideo;
    CogeAudio*    m_pAudio;

    CogeMovie*    m_pCurrentMovie;

    ogeMovieMap   m_MovieMap;

    int m_iState;

    bool m_bVideoIsOK;

    bool m_bAudioIsOK;
    Uint16 m_iAudioFormat;
    int m_iAudioFreq;
    int m_iAudioChannels;

    int m_iMovieVolume;

    bool m_bIsWorking;

    void DelAllMovie();

protected:

public:

    CogeMoviePlayer();
    ~CogeMoviePlayer();

    int Initialize(CogeVideo* pVideo, CogeAudio* pAudio);
    void Finalize();

    bool IsVideoOK();
    bool IsAudioOK();

    int GetState();

    CogeMovie* NewMovie(const std::string& sMovieName, const std::string& sFileName);
    CogeMovie* NewMovieFromBuffer(const std::string& sMovieName, char* pBuffer, int iBufferSize);
    CogeMovie* FindMovie(const std::string& sMovieName);
    CogeMovie* GetMovie(const std::string& sMovieName, const std::string& sFileName);
    CogeMovie* GetMovieFromBuffer(const std::string& sMovieName, char* pBuffer, int iBufferSize);
    int DelMovie(const std::string& sMovieName);

    void StopMovie();
    void PauseMovie();
    void ResumeMovie();

    int PrepareMovie(const std::string& sMovieName);
    int PrepareMovie(CogeMovie* pTheMovie);

    CogeMovie* GetCurrentMovie();

    int GetMovieVolume();
    void SetMovieVolume(int iValue);

    int PlayMovie(int iLoopTimes = 0);

    bool IsPlaying();


    friend class CogeMovie;

};

class CogeMovie
{
private:

    CogeMoviePlayer*  m_pPlayer;

#ifdef __OGE_WITH_MOVIE__
    SMPEG*            m_pMovie;
#else
    void*             m_pMovie;
#endif

    SDL_AudioSpec     m_AudioSpec;

    std::string m_sName;

    int m_iState;

protected:

public:

    CogeMovie(const std::string& sName, CogeMoviePlayer* pPlayer);
    ~CogeMovie();

    int GetState();
    const std::string& GetName();

    int Play(int iLoopTimes = 0);
    int Pause();
    int Resume();
    int Stop();

    int Rewind();

    friend class CogeMoviePlayer;

};


#endif // __OGE_MOVIE_H_INCLUDED__
