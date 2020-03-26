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

#include "ogeAudio.h"
#include "ogeCommon.h"

#define _OGE_MAX_SOUND_CHANNEL_            128

// default frequency is 44K
#define _OGE_SOUND_DEFAULT_FREQUENCY_      44100

/*
static Mix_Chunk* OGE_LoadWAV(const char* filepath)
{
    Mix_Chunk* pResult = NULL;

#ifdef __OGE_WITH_SDL2__

    SDL_RWops *file = SDL_RWFromFile(filepath, "rb");
    if(file) pResult = Mix_LoadWAV_RW(file, 1);

#else

    pResult = Mix_LoadWAV(filepath);

#endif

    return pResult;
}

static Mix_Music* OGE_LoadMUS(const char* filepath)
{
    Mix_Music* pResult = NULL;

#ifdef __OGE_WITH_SDL2__

    SDL_RWops *file = SDL_RWFromFile(filepath, "rb");
    if(file) pResult = Mix_LoadMUS_RW(file);

#else

    pResult = Mix_LoadMUS(filepath);

#endif

    return pResult;
}
*/

static CogeSound* SoundChannels[_OGE_MAX_SOUND_CHANNEL_ + 1];

void WhenMusicFinished()
{
	return;
}

void WhenSoundFinished(int iChannel)
{
	SoundChannels[iChannel] = NULL;
}

int GetFreeChannel()
{
    for(int i=0; i<=_OGE_MAX_SOUND_CHANNEL_; i++)
    {
        if (SoundChannels[i] == NULL) return i;
    }

    return -1;
}

/*------------------ CogeAudio ------------------*/

CogeAudio::CogeAudio():
m_pCurrentMusic(NULL)
{
    m_iState = -1;

    //m_bAudioIsOK = false;
    m_bAudioIsOpen = false;

    m_iAudioRate = _OGE_SOUND_DEFAULT_FREQUENCY_;
    m_iAudioFormat = AUDIO_S16;
    m_iAudioChannels = 2;
    m_iAudioBuffers = 4096;

    m_iSoundVolume = 0;
    m_iMusicVolume = 0;

    m_bAudioIsOK = SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0;

    for(int i=0; i<=_OGE_MAX_SOUND_CHANNEL_; i++) SoundChannels[i] = NULL;

}

CogeAudio::~CogeAudio()
{
    Finalize();

    if(m_bAudioIsOK)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        m_bAudioIsOK = false;
    }
}

int CogeAudio::Initialize(int iAudioRate, int iAudioChannels, int iAudioFormat, int iAudioBuffers)
{
    Finalize();
    if(!m_bAudioIsOK) return -1;

    if (Mix_OpenAudio(iAudioRate, iAudioFormat, iAudioChannels, iAudioBuffers) >= 0)
    {
		Mix_QuerySpec(&m_iAudioRate, &m_iAudioFormat, &m_iAudioChannels);
		m_iAudioBuffers = iAudioBuffers;
		m_bAudioIsOpen = true;
		m_iState = 0;

		//Mix_AllocateChannels(MIX_CHANNELS);
		Mix_AllocateChannels(_OGE_MAX_SOUND_CHANNEL_);

		SetSoundVolume(MIX_MAX_VOLUME/2);
		SetMusicVolume(MIX_MAX_VOLUME/2);

		Mix_ChannelFinished(WhenSoundFinished);

		return 0;
	}
	else
	{
		OGE_Log("Couldn't open audio: %s\n", SDL_GetError());
		return -1;
	}


}

void CogeAudio::Finalize()
{
    StopAllSound();
    StopAllMusic();

    m_iState = -1;

    DelAllSound();
    DelAllMusic();

    if(m_bAudioIsOpen)
    {
        Mix_CloseAudio();
        m_bAudioIsOpen = false;
    }
}

bool CogeAudio::IsAudioOpen()
{
    return m_bAudioIsOpen;
}

bool CogeAudio::IsAudioOK()
{
    return m_bAudioIsOK;
}

int CogeAudio::GetState()
{
    return m_iState;
}

void CogeAudio::StopAllSound()
{
    if(!m_bAudioIsOpen) return;
    Mix_HaltChannel(-1);
}

void CogeAudio::StopAllMusic()
{
    if(!m_bAudioIsOpen) return;
    Mix_HaltMusic();
}

void CogeAudio::PauseAllSound()
{
    Mix_Pause(-1);
}
void CogeAudio::PauseAllMusic()
{
    if(Mix_PlayingMusic()) Mix_PauseMusic();
}

void CogeAudio::ResumeAllSound()
{
    Mix_Resume(-1);
}
void CogeAudio::ResumeAllMusic()
{
    if(Mix_PausedMusic()) Mix_ResumeMusic();
}

int CogeAudio::PrepareMusic(const std::string& sMusicName)
{
    CogeMusic* pMusic = FindMusic(sMusicName);
    if(pMusic == NULL) return -1;
    if(m_pCurrentMusic == pMusic) return 0;
    Mix_HaltMusic();
    m_pCurrentMusic = pMusic;
    return 1;

}

int CogeAudio::PrepareMusic(CogeMusic* pTheMusic)
{
    if(pTheMusic == NULL) return -1;
    if(m_pCurrentMusic == pTheMusic) return 0;
    Mix_HaltMusic();
    m_pCurrentMusic = pTheMusic;
    return 1;
}
CogeMusic* CogeAudio::GetCurrentMusic()
{
    return m_pCurrentMusic;
}

int CogeAudio::PlayMusic(int iLoopTimes)
{
    if(!m_bAudioIsOpen) return -1;
    if(m_pCurrentMusic) return m_pCurrentMusic->Play(iLoopTimes);
    else return -1;
}

int CogeAudio::FadeInMusic(const std::string& sMusicName, int iLoopTimes, int iFadingTime)
{
    if(PrepareMusic(sMusicName) >= 0)
    {
        if(m_pCurrentMusic) return m_pCurrentMusic->FadeIn(iLoopTimes, iFadingTime);
    }
    return -1;
}
int CogeAudio::FadeOutMusic(int iFadingTime)
{
    if (Mix_FadingMusic() != MIX_FADING_OUT) return Mix_FadeOutMusic(iFadingTime);
    return -1;
}

void CogeAudio::SetSoundVolume(int iValue)
{
    if(!m_bAudioIsOpen) return;
    if(iValue != m_iSoundVolume)
    {
        Mix_Volume(-1, iValue);
        m_iSoundVolume = Mix_Volume(-1, -1);
    }
}

void CogeAudio::SetMusicVolume(int iValue)
{
    if(!m_bAudioIsOpen) return;
    if(iValue != m_iMusicVolume)
    {
        Mix_VolumeMusic(iValue);
        m_iMusicVolume = Mix_VolumeMusic(-1);
    }
}

int CogeAudio::GetSoundVolume()
{
    //m_iSoundVolume = Mix_Volume(-1, -1);
    return m_iSoundVolume;
}

int CogeAudio::GetMusicVolume()
{
    //m_iMusicVolume = Mix_VolumeMusic(-1);
    return m_iMusicVolume;
}


CogeMusic* CogeAudio::NewMusic(const std::string& sMusicName, const std::string& sFileName)
{
    if(!m_bAudioIsOpen) return NULL;

	ogeMusicMap::iterator it;

    it = m_MusicMap.find(sMusicName);
	if (it != m_MusicMap.end())
	{
        OGE_Log("The Music name '%s' is in use.\n", sMusicName.c_str());
        return NULL;
	}

	CogeMusic* pNewMusic = NULL;

	if (sFileName.length() > 0)
	{
	    pNewMusic = new CogeMusic(sMusicName, this);
	    //pNewMusic->m_pMusic = OGE_LoadMUS(sFileName.c_str());
	    pNewMusic->m_pMusic = Mix_LoadMUS(sFileName.c_str());
		if(pNewMusic->m_pMusic == NULL)
        {
            delete pNewMusic;
            pNewMusic = NULL;
            OGE_Log("Unable to load music file(%s): %s\n", sFileName.c_str(), Mix_GetError());
            return NULL;
        }

	}
	else return NULL;

    if(pNewMusic)
    {
        pNewMusic->m_iMusicType = Mix_GetMusicType(pNewMusic->m_pMusic);
        pNewMusic->m_iState = 0;
        m_MusicMap.insert(ogeMusicMap::value_type(sMusicName, pNewMusic));

    }

	return pNewMusic;

}

CogeMusic* CogeAudio::NewMusicFromBuffer(const std::string& sMusicName, char* pBuffer, int iBufferSize)
{
    if(!m_bAudioIsOpen) return NULL;

	ogeMusicMap::iterator it;

    it = m_MusicMap.find(sMusicName);
	if (it != m_MusicMap.end())
	{
        OGE_Log("The Music name '%s' is in use.\n", sMusicName.c_str());
        return NULL;
	}

	CogeMusic* pNewMusic = NULL;

	if (pBuffer && iBufferSize > 0)
	{
	    pNewMusic = new CogeMusic(sMusicName, this);
	    pNewMusic->m_pMusic = Mix_LoadMUS_RW(SDL_RWFromMem((void*)pBuffer, iBufferSize));
		if(pNewMusic->m_pMusic == NULL)
        {
            delete pNewMusic;
            pNewMusic = NULL;
            OGE_Log("Unable to load music from buffer: %s\n", Mix_GetError());
            return NULL;
        }

	}
	else return NULL;

	if(pNewMusic)
    {
        pNewMusic->m_iMusicType = Mix_GetMusicType(pNewMusic->m_pMusic);
        pNewMusic->m_iState = 0;
        m_MusicMap.insert(ogeMusicMap::value_type(sMusicName, pNewMusic));

    }

	return pNewMusic;
}

CogeMusic* CogeAudio::FindMusic(const std::string& sMusicName)
{
    ogeMusicMap::iterator it;

    it = m_MusicMap.find(sMusicName);
	if (it != m_MusicMap.end()) return it->second;
	else return NULL;

}

CogeMusic* CogeAudio::GetMusic(const std::string& sMusicName, const std::string& sFileName)
{
    CogeMusic* pMatchedMusic = FindMusic(sMusicName);
    if (!pMatchedMusic) pMatchedMusic = NewMusic(sMusicName, sFileName);
    //if (pMatchedMusic) pMatchedMusic->Hire();
    return pMatchedMusic;
}

CogeMusic* CogeAudio::GetMusicFromBuffer(const std::string& sMusicName, char* pBuffer, int iBufferSize)
{
    CogeMusic* pMatchedMusic = FindMusic(sMusicName);
    if (!pMatchedMusic) pMatchedMusic = NewMusicFromBuffer(sMusicName, pBuffer, iBufferSize);
    //if (pMatchedMusic) pMatchedMusic->Hire();
    return pMatchedMusic;
}

int CogeAudio::DelMusic(const std::string& sMusicName)
{
    CogeMusic* pMatchedMusic = FindMusic(sMusicName);
	if (pMatchedMusic)
	{
	    pMatchedMusic->Stop();
	    //pMatchedMusic->Fire();
	    //if (pMatchedMusic->m_iTotalUsers > 0) return 0;
		m_MusicMap.erase(sMusicName);
		delete pMatchedMusic;
		return 1;
	}
	return -1;
}

void CogeAudio::DelAllMusic()
{
    StopAllMusic();
    m_pCurrentMusic = NULL;
    ogeMusicMap::iterator it;
	CogeMusic* pMatchedMusic = NULL;

	it = m_MusicMap.begin();

	while (it != m_MusicMap.end())
	{
		pMatchedMusic = it->second;
		m_MusicMap.erase(it);
		delete pMatchedMusic;
		it = m_MusicMap.begin();
	}

}

CogeSound * CogeAudio::NewSound(const std::string& sSoundName, const std::string& sFileName)
{
    if(!m_bAudioIsOpen) return NULL;

	ogeSoundMap::iterator it;

    it = m_SoundMap.find(sSoundName);
	if (it != m_SoundMap.end())
	{
        OGE_Log("The Sound name '%s' is in use.\n", sSoundName.c_str());
        return NULL;
	}

	CogeSound* pNewSound = NULL;

	if (sFileName.length() > 0)
	{
	    pNewSound = new CogeSound(sSoundName, this);
	    //pNewSound->m_pSound = OGE_LoadWAV(sFileName.c_str());
	    pNewSound->m_pSound = Mix_LoadWAV(sFileName.c_str());
		if(pNewSound->m_pSound == NULL)
        {
            delete pNewSound;
            pNewSound = NULL;
            OGE_Log("Unable to load sound file: %s\n", Mix_GetError());
            return NULL;
        }

	}
	else return NULL;

    if(pNewSound)
    {
        pNewSound->m_iState = 0;
        m_SoundMap.insert(ogeSoundMap::value_type(sSoundName, pNewSound));
    }

	return pNewSound;

}

CogeSound* CogeAudio::NewSoundFromBuffer(const std::string& sSoundName, char* pBuffer, int iBufferSize)
{
    if(!m_bAudioIsOpen) return NULL;

	ogeSoundMap::iterator it;

    it = m_SoundMap.find(sSoundName);
	if (it != m_SoundMap.end())
	{
        OGE_Log("The Sound name '%s' is in use.\n", sSoundName.c_str());
        return NULL;
	}

	CogeSound* pNewSound = NULL;

	if (pBuffer && iBufferSize > 0)
	{
	    pNewSound = new CogeSound(sSoundName, this);
	    pNewSound->m_pSound = Mix_LoadWAV_RW(SDL_RWFromMem((void*)pBuffer, iBufferSize), 0);
		if(pNewSound->m_pSound == NULL)
        {
            delete pNewSound;
            pNewSound = NULL;
            OGE_Log("Unable to load sound from buffer: %s\n", Mix_GetError());
            return NULL;
        }

	}
	else return NULL;

	if(pNewSound)
    {
        pNewSound->m_iState = 0;
        m_SoundMap.insert(ogeSoundMap::value_type(sSoundName, pNewSound));
    }

	return pNewSound;

}

CogeSound* CogeAudio::FindSound(const std::string& sSoundName)
{
    ogeSoundMap::iterator it;

    it = m_SoundMap.find(sSoundName);
	if (it != m_SoundMap.end()) return it->second;
	else return NULL;
}

CogeSound* CogeAudio::GetSound(const std::string& sSoundName, const std::string& sFileName)
{
    CogeSound* pMatchedSound = FindSound(sSoundName);
    if (!pMatchedSound) pMatchedSound = NewSound(sSoundName, sFileName);
    //if (pMatchedSound) pMatchedSound->Hire();
    return pMatchedSound;
}

CogeSound* CogeAudio::GetSoundFromBuffer(const std::string& sSoundName, char* pBuffer, int iBufferSize)
{
    CogeSound* pMatchedSound = FindSound(sSoundName);
    if (!pMatchedSound) pMatchedSound = NewSoundFromBuffer(sSoundName, pBuffer, iBufferSize);
    //if (pMatchedSound) pMatchedSound->Hire();
    return pMatchedSound;
}

int CogeAudio::DelSound(const std::string& sSoundName)
{
    CogeSound* pMatchedSound = FindSound(sSoundName);
	if (pMatchedSound)
	{
	    pMatchedSound->Stop();
	    //pMatchedSound->Fire();
	    //if (pMatchedSound->m_iTotalUsers > 0) return 0;
		m_SoundMap.erase(sSoundName);
		delete pMatchedSound;
		return 1;
	}
	return -1;
}

void CogeAudio::DelAllSound()
{
    StopAllSound();
    ogeSoundMap::iterator it;
	CogeSound* pMatchedSound = NULL;

	it = m_SoundMap.begin();

	while (it != m_SoundMap.end())
	{
		pMatchedSound = it->second;
		m_SoundMap.erase(it);
		delete pMatchedSound;
		it = m_SoundMap.begin();
	}
}




/*------------------ CogeMusic ------------------*/

CogeMusic::CogeMusic(const std::string& sName, CogeAudio* pAudio):
m_pAudio(pAudio),
m_pMusic(NULL),
m_sName(sName)
{
    m_iState = -1;

    //m_iTotalUsers = 0;

    m_iPlayTimes = 0;

    m_iMusicType = MUS_NONE;
}

CogeMusic::~CogeMusic()
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(Mix_PlayingMusic() || Mix_PausedMusic()) Mix_HaltMusic();
            m_pAudio->m_pCurrentMusic = NULL;
        }
    }

    if(m_pMusic)
    {
        Mix_FreeMusic(m_pMusic);
        m_pMusic = NULL;
    }

}

/*
void CogeMusic::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}
void CogeMusic::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}
*/

const std::string& CogeMusic::GetName()
{
    return m_sName;
}

int CogeMusic::GetState()
{
    return m_iState;

}

void* CogeMusic::GetDataBuffer()
{
    return m_pDataBuf;
}
void CogeMusic::SetDataBuffer(void* pData)
{
    m_pDataBuf = pData;
}

int CogeMusic::Play(int iLoopTimes)
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(!Mix_PlayingMusic())
            {
                if(m_iPlayTimes == 0 && m_iMusicType == MUS_MP3) { Mix_PlayMusic(m_pMusic, 0); Mix_HaltMusic(); } // ... bug?

                Mix_PlayMusic(m_pMusic, iLoopTimes);

                m_iPlayTimes++;

                return 1;
            }
        }
    }

    return -1;

}

int CogeMusic::Pause()
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(Mix_PlayingMusic())
            {
                Mix_PauseMusic();
                return 1;
            }
        }
    }

    return -1;

}

int CogeMusic::Resume()
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(Mix_PausedMusic())
            {
                Mix_ResumeMusic();
                return 1;
            }
        }
    }

    return -1;

}

int CogeMusic::Stop()
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(Mix_PlayingMusic() || Mix_PausedMusic())
            {
                Mix_HaltMusic();
                return 1;
            }
        }
    }

    return -1;

}

int CogeMusic::Rewind()
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            Mix_RewindMusic();
            return 1;
        }
    }

    return -1;
}

int CogeMusic::FadeIn(int iLoopTimes, int iFadingTime)
{
    if(m_pAudio && m_pMusic)
    {
        if(m_pAudio->m_pCurrentMusic == this)
        {
            if(!Mix_PlayingMusic() && (Mix_FadingMusic() != MIX_FADING_IN))
            {
                if(m_iPlayTimes == 0 && m_iMusicType == MUS_MP3) { Mix_PlayMusic(m_pMusic, 0); Mix_HaltMusic(); } // ... bug?

                Mix_FadeInMusic(m_pMusic, iLoopTimes, iFadingTime);

                m_iPlayTimes++;

                return 1;
            }
        }
    }

    return -1;
}
int CogeMusic::FadeOut(int iFadingTime)
{
    if(m_pAudio) return m_pAudio->FadeOutMusic(iFadingTime);
    return -1;
}

/*------------------ CogeSound ------------------*/

CogeSound::CogeSound(const std::string& sName, CogeAudio* pAudio):
m_pAudio(pAudio),
m_pSound(NULL),
m_sName(sName)
{
    m_iState = -1;

    m_iChannel = -1;

    //m_iTotalUsers = 0;
}

CogeSound::~CogeSound()
{
    if(m_pAudio && m_pSound && m_iChannel>=0 && SoundChannels[m_iChannel] == this)
    {
        if(Mix_Playing(m_iChannel) || Mix_Paused(m_iChannel)) Mix_HaltChannel(m_iChannel);
    }

    if(m_pSound)
    {
        Mix_FreeChunk(m_pSound);
        m_pSound = NULL;
        m_iChannel = -1;
    }

}

/*
void CogeSound::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}
void CogeSound::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}
*/

const std::string& CogeSound::GetName()
{
    return m_sName;
}

int CogeSound::GetState()
{
    return m_iState;
}

int CogeSound::Stop()
{
    if(m_pAudio && m_pSound && m_iChannel>=0 && SoundChannels[m_iChannel] == this)
    {
        if(Mix_Playing(m_iChannel) || Mix_Paused(m_iChannel)) Mix_HaltChannel(m_iChannel);
        return m_iChannel;
    }
    else return -1;
}

int CogeSound::Resume()
{
    if(m_pAudio && m_pSound && m_iChannel>=0 && SoundChannels[m_iChannel] == this)
    {
        if(Mix_Paused(m_iChannel)) Mix_Resume(m_iChannel);
        return m_iChannel;
    }
    else return -1;
}

int CogeSound::Pause()
{
    if(m_pAudio && m_pSound && m_iChannel>=0 && SoundChannels[m_iChannel] == this)
    {
        if(Mix_Playing(m_iChannel)) Mix_Pause(m_iChannel);
        return m_iChannel;
    }
    else return -1;
}

int CogeSound::Play(int iLoopTimes)
{
    if(m_pAudio && m_pSound)
    {
        if( SoundChannels[m_iChannel] == this)
        {
            //Mix_RewindChannel(m_iChannel);

            if(Mix_Paused(m_iChannel))
            {
                Mix_Resume(m_iChannel);
                return m_iChannel;
            }

            if(Mix_Playing(m_iChannel)) Mix_HaltChannel(m_iChannel);
        }
        m_iChannel = Mix_PlayChannel(-1, m_pSound, iLoopTimes);
        if(m_iChannel >= 0) SoundChannels[m_iChannel] = this;
        return m_iChannel;
    }
    else return -1;

}















