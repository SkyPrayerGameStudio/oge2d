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

#ifndef __OGE_AUDIO_H_INCLUDED__
#define __OGE_AUDIO_H_INCLUDED__

#include "SDL.h"
#include "SDL_mixer.h"

#include <string>
#include <map>

class CogeSound;
class CogeMusic;

class CogeAudio
{
private:

    typedef std::map<std::string, CogeSound*> ogeSoundMap;
    typedef std::map<std::string, CogeMusic*> ogeMusicMap;

    ogeSoundMap      m_SoundMap;
    ogeMusicMap      m_MusicMap;

    CogeMusic*       m_pCurrentMusic;

    int m_iState;

    bool m_bAudioIsOK;
    bool m_bAudioIsOpen;

    int m_iAudioRate;       // 22k  = 22050, 44k = 44100
    Uint16 m_iAudioFormat;  // 16-bit stereo = AUDIO_S16
    int m_iAudioChannels;   // 1 for mono, 2 for stereo, 4 for quadraphonic. SDL_Mixer supports up to 8 channels for playback
    int m_iAudioBuffers;    // 4k buffer  = 4096

    int m_iSoundVolume;
    int m_iMusicVolume;

    void DelAllSound();
    void DelAllMusic();


protected:

public:

    CogeAudio();
    ~CogeAudio();

    int Initialize(int iAudioRate = 44100,  int iAudioChannels = 2,
                   int iAudioFormat = AUDIO_S16,  int iAudioBuffers = 4096);
    void Finalize();

    int GetState();

    bool IsAudioOK();
    bool IsAudioOpen();

    CogeSound* NewSound(const std::string& sSoundName, const std::string& sFileName);
    CogeSound* NewSoundFromBuffer(const std::string& sSoundName, char* pBuffer, int iBufferSize);
    CogeSound* FindSound(const std::string& sSoundName);
    CogeSound* GetSound(const std::string& sSoundName, const std::string& sFileName);
    CogeSound* GetSoundFromBuffer(const std::string& sSoundName, char* pBuffer, int iBufferSize);
    int DelSound(const std::string& sSoundName);

    CogeMusic* NewMusic(const std::string& sMusicName, const std::string& sFileName);
    CogeMusic* NewMusicFromBuffer(const std::string& sMusicName, char* pBuffer, int iBufferSize);
    CogeMusic* FindMusic(const std::string& sMusicName);
    CogeMusic* GetMusic(const std::string& sMusicName, const std::string& sFileName);
    CogeMusic* GetMusicFromBuffer(const std::string& sMusicName, char* pBuffer, int iBufferSize);
    int DelMusic(const std::string& sMusicName);

    int GetSoundVolume();
    int GetMusicVolume();
    void SetSoundVolume(int iValue);
    void SetMusicVolume(int iValue);

    void StopAllSound();
    void StopAllMusic();

    void PauseAllSound();
    void PauseAllMusic();

    void ResumeAllSound();
    void ResumeAllMusic();

    int PrepareMusic(const std::string& sMusicName);
    int PrepareMusic(CogeMusic* pTheMusic);
    CogeMusic* GetCurrentMusic();

    int PlayMusic(int iLoopTimes = 0);

    int FadeInMusic(const std::string& sMusicName, int iLoopTimes = 0, int iFadingTime = 1000);
    int FadeOutMusic(int iFadingTime = 1000);


    friend class CogeSound;
    friend class CogeMusic;

};


class CogeMusic
{
private:

    CogeAudio*  m_pAudio;
    Mix_Music*  m_pMusic;

    std::string m_sName;

    int m_iState;

    //int m_iTotalUsers;

    int m_iMusicType;

    int m_iPlayTimes;

    void* m_pDataBuf;

    //int m_iCurrentChannel;

    //void Hire();
    //void Fire();


protected:

public:

    CogeMusic(const std::string& sName, CogeAudio*  pAudio);
    ~CogeMusic();


    int GetState();
    const std::string& GetName();
    //int GetChannel();

    void* GetDataBuffer();
    void SetDataBuffer(void* pData);

    int Play(int iLoopTimes = 0);
    int Pause();
    int Resume();
    int Stop();

    int Rewind();

    int FadeIn(int iLoopTimes = 0, int iFadingTime = 1000);
    int FadeOut(int iFadingTime = 1000);

    friend class CogeAudio;

};


class CogeSound
{
private:

    CogeAudio*  m_pAudio;
    Mix_Chunk*  m_pSound;

    std::string m_sName;

    int m_iState;

    int m_iChannel;

    //int m_iTotalUsers;

    //void Hire();
    //void Fire();


protected:

public:

    CogeSound(const std::string& sName, CogeAudio*  pAudio);
    ~CogeSound();

    int GetState();
    const std::string& GetName();
    //int GetChannel();

    int Play(int iLoopTimes = 0);
    int Pause();
    int Resume();
    int Stop();

    friend class CogeAudio;

};


#endif // __OGE_AUDIO_H_INCLUDED__
