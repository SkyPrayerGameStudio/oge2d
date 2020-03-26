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

#include "ogeEngine.h"

#include "ogeAppParam.h"
#include "ogeCommon.h"
#include "ogeIniFile.h"

#include "clay.h"
#include "clay_plugin.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>

#define _OGE_GLOBAL_APP_DATA_NAME_   "_OGE_GLOBAL_APP_DATA_"

#define _OGE_INVALID_POS_             -999999
#define _OGE_DEFAULT_CPS_             30

#if SDL_VERSION_ATLEAST(2,0,0)
#define OGE_DEFAULT_REPEAT_DELAY      500
#define OGE_DEFAULT_REPEAT_INTERVAL   30
#else
#define OGE_DEFAULT_REPEAT_DELAY      SDL_DEFAULT_REPEAT_DELAY
#define OGE_DEFAULT_REPEAT_INTERVAL   SDL_DEFAULT_REPEAT_INTERVAL
#endif

static CogeEngine* g_game = NULL;

//static int g_iEffectAmount = 0; // for test ...

static bool GetValidRect(int x, int y, SDL_Rect &rcSrc, SDL_Rect &rcDst)
{
    //SDL_Rect rc = {0};

    int w = rcSrc.w;
    int h = rcSrc.h;

    if(x > rcDst.w || y > rcDst.h) return false;

    if(x < 0) {rcSrc.x=rcSrc.x-x; w=w+x; x=0;}
	if(y < 0) {rcSrc.y=rcSrc.y-y; h=h+y; y=0;}

	//if(w < 0) w = 0;
	//if(h < 0) h = 0;

	if(w <= 0 || h <= 0) return false;

	//if(x > m_iWidth) x = m_iWidth;
	//if(y > m_iHeight) y = m_iHeight;

	if(x+w > rcDst.w) w = rcDst.w - x;
	if(y+h > rcDst.h) h = rcDst.h - y;

	rcSrc.w = w;
	rcSrc.h = h;

	rcDst.x = x;
	rcDst.y = y;
	rcDst.w = w;
	rcDst.h = h;

	return true;
}

static bool OverlapRect(const CogeRect& rc1, const CogeRect& rc2)
{
	return  (rc1.left   <  rc2.right  &&
             rc1.right  >  rc2.left   &&
             rc1.top    <  rc2.bottom &&
             rc1.bottom >  rc2.top);
}
/*
static bool OverlapRect(const CogeRect* rc1, const CogeRect* rc2)
{
	return  (rc1->left   <  rc2->right  &&
             rc1->right  >  rc2->left   &&
             rc1->top    <  rc2->bottom &&
             rc1->bottom >  rc2->top);
}

static bool OverlapRect(int l1, int t1, int w1, int h1,
                 int l2, int t2, int w2, int h2)
{
	return  (l1      <  l2+w2      &&
             l1+w1   >  l2         &&
             t1      <  t2+h2      &&
             t1+h1   >  t2);
}

static bool CoverRect(const CogeRect& rc1, const CogeRect& rc2)
{
	return  (rc1.left   <=  rc2.left    &&
             rc1.right  >=  rc2.right   &&
             rc1.top    <=  rc2.top     &&
             rc1.bottom >=  rc2.bottom);
}

static bool CoverRect(const CogeRect* rc1, const CogeRect* rc2)
{
	return  (rc1->left   <=  rc2->left    &&
             rc1->right  >=  rc2->right   &&
             rc1->top    <=  rc2->top     &&
             rc1->bottom >=  rc2->bottom);
}

static bool CoverRect(int l1, int t1, int w1, int h1,
               int l2, int t2, int w2, int h2)
{
	return  (l1      <=  l2        &&
             l1+w1   >=  l2+w2     &&
             t1      <=  t2        &&
             t1+h1   >=  t2+h2);
}
*/

static bool PointInRect(int x, int y, const CogeRect& rc)
{
	return (x >= rc.left  &&
		    x <  rc.right &&
			y >= rc.top   &&
			y < rc.bottom);
}

static int CallbackOnAcceptClient()
{
    if(g_game)
    {
        CogeScene* scene = g_game->GetActiveScene();
        if(scene)
        {
            int rsl = scene->CallEvent(Event_OnAcceptClient);
            scene->CallProxySprEvent(Event_OnCollide);
            return rsl;
        }
        else return g_game->CallEvent(Event_OnAcceptClient);
    }
    return -1;
}

static int CallbackOnReceiveFromRemoteClient()
{
    if(g_game)
    {
        CogeScene* scene = g_game->GetActiveScene();
        if(scene)
        {
            int rsl = scene->CallEvent(Event_OnReceiveFromRemoteClient);
            scene->CallProxySprEvent(Event_OnMouseIn);
            return rsl;
        }
        else return g_game->CallEvent(Event_OnReceiveFromRemoteClient);
    }
    return -1;
}

static int CallbackOnRemoteClientDisconnect()
{
    if(g_game)
    {
        CogeScene* scene = g_game->GetActiveScene();
        if(scene)
        {
            int rsl = scene->CallEvent(Event_OnRemoteClientDisconnect);
            scene->CallProxySprEvent(Event_OnMouseOut);
            return rsl;
        }
        else return g_game->CallEvent(Event_OnRemoteClientDisconnect);
    }
    return -1;
}

static int CallbackOnRemoteServerDisconnect()
{
    if(g_game)
    {
        CogeScene* scene = g_game->GetActiveScene();
        if(scene)
        {
            int rsl = scene->CallEvent(Event_OnRemoteServerDisconnect);
            scene->CallProxySprEvent(Event_OnLeave);
            return rsl;
        }
        else return g_game->CallEvent(Event_OnRemoteServerDisconnect);
    }
    return -1;
}

static int CallbackOnReceiveFromRemoteServer()
{
    if(g_game)
    {
        CogeScene* scene = g_game->GetActiveScene();
        if(scene)
        {
            int rsl = scene->CallEvent(Event_OnReceiveFromRemoteServer);
            scene->CallProxySprEvent(Event_OnEnter);
            return rsl;
        }
        else return g_game->CallEvent(Event_OnReceiveFromRemoteServer);
    }
    return -1;
}

static int CallbackCheckRemoteDebugRequest()
{
    if(g_game && g_game->IsRemoteDebugMode())
    {
        return g_game->ScanRemoteDebugRequest();
    }
    return -1;
}

static int CallbackRemoteDebugService()
{
    if(g_game && g_game->IsRemoteDebugMode())
    {
        CogeSocket* pDebugSocket = g_game->GetDebugSocket();
        if(pDebugSocket)
        {
            int iDataSize = pDebugSocket->GetRecvDataSize();
            char* pInBuf = pDebugSocket->GetInBuf();
            char* pOutBuf = pDebugSocket->GetOutBuf();
            if(iDataSize > 0)
            {
                if(g_game->GetScripter())
                {
                    iDataSize = g_game->GetScripter()->HandleDebugRequest(iDataSize, pInBuf, pOutBuf);
                    if(iDataSize > 0) pDebugSocket->SendData(iDataSize);
                }
            }

            return iDataSize;
        }
    }
    return -1;
}

CogeEngine::CogeEngine():
m_pBuffer(NULL),
m_pPacker(NULL),
m_pVideo(NULL),
m_pAudio(NULL),
m_pPlayer(NULL),
m_pScripter(NULL),
m_pScript(NULL),
m_pActiveScene(NULL),
m_pLastActiveScene(NULL),
m_pNextActiveScene(NULL),
m_pCurrentMask(NULL),
m_pAppGameData(NULL),
m_pCustomData(NULL),
m_pNetwork(NULL),
m_pIM(NULL),
m_pDatabase(NULL),
//m_pLoadingScene(NULL),
m_pCurrentGameScene(NULL),
m_pCurrentGameSprite(NULL),
m_iScriptState(-1),
m_iState(-1)
{
    m_pBuffer = new CogeBufferManager();
    m_pPacker = new CogePacker(m_pBuffer);

    srand(SDL_GetTicks());

    memset((void*)&m_Events[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);

    memset((void*)&m_Players[0],  0, sizeof(int) * _OGE_MAX_PLAYER_COUNT_ );

    memset((void*)&m_Npcs[0],     0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );

    memset((void*)&m_UserGroups[0],0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserPaths[0], 0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserImages[0],0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserData[0],  0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserObjects[0],0,sizeof(int) * _OGE_MAX_NPC_COUNT_ );

#ifdef __OGE_WITH_SDL2__
    memset((void*)&m_NKeyState[0], 0, sizeof(bool)* _OGE_MAX_N_KEY_NUMBER_ );
    memset((void*)&m_NKeyTime[0],  0, sizeof(int) * _OGE_MAX_N_KEY_NUMBER_ );
    memset((void*)&m_SKeyState[0], 0, sizeof(bool)* _OGE_MAX_S_KEY_NUMBER_ );
    memset((void*)&m_SKeyTime[0],  0, sizeof(int) * _OGE_MAX_S_KEY_NUMBER_ );
#else
    memset((void*)&m_KeyState[0], 0, sizeof(bool)* _OGE_MAX_KEY_NUMBER_ );
    memset((void*)&m_KeyTime[0],  0, sizeof(int) * _OGE_MAX_KEY_NUMBER_ );
#endif

    memset((void*)&m_KeyMap[0],   0, sizeof(int) * _OGE_MAX_SCANCODE_ );

    memset((void*)&m_Joysticks[0],0, sizeof(CogeJoystick) * _OGE_MAX_JOY_COUNT_ );

    memset((void*)&m_VMouseList[0],0, sizeof(CogeVMouse) * _OGE_MAX_VMOUSE_ );

    memset((void*)&m_Socks[0],    0, sizeof(int) * _OGE_MAX_SOCK_COUNT_ );

    m_sAppDefaultDbFile = "";

    m_sActiveSceneName = "";
    m_sNextSceneName = "";

    m_iLockedCPS       = 0;
	m_iCycleInterval   = 0;
	m_iCurrentInterval = 0;
	m_iLastCycleTime = 0;
	m_iCycleRate     = 0;
	m_iOldCycleRate  = 0;
    m_iCycleCount    = 0;
	m_iGlobalTime    = 0;

    m_bShowVideoMode = false;
    m_bShowFPS = false;
    m_bShowMousePos = false;

    m_bUseDirtyRect = false;

    m_bFreeze = false;

    m_bKeyEventHappened = false;
    m_bEnableInput = true;

    m_bPackedIndex = false;
    m_bPackedImage = false;
    //m_bPackedMask = false;
    m_bPackedMedia = false;
    m_bPackedFont = false;
    m_bPackedScript = false;
    m_bPackedSprite = false;
    m_bPackedScene = false;

    m_bEncryptedIndex = false;
    m_bEncryptedImage = false;
    m_bEncryptedMedia = false;
    m_bEncryptedFont = false;
    m_bEncryptedScript = false;
    m_bEncryptedSprite = false;
    m_bEncryptedScene = false;

    m_bBinaryScript = false;

    m_iKeyDelay = OGE_DEFAULT_REPEAT_DELAY;
    m_iKeyInterval = OGE_DEFAULT_REPEAT_INTERVAL;

    m_iJoystickCount = 0;

    //m_iThreadForInput = 0;

    //m_bAudioIsOK = false;
    //m_bAudioIsOpen = false;

}

CogeEngine::CogeEngine(CogeScripter* pScripter):
m_pBuffer(NULL),
m_pPacker(NULL),
m_pVideo(NULL),
m_pAudio(NULL),
m_pPlayer(NULL),
m_pScripter(pScripter),
m_pScript(NULL),
m_pActiveScene(NULL),
m_pLastActiveScene(NULL),
m_pNextActiveScene(NULL),
m_pCurrentMask(NULL),
m_pAppGameData(NULL),
m_pCustomData(NULL),
m_pNetwork(NULL),
m_pIM(NULL),
m_pDatabase(NULL),
//m_pLoadingScene(NULL),
m_pCurrentGameScene(NULL),
m_pCurrentGameSprite(NULL),
m_iScriptState(-1),
m_iState(-1)
{
    m_pBuffer = new CogeBufferManager();
    m_pPacker = new CogePacker(m_pBuffer);

    srand(SDL_GetTicks());

    memset((void*)&m_Events[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);

    memset((void*)&m_Players[0],  0, sizeof(int) * _OGE_MAX_PLAYER_COUNT_ );

    memset((void*)&m_Npcs[0],     0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );

    memset((void*)&m_UserGroups[0],0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserPaths[0], 0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserImages[0],0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserData[0],  0, sizeof(int) * _OGE_MAX_NPC_COUNT_ );
    memset((void*)&m_UserObjects[0],0,sizeof(int) * _OGE_MAX_NPC_COUNT_ );

#ifdef __OGE_WITH_SDL2__
    memset((void*)&m_NKeyState[0], 0, sizeof(bool)* _OGE_MAX_N_KEY_NUMBER_ );
    memset((void*)&m_NKeyTime[0],  0, sizeof(int) * _OGE_MAX_N_KEY_NUMBER_ );
    memset((void*)&m_SKeyState[0], 0, sizeof(bool)* _OGE_MAX_S_KEY_NUMBER_ );
    memset((void*)&m_SKeyTime[0],  0, sizeof(int) * _OGE_MAX_S_KEY_NUMBER_ );
#else
    memset((void*)&m_KeyState[0], 0, sizeof(bool)* _OGE_MAX_KEY_NUMBER_ );
    memset((void*)&m_KeyTime[0],  0, sizeof(int) * _OGE_MAX_KEY_NUMBER_ );
#endif

    memset((void*)&m_KeyMap[0],   0, sizeof(int) * _OGE_MAX_SCANCODE_ );

    memset((void*)&m_Joysticks[0],0, sizeof(CogeJoystick) * _OGE_MAX_JOY_COUNT_ );

    memset((void*)&m_VMouseList[0],0, sizeof(CogeVMouse) * _OGE_MAX_VMOUSE_ );

    memset((void*)&m_Socks[0],    0, sizeof(int) * _OGE_MAX_SOCK_COUNT_ );

    m_sAppDefaultDbFile = "";

    m_sActiveSceneName = "";
    m_sNextSceneName = "";

	m_iLockedCPS       = 0;
	m_iCycleInterval   = 0;
	m_iCurrentInterval = 0;
	m_iLastCycleTime = 0;
	m_iCycleRate     = 0;
	m_iOldCycleRate  = 0;
    m_iCycleCount    = 0;
	m_iGlobalTime    = 0;

    m_iVideoWidth = 0;
    m_iVideoHeight = 0;
    m_iVideoBPP = 0;

    m_iMouseX = 0;
    m_iMouseY = 0;
    m_iMouseLeft = 0;
    m_iMouseRight = 0;
    m_iMouseOldLeft = 0;
    m_iMouseOldRight = 0;

    m_iMouseState = 0;

    m_iMainVMouseId = -1;
    m_iActiveVMouseCount = 0;

    m_iJoystickCount = 0;

    m_iKeyDelay = OGE_DEFAULT_REPEAT_DELAY;
    m_iKeyInterval = OGE_DEFAULT_REPEAT_INTERVAL;

    //m_iThreadForInput = 0;

    //m_bAudioIsOK = false;
    //m_bAudioIsOpen = false;

    m_bShowVideoMode = false;
    m_bShowFPS = false;
    m_bShowMousePos = false;

    m_bUseDirtyRect = false;

    m_bFreeze = false;

    m_bKeyEventHappened = false;
    m_bEnableInput = true;

    m_bPackedIndex = false;
    m_bPackedImage = false;
    //m_bPackedMask = false;
    m_bPackedMedia = false;
    m_bPackedFont = false;
    m_bPackedScript = false;
    m_bPackedSprite = false;
    m_bPackedScene = false;

    m_bEncryptedIndex = false;
    m_bEncryptedImage = false;
    m_bEncryptedMedia = false;
    m_bEncryptedFont = false;
    m_bEncryptedScript = false;
    m_bEncryptedSprite = false;
    m_bEncryptedScene = false;

    m_bBinaryScript = false;

    //m_EventMap.insert(ogeEventIdMap::value_type(Event_OnInit, iOnInitId));

    RegisterEventNames();

}

CogeEngine::~CogeEngine()
{
    Finalize();

    m_EventIdMap.clear();

    if(m_pPacker) { delete m_pPacker; m_pPacker = NULL; }
    if(m_pBuffer) { delete m_pBuffer; m_pBuffer = NULL; }
}

void CogeEngine::RegisterEventNames()
{
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnInit",       Event_OnInit));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnFinalize",   Event_OnFinalize));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnUpdate",     Event_OnUpdate));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnCollide",    Event_OnCollide));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnDraw",       Event_OnDraw));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnPathStep",   Event_OnPathStep));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnPathFin",    Event_OnPathFin));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnAnimaFin",   Event_OnAnimaFin));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnGetFocus",   Event_OnGetFocus));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnLoseFocus",  Event_OnLoseFocus));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseOver",  Event_OnMouseOver));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseIn",    Event_OnMouseIn));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseOut",   Event_OnMouseOut));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseDown",  Event_OnMouseDown));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseUp",    Event_OnMouseUp));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseDrag",  Event_OnMouseDrag));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnMouseDrop",  Event_OnMouseDrop));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnKeyDown",    Event_OnKeyDown));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnKeyUp",      Event_OnKeyUp));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnLeave",      Event_OnLeave));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnEnter",      Event_OnEnter));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnActive",     Event_OnActive));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnDeactive",   Event_OnDeactive));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnOpen",       Event_OnOpen));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnClose",      Event_OnClose));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnDrawSprFin", Event_OnDrawSprFin));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnDrawWinFin", Event_OnDrawWinFin));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnSceneActive",Event_OnSceneActive));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnSceneDeactive",Event_OnSceneDeactive));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnTimerTime",  Event_OnTimerTime));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnCustomEnd",  Event_OnCustomEnd));

    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnAcceptClient",    Event_OnAcceptClient));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnReceiveFromRemoteClient", Event_OnReceiveFromRemoteClient));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnRemoteClientDisconnect",  Event_OnRemoteClientDisconnect));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnRemoteServerDisconnect",  Event_OnRemoteServerDisconnect));
    m_EventIdMap.insert(std::map<std::string, int>::value_type("OnReceiveFromRemoteServer", Event_OnReceiveFromRemoteServer));

}

CogeVideo* CogeEngine::GetVideo()
{
    return m_pVideo;
}

CogeImage* CogeEngine::GetScreen()
{
    if (!m_pVideo) return NULL;
    return m_pVideo->GetScreen();
}

CogeNetwork* CogeEngine::GetNetwork()
{
    return m_pNetwork;
}

CogeScripter* CogeEngine::GetScripter()
{
    return m_pScripter;
}

CogeIM* CogeEngine::GetIM()
{
    return m_pIM;
}

CogeDatabase* CogeEngine::GetDatabase()
{
    return m_pDatabase;
}

int CogeEngine::GetOS()
{

#ifdef __WINCE__
    return OS_WinCE;
#elif defined __ANDROID__
    return OS_Android;
#elif defined __IPHONE__
    return OS_iPhone;

#elif defined __WIN32__
    return OS_Win32;
#elif defined __LINUX__
    return OS_Linux;
#elif defined __MACOS__
    return OS_MacOS;

#else
    return OS_Unknown;
#endif

}

int CogeEngine::GetState()
{
	return m_iState;
}

const std::string& CogeEngine::GetName()
{
    return m_sName;
}
const std::string& CogeEngine::GetTitle()
{
    return m_sTitle;
}

int CogeEngine::GetLockedCPS()
{
    return m_iLockedCPS;
}
void CogeEngine::LockCPS(int iCPS)
{
    m_iCycleRateWanted = iCPS;
	m_iCycleInterval = 1000/iCPS;
	m_iLockedCPS = iCPS;
}
void CogeEngine::UnlockCPS()
{
    m_iLockedCPS = 0;
}
int CogeEngine::GetCPS()
{
    return m_iCycleRate;
}
int CogeEngine::GetGameUpdateInterval()
{
    return m_iCycleInterval;
}
int CogeEngine::GetCurrentInterval()
{
    return m_iCurrentInterval;
}

void CogeEngine::DrawFPS(int x, int y)
{
    if(m_pVideo) m_pVideo->DrawFPS(x, y);
}

void CogeEngine::DrawMousePos(int x, int y)
{
    if(m_pVideo)
    {

        char sMousePos[25];
        //if(m_pActiveScene) sprintf(sMousePos,"X=%d,Y=%d L=%d,R=%d",
        //                           m_pActiveScene->m_SceneViewRect.left, m_pActiveScene->m_SceneViewRect.top,
        //                           m_iMouseLeft, m_iMouseRight);
        //else
        sprintf(sMousePos,"X=%d,Y=%d L=%d,R=%d", m_iMouseX, m_iMouseY, m_iMouseLeft, m_iMouseRight);

        /*
        m_pTextArea->FillRect(0xff00ff);
        m_pTextArea->DotTextOut(sFPS, x, y);
        m_pMainScreen->Draw(m_pTextArea, x, y);
        */

        m_pVideo->DotTextOut(sMousePos, x, y);

    }
}

void CogeEngine::DrawVideoMode(int x, int y)
{
    if(m_pVideo)
    {

        char sVideoMode[16];

        sprintf(sVideoMode,"%dx%dx%d", m_iVideoWidth, m_iVideoHeight, m_iVideoBPP);

        /*
        m_pTextArea->FillRect(0xff00ff);
        m_pTextArea->DotTextOut(sFPS, x, y);
        m_pMainScreen->Draw(m_pTextArea, x, y);
        */

        m_pVideo->DotTextOut(sVideoMode, x, y);

    }
}

/*
void CogeEngine::SetEvent_OnInit(void* pEvent)
{
    if (pEvent) m_OnInit = (PogeEvent)pEvent;
}

void CogeEngine::SetEvent_OnUpdateScreen(void* pEvent)
{
    if (pEvent) m_OnUpdateScreen = (PogeEvent)pEvent;
}

void CogeEngine::DoInit()
{
    //if (m_OnInit) m_OnInit(Sender);
    if (m_iState < 0) return;
    if (m_iScriptState < 0) return;

    int id = -1;

    ogeEventIdMap::iterator it;

    it = m_EventMap.find(Event_OnInit);
	if (it != m_EventMap.end()) id = it->second;

	if(id >= 0) m_Script->CallFunction(id);

}
void CogeEngine::DoDraw()
{
    //if (m_OnUpdateScreen) m_OnUpdateScreen(Sender);

    if (m_iState < 0) return;
    if (m_iScriptState < 0) return;

    int id = -1;

    ogeEventIdMap::iterator it;

    it = m_EventMap.find(Event_OnDraw);
	if (it != m_EventMap.end()) id = it->second;

	if(id >= 0) m_Script->CallFunction(id);
}
*/


int CogeEngine::CallEvent(int iEventCode)
{
    if (m_iScriptState < 0) return -1;

    int id = m_Events[iEventCode];

    int rsl = -1;

	if(id >= 0)
    {
        rsl = m_pScript->CallFunction(id);
        //m_pScript->CallGC();
    }

	return rsl;
}

void CogeEngine::HandleAppEvents()
{
    m_bKeyEventHappened = false;

    Uint32 iKeyCode = 0;

    //SDL_Log("Start to SDL_PollEvent() ... \n");

#ifdef __OGE_WITH_SDL2__
    for(int i=0; i<_OGE_MAX_VMOUSE_; i++)
    {
        if(m_VMouseList[i].pressed == false) m_VMouseList[i].active = false;
        m_VMouseList[i].done = false;
    }
#endif


    while (m_iState > 0 && SDL_PollEvent(&m_GlobalEvent))
    {
        //m_bKeyEventHappened = true;

        //SDL_Log("Type of SDL_PollEvent(): %d \n", m_GlobalEvent.type);

        // check for messages
        switch (m_GlobalEvent.type)
        {
            // exit if the window is closed
        case SDL_QUIT:
            Terminate();
            break;

#ifdef __OGE_WITH_SDL2__

        case SDL_FINGERMOTION:
        {
            //OGE_Log(" SDL_FINGERMOTION \n");

            iKeyCode = ((Uint64)m_GlobalEvent.tfinger.fingerId) % _OGE_MAX_VMOUSE_;

            if(iKeyCode >= _OGE_MAX_VMOUSE_) break;

            SDL_Touch* inTouch = SDL_GetTouch(m_GlobalEvent.tfinger.touchId);
            if(inTouch == NULL) break; //The touch has been removed

            float x = ((float)m_GlobalEvent.tfinger.x)/inTouch->xres;
            float y = ((float)m_GlobalEvent.tfinger.y)/inTouch->yres;

            m_VMouseList[iKeyCode].x = lround(m_pVideo->GetWidth() * x);
            m_VMouseList[iKeyCode].y = lround(m_pVideo->GetHeight()* y);
            m_VMouseList[iKeyCode].pressed = true;
            m_VMouseList[iKeyCode].moving = true;
            m_VMouseList[iKeyCode].active = true;

            if(m_pActiveScene)
            {
                m_VMouseList[iKeyCode].x += m_pActiveScene->m_SceneViewRect.left;
                m_VMouseList[iKeyCode].y += m_pActiveScene->m_SceneViewRect.top;
            }

            break;
        }
        case SDL_FINGERDOWN:
        {
            //OGE_Log(" SDL_FINGERDOWN \n");

            iKeyCode = ((Uint64)m_GlobalEvent.tfinger.fingerId) % _OGE_MAX_VMOUSE_;

            if(iKeyCode >= _OGE_MAX_VMOUSE_) break;

            SDL_Touch* inTouch = SDL_GetTouch(m_GlobalEvent.tfinger.touchId);
            if(inTouch == NULL) break; //The touch has been removed

            float x = ((float)m_GlobalEvent.tfinger.x)/inTouch->xres;
            float y = ((float)m_GlobalEvent.tfinger.y)/inTouch->yres;

            m_VMouseList[iKeyCode].x = lround(m_pVideo->GetWidth() * x);
            m_VMouseList[iKeyCode].y = lround(m_pVideo->GetHeight()* y);
            m_VMouseList[iKeyCode].pressed = true;
            m_VMouseList[iKeyCode].moving = false;
            m_VMouseList[iKeyCode].active = true;

            if(m_pActiveScene)
            {
                m_VMouseList[iKeyCode].x += m_pActiveScene->m_SceneViewRect.left;
                m_VMouseList[iKeyCode].y += m_pActiveScene->m_SceneViewRect.top;
            }

            break;
        }
        case SDL_FINGERUP:
        {
            //OGE_Log(" SDL_FINGERUP \n");

            iKeyCode = ((Uint64)m_GlobalEvent.tfinger.fingerId) % _OGE_MAX_VMOUSE_;

            if(iKeyCode >= _OGE_MAX_VMOUSE_) break;

            SDL_Touch* inTouch = SDL_GetTouch(m_GlobalEvent.tfinger.touchId);
            if(inTouch == NULL) break; //The touch has been removed

            float x = ((float)m_GlobalEvent.tfinger.x)/inTouch->xres;
            float y = ((float)m_GlobalEvent.tfinger.y)/inTouch->yres;

            m_VMouseList[iKeyCode].x = lround(m_pVideo->GetWidth() * x);
            m_VMouseList[iKeyCode].y = lround(m_pVideo->GetHeight()* y);
            m_VMouseList[iKeyCode].pressed = false;
            m_VMouseList[iKeyCode].moving = false;
            m_VMouseList[iKeyCode].active = true;

            if(m_pActiveScene)
            {
                m_VMouseList[iKeyCode].x += m_pActiveScene->m_SceneViewRect.left;
                m_VMouseList[iKeyCode].y += m_pActiveScene->m_SceneViewRect.top;
            }

            break;
        }

#endif


        case SDL_KEYUP:

            iKeyCode = m_GlobalEvent.key.keysym.sym;

            if(m_KeyMap[m_GlobalEvent.key.keysym.scancode] > 0)
                iKeyCode = m_KeyMap[m_GlobalEvent.key.keysym.scancode];

#ifdef __OGE_WITH_SDL2__
            if(iKeyCode > 0x1000)
            {
                iKeyCode = iKeyCode & 0x00000FFF;
                m_SKeyState[iKeyCode] = false;
            }
            else
            {
                m_NKeyState[iKeyCode] = false;
            }
#else
            m_KeyState[iKeyCode] = false;
#endif
            if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
            break;

            // check for keypresses
        case SDL_KEYDOWN:

            // exit if ESCAPE is pressed
            //if (m_GlobalEvent.key.keysym.sym == SDLK_ESCAPE) m_iState = 0;

            //OGE_Log("sym: %d, scancode: %d\n", m_GlobalEvent.key.keysym.sym, m_GlobalEvent.key.keysym.scancode);

            if (!HandleIMEvents())
            {
                iKeyCode = m_GlobalEvent.key.keysym.sym;

                if(m_KeyMap[m_GlobalEvent.key.keysym.scancode] > 0)
                    iKeyCode = m_KeyMap[m_GlobalEvent.key.keysym.scancode];

#ifdef __OGE_WITH_SDL2__

                if(iKeyCode > 0x1000)
                {
                    iKeyCode = iKeyCode & 0x00000FFF;
                    m_SKeyState[iKeyCode] = true;
                }
                else
                {
                    m_NKeyState[iKeyCode] = true;
                }
#else
                m_KeyState[iKeyCode] = true;
#endif
                if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
            }

            break;

        case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */

            //printf("joystick - type: %d, which: %d, axis: %d, value: %d\n",
            //               m_GlobalEvent.jaxis.type, m_GlobalEvent.jaxis.which, m_GlobalEvent.jaxis.axis, m_GlobalEvent.jaxis.value);


            if ( (m_GlobalEvent.jaxis.which < m_iJoystickCount)
                && ( m_GlobalEvent.jaxis.value < -3200 || m_GlobalEvent.jaxis.value > 3200 ) )
            {
                if( m_GlobalEvent.jaxis.axis == 0)
                {
                    // Left-right movement code goes here
                    if(m_GlobalEvent.jaxis.value < 0)
                    {
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Left] = true;
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Right] = false;
                        if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                    }
                    else
                    {
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Left] = false;
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Right] = true;
                        if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                    }

                }

                if( m_GlobalEvent.jaxis.axis == 1)
                {
                    // Up-Down movement code goes here

                    if(m_GlobalEvent.jaxis.value < 0)
                    {
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Up] = true;
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Down] = false;
                        if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                    }
                    else
                    {
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Up] = false;
                        m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Down] = true;
                        if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                    }
                }

                //printf("joystick - type: %d, which: %d, axis: %d, value: %d\n",
                //           m_GlobalEvent.jaxis.type, m_GlobalEvent.jaxis.which, m_GlobalEvent.jaxis.axis, m_GlobalEvent.jaxis.value);

            }
            else if(m_GlobalEvent.jaxis.which < m_iJoystickCount)
            {
                if( m_GlobalEvent.jaxis.axis == 0)
                {
                    // Left-right movement code goes here
                    m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Left] = false;
                    m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Right] = false;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;

                }

                if( m_GlobalEvent.jaxis.axis == 1)
                {
                    // Up-Down movement code goes here

                    m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Up] = false;
                    m_Joysticks[m_GlobalEvent.jaxis.which].m_KeyState[Dir_Down] = false;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                }
            }


            break;

            case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */

            //printf("joystick - type: %d, which: %d, button: %d, state: %d\n",
            //               m_GlobalEvent.jbutton.type, m_GlobalEvent.jbutton.which, m_GlobalEvent.jbutton.button, m_GlobalEvent.jbutton.state);


            if ( m_GlobalEvent.jbutton.which < m_iJoystickCount )
            {
                if(m_GlobalEvent.jbutton.state == SDL_PRESSED)
                {
                    m_Joysticks[m_GlobalEvent.jbutton.which].m_KeyState[m_GlobalEvent.jbutton.button + 10] = true;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                }
                else if(m_GlobalEvent.jbutton.state == SDL_RELEASED)
                {
                    m_Joysticks[m_GlobalEvent.jbutton.which].m_KeyState[m_GlobalEvent.jbutton.button + 10] = false;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                }
            }

            break;

            case SDL_JOYBUTTONUP:  /* Handle Joystick Button Presses */

            //printf("joystick - type: %d, which: %d, button: %d, state: %d\n",
            //               m_GlobalEvent.jbutton.type, m_GlobalEvent.jbutton.which, m_GlobalEvent.jbutton.button, m_GlobalEvent.jbutton.state);


            if ( m_GlobalEvent.jbutton.which < m_iJoystickCount )
            {
                if(m_GlobalEvent.jbutton.state == SDL_PRESSED)
                {
                    m_Joysticks[m_GlobalEvent.jbutton.which].m_KeyState[m_GlobalEvent.jbutton.button + 10] = true;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                }
                else if(m_GlobalEvent.jbutton.state == SDL_RELEASED)
                {
                    m_Joysticks[m_GlobalEvent.jbutton.which].m_KeyState[m_GlobalEvent.jbutton.button + 10] = false;
                    if(m_iState > 0 && m_bEnableInput) m_bKeyEventHappened = true;
                }
            }

            break;

        } // end switch
    }

    //SDL_Log("End of SDL_PollEvent() ... \n");
}

void CogeEngine::UpdateMouseInput()
{
    if(m_iState <= 0) return;

#ifdef __OGE_WITH_MOUSE__

    m_iMouseOldLeft = m_iMouseLeft;
    m_iMouseOldRight = m_iMouseRight;
    m_iMouseState = SDL_GetMouseState(&m_iMouseX, &m_iMouseY);

#if defined(__ANDROID__) || defined(__WINCE__) || defined(__IPHONE__)
    m_pVideo->GetRealPos(m_iMouseX, m_iMouseY, &m_iMouseX, &m_iMouseY);
#endif

    if(m_pActiveScene)
    {
        m_iMouseX = m_iMouseX + m_pActiveScene->m_SceneViewRect.left;
        m_iMouseY = m_iMouseY + m_pActiveScene->m_SceneViewRect.top;
    }

    if(m_bEnableInput && (m_iMouseState & SDL_BUTTON(SDL_BUTTON_LEFT))) m_iMouseLeft = 1;
    else m_iMouseLeft = 0;
    if(m_bEnableInput && (m_iMouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))) m_iMouseRight = 1;
    else m_iMouseRight = 0;

#else

/*
    m_iMouseState = SDL_GetMouseState(&m_iMouseX, &m_iMouseY);

    if((m_iMouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0)
    {
        m_VMouseList[0].x = m_iMouseX;
        m_VMouseList[0].y = m_iMouseY;
        m_VMouseList[0].pressed = true;
        m_VMouseList[0].active = true;
    }
    else
    {
        m_VMouseList[0].x = m_iMouseX;
        m_VMouseList[0].y = m_iMouseY;
        m_VMouseList[0].pressed = false;
        m_VMouseList[0].active = true;
    }
*/

    m_iMainVMouseId = -1;
    m_iActiveVMouseCount = 0;
    int iReleaseId = -1;

    for(int i=_OGE_MAX_VMOUSE_-1; i>=0; i--)
    {
        if(m_VMouseList[i].pressed || m_VMouseList[i].active)
        {
            m_iActiveVMouseCount++;

            if(m_VMouseList[i].pressed)
            {
                if(m_iMainVMouseId < 0)
                {
                    m_iMainVMouseId = i;
                }
            }
            else
            {
                iReleaseId = i;
            }
        }
    }

    if(m_iMainVMouseId < 0 && iReleaseId >= 0)
    {
        m_iMainVMouseId = iReleaseId;
    }

    if(m_iMainVMouseId >= 0)
    {
        m_iMouseOldLeft = m_iMouseLeft;
        m_iMouseOldRight = m_iMouseRight;

        m_iMouseRight = 0;
        m_iMouseLeft = (m_bEnableInput && m_VMouseList[m_iMainVMouseId].pressed) ? 1 : 0;

        m_iMouseX = m_VMouseList[m_iMainVMouseId].x;
        m_iMouseY = m_VMouseList[m_iMainVMouseId].y;

        //if(m_VMouseList[m_iMainVMouseId].pressed) OGE_Log("Press: %d, x = %d, y = %d \n", 1, m_iMouseX, m_iMouseY);
        //else OGE_Log("Press: %d, x = %d, y = %d \n", 0, m_iMouseX, m_iMouseY);

    }

    //OGE_Log("m_iMainVMouseId: %d \n", m_iMainVMouseId);

#endif


}

bool CogeEngine::HandleIMEvents()
{
    if(m_pIM && m_pIM->GetActiveInputter())
    {
        int iKeyStateFlag = 0;

#ifdef __OGE_WITH_IME__

        iKeyStateFlag = 0;

#else

#ifdef __OGE_WITH_SDL2__
        if(IsKeyDown(0x10E1)) iKeyStateFlag = iKeyStateFlag | 1;
        if(IsKeyDown(0x10E5)) iKeyStateFlag = iKeyStateFlag | 64;
#else
        if(IsKeyDown(304)) iKeyStateFlag = iKeyStateFlag | 1;
        if(IsKeyDown(303)) iKeyStateFlag = iKeyStateFlag | 64;
#endif

#endif
        return m_pIM->HandleInput((void*)(&m_GlobalEvent),
                                  m_KeyMap[m_GlobalEvent.key.keysym.scancode], iKeyStateFlag);
    }
    else return false;
}

void CogeEngine::HandleEngineEvents()
{
    if(m_InitScenes.size() > 0)
    {
        for(ogeSceneMap::iterator it = m_InitScenes.begin(); it != m_InitScenes.end(); it++)
        {
            m_pCurrentGameScene = it->second;
            it->second->CallEvent(Event_OnInit);
        }
        m_InitScenes.clear();
    }

    m_pCurrentGameScene = NULL;

    if(m_InitSprites.size() > 0)
    {
        for(ogeSpriteMap::iterator it = m_InitSprites.begin(); it != m_InitSprites.end(); it++)
        {
            m_pCurrentGameSprite = it->second;
            it->second->CallEvent(Event_OnInit);
        }
        m_InitSprites.clear();
    }

    m_pCurrentGameSprite = NULL;
}

void CogeEngine::HandleNetworkEvents()
{
    if(m_iState > 0 && m_pNetwork) m_pNetwork->Update();
}

void CogeEngine::UpdateActiveScene()
{
    if(m_iState > 0 && m_pActiveScene)
    {
        m_pCurrentGameScene = m_pActiveScene;
        m_pActiveScene->Update();
    }
}


void CogeEngine::StopBroadcastKeyEvent()
{
    m_bKeyEventHappened = false;
}

void CogeEngine::SkipCurrentMouseEvent()
{
    if(m_pActiveScene) m_pActiveScene->SkipCurrentMouseEvent();
}

void CogeEngine::EnableInput()
{
    m_bEnableInput = true;
}
void CogeEngine::DisableInput()
{
    m_bEnableInput = false;
}

void CogeEngine::ResetVMouseState()
{
    m_iMainVMouseId = -1;
    m_iActiveVMouseCount = 0;
    memset((void*)&m_VMouseList[0],0, sizeof(CogeVMouse) * _OGE_MAX_VMOUSE_ );
}

void CogeEngine::ShowInfo(int iInfoType)
{
    switch (iInfoType)
    {
    case Info_FPS:
    m_bShowFPS = true;
    break;
    case Info_VideoMode:
    m_bShowVideoMode = true;
    break;
    case Info_MousePos:
    m_bShowMousePos = true;
    break;
    }
}
void CogeEngine::HideInfo(int iInfoType)
{
    switch (iInfoType)
    {
    case Info_FPS:
    m_bShowFPS = false;
    break;
    case Info_VideoMode:
    m_bShowVideoMode = false;
    break;
    case Info_MousePos:
    m_bShowMousePos = false;
    break;
    }
}

bool CogeEngine::GetFreeze()
{
    return m_bFreeze;
}
void CogeEngine::SetFreeze(bool bValue)
{
    m_bFreeze = bValue;
}

void CogeEngine::EnableKeyRepeat()
{
#if SDL_VERSION_ATLEAST(2,0,0)
    return;
#else
    SDL_EnableKeyRepeat(m_iKeyDelay, m_iKeyInterval);
#endif
}
void CogeEngine::DisableKeyRepeat()
{
#if SDL_VERSION_ATLEAST(2,0,0)
    return;
#else
    SDL_EnableKeyRepeat(0, 0);
#endif
}

bool CogeEngine::IsDirtyRectMode()
{
    return m_bUseDirtyRect;
}

bool CogeEngine::IsUnicodeIM()
{
    if (m_pIM) return m_pIM->GetUnicodeMode();
    else return false;
}

bool CogeEngine::IsInputMethodReady()
{
    if (m_pIM) return m_pIM->IsInputMethodReady();
    else return false;
}

void CogeEngine::SetDirtyRectMode(bool bValue)
{
    if(m_bUseDirtyRect != bValue) m_bUseDirtyRect = bValue;
    if(m_bUseDirtyRect && m_pActiveScene)
    {
        if(m_pActiveScene->m_pBackground == m_pVideo->GetDefaultBg())
        {
            m_pVideo->ClearDefaultBg(m_pActiveScene->m_iBackgroundColor);
        }
        m_pVideo->DrawImage(m_pActiveScene->m_pBackground, 0, 0);
    }
}

void CogeEngine::UpdateScreen(int x, int y)
{
    if (m_iState < 0) return;

    m_pVideo->Update(x, y);

    m_iFrameInterval = m_pVideo->GetScreenUpdateInterval();
}

void CogeEngine::ScrollScene(int iIncX, int iIncY, bool bImmediately)
{
    if (m_pActiveScene) m_pActiveScene->ScrollView(iIncX, iIncY, bImmediately);
}
void CogeEngine::SetSceneViewPos(int iLeft, int iTop, bool bImmediately)
{
    if (m_pActiveScene) m_pActiveScene->SetViewPos(iLeft, iTop, bImmediately);
}

int CogeEngine::GetSceneViewPosX()
{
    if (m_pActiveScene) return m_pActiveScene->m_SceneViewRect.left;
    else return 0;

}
int CogeEngine::GetSceneViewPosY()
{
    if (m_pActiveScene) return m_pActiveScene->m_SceneViewRect.top;
    else return 0;
}

int CogeEngine::GetSceneViewWidth()
{
    return m_iVideoWidth;
}
int CogeEngine::GetSceneViewHeight()
{
    return m_iVideoHeight;
}

int CogeEngine::GetCurrentKey()
{
    Uint32 iKeyCode = m_GlobalEvent.key.keysym.sym;

    if(m_KeyMap[m_GlobalEvent.key.keysym.scancode] > 0)
                iKeyCode = m_KeyMap[m_GlobalEvent.key.keysym.scancode];

#ifdef __OGE_WITH_SDL2__

    if(iKeyCode > 0x1000) iKeyCode = 0x1000 | (iKeyCode & 0x00000FFF);

#endif

    return iKeyCode;

}

int CogeEngine::GetCurrentScanCode()
{
    return m_GlobalEvent.key.keysym.scancode;
}

int CogeEngine::SetCurrentMask(const std::string& sMaskName)
{
    ogeImageMap::iterator it;
    CogeImage* pMatchedImage = NULL;

    it = m_TransMasks.find(sMaskName);
	if (it != m_TransMasks.end()) pMatchedImage = it->second;
	if(pMatchedImage)
	{
        m_pCurrentMask = pMatchedImage;
        return 1;
	}
	else return -1;

}

CogeImage* CogeEngine::RandomMask()
{
    int iCount = m_TransMasks.size();
    int k = rand()%iCount + 1;

    ogeImageMap::iterator it = m_TransMasks.begin();
	CogeImage* pMatchedImage = NULL;

	while (iCount > 0)
	{
		pMatchedImage = it->second;

		if(iCount == k) break;

		it++;

		iCount--;
	}

	if(pMatchedImage) m_pCurrentMask = pMatchedImage;

	return pMatchedImage;
}

int CogeEngine::GetValidPackedFilePathLength(const std::string& sResFilePath)
{
    //unsigned int index = sResFilePath.find('.');
    /*
    size_t index = sResFilePath.find_last_of('.');
    if(index == std::string::npos) return 0;
    index = sResFilePath.find_last_of('.', index);
    if(index == std::string::npos) return 0;
    else
    {
        index = sResFilePath.find('/', index);
        if(index == std::string::npos) return 0;
        else return index;
    }
    */

    if(sResFilePath.length() == 0) return 0;

    int rsl = 0;
    //string sResFilePath = "/sdcard/net.sf.oge2d/image.pack/abc.bmp";
    size_t index = sResFilePath.find_last_of('.');
    if(index == std::string::npos) rsl = 0;
    else
    {
        std::string sPath = sResFilePath.substr(0, index);
        index = sPath.find_last_of('.');
        if(index == std::string::npos) rsl = 0;
        else
        {
            index = sPath.find('/', index);
            if(index == std::string::npos) rsl = 0;
            else rsl = index;
        }
    }

    return rsl;

}

bool CogeEngine::LoadIniFile(CogeIniFile* pIniFile, const std::string& sIniFile, bool bPacked, bool bEncrypted)
{
    bool result = false;

    if(!bPacked)
    {
        result = pIniFile->Load(sIniFile);
        if(result) pIniFile->SetCurrentPath(OGE_GetAppGameDir());
    }
    else
    {
        int iPos = GetValidPackedFilePathLength(sIniFile);
        if(iPos == 0) return LoadIniFile(pIniFile, sIniFile, false, false);

        std::string sPackedFilePath = sIniFile.substr(0, iPos);
        std::string sPackedBlockPath = sIniFile.substr(iPos+1);

        //printf(sPackedFilePath.c_str());
        //printf("\n");
        //printf(sPackedBlockPath.c_str());
        //printf("\n");

        int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

        if(iSize > 0)
        {
            CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
            if(buf != NULL && buf->GetState() >= 0)
            {
                buf->Hire();

                char* pFileData = buf->GetBuffer();

                int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, bEncrypted);
                if(iReadSize > 0)
                {
                    result = pIniFile->LoadFromBuffer(pFileData, iReadSize);
                    if(result) pIniFile->SetCurrentPath(OGE_GetAppGameDir());
                }

                buf->Fire();
            }
        }
    }

    return result;
}

int CogeEngine::Initialize(int iWidth, int iHeight, int iBPP, bool bFullScreen)
{
    if(g_game != NULL) return -1;

    Finalize();

    //m_iThreadForInput = 1;

    int iBaseInit = 0;
    int iFlag = SDL_INIT_VIDEO | SDL_INIT_TIMER;
    //if(m_iThreadForInput > 0) iFlag = iFlag | SDL_INIT_EVENTTHREAD;

    iBaseInit = SDL_Init( iFlag );

    if(iBaseInit < 0)
    {
        OGE_Log( "Unable to init SDL: %s\n", SDL_GetError() );
        return -1;
    }

    atexit(SDL_Quit);

    m_pVideo = new CogeVideo();

    if(!m_pVideo) return -1;

    if (m_pVideo->GetState() < 0)
    {
        delete(m_pVideo);
        m_pVideo = NULL;
        return -1;
    }

    m_pVideo->SetDefaultResPath(OGE_GetAppGamePath());

    /*
    m_pScripter = new CogeScripter();

    if(!m_pScripter) return -1;

    if (m_pScripter->GetState() < 0)
    {
        delete(m_pScripter);
        m_pScripter = NULL;
        return -1;
    }
    */

    //if (bFullScreen) m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_SWSURFACE|SDL_FULLSCREEN);
    //else m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP);

    //if (bFullScreen) m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_SWSURFACE|SDL_FULLSCREEN);
    //else m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_HWPALETTE|SDL_ASYNCBLIT);

    m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, bFullScreen);

    if (m_iState >= 0)
    {
        m_iVideoWidth = iWidth;
        m_iVideoHeight = iHeight;
        m_iVideoBPP = iBPP;
    }

    m_pAppGameData = GetGameData(_OGE_GLOBAL_APP_DATA_NAME_, "");

    /*
    if(m_pAppGameData)
    {
        m_pAppGameData->SetIntVarCount(_OGE_GLOBAL_VAR_INT_COUNT_);
        m_pAppGameData->SetFloatVarCount(_OGE_GLOBAL_VAR_FLOAT_COUNT_);
        m_pAppGameData->SetStrVarCount(_OGE_GLOBAL_VAR_STR_COUNT_);
    }
    */

    m_pCustomData = NULL;

    g_game = this;

    return m_iState;

}

int CogeEngine::Initialize(const std::string& sConfigFileName)
{
    if(g_game != NULL) return -1;

    Finalize();

#if !SDL_VERSION_ATLEAST(2,0,0)
    SDL_putenv("SDL_VIDEO_WINDOW_POS");
    SDL_putenv("SDL_VIDEO_CENTERED=1");
#endif

    //putenv("SDL_STDIO_REDIRECT=0"); // we may set it to false(0) when in debug mode ...

    //m_iThreadForInput = 1;

    int iBaseInit = 0;
    //int iFlag = SDL_INIT_VIDEO | SDL_INIT_TIMER;
    int iFlag = SDL_INIT_VIDEO;
    //if(m_iThreadForInput > 0) iFlag = iFlag | SDL_INIT_EVENTTHREAD;

    iBaseInit = SDL_Init( iFlag );

    if(iBaseInit < 0)
    {
        OGE_Log( "Unable to init SDL: %s\n", SDL_GetError() );
        return -1;
    }

    atexit(SDL_Quit);

    m_pVideo = new CogeVideo();

    if(!m_pVideo) return -1;

    if (m_pVideo->GetState() < 0)
    {
        delete(m_pVideo);
        m_pVideo = NULL;
        return -1;
    }

    m_pVideo->SetDefaultResPath(OGE_GetAppGamePath());

    m_pAudio = new CogeAudio();
    if(m_pAudio)
    {
        m_pAudio->Initialize();
        if(!m_pAudio->IsAudioOK() || !m_pAudio->IsAudioOpen())
        {
            delete m_pAudio;
            m_pAudio = NULL;
        }
    }

    if(m_pVideo && m_pAudio)
    {
        m_pPlayer = new CogeMoviePlayer();
        m_pPlayer->Initialize(m_pVideo, m_pAudio);
        if (m_pPlayer->GetState() < 0)
        {
            delete(m_pPlayer);
            m_pPlayer = NULL;
        }
    }

    m_pNetwork = new CogeNetwork();
    if(m_pNetwork)
    {
        m_pNetwork->Initialize();
        if(m_pNetwork->GetState() < 0)
        {
            delete m_pNetwork;
            m_pNetwork = NULL;
        }
    }

    m_pIM = NULL;
    //m_pIM = new CogeIM();
    /*
    if(m_pIM)
    {
        m_pIM->Initialize();
        if (m_pIM->GetState() < 0)
        {
            delete(m_pIM);
            m_pIM = NULL;
        }
    }
    */

    m_pDatabase = new CogeDatabase();
    if(m_pDatabase)
    {
        if (m_pDatabase->GetValidTypes() == 0)
        {
            delete(m_pDatabase);
            m_pDatabase = NULL;
        }
    }

    //m_pAppGameData = GetGameData(_OGE_GLOBAL_APP_DATA_NAME_);
    //m_pAppGameData = NewFixedGameData(_OGE_GLOBAL_APP_DATA_NAME_);

    /*
    if(m_pAppGameData)
    {
        m_pAppGameData->SetIntVarCount(_OGE_GLOBAL_VAR_INT_COUNT_);
        m_pAppGameData->SetFloatVarCount(_OGE_GLOBAL_VAR_FLOAT_COUNT_);
        m_pAppGameData->SetStrVarCount(_OGE_GLOBAL_VAR_STR_COUNT_);
    }
    */

    m_pCustomData = NULL;

    //LockCPS(200);

    /*
    m_pScripter = new CogeScripter();

    if(!m_pScripter) return -1;

    if (m_pScripter->GetState() < 0)
    {
        delete(m_pScripter);
        m_pScripter = NULL;
        return -1;
    }
    */

    //CogeIniFile ini;

    if (m_AppIniFile.Load(sConfigFileName))
    {
        //m_AppIniFile.Show();

        m_AppIniFile.SetCurrentPath(OGE_GetAppGameDir());

        m_sName = m_AppIniFile.ReadString("Game", "Name", "");
        m_sTitle = m_AppIniFile.ReadString("Game", "Title", "Game");

        std::string sIconFile = m_AppIniFile.ReadFilePath("Game", "Icon", "");
        std::string sIconMask = m_AppIniFile.ReadString("Game", "IconMask", "");

        //OGE_Log("sIconFile: [%s] \n", sIconFile.c_str());

        int iCps = m_AppIniFile.ReadInteger("Game", "CPS", 0);

        if(iCps > 0) LockCPS(iCps);
        else if(iCps < 0) UnlockCPS();
        else if(iCps == 0) LockCPS(_OGE_DEFAULT_CPS_);

#ifndef __OGE_WITH_GLWIN__
        if (m_sTitle.length() > 0) m_pVideo->SetWindowCaption(m_sTitle);
        if (sIconFile.length() > 0) m_pVideo->SetWindowIcon(sIconFile, sIconMask);
#endif

        //if (m_sTitle.length() > 0) m_pVideo->SetWindowCaption(m_sTitle);
        //if (sIconFile.length() > 0) m_pVideo->SetWindowIcon(sIconFile, sIconMask);

        int iWidth  = m_AppIniFile.ReadInteger("Screen", "Width", 640);
        int iHeight = m_AppIniFile.ReadInteger("Screen", "Height", 480);
        int iBPP    = m_AppIniFile.ReadInteger("Screen", "BPP", 32);
        bool bFullScreen = m_AppIniFile.ReadInteger("Screen", "FullScreen", 0) != 0;

        int iFps = m_AppIniFile.ReadInteger("Screen", "FPS", 0);

        m_bShowFPS = m_AppIniFile.ReadInteger("Screen", "ShowFPS", 0) != 0;
        m_bShowVideoMode = m_AppIniFile.ReadInteger("Screen", "ShowVideoMode", 0) != 0;
        m_bShowMousePos = m_AppIniFile.ReadInteger("Screen", "ShowMousePos", 0) != 0;

        m_bPackedIndex  = m_AppIniFile.ReadInteger("Pack", "Index",  0) != 0;
        m_bPackedImage  = m_AppIniFile.ReadInteger("Pack", "Image",  0) != 0;
        m_bPackedMedia  = m_AppIniFile.ReadInteger("Pack", "Media",  0) != 0;
        m_bPackedFont   = m_AppIniFile.ReadInteger("Pack", "Font",   0) != 0;
        m_bPackedScript = m_AppIniFile.ReadInteger("Pack", "Script", 0) != 0;
        m_bPackedSprite = m_AppIniFile.ReadInteger("Pack", "Sprite", 0) != 0;
        m_bPackedScene  = m_AppIniFile.ReadInteger("Pack", "Scene",  0) != 0;

        m_bEncryptedIndex  = m_AppIniFile.ReadInteger("Encrypt", "Index",  0) != 0;
        m_bEncryptedImage  = m_AppIniFile.ReadInteger("Encrypt", "Image",  0) != 0;
        m_bEncryptedMedia  = m_AppIniFile.ReadInteger("Encrypt", "Media",  0) != 0;
        m_bEncryptedFont   = m_AppIniFile.ReadInteger("Encrypt", "Font",   0) != 0;
        m_bEncryptedScript = m_AppIniFile.ReadInteger("Encrypt", "Script", 0) != 0;
        m_bEncryptedSprite = m_AppIniFile.ReadInteger("Encrypt", "Sprite", 0) != 0;
        m_bEncryptedScene  = m_AppIniFile.ReadInteger("Encrypt", "Scene",  0) != 0;

        if(m_pScripter)
        {
            int iScriptBinMode = m_AppIniFile.ReadInteger("Script", "BinMode",  0);
            m_bBinaryScript = iScriptBinMode != 0;
            m_pScripter->SetBinaryMode(iScriptBinMode);
        }

        std::string sAppConfigFile = m_AppIniFile.ReadFilePath("Game", "Config", "");
        if(sAppConfigFile.length()>0)
        {
            m_UserConfigFile.Load(sAppConfigFile);
            m_UserConfigFile.SetCurrentPath(OGE_GetAppGameDir());
        }

        m_sAppDefaultDbFile = m_AppIniFile.ReadFilePath("Game", "Database", "");
        if(m_sAppDefaultDbFile.length()>0)
        {
            if(!OGE_IsFileExisted(m_sAppDefaultDbFile.c_str())) m_sAppDefaultDbFile = "";
        }


        //std::string sImageRegisterFile = m_AppIniFile.ReadString("Resource", "Image", "img.ini");
        std::string sImageRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Image", "img.ini");
        //m_ImageIniFile.Load(sImageRegisterFile);
        //m_ImageIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_ImageIniFile, sImageRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sFonteRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Font", "font.ini");
        //m_FontIniFile.Load(sFonteRegisterFile);
        //m_FontIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_FontIniFile, sFonteRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sAudioRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Media", "media.ini");
        //m_MediaIniFile.Load(sAudioRegisterFile);
        //m_MediaIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_MediaIniFile, sAudioRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sSpriteRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Sprite", "obj.ini");
        //m_ObjectIniFile.Load(sSpriteRegisterFile);
        //m_ObjectIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_ObjectIniFile, sSpriteRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sPathRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Path", "path.ini");
        //m_PathIniFile.Load(sPathRegisterFile);
        //m_PathIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_PathIniFile, sPathRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        //std::string sThinkerRegisterFile = m_AppIniFile.ReadPath("Resource", "AI", "ai.ini");
        //m_ThinkerIniFile.Load(sThinkerRegisterFile);
        //m_ThinkerIniFile.SetCurrentPath(OGE_GetAppGameDir());

        //std::string sEventRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Event", "event.ini");
        //m_EventIniFile.Load(sEventRegisterFile);
        //m_EventIniFile.SetCurrentPath(OGE_GetAppGameDir());
        //LoadIniFile(&m_EventIniFile, sEventRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sGroupRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Group", "group.ini");
        //m_GroupIniFile.Load(sGroupRegisterFile);
        //m_GroupIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_GroupIniFile, sGroupRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        //std::string sMapRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Map", "map.ini");
        //m_GameMapIniFile.Load(sMapRegisterFile);
        //m_GameMapIniFile.SetCurrentPath(OGE_GetAppGameDir());
        //LoadIniFile(&m_GameMapIniFile, sMapRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sDataRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Data", "data.ini");
        //m_GameDataIniFile.Load(sDataRegisterFile);
        //m_GameDataIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_GameDataIniFile, sDataRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sLibraryRegisterFile = m_AppIniFile.ReadFilePath("Resource", "Library", "lib.ini");
        //m_LibraryIniFile.Load(sLibraryRegisterFile);
        //m_LibraryIniFile.SetCurrentPath(OGE_GetAppGameDir());
        LoadIniFile(&m_LibraryIniFile, sLibraryRegisterFile, m_bPackedIndex, m_bEncryptedIndex);

        std::string sConstIntFile = m_AppIniFile.ReadFilePath("Resource", "Integer", "int.ini");
        LoadIniFile(&m_ConstIntIniFile, sConstIntFile, m_bPackedIndex, m_bEncryptedIndex);


        // global data ...
        m_pAppGameData = NewFixedGameData(m_sName);
        if(m_pAppGameData)
        {
            int iIntCount  = m_AppIniFile.ReadInteger("Data", "IntVarCount", 0);
            int iFloatCount  = m_AppIniFile.ReadInteger("Data", "FloatVarCount", 0);
            int iStrCount  = m_AppIniFile.ReadInteger("Data", "StrVarCount", 0);

            m_pAppGameData->SetIntVarCount(iIntCount);
            m_pAppGameData->SetFloatVarCount(iFloatCount);
            m_pAppGameData->SetStrVarCount(iStrCount);

            std::string sKeyName = "";
            std::string sValue = "";
            int iValue = 0;
            double fValue = 0;

            for(int i=1; i<=iIntCount; i++)
            {
                sKeyName = "IntVar" + OGE_itoa(i);
                sValue = m_AppIniFile.ReadString("Data", sKeyName, "");
                if(sValue.length() > 0)
                {
                    iValue = m_AppIniFile.ReadInteger("Data", sKeyName, 0);
                    m_pAppGameData->SetIntVar(i, iValue);
                }
            }

            for(int i=1; i<=iFloatCount; i++)
            {
                sKeyName = "FloatVar" + OGE_itoa(i);
                sValue = m_AppIniFile.ReadString("Data", sKeyName, "");
                if(sValue.length() > 0)
                {
                    fValue = m_AppIniFile.ReadFloat("Data", sKeyName, 0);
                    m_pAppGameData->SetFloatVar(i, fValue);
                }
            }

            for(int i=1; i<=iStrCount; i++)
            {
                sKeyName = "StrVar" + OGE_itoa(i);
                sValue = m_AppIniFile.ReadString("Data", sKeyName, "");
                if(sValue.length() > 0)
                {
                    m_pAppGameData->SetStrVar(i, sValue);
                }
            }

        }

        LoadConstInt();

        if(m_pScripter)
        {
            //std::string sScriptFile = m_AppIniFile.ReadString("Script", "File", "");
            //std::string sScriptFile = m_AppIniFile.ReadPath("Script", "File", "");

            //std::string sScriptFile = "";
            std::string sScriptName = m_AppIniFile.ReadString("Script", "Name", "");
            //sScriptFile = m_ObjectIniFile.ReadPath("Scripts", sScriptName, "");

            //printf(sScriptName.c_str());
            //printf("\n");
            //printf(sScriptFile.c_str());
            //printf("\n");

            //printf(sScriptFile.c_str());
            //printf("\n");

            //if(sScriptFile.length() > 0) m_pScript = m_pScripter->GetScript(sScriptName, sScriptFile);

            CogeScript* pScript = NULL;

            //if(sScriptName.length() > 0) pScript = GetScript(sScriptName);
            if(sScriptName.length() > 0)
            {
                std::string sScriptFileName  = m_ObjectIniFile.ReadFilePath("Scripts", sScriptName, "");
                if(sScriptFileName.length() > 0) pScript = NewFixedScript(sScriptFileName);
            }

            if(pScript && pScript->GetState() >= 0)
            {
                m_pScript = pScript;
                m_pScript->Hire();

                std::map<std::string, int>::iterator it = m_EventIdMap.begin(); //, itb, ite;
                //itb = m_EventIdMap.begin();
                //ite = m_EventIdMap.end();

                int iCount = m_EventIdMap.size();

                int iEventId  = -1;
                std::string sEventFunctionName = "";

                //for(it=itb; it!=ite; it++)
                while(iCount > 0)
                {
                    iEventId  = -1;
                    sEventFunctionName = m_AppIniFile.ReadString("Script", it->first, "");
                    if(sEventFunctionName.length() > 0)
                    {
                        iEventId  = m_pScript->PrepareFunction(sEventFunctionName);
                        //if(iEventId >= 0) m_Events[it->second] = iEventId;
                    }

                    m_Events[it->second] = iEventId;

                    it++;

                    iCount--;
                }

                m_iScriptState = 0;

            }
        }

        //if (bFullScreen) m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_SWSURFACE|SDL_FULLSCREEN);
        //else m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP); // SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_HWPALETTE

        //if (bFullScreen) m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_SWSURFACE|SDL_FULLSCREEN);
        //else m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_HWPALETTE|SDL_ASYNCBLIT);

        //if (bFullScreen) m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, SDL_SWSURFACE|SDL_FULLSCREEN);
        //else m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP);

        m_iState = m_pVideo->Initialize(iWidth, iHeight, iBPP, bFullScreen);

        if ( m_iState >= 0 && m_pVideo )
        {
            m_iVideoWidth = m_pVideo->GetWidth();
            m_iVideoHeight = m_pVideo->GetHeight();
            m_iVideoBPP = m_pVideo->GetBPP();

            if(iFps > 0) m_pVideo->LockFPS(iFps);
            else if(iFps < 0) m_pVideo->UnlockFPS();

#ifdef __OGE_WITH_GLWIN__
            if (m_sTitle.length() > 0) m_pVideo->SetWindowCaption(m_sTitle);
            if (sIconFile.length() > 0) m_pVideo->SetWindowIcon(sIconFile, sIconMask);
#endif

        }

        if(m_iState >= 0 && m_pVideo)
        {
            m_pIM = new CogeIM(m_pVideo->GetMainWindow());
            if(m_pIM)
            {
                m_pIM->Initialize();
                if (m_pIM->GetState() < 0)
                {
                    delete(m_pIM);
                    m_pIM = NULL;
                }
            }
        }

        m_iKeyDelay = m_AppIniFile.ReadInteger("Input", "KeyDelay", OGE_DEFAULT_REPEAT_DELAY);
        m_iKeyInterval = m_AppIniFile.ReadInteger("Input", "KeyInterval", OGE_DEFAULT_REPEAT_INTERVAL);

        if ( m_iState >= 0 && m_pVideo )
        {
            std::string sDefaultCharset = m_AppIniFile.ReadString("Game", "DefaultCharset", "");
            if(sDefaultCharset.length() > 0) SetDefaultCharset(sDefaultCharset);

            std::string sSystemCharset = m_AppIniFile.ReadString("Game", "SystemCharset", "");
            if(sSystemCharset.length() > 0) SetSystemCharset(sSystemCharset);

            if(m_pIM)
            {
                if(m_pIM->IsInputMethodReady())
                {
                    if(m_pIM->GetUnicodeMode()) SetSystemCharset("UTF-16");
                }
                else SetSystemCharset("UTF-8");
            }

            // font ...
            std::string sDefaultFont = m_AppIniFile.ReadString("Font", "Name", "");
            if(sDefaultFont.length() > 0) // && OGE_FileExists(sDefaultFontFile.c_str())
            {
                //if(InstallFont("Default",  iFontSize, sDefaultFontFile)) UseFont("Default");
                int iFontSize = m_AppIniFile.ReadInteger("Font", "Size", _OGE_DOT_FONT_SIZE_);
                if(iFontSize <= 0) iFontSize = _OGE_DOT_FONT_SIZE_;
                CogeFont* pDefaultFont = GetFont(sDefaultFont, iFontSize);
                if(pDefaultFont) SetDefaultFont(pDefaultFont);
            }

            // load masks
            int iMaskCount = m_AppIniFile.ReadInteger("Mask", "Count",  0);

            for(int i=1; i<=iMaskCount; i++)
            {
                std::string sIdx = "Mask" + OGE_itoa(i);
                std::string sMaskName = m_AppIniFile.ReadString("Mask", sIdx, "");

                if(sMaskName.length() > 0)
                {
                    CogeImage* pMaskImg = GetImage(sMaskName);
                    if(pMaskImg) //m_TransMasks.push_back(pMaskImg);
                    m_TransMasks.insert(ogeImageMap::value_type(sMaskName, pMaskImg));
                }
            }

        }

        if(m_pAudio)
        {
            int iFrequency = m_AppIniFile.ReadInteger("Audio", "Frequency", 0);
            int iChannels = m_AppIniFile.ReadInteger("Audio", "Channels", 0);
            bool bNeedReset = iFrequency > 0 || iChannels > 0;
            if(iFrequency > 0 && iChannels > 0) m_pAudio->Initialize(iFrequency, iChannels);
            if(iFrequency > 0 && iChannels <= 0) m_pAudio->Initialize(iFrequency);
            if(iFrequency <= 0 && iChannels > 0) m_pAudio->Initialize(44100, iChannels);
            if(bNeedReset && m_pPlayer) m_pPlayer->Initialize(m_pVideo, m_pAudio);

            int iSoundVolume = m_AppIniFile.ReadInteger("Audio", "SoundVolume", m_pAudio->GetSoundVolume());
            int iMusicVolume = m_AppIniFile.ReadInteger("Audio", "MusicVolume", m_pAudio->GetMusicVolume());
            m_pAudio->SetSoundVolume(iSoundVolume);
            m_pAudio->SetMusicVolume(iMusicVolume);

            if(m_ObjectIniFile.GetState() >= 0)
            {
                int iSoundCount = m_ObjectIniFile.ReadInteger("Sounds", "Count", 0);
                if(iSoundCount > 0)
                {
                    m_Sounds.resize(iSoundCount + 1);
                    for(int iSoundIdx = 0; iSoundIdx <= iSoundCount; iSoundIdx++) m_Sounds[iSoundIdx] = NULL;
                }
            }
        }

        if ( m_iState >= 0 )
        {
            if(m_ObjectIniFile.GetState() >= 0)
            {
                int iGroupCount = m_ObjectIniFile.ReadInteger("Groups", "Count", 0);
                if(iGroupCount > 0)
                {
                    m_Groups.resize(iGroupCount + 1);
                    for(int iGroupIdx = 0; iGroupIdx <= iGroupCount; iGroupIdx++) m_Groups[iGroupIdx] = NULL;
                }
            }
        }

        if ( m_iState >= 0 )
        {
            if(m_ObjectIniFile.GetState() >= 0)
            {
                int iPathCount = m_ObjectIniFile.ReadInteger("Paths", "Count", 0);
                if(iPathCount > 0)
                {
                    m_Paths.resize(iPathCount + 1);
                    for(int iPathIdx = 0; iPathIdx <= iPathCount; iPathIdx++) m_Paths[iPathIdx] = NULL;
                }
            }
        }

        if(m_pNetwork && m_iScriptState >= 0)
        {
            /*
            m_pNetwork->RegisterEvents(m_Events[Event_OnAcceptClient],
                                       m_Events[Event_OnReceiveFromRemoteClient],
                                       m_Events[Event_OnRemoteClientDisconnect],
                                       m_Events[Event_OnRemoteServerDisconnect],
                                       m_Events[Event_OnReceiveFromRemoteServer]);
            */

            m_pNetwork->RegisterEvents((void*)CallbackOnAcceptClient,
                                       (void*)CallbackOnReceiveFromRemoteClient,
                                       (void*)CallbackOnRemoteClientDisconnect,
                                       (void*)CallbackOnRemoteServerDisconnect,
                                       (void*)CallbackOnReceiveFromRemoteServer,
                                       (void*)CallbackRemoteDebugService);
        }

        if(m_pScripter && m_iScriptState >= 0)
        {
            m_pScripter->RegisterCheckDebugRequestFunction((void*)CallbackCheckRemoteDebugRequest);
        }

        //const std::vector<std::string> fields = ini.GetFieldNames("Screen");

    }
    else
    {
        m_iState = m_pVideo->Initialize(640, 480, 16, false);

        if ( m_iState >= 0 && m_pVideo )
        {
            m_iVideoWidth = m_pVideo->GetWidth();
            m_iVideoHeight = m_pVideo->GetHeight();
            m_iVideoBPP = m_pVideo->GetBPP();
        }

        OGE_Log("Load Application Coonfig File Failed.\n");

        //if (m_iState >= 0) DoInit(this);
    }


    // init common SDL settings ...

	//SDL_EnableUNICODE(1);

    OpenJoysticks();

    // all basic parts are ok now ...

    LoadLibraries();

    LoadKeyMap();

    g_game = this;

    // debug ...
    if(!m_bPackedScript && !m_bBinaryScript)
    {
        std::string sDebugServerAddr = OGE_GetAppDebugServerAddr();
        if(sDebugServerAddr.length() > 0)
        {
            if(sDebugServerAddr.compare("local") == 0) StartLocalDebug();
            else StartRemoteDebug(sDebugServerAddr);
        }
    }


    // init event
    if (m_iState >= 0 && m_iScriptState >= 0)
    {
        EnableKeyRepeat();
        CallEvent(Event_OnInit);
    }

    // active first scene ...

    int iFirstSceneState = -1;

    std::string sDebugSceneName = OGE_GetAppDebugSceneName();
    if(sDebugSceneName.length() > 0)
    {
        iFirstSceneState = SetActiveScene(sDebugSceneName);
    }

    if(iFirstSceneState < 0)
    {

        // load scenes ...

        if ( m_iState >= 0 )
        {
            // load scene
            int iSceneCount = m_AppIniFile.ReadInteger("Scene", "Count",  0);

            for(int i=1; i<=iSceneCount; i++)
            {
                std::string sIdx = "Scene" + OGE_itoa(i);
                std::string sSceneName = m_AppIniFile.ReadString("Scene", sIdx, "");

                if(sSceneName.length() > 0) GetScene(sSceneName);
            }

        }


        std::string sActiveSceneName = m_AppIniFile.ReadString("Scene", "Active", "");
        if(sActiveSceneName.length() > 0)
        {
            SetActiveScene(sActiveSceneName);
        }
    }

    return m_iState;

}

void CogeEngine::Finalize()
{
    g_game = NULL;

    StopRemoteDebug();

    StopAudio();

    m_iState = -1;
    m_iScriptState = -1;

    //m_EventMap.clear();
    //m_SoundCodeMap.clear();

    m_Sounds.clear();
    m_Groups.clear();
    m_Paths.clear();

    DelAllScenes();

    DelAllGroups();

    //DelAllSprites();

    DelAllPaths();

    //DelAllThinkers();

    DelAllMaps();

    DelAllGameData();

    DelAllIniFiles();

    DelAllStreamBufs();

    m_pAppGameData = NULL;

    m_pCustomData = NULL;

    m_TransMasks.clear();

    if(m_pScripter && m_pScript)
    {
        //m_pScript->Fire();
        //m_pScripter->DelScript(m_pScript->GetName());

        m_pScripter->DelScript(m_pScript);

    }
    m_pScript = NULL;

    if(m_pDatabase)
    {
        delete(m_pDatabase);
        m_pDatabase = NULL;
    }

    if(m_pIM)
    {
        delete(m_pIM);
        m_pIM = NULL;
    }

    if(m_pNetwork)
    {
        delete(m_pNetwork);
        m_pNetwork = NULL;
    }

    if (m_pPlayer)
    {
        delete(m_pPlayer);
        m_pPlayer = NULL;
    }

    if(m_pAudio)
    {
        delete(m_pAudio);
        m_pAudio = NULL;
    }

    if(m_pVideo)
    {
        delete(m_pVideo);
        m_pVideo = NULL;
    }

    m_InitScenes.clear();
    m_InitSprites.clear();

    //g_game = NULL;

}

int CogeEngine::Run()
{
    //SDL_Thread* mouse_thread = NULL;
	//SDL_Thread* keybd_thread = NULL;

    if (m_iState != 0) return m_iState;

    m_pVideo->Resume(); // ensure screen will be updated ...

    if (m_pVideo->GetState() <= 0)
    {
        m_iState = -1;
        return m_iState;
    }

    // ready to run ...
    if (m_iState == 0) m_iState = 1;

    m_iMouseX = m_pVideo->GetWidth()/2;
    m_iMouseY = m_pVideo->GetHeight()/2;

    //SDL_WarpMouse(m_iMouseX , m_iMouseY);
    m_pVideo->OGE_WarpMouse(m_iMouseX , m_iMouseY);

    while (m_iState > 0)
    {
        int iCurrentTime = SDL_GetTicks();

        if (m_iLockedCPS > 0 && m_iCycleInterval > 0)
        {
            while (iCurrentTime - m_iGlobalTime < m_iCycleInterval)
            {
                SDL_Delay(1);  // cool down cpu time ...
                iCurrentTime = SDL_GetTicks();
            }

            m_iCurrentInterval = iCurrentTime - m_iGlobalTime;

        }
        else
        {
            m_iCycleInterval = iCurrentTime - m_iGlobalTime;
            m_iCurrentInterval = m_iCycleInterval;
        }


        m_iCycleCount++;

        if (iCurrentTime - m_iLastCycleTime >= 1000)
        {
            m_iOldCycleRate = m_iCycleRate;
            m_iCycleRate = m_iCycleCount;
            m_iCycleCount = 0;
            m_iLastCycleTime = iCurrentTime;
        }

        m_iGlobalTime = iCurrentTime;

        // start to update ...

        HandleAppEvents();
        UpdateMouseInput();

        HandleNetworkEvents();

        HandleEngineEvents();
        UpdateActiveScene();

        if (m_iState > 0 && m_iScriptState >= 0) CallEvent(Event_OnUpdate);

    }

    CloseIM();

    StopAudio();

    CloseJoysticks();

    if (m_iScriptState >= 0) CallEvent(Event_OnFinalize);

    return m_iState;
}

CogeScene* CogeEngine::NewScene(const std::string& sSceneName, const std::string& sConfigFileName)
{
    ogeSceneMap::iterator it;

    it = m_SceneMap.find(sSceneName);
	if (it != m_SceneMap.end())
	{
        OGE_Log("The scene name '%s' is in use.\n", sSceneName.c_str());
        return NULL;
	}

    CogeScene* pNewScene = new CogeScene(sSceneName, this);
    //m_pLoadingScene = pNewScene;
    pNewScene->Initialize(sConfigFileName);
    //m_pLoadingScene = NULL;
    if(pNewScene->m_iState >= 0 && pNewScene->m_iScriptState >= 0)
    {
        m_SceneMap.insert(ogeSceneMap::value_type(sSceneName, pNewScene));
        return pNewScene;
    }
    else
    {
        delete pNewScene;
        return NULL;
    }

}

CogeScene* CogeEngine::FindScene(const std::string& sSceneName)
{
    ogeSceneMap::iterator it;

    it = m_SceneMap.find(sSceneName);
	if (it != m_SceneMap.end()) return it->second;
	else return NULL;
}


CogeScene* CogeEngine::GetScene(const std::string& sSceneName, const std::string& sConfigFileName)
{
    CogeScene* pMatchedScene = FindScene(sSceneName);
    if (pMatchedScene) return pMatchedScene;
    else return NewScene(sSceneName, sConfigFileName);
}

CogeScene* CogeEngine::GetScene(const std::string& sSceneName)
{
    std::string sConfigFileName = m_ObjectIniFile.ReadFilePath("Scenes", sSceneName, "");

    if (sConfigFileName.length() <= 0)
    {
        //printf("Scene not registered.\n");
        return NULL;
    }
    return GetScene(sSceneName, sConfigFileName);
}

int CogeEngine::GetSceneCount()
{
    return m_SceneMap.size();
}
CogeScene* CogeEngine::GetSceneByIndex(int idx)
{
    CogeScene* pScene = NULL;
    int iCount = m_SceneMap.size();
    ogeSceneMap::iterator it = m_SceneMap.begin();
    ogeSceneMap::iterator ite = m_SceneMap.end();
    for(int i=0; i<iCount; i++)
    {
        if(it == ite) break;
        if(idx == i)
        {
            pScene = it->second;
            break;
        }
        it++;
    }
    return pScene;
}

int CogeEngine::DelScene(const std::string& sSceneName)
{
    if(sSceneName.length() == 0) return 0;

    if(sSceneName.compare(m_sActiveSceneName) == 0
       || sSceneName.compare(m_sNextSceneName) == 0)
    {
        return 0;
    }

    CogeScene* pMatchedScene = NULL;
    ogeSceneMap::iterator its = m_SceneMap.find(sSceneName);
	if (its != m_SceneMap.end()) pMatchedScene = its->second;

    if (pMatchedScene)
    {
        if(pMatchedScene == m_pActiveScene
           || pMatchedScene == m_pNextActiveScene)
        {
            return 0;
        }
    }

	if (pMatchedScene)
	{
	    if(pMatchedScene == m_pLastActiveScene) m_pLastActiveScene = NULL;

		m_SceneMap.erase(its);
		delete pMatchedScene;
		return 1;

	}
	return 0;
}

int CogeEngine::DelScene(CogeScene* pScene)
{
	if(!pScene) return 0;

    bool bFound = false;
    CogeScene* pMatchedScene = NULL;
    ogeSceneMap::iterator its;

    for(its = m_SceneMap.begin(); its != m_SceneMap.end(); its++)
    {
        if(its->second == pScene)
        {
            bFound = true;
            break;
        }
    }
    if(bFound) pMatchedScene = its->second;

    if (pMatchedScene)
    {
        if(pMatchedScene == m_pActiveScene
           || pMatchedScene == m_pNextActiveScene)
        {
            return 0;
        }
    }

	if (pMatchedScene)
	{
	    if(pMatchedScene == m_pLastActiveScene) m_pLastActiveScene = NULL;

		m_SceneMap.erase(its);
		delete pMatchedScene;
		return 1;

	}

	return 0;



}

void CogeEngine::DelAllScenes()
{
    // free all Scenes
	ogeSceneMap::iterator it;
	CogeScene* pMatchedScene = NULL;

	it = m_SceneMap.begin();
	while (it != m_SceneMap.end())
	{
		pMatchedScene = it->second;
		m_SceneMap.erase(it);
		delete pMatchedScene;
		it = m_SceneMap.begin();
	}

    /*
	it = m_SceneMap.begin();
	int iCount = m_SceneMap.size();
	for(int i=0; i<iCount; i++)
	{
	    pMatchedScene = it->second;
	    delete pMatchedScene;
	    it++;
	}
	*/

	m_SceneMap.clear();
}

int CogeEngine::SetActiveScene(const std::string& sSceneName)
{
    if(m_iState < 0) return -1;

    //CogeScene* pScene = FindScene(sSceneName);
    CogeScene* pScene = GetScene(sSceneName);  // even it had not been loaded before ...

	if (pScene) return SetActiveScene(pScene);
	else return -1;

}

int CogeEngine::SetActiveScene(CogeScene* pActiveScene)
{
    if(m_iState < 0) return -1;

    if (pActiveScene)
	{
	    //StopAudio();

	    DisableIM();

	    if(m_pActiveScene) m_pActiveScene->Deactivate();
	    m_pLastActiveScene = m_pActiveScene;
	    m_pActiveScene = pActiveScene;
	    m_sActiveSceneName = m_pActiveScene->m_sName;

	    if(m_pScripter) m_pScripter->GC();

	    if(m_pActiveScene->m_pScreen == NULL) m_pActiveScene->m_pScreen = m_pVideo->GetDefaultScreen();

	    if(m_pActiveScene->m_pBackground == NULL)
	    {
            m_pActiveScene->m_pBackground = m_pVideo->GetDefaultBg();
            m_pVideo->ClearDefaultBg(m_pActiveScene->m_iBackgroundColor);
	    }

	    m_pVideo->SetScreen(m_pActiveScene->m_pScreen);
	    m_pVideo->DrawImage(m_pActiveScene->m_pBackground, 0, 0);

	    m_pActiveScene->Activate();

	    if(m_pActiveScene->m_sChapterName.length() > 0)
        {
            std::vector<CogeScene*> otherScenes;
            ogeSceneMap::iterator its;
            for(its = m_SceneMap.begin(); its != m_SceneMap.end(); its++)
            {
                if(its->second == m_pActiveScene) continue;
                else if(its->second->m_sChapterName.length() > 0)
                {
                    if(its->second->m_sChapterName.compare(m_pActiveScene->m_sChapterName) != 0)
                    {
                        otherScenes.push_back(its->second);
                    }
                }
            }

            int iOtherCount = otherScenes.size();
            for(int i=0; i<iOtherCount; i++)
            {
                CogeScene* pMatchedScene = otherScenes[i];
                if(pMatchedScene) DelScene(pMatchedScene);
            }

        }

		return 1;

	}
	else return -1;
}

int CogeEngine::SetNextScene(const std::string& sSceneName)
{
    if(m_pActiveScene) return m_pActiveScene->SetNextScene(sSceneName);
    else return -1;
}
int CogeEngine::SetNextScene(CogeScene* pNextScene)
{
    if(m_pActiveScene) return m_pActiveScene->SetNextScene(pNextScene);
    else return -1;
}

void CogeEngine::FadeIn (int iFadeType)
{
    if(m_pActiveScene) m_pActiveScene->FadeIn(iFadeType);
}
void CogeEngine::FadeOut(int iFadeType)
{
    if(m_pActiveScene) m_pActiveScene->FadeOut(iFadeType);
}

CogeSprite* CogeEngine::NewSpriteWithConfig(const std::string& sSpriteName, const std::string& sConfigFileName)
{
    /*
    ogeSpriteMap::iterator it;

    it = m_SpriteMap.find(sSpriteName);
	if (it != m_SpriteMap.end())
	{
        OGE_Log("The sprite name '%s' is in use.\n", sSpriteName.c_str());
        return NULL;
	}
	*/

    CogeSprite* pNewSprite = new CogeSprite(sSpriteName, this);
    pNewSprite->Initialize(sConfigFileName);
    if(pNewSprite->m_iState >= 0 && pNewSprite->m_iScriptState >= 0)
    {
        //m_SpriteMap.insert(ogeSpriteMap::value_type(sSpriteName, pNewSprite)); // this will be done in sprite's init function
        return pNewSprite;
    }
    else
    {
        delete pNewSprite;
        return NULL;
    }
}

CogeSprite* CogeEngine::NewSprite(const std::string& sSpriteName, const std::string& sBaseName)
{
    std::string sConfigFileName = m_ObjectIniFile.ReadFilePath("Sprites", sBaseName, "");
    if (sConfigFileName.length() <= 0) return NULL;

    return NewSpriteWithConfig(sSpriteName, sConfigFileName);

}
/*
CogeSprite* CogeEngine::FindSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator it;

    it = m_SpriteMap.find(sSpriteName);
	if (it != m_SpriteMap.end()) return it->second;
	else return NULL;
}
CogeSprite* CogeEngine::GetSpriteWithConfig(const std::string& sSpriteName, const std::string& sConfigFileName)
{
    CogeSprite* pMatchedSprite = FindSprite(sSpriteName);
    if (!pMatchedSprite) pMatchedSprite = NewSpriteWithConfig(sSpriteName, sConfigFileName);
    //if (pMatchedSprite) pMatchedSprite->Hire();
    return pMatchedSprite;
}
CogeSprite* CogeEngine::GetSprite(const std::string& sSpriteName, const std::string& sBaseName)
{
    CogeSprite* pMatchedSprite = FindSprite(sSpriteName);
    if (pMatchedSprite) return pMatchedSprite;

    std::string sConfigFileName = m_ObjectIniFile.ReadFilePath("Sprites", sBaseName, "");
    if (sConfigFileName.length() <= 0) return NULL;

    return GetSpriteWithConfig(sSpriteName, sConfigFileName);
}
int CogeEngine::DelSprite(const std::string& sSpriteName)
{
    CogeSprite* pMatchedSprite = NULL;
    ogeSpriteMap::iterator it = m_SpriteMap.find(sSpriteName);
	if (it != m_SpriteMap.end()) pMatchedSprite = it->second;

	if (pMatchedSprite)
	{
	    //pMatchedSprite->Fire();
	    //if (pMatchedSprite->m_iTotalUsers > 0) return 0;
		m_SpriteMap.erase(it);
		delete pMatchedSprite;
		return 1;

	}
	return -1;
}
*/

int CogeEngine::DelSprite(CogeSprite* pSprite)
{
    /*
    if(!pSprite) return 0;

    bool bFound = false;
    CogeSprite* pMatchedSprite = NULL;
    ogeSpriteMap::iterator its;

    for(its = m_SpriteMap.begin(); its != m_SpriteMap.end(); its++)
    {
        if(its->second == pSprite)
        {
            bFound = true;
            break;
        }
    }
    if(bFound) pMatchedSprite = its->second;

	if (pMatchedSprite)
	{
	    //pMatchedSprite->Fire();
	    //if (pMatchedSprite->m_iTotalUsers > 0) return 0;
		m_SpriteMap.erase(its);
		delete pMatchedSprite;
		return 1;

	}
	return 0;
	*/

	if(pSprite == NULL) return 0;
	else
    {
        delete pSprite;
        return 1;
    }

}

/*
void CogeEngine::DelAllSprites()
{
	ogeSpriteMap::iterator it;
	CogeSprite* pMatchedSprite = NULL;


	//it = m_SpriteMap.begin();
	//while (it != m_SpriteMap.end())
	//{
	//	pMatchedSprite = it->second;
	//	m_SpriteMap.erase(it);
	//	delete pMatchedSprite;
	//	it = m_SpriteMap.begin();
	//}


    it = m_SpriteMap.begin();
	int iCount = m_SpriteMap.size();
	for(int i=0; i<iCount; i++)
	{
	    pMatchedSprite = it->second;
	    delete pMatchedSprite;
	    it++;
	}
	m_SpriteMap.clear();

}
*/

CogePath* CogeEngine::NewPath(const std::string& sPathName)
{
    ogePathMap::iterator it;

    it = m_PathMap.find(sPathName);
	if (it != m_PathMap.end())
	{
	    OGE_Log("The path name '%s' is in use.\n", sPathName.c_str());
        return NULL;
    }

    int iCount = m_PathIniFile.ReadInteger(sPathName, "Count", -1);

    if(iCount < 0) return NULL;

    int iType  = m_PathIniFile.ReadInteger(sPathName, "Type", 0);
    int iSegmentLen  = m_PathIniFile.ReadInteger(sPathName, "SegmentLength", 0); // for road parser

    CogePath* pNewPath = new CogePath();

    if(!pNewPath || !m_pVideo) return NULL;

    pNewPath->m_iType = iType;

    pNewPath->m_iSegmentLength = iSegmentLen;

    pNewPath->m_sName = sPathName;

    int x=0; int y=0; int p=0;

    int w = m_pVideo->GetWidth();

    for(int i=1; i<=iCount; i++)
    {
        std::string sPoint = m_PathIniFile.ReadString(sPathName, "P"+OGE_itoa(i), "0,0");

        size_t iPos = sPoint.find(',');
        if(iPos == std::string::npos)
        {
            p = m_PathIniFile.ReadInteger(sPathName, "P"+OGE_itoa(i), 0);
            y = p/w; x = p%w;
        }
        else
        {
            std::string sPosX = sPoint.substr(0, iPos);
            if(sPosX.length() == 0) sPosX = "0";
            std::string sPosY = sPoint.substr(iPos+1);
            if(sPosY.length() == 0) sPosY = "0";
            x = atoi(sPosX.c_str()); y = atoi(sPosY.c_str());
        }

        CogePoint* pPoint = new CogePoint();
        pPoint->x = x;
        pPoint->y = y;

        pNewPath->m_Keys.push_back(pPoint);
    }

    if(iType == 0 && iCount > 0) // points
    {
        pNewPath->m_Points.resize(iCount);
        for(int i=1; i<=iCount; i++)
        {
            CogePoint* pPoint = new CogePoint();
            pPoint->x = pNewPath->m_Keys[i-1]->x;
            pPoint->y = pNewPath->m_Keys[i-1]->y;

            pNewPath->m_Points[i-1] = pPoint;
        }
    }

    if(iType == 1 && iCount > 0) // straight lines
    {
        pNewPath->ParseLineRoad();
    }

    if(iType == 2 && iCount > 0) // curves
    {
        pNewPath->ParseBezierRoad();
    }

    m_PathMap.insert(ogePathMap::value_type(sPathName, pNewPath));

    return pNewPath;

}
CogePath* CogeEngine::FindPath(const std::string& sPathName)
{
    ogePathMap::iterator it;

    it = m_PathMap.find(sPathName);
	if (it != m_PathMap.end()) return it->second;
	else return NULL;
}
CogePath* CogeEngine::GetPath(const std::string& sPathName)
{
    CogePath* pMatchedPath = FindPath(sPathName);
    if (!pMatchedPath) pMatchedPath = NewPath(sPathName);
    //if (pMatchedSprite) pMatchedSprite->Hire();
    return pMatchedPath;
}

CogePath* CogeEngine::GetPath(int iPathCode)
{
    int iTotal = m_Paths.size();
    if(iPathCode < 0 || iPathCode >= iTotal) return NULL;
    CogePath* pPath = m_Paths[iPathCode];
    if(pPath == NULL)
    {
        std::string sPathName  = m_ObjectIniFile.ReadString("Paths", "Path"+OGE_itoa(iPathCode), "");
        if(sPathName.length() > 0)
        {
            pPath = GetPath(sPathName);
            m_Paths[iPathCode] = pPath;
        }
    }

    return pPath;
}

int CogeEngine::DelPath(const std::string& sPathName)
{
    CogePath* pMatchedPath = NULL;
    ogePathMap::iterator it = m_PathMap.find(sPathName);
	if (it != m_PathMap.end()) pMatchedPath = it->second;

	if (pMatchedPath)
	{
	    int iPathCount = m_Paths.size();
        for(int i = 0; i < iPathCount; i++)
        {
            if(m_Paths[i] == pMatchedPath)
            {
                m_Paths[i] = NULL;
                break;
            }
        }

	    //pMatchedPath->Fire();
	    //if (pMatchedSprite->m_iTotalUsers > 0) return false;
		m_PathMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedPath;
		return 1;

	}
	return -1;
}

CogeGameData* CogeEngine::NewFixedGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount)
{
    //std::string sName = OGE_itoa(m_GameDataList.size()+1);
    //CogeGameData* pNewGameData = new CogeGameData("D" + sName);

    CogeGameData* pNewGameData = new CogeGameData(sGameDataName);

    if(iIntCount > 0) pNewGameData->SetIntVarCount(iIntCount);
    if(iFloatCount > 0) pNewGameData->SetFloatVarCount(iFloatCount);
    if(iStrCount > 0) pNewGameData->SetStrVarCount(iStrCount);

    m_GameDataList.push_back(pNewGameData);

    return pNewGameData;
}

CogeGameData* CogeEngine::NewGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount)
{
    ogeGameDataMap::iterator it;

    it = m_GameDataMap.find(sGameDataName);
	if (it != m_GameDataMap.end())
	{
	    OGE_Log("The GameData name '%s' is in use.\n", sGameDataName.c_str());
        return NULL;
    }

    CogeGameData* pNewGameData = new CogeGameData(sGameDataName);

    if(iIntCount > 0) pNewGameData->SetIntVarCount(iIntCount);
    if(iFloatCount > 0) pNewGameData->SetFloatVarCount(iFloatCount);
    if(iStrCount > 0) pNewGameData->SetStrVarCount(iStrCount);

    m_GameDataMap.insert(ogeGameDataMap::value_type(sGameDataName, pNewGameData));

    return pNewGameData;
}
CogeGameData* CogeEngine::NewGameData(const std::string& sGameDataName, const std::string& sTemplateName)
{
    ogeGameDataMap::iterator it;

    it = m_GameDataMap.find(sGameDataName);
	if (it != m_GameDataMap.end())
	{
	    OGE_Log("The GameData name '%s' is in use.\n", sGameDataName.c_str());
        return NULL;
    }

    CogeGameData* pNewGameData = new CogeGameData(sGameDataName);

    if(sTemplateName.length() > 0)
    {
        int iIntCount  = m_GameDataIniFile.ReadInteger(sTemplateName, "IntVarCount", 0);
        int iFloatCount  = m_GameDataIniFile.ReadInteger(sTemplateName, "FloatVarCount", 0);
        int iStrCount  = m_GameDataIniFile.ReadInteger(sTemplateName, "StrVarCount", 0);

        if(iIntCount > 0) pNewGameData->SetIntVarCount(iIntCount);
        if(iFloatCount > 0) pNewGameData->SetFloatVarCount(iFloatCount);
        if(iStrCount > 0) pNewGameData->SetStrVarCount(iStrCount);

        if(pNewGameData)
        {
            std::string sKeyName = "";
            std::string sValue = "";
            int iValue = 0;
            double fValue = 0;

            int iIntCount = pNewGameData->GetIntVarCount();
            for(int i=1; i<=iIntCount; i++)
            {
                sKeyName = "IntVar" + OGE_itoa(i);
                sValue = m_GameDataIniFile.ReadString(sTemplateName, sKeyName, "");
                if(sValue.length() > 0)
                {
                    iValue = m_GameDataIniFile.ReadInteger(sTemplateName, sKeyName, 0);
                    pNewGameData->SetIntVar(i, iValue);
                }
            }

            int iFloatCount = pNewGameData->GetFloatVarCount();
            for(int i=1; i<=iFloatCount; i++)
            {
                sKeyName = "FloatVar" + OGE_itoa(i);
                sValue = m_GameDataIniFile.ReadString(sTemplateName, sKeyName, "");
                if(sValue.length() > 0)
                {
                    fValue = m_GameDataIniFile.ReadFloat(sTemplateName, sKeyName, 0);
                    pNewGameData->SetFloatVar(i, fValue);
                }
            }

            int iStrCount = pNewGameData->GetStrVarCount();
            for(int i=1; i<=iStrCount; i++)
            {
                sKeyName = "StrVar" + OGE_itoa(i);
                sValue = m_GameDataIniFile.ReadString(sTemplateName, sKeyName, "");
                if(sValue.length() > 0)
                {
                    pNewGameData->SetStrVar(i, sValue);
                }
            }
        }

    }

    m_GameDataMap.insert(ogeGameDataMap::value_type(sGameDataName, pNewGameData));

    return pNewGameData;
}
CogeGameData* CogeEngine::FindGameData(const std::string& sGameDataName)
{
    ogeGameDataMap::iterator it;

    it = m_GameDataMap.find(sGameDataName);
	if (it != m_GameDataMap.end()) return it->second;
	else return NULL;
}
CogeGameData* CogeEngine::GetGameData(const std::string& sGameDataName, const std::string& sTemplateName)
{
    CogeGameData* pMatchedGameData = FindGameData(sGameDataName);
    if (!pMatchedGameData) pMatchedGameData = NewGameData(sGameDataName, sTemplateName);
    return pMatchedGameData;
}
CogeGameData* CogeEngine::GetGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount)
{
    CogeGameData* pMatchedGameData = FindGameData(sGameDataName);
    if (!pMatchedGameData) pMatchedGameData = NewGameData(sGameDataName, iIntCount, iFloatCount, iStrCount);
    return pMatchedGameData;
}
int CogeEngine::DelGameData(const std::string& sGameDataName)
{
    CogeGameData* pMatchedGameData = FindGameData(sGameDataName);
	if (pMatchedGameData)
	{
	    //if(pMatchedGameData->m_iTotalUsers > 0) return 0;
        m_GameDataMap.erase(sGameDataName);
        delete pMatchedGameData;
        return 1;

	}
	return -1;
}

int CogeEngine::DelGameData(CogeGameData* pGameData)
{
	if(!pGameData) return 0;

    bool bFound = false;

    if(!bFound)
    {
        ogeGameDataMap::iterator it;
        CogeGameData* pMatchedGameData = NULL;

        it = m_GameDataMap.begin();

        while (it != m_GameDataMap.end())
        {
            pMatchedGameData = it->second;
            if(pMatchedGameData == pGameData)
            {
                bFound = true;
                break;
            }
            it++;
        }

        if(bFound)
        {
            m_GameDataMap.erase(it);
            if(pMatchedGameData) delete pMatchedGameData;
        }

    }

    if(!bFound)
    {
        ogeGameDataList::iterator itx;
        CogeGameData* pMatchedGameData = NULL;

        itx = m_GameDataList.begin();

        while (itx != m_GameDataList.end())
        {
            pMatchedGameData = *itx;
            if(pMatchedGameData == pGameData)
            {
                bFound = true;
                break;
            }
            itx++;
        }

        if(bFound)
        {
            m_GameDataList.erase(itx);
            if(pMatchedGameData) delete pMatchedGameData;
        }
    }

    if(bFound) return 1;
    else return 0;
}

int CogeEngine::SaveGameDataToIniFile(CogeGameData* pGameData, CogeIniFile* pIniFile)
{
    if(pGameData == NULL || pIniFile == NULL) return -1;

    std::string sSection = pGameData->GetName();

    std::string sKeyName = "";
    std::string sValue = "";
    int iValue = 0;
    double fValue = 0;

    int iIntCount = pGameData->GetIntVarCount();
    pIniFile->WriteInteger(sSection, "IntVarCount", iIntCount);
    for(int i=1; i<=iIntCount; i++)
    {
        sKeyName = "IntVar" + OGE_itoa(i);
        iValue = pGameData->GetIntVar(i);
        pIniFile->WriteInteger(sSection, sKeyName, iValue);
    }

    int iFloatCount = pGameData->GetFloatVarCount();
    pIniFile->WriteInteger(sSection, "FloatVarCount", iFloatCount);
    for(int i=1; i<=iFloatCount; i++)
    {
        sKeyName = "FloatVar" + OGE_itoa(i);
        fValue = pGameData->GetFloatVar(i);
        pIniFile->WriteFloat(sSection, sKeyName, fValue);
    }

    int iStrCount = pGameData->GetStrVarCount();
    pIniFile->WriteInteger(sSection, "StrVarCount", iStrCount);
    for(int i=1; i<=iStrCount; i++)
    {
        sKeyName = "StrVar" + OGE_itoa(i);
        sValue = pGameData->GetStrVar(i);
        pIniFile->WriteString(sSection, sKeyName, sValue);
    }

    pIniFile->Save();

    return iIntCount + iFloatCount + iStrCount;

}

int CogeEngine::LoadGameDataFromIniFile(CogeGameData* pGameData, CogeIniFile* pIniFile)
{
    if(pGameData == NULL || pIniFile == NULL) return -1;
    std::string sSection = pGameData->GetName();

    //pGameData->Clear();

    int iIntCount  = pIniFile->ReadInteger(sSection, "IntVarCount", -1);
    int iFloatCount  = pIniFile->ReadInteger(sSection, "FloatVarCount", -1);
    int iStrCount  = pIniFile->ReadInteger(sSection, "StrVarCount", -1);

    if(iIntCount >= 0 && iFloatCount >= 0 && iStrCount >= 0) pGameData->Clear();

    if(iIntCount >= 0) pGameData->SetIntVarCount(iIntCount); else iIntCount = 0;
    if(iFloatCount >= 0)pGameData->SetFloatVarCount(iFloatCount); else iFloatCount = 0;
    if(iStrCount >= 0)pGameData->SetStrVarCount(iStrCount); else iStrCount = 0;

    std::string sKeyName = "";
    std::string sValue = "";
    int iValue = 0;
    double fValue = 0;

    for(int i=1; i<=iIntCount; i++)
    {
        sKeyName = "IntVar" + OGE_itoa(i);
        sValue = pIniFile->ReadString(sSection, sKeyName, "");
        if(sValue.length() > 0)
        {
            iValue = pIniFile->ReadInteger(sSection, sKeyName, 0);
            pGameData->SetIntVar(i, iValue);
        }
    }

    for(int i=1; i<=iFloatCount; i++)
    {
        sKeyName = "FloatVar" + OGE_itoa(i);
        sValue = pIniFile->ReadString(sSection, sKeyName, "");
        if(sValue.length() > 0)
        {
            fValue = pIniFile->ReadFloat(sSection, sKeyName, 0);
            pGameData->SetFloatVar(i, fValue);
        }
    }

    for(int i=1; i<=iStrCount; i++)
    {
        sKeyName = "StrVar" + OGE_itoa(i);
        sValue = pIniFile->ReadString(sSection, sKeyName, "");
        pGameData->SetStrVar(i, sValue);
    }

    return iIntCount + iFloatCount + iStrCount;
}

int CogeEngine::SaveGameDataToDb(CogeGameData* pGameData, int iSessionId)
{
    if(m_pDatabase == NULL || pGameData == NULL || iSessionId == 0) return -1;

    int iRsl = -1;
    int iTotal = 0;

    std::string sSection = pGameData->GetName();
    int iIntCount = pGameData->GetIntVarCount();
    int iFloatCount = pGameData->GetFloatVarCount();
    int iStrCount = pGameData->GetStrVarCount();

    iRsl = m_pDatabase->RunSql(iSessionId, "delete from datasum where game_data = '" + sSection + "';");
    if(iRsl < 0) return iRsl;
    else
    {
        std::string sSql = "insert into datasum (game_data, int_count, float_count, str_count) values ('"
                                            + sSection +"', "
                                            + OGE_itoa(iIntCount) + ", " + OGE_itoa(iFloatCount) + ", " + OGE_itoa(iStrCount) + ");";

        iRsl = m_pDatabase->RunSql(iSessionId, sSql);
        if(iRsl < 0) return iRsl;
    }


    iRsl = m_pDatabase->RunSql(iSessionId, "delete from intdata where game_data = '" + sSection + "';");
    if(iRsl < 0) return iRsl;
    for(int i=1; i<=iIntCount; i++)
    {
        int iValue = pGameData->GetIntVar(i);
        std::string sSql = "insert into intdata (game_data, var_index, var_value) values ('"
                                            + sSection +"', " + OGE_itoa(i) + ", " + OGE_itoa(iValue) + ");";

        iRsl = m_pDatabase->RunSql(iSessionId, sSql);
        if(iRsl < 0) return iRsl;
        else iTotal++;
    }

    iRsl = m_pDatabase->RunSql(iSessionId, "delete from floatdata where game_data = '" + sSection + "';");
    if(iRsl < 0) return iRsl;
    for(int i=1; i<=iFloatCount; i++)
    {
        double fValue = pGameData->GetFloatVar(i);
        std::string sSql = "insert into floatdata (game_data, var_index, var_value) values ('"
                                            + sSection +"', " + OGE_itoa(i) + ", " + OGE_ftoa(fValue) + ");";

        iRsl = m_pDatabase->RunSql(iSessionId, sSql);
        if(iRsl < 0) return iRsl;
        else iTotal++;
    }

    iRsl = m_pDatabase->RunSql(iSessionId, "delete from strdata where game_data = '" + sSection + "';");
    if(iRsl < 0) return iRsl;
    for(int i=1; i<=iStrCount; i++)
    {
        std::string sValue = pGameData->GetStrVar(i);
        std::string sSql = "insert into strdata (game_data, var_index, var_value) values ('"
                                            + sSection +"', " + OGE_itoa(i) + ", '" + sValue + "');";

        iRsl = m_pDatabase->RunSql(iSessionId, sSql);
        if(iRsl < 0) return iRsl;
        else iTotal++;
    }

    return iTotal;

}
int CogeEngine::LoadGameDataFromDb(CogeGameData* pGameData, int iSessionId)
{
    if(m_pDatabase == NULL || pGameData == NULL || iSessionId == 0) return -1;

    //int iRsl = -1;
    //int iTotal = 0;
    int iQueryId = 0;

    std::string sSection = pGameData->GetName();

    std::vector<int> arrInt;
    std::vector<double> arrFloat;
    std::vector<std::string> arrStr;

    int iIntCount  = 0;
    int iFloatCount  = 0;
    int iStrCount  = 0;

    iQueryId = m_pDatabase->OpenQuery(iSessionId,
                                "select * from datasum where game_data = '" + sSection + "';");
    if(iQueryId == 0) return -1;
    else
    {
        int iRecNo = m_pDatabase->FirstRecord(iQueryId);
        if(iRecNo != 0)
        {
            iIntCount = m_pDatabase->GetIntFieldValue(iQueryId, "int_count");
            iFloatCount = m_pDatabase->GetIntFieldValue(iQueryId, "float_count");
            iStrCount = m_pDatabase->GetIntFieldValue(iQueryId, "str_count");
        }
        m_pDatabase->CloseQuery(iQueryId);

        if(iRecNo == 0) return 0;
    }


    if(iIntCount > 0)
    {
        iQueryId = m_pDatabase->OpenQuery(iSessionId,
                                    "select * from intdata where game_data = '" + sSection + "' order by var_id asc;");
        if(iQueryId == 0) return -1;
        else
        {
            int iRecNo = m_pDatabase->FirstRecord(iQueryId);
            while(iRecNo != 0)
            {
                int iValue = m_pDatabase->GetIntFieldValue(iQueryId, "var_value");
                arrInt.push_back(iValue);
                iRecNo = m_pDatabase->NextRecord(iQueryId);
            }
            m_pDatabase->CloseQuery(iQueryId);
        }
    }

    if(iFloatCount > 0)
    {
        iQueryId = m_pDatabase->OpenQuery(iSessionId,
                                    "select * from floatdata where game_data = '" + sSection + "' order by var_id asc;");
        if(iQueryId == 0) return -1;
        else
        {
            int iRecNo = m_pDatabase->FirstRecord(iQueryId);
            while(iRecNo != 0)
            {
                double fValue = m_pDatabase->GetFloatFieldValue(iQueryId, "var_value");
                arrFloat.push_back(fValue);
                iRecNo = m_pDatabase->NextRecord(iQueryId);
            }
            m_pDatabase->CloseQuery(iQueryId);
        }
    }

    if(iStrCount > 0)
    {
        iQueryId = m_pDatabase->OpenQuery(iSessionId,
                                    "select * from strdata where game_data = '" + sSection + "' order by var_id asc;");
        if(iQueryId == 0) return -1;
        else
        {
            int iRecNo = m_pDatabase->FirstRecord(iQueryId);
            while(iRecNo != 0)
            {
                std::string sValue = m_pDatabase->GetStrFieldValue(iQueryId, "var_value");
                arrStr.push_back(sValue);
                iRecNo = m_pDatabase->NextRecord(iQueryId);
            }
            m_pDatabase->CloseQuery(iQueryId);
        }
    }


    pGameData->Clear();

    iIntCount  = arrInt.size();
    iFloatCount  = arrFloat.size();
    iStrCount  = arrStr.size();

    pGameData->SetIntVarCount(iIntCount);
    pGameData->SetFloatVarCount(iFloatCount);
    pGameData->SetStrVarCount(iStrCount);

    for(int i=1; i<=iIntCount; i++)
    {
        pGameData->SetIntVar(i, arrInt[i-1]);
    }

    for(int i=1; i<=iFloatCount; i++)
    {
        pGameData->SetFloatVar(i, arrFloat[i-1]);
    }

    for(int i=1; i<=iStrCount; i++)
    {
        pGameData->SetStrVar(i, arrStr[i-1]);
    }

    return iIntCount + iFloatCount + iStrCount;

}

/*
CogeThinker* CogeEngine::NewThinker(const std::string& sThinkerName)
{
    ogeThinkerMap::iterator itt;

    itt = m_ThinkerMap.find(sThinkerName);
	if (itt != m_ThinkerMap.end())
	{
	    printf("The thinker name is in use.\n");
        return NULL;
    }

    std::string sScriptFile = m_ThinkerIniFile.ReadPath(sThinkerName, "File", "");

    CogeScript* pScript = NULL;

    if(sScriptFile.length() > 0) pScript = m_pScripter->GetScript("AIScript_" + sThinkerName, sScriptFile);
    if(pScript==NULL || pScript->GetState() < 0) return NULL;

    CogeThinker* pNewThinker = new CogeThinker(sThinkerName);
    pNewThinker->SetScript(pScript);

    std::map<std::string, int>::iterator it = m_EventIdMap.begin(); //, itb, ite;

    int iCount = m_EventIdMap.size();

    int iEventId  = -1;
    std::string sEventFunctionName = "";

    while(iCount > 0)
    {
        iEventId  = -1;
        sEventFunctionName = m_ThinkerIniFile.ReadString(sThinkerName, it->first, "");
        if(sEventFunctionName.length() > 0)
        {
            iEventId  = pScript->PrepareFunction(sEventFunctionName);
            //if(iEventId >= 0) m_Events[it->second] = iEventId;
        }

        pNewThinker->m_Events[it->second] = iEventId;

        it++;

        iCount--;
    }

    pNewThinker->m_iState = 0;

    return pNewThinker;
}
CogeThinker* CogeEngine::FindThinker(const std::string& sThinkerName)
{
    ogeThinkerMap::iterator it;

    it = m_ThinkerMap.find(sThinkerName);
	if (it != m_ThinkerMap.end()) return it->second;
	else return NULL;
}
CogeThinker* CogeEngine::GetThinker(const std::string& sThinkerName)
{
    CogeThinker* pMatchedThinker = FindThinker(sThinkerName);
    if (!pMatchedThinker) pMatchedThinker = NewThinker(sThinkerName);
    return pMatchedThinker;
}
int CogeEngine::DelThinker(const std::string& sThinkerName)
{
    CogeThinker* pMatchedThinker = FindThinker(sThinkerName);
	if (pMatchedThinker)
	{
		m_ThinkerMap.erase(sThinkerName);
		delete pMatchedThinker;
		return 1;
	}
	return -1;
}
*/

CogeScript* CogeEngine::NewFixedScript(const std::string& sScriptFile)
{
    if(m_pScripter)
    {
        std::string sFileName  = sScriptFile;
        if(sFileName.length() == 0) return NULL;

        CogeScript* result = NULL;

        if(!m_bPackedScript) result = m_pScripter->NewFixedScript(sFileName);
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0) result = m_pScripter->NewFixedScript(sFileName);
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize + 8); // ...
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedScript);
                        if(iReadSize > 0)
                        {
                            result = m_pScripter->NewFixedScriptFromBuffer(pFileData, iReadSize);
                        }

                        buf->Fire();
                    }
                }
            }
        }

        return result;


    }
    else return NULL;
}

CogeScript* CogeEngine::NewScript(const std::string& sScriptName)
{
    if(m_pScripter)
    {
        std::string sFileName  = m_ObjectIniFile.ReadFilePath("Scripts", sScriptName, "");
        if(sFileName.length() == 0) return NULL;

        CogeScript* result = NULL;

        if(!m_bPackedScript) result = m_pScripter->NewScript(sScriptName, sFileName);
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0) result = m_pScripter->NewScript(sScriptName, sFileName);
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize + 8); // ...
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedScript);
                        if(iReadSize > 0)
                        {
                            result = m_pScripter->NewScriptFromBuffer(sScriptName, pFileData, iReadSize);
                        }

                        buf->Fire();
                    }
                }
            }
        }

        return result;


    }
    else return NULL;

}
CogeScript* CogeEngine::FindScript(const std::string& sScriptName)
{
    if(m_pScripter) return m_pScripter->FindScript(sScriptName);
    else return NULL;
}
CogeScript* CogeEngine::GetScript(const std::string& sScriptName)
{
    if(m_pScripter)
    {
        CogeScript* pMatchedScript = FindScript(sScriptName);
        if (!pMatchedScript) pMatchedScript = NewScript(sScriptName);
        return pMatchedScript;
    }
    else return NULL;
}
int CogeEngine::DelScript(const std::string& sScriptName)
{
    if(m_pScripter) return m_pScripter->DelScript(sScriptName);
    else return -1;
}

CogeSpriteGroup* CogeEngine::NewGroup(const std::string& sGroupName)
{
    ogeGroupMap::iterator it;

    it = m_GroupMap.find(sGroupName);
	if (it != m_GroupMap.end())
	{
	    OGE_Log("The group name '%s' is in use.\n", sGroupName.c_str());
        return NULL;
    }

    if(m_GroupIniFile.GetSectionByName(sGroupName) == NULL) return NULL;

    CogeSpriteGroup* pNewGroup = new CogeSpriteGroup(sGroupName, this);

    int iCount = m_GroupIniFile.ReadInteger(sGroupName, "Count", -1);
    if(iCount < 0) iCount = 0;
    CogeSprite* pFirstSprite = NULL;
    for(int i=1; i<=iCount; i++)
    {
        std::string sSpriteName = m_GroupIniFile.ReadString(sGroupName, "Sprite"+OGE_itoa(i), "");
        if(sSpriteName.length() > 0)
        {
            CogeSprite* pNewSprite = pNewGroup->AddSprite("", sSpriteName);
            if(pNewSprite && pFirstSprite == NULL)
            {
                pFirstSprite = pNewSprite;
                //pNewGroup->m_sRoot = sSpriteName;
            }
        }
    }

    std::string sResSceneName = m_GroupIniFile.ReadString(sGroupName, "SpriteRes", "");
    if(sResSceneName.length() > 0) pNewGroup->LoadSpritesFromScene(sResSceneName);

    if(m_GroupIniFile.ReadInteger(sGroupName, "AutoGen", 0) > 0)
    {
        std::string sTemplate = m_GroupIniFile.ReadString(sGroupName, "Template", "");
        int iCopyCount = m_GroupIniFile.ReadInteger(sGroupName, "CopyCount", 0);
        if(sTemplate.length() > 0 && iCopyCount > 0) pNewGroup->GenSprites(sTemplate, iCopyCount);
    }

    m_GroupMap.insert(ogeGroupMap::value_type(sGroupName, pNewGroup));

    return pNewGroup;
}
CogeSpriteGroup* CogeEngine::FindGroup(const std::string& sGroupName)
{
    ogeGroupMap::iterator it;

    it = m_GroupMap.find(sGroupName);
	if (it != m_GroupMap.end()) return it->second;
	else return NULL;
}
CogeSpriteGroup* CogeEngine::GetGroup(const std::string& sGroupName)
{
    CogeSpriteGroup* pMatchedGroup = FindGroup(sGroupName);
    if (!pMatchedGroup) pMatchedGroup = NewGroup(sGroupName);
    //if (pMatchedSprite) pMatchedSprite->Hire();
    return pMatchedGroup;
}
CogeSpriteGroup* CogeEngine::GetGroup(int iGroupCode)
{
    int iTotal = m_Groups.size();
    if(iGroupCode < 0 || iGroupCode >= iTotal) return NULL;
    CogeSpriteGroup* pGroup = m_Groups[iGroupCode];
    if(pGroup == NULL)
    {
        std::string sGroupName  = m_ObjectIniFile.ReadString("Groups", "Group"+OGE_itoa(iGroupCode), "");
        if(sGroupName.length() > 0)
        {
            pGroup = GetGroup(sGroupName);
            m_Groups[iGroupCode] = pGroup;
        }
    }

    return pGroup;
}

int CogeEngine::DelGroup(const std::string& sGroupName)
{
    CogeSpriteGroup* pMatchedGroup = NULL;
    ogeGroupMap::iterator it = m_GroupMap.find(sGroupName);
	if (it != m_GroupMap.end()) pMatchedGroup = it->second;

	if (pMatchedGroup)
	{
	    //pMatchedPath->Fire();
	    //if (pMatchedSprite->m_iTotalUsers > 0) return false;

	    CogeScene* pScene = pMatchedGroup->GetCurrentScene();
	    if(pScene)
        {
            int iGroupCount = pScene->m_Groups.size();
            for(int i=0; i<iGroupCount; i++)
            {
                CogeSpriteGroup* pGroup = pScene->m_Groups[i];
                if(pGroup && pGroup == pMatchedGroup)
                {
                    pScene->m_Groups[i] = NULL;
                    break;
                }
            }
        }

        int iGroupCount = this->m_Groups.size();
        for(int i=0; i<iGroupCount; i++)
        {
            CogeSpriteGroup* pGroup = this->m_Groups[i];
            if(pGroup && pGroup == pMatchedGroup)
            {
                this->m_Groups[i] = NULL;
                break;
            }
        }

		m_GroupMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedGroup;
		return 1;

	}
	return -1;
}
int CogeEngine::GetGroupCount()
{
    return m_GroupMap.size();
}
CogeSpriteGroup* CogeEngine::GetGroupByIndex(int idx)
{
    CogeSpriteGroup* pGroup = NULL;
    int iCount = m_GroupMap.size();
    ogeGroupMap::iterator it = m_GroupMap.begin();
    ogeGroupMap::iterator ite = m_GroupMap.end();
    for(int i=0; i<iCount; i++)
    {
        if(it == ite) break;
        if(idx == i)
        {
            pGroup = it->second;
            break;
        }
        it++;
    }
    return pGroup;
}

CogeGameMap* CogeEngine::NewGameMap(const std::string& sGameMapName)
{
    ogeGameMapMap::iterator it;

    it = m_GameMapMap.find(sGameMapName);
	if (it != m_GameMapMap.end())
	{
	    OGE_Log("The GameMap name '%s' is in use.\n", sGameMapName.c_str());
        return NULL;
    }

    std::string sConfigFileName = m_ObjectIniFile.ReadFilePath("Maps", sGameMapName, "");
    if (sConfigFileName.length() <= 0) return NULL;

    CogeGameMap* pNewGameMap = new CogeGameMap(sGameMapName, this);

    if(pNewGameMap->LoadConfigFile(sConfigFileName) == false)
    {
        delete pNewGameMap;
        return NULL;
    }

    bool bInit = false;

    std::string sBgName = pNewGameMap->m_IniFile.ReadString("GameMap", "Background", "");
    if(sBgName.length() > 0)
    {
        CogeImage* pBackground = GetImage(sBgName);
        if(pBackground)
        {
            pNewGameMap->SetBgImage(pBackground);

            pNewGameMap->m_iMapType = pNewGameMap->m_IniFile.ReadInteger("GameMap", "MapType", 0);
            pNewGameMap->m_iTileWidth = pNewGameMap->m_IniFile.ReadInteger("GameMap", "TileWidth", 0);
            pNewGameMap->m_iTileHeight = pNewGameMap->m_IniFile.ReadInteger("GameMap", "TileHeight", 0);
            //pNewGameMap->m_iTileDepth = pNewGameMap->m_IniFile.ReadInteger("GameMap", "TileDepth", 0);
            pNewGameMap->m_iTileRootX = pNewGameMap->m_IniFile.ReadInteger("GameMap", "TileRootX", 0);
            pNewGameMap->m_iTileRootY = pNewGameMap->m_IniFile.ReadInteger("GameMap", "TileRootY", 0);
            pNewGameMap->m_iFirstX = pNewGameMap->m_IniFile.ReadInteger("GameMap", "FirstX", 0);
            pNewGameMap->m_iFirstY = pNewGameMap->m_IniFile.ReadInteger("GameMap", "FirstY", 0);
            int iColumnCount = pNewGameMap->m_IniFile.ReadInteger("GameMap", "ColumnCount", 0);
            int iRowCount = pNewGameMap->m_IniFile.ReadInteger("GameMap", "RowCount", 0);

            pNewGameMap->SetMapSize(iColumnCount, iRowCount);

            int iTileValue = 0;

            for(int j=0; j<iRowCount; j++)
            for(int i=0; i<iColumnCount; i++)
            {
                //pNewGameMap->m_Tiles[j*iColumnCount + i] = pNewGameMap->m_IniFile.ReadInteger("Tiles", "X"+OGE_itoa(i)+"Y"+OGE_itoa(j), 0);
                iTileValue = pNewGameMap->m_IniFile.ReadInteger("Tiles", "X"+OGE_itoa(i)+"Y"+OGE_itoa(j), 0);
                pNewGameMap->m_Tiles[j*iColumnCount + i] = iTileValue;
                pNewGameMap->m_OriginalTiles[j*iColumnCount + i] = iTileValue;
                pNewGameMap->m_Data[j*iColumnCount + i] = 0;
            }


            pNewGameMap->InitFinders();

            bInit =  true;
        }

    }

    if(!bInit)
    {
        delete pNewGameMap;
        return NULL;
    }

    m_GameMapMap.insert(ogeGameMapMap::value_type(sGameMapName, pNewGameMap));

    return pNewGameMap;

}
CogeGameMap* CogeEngine::FindGameMap(const std::string& sGameMapName)
{
    ogeGameMapMap::iterator it;

    it = m_GameMapMap.find(sGameMapName);
	if (it != m_GameMapMap.end()) return it->second;
	else return NULL;
}
CogeGameMap* CogeEngine::GetGameMap(const std::string& sGameMapName)
{
    CogeGameMap* pMatchedGameMap = FindGameMap(sGameMapName);
    if (!pMatchedGameMap) pMatchedGameMap = NewGameMap(sGameMapName);
    //if (pMatchedSprite) pMatchedSprite->Hire();
    return pMatchedGameMap;
}
int CogeEngine::DelGameMap(const std::string& sGameMapName)
{
    CogeGameMap* pMatchedGameMap = FindGameMap(sGameMapName);
	if (pMatchedGameMap)
	{
	    //pMatchedPath->Fire();
	    if (pMatchedGameMap->m_iTotalUsers > 0) return 0;
		m_GameMapMap.erase(sGameMapName);
		//pMatchedPath->points.clear();
		delete pMatchedGameMap;
		return 1;
	}
	return -1;
}

int CogeEngine::DelGameMap(CogeGameMap* pGameMap)
{
    if(!pGameMap) return 0;

    CogeGameMap* pMatchedGameMap = NULL;
    ogeGameMapMap::iterator it = m_GameMapMap.begin();

    while(it != m_GameMapMap.end())
    {
        pMatchedGameMap = it->second;

        if(pMatchedGameMap == pGameMap)
        {
            if (pMatchedGameMap->m_iTotalUsers > 0) return 0;
            else
            {
                m_GameMapMap.erase(it);
                delete pMatchedGameMap;
                return 1;
            }
        }

        it++;
    }

    return 0;
}

CogeIniFile* CogeEngine::NewIniFile(const std::string& sFilePath)
{
    /*
    ogeIniFileMap::iterator it;

    it = m_IniFileMap.find(sIniFileName);
	if (it != m_IniFileMap.end())
	{
	    OGE_Log("The IniFile name '%s' is in use.\n", sIniFileName.c_str());
        return NULL;
    }
    */

    CogeIniFile* pNewIniFile = new CogeIniFile();
    if(!pNewIniFile->Load(sFilePath))
    {
        OGE_Log("Fail to load ini file: %s \n", sFilePath.c_str());
        delete pNewIniFile;
        return NULL;
    }

    //m_IniFileMap.insert(ogeIniFileMap::value_type(sIniFileName, pNewIniFile));

    m_IniFileMap.insert(ogeIniFileMap::value_type((int)pNewIniFile, pNewIniFile));

    return pNewIniFile;

}
CogeIniFile* CogeEngine::FindIniFile(int iIniFileId)
{
    ogeIniFileMap::iterator it;

    it = m_IniFileMap.find(iIniFileId);
	if (it != m_IniFileMap.end()) return it->second;
	else return NULL;
}
/*
CogeIniFile* CogeEngine::GetIniFile(const std::string& sIniFileName, const std::string& sFilePath)
{
    CogeIniFile* pMatchedIniFile = FindIniFile(sIniFileName);
    if (!pMatchedIniFile) pMatchedIniFile = NewIniFile(sIniFileName, sFilePath);
    return pMatchedIniFile;
}
*/
int CogeEngine::DelIniFile(int iIniFileId)
{
    CogeIniFile* pMatchedIniFile = FindIniFile(iIniFileId);
	if (pMatchedIniFile)
	{
		m_IniFileMap.erase(iIniFileId);
		delete pMatchedIniFile;
		return 1;
	}
	return 0;
}

CogeStreamBuffer* CogeEngine::NewStreamBuf(char* pBuf, int iBufSize, int iReadWritePos)
{
    if(pBuf == NULL || iBufSize <= 0) return NULL;

    CogeStreamBuffer* sbuf = new CogeStreamBuffer(pBuf, iBufSize);
    m_StreamBufMap.insert(ogeStreamBufMap::value_type((int)sbuf, sbuf));

    return sbuf;
}
CogeStreamBuffer* CogeEngine::FindStreamBuf(int iStreamBufId)
{
    ogeStreamBufMap::iterator it;

    it = m_StreamBufMap.find(iStreamBufId);
	if (it != m_StreamBufMap.end()) return it->second;
	else return NULL;
}
int CogeEngine::DelStreamBuf(int iStreamBufId)
{
    CogeStreamBuffer* pMatched = FindStreamBuf(iStreamBufId);
	if (pMatched)
	{
		m_StreamBufMap.erase(iStreamBufId);
		delete pMatched;
		return 1;
	}
	return 0;
}

void CogeEngine::DelAllMaps()
{
    ogeGameMapMap::iterator it;
	CogeGameMap* pMatchedGameMap = NULL;

	it = m_GameMapMap.begin();

	while (it != m_GameMapMap.end())
	{
		pMatchedGameMap = it->second;
		m_GameMapMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedGameMap;
		it = m_GameMapMap.begin();
	}
}

void CogeEngine::DelAllGameData()
{
    ogeGameDataMap::iterator it;
	CogeGameData* pMatchedGameData = NULL;

	it = m_GameDataMap.begin();

	while (it != m_GameDataMap.end())
	{
		pMatchedGameData = it->second;
		m_GameDataMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedGameData;
		it = m_GameDataMap.begin();
	}

	ogeGameDataList::iterator itx;
	pMatchedGameData = NULL;

	itx = m_GameDataList.begin();

	while (itx != m_GameDataList.end())
	{
		pMatchedGameData = *itx;
		m_GameDataList.erase(itx);
		//pMatchedPath->points.clear();
		delete pMatchedGameData;
		itx = m_GameDataList.begin();
	}
}

void CogeEngine::DelAllGroups()
{
    ogeGroupMap::iterator it;
	CogeSpriteGroup* pMatchedGroup = NULL;

	it = m_GroupMap.begin();

	while (it != m_GroupMap.end())
	{
		pMatchedGroup = it->second;
		m_GroupMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedGroup;
		it = m_GroupMap.begin();
	}
}

void CogeEngine::DelAllPaths()
{
    ogePathMap::iterator it;
	CogePath* pMatchedPath = NULL;

	it = m_PathMap.begin();

	while (it != m_PathMap.end())
	{
		pMatchedPath = it->second;
		m_PathMap.erase(it);
		//pMatchedPath->points.clear();
		delete pMatchedPath;
		it = m_PathMap.begin();
	}
}

void CogeEngine::DelAllIniFiles()
{
    ogeIniFileMap::iterator it;
	CogeIniFile* pMatchedIniFile = NULL;

	it = m_IniFileMap.begin();

	while (it != m_IniFileMap.end())
	{
		pMatchedIniFile = it->second;
		m_IniFileMap.erase(it);
		delete pMatchedIniFile;
		it = m_IniFileMap.begin();
	}
}

void CogeEngine::DelAllStreamBufs()
{
    ogeStreamBufMap::iterator it;
	CogeStreamBuffer* pMatched = NULL;

	it = m_StreamBufMap.begin();

	while (it != m_StreamBufMap.end())
	{
		pMatched = it->second;
		m_StreamBufMap.erase(it);
		delete pMatched;
		it = m_StreamBufMap.begin();
	}
}

/*
void CogeEngine::DelAllThinkers()
{
    ogeThinkerMap::iterator it;
	CogeThinker* pMatchedThinker = NULL;

	it = m_ThinkerMap.begin();

	while (it != m_ThinkerMap.end())
	{
		pMatchedThinker = it->second;
		m_ThinkerMap.erase(it);
		delete pMatchedThinker;
		it = m_ThinkerMap.begin();
	}
}
*/

CogeImage* CogeEngine::NewImage(const std::string& sImageName)
{
    if(m_pVideo)
    {
        CogeImage* pNewImage = NULL;

        int iWidth  = m_ImageIniFile.ReadInteger(sImageName, "Width", -1);
        int iHeight = m_ImageIniFile.ReadInteger(sImageName, "Height", -1);

        if(iWidth < 0 || iHeight < 0) return NULL;

        std::string sColorKey = m_ImageIniFile.ReadString(sImageName, "ColorKey", "-1");
        int iColorKeyRGB = m_pVideo->StringToColor(sColorKey);

        bool bLoadAlphaChannel = m_ImageIniFile.ReadInteger(sImageName, "AlphaChannel", 0) != 0;
        bool bCreateLocalClipboard = m_ImageIniFile.ReadInteger(sImageName, "LocalClipboard", 0) != 0;

        std::string sFileName  = m_ImageIniFile.ReadFilePath(sImageName, "File", "");

        if(!m_bPackedImage)
        {
            if(sFileName.length() == 0 || !OGE_IsFileExisted(sFileName.c_str())) sFileName = ""; // used to handle img with no file
            pNewImage = m_pVideo->NewImage(sImageName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, bCreateLocalClipboard, sFileName);
        }
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0)
            {
                if(sFileName.length() == 0 || !OGE_IsFileExisted(sFileName.c_str())) sFileName = ""; // used to handle img with no file
                pNewImage = m_pVideo->NewImage(sImageName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, bCreateLocalClipboard, sFileName);
            }
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedImage);
                        if(iReadSize > 0)
                        {
                            pNewImage = m_pVideo->NewImageFromBuffer(sImageName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, bCreateLocalClipboard, pFileData, iReadSize);
                        }

                        buf->Fire();
                    }
                }
            }

        }


        if(pNewImage)
        {
            /*
            std::string sBgColor = m_ImageIniFile.ReadString(sImageName, "BgColor", "-1");
            int iBgColor = m_pVideo->StringToColor(sBgColor);
            if(iBgColor != -1) pNewImage->FillRect(iBgColor);

            CogeFont* pFont = NULL;
            std::string sFontName = m_ImageIniFile.ReadString(sImageName, "DrawFontName", "");
            int iFontSize = m_ImageIniFile.ReadInteger(sImageName, "DrawFontSize", _OGE_DOT_FONT_SIZE_);
            if(sFontName.length() > 0) pFont = GetFont(sFontName, iFontSize);
            if(pFont != NULL) pNewImage->SetDefaultFont(pFont);
            std::string sPenColor = m_ImageIniFile.ReadString(sImageName, "DrawPen", "0");
            int iPenColor = m_pVideo->StringToColor(sPenColor);
            pNewImage->SetPenColor(iPenColor);

            std::string sText = m_ImageIniFile.ReadString(sImageName, "DrawText", "");
            if(sText.length() > 0)
            {
                int iPosX = m_ImageIniFile.ReadInteger(sImageName, "DrawPosX", 0);
                int iPosY = m_ImageIniFile.ReadInteger(sImageName, "DrawPosY", 0);
                int iUseAlign = m_ImageIniFile.ReadInteger(sImageName, "DrawAlign", 0);
                if(iUseAlign > 0)
                {
                    pNewImage->PrintTextAlign(sText, iPosX, iPosY, pFont);
                }
                else
                {
                    pNewImage->PrintText(sText, iPosX, iPosY, pFont);
                }
            }
            */

            return pNewImage;
        }
        else return NULL;
    }
    else return NULL;
}

CogeImage* CogeEngine::GetImage(const std::string& sImageName)
{
    if(m_pVideo)
    {
        CogeImage* pMatchedImage = FindImage(sImageName);
        if (!pMatchedImage) pMatchedImage = NewImage(sImageName);
        return pMatchedImage;
    }
    else return NULL;

}

CogeImage* CogeEngine::FindImage(const std::string& sImageName)
{
    if(m_pVideo) return m_pVideo->FindImage(sImageName);
    else return NULL;
}
bool CogeEngine::DelImage(const std::string& sImageName)
{
    if(m_pVideo) return m_pVideo->DelImage(sImageName);
    else return false;
}

CogeMovie* CogeEngine::NewMovie(const std::string& sMovieName)
{
    if(m_pPlayer)
    {
        std::string sFileName  = m_MediaIniFile.ReadFilePath(sMovieName, "File", "");
        int iType  = m_MediaIniFile.ReadInteger(sMovieName, "Type", 2);
        if(sFileName.length() <= 0 || iType != 2) return NULL;

        CogeMovie* result = NULL;

        if(!m_bPackedMedia) result = m_pPlayer->NewMovie(sMovieName, sFileName);
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0) result = m_pPlayer->NewMovie(sMovieName, sFileName);
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedMedia);
                        if(iReadSize > 0)
                        {
                            result = m_pPlayer->NewMovieFromBuffer(sMovieName, pFileData, iReadSize);
                        }

                        buf->Fire();
                    }
                }
            }
        }

        return result;

    }
    else return NULL;
}
CogeMovie* CogeEngine::GetMovie(const std::string& sMovieName)
{
    if(m_pPlayer)
    {
        CogeMovie* pMatchedMovie = FindMovie(sMovieName);
        if (!pMatchedMovie) pMatchedMovie = NewMovie(sMovieName);
        return pMatchedMovie;
    }
    else return NULL;
}
CogeMovie* CogeEngine::FindMovie(const std::string& sMovieName)
{
    if(m_pPlayer) return m_pPlayer->FindMovie(sMovieName);
    else return NULL;
}
int CogeEngine::DelMovie(const std::string& sMovieName)
{
    if(m_pPlayer) return m_pPlayer->DelMovie(sMovieName);
    else return-1;
}

CogeSound* CogeEngine::NewSound(const std::string& sSoundName)
{
    if(m_pAudio)
    {
        std::string sFileName  = m_MediaIniFile.ReadFilePath(sSoundName, "File", "");
        int iType  = m_MediaIniFile.ReadInteger(sSoundName, "Type", 0);
        if(sFileName.length() <= 0 || iType != 0) return NULL;

        //return m_pAudio->NewSound(sSoundName, sFileName);

        CogeSound* result = NULL;

        if(!m_bPackedMedia) result = m_pAudio->NewSound(sSoundName, sFileName);
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0) result = m_pAudio->NewSound(sSoundName, sFileName);
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedMedia);
                        if(iReadSize > 0)
                        {
                            result = m_pAudio->NewSoundFromBuffer(sSoundName, pFileData, iReadSize);
                        }

                        buf->Fire();
                    }
                }
            }
        }

        return result;


    }
    else return NULL;
}

CogeSound* CogeEngine::FindSound(const std::string& sSoundName)
{
    if(m_pAudio) return m_pAudio->FindSound(sSoundName);
    else return NULL;
}

CogeSound* CogeEngine::GetSound(const std::string& sSoundName)
{
    if(m_pAudio)
    {
        CogeSound* pMatchedSound = FindSound(sSoundName);
        if (!pMatchedSound) pMatchedSound = NewSound(sSoundName);
        return pMatchedSound;
    }
    else return NULL;

}

CogeMusic* CogeEngine::NewMusic(const std::string& sMusicName)
{
    if(m_pAudio)
    {
        //std::string sFileName  = m_MediaIniFile.ReadString(sMusicName, "File", "");
        std::string sFileName  = m_MediaIniFile.ReadFilePath(sMusicName, "File", "");
        int iType  = m_MediaIniFile.ReadInteger(sMusicName, "Type", 1);
        if(sFileName.length() <= 0 || iType != 1) return NULL;

        //return m_pAudio->GetMusic(sMusicName, sFileName);

        CogeMusic* result = NULL;

        if(!m_bPackedMedia) result = m_pAudio->NewMusic(sMusicName, sFileName);
        else
        {
            int iPos = GetValidPackedFilePathLength(sFileName);
            if(iPos == 0) result = m_pAudio->NewMusic(sMusicName, sFileName);
            else
            {
                std::string sPackedFilePath = sFileName.substr(0, iPos);
                std::string sPackedBlockPath = sFileName.substr(iPos+1);

                int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                if(iSize > 0)
                {
                    CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
                    if(buf != NULL && buf->GetState() >= 0)
                    {
                        buf->Hire();

                        char* pFileData = buf->GetBuffer();

                        int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedMedia);
                        if(iReadSize > 0)
                        {
                            result = m_pAudio->NewMusicFromBuffer(sMusicName, pFileData, iReadSize);
                        }

                        if(!result) buf->Fire();
                        else result->SetDataBuffer((void*)buf);
                    }
                }
            }
        }

        return result;

    }
    else return NULL;
}
CogeMusic* CogeEngine::FindMusic(const std::string& sMusicName)
{
    if(m_pAudio) return m_pAudio->FindMusic(sMusicName);
    else return NULL;
}
CogeMusic* CogeEngine::GetMusic(const std::string& sMusicName)
{
    if(m_pAudio)
    {
        CogeMusic* pMatchedMusic = FindMusic(sMusicName);
        if (!pMatchedMusic) pMatchedMusic = NewMusic(sMusicName);
        return pMatchedMusic;
    }
    else return NULL;
}

int CogeEngine::DelMusic(const std::string& sMusicName)
{
    int rsl = -1;
    if(m_pAudio)
    {
        CogeMusic* pMusic = m_pAudio->FindMusic(sMusicName);
        if(pMusic)
        {
            CogeScene* pScene = NULL;
            ogeSceneMap::iterator it;
            for(it = m_SceneMap.begin(); it != m_SceneMap.end(); it++)
            {
                pScene = it->second;
                if(pScene->m_pBackgroundMusic == pMusic)
                {
                    pScene->m_pBackgroundMusic = NULL;
                }
            }

            CogeBuffer* buf = (CogeBuffer*) pMusic->GetDataBuffer();
            if(buf) buf->Fire();
            rsl = m_pAudio->DelMusic(sMusicName);

        }
    }
    return rsl;
}

int CogeEngine::DelSound(const std::string& sSoundName)
{
    if(m_pAudio)
    {
        CogeSound* pSound = FindSound(sSoundName);

        if(pSound)
        {
            int iSoundCount = m_Sounds.size();
            for(int i = 0; i < iSoundCount; i++)
            {
                if(m_Sounds[i] == pSound)
                {
                    m_Sounds[i] = NULL;
                    break;
                }
            }
        }

        return m_pAudio->DelSound(sSoundName);
    }

    else return-1;
}

/*
int CogeEngine::UseMusic(const std::string& sMusicName)
{
    if(m_pAudio)
    {
        int iRsl = m_pAudio->PrepareMusic(sMusicName);
        if(iRsl == -1)
        {
            GetMusic(sMusicName);
            return m_pAudio->PrepareMusic(sMusicName);
        }
        else return iRsl;
    }
    else return -1;
}
*/

int CogeEngine::PlayMusic(int iLoopTimes)
{
    if(m_pAudio)
    {
        if(m_pAudio->GetCurrentMusic()) return m_pAudio->PlayMusic();
        else return PlayBgMusic(iLoopTimes);
    }
    else return -1;
}

int CogeEngine::PlayBgMusic(int iLoopTimes)
{
    if(m_pActiveScene) return m_pActiveScene->PlayBgMusic(iLoopTimes);
    else return -1;
}

int CogeEngine::StopMusic()
{
    if(m_pAudio)
    {
        CogeMusic* pCurrentMusic = m_pAudio->GetCurrentMusic();
        if(pCurrentMusic) return pCurrentMusic->Stop();
        else return -1;
    }
    else return -1;
}

int CogeEngine::PauseMusic()
{
    if(m_pAudio)
    {
        CogeMusic* pCurrentMusic = m_pAudio->GetCurrentMusic();
        if(pCurrentMusic) return pCurrentMusic->Pause();
        else return -1;
    }
    else return -1;

}

int CogeEngine::ResumeMusic()
{
    if(m_pAudio)
    {
        CogeMusic* pCurrentMusic = m_pAudio->GetCurrentMusic();
        if(pCurrentMusic) return pCurrentMusic->Resume();
        else return -1;
    }
    else return -1;

}
int CogeEngine::StopAudio()
{
    if(m_pAudio)
    {
        m_pAudio->StopAllMusic();
        m_pAudio->StopAllSound();
        return 0;
    }
    else return -1;
}
int CogeEngine::PlaySound(const std::string& sSoundName, int iLoopTimes)
{
    if(m_pAudio)
    {
        CogeSound* pSound = GetSound(sSoundName);
        if(pSound) return pSound->Play(iLoopTimes);
        else return -1;
    }
    else return -1;

}

/*
int CogeEngine::PlaySound(int iSoundCode)
{
    if(m_pActiveScene) return m_pActiveScene->PlaySound(iSoundCode);
    else return -1;
}

int CogeEngine::BindSoundCode(int iSoundCode, CogeSound* pTheSound)
{
    if(pTheSound == NULL) return -1;

    ogeSoundCodeMap::iterator it;
	CogeSound* pMatchedSound = NULL;
    it = m_SoundCodeMap.find(iSoundCode);
    if(it != m_SoundCodeMap.end())
	{
		pMatchedSound = it->second;
		if(pTheSound == pMatchedSound) return 0;
		m_SoundCodeMap.erase(it);
		//m_pEngine->DelSound(pMatchedSound->GetName());
	}

    m_SoundCodeMap.insert(ogeSoundCodeMap::value_type(iSoundCode, pTheSound));

    return 1;

}

CogeSound* CogeEngine::FindSoundByCode(int iSoundCode)
{
    ogeSoundCodeMap::iterator it = m_SoundCodeMap.find(iSoundCode);

    if(it != m_SoundCodeMap.end()) return it->second;
    else return NULL;
}
*/

int CogeEngine::PlaySound(int iSoundCode, int iLoopTimes)
{
    //CogeSound* pSound = FindSoundByCode(iSoundCode);
    //if(pSound) return pSound->Play();
    //else return -1;

    int iTotal = m_Sounds.size();
    if(iSoundCode < 0 || iSoundCode >= iTotal) return -1;
    CogeSound* pSound = m_Sounds[iSoundCode];
    if(pSound == NULL)
    {
        std::string sSoundName  = m_ObjectIniFile.ReadString("Sounds", "Sound"+OGE_itoa(iSoundCode), "");
        if(sSoundName.length() > 0)
        {
            pSound = GetSound(sSoundName);
            m_Sounds[iSoundCode] = pSound;
        }
    }

    if(pSound) return pSound->Play(iLoopTimes);
    else return -1;
}

int CogeEngine::StopSound(const std::string& sSoundName)
{
    if(m_pAudio)
    {
        CogeSound* pSound = GetSound(sSoundName);
        if(pSound) return pSound->Stop();
        else return -1;
    }
    else return -1;
}
int CogeEngine::StopSound(int iSoundCode)
{
    int iTotal = m_Sounds.size();
    if(iSoundCode < 0 || iSoundCode >= iTotal) return -1;
    CogeSound* pSound = m_Sounds[iSoundCode];
    if(pSound == NULL)
    {
        std::string sSoundName  = m_ObjectIniFile.ReadString("Sounds", "Sound"+OGE_itoa(iSoundCode), "");
        if(sSoundName.length() > 0)
        {
            pSound = GetSound(sSoundName);
            m_Sounds[iSoundCode] = pSound;
        }
    }

    if(pSound) return pSound->Stop();
    else return -1;
}

int CogeEngine::PlayMusic(const std::string& sMusicName, int iLoopTimes)
{
    /*
    if(UseMusic(sMusicName) >= 0) return PlayMusic();
    else return -1;
    */

    //if(m_pActiveScene) return m_pActiveScene->PlayMusic(sMusicName, iLoopTimes);
    //else return -1;

    if(m_pAudio == NULL) return -1;

    CogeMusic* pMatchedMusic = GetMusic(sMusicName);
    if(pMatchedMusic)
    {
        CogeMusic* pCurrentMusic = m_pAudio->GetCurrentMusic();
        if(pMatchedMusic == pCurrentMusic)
        {
            return pMatchedMusic->Play(iLoopTimes);
        }
        else
        {
            m_pAudio->PrepareMusic(pMatchedMusic);
            return pMatchedMusic->Play(iLoopTimes);
        }
    }
    else return -1;


}

int CogeEngine::FadeInMusic(const std::string& sMusicName, int iLoopTimes, int iFadingTime)
{
    //if(m_pActiveScene) return m_pActiveScene->FadeInMusic(sMusicName, iLoopTimes, iFadingTime);
    //else return -1;

    if(m_pAudio) return m_pAudio->FadeInMusic(sMusicName, iLoopTimes, iFadingTime);
    else return -1;
}

int CogeEngine::FadeOutMusic(int iFadingTime)
{
    //if(m_pActiveScene) return m_pActiveScene->FadeOutMusic(iFadingTime);
    //else return -1;

    if(m_pAudio) return m_pAudio->FadeOutMusic(iFadingTime);
    else return -1;

}

int CogeEngine::SetBgMusic(const std::string& sMusicName)
{
    if(m_pActiveScene) return m_pActiveScene->SetBgMusic(sMusicName);
    else return -1;
}

int CogeEngine::PlayMovie(const std::string& sMovieName, int iLoopTimes)
{
    if(m_pPlayer == NULL) return -1;

    CogeMovie* pMatchedMovie = GetMovie(sMovieName);
    if(pMatchedMovie)
    {
        CogeMovie* pCurrentMovie = m_pPlayer->GetCurrentMovie();
        if(pMatchedMovie == pCurrentMovie)
        {
            return pMatchedMovie->Play(iLoopTimes);
        }
        else
        {
            m_pPlayer->PrepareMovie(pMatchedMovie);
            return pMatchedMovie->Play(iLoopTimes);
        }
    }
    else return -1;

}
int CogeEngine::StopMovie()
{
    if(m_pPlayer) m_pPlayer->StopMovie();
    else return -1;
    return 0;
}

bool CogeEngine::IsPlayingMovie()
{
    if(m_pPlayer) return m_pPlayer->IsPlaying();
    else return false;
}

int CogeEngine::GetSoundVolume()
{
    if(m_pAudio) return m_pAudio->GetSoundVolume();
    else return 0;
}
int CogeEngine::GetMusicVolume()
{
    if(m_pAudio) return m_pAudio->GetMusicVolume();
    else return 0;
}
void CogeEngine::SetSoundVolume(int iValue)
{
    if(m_pAudio) m_pAudio->SetSoundVolume(iValue);

    /*
    if(m_AppIniFile.GetState() >= 0)
    {
        m_AppIniFile.WriteInteger("Audio", "SoundVolume", iValue);
        m_AppIniFile.Save();
    }
    */

}
void CogeEngine::SetMusicVolume(int iValue)
{
    if(m_pAudio) m_pAudio->SetMusicVolume(iValue);

    /*
    if(m_AppIniFile.GetState() >= 0)
    {
        m_AppIniFile.WriteInteger("Audio", "MusicVolume", iValue);
        m_AppIniFile.Save();
    }
    */
}

bool CogeEngine::GetFullScreen()
{
    if(m_pVideo) return m_pVideo->GetFullScreen();
    else return false;
}
void CogeEngine::SetFullScreen(bool bValue)
{
    if(m_pVideo) m_pVideo->SetFullScreen(bValue);

    /*
    if(m_AppIniFile.GetState() >= 0)
    {
        if(bValue) m_AppIniFile.WriteInteger("Screen", "FullScreen", 1);
        else m_AppIniFile.WriteInteger("Screen", "FullScreen", 0);
        m_AppIniFile.Save();
    }
    */

}

/*
bool CogeEngine::InstallFont(const std::string& sFontName, int iFontSize, const std::string& sFontFileName)
{
    if(m_pVideo) return m_pVideo->InstallFont(sFontName, iFontSize, sFontFileName);
    else return false;
}
void CogeEngine::UninstallFont(const std::string& sFontName)
{
    if(m_pVideo) m_pVideo->UninstallFont(sFontName);
}

bool CogeEngine::UseFont(const std::string& sFontName)
{
    if(m_pVideo) return m_pVideo->UseFont(sFontName);
    else return false;
}
*/

CogeFont* CogeEngine::NewFont(const std::string& sFontName, int iFontSize)
{
    if(m_pVideo)
    {
        CogeFont* result = NULL;

        std::string sFontFileName  = m_FontIniFile.ReadFilePath(sFontName, "File", "");
        if(sFontFileName.length() > 0)
        {
            if(!m_bPackedFont) result = m_pVideo->NewFont(sFontName, iFontSize, sFontFileName);
            else
            {
                int iPos = GetValidPackedFilePathLength(sFontFileName);
                if(iPos == 0) result = m_pVideo->NewFont(sFontName, iFontSize, sFontFileName);
                else
                {
                    std::string sPackedFilePath = sFontFileName.substr(0, iPos);
                    std::string sPackedBlockPath = sFontFileName.substr(iPos+1);

                    int iSize = m_pPacker->GetFileSize(sPackedFilePath, sPackedBlockPath);

                    if(iSize > 0)
                    {
                        //char* buf = (char*)malloc(iSize);

                        CogeBuffer* buf = m_pBuffer->GetFreeBuffer(iSize);
                        if(buf != NULL && buf->GetState() >= 0)
                        //if(buf)
                        {
                            buf->Hire();
                            char* pFileData = buf->GetBuffer();
                            //char* pFileData = buf;

                            int iReadSize = m_pPacker->ReadFile(sPackedFilePath, sPackedBlockPath, pFileData, m_bEncryptedFont);
                            if(iReadSize > 0)
                            {
                                result = m_pVideo->NewFontFromBuffer(sFontName, iFontSize, pFileData, iReadSize);
                            }

                            if(!result) buf->Fire();
                            else result->SetDataBuffer((void*)buf);

                            //free(buf);
                        }
                    }
                }
            }

        }

        return result;
    }
    else return NULL;
}
CogeFont* CogeEngine::FindFont(const std::string& sFontName, int iFontSize)
{
    if(m_pVideo) return m_pVideo->FindFont(sFontName, iFontSize);
    else return NULL;
}
CogeFont* CogeEngine::GetFont(const std::string& sFontName, int iFontSize)
{
    if(m_pVideo)
    {
        CogeFont* pFont = FindFont(sFontName, iFontSize);
        if(pFont) return pFont;
        else return NewFont(sFontName, iFontSize);
    }
    else return NULL;
}
int CogeEngine::DelFont(const std::string& sFontName, int iFontSize)
{
    int rsl = -1;
    if(m_pVideo)
    {
        CogeFont* pFont = m_pVideo->FindFont(sFontName, iFontSize);
        if(pFont)
        {
            CogeBuffer* buf = (CogeBuffer*) pFont->GetDataBuffer();
            if(buf) buf->Fire();
            rsl = m_pVideo->DelFont(sFontName, iFontSize);
        }
    }
    return rsl;
}

CogeFont* CogeEngine::GetDefaultFont()
{
    if(m_pVideo) return m_pVideo->GetDefaultFont();
    else return NULL;
}
void CogeEngine::SetDefaultFont(CogeFont* pFont)
{
    if(m_pVideo) m_pVideo->SetDefaultFont(pFont);
}

const std::string& CogeEngine::GetDefaultCharset()
{
    if(m_pVideo) return m_pVideo->GetDefaultCharset();
    else return _OGE_EMPTY_STR_;
}
void CogeEngine::SetDefaultCharset(const std::string& sDefaultCharset)
{
    if(m_pVideo) m_pVideo->SetDefaultCharset(sDefaultCharset);
}

const std::string& CogeEngine::GetSystemCharset()
{
    if(m_pVideo) return m_pVideo->GetSystemCharset();
    else return _OGE_EMPTY_STR_;
}
void CogeEngine::SetSystemCharset(const std::string& sSystemCharset)
{
    if(m_pVideo) m_pVideo->SetSystemCharset(sSystemCharset);
}

int CogeEngine::StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer)
{
    if(m_pVideo) return m_pVideo->StringToUnicode(sText, sCharsetName, pUnicodeBuffer);
    else return 0;
}
std::string CogeEngine::UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName)
{
    if(m_pVideo) return m_pVideo->UnicodeToString(pUnicodeBuffer, iBufferSize, sCharsetName);
    else return "";
}

CogeTextInputter* CogeEngine::NewTextInputter(const std::string& sInputterName)
{
    if(m_pIM) return m_pIM->NewInputter(sInputterName);
    else return NULL;
}
CogeTextInputter* CogeEngine::FindTextInputter(const std::string& sInputterName)
{
    if(m_pIM) return m_pIM->FindInputter(sInputterName);
    else return NULL;
}
CogeTextInputter* CogeEngine::GetTextInputter(const std::string& sInputterName)
{
    if(m_pIM) return m_pIM->GetInputter(sInputterName);
    else return NULL;
}

void CogeEngine::DisableIM()
{
    if(m_pIM) m_pIM->DisableIM();
}

void CogeEngine::CloseIM()
{
    if(m_pIM)
    {
        if(m_pIM->IsSoftKeyboardShown()) m_pIM->ShowSoftKeyboard(false);
        m_pIM->DisableIM();
    }
}

void CogeEngine::SetKeyDown(int iKey, bool bDown)
{
    if(!m_bEnableInput) return;

    bool bHasSet = false;

#ifdef __OGE_WITH_SDL2__

    Uint32 iKeyCode = iKey;
    if(iKeyCode > 0x1000)
    {
        iKeyCode = iKeyCode & 0x00000FFF;
        if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_S_KEY_NUMBER_) return;
        else
        {
            if(m_SKeyState[iKeyCode] != bDown)
            {
                m_SKeyState[iKeyCode] = bDown;
                bHasSet = true;
            }

        }

    }
    else
    {
        if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_N_KEY_NUMBER_) return;
        else
        {
            if(m_NKeyState[iKeyCode] != bDown)
            {
                m_NKeyState[iKeyCode] = bDown;
                bHasSet = true;
            }

        }
    }

#else

    if(iKey < 0 || iKey >= _OGE_MAX_KEY_NUMBER_) return;
    else
    {
        if(m_KeyState[iKey] != bDown)
        {
            m_KeyState[iKey] = bDown;
            bHasSet = true;
        }
    }

#endif

    if(bHasSet)
    {
        SDL_Event e = {0};

#if SDL_VERSION_ATLEAST(2,0,0)
        e.key.keysym.sym = iKey;
#else
        e.key.keysym.sym = (SDLKey)iKey;
#endif
        e.key.keysym.unicode = 0;

        if(bDown)
        {
            e.key.state = SDL_PRESSED;
            e.type = SDL_KEYDOWN;
        }
        else
        {
            e.key.state = SDL_RELEASED;
            e.type = SDL_KEYUP;
        }

        SDL_PushEvent(&e);
    }
}

void CogeEngine::SetKeyUp(int iKey)
{
    SetKeyDown(iKey, false);
}

bool CogeEngine::IsKeyDown(int iKey)
{
    if(m_bEnableInput)
    {
#ifdef __OGE_WITH_SDL2__
        Uint32 iKeyCode = iKey;
        if(iKeyCode > 0x1000)
        {
            iKeyCode = iKeyCode & 0x00000FFF;
            if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_S_KEY_NUMBER_) return false;
            else
            {
                //Uint8 *keystate = SDL_GetKeyState(NULL);
                //return keystate[iKey];
                return m_SKeyState[iKeyCode];
            }

        }
        else
        {
            if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_N_KEY_NUMBER_) return false;
            else
            {
                //Uint8 *keystate = SDL_GetKeyState(NULL);
                //return keystate[iKey];
                return m_NKeyState[iKeyCode];
            }
        }
#else
        if(iKey < 0 || iKey >= _OGE_MAX_KEY_NUMBER_) return false;
        else
        {
            //Uint8 *keystate = SDL_GetKeyState(NULL);
            //return keystate[iKey];
            return m_KeyState[iKey];
        }
#endif
    }
    else return false;
}

bool CogeEngine::IsKeyDown(int iKey, int iInterval)
{
    if(m_bEnableInput)
    {

#ifdef __OGE_WITH_SDL2__

        Uint32 iKeyCode = iKey;
        if(iKeyCode > 0x1000)
        {
            iKeyCode = iKeyCode & 0x00000FFF;
            if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_S_KEY_NUMBER_) return false;
            else
            {
                int iTicks = SDL_GetTicks();
                if(iTicks - m_SKeyTime[iKeyCode] >= iInterval)
                {
                    if(m_SKeyState[iKeyCode])
                    {
                        m_SKeyTime[iKeyCode] = iTicks;
                        return true;
                    }
                    else return false;
                }
                else return false;
            }

        }
        else
        {
            if(iKeyCode < 0 || iKeyCode >= _OGE_MAX_N_KEY_NUMBER_) return false;
            else
            {
                int iTicks = SDL_GetTicks();
                if(iTicks - m_NKeyTime[iKeyCode] >= iInterval)
                {
                    if(m_NKeyState[iKeyCode])
                    {
                        m_NKeyTime[iKeyCode] = iTicks;
                        return true;
                    }
                    else return false;
                }
                else return false;
            }

        }

#else
        if(iKey < 0 || iKey >= _OGE_MAX_KEY_NUMBER_) return false;
        else
        {
            int iTicks = SDL_GetTicks();
            if(iTicks - m_KeyTime[iKey] >= iInterval)
            {
                if(m_KeyState[iKey])
                {
                    m_KeyTime[iKey] = iTicks;
                    return true;
                }
                else return false;
            }
            else return false;
        }
#endif
    }
    else return false;
}

bool CogeEngine::IsMouseLeftButtonDown()
{
    return m_iMouseLeft > 0;
}
bool CogeEngine::IsMouseRightButtonDown()
{
    return m_iMouseRight > 0;
}

bool CogeEngine::IsMouseLeftButtonUp()
{
    return m_iMouseLeft == 0 && m_iMouseOldLeft > 0;
}
bool CogeEngine::IsMouseRightButtonUp()
{
    return m_iMouseRight == 0 && m_iMouseOldRight > 0;
}

int CogeEngine::GetMouseX()
{
    return m_iMouseX;
}
int CogeEngine::GetMouseY()
{
    return m_iMouseY;
}

void CogeEngine::HideSystemMouseCursor()
{
    SDL_ShowCursor(SDL_DISABLE);
}
void CogeEngine::ShowSystemMouseCursor()
{
    SDL_ShowCursor(SDL_ENABLE);
}

int CogeEngine::OpenJoysticks()
{

#ifdef __OGE_WITH_JOY__

    if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) return -1;
    m_iJoystickCount = SDL_NumJoysticks();
    if(m_iJoystickCount > _OGE_MAX_JOY_COUNT_) m_iJoystickCount = _OGE_MAX_JOY_COUNT_;
    if(m_iJoystickCount > 0) SDL_JoystickEventState(SDL_ENABLE);
    for(int i = 0; i < m_iJoystickCount; i++)
    {
        m_Joysticks[i].id = i;
        m_Joysticks[i].joy = SDL_JoystickOpen(i);
        memset((void*)&(m_Joysticks[i].m_KeyState[0]), 0, sizeof(bool)* _OGE_MAX_JOY_KEY_NUMBER_ );
        memset((void*)&(m_Joysticks[i].m_KeyTime[0]),  0, sizeof(int) * _OGE_MAX_JOY_KEY_NUMBER_ );
    }
    return m_iJoystickCount;

#else

    OGE_Log("Joystick function is unavailable!\n");
    SDL_JoystickEventState(SDL_DISABLE);
    return -1;

#endif

}

int CogeEngine::GetJoystickCount()
{
    return m_iJoystickCount;
}

bool CogeEngine::IsJoyKeyDown(int iJoystickId, int iKey)
{
    if(!m_bEnableInput) return false;
    if(iJoystickId < 0 || iJoystickId >= m_iJoystickCount) return false;
    if(iKey < 0 || iKey >= _OGE_MAX_JOY_KEY_NUMBER_) return false;
    return m_Joysticks[iJoystickId].m_KeyState[iKey];

}
bool CogeEngine::IsJoyKeyDown(int iJoystickId, int iKey, int iInterval)
{
    if(!m_bEnableInput) return false;
    if(iJoystickId < 0 || iJoystickId >= m_iJoystickCount) return false;
    if(iKey < 0 || iKey >= _OGE_MAX_JOY_KEY_NUMBER_) return false;

    int iTicks = SDL_GetTicks();
    if(iTicks - m_Joysticks[iJoystickId].m_KeyTime[iKey] >= iInterval)
    {
        m_Joysticks[iJoystickId].m_KeyTime[iKey] = iTicks;
        return m_Joysticks[iJoystickId].m_KeyState[iKey];
    }
    else return false;
}
void CogeEngine::CloseJoysticks()
{
    for(int i = 0; i < m_iJoystickCount; i++)
    {
        m_Joysticks[i].id = 0;
        if(m_Joysticks[i].joy) { SDL_JoystickClose(m_Joysticks[i].joy); m_Joysticks[i].joy = NULL; }
        memset((void*)&(m_Joysticks[i].m_KeyState[0]), 0, sizeof(bool)* _OGE_MAX_JOY_KEY_NUMBER_ );
        memset((void*)&(m_Joysticks[i].m_KeyTime[0]),  0, sizeof(int) * _OGE_MAX_JOY_KEY_NUMBER_ );
    }
    SDL_JoystickEventState(SDL_DISABLE);
}

void CogeEngine::Screenshot()
{
    if(m_pVideo && m_pActiveScene)
    {
        CogeImage* pLastScreen = m_pVideo->GetLastScreen();
        if(pLastScreen)
        {
            pLastScreen->CopyRect(m_pActiveScene->m_pScreen, 0, 0,
            m_pActiveScene->m_SceneViewRect.left, m_pActiveScene->m_SceneViewRect.top,
            m_iVideoWidth, m_iVideoHeight);
        }
    }
}
CogeImage* CogeEngine::GetLastScreenshot()
{
    if(m_pVideo) return m_pVideo->GetLastScreen();
    else return NULL;
}
void CogeEngine::SaveScreenshot(const std::string& sBmpFileName)
{
    if(m_pVideo)
    {
        CogeImage* pLastScreen = m_pVideo->GetLastScreen();
        if(pLastScreen) pLastScreen->SaveAsBMP(sBmpFileName);
    }
}

CogeGameData* CogeEngine::GetAppGameData()
{
    return m_pAppGameData;
}

void CogeEngine::SetCustomData(CogeGameData* pGameData)
{
    m_pCustomData = pGameData;
}
CogeGameData* CogeEngine::GetCustomData()
{
    return m_pCustomData;
}

CogeScene* CogeEngine::GetCurrentScene()
{
    return m_pCurrentGameScene;
}

CogeScene* CogeEngine::GetActiveScene()
{
    return m_pActiveScene;
}
CogeScene* CogeEngine::GetLastActiveScene()
{
    return m_pLastActiveScene;
}

CogeScene* CogeEngine::GetNextActiveScene()
{
    return m_pNextActiveScene;
}

CogeSprite* CogeEngine::GetCurrentSprite()
{
    return m_pCurrentGameSprite;
}

void CogeEngine::CallCurrentSpriteBaseEvent()
{
    if(m_pCurrentGameSprite) m_pCurrentGameSprite->CallBaseEvent();
}

CogeSprite* CogeEngine::GetFocusSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetFocusSprite();
    else return NULL;
}

void CogeEngine::SetFocusSprite(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetFocusSprite(pSprite);
}

CogeSprite* CogeEngine::GetModalSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetModalSprite();
    else return NULL;
}
void CogeEngine::SetModalSprite(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetModalSprite(pSprite);
}

CogeSprite* CogeEngine::GetMouseSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetMouseSprite();
    else return NULL;
}
void CogeEngine::SetMouseSprite(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetMouseSprite(pSprite);
}

CogeSprite* CogeEngine::GetFirstSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetFirstSprite();
    else return NULL;
}
void CogeEngine::SetFirstSprite(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetFirstSprite(pSprite);
}

CogeSprite* CogeEngine::GetLastSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetLastSprite();
    else return NULL;
}
void CogeEngine::SetLastSprite(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetLastSprite(pSprite);
}

int CogeEngine::GetTopZ()
{
    if(m_pActiveScene) return m_pActiveScene->GetTopZ();
    else return 0;
}

int CogeEngine::BringSpriteToTop(CogeSprite* pSprite)
{
    if(m_pActiveScene) return m_pActiveScene->BringSpriteToTop(pSprite);
    else return pSprite->GetPosZ();
}

CogeSprite* CogeEngine::GetPlayerSprite(int iPlayerId)
{
    if(iPlayerId < 0 || iPlayerId >= _OGE_MAX_PLAYER_COUNT_) return NULL;
    else return m_Players[iPlayerId];
}
void CogeEngine::SetPlayerSprite(CogeSprite* pSprite, int iPlayerId)
{
    if(iPlayerId < 0 || iPlayerId >= _OGE_MAX_PLAYER_COUNT_) return;
    else m_Players[iPlayerId] = pSprite;
}

CogeSprite* CogeEngine::GetUserSprite(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_Npcs[iNpcId];
}
void CogeEngine::SetUserSprite(CogeSprite* pSprite, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_Npcs[iNpcId] = pSprite;
}

CogeSprite* CogeEngine::GetScenePlayerSprite(int iPlayerId)
{
    if(m_pActiveScene) return m_pActiveScene->GetPlayerSprite(iPlayerId);
    else return NULL;
}
void CogeEngine::SetScenePlayerSprite(CogeSprite* pSprite, int iPlayerId)
{
    if(m_pActiveScene) m_pActiveScene->SetPlayerSprite(pSprite, iPlayerId);
}

CogeSprite* CogeEngine::GetSceneDefaultPlayerSpr()
{
    if(m_pActiveScene) return m_pActiveScene->GetDefaultPlayerSprite();
    else return NULL;
}
void CogeEngine::SetSceneDefaultPlayerSpr(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->SetDefaultPlayerSprite(pSprite);
}

CogeSprite* CogeEngine::GetSceneNpcSprite(int iNpcId)
{
    if(m_pActiveScene) return m_pActiveScene->GetNpcSprite(iNpcId);
    else return NULL;
}

/*
void CogeEngine::SetSceneNpcSprite(CogeSprite* pSprite, int iNpcId)
{
    if(m_pActiveScene) m_pActiveScene->SetNpcSprite(pSprite, iNpcId);
}
*/

/*
CogeSprite* CogeEngine::GetPlotSprite()
{
    if(m_pActiveScene) return m_pActiveScene->GetPlotSprite();
    else return NULL;
}

CogeSprite* CogeEngine::OpenPlot(CogeSprite* pSprite)
{
    if(m_pActiveScene) return m_pActiveScene->OpenPlot(pSprite);
    else return NULL;
}
void CogeEngine::ClosePlot(CogeSprite* pSprite)
{
    if(m_pActiveScene) m_pActiveScene->ClosePlot(pSprite);
}

bool CogeEngine::IsPlayingPlot(CogeSprite* pSprite)
{
    return m_pActiveScene && m_pActiveScene->IsPlayingPlot(pSprite);
}
*/

void CogeEngine::ActivateGroup(CogeSpriteGroup* pGroup)
{
    if(pGroup && m_pActiveScene) pGroup->SetCurrentScene(m_pActiveScene);
}
void CogeEngine::DeactivateGroup(CogeSpriteGroup* pGroup)
{
    if(pGroup) pGroup->SetCurrentScene(NULL);
}

CogeSpriteGroup* CogeEngine::GetUserGroup(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_UserGroups[iNpcId];
}
void CogeEngine::SetUserGroup(CogeSpriteGroup* pGroup, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_UserGroups[iNpcId] = pGroup;
}

CogePath* CogeEngine::GetUserPath(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_UserPaths[iNpcId];
}
void CogeEngine::SetUserPath(CogePath* pPath, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_UserPaths[iNpcId] = pPath;
}

CogeImage* CogeEngine::GetUserImage(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_UserImages[iNpcId];
}
void CogeEngine::SetUserImage(CogeImage* pImage, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_UserImages[iNpcId] = pImage;
}

CogeGameData* CogeEngine::GetUserData(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_UserData[iNpcId];
}
void CogeEngine::SetUserData(CogeGameData* pData, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_UserData[iNpcId] = pData;
}

void* CogeEngine::GetUserObject(int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return NULL;
    else return m_UserObjects[iNpcId];
}
void CogeEngine::SetUserObject(void* pObject, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_UserObjects[iNpcId] = pObject;
}

void CogeEngine::Pause()
{
    if(m_pActiveScene) m_pActiveScene->Pause();
}
void CogeEngine::Resume()
{
    if(m_pActiveScene) m_pActiveScene->Resume();
}
bool CogeEngine::IsPlaying()
{
    return m_pActiveScene && m_pActiveScene->IsPlaying();
}

int CogeEngine::GetSceneTime()
{
    if(m_pActiveScene) return m_pActiveScene->GetActiveTime();
    else return 0;
}

int CogeEngine::OpenLocalServer(int iPort)
{
    if(m_pNetwork) return m_pNetwork->OpenLocalServer(iPort);
    else return -1;
}
int CogeEngine::CloseLocalServer()
{
    if(m_pNetwork) return m_pNetwork->CloseLocalServer();
    else return -1;
}

CogeSocket* CogeEngine::ConnectServer(const std::string& sAddr)
{
    if(m_pNetwork) return m_pNetwork->ConnectServer(sAddr);
    else return NULL;
}

int CogeEngine::DisconnectServer(const std::string& sAddr)
{
    if(m_pNetwork) return m_pNetwork->DisconnectServer(sAddr);
    else return -1;
}

int CogeEngine::DisconnectClient(const std::string& sAddr)
{
    if(m_pNetwork) return m_pNetwork->KickClient(sAddr);
    else return -1;
}

const std::string& CogeEngine::GetCurrentSocketAddr()
{
    if(m_pNetwork)
    {
        CogeSocket* pCurrentSocket = m_pNetwork->GetSocket();
        if(pCurrentSocket) return pCurrentSocket->GetAddr();
        else return _OGE_EMPTY_STR_;
    }
    else return _OGE_EMPTY_STR_;
}

int CogeEngine::GetRecvDataSize()
{
    if(m_pNetwork) return m_pNetwork->GetRecvDataSize();
    else return 0;
}

int CogeEngine::GetRecvDataType()
{
    if(m_pNetwork) return m_pNetwork->GetMsgType();
    else return -1;
}

/*
std::string CogeEngine::GetRecvMsg()
{
    if(m_pNetwork) return m_pNetwork->GetRecvMsg();
    else return "";
}
int CogeEngine::SendMsg(const std::string& sMsg)
{
    if(m_pNetwork) return m_pNetwork->SendMsg(sMsg);
    else return 0;
}
*/

char* CogeEngine::GetRecvData()
{
    if(m_pNetwork) return m_pNetwork->GetRecvData();
    else return NULL;
}

int CogeEngine::SendData(char* pData, int iLen, int iMsgType)
{
    if(m_pNetwork) return m_pNetwork->SendData(pData, iLen, iMsgType);
    else return 0;
}

int CogeEngine::RefreshClientList()
{
    if(m_pNetwork) return m_pNetwork->RefreshClientList();
    else return 0;
}
int CogeEngine::RefreshServerList()
{
    if(m_pNetwork) return m_pNetwork->RefreshServerList();
    else return 0;
}
CogeSocket* CogeEngine::GetClientByIndex(int idx)
{
    if(m_pNetwork) return m_pNetwork->GetClientByIndex(idx);
    else return NULL;
}
CogeSocket* CogeEngine::GetServerByIndex(int idx)
{
    if(m_pNetwork) return m_pNetwork->GetServerByIndex(idx);
    else return NULL;
}
CogeSocket* CogeEngine::GetCurrentSocket()
{
    if(m_pNetwork) return m_pNetwork->GetSocket();
    else return NULL;
}

CogeSocket* CogeEngine::GetPlayerSocket(int idx)
{
    if(idx >= 0 && idx < _OGE_MAX_SOCK_COUNT_) return m_Socks[idx];
    else return NULL;
}
void CogeEngine::SetPlayerSocket(CogeSocket* pSocket, int idx)
{
    if(idx >= 0 && idx < _OGE_MAX_SOCK_COUNT_) m_Socks[idx] = pSocket;
}

CogeSocket* CogeEngine::GetDebugSocket()
{
    if(m_pNetwork) return m_pNetwork->GetDebugSocket();
    else return NULL;
}

const std::string& CogeEngine::GetLocalIp()
{
    if(m_pNetwork) return m_pNetwork->GetLocalIp();
    else return _OGE_EMPTY_STR_;
}
CogeSocket* CogeEngine::FindClient(const std::string& sAddr)
{
    if(m_pNetwork) return m_pNetwork->FindClient(sAddr);
    else return NULL;
}
CogeSocket* CogeEngine::FindServer(const std::string& sAddr)
{
    if(m_pNetwork) return m_pNetwork->FindServer(sAddr);
    else return NULL;
}
CogeSocket* CogeEngine::GetLocalServer()
{
    if(m_pNetwork) return m_pNetwork->GetLocalServer();
    else return NULL;
}
int CogeEngine::ScanRemoteDebugRequest()
{
    if(m_pNetwork) return m_pNetwork->ScanDebugRequest();
    else return -1;
}
int CogeEngine::StartRemoteDebug(const std::string& sAddr)
{
    int rsl = -1;
    if(m_pNetwork) rsl = m_pNetwork->ConnectDebugServer(sAddr);
    if(rsl >= 0)
    {
        if(m_pScripter)
        {
            m_pScripter->SetDebugMode(Debug_Remote);
            //ScanRemoteDebugRequest();
        }
    }
    return rsl;
}
int CogeEngine::StopRemoteDebug()
{
    int rsl = 0;
    if(m_pNetwork) rsl = m_pNetwork->DisconnectDebugServer();
    if(m_pScripter) m_pScripter->SetDebugMode(Debug_None);
    return rsl;
}
bool CogeEngine::IsRemoteDebugMode()
{
    if(m_pNetwork) return m_pNetwork->IsRemoteDebugMode();
    else return false;
}

int CogeEngine::StartLocalDebug()
{
    if(m_pScripter == NULL) return 0;

    std::string sInputConfigFile = OGE_GetAppInputFile();
    if(sInputConfigFile.length() == 0) return 0;

    CogeIniFile ini;
    if(!ini.Load(sInputConfigFile)) return -1;

    int iRsl = 0;

    int iBPCount = ini.ReadInteger("Input", "BPCount", 0);

    for(int i=1; i<=iBPCount; i++)
    {
        std::string sIdx = "BP" + OGE_itoa(i);
        std::string sBreakPoint = ini.ReadString("Input", sIdx, "");
        if(sBreakPoint.length() > 0)
        {
            size_t iPos = sBreakPoint.find(':');
            if(iPos != std::string::npos)
            {
                std::string sFileName = sBreakPoint.substr(0, iPos);
                std::string sLineNumber = sBreakPoint.substr(iPos+1);
                int iLineNumber = atoi(sLineNumber.c_str());

                m_pScripter->AddBreakPoint(sFileName, iLineNumber);

                iRsl++;
            }
        }
    }

    if(iRsl > 0) m_pScripter->SetDebugMode(Debug_Local);

    return iRsl;

}

CogeScript* CogeEngine::GetCurrentScript()
{
    if(m_pScripter) return m_pScripter->GetCurrentScript();
    else return NULL;
}

int CogeEngine::OpenDefaultDb()
{
    if(m_sAppDefaultDbFile.length() > 0)
    {
        if(m_pDatabase) return m_pDatabase->OpenDb(DB_SQLITE3, m_sAppDefaultDbFile);
        else return 0;
    }
    else return 0;
}

CogeIniFile* CogeEngine::GetUserConfig()
{
    return &m_UserConfigFile;
}

CogeIniFile* CogeEngine::GetAppConfig()
{
    return &m_AppIniFile;
}

int CogeEngine::LoadConstInt()
{
    if(m_pScripter == NULL || m_bBinaryScript) return 0;

    int iTotal = 0;

    int iCount = m_ConstIntIniFile.GetSectionCount();
    for(int i=0; i<iCount; i++)
    {
        std::string sType = m_ConstIntIniFile.GetSectionNameByIndex(i);
        std::map<std::string, std::string>* pSection = m_ConstIntIniFile.GetSectionByIndex(i);
        if(sType.length() == 0 || pSection == NULL) continue;

        //size_t iSceneFlagPos = sType.find('_');
        //bool bIsScene = iSceneFlagPos != std::string::npos;

        std::string sTypeName = "ogeUser" + sType;
        std::string sPrefix = sType + "_";

        //if(bIsScene)
        //{
        //    //sTypeName = "oge" + sType;
        //    sTypeName = sType.substr(iSceneFlagPos+1);
        //    sPrefix = "";
        //}

        m_pScripter->RegisterEnum(sTypeName);

        std::map<std::string, std::string>::iterator it = pSection->begin();
        while(it != pSection->end())
        {
            std::string sName = it->first;
            std::string sValue = it->second;
            int iValue = atoi(sValue.c_str());

            m_pScripter->RegisterEnumValue(sTypeName, sPrefix + sName, iValue);

            iTotal++;

            it++;
        }
    }

    return iTotal;
}

int CogeEngine::LoadKeyMap()
{
    int iCount = 0;

    std::string sConfigFileName = OGE_GetAppGameDir() + "/" + _OGE_DEFAULT_KEY_MAP_FILE_;

    if(sConfigFileName.length() > 0 && OGE_IsFileExisted(sConfigFileName.c_str()))
    {
        CogeIniFile ini;
        if (ini.Load(sConfigFileName))
        {
            int iMinCode = ini.ReadInteger("KeyMap", "Min", 1);
            if(iMinCode <= 0) iMinCode = 1;
            int iMaxCode = ini.ReadInteger("KeyMap", "Max", _OGE_MAX_SCANCODE_);
            if(iMaxCode >= _OGE_MAX_SCANCODE_) iMaxCode = _OGE_MAX_SCANCODE_ - 1;
            for(int i=iMinCode; i<=iMaxCode; i++)
            {
                m_KeyMap[i] = ini.ReadInteger("KeyMap", "S" + OGE_itoa(i), 0);
                iCount++;
            }
        }
    }

    return iCount;
}

int CogeEngine::LoadLibraries()
{
    int iCount = m_LibraryIniFile.ReadInteger("Library", "Count",  0);

    for(int i=1; i<=iCount; i++)
    {
        std::string sIdx = "Library_" + OGE_itoa(i);
        std::string sLibraryName = m_LibraryIniFile.ReadString(sIdx, "Name", "");
        std::string sLibraryFile = m_LibraryIniFile.ReadFilePath(sIdx, "File", "");

        if(sLibraryName.length() > 0 && sLibraryFile.length() > 0)
            clay::lib::Plugin(sLibraryName, sLibraryFile);
    }

    return iCount;

}

bool CogeEngine::LibraryExists(const std::string& sLibraryName)
{
    return clay::lib::ProviderExists(sLibraryName);
}
bool CogeEngine::ServiceExists(const std::string& sLibraryName, const std::string& sServiceName)
{
    return clay::lib::ServiceExists(sLibraryName, sServiceName);
}

int CogeEngine::CallService(const std::string& sLibraryName, const std::string& sServiceName, void* pParam)
{
    return clay::lib::CallService(sLibraryName, sServiceName, pParam);
}

/*
std::string CogeEngine::ReadString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    return m_UserConfigFile.ReadString(sSectionName, sFieldName, sDefault);
}
std::string CogeEngine::ReadPath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    return m_UserConfigFile.ReadPath(sSectionName, sFieldName, sDefault);
}
int CogeEngine::ReadInteger(const std::string& sSectionName, const std::string& sFieldName, int iDefault)
{
    return m_UserConfigFile.ReadInteger(sSectionName, sFieldName, iDefault);
}
double CogeEngine::ReadFloat(const std::string& sSectionName, const std::string& sFieldName, double fDefault)
{
    return m_UserConfigFile.ReadFloat(sSectionName, sFieldName, fDefault);
}

bool CogeEngine::WriteString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue)
{
    return m_UserConfigFile.WriteString(sSectionName, sFieldName, sValue);
}
bool CogeEngine::WriteInteger(const std::string& sSectionName, const std::string& sFieldName, int iValue)
{
    return m_UserConfigFile.WriteInteger(sSectionName, sFieldName, iValue);
}
bool CogeEngine::WriteFloat(const std::string& sSectionName, const std::string& sFieldName, double fValue)
{
    return m_UserConfigFile.WriteFloat(sSectionName, sFieldName, fValue);
}

bool CogeEngine::SaveConfig()
{
    return m_UserConfigFile.Save();
}
*/

int CogeEngine::Terminate()
{
    if(m_iState > 0)
    {
        OGE_Log("\nExiting game... \n\n");
        if (m_sTitle.length() == 0) m_sTitle = "Game";
        m_sTitle = m_sTitle + " ( Exiting... ) ";

#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_Window *window = SDL_GetKeyboardFocus();
        if (window) SDL_SetWindowTitle(window, m_sTitle.c_str());
#else
        SDL_WM_SetCaption(m_sTitle.c_str(), NULL);
#endif

        m_iState = 0;

    }
    return m_iState;
}

int CogeEngine::PrepareFirstChapter()
{
    return 0;
}

/*---------------- Chapter -----------------*/
/*
CogeChapter::CogeChapter(const std::string& sName, CogeEngine* pTheEngine)
{

}

CogeChapter::~CogeChapter()
{

}

int CogeChapter::Initialize(const std::string& sConfigFileName)
{
    return 0;
}

void CogeChapter::Finalize()
{
    return;
}

CogeScene* CogeChapter::NewScene(const std::string& sSceneName, const std::string& sConfigFileName)
{
    return NULL;
}

CogeScene* CogeChapter::FindScene(const std::string& sSceneName)
{
    ogeSceneMap::iterator it;

    it = m_SceneMap.find(sSceneName);
	if (it != m_SceneMap.end()) return it->second;
	else return NULL;
}


CogeScene* CogeChapter::GetScene(const std::string& sSceneName, const std::string& sConfigFileName)
{
    CogeScene* pMatchedScene = FindScene(sSceneName);
    if (pMatchedScene) return pMatchedScene;
    else return NewScene(sSceneName, sConfigFileName);
}

int CogeChapter::DelScene(const std::string& sSceneName)
{
    CogeScene* pMatchedScene = FindScene(sSceneName);
	if (pMatchedScene)
	{
	    //if (pMatchedChapter->m_iTotalUsers > 0) return false;
		m_SceneMap.erase(sSceneName);
		delete pMatchedScene;
		return 1;

	}
	return -1;
}

int CogeChapter::PrepareFirstScene()
{
    return 0;
}
*/

/*---------------- Scene -----------------*/

CogeScene::CogeScene(const std::string& sName, CogeEngine* pTheEngine):
m_pEngine(pTheEngine),
m_pScript(NULL),
m_pScreen(NULL),
m_pBackground(NULL),
m_pLightMap(NULL),
m_pFadeMask(NULL),
m_pMap(NULL),
m_pBackgroundMusic(NULL),
m_pCurrentSpr(NULL),
m_pDraggingSpr(NULL),
m_pModalSpr(NULL),
m_pFocusSpr(NULL),
m_pMouseSpr(NULL),
m_pPlotSpr(NULL),
m_pGettingFocusSpr(NULL),
m_pLosingFocusSpr(NULL),
m_pPlayerSpr(NULL),
m_pFirstSpr(NULL),
m_pLastSpr(NULL),
//m_pNextScene(NULL),
//m_pSocketScene(NULL),
m_pGameData(NULL),
m_pCustomData(NULL),
m_sName(sName),
m_iSceneType(-1)
{
    memset((void*)&m_Events[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);
    memset((void*)&m_Players[0],  0, sizeof(int) * _OGE_MAX_PLAYER_COUNT_);

    //memset((void*)&m_Npcs[0],     0, sizeof(int) * _OGE_MAX_NPC_COUNT_);

    m_iScriptState = -1;
    m_iState = -1;

    //m_sNextSceneName = "";
    m_sChapterName = "";

    m_sTagName = "";

    //m_bUseDirtyRect = false;
    m_iTotalDirtyRects = 0;
    //m_bNeedRedrawBg = false;

    m_iTopZ = 0;

    m_SceneViewRect.left   = 0;
    m_SceneViewRect.top    = 0;
    m_SceneViewRect.right  = 0;
    m_SceneViewRect.bottom = 0;

    m_iBackgroundWidth = 0;
    m_iBackgroundHeight = 0;

    m_iBackgroundColor = 0;
    m_iBackgroundColorRGB = 0;

    m_bMouseDown = false;
    m_bIsMouseEventHandled = false;

    m_bBusy = false;

    //m_iFadeState = 0;

    m_iFadeState = 0;
    m_iFadeEffectValue = 0;
    m_iFadeStep = 0;
    m_iFadeEffectCurrentValue = 0;
    m_iFadeEffectStartValue = 0;
    m_iFadeInterval = 0;
    m_iFadeTime = 0;
    m_iFadeType = 0;

    m_iFadeInType  = 0;
    m_iFadeOutType = 0;
    m_bAutoFade    = true;

    m_iScaleValue = 0;

    m_iLightMode = 0;
    m_bEnableSpriteLight = false;

    m_iScrollStepX = 0;
    m_iScrollStepY = 0;
    m_iScrollState = 0;
    m_iScrollInterval = 0;
    m_iScrollTime = 0;
    m_bScrollLoop = false;

    m_iScrollCurrentX = 0;
    m_iScrollCurrentY = 0;

    m_iScrollTargetX = -1;
    m_iScrollTargetY = -1;

    m_bEnableDefaultTimer = false;
    m_iTimerLastTick = 0;
    m_iTimerInterval = 0;
    m_iTimerEventCount = 0;
    //m_iPlotTimerTriggerFlag = 0;

    m_bHasDefaultData = false;
    m_bOwnCustomData = false;

    m_iPendingPosX = _OGE_INVALID_POS_;
    m_iPendingPosY = _OGE_INVALID_POS_;
    m_iPendingScrollX = 0;
    m_iPendingScrollY = 0;


}

CogeScene::~CogeScene()
{
    Finalize();
}

CogeScript* CogeScene::NewScript(const std::string& sScriptName)
{
    std::string sFileName  = m_IniFile.ReadFilePath("Scripts", sScriptName, "");
    if(sFileName.length() == 0) return NULL;

    return m_pEngine->NewFixedScript(sFileName);
}

int CogeScene::CallEvent(int iEventCode)
{
    //if (m_iState < 0) return;
    if (m_iScriptState < 0) return -1;

    if(m_pEngine->m_bFreeze)
    {
        //if(iEventCode == Event_OnUpdate) return -1;
        if(iEventCode >= Event_OnMouseOver && iEventCode <= Event_OnKeyUp) return -1;
    }

    int id = m_Events[iEventCode];

	int rsl = -1;

	if(id >= 0)
    {
        rsl = m_pScript->CallFunction(id);
        //m_pScript->CallGC();
    }

	return rsl;
}

void CogeScene::HandleEvent()
{
    if(!m_pEngine->m_bFreeze && m_pEngine->m_bKeyEventHappened)
    switch (m_pEngine->m_GlobalEvent.type)
    {

    // check for keypresses
    case SDL_KEYDOWN:

        //m_pEngine->m_pCurrentGameScene = this;
        CallProxySprEvent(Event_OnPathStep);
        CallEvent(Event_OnKeyDown);
        break;

    case SDL_KEYUP:

        //m_pEngine->m_pCurrentGameScene = this;
        CallProxySprEvent(Event_OnPathFin);
        CallEvent(Event_OnKeyUp);
        break;

    } // end switch
}

CogeGameMap* CogeScene::GetMap()
{
    return m_pMap;
}

CogeImage* CogeScene::GetBackground()
{
    return m_pBackground;
}

int CogeScene::SetMap(const std::string& sMapName, int iNewLeft, int iNewTop)
{
    CogeGameMap* pMap = NULL;
    pMap = m_pEngine->GetGameMap(sMapName);
    if (pMap == NULL || pMap->m_pBackground == NULL) return -1;

    if(iNewLeft < 0) iNewLeft = 0;
    if(iNewTop < 0)  iNewTop  = 0;

    int iNewRight  = iNewLeft + m_pEngine->m_iVideoWidth;
    int iNewBottom = iNewTop + m_pEngine->m_iVideoHeight;

    if(iNewRight > pMap->m_pBackground->GetWidth())
    {
        iNewRight = pMap->m_pBackground->GetWidth();
        iNewLeft = iNewRight - m_pEngine->m_iVideoWidth;
        //if(iNewLeft < 0) iNewLeft = 0;
    }

    if(iNewBottom > pMap->m_pBackground->GetHeight())
    {
        iNewBottom = pMap->m_pBackground->GetHeight();
        iNewTop = iNewBottom - m_pEngine->m_iVideoHeight;
        //if(iNewTop < 0)  iNewTop  = 0;
    }

    bool bNeedUpdateScreen = false;

    if(m_pMap)
    {
        ogeSpriteMap::iterator it, itb, ite;
        CogeSprite* pCurrentSpr = NULL;

        itb = m_ActiveSprites.begin();
        ite = m_ActiveSprites.end();

        for (it=itb; it!=ite; it++)
        {
            pCurrentSpr = it->second;

            if(!pCurrentSpr->m_bIsRelative)
            {
                pCurrentSpr->m_iPosX = iNewLeft + (pCurrentSpr->m_iPosX - m_SceneViewRect.left);
                pCurrentSpr->m_iPosY = iNewTop + (pCurrentSpr->m_iPosY - m_SceneViewRect.top);
                pCurrentSpr->m_iPathOffsetX = iNewLeft + (pCurrentSpr->m_iPathOffsetX - m_SceneViewRect.left);
                pCurrentSpr->m_iPathOffsetY = iNewTop + (pCurrentSpr->m_iPathOffsetY - m_SceneViewRect.top);
            }
        }

        m_pMap->Fire();
        //m_pEngine->DelGameMap(m_pMap->m_sName);
        m_pMap = NULL;

        bNeedUpdateScreen = true;
    }

    m_pMap = pMap;
    m_pBackground = m_pMap->m_pBackground;
    m_pMap->Hire();
    m_pMap->Reset();

    if(m_pScreen)
    {
        if(m_pScreen->GetWidth()  == m_pMap->m_iBgWidth &&
           m_pScreen->GetHeight() == m_pMap->m_iBgHeight )
        {
            if(m_pMap)
            {
                m_iBackgroundWidth = m_pMap->m_iBgWidth;
                m_iBackgroundHeight = m_pMap->m_iBgHeight;

                if(m_pScreen && m_pBackground)
                {
                    if(m_pEngine->m_bUseDirtyRect) m_pScreen->CopyRect(m_pBackground, 0, 0);
                    //m_pScreen->UpdateRect();
                }
            }
            return 0;
        }
        else
        {
            if(m_pScreen != m_pEngine->m_pVideo->GetDefaultScreen())
            {
                m_pScreen->Fire();
                //m_pEngine->m_pVideo->DelImage(m_pScreen->GetName());
                m_pEngine->m_pVideo->DelImage(m_pScreen);
            }
            m_pScreen = NULL;
        }
    }

    std::string sScreenName = m_sName + "_" + OGE_itoa(m_pMap->m_iBgWidth) + "x" + OGE_itoa(m_pMap->m_iBgHeight);

    CogeImage* pScreen = m_pEngine->m_pVideo->GetImage(sScreenName, m_pMap->m_iBgWidth, m_pMap->m_iBgHeight);
    if (pScreen == NULL) return -1;
    else
    {
        m_pScreen = pScreen;
        m_pScreen->Hire();
    }

    if(m_pMap)
    {
        m_iBackgroundWidth = m_pMap->m_iBgWidth;
        m_iBackgroundHeight = m_pMap->m_iBgHeight;

        if(m_pScreen && m_pBackground)
        {
            if(m_pEngine->m_bUseDirtyRect) m_pScreen->CopyRect(m_pBackground, 0, 0);
            //if(bNeedUpdateRect) m_pScreen->UpdateRect();

            if(bNeedUpdateScreen)
            {
                m_pEngine->m_pVideo->SetScreen(m_pScreen);
            }
        }
    }

    SetViewPos(iNewLeft, iNewTop, true);

    return 1;
}

CogeSprite* CogeScene::AddSprite(CogeSprite* pSprite)
{
	if (pSprite == NULL) return NULL;

	std::string sSpriteName = pSprite->GetName();

	//ogeSpriteMap::iterator its;
	//CogeSprite* pMatchedSprite = NULL;
    //its = m_SpriteMap.find(sSpriteName);
    //if (its != m_SpriteMap.end()) return its->second;

    /*
	if (its != m_SpriteMap.end())
	{
	    pMatchedSprite = its->second;
	    if(pSprite == pMatchedSprite) return pSprite;
	    else RemoveSprite(pMatchedSprite);

        //m_SpriteMap.erase(its);
        //pMatchedSprite->Fire();

        //return pMatchedSprite;
	}
	*/

	m_ActiveSprites.erase(sSpriteName);
	m_DeactiveSprites.erase(sSpriteName);

	m_SpriteMap.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));

	pSprite->m_pCurrentScene = this;
	pSprite->m_bActive = false;
	pSprite->m_bBusy = false;
	pSprite->m_bEnableTimer = false;

	//pTheSprite->Hire();

	m_DeactiveSprites.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));

	int iCount = pSprite->m_Children.size();
    if(iCount > 0)
    {
        ogeSpriteMap::iterator it = pSprite->m_Children.begin();
        while(iCount > 0)
        {
            AddSprite(it->second);
            it++;
            iCount--;
        }
    }

	return pSprite;
}


CogeSprite* CogeScene::AddSprite(const std::string& sSpriteName, const std::string& sBaseName)
{
    if (sBaseName.length() == 0)
    {
        OGE_Log("Error in CogeScene::AddSprite(): Missing sprite template name.\n");
        return NULL;
    }

    std::string sRealName = sSpriteName;

    if (sRealName.length() == 0) sRealName = m_sName + "_" + sBaseName; // auto name it ...

    ogeSpriteMap::iterator its = m_SpriteMap.find(sRealName);
    if (its != m_SpriteMap.end()) return its->second;

    //CogeSprite* pTheSprite = m_pEngine->GetSprite(sRealName, sBaseName);
    CogeSprite* pTheSprite = m_pEngine->NewSprite(sRealName, sBaseName);

	if (pTheSprite == NULL) return NULL;

	return AddSprite(pTheSprite);

}

int CogeScene::RemoveSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator its;

    m_ActiveSprites.erase(sSpriteName);
	m_DeactiveSprites.erase(sSpriteName);

	CogeSprite* pMatchedSprite = NULL;
    its = m_SpriteMap.find(sSpriteName);
	if (its != m_SpriteMap.end())
	{
	    pMatchedSprite = its->second;

        if(pMatchedSprite->m_pActiveUpdateScript) pMatchedSprite->m_pActiveUpdateScript->Stop();
		if(pMatchedSprite->IsPlayingPlot()) pMatchedSprite->DisablePlot();

	    pMatchedSprite->m_bActive = false;
	    pMatchedSprite->m_bBusy = false;
	    pMatchedSprite->m_bEnableTimer = false;
	    pMatchedSprite->m_pMasterSpr = NULL;
	    pMatchedSprite->m_pReplacedSpr = NULL;

	    pMatchedSprite->m_iLastDisplayFlag = 0;

	    pMatchedSprite->ClearRunningCustomEvents();

        if(pMatchedSprite->m_pTextInputter && m_pEngine->m_pIM)
        {
            CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
            if(pTextInputter == pMatchedSprite->m_pTextInputter) m_pEngine->DisableIM();
        }

	    int iCount = pMatchedSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pMatchedSprite->m_Children.begin();
            while(iCount > 0)
            {
                RemoveSprite(it->second);
                it++;
                iCount--;
            }
        }

	    pMatchedSprite->m_pCurrentScene = NULL;

	    if(pMatchedSprite == m_pPlotSpr) m_pPlotSpr = NULL;
	    if(pMatchedSprite == m_pFocusSpr) m_pFocusSpr = NULL;
	    if(pMatchedSprite == m_pModalSpr) m_pModalSpr = NULL;
	    if(pMatchedSprite == m_pDraggingSpr) m_pDraggingSpr = NULL;
	    if(pMatchedSprite == m_pGettingFocusSpr) m_pGettingFocusSpr = NULL;
	    if(pMatchedSprite == m_pLosingFocusSpr) m_pLosingFocusSpr = NULL;
	    if(pMatchedSprite == m_pPlayerSpr) m_pPlayerSpr = NULL;
	    if(pMatchedSprite == m_pMouseSpr) m_pMouseSpr = NULL;
	    if(pMatchedSprite == m_pFirstSpr) m_pFirstSpr = NULL;
	    if(pMatchedSprite == m_pLastSpr) m_pLastSpr = NULL;

        m_SpriteMap.erase(its);
        if(pMatchedSprite->m_pCurrentGroup == NULL) m_pEngine->DelSprite(pMatchedSprite);
        //pMatchedSprite->Fire();
        return 1;
	}
	else return 0;
}

int CogeScene::RemoveSprite(CogeSprite* pSprite)
{
    if(!pSprite) return 0;

    bool bFound = false;
    ogeSpriteMap::iterator its;

    for(its = m_ActiveSprites.begin(); its != m_ActiveSprites.end(); its++)
    {
        if(its->second == pSprite)
        {
            bFound = true;
            break;
        }
    }
    if(bFound) m_ActiveSprites.erase(its);

    bFound = false;

    for(its = m_DeactiveSprites.begin(); its != m_DeactiveSprites.end(); its++)
    {
        if(its->second == pSprite)
        {
            bFound = true;
            break;
        }
    }
    if(bFound) m_DeactiveSprites.erase(its);


    bFound = false;

    for(its = m_SpriteMap.begin(); its != m_SpriteMap.end(); its++)
    {
        if(its->second == pSprite)
        {
            bFound = true;
            break;
        }
    }

	if (bFound)
	{
	    CogeSprite* pMatchedSprite = its->second;

        if(pMatchedSprite->m_pActiveUpdateScript) pMatchedSprite->m_pActiveUpdateScript->Stop();
		if(pMatchedSprite->IsPlayingPlot()) pMatchedSprite->DisablePlot();

	    pMatchedSprite->m_bActive = false;
	    pMatchedSprite->m_bBusy = false;
	    pMatchedSprite->m_bEnableTimer = false;
	    pMatchedSprite->m_pMasterSpr = NULL;
	    pMatchedSprite->m_pReplacedSpr = NULL;

	    pMatchedSprite->m_iLastDisplayFlag = 0;

	    pMatchedSprite->ClearRunningCustomEvents();

        if(pMatchedSprite->m_pTextInputter && m_pEngine->m_pIM)
        {
            CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
            if(pTextInputter == pMatchedSprite->m_pTextInputter) m_pEngine->DisableIM();
        }

	    int iCount = pMatchedSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pMatchedSprite->m_Children.begin();
            while(iCount > 0)
            {
                RemoveSprite(it->second);
                it++;
                iCount--;
            }
        }

	    pMatchedSprite->m_pCurrentScene = NULL;

	    if(pMatchedSprite == m_pPlotSpr) m_pPlotSpr = NULL;
	    if(pMatchedSprite == m_pFocusSpr) m_pFocusSpr = NULL;
	    if(pMatchedSprite == m_pModalSpr) m_pModalSpr = NULL;
	    if(pMatchedSprite == m_pDraggingSpr) m_pDraggingSpr = NULL;
	    if(pMatchedSprite == m_pGettingFocusSpr) m_pGettingFocusSpr = NULL;
	    if(pMatchedSprite == m_pLosingFocusSpr) m_pLosingFocusSpr = NULL;
	    if(pMatchedSprite == m_pPlayerSpr) m_pPlayerSpr = NULL;
	    if(pMatchedSprite == m_pMouseSpr) m_pMouseSpr = NULL;
	    if(pMatchedSprite == m_pFirstSpr) m_pFirstSpr = NULL;
	    if(pMatchedSprite == m_pLastSpr) m_pLastSpr = NULL;

        m_SpriteMap.erase(its);
        if(pMatchedSprite->m_pCurrentGroup == NULL) m_pEngine->DelSprite(pMatchedSprite);
        //pMatchedSprite->Fire();
        return 1;
	}
	else return 0;
}

CogeSprite* CogeScene::FindSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator it;

    it = m_SpriteMap.find(sSpriteName);
	if (it != m_SpriteMap.end()) return it->second;
	else return NULL;
}

CogeSprite* CogeScene::FindActiveSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator it;

    it = m_ActiveSprites.find(sSpriteName);
	if (it != m_ActiveSprites.end()) return it->second;
	else return NULL;
}
CogeSprite* CogeScene::FindDeactiveSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator it;

    //std::string sRealName = sSpriteName;
    //if(sRealName.find(m_sName + "_", 0) == std::string::npos)
    //sRealName = m_sName + "_" + sRealName;

    it = m_DeactiveSprites.find(sSpriteName);
	if (it != m_DeactiveSprites.end()) return it->second;
	else return NULL;
}

int CogeScene::ActivateSprite(const std::string& sSpriteName)
{
    //std::string sRealName = sSpriteName;
    //if(sRealName.find(m_sName + "_", 0) == std::string::npos)
    //sRealName = m_sName + "_" + sRealName;

    CogeSprite* pSprite = FindActiveSprite(sSpriteName);
    if(pSprite)
    {
        pSprite->m_pCurrentScene = this;
        pSprite->m_bActive = true;
        pSprite->m_bBusy = true;
        pSprite->ResetAnima();

        if(pSprite->m_iUnitType == Spr_Timer && pSprite->m_iDefaultTimerInterval > 0)
        {
            pSprite->OpenTimer(pSprite->m_iDefaultTimerInterval);
        }

        //pSprite->CallEvent(Event_OnActive);
        m_Activating.push_back(pSprite);

        if(pSprite->m_bIsRelative && pSprite->m_pParent == NULL) pSprite->AdjustSceneView();

        int iCount = pSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pSprite->m_Children.begin();
            while(iCount > 0)
            {
                it->second->SetActive(true);
                it++;
                iCount--;
            }
        }

        return 0;
    }
    else
    {
        pSprite = FindDeactiveSprite(sSpriteName);
        if(pSprite)
        {
            m_DeactiveSprites.erase(sSpriteName);
            m_ActiveSprites.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));

            pSprite->m_pCurrentScene = this;
            pSprite->m_bActive = true;
            pSprite->m_bBusy = true;
            pSprite->ResetAnima();

            if(pSprite->m_iUnitType == Spr_Timer && pSprite->m_iDefaultTimerInterval > 0)
            {
                pSprite->OpenTimer(pSprite->m_iDefaultTimerInterval);
            }

            //pSprite->CallEvent(Event_OnActive);
            m_Activating.push_back(pSprite);

            if(pSprite->m_bIsRelative && pSprite->m_pParent == NULL) pSprite->AdjustSceneView();

            int iCount = pSprite->m_Children.size();
            if(iCount > 0)
            {
                ogeSpriteMap::iterator it = pSprite->m_Children.begin();
                while(iCount > 0)
                {
                    it->second->SetActive(true);
                    it++;
                    iCount--;
                }
            }

            return 1;
        }
        else
        {
            pSprite = FindSprite(sSpriteName);
            if(pSprite)
            {
                m_ActiveSprites.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));

                pSprite->m_pCurrentScene = this;
                pSprite->m_bActive = true;
                pSprite->m_bBusy = true;
                pSprite->ResetAnima();

                if(pSprite->m_iUnitType == Spr_Timer && pSprite->m_iDefaultTimerInterval > 0)
                {
                    pSprite->OpenTimer(pSprite->m_iDefaultTimerInterval);
                }

                //pSprite->CallEvent(Event_OnActive);
                m_Activating.push_back(pSprite);

                if(pSprite->m_bIsRelative && pSprite->m_pParent == NULL) pSprite->AdjustSceneView();

                int iCount = pSprite->m_Children.size();
                if(iCount > 0)
                {
                    ogeSpriteMap::iterator it = pSprite->m_Children.begin();
                    while(iCount > 0)
                    {
                        it->second->SetActive(true);
                        it++;
                        iCount--;
                    }
                }

                return 2;
            }
            else return -1;
        }

    }
}

int CogeScene::DeactivateSprite(const std::string& sSpriteName)
{
    //std::string sRealName = sSpriteName;
    //if(sRealName.find(m_sName + "_", 0) == std::string::npos)
    //sRealName = m_sName + "_" + sRealName;

    CogeSprite* pSprite = FindDeactiveSprite(sSpriteName);
    if(pSprite)
    {
        //pSprite->SetPos(iPosX, iPosY, iPosZ);
        //pSprite->m_pCurrentScene = NULL;

        if(pSprite->m_pActiveUpdateScript) pSprite->m_pActiveUpdateScript->Stop();
		if(pSprite->IsPlayingPlot()) pSprite->DisablePlot();

        pSprite->m_pCurrentScene = this;
        pSprite->m_bActive = false;
        pSprite->m_bBusy = false;
        pSprite->m_bEnableTimer = false;
        pSprite->m_pMasterSpr = NULL;
        pSprite->m_pReplacedSpr = NULL;
        //pSprite->SetPos(iPosX, iPosY, iPosZ);

        //pSprite->AbortPath();

        pSprite->m_iLastDisplayFlag = 0;

        pSprite->ClearRunningCustomEvents();

        if(pSprite->m_pTextInputter && m_pEngine->m_pIM)
        {
            CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
            if(pTextInputter == pSprite->m_pTextInputter) m_pEngine->DisableIM();
        }

        //pSprite->CallEvent(Event_OnDeactive);
        m_Deactivating.push_back(pSprite);

        int iCount = pSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pSprite->m_Children.begin();
            while(iCount > 0)
            {
                it->second->SetActive(false);
                it++;
                iCount--;
            }
        }

        return 0;
    }
    else
    {
        pSprite = FindActiveSprite(sSpriteName);
        if(pSprite)
        {
            //pSprite->m_pCurrentScene = NULL;
            m_ActiveSprites.erase(sSpriteName);
            m_DeactiveSprites.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));
            //pSprite->SetPos(iPosX, iPosY, iPosZ);

            if(pSprite->m_pActiveUpdateScript) pSprite->m_pActiveUpdateScript->Stop();
            if(pSprite->IsPlayingPlot()) pSprite->DisablePlot();

            pSprite->m_pCurrentScene = this;
            pSprite->m_bActive = false;
            pSprite->m_bBusy = false;
            pSprite->m_bEnableTimer = false;
            pSprite->m_pMasterSpr = NULL;
            pSprite->m_pReplacedSpr = NULL;
            //pSprite->SetPos(iPosX, iPosY, iPosZ);

            //pSprite->AbortPath();

            pSprite->m_iLastDisplayFlag = 0;

            pSprite->ClearRunningCustomEvents();

            if(pSprite->m_pTextInputter && m_pEngine->m_pIM)
            {
                CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
                if(pTextInputter == pSprite->m_pTextInputter) m_pEngine->DisableIM();
            }

            //pSprite->CallEvent(Event_OnDeactive);
            m_Deactivating.push_back(pSprite);

            int iCount = pSprite->m_Children.size();
            if(iCount > 0)
            {
                ogeSpriteMap::iterator it = pSprite->m_Children.begin();
                while(iCount > 0)
                {
                    it->second->SetActive(false);
                    it++;
                    iCount--;
                }
            }

            return 1;
        }
        else
        {
            pSprite = FindSprite(sSpriteName);
            if(pSprite)
            {
                //pSprite->m_pCurrentScene = NULL;
                m_DeactiveSprites.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));
                //pSprite->SetPos(iPosX, iPosY, iPosZ);

                if(pSprite->m_pActiveUpdateScript) pSprite->m_pActiveUpdateScript->Stop();
                if(pSprite->IsPlayingPlot()) pSprite->DisablePlot();

                pSprite->m_pCurrentScene = this;
                pSprite->m_bActive = false;
                pSprite->m_bBusy = false;
                pSprite->m_bEnableTimer = false;
                pSprite->m_pMasterSpr = NULL;
                pSprite->m_pReplacedSpr = NULL;
                //pSprite->SetPos(iPosX, iPosY, iPosZ);

                //pSprite->AbortPath();

                pSprite->m_iLastDisplayFlag = 0;

                pSprite->ClearRunningCustomEvents();

                if(pSprite->m_pTextInputter && m_pEngine->m_pIM)
                {
                    CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
                    if(pTextInputter == pSprite->m_pTextInputter) m_pEngine->DisableIM();
                }

                //pSprite->CallEvent(Event_OnDeactive);
                m_Deactivating.push_back(pSprite);

                int iCount = pSprite->m_Children.size();
                if(iCount > 0)
                {
                    ogeSpriteMap::iterator it = pSprite->m_Children.begin();
                    while(iCount > 0)
                    {
                        it->second->SetActive(false);
                        it++;
                        iCount--;
                    }
                }

                return 2;
            }
            else return -1;
        }

    }
}

int CogeScene::PrepareActivateSprite(CogeSprite* pSprite)
{
    if(pSprite == NULL) return -1;
    if(pSprite == m_pFirstSpr || pSprite == m_pLastSpr) return 0;

    ogeSpriteList::iterator it;
    int count = 0;

    // check it in deactive list ...
    /*
    count = m_PrepareDeactive.size();
	it = m_PrepareDeactive.begin();
	while(count > 0)
	{
	    if(pSprite == *it)
	    {
	        m_PrepareDeactive.erase(it);
	        break;
	    }
	    it++;
	    count--;
    }
    */

    //check active list ...
	count = m_PrepareActive.size();
	it = m_PrepareActive.begin();
	while(count > 0)
	{
	    if(pSprite == *it) break;
	    it++; count--;
    }
    if(count > 0) return 0;
    else
    {
        m_PrepareDeactive.remove(pSprite);
        m_PrepareActive.push_back(pSprite);
        return m_PrepareActive.size();
    }

}
int CogeScene::PrepareDeactivateSprite(CogeSprite* pSprite)
{
    if(pSprite == NULL) return -1;
    if(pSprite == m_pFirstSpr || pSprite == m_pLastSpr) return 0;

    ogeSpriteList::iterator it;
    int count = 0;

    //check active list ...
    /*
    count = m_PrepareActive.size();
	it = m_PrepareActive.begin();
	while(count > 0)
	{
	    if(pSprite == *it)
	    {
	        m_PrepareActive.erase(it);
	    }
	    it++;
	    count--;
    }
    */

    //check deactive list ...
	count = m_PrepareDeactive.size();
	it = m_PrepareDeactive.begin();
	while(count > 0)
	{
	    if(pSprite == *it) break;
	    it++; count--;
    }

    if(count > 0) return 0;
    else
    {
        m_PrepareActive.remove(pSprite);
        m_PrepareDeactive.push_back(pSprite);
        return m_PrepareDeactive.size();
    }
}

CogeSprite* CogeScene::GetCurrentSprite()
{
    return m_pCurrentSpr;
}

CogeSprite* CogeScene::GetFocusSprite()
{
    return m_pFocusSpr;
}

void CogeScene::SetFocusSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if (pSprite->m_pCurrentScene != this) return;
        if (pSprite->m_bEnableFocus  != true) return;

        CogeTextInputter* pTextInputter = pSprite->GetTextInputter();
        if(!pTextInputter) m_pEngine->DisableIM();

        if(m_pFocusSpr && m_pFocusSpr != pSprite)
        {
            //m_pCurrentSpr = m_pFocusSpr;
            //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
            //m_pFocusSpr->CallEvent(Event_OnLoseFocus);

            m_pLosingFocusSpr = m_pFocusSpr;
        }

        if(pTextInputter) pTextInputter->Focus();

        if(m_pFocusSpr != pSprite)
        {
            m_pFocusSpr = pSprite;

            //m_pCurrentSpr = pSprite;
            //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
            //pSprite->CallEvent(Event_OnGetFocus); // will not exec if it's called by current sprite's script ...

            m_pGettingFocusSpr = m_pFocusSpr;
        }

        if(m_pFocusSpr) BringSpriteToTop(m_pFocusSpr);
    }
    else
    {
        if(m_pFocusSpr)
        {
            CogeTextInputter* pTextInputter = m_pFocusSpr->GetTextInputter();
            if(pTextInputter) m_pEngine->DisableIM();

            //m_pCurrentSpr = m_pFocusSpr;
            //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
            //m_pFocusSpr->CallEvent(Event_OnLoseFocus);

            m_pLosingFocusSpr = m_pFocusSpr;
        }

        m_pFocusSpr = NULL;
    }
}

CogeSprite* CogeScene::GetModalSprite()
{
    return m_pModalSpr;
}
void CogeScene::SetModalSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if(pSprite->m_pCurrentScene != this) return;
        m_pModalSpr = pSprite;
    }
    else
    {
        m_pModalSpr = NULL;
    }

}

CogeSprite* CogeScene::GetMouseSprite()
{
    return m_pMouseSpr;
}
void CogeScene::SetMouseSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if(pSprite->m_pCurrentScene != this) return;
        pSprite->SetActive(false);
        m_pMouseSpr = pSprite;
    }
    else
    {
        m_pMouseSpr = NULL;
    }
}


CogeSprite* CogeScene::GetFirstSprite()
{
    return m_pFirstSpr;
}
void CogeScene::SetFirstSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if(pSprite->m_pCurrentScene != this) return;
        pSprite->SetActive(false);
        m_pFirstSpr = pSprite;
        m_Activating.remove(m_pFirstSpr);
    }
    else
    {
        m_pFirstSpr = NULL;
    }
}

CogeSprite* CogeScene::GetLastSprite()
{
    return m_pLastSpr;
}
void CogeScene::SetLastSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if(pSprite->m_pCurrentScene != this) return;
        pSprite->SetActive(false);
        m_pLastSpr = pSprite;
        m_Activating.remove(m_pLastSpr);
    }
    else
    {
        m_pLastSpr = NULL;
    }
}


int CogeScene::GetTopZ()
{
    return m_iTopZ;
}
int CogeScene::BringSpriteToTop(CogeSprite* pSprite)
{
    if(pSprite == NULL) return m_iTopZ;

    if(pSprite->m_pCurrentScene == this && m_iTopZ > pSprite->m_iPosZ)
    {
        pSprite->UpdatePosZ(m_iTopZ + 1);
    }

    return pSprite->m_iPosZ;
}

int CogeScene::GetSpriteCount(int iState)
{
    if(iState == 0) return m_SpriteMap.size();
    else if(iState > 0) return m_ActiveSprites.size();
    else return m_DeactiveSprites.size();
}
CogeSprite* CogeScene::GetSpriteByIndex(int iIndex, int iState)
{
    CogeSprite* pSprite = NULL;

    int iCount = m_SpriteMap.size();
    ogeSpriteMap::iterator it = m_SpriteMap.begin();
    ogeSpriteMap::iterator ite = m_SpriteMap.end();

    if(iState > 0)
    {
        iCount = m_ActiveSprites.size();
        it = m_ActiveSprites.begin();
        ite = m_ActiveSprites.end();
    }
    else if(iState < 0)
    {
        iCount = m_DeactiveSprites.size();
        it = m_DeactiveSprites.begin();
        ite = m_DeactiveSprites.end();
    }


    for(int i=0; i<iCount; i++)
    {
        if(it == ite) break;
        if(iIndex == i)
        {
            pSprite = it->second;
            break;
        }
        it++;
    }

    return pSprite;
}

CogeSprite* CogeScene::GetDefaultPlayerSprite()
{
    return m_pPlayerSpr;
}
void CogeScene::SetDefaultPlayerSprite(CogeSprite* pSprite)
{
    if(pSprite)
    {
        if(pSprite->m_pCurrentScene != this) return;
        m_pPlayerSpr = pSprite;
    }
    else
    {
        m_pPlayerSpr = NULL;
    }
}

CogeSprite* CogeScene::GetPlayerSprite(int iPlayerId)
{
    if(iPlayerId < 0 || iPlayerId >= _OGE_MAX_PLAYER_COUNT_) return NULL;
    else return m_Players[iPlayerId];
}
void CogeScene::SetPlayerSprite(CogeSprite* pSprite, int iPlayerId)
{
    if(iPlayerId < 0 || iPlayerId >= _OGE_MAX_PLAYER_COUNT_) return;
    else m_Players[iPlayerId] = pSprite;
}

CogeSprite* CogeScene::GetNpcSprite(size_t iNpcId)
{
    if(iNpcId >= m_Npcs.size()) return NULL;
    else return m_Npcs[iNpcId];
}
/*
void CogeScene::SetNpcSprite(CogeSprite* pSprite, int iNpcId)
{
    if(iNpcId < 0 || iNpcId >= _OGE_MAX_NPC_COUNT_) return;
    else m_Npcs[iNpcId] = pSprite;
}
*/

CogeSprite* CogeScene::GetPlotSprite()
{
    return m_pPlotSpr;
}
void CogeScene::SetPlotSprite(CogeSprite* pSprite)
{
    if(pSprite && pSprite->GetType() == Spr_Plot && pSprite->GetCurrentScene() == this)
    {
        m_pPlotSpr = pSprite;
    }
}

void CogeScene::Pause()
{
    if(m_iState < 0) return;
    m_iState = 1;
    m_iTimePoint = SDL_GetTicks();
}
void CogeScene::Resume()
{
    if(m_iState < 0) return;
    m_iState = 0;
    m_iPauseTime = m_iPauseTime + (SDL_GetTicks() - m_iTimePoint);
}
bool CogeScene::IsPlaying()
{
    return m_iState == 0;
}

int CogeScene::GetActiveTime()
{
    return m_iCurrentTime;
}

int CogeScene::Initialize(const std::string& sConfigFileName)
{
    Finalize();

    if(m_pEngine == NULL) return -1;
    if(m_pEngine->GetVideo() == NULL) return -1;

    m_SceneViewRect.left   = 0;
    m_SceneViewRect.top    = 0;
    m_SceneViewRect.right  = m_SceneViewRect.left + m_pEngine->GetVideo()->GetWidth();
    m_SceneViewRect.bottom = m_SceneViewRect.top  + m_pEngine->GetVideo()->GetHeight();

    //bool bLoadedConfig = m_IniFile.Load(sConfigFileName);
    bool bLoadedConfig = m_pEngine->LoadIniFile(&m_IniFile, sConfigFileName, m_pEngine->m_bPackedScene, m_pEngine->m_bEncryptedScene);

    if (bLoadedConfig)
    {
        m_IniFile.SetCurrentPath(OGE_GetAppGameDir());

        // section scene ...

        std::string sBgColor  = m_IniFile.ReadString("Scene", "BgColor", "0");
        int iBgColor = m_pEngine->m_pVideo->StringToColor(sBgColor);
        SetBgColor(iBgColor);

        std::string sMapName  = m_IniFile.ReadString("Scene", "Map", "");
        //if(sMapName.length() <= 0 || SetMap(sMapName) < 0) return -1;
        if(sMapName.length() > 0 ) SetMap(sMapName);

        std::string sChapterName  = m_IniFile.ReadString("Scene", "Chapter", "");
        SetChapterName(sChapterName);

        std::string sTagName  = m_IniFile.ReadString("Scene", "Tag", "");
        SetTagName(sTagName);

        ogeIntStrMap SpriteParentMap;

        int iSpriteCount = m_IniFile.ReadInteger("Sprites", "Count",  0);

        std::string sConfigGroup = "S_" + m_sName;

        if(m_pEngine->m_bBinaryScript == false && iSpriteCount > 0)
        {
            m_pEngine->m_pScripter->BeginConfigGroup(sConfigGroup);

            m_pEngine->m_pScripter->RegisterEnum(m_sName);

            for(int k=1; k<=iSpriteCount; k++)
            {
                std::string sIdx = "Sprite_" + OGE_itoa(k);
                std::string sSpriteName = m_IniFile.ReadString(sIdx, "Name", "");
                if(sSpriteName.length() > 0)
                    m_pEngine->m_pScripter->RegisterEnumValue(m_sName, sSpriteName, k);
            }

            //m_pEngine->m_pScripter->EndConfigGroup();

        }


        // section script ...
        if(m_pEngine->m_pScripter && m_pEngine->m_iScriptState >= 0)
        {
            //std::string sScriptFile = "";
            std::string sScriptName = m_IniFile.ReadString("Script", "Name", "");


            CogeScript* pScript = NULL;

            //if(sScriptName.length() > 0) pScript = m_pEngine->GetScript(sScriptName);
            if(sScriptName.length() > 0) pScript = NewScript(sScriptName);

            if(pScript && pScript->GetState() >= 0)
            {
                m_pScript = pScript;
                m_pScript->Hire();

                std::map<std::string, int>::iterator it = m_pEngine->m_EventIdMap.begin(); //, itb, ite;

                int iCount = m_pEngine->m_EventIdMap.size();

                int iEventId  = -1;
                std::string sEventFunctionName = "";

                //for(it=itb; it!=ite; it++)
                while(iCount > 0)
                {
                    iEventId  = -1;
                    sEventFunctionName = m_IniFile.ReadString("Script", it->first, "");
                    if(sEventFunctionName.length() > 0)
                    {
                        iEventId  = m_pScript->PrepareFunction(sEventFunctionName);
                        //if(iEventId >= 0) m_Events[it->second] = iEventId;
                    }

                    m_Events[it->second] = iEventId;

                    it++;

                    iCount--;
                }

                m_iScriptState = 0;

            }
        }

        // action section
        if(m_pEngine->m_pVideo)
        {
            m_iFadeInType  = m_IniFile.ReadInteger("Action", "FadeIn",   0);
            m_iFadeOutType = m_IniFile.ReadInteger("Action", "FadeOut",  0);
            m_iFadeInterval= m_IniFile.ReadInteger("Action", "FadeInterval",  0);
            //m_bAutoFade    = m_IniFile.ReadInteger("Action", "AutoFade", 1) == 1;
            m_bAutoFade    = true; // always use auto-fade mode so that we can ensure OnOpen/OnClose event could be executed ...

            std::string sFadeMaskName = m_IniFile.ReadString("Action", "FadeMask", "");
            if(m_iFadeInType == Fade_Mask && sFadeMaskName.length() > 0)
            {
                CogeImage* pMaskImage = m_pEngine->GetImage(sFadeMaskName);
                SetFadeMask(pMaskImage);
            }
        }

        m_iFadeTime = 0;

        // mask section
        if(m_pEngine->m_pVideo)
        {
            m_bEnableSpriteLight  = m_IniFile.ReadInteger("Mask", "EnableSpriteLight",  0) == 1;
            m_iLightMode          = m_IniFile.ReadInteger("Mask", "LightMode",  0);

            std::string sMaskName = m_IniFile.ReadString("Mask", "LightMaskImage", "");

            if(sMaskName.length() > 0)
            {
                CogeImage* pImage = m_pEngine->GetImage(sMaskName);
                SetLightMap(pImage, m_iLightMode);
            }
            else m_iLightMode = 0;

        }

        // audio section ...
        if(m_pEngine->m_pAudio)
        {
            std::string sBgMusicName = m_IniFile.ReadString("Scene", "BgMusic", "");
            if(sBgMusicName.length() > 0) SetBgMusic(sBgMusicName);
        }

        // section data ...

        m_bHasDefaultData = m_IniFile.ReadInteger("Data", "Default", 0) != 0;

        if(m_bHasDefaultData)
        {
            //std::string sDataName = m_sName;
            int iIntCount  = m_IniFile.ReadInteger("Data", "IntVarCount", 0);
            int iFloatCount  = m_IniFile.ReadInteger("Data", "FloatVarCount", 0);
            int iStrCount  = m_IniFile.ReadInteger("Data", "StrVarCount", 0);

            CogeGameData* pGameData = m_pEngine->NewFixedGameData(m_sName, iIntCount, iFloatCount, iStrCount);

            if(pGameData)
            {
                std::string sKeyName = "";
                std::string sValue = "";
                int iValue = 0;
                double fValue = 0;

                int iIntCount = pGameData->GetIntVarCount();
                for(int i=1; i<=iIntCount; i++)
                {
                    sKeyName = "IntVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        iValue = m_IniFile.ReadInteger("Data", sKeyName, 0);
                        pGameData->SetIntVar(i, iValue);
                    }
                }

                int iFloatCount = pGameData->GetFloatVarCount();
                for(int i=1; i<=iFloatCount; i++)
                {
                    sKeyName = "FloatVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        fValue = m_IniFile.ReadFloat("Data", sKeyName, 0);
                        pGameData->SetFloatVar(i, fValue);
                    }
                }

                int iStrCount = pGameData->GetStrVarCount();
                for(int i=1; i<=iStrCount; i++)
                {
                    sKeyName = "StrVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        pGameData->SetStrVar(i, sValue);
                    }
                }

                BindGameData(pGameData);
            }
        }


        // section sprite ...

        for(int k=1; k<=iSpriteCount; k++)
        {

            std::string sIdx = "Sprite_" + OGE_itoa(k);
            std::string sSpriteName = m_IniFile.ReadString(sIdx, "Name", "");

            std::string sBaseSprite = m_IniFile.ReadString(sIdx, "Base", "");

            if(sBaseSprite.length() <= 0) continue;

            CogeSprite* pTheSprite = NULL;

            pTheSprite = AddSprite(sSpriteName, sBaseSprite);

            if (pTheSprite == NULL) continue;
            //else SetNpcSprite(pTheSprite, k-1);

            m_Npcs.push_back(pTheSprite);
            //SetNpcSprite(pTheSprite, k-1);

            sSpriteName = pTheSprite->GetName();

            std::string sLocalScript = m_IniFile.ReadString(sIdx, "Script", "");
            if(sLocalScript.length() > 0) pTheSprite->LoadLocalScript(&m_IniFile, sIdx);

            // the new image of the sprite
            std::string sNewImage = m_IniFile.ReadString(sIdx, "AltImage", "");
            if(sNewImage.length() > 0)
            {
                CogeImage* pNewImage = m_pEngine->GetImage(sNewImage);
                if(pNewImage) pTheSprite->SetImage(pNewImage);
            }

            // and set status of the sprite

            int iDir     = m_IniFile.ReadInteger(sIdx, "Dir",   Dir_Down);
            int iAct     = m_IniFile.ReadInteger(sIdx, "Act",   Act_Stand);

            pTheSprite->SetAnima(iDir, iAct);

            int iSpeedX  = m_IniFile.ReadInteger(sIdx, "SpeedX", -1);
            int iSpeedY  = m_IniFile.ReadInteger(sIdx, "SpeedY", -1);

            int iTimerInterval = m_IniFile.ReadInteger(sIdx, "TimerInterval", -1);
            if(iTimerInterval > 0)
            {
                if(pTheSprite->m_iUnitType == Spr_Timer)
                    pTheSprite->m_iDefaultTimerInterval = iTimerInterval;
            }

            int iObjectTag = m_IniFile.ReadInteger(sIdx, "Tag", -1);
            if(iObjectTag > 0) pTheSprite->SetObjectTag(iObjectTag);

            if( iSpeedX >= 0 && iSpeedY >= 0 ) pTheSprite->SetSpeed(iSpeedX, iSpeedY);

            int iActive  = m_IniFile.ReadInteger(sIdx, "Active",  0);
            int iPosX    = m_IniFile.ReadInteger(sIdx, "PosX",    0);
            int iPosY    = m_IniFile.ReadInteger(sIdx, "PosY",    0);
            int iPosZ    = m_IniFile.ReadInteger(sIdx, "PosZ",    0);

            if(iActive > 0) ActivateSprite(sSpriteName);

            pTheSprite->SetPos(iPosX, iPosY, iPosZ);

            pTheSprite->m_iRelativeX  = m_IniFile.ReadInteger(sIdx, "RelativeX", 0);
            pTheSprite->m_iRelativeY  = m_IniFile.ReadInteger(sIdx, "RelativeY", 0);

            int iRelative = m_IniFile.ReadInteger(sIdx, "Relative",   -1);
            if(iRelative >= 0) pTheSprite->m_bIsRelative = iRelative > 0;

            int iVisible = m_IniFile.ReadInteger(sIdx, "Visible",   -1);
            if(iVisible >= 0) pTheSprite->m_bVisible = iVisible > 0;

            int iEnableAnima = m_IniFile.ReadInteger(sIdx, "EnableAnima", -1);
            if(iEnableAnima >= 0) pTheSprite->m_bEnableAnima = iEnableAnima > 0;

            int iEnableMovement = m_IniFile.ReadInteger(sIdx, "EnableMovement", -1);
            if(iEnableMovement >= 0) pTheSprite->m_bEnableMovement = iEnableMovement > 0;

            int iEnableCollision = m_IniFile.ReadInteger(sIdx, "EnableCollision", -1);
            if(iEnableCollision >= 0) pTheSprite->m_bEnableCollision = iEnableCollision > 0;

            int iEnableInput = m_IniFile.ReadInteger(sIdx, "EnableInput", -1);
            if(iEnableInput >= 0) pTheSprite->m_bEnableInput = iEnableInput > 0;

            int iEnableFocus = m_IniFile.ReadInteger(sIdx, "EnableFocus", -1);
            if(iEnableFocus >= 0) pTheSprite->m_bEnableFocus = iEnableFocus > 0;

            int iEnableDrag = m_IniFile.ReadInteger(sIdx, "EnableDrag", -1);
            if(iEnableDrag >= 0) pTheSprite->m_bEnableDrag = iEnableDrag > 0;

            //  reset game data ...
            CogeGameData* pGameData = pTheSprite->GetGameData();

            if(pGameData)
            {
                std::string sKeyName = "";
                std::string sValue = "";
                int iValue = 0;
                double fValue = 0;

                int iIntCount = pGameData->GetIntVarCount();
                for(int i=1; i<=iIntCount; i++)
                {
                    sKeyName = "IntVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString(sIdx, sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        iValue = m_IniFile.ReadInteger(sIdx, sKeyName, 0);
                        pGameData->SetIntVar(i, iValue);
                    }
                }

                int iFloatCount = pGameData->GetFloatVarCount();
                for(int i=1; i<=iFloatCount; i++)
                {
                    sKeyName = "FloatVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString(sIdx, sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        fValue = m_IniFile.ReadFloat(sIdx, sKeyName, 0);
                        pGameData->SetFloatVar(i, fValue);
                    }
                }

                int iStrCount = pGameData->GetStrVarCount();
                for(int i=1; i<=iStrCount; i++)
                {
                    /*
                    sKeyName = "StrVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString(sIdx, sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        pGameData->SetStrVar(i, sValue);
                    }
                    */

                    sKeyName = "StrVar" + OGE_itoa(i);
                    sValue = m_IniFile.ReadString(sIdx, sKeyName, "\n");
                    if(sValue.length() == 1 && sValue[0] == '\n') continue;
                    else
                    {
                        pGameData->SetStrVar(i, sValue);
                    }
                }

            }

            // bind parent in scene ...
            std::string sSpriteParentName = m_IniFile.ReadString(sIdx, "Parent", "");
            if(sSpriteParentName.length() > 0)
            {
                int iSprId = (int) pTheSprite;
                SpriteParentMap.insert(ogeIntStrMap::value_type(iSprId, sSpriteParentName));
            }



        }


        if(m_pEngine->m_bBinaryScript == false && iSpriteCount > 0)
        {
            m_pEngine->m_pScripter->EndConfigGroup();
            m_pEngine->m_pScripter->RemoveConfigGroup(sConfigGroup);
        }


        // bind parent in scene ...

        iSpriteCount = SpriteParentMap.size();

        if(iSpriteCount > 0)
        {
            //CogeSprite* pMatchedSprite = NULL;
            ogeIntStrMap::iterator its = SpriteParentMap.begin();

            while (iSpriteCount > 0)
            {
                CogeSprite* pMatchedSprite = (CogeSprite*) its->first;
                std::string sSpriteParentName = its->second;

                pMatchedSprite->SetParent(sSpriteParentName, pMatchedSprite->m_iRelativeX, pMatchedSprite->m_iRelativeY);

                its++;

                iSpriteCount--;

            }
        }

        SpriteParentMap.clear();


        // section groups ...

        int iGroupCount = m_IniFile.ReadInteger("Groups", "Count",  0);
        for(int i=1; i<=iGroupCount; i++)
        {
            std::string sIdx = "Group" + OGE_itoa(i);
            std::string sGroupName = m_IniFile.ReadString("Groups", sIdx, "");

            if(sGroupName.length() > 0)
            {
                CogeSpriteGroup* pGroup = m_pEngine->GetGroup(sGroupName);
                if(pGroup) m_Groups.push_back(pGroup);
            }

        }

        // first sprite & last sprite ...
        std::string sFirstSprName  = m_IniFile.ReadString("Scene", "FirstSprite", "");
        if(sFirstSprName.length() > 0 )
        {
            CogeSprite* pTheSprite = FindSprite(sFirstSprName);
            if(pTheSprite) SetFirstSprite(pTheSprite);
        }
        std::string sLastSprName  = m_IniFile.ReadString("Scene", "LastSprite", "");
        if(sLastSprName.length() > 0 )
        {
            CogeSprite* pTheSprite = FindSprite(sLastSprName);
            if(pTheSprite) SetLastSprite(pTheSprite);
        }


        // when all is ok ...

        m_iState = 0;

        if(m_pEngine->m_pActiveScene) m_pEngine->m_InitScenes.insert(ogeSceneMap::value_type(m_sName, this));
        else
        {
            CogeScene* pCurrentScene = m_pEngine->m_pCurrentGameScene;
            m_pEngine->m_pCurrentGameScene = this;

            CallEvent(Event_OnInit); // not work when create in script ...

            m_pEngine->m_pCurrentGameScene = pCurrentScene;
        }

        if(m_pEngine->m_pActiveScene == NULL)
        {
            CogeSprite* pCurrentSpr = m_pEngine->m_pCurrentGameSprite;
            if(m_pEngine->m_InitSprites.size() > 0)
            {
                for(ogeSpriteMap::iterator iti = m_pEngine->m_InitSprites.begin(); iti != m_pEngine->m_InitSprites.end(); iti++)
                {
                    m_pEngine->m_pCurrentGameSprite = iti->second;
                    iti->second->CallEvent(Event_OnInit);
                }
                m_pEngine->m_InitSprites.clear();
            }
            m_pEngine->m_pCurrentGameSprite = pCurrentSpr;
        }


    }
    else
    {

        //printf("Load Sprite Config File Failed.\n");
        //if (m_iState >= 0) DoInit(this);
    }

    if(m_iState < 0 ) OGE_Log("Load Scene Config File Failed.\n");
    if(m_iScriptState < 0) OGE_Log("Load  Scene Script Failed.\n");

    return m_iState;
}

void CogeScene::Finalize()
{
    m_iState = -1;

    if (!m_pEngine) return;

    if(m_pLightMap)
    {
        m_pLightMap->Fire();
        //m_pEngine->m_pVideo->DelImage(m_pLightMap->GetName());
        m_pEngine->m_pVideo->DelImage(m_pLightMap);
    }
    m_pLightMap = NULL;

    m_pFadeMask = NULL;

    CogeImage* pScreen = m_pEngine->m_pVideo->GetScreen();

    if (pScreen == m_pScreen)
    {
        m_pEngine->m_pVideo->DefaultScreen();
    }

    if (m_pScreen)
    {
        if(m_pScreen != m_pEngine->m_pVideo->GetDefaultScreen())
        {
            m_pScreen->Fire();
            //m_pEngine->m_pVideo->DelImage(m_pScreen->GetName());
            m_pEngine->m_pVideo->DelImage(m_pScreen);
        }

        m_pScreen = NULL;
    }

    if(m_pMap)
    {
        m_pMap->Fire();
        //m_pEngine->DelGameMap(m_pMap->m_sName);
        m_pEngine->DelGameMap(m_pMap);
        m_pMap = NULL;
    }
    else if(m_pBackground)
    {
        m_pBackground->Fire();
        //m_pEngine->m_pVideo->DelImage(m_pBackground->GetName());
        m_pEngine->m_pVideo->DelImage(m_pBackground);
    }

    m_pBackground = NULL;

    if (m_pScript)
    {
        //m_pScript->Fire();
        //m_pEngine->m_pScripter->DelScript(m_pScript->GetName());

        m_pEngine->m_pScripter->DelScript(m_pScript);

        m_pScript = NULL;
    }

    //m_EventMap.clear();

    ClearDirtyRects();

    m_SpritesInView.clear();
    //m_SpriteMap.clear();

    m_Activating.clear();
    m_Deactivating.clear();

    m_PrepareActive.clear();
    m_PrepareDeactive.clear();

    m_ActiveSprites.clear();
    m_DeactiveSprites.clear();

    m_Groups.clear();

    m_Npcs.clear();

    ogeSpriteMap::iterator its;
	CogeSprite* pMatchedSprite = NULL;

	its = m_SpriteMap.begin();

	while (its != m_SpriteMap.end())
	{
		pMatchedSprite = its->second;
		pMatchedSprite->m_pCurrentScene = NULL;
		m_SpriteMap.erase(its);
		if(pMatchedSprite->m_pCurrentGroup == NULL) m_pEngine->DelSprite(pMatchedSprite);
		its = m_SpriteMap.begin();
	}


	m_pBackgroundMusic = NULL;

	//CogeGameData* pData = GetGameData();
	//RemoveGameData();
	//if(pData) m_pEngine->DelGameData(pData->GetName()); // normally we may delete it ...

	if(m_bHasDefaultData && m_pGameData)
    {
        //std::string sDataName = m_sName + "_Data";
        //m_pEngine->DelGameData(sDataName);
        m_pEngine->DelGameData(m_pGameData);
        m_pGameData = NULL;
    }

    if(m_bOwnCustomData && m_pCustomData)
    {
        std::string sDataName = m_pCustomData->GetName();
        m_pEngine->DelGameData(sDataName);
    }

    m_pCustomData = NULL;

}

const std::string& CogeScene::GetName()
{
    return m_sName;
}

const std::string& CogeScene::GetChapterName()
{
    return m_sChapterName;
}

void CogeScene::SetChapterName(const std::string& sChapterName)
{
    m_sChapterName = sChapterName;
}

const std::string& CogeScene::GetTagName()
{
    return m_sTagName;
}
void CogeScene::SetTagName(const std::string& sTagName)
{
    m_sTagName = sTagName;
}

CogeGameData* CogeScene::GetGameData()
{
    return m_pGameData;
}

CogeGameData* CogeScene::GetCustomData()
{
    return m_pCustomData;
}

void CogeScene::SetCustomData(CogeGameData* pGameData, bool bOwnIt)
{
    m_pCustomData = pGameData;
    if(m_pCustomData && bOwnIt) m_bOwnCustomData = true;
    else m_bOwnCustomData = false;
}

int CogeScene::BindGameData(CogeGameData* pGameData)
{
    if(pGameData == NULL)
    {
        RemoveGameData();
        return 0;
    }

    if(pGameData != m_pGameData)
    {
        if(m_pGameData) RemoveGameData();
        m_pGameData = pGameData;
        //pGameData->Hire();
        return 1;
    }
    else return 0;

}

void CogeScene::RemoveGameData()
{
    //if(m_pGameData) m_pGameData->Fire();
    m_pGameData = NULL;
}

bool CogeScene::IsDirtyRectMode()
{
    return m_pEngine->m_bUseDirtyRect;
}

int CogeScene::AddDirtyRect(CogeRect* rc)
{
    if(!m_pEngine->m_bUseDirtyRect) return -1;
    if(m_iTotalDirtyRects >= _OGE_MAX_DIRTY_RECT_NUMBER_) return 0;
    m_DirtyRects.push_back(rc);
    return ++m_iTotalDirtyRects;

}

int CogeScene::AutoAddDirtyRect(CogeSprite* pSprite)
{
    if (m_pEngine->m_bUseDirtyRect && pSprite)
    {
        if (OverlapRect(pSprite->m_DrawPosRect, pSprite->m_OldPosRect))
        {
            pSprite->UpdateDirtyRect();
            return AddDirtyRect(&pSprite->m_DirtyRect);
        }
        else
        {
            AddDirtyRect(&pSprite->m_DrawPosRect);
            return AddDirtyRect(&pSprite->m_OldPosRect);
        }
    }
    else return -1;
}

int CogeScene::Prepare()
{
    return 0;
}

int CogeScene::PlayBgMusic(int iLoopTimes)
{
    if(m_pEngine->m_pAudio && m_pBackgroundMusic)
    {
        CogeMusic* pCurrentMusic = m_pEngine->m_pAudio->GetCurrentMusic();
        if(m_pBackgroundMusic == pCurrentMusic) return m_pBackgroundMusic->Play(iLoopTimes);
        else
        {
            m_pEngine->m_pAudio->PrepareMusic(m_pBackgroundMusic);
            return m_pBackgroundMusic->Play(iLoopTimes);
        }
    }
    else return -1;
}

const std::string& CogeScene::GetBgMusicName()
{
    if(m_pBackgroundMusic == NULL) return _OGE_EMPTY_STR_;
    else return m_pBackgroundMusic->GetName();
}

int CogeScene::SetBgMusic(const std::string& sMusicName)
{
    if(sMusicName.length() == 0)
    {
        m_pBackgroundMusic = NULL;
        return 0;
    }

    if(m_pEngine->m_pAudio == NULL) return -1;

    m_pBackgroundMusic = m_pEngine->GetMusic(sMusicName);

    if(m_pBackgroundMusic) return 1;
    else return 0;
}

int CogeScene::SetBgColor(int iRGBColor)
{
    m_iBackgroundColorRGB = iRGBColor & 0x00ffffff;
    m_iBackgroundColor = m_iBackgroundColorRGB;
    if(m_pEngine) m_iBackgroundColor = m_pEngine->GetVideo()->FormatColor(m_iBackgroundColorRGB);
    return m_iBackgroundColor;
}

void CogeScene::Activate()
{
    m_pEngine->m_pCurrentGameScene = this;
    m_pEngine->m_pNextActiveScene = NULL;

    //m_pEngine->m_sActiveSceneName = m_sName;
    m_pEngine->m_sNextSceneName = "";

    m_bMouseDown = false;
    m_bIsMouseEventHandled = false;

    m_iActiveTime = SDL_GetTicks();
    m_iCurrentTime = 0;
    m_iTimePoint = 0;
    m_iPauseTime = 0;

    m_pDraggingSpr = NULL;
    //m_pModalSpr = NULL;
    //m_pFocusSpr = NULL;

    m_pGettingFocusSpr = NULL;
    m_pLosingFocusSpr = NULL;

    //m_pFirstSpr = NULL;
    //m_pLastSpr = NULL;

    //m_pNextScene = NULL;
    //m_pSocketScene = NULL;

    //m_bNeedRedrawBg = false;

    //m_sNextSceneName = "";

    ClearDirtyRects();

    m_pEngine->ResetVMouseState();

    if(m_pBackgroundMusic) PlayBgMusic(-1);
    //else m_pEngine->StopMusic();

	int iGroupCount = m_Groups.size();
	for(int i=0; i<iGroupCount; i++)
    {
        CogeSpriteGroup* pGroup = m_Groups[i];
        if(pGroup) pGroup->SetCurrentScene(this);
    }

    if(m_pFirstSpr)
    {
        m_pCurrentSpr = m_pFirstSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->CallEvent(Event_OnActive);
    }

	CallEvent(Event_OnActive);

	if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(Event_OnActive);
    }

	//if(m_bAutoFade) FadeIn();

	if(m_pFirstSpr)
    {
        m_pCurrentSpr = m_pFirstSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->CallEvent(Event_OnSceneActive);
    }

    //bool bHasLastSpr = m_pLastSpr != NULL;

	ogeSpriteMap::iterator it, itb, ite;

	itb = m_SpriteMap.begin();
	ite = m_SpriteMap.end();

	for (it=itb; it!=ite; it++)
	{
		m_pCurrentSpr = it->second;
		if(m_pCurrentSpr == m_pFirstSpr || m_pCurrentSpr == m_pLastSpr) continue;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		m_pCurrentSpr->CallEvent(Event_OnSceneActive);
	}

	//if(bHasLastSpr && m_pLastSpr)
    if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(Event_OnSceneActive);
    }

	// check sprites' lives ..
    // this will always be the last step in every update cycle ...
    int iMaxCycles = 5;
    int iLeftCount = CheckSpriteLifeCycles();
    while(iLeftCount > 0 && iMaxCycles > 0)
    {
        iLeftCount = CheckSpriteLifeCycles();
        iMaxCycles--;
    }

	CheckSpriteCustomEvents(true);

	if(m_bAutoFade) FadeIn();

}
void CogeScene::Deactivate()
{
    CogeScene* pCurrentScene = m_pEngine->m_pCurrentGameScene;
    m_pEngine->m_pCurrentGameScene = this;

    //m_ActiveSprites.clear();
    //m_pNextScene = NULL;

    m_bEnableDefaultTimer = false;
    //m_iPlotTimerTriggerFlag = 0;

    //m_bNeedRedrawBg = false;

    if(m_pFirstSpr)
    {
        m_pCurrentSpr = m_pFirstSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->CallEvent(Event_OnSceneDeactive);
    }

    //bool bHasLastSpr = m_pLastSpr != NULL;

    ogeSpriteMap::iterator it, itb, ite;
	//CogeSprite* pMatchedSprite = NULL;

	itb = m_ActiveSprites.begin();
	ite = m_ActiveSprites.end();

	for (it=itb; it!=ite; it++)
	{
		//pMatchedSprite = it->second;
		m_pCurrentSpr = it->second;

		if(m_pCurrentSpr == m_pFirstSpr || m_pCurrentSpr == m_pLastSpr) continue;

		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		//if(m_pCurrentSpr->m_pActiveUpdateScript) m_pCurrentSpr->m_pActiveUpdateScript->Stop();
		if(m_pCurrentSpr->m_pActiveUpdateScript) m_pCurrentSpr->m_pActiveUpdateScript->Stop();
		if(m_pCurrentSpr->IsPlayingPlot()) m_pCurrentSpr->DisablePlot();
		m_pCurrentSpr->CallEvent(Event_OnSceneDeactive);
	}

	//if(bHasLastSpr && m_pLastSpr)
    if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(Event_OnSceneDeactive);
    }


	int iGroupCount = m_Groups.size();
	for(int i=0; i<iGroupCount; i++)
    {
        CogeSpriteGroup* pGroup = m_Groups[i];
        if(pGroup) pGroup->SetCurrentScene(NULL);
    }


    /*
    ogeGroupMap::iterator itg;
	CogeSpriteGroup* pMatchedGroup = NULL;

	itg = m_pEngine->m_GroupMap.begin();

	while (itg != m_pEngine->m_GroupMap.end())
	{
		pMatchedGroup = itg->second;

		if(pMatchedGroup->GetCurrentScene() == this)
            pMatchedGroup->SetCurrentScene(NULL);

        itg++;
	}
	*/

    if(m_pFirstSpr)
    {
        m_pCurrentSpr = m_pFirstSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->CallEvent(Event_OnDeactive);
    }

    CallEvent(Event_OnDeactive);

    if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(Event_OnDeactive);
    }



    // check sprites' lives ..
    // this will always be the last step in every update cycle ...
    int iMaxCycles = 5;
    int iLeftCount = CheckSpriteLifeCycles();
    while(iLeftCount > 0 && iMaxCycles > 0)
    {
        iLeftCount = CheckSpriteLifeCycles();
        iMaxCycles--;
    }

	CheckSpriteCustomEvents(true);


    itb = m_SpriteMap.begin();
	ite = m_SpriteMap.end();

	for (it=itb; it!=ite; it++)
	{
		//pMatchedSprite = it->second;
		//it->second->m_pCurrentScene = NULL;
		it->second->ClearRunningCustomEvents();
	}


	//if(m_pPlotSpr) ClosePlot();

	m_pPlotSpr = NULL;

	m_pDraggingSpr = NULL;
	//m_pModalSpr = NULL;
    //m_pFocusSpr = NULL;

    m_pGettingFocusSpr = NULL;
    m_pLosingFocusSpr = NULL;

    //m_pFirstSpr = NULL;
    //m_pLastSpr = NULL;

	m_bMouseDown = false;
    m_bIsMouseEventHandled = false;

    m_pEngine->ResetVMouseState();

    ClearDirtyRects();

	if(m_pScreen == m_pEngine->m_pVideo->GetDefaultScreen()) m_pScreen = NULL;
	if(m_pBackground == m_pEngine->m_pVideo->GetDefaultBg()) m_pBackground = NULL;

    m_pEngine->m_pCurrentGameScene = pCurrentScene;

}

int CogeScene::SetNextScene(CogeScene* pNextScene)
{
    if(pNextScene)
    {
        //m_pNextScene = pNextScene;
        m_pEngine->m_pNextActiveScene = pNextScene;
        if(m_bAutoFade) FadeOut();
        return 1;
    }
    else return -1;
}
int CogeScene::SetNextScene(const std::string& sSceneName)
{
    //m_sNextSceneName = sSceneName;
    m_pEngine->m_sNextSceneName = sSceneName;
    if(m_bAutoFade) FadeOut();
    else
    {
        CogeScene* pNextScene = m_pEngine->GetScene(sSceneName);
        //if(pNextScene) m_pNextScene = pNextScene;
        if(pNextScene) m_pEngine->m_pNextActiveScene = pNextScene;
        else m_pEngine->m_sNextSceneName = "";
    }
    return 1;

}

void CogeScene::FadeIn (int iFadeType, int iStartValue, int iEndValue, int iStep)
{
    m_iFadeState = 1;
    m_iFadeEffectValue = iEndValue;
    m_iFadeStep = iStep;
    m_iFadeEffectCurrentValue = iStartValue;
    m_iFadeType = iFadeType;

    m_iFadeEffectStartValue = m_iFadeEffectCurrentValue;

}
void CogeScene::FadeOut(int iFadeType, int iStartValue, int iEndValue, int iStep)
{
    m_iFadeState = -1;
    m_iFadeEffectValue = iEndValue;
    m_iFadeStep = iStep;
    m_iFadeEffectCurrentValue = iStartValue;
    m_iFadeType = iFadeType;

    m_iFadeEffectStartValue = m_iFadeEffectCurrentValue;

    //m_pNextScene = pNextScene;
}

void CogeScene::FadeIn (int iFadeType)
{
    if(m_iFadeState != 0) return;

    if(iFadeType < 0) iFadeType = m_iFadeInType;

    m_iFadeState = 1;
    m_iFadeType = iFadeType;

#if defined(__ANDROID__) || defined(__WINCE__) || defined(__IPHONE__)
    int fFrameSpeed = 8;
#else
    //float fFrameSpeed = 1;
    int fFrameSpeed = 1;
#endif

    int iCPS = m_pEngine->GetLockedCPS();
    if(iCPS > 0 && iCPS < 40) fFrameSpeed = 8;

    switch(m_iFadeType)
    {
    case Fade_None:
        m_iFadeEffectValue = 0;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = 1;
    break;
    case Fade_Lightness:
    case Fade_Dark:
        m_iFadeEffectValue = 0;
        m_iFadeEffectCurrentValue = -255;
        m_iFadeStep = (int)(2 * fFrameSpeed);
    break;
    case Fade_Bright:
        m_iFadeEffectValue = 0;
        m_iFadeEffectCurrentValue = 255;
        m_iFadeStep = (int)(-2 * fFrameSpeed);
    break;
    case Fade_Alpha:
        m_iFadeEffectValue = 0;
        m_iFadeEffectCurrentValue = 255;
        m_iFadeStep = (int)(-2 * fFrameSpeed);
    break;
    case Fade_Mask:
        m_iFadeEffectValue = 255;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = (int)(2 * fFrameSpeed);
    break;
    case Fade_RandomMask:
        m_pEngine->RandomMask();
        m_iFadeEffectValue = 255;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = (int)(2 * fFrameSpeed);
    break;

    }

    m_iFadeEffectStartValue = m_iFadeEffectCurrentValue;

}
void CogeScene::FadeOut(int iFadeType)
{
    if(m_iFadeState != 0) return;

    if(iFadeType < 0) iFadeType = m_iFadeOutType;

    m_iFadeState = -1;
    m_iFadeType = iFadeType;

#if defined(__ANDROID__) || defined(__WINCE__) || defined(__IPHONE__)
    int fFrameSpeed = 8;
#else
    //float fFrameSpeed = 1;
    int fFrameSpeed = 1;
#endif

    int iCPS = m_pEngine->GetLockedCPS();
    if(iCPS > 0 && iCPS < 40) fFrameSpeed = 8;

    switch(m_iFadeType)
    {
    case Fade_None:
        //m_iFadeEffectValue = 1;
        m_iFadeEffectValue = 0;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = 1;
    break;
    case Fade_Lightness:
    case Fade_Dark:
        m_iFadeEffectValue = -255;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = (int)(-2 * fFrameSpeed);
    break;
    case Fade_Bright:
        m_iFadeEffectValue = 255;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = (int)(2 * fFrameSpeed);
    break;
    case Fade_Alpha:
        m_iFadeEffectValue = 1;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = 1;
    break;
    case Fade_Mask:
    case Fade_RandomMask:
        m_iFadeEffectValue = 1;
        m_iFadeEffectCurrentValue = 0;
        m_iFadeStep = 1;
    break;

    }

    m_iFadeEffectStartValue = m_iFadeEffectCurrentValue;

    //m_pNextScene = pNextScene;
}

int CogeScene::GetFadeInSpeed()
{
    return m_iFadeStep;
}

int CogeScene::GetFadeOutSpeed()
{
    return m_iFadeStep;
}

int CogeScene::GetFadeInType()
{
    return m_iFadeType;
}

int CogeScene::GetFadeOutType()
{
    return m_iFadeType;
}

int CogeScene::SetFadeInType(int iType)
{
   FadeIn(iType);
   return m_iFadeType;
}
int CogeScene::SetFadeOutType(int iType)
{
    FadeOut(iType);
    return m_iFadeType;
}

int CogeScene::SetFadeInSpeed(int iSpeed)
{
    switch(m_iFadeType)
    {
    case Fade_None:
        m_iFadeStep = 1;
    break;
    case Fade_Lightness:
    case Fade_Dark:
        m_iFadeStep = iSpeed;
    break;
    case Fade_Bright:
        m_iFadeStep = 0 - iSpeed;
    break;
    case Fade_Alpha:
        m_iFadeStep = 0 - iSpeed;
    break;
    case Fade_Mask:
        m_iFadeStep = iSpeed;
    break;
    case Fade_RandomMask:
        m_iFadeStep = iSpeed;
    break;

    }

    return m_iFadeStep;
}

int CogeScene::SetFadeOutSpeed(int iSpeed)
{
    switch(m_iFadeType)
    {
    case Fade_None:
        m_iFadeStep = 1;
    break;
    case Fade_Lightness:
    case Fade_Dark:
        m_iFadeStep = 0 - iSpeed;
    break;
    case Fade_Bright:
        m_iFadeStep = iSpeed;
    break;
    case Fade_Alpha:
        m_iFadeStep = 1;
    break;
    case Fade_Mask:
    case Fade_RandomMask:
        m_iFadeStep = 1;
    break;

    }

    return m_iFadeStep;
}


bool CogeScene::GetAutoFade()
{
    return m_bAutoFade;
}
void CogeScene::SetAutoFade(bool value)
{
    m_bAutoFade = value;
}

CogeImage* CogeScene::GetFadeMask()
{
    return m_pFadeMask;
}
void CogeScene::SetFadeMask(CogeImage* pMaskImage)
{
    m_pFadeMask = pMaskImage;
}

/*
void CogeScene::EnableAutoScroll(int iStepX, int iStepY)
{
    m_iScrollState = 1;
    m_iScrollStepX = iStepX;
    m_iScrollStepY = iStepY;
}
void CogeScene::DisableAutoScroll()
{
    m_iScrollState = -1;
}
*/

int CogeScene::GetAutoScrollState()
{
    return m_iScrollState;
}
void CogeScene::StopAutoScroll()
{
    m_iScrollState = 0;
    m_iScrollCurrentX = 0;
    m_iScrollCurrentY = 0;
    m_iScrollTargetX = -1;
    m_iScrollTargetY = -1;
    m_bScrollLoop = false;
}
void CogeScene::StartAutoScroll(bool bResetParams, int iStepX, int iStepY, int iInterval, bool bLoop)
{
    if(bResetParams)
    {
        m_iScrollStepX = iStepX;
        m_iScrollStepY = iStepY;
        m_iScrollInterval = iInterval;

        m_bScrollLoop = bLoop;
    }

    m_iScrollCurrentX = 0;
    m_iScrollCurrentY = 0;

    if(m_bScrollLoop)
    {
        m_iScrollTargetX = -1;
        m_iScrollTargetY = -1;
    }

    m_iScrollState = 1;
}

void CogeScene::SetScrollSpeed(int iStepX, int iStepY, int iInterval)
{
    m_iScrollStepX = iStepX;
    m_iScrollStepY = iStepY;
    m_iScrollInterval = iInterval;
}

void CogeScene::SetScrollTarget(int iTargetX, int iTargetY)
{
    m_iScrollTargetX = iTargetX;
    m_iScrollTargetY = iTargetY;
}

int CogeScene::GetScrollTargetX()
{
    return m_iScrollTargetX;
}
int CogeScene::GetScrollTargetY()
{
    return m_iScrollTargetY;
}

int CogeScene::GetScrollStepX()
{
    return m_iScrollStepX;
}
int CogeScene::GetScrollStepY()
{
    return m_iScrollStepY;
}
int CogeScene::GetScrollX()
{
    return m_iScrollCurrentX;
}
int CogeScene::GetScrollY()
{
    return m_iScrollCurrentY;
}

void CogeScene::DoScroll()
{
    if(m_iScrollState <= 0) return;

    //m_iScrollTime += m_pEngine->m_iFrameInterval;
    m_iScrollTime += m_pEngine->m_iCurrentInterval;
    if (m_iScrollTime <= m_iScrollInterval)
    {
        m_iScrollCurrentX = 0;
        m_iScrollCurrentY = 0;
        return;
    }
    else m_iScrollTime = 0;


    int iOldMouseX = m_pEngine->m_iMouseX - m_SceneViewRect.left;
    int iOldMouseY = m_pEngine->m_iMouseY - m_SceneViewRect.top;

    int iNextLeft = m_SceneViewRect.left + m_iScrollStepX;
    int iNextTop  = m_SceneViewRect.top  + m_iScrollStepY;

    if(m_iScrollStepX < 0)
    {
        if(iNextLeft < 0 && m_bScrollLoop)
        {
            int iNewLeft = m_pBackground->GetWidth() - m_pEngine->GetVideo()->GetWidth() + iNextLeft;

            ogeSpriteMap::iterator it, itb, ite;
            CogeSprite* pCurrentSpr = NULL;

            itb = m_ActiveSprites.begin();
            ite = m_ActiveSprites.end();

            for (it=itb; it!=ite; it++)
            {
                pCurrentSpr = it->second;

                if(!pCurrentSpr->m_bIsRelative)
                {
                    pCurrentSpr->m_iPosX = iNewLeft + (pCurrentSpr->m_iPosX - m_iScrollStepX - m_SceneViewRect.left);
                    pCurrentSpr->m_iPathOffsetX = iNewLeft + (pCurrentSpr->m_iPathOffsetX - m_iScrollStepX - m_SceneViewRect.left);
                }

            }

            if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
            {

                m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

                //m_pScreen->Draw(m_pBackground, 0, 0);
                m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
            }

            m_SceneViewRect.left = iNewLeft;

        }
        else
            m_SceneViewRect.left = iNextLeft;
    }
    else
    {
        if(iNextLeft > m_pBackground->GetWidth() && m_bScrollLoop)
        {
            //m_SceneViewRect.left = iNextLeft - m_pBackground->GetWidth();

            int iNewLeft = iNextLeft - m_pBackground->GetWidth();

            ogeSpriteMap::iterator it, itb, ite;
            CogeSprite* pCurrentSpr = NULL;

            itb = m_ActiveSprites.begin();
            ite = m_ActiveSprites.end();

            for (it=itb; it!=ite; it++)
            {
                pCurrentSpr = it->second;

                if(!pCurrentSpr->m_bIsRelative)
                {
                    pCurrentSpr->m_iPosX = iNewLeft + (pCurrentSpr->m_iPosX - m_iScrollStepX - m_SceneViewRect.left);
                    pCurrentSpr->m_iPathOffsetX = iNewLeft + (pCurrentSpr->m_iPathOffsetX - m_iScrollStepX - m_SceneViewRect.left);
                }
            }

            if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
            {
                //m_pScreen->Draw(m_pBackground, 0, 0);

                m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

                m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
            }

            m_SceneViewRect.left = iNewLeft;

        }
        else
            m_SceneViewRect.left = iNextLeft;
    }

    if(m_iScrollStepY < 0)
    {
        if(iNextTop < 0 && m_bScrollLoop)
        {
            //m_SceneViewRect.top = m_pBackground->GetHeight() - m_pEngine->GetVideo()->GetHeight() + iNextTop;

            int iNewTop = m_pBackground->GetHeight() - m_pEngine->GetVideo()->GetHeight() + iNextTop;

            ogeSpriteMap::iterator it, itb, ite;
            CogeSprite* pCurrentSpr = NULL;

            itb = m_ActiveSprites.begin();
            ite = m_ActiveSprites.end();

            for (it=itb; it!=ite; it++)
            {
                pCurrentSpr = it->second;

                if(!pCurrentSpr->m_bIsRelative)
                {
                    pCurrentSpr->m_iPosY = iNewTop + (pCurrentSpr->m_iPosY - m_iScrollStepY - m_SceneViewRect.top);
                    pCurrentSpr->m_iPathOffsetY = iNewTop + (pCurrentSpr->m_iPathOffsetY - m_iScrollStepY - m_SceneViewRect.top);
                }

            }

            if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
            {
                //m_pScreen->Draw(m_pBackground, 0, 0);
                //printf("iNextTop < 0 : Redraw all bg \n");

                //OGE_Log("iNextTop < 0 : %d (%d) \n", iNewTop, m_SceneViewRect.top);

                m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

                m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
            }

            m_SceneViewRect.top = iNewTop;

        }
        else
            m_SceneViewRect.top = iNextTop;
    }
    else
    {
        if(iNextTop > m_pBackground->GetHeight() && m_bScrollLoop)
        {
            //m_SceneViewRect.top = iNextTop - m_pBackground->GetHeight();

            int iNewTop = iNextTop - m_pBackground->GetHeight();

            ogeSpriteMap::iterator it, itb, ite;
            CogeSprite* pCurrentSpr = NULL;

            itb = m_ActiveSprites.begin();
            ite = m_ActiveSprites.end();

            for (it=itb; it!=ite; it++)
            {
                pCurrentSpr = it->second;

                if(!pCurrentSpr->m_bIsRelative)
                {
                    pCurrentSpr->m_iPosY = iNewTop + (pCurrentSpr->m_iPosY - m_iScrollStepY - m_SceneViewRect.top);
                    pCurrentSpr->m_iPathOffsetY = iNewTop + (pCurrentSpr->m_iPathOffsetY - m_iScrollStepY - m_SceneViewRect.top);
                }

            }

            if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
            {
                //m_pScreen->Draw(m_pBackground, 0, 0);
                //OGE_Log("iNextTop > m_pBackground->GetHeight() : %d (%d) \n", iNewTop, m_SceneViewRect.top);

                m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

                m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
            }

            m_SceneViewRect.top = iNewTop;

        }
        else
            m_SceneViewRect.top = iNextTop;
    }

    m_iScrollCurrentX = m_iScrollStepX;
    m_iScrollCurrentY = m_iScrollStepY;

    ValidateViewRect();

    m_pEngine->m_iMouseX = iOldMouseX + m_SceneViewRect.left;
    m_pEngine->m_iMouseY = iOldMouseY + m_SceneViewRect.top;

    if(m_pMouseSpr)
    {
        m_pMouseSpr->SetPos(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY);
    }

    // check for stop scrolling ...
    if(!m_bScrollLoop && m_iScrollTargetX >= 0 && m_iScrollTargetY >= 0)
    {
        bool bEndX = false;
        bool bEndY = false;

        if(m_iScrollStepX <= 0)
        {
            if(m_SceneViewRect.left <= m_iScrollTargetX) bEndX = true;
        }
        else
        {
            if(m_SceneViewRect.left >= m_iScrollTargetX) bEndX = true;
        }

        if(bEndX)
        {
            if(m_iScrollStepY <= 0)
            {
                if(m_SceneViewRect.top <= m_iScrollTargetY) bEndY = true;
            }
            else
            {
                if(m_SceneViewRect.top >= m_iScrollTargetY) bEndY = true;
            }

            if(bEndY)
            {
                StopAutoScroll();
                if(m_pPlotSpr && m_pPlotSpr->GetActive())
                {
                    m_pPlotSpr->ResumeUpdateScript();
                }
            }
        }
    }

}

int CogeScene::DoFade()
{
    if(m_iFadeState == 0) return -1;

    if(m_iFadeStep  == 0)
    {
        m_iFadeState = 0;
        return -1;
    }

    if(m_iFadeEffectCurrentValue != m_iFadeEffectStartValue)
    {
        //m_iFadeTime += m_pEngine->m_iFrameInterval;
        m_iFadeTime += m_pEngine->m_iCurrentInterval;

        //if (m_iFadeTime <= m_iFadeInterval) return -1;
        //else m_iFadeTime = 0;

        if (m_iFadeTime > m_iFadeInterval)
        {
            m_iFadeEffectCurrentValue += m_iFadeStep;
            m_iFadeTime = 0;
        }
    }
    else m_iFadeEffectCurrentValue += m_iFadeStep;

    //m_iFadeEffectCurrentValue += m_iFadeStep;

    if(m_pEngine->m_bUseDirtyRect)
        m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;

    if(m_iFadeStep > 0)
    {
        if(m_iFadeEffectCurrentValue > m_iFadeEffectValue)
        {
            //m_pEngine->m_pCurrentGameScene = this;
            if(m_iFadeState > 0)
            {
                CallProxySprEvent(Event_OnGetFocus);
                CallEvent(Event_OnOpen);
                //m_pEngine->m_bFreeze = false;
                //if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
                //{
                //    m_pScreen->Draw(m_pBackground,
                //                m_SceneViewRect.left, m_SceneViewRect.top,
                //                m_SceneViewRect.left, m_SceneViewRect.top,
                //                m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
                //}
            }
            else
            {
                CallProxySprEvent(Event_OnLoseFocus);
                CallEvent(Event_OnClose);
                //m_pEngine->m_bFreeze = false;

                //if(m_pNextScene == NULL && m_sNextSceneName.length() > 0)
                if(m_pEngine->m_pNextActiveScene == NULL && m_pEngine->m_sNextSceneName.length() > 0)
                {
                    CogeScene* pActiveScene = m_pEngine->m_pActiveScene;
                    m_pEngine->m_pActiveScene = NULL;

                    CogeScene* pNextScene = m_pEngine->GetScene(m_pEngine->m_sNextSceneName);
                    //if(pNextScene) m_pNextScene = pNextScene;
                    if(pNextScene) m_pEngine->m_pNextActiveScene = pNextScene;
                    else m_pEngine->m_sNextSceneName = "";

                    m_pEngine->m_pActiveScene = pActiveScene;

                }
            }

            m_iFadeState = 0;
            return 0;
        }
    }
    else
    {
        if(m_iFadeEffectCurrentValue < m_iFadeEffectValue)
        {
            //m_pEngine->m_pCurrentGameScene = this;
            if(m_iFadeState > 0)
            {
                CallProxySprEvent(Event_OnGetFocus);
                CallEvent(Event_OnOpen);
                //m_pEngine->m_bFreeze = false;
                //if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
                //{
                //    m_pScreen->Draw(m_pBackground,
                //                m_SceneViewRect.left, m_SceneViewRect.top,
                //                m_SceneViewRect.left, m_SceneViewRect.top,
                //                m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
                //}
            }
            else
            {
                CallProxySprEvent(Event_OnLoseFocus);
                CallEvent(Event_OnClose);
                //m_pEngine->m_bFreeze = false;

                //if(m_pNextScene == NULL && m_sNextSceneName.length() > 0)
                if(m_pEngine->m_pNextActiveScene == NULL && m_pEngine->m_sNextSceneName.length() > 0)
                {
                    CogeScene* pActiveScene = m_pEngine->m_pActiveScene;
                    m_pEngine->m_pActiveScene = NULL;

                    CogeScene* pNextScene = m_pEngine->GetScene(m_pEngine->m_sNextSceneName);
                    //if(pNextScene) m_pNextScene = pNextScene;
                    if(pNextScene) m_pEngine->m_pNextActiveScene = pNextScene;
                    else m_pEngine->m_sNextSceneName = "";

                    m_pEngine->m_pActiveScene = pActiveScene;
                }
            }
            m_iFadeState = 0;
            return 0;
        }
    }

    CogeImage* pFadeMask = NULL;
    if(m_pFadeMask) pFadeMask = m_pFadeMask;
    else pFadeMask = m_pEngine->m_pCurrentMask;

    //CogeImage* pLastScreen = m_pEngine->m_pVideo->GetLastScreen();
    CogeImage* pClipboardA = m_pEngine->m_pVideo->GetClipboardA();
    CogeImage* pClipboardB = m_pEngine->m_pVideo->GetClipboardB();

    switch(m_iFadeType)
    {
    case Fade_None:

        ////m_pEngine->m_pVideo->m_pDefaultScreen->Draw(m_pScreen, 0, 0,
        //pLastScreen->CopyRect(m_pScreen, 0, 0,
        //m_SceneViewRect.left, m_SceneViewRect.top, m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

    break;
    case Fade_Lightness:
    case Fade_Dark:
    case Fade_Bright:

        m_pScreen->Lightness(m_SceneViewRect.left, m_SceneViewRect.top,
        m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight, m_iFadeEffectCurrentValue);

    break;
    case Fade_Alpha:

        if(m_iFadeState < 0)
        pClipboardA->CopyRect(m_pScreen, 0, 0,
        m_SceneViewRect.left, m_SceneViewRect.top, m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
        else
        m_pScreen->BltAlphaBlend(pClipboardA, m_iFadeEffectCurrentValue,
        m_SceneViewRect.left, m_SceneViewRect.top, 0, 0, m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

    break;
    case Fade_Mask:
    case Fade_RandomMask:

        if(m_iFadeState < 0)
        pClipboardA->CopyRect(m_pScreen, 0, 0,
        m_SceneViewRect.left, m_SceneViewRect.top, m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
        else if(pFadeMask)
        {
            pClipboardB->CopyRect(pFadeMask, 0, 0,
            0, 0, m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

            pClipboardB->SubLight(0, 0,
            m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight, m_iFadeEffectCurrentValue);

            m_pScreen->BltMask(pClipboardA, pClipboardB, m_SceneViewRect.left, m_SceneViewRect.top, 0, 0, 0, 0,
            m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

        }

    break;

    }

    return 1;
}

void CogeScene::DrawBackground()
{
	if(m_iState < 0) return;

	if(m_pScreen && m_pBackground)
	{
	    //if(m_pBackground == m_pEngine->m_pVideo->GetDefaultBg())
        //{
        //    m_pEngine->m_pVideo->ClearDefaultBg(m_iBackgroundColor);
        //}

	    if(m_pEngine->m_bUseDirtyRect)
	    {
            if(m_iTotalDirtyRects > 0 && m_iTotalDirtyRects <= _OGE_MAX_DIRTY_RECT_NUMBER_)
            {
                CogeRect* rc = NULL;
                ogeRectList::iterator it  = m_DirtyRects.begin();

                size_t count = m_DirtyRects.size();

                while(count > 0) //(it!=ite)
                {
                    rc = *it;
                    if(rc->left < 0) { rc->right = rc->right - rc->left; rc->left = 0; }
                    if(rc->top < 0)  { rc->bottom = rc->bottom - rc->top; rc->top = 0; }

                    /*
                    if(rc) m_pScreen->CopyRect(m_pBackground,
                                    rc->left, rc->top,
                                    rc->left, rc->top,
                                    rc->right - rc->left, rc->bottom - rc->top);
                    */

                    if(rc) m_pScreen->Draw(m_pBackground,
                                    rc->left, rc->top,
                                    rc->left, rc->top,
                                    rc->right - rc->left, rc->bottom - rc->top);

                    it++;

                    count--;
                }
            }
            else if(m_iTotalDirtyRects > _OGE_MAX_DIRTY_RECT_NUMBER_)
            {
                if(m_pBackground == m_pEngine->m_pVideo->GetDefaultBg())
                {
                    m_pEngine->m_pVideo->ClearDefaultBg(m_iBackgroundColor);
                }
                m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
            }
        }
        else
        {
            if(m_pBackground == m_pEngine->m_pVideo->GetDefaultBg())
            {
                m_pEngine->m_pVideo->ClearDefaultBg(m_iBackgroundColor);
            }
            m_pScreen->CopyRect(m_pBackground,
                                m_SceneViewRect.left, m_SceneViewRect.top,
                                m_SceneViewRect.left, m_SceneViewRect.top,
                                m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
        }
    }
    //else
    //{
    //    OGE_Log("Screen or Background is NULL: %d, %d \n", (int)m_pScreen, (int)m_pBackground);
    //}

}

void CogeScene::ScrollView(int iIncX, int iIncY, bool bImmediately, bool bUpdateRelSpr)
{
    if(!bImmediately)
    {
        m_iPendingScrollX = m_iPendingScrollX + iIncX;
        m_iPendingScrollY = m_iPendingScrollY + iIncY;
    }
    else
    {
        int iOldMouseX = m_pEngine->m_iMouseX - m_SceneViewRect.left;
        int iOldMouseY = m_pEngine->m_iMouseY - m_SceneViewRect.top;

        m_SceneViewRect.left   =   m_SceneViewRect.left + iIncX;
        m_SceneViewRect.top    =   m_SceneViewRect.top  + iIncY;

        ValidateViewRect();

        m_pEngine->m_iMouseX = iOldMouseX + m_SceneViewRect.left;
        m_pEngine->m_iMouseY = iOldMouseY + m_SceneViewRect.top;

        int count = m_SpritesInView.size();
        if(count > 0 && bUpdateRelSpr)
        {
            ogeSpriteList::iterator it = m_SpritesInView.begin();
            while (count>0)
            {
                CogeSprite* spr = *it;
                if(spr->m_bIsRelative && spr->m_pParent == NULL)
                {
                    spr->AdjustSceneView();
                    spr->PrepareNextFrame();
                }
                it++;
                count--;
            }
        }

        if(m_pMouseSpr)
        {
            m_pMouseSpr->SetPos(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY);
        }
    }

}

void CogeScene::SetViewPos(int iPosX, int iPosY, bool bImmediately, bool bUpdateRelSpr)
{
    if(!bImmediately)
    {
        m_iPendingPosX = iPosX;
        m_iPendingPosY = iPosY;
    }
    else
    {

        if(m_pEngine->m_bUseDirtyRect && m_pScreen && m_pBackground)
        {
            //m_pScreen->Draw(m_pBackground, 0, 0);
            //printf("iNextTop > m_pBackground->GetHeight() : Redraw all bg \n");
            //m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;

            m_pScreen->CopyRect(m_pBackground,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_SceneViewRect.left, m_SceneViewRect.top,
                                    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);

            m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
        }

        int iOldMouseX = m_pEngine->m_iMouseX - m_SceneViewRect.left;
        int iOldMouseY = m_pEngine->m_iMouseY - m_SceneViewRect.top;

        m_SceneViewRect.left   =   iPosX;
        m_SceneViewRect.top    =   iPosY;

        ValidateViewRect();

        m_pEngine->m_iMouseX = iOldMouseX + m_SceneViewRect.left;
        m_pEngine->m_iMouseY = iOldMouseY + m_SceneViewRect.top;

        //if(m_pEngine->m_bUseDirtyRect) m_bNeedRedrawBg = true;

        /*
        int count = m_SpritesInView.size();
        if(count > 0 && bUpdateRelSpr)
        {
            ogeSpriteList::iterator it = m_SpritesInView.begin();
            while (count>0)
            {
                CogeSprite* spr = *it;
                if(spr->m_bIsRelative && spr->m_pParent == NULL)
                {
                    spr->AdjustSceneView();
                    spr->PrepareNextFrame();
                }
                it++;
                count--;
            }
        }
        */

        int count = m_ActiveSprites.size();
        if(count > 0 && bUpdateRelSpr)
        {
            ogeSpriteMap::iterator it, itb, ite;
            CogeSprite* spr = NULL;

            itb = m_ActiveSprites.begin();
            ite = m_ActiveSprites.end();

            for (it=itb; it!=ite; it++)
            {
                spr = it->second;

                if(spr->m_bIsRelative && spr->m_pParent == NULL)
                {
                    spr->AdjustSceneView();
                    spr->PrepareNextFrame();
                }

            }
        }

        if(m_pMouseSpr)
        {
            m_pMouseSpr->SetPos(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY);
        }

    }

}

bool CogeScene::OpenTimer(int iInterval)
{
    if(iInterval <= 0) return false;
    m_iTimerInterval = iInterval;
    m_iTimerLastTick = SDL_GetTicks();
    m_iTimerEventCount = 0;
    m_bEnableDefaultTimer = true;
    //if(bEnablePlotTrigger) m_iPlotTimerTriggerFlag = 1;
    //else m_iPlotTimerTriggerFlag = 0;
    return m_bEnableDefaultTimer;
}
void CogeScene::CloseTimer()
{
    m_bEnableDefaultTimer = false;
    //m_iPlotTimerTriggerFlag = 0;
}
int CogeScene::GetTimerInterval()
{
    if(m_bEnableDefaultTimer) return m_iTimerInterval;
    else return -1;
}
bool CogeScene::IsTimerWaiting()
{
    return m_bEnableDefaultTimer;
}

void CogeScene::CheckSpriteInteraction()
{
    if(m_iState < 0) return;

    ogeSpriteMap::iterator its, itd, itb, ite;

	//bool bNeedCheckDirtyRect = m_pEngine->m_bUseDirtyRect;

    /*
	if(m_pMouseSpr)
	{
		if (bNeedCheckDirtyRect)
		{
		    if (OverlapRect(m_pMouseSpr->m_DrawPosRect, m_pMouseSpr->m_OldPosRect))
		    {
		        m_pMouseSpr->UpdateDirtyRect();
				AddDirtyRect(&m_pMouseSpr->m_DirtyRect);
		    }
			else
			{
				AddDirtyRect(&m_pMouseSpr->m_DrawPosRect);
				AddDirtyRect(&m_pMouseSpr->m_OldPosRect);
			}

		}
	}
	*/

	// check in view or not

	m_SpritesInView.clear();

	//bool bNewPosRectDirty = false;
    //bool bOldPosRectDirty = false;

    m_iTopZ = 0;

	itb = m_ActiveSprites.begin();
	ite = m_ActiveSprites.end();

	for ( its=itb; its!=ite; its++ )
	{
	    CogeSprite* spr = its->second;

	    //if(spr == m_pMouseSpr || spr == m_pFirstSpr || spr == m_pLastSpr) continue;

		spr->m_bGetInView = false;

		if (!spr->m_bVisible) continue;

		if (spr->m_iPosZ > m_iTopZ) m_iTopZ = spr->m_iPosZ;

		//bNewPosRectDirty = false;
        //bOldPosRectDirty = false;

		//bNeedCheckDirtyRect = m_pEngine->m_bUseDirtyRect && (!(spr->m_bIsRelative && spr->m_pParent));
		//bNeedCheckDirtyRect = bNeedCheckDirtyRect &&
		//                     (spr->m_bEnableMovement ||
		//                      spr->m_bEnableAnima    ||
		//                      spr->m_bDefaultDraw);

		if (OverlapRect(m_SceneViewRect, spr->m_DrawPosRect))
		{
		    if(spr->m_pParent && spr->m_bIsRelative && !spr->m_bMouseDrag)
		    {
		        if (OverlapRect(spr->m_pParent->m_DrawPosRect, spr->m_DrawPosRect))
		        {
		            spr->m_bGetInView = true;
                    m_SpritesInView.push_back(spr);
                }
            }
            else
            {
                spr->m_bGetInView = true;
                m_SpritesInView.push_back(spr);
            }

            /*
			if (bNeedCheckDirtyRect)
			{
				//AddDirtyRect(&spr->m_DrawPosRect);
				//if(spr->m_iUnitType != Spr_Static || !spr->m_bUseDefaultDraw)
				bNewPosRectDirty = true;
			}
			*/

		}

        /*
		if (bNeedCheckDirtyRect && OverlapRect(m_SceneViewRect, spr->m_OldPosRect))
		{
            //AddDirtyRect(&spr->m_OldPosRect);
            //if(spr->m_iUnitType != Spr_Static || !spr->m_bUseDefaultDraw)
            bOldPosRectDirty = true;
		}

		if(bNewPosRectDirty && bOldPosRectDirty)
		{
		    if (OverlapRect(spr->m_DrawPosRect, spr->m_OldPosRect))
		    {
		        spr->UpdateDirtyRect();
				AddDirtyRect(&spr->m_DirtyRect);
		    }
			else
			{
				AddDirtyRect(&spr->m_DrawPosRect);
				AddDirtyRect(&spr->m_OldPosRect);
			}
        }
        else
        {
            if(bNewPosRectDirty) AddDirtyRect(&spr->m_DrawPosRect);
            if(bOldPosRectDirty) AddDirtyRect(&spr->m_OldPosRect);
        }
        */

        if (spr->m_bGetInView)
        {
            if(spr->m_iLastDisplayFlag == 0)
            {
                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnEnter);
            }
            spr->m_iLastDisplayFlag = 1;
        }
        else
        {
            if(spr->m_iLastDisplayFlag == 1)
            {
                //AutoAddDirtyRect(spr);

                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnLeave);
            }
            spr->m_iLastDisplayFlag = 0;
        }

	}

	//itb = m_AliveSpriteMap.begin();
	//ite = m_AliveSpriteMap.end();

	// check collision

	for (its=itb; its!=ite; its++)
	{
	    CogeSprite* spr1 = its->second;

	    //if(spr1 == m_pMouseSpr || spr1 == m_pFirstSpr || spr1 == m_pLastSpr) continue;

	    spr1->m_bGetCollided = false;

		if (!spr1->m_bEnableCollision) continue;

		//if (its->second->m_iUnitType == Spr_Anima) continue;

		for (itd=its, itd++; itd!=ite; itd++)
		{
		    CogeSprite* spr2 = itd->second;

		    //if(spr2 == m_pMouseSpr || spr2 == m_pFirstSpr || spr2 == m_pLastSpr) continue;

			if (!spr2->m_bEnableCollision) continue;
            //if (itd->second->m_iUnitType == Spr_Anima) continue;

			if (OverlapRect(spr1->m_Body, spr2->m_Body))
			{
				// load OnCollide event ...

				m_pCurrentSpr = spr1;
				m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
				m_pCurrentSpr->m_pCollidedSpr = spr2;

				m_pCurrentSpr->CallEvent(Event_OnCollide);

				m_pCurrentSpr = spr2;
				m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
				m_pCurrentSpr->m_pCollidedSpr = spr1;

				m_pCurrentSpr->CallEvent(Event_OnCollide);

			}
		}
	}

	// sort the sprites ...
	m_SpritesInView.sort(ogeSprPosCompFunc());


	// check mouse event ...

	m_SpritesForTouches.clear();

	ogeSpriteList::iterator it;
	CogeSprite* spr = NULL;

	int count = m_SpritesInView.size();

	//POINT pt;

	if(m_pDraggingSpr)
	{
	    m_bIsMouseEventHandled = true;

	    spr = m_pDraggingSpr;

	    if(m_pEngine->m_iMouseLeft > 0 || m_pEngine->m_iMouseRight > 0)
        {
            if(m_pEngine->m_iMouseX != spr->m_iLastMouseDownX || m_pEngine->m_iMouseY != spr->m_iLastMouseDownY)
            {

                spr->m_iPosX = m_pEngine->m_iMouseX - spr->m_iLastW;
                spr->m_iPosY = m_pEngine->m_iMouseY - spr->m_iLastH;

                if(spr->m_pParent && spr->m_bIsRelative)
                {
                    spr->m_iRelativeX = spr->m_iPosX - spr->m_pParent->m_iPosX;
                    spr->m_iRelativeY = spr->m_iPosY - spr->m_pParent->m_iPosY;
                }

                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnMouseDrag);

                spr->Update();

                AutoAddDirtyRect(spr);

                spr->ChildrenAdjust();

            }
        }
        else
        {

            spr->m_bMouseDown = false;

            spr->m_iLastMouseDownX = -1;
            spr->m_iLastMouseDownY = -1;

            //spr->MouseUp();
            m_pCurrentSpr = spr;
            m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
            spr->CallEvent(Event_OnMouseUp);

            if(spr->m_bMouseDrag)
            {
                spr->m_bMouseDrag = false;
                //spr->MouseDrop();
                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnMouseDrop);
            }

            m_pDraggingSpr = NULL;
        }
	}
	else m_bIsMouseEventHandled = false;

	it = m_SpritesInView.end();

	while (count > 0)
	{
		it--;

		spr = *it;

		spr->m_bGetMouse = false;

		if (!spr->m_bEnableInput) {count--; continue;}

		if(m_pModalSpr)
		{
		    if(spr != m_pModalSpr && spr->m_pRoot != m_pModalSpr) {count--; continue;}
		}


		if(m_bIsMouseEventHandled)
		{

#ifdef __OGE_WITH_MOUSE__

            // if there is some sprite catched and handled the mouse event,
            // we just need to check OnMouseOut event for the other sprites ...
		    if(!PointInRect(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY, spr->m_Body))
		    {
		        if(spr->m_bMouseIn)
                {
                    spr->m_bMouseIn = false;
                    //spr->MouseOut();

                    m_pCurrentSpr = spr;
                    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    spr->CallEvent(Event_OnMouseOut);

                    if(spr->m_bMouseDown)
                    {
                        spr->m_bMouseDown = false;
                        spr->m_iLastMouseDownX = -1;
                        spr->m_iLastMouseDownY = -1;

                        //m_pCurrentSpr = spr;
                        //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                        //spr->CallEvent(Event_OnMouseUp);
                    }

                }
		    }

#endif

		    m_SpritesForTouches.push_back(spr);

		    count--;
		    continue;
		}

		//pt = spr->GetMouse()->GetPoint();

		if (PointInRect(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY, spr->m_Body))
		{

			spr->m_bGetMouse = true;

            m_bIsMouseEventHandled = true;

            if(!spr->m_bMouseIn)
            {
                spr->m_bMouseIn = true;

                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnMouseIn);

            }

            m_pCurrentSpr = spr;
            m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
            spr->CallEvent(Event_OnMouseOver);

            if(m_pEngine->m_iMouseLeft > 0 || m_pEngine->m_iMouseRight > 0)
            {
                //printf("\n Press Mouse Button ... \n");

                if(!spr->m_bMouseDown)
                {
                    // handle z order for windows ...
                    bool bIsWindow = spr->m_iUnitType >= Spr_Window && spr->m_iUnitType <= Spr_InputText;
                    if (bIsWindow)
                    {
                        if(m_iTopZ > spr->m_iPosZ) //spr->m_pParent->Active();
                        {
                            if (spr->m_pParent && spr->m_bIsRelative) spr->m_pParent->UpdatePosZ(m_iTopZ + 1);
                            else spr->UpdatePosZ(m_iTopZ + 1);
                        }
                    }


                    spr->m_bMouseDown = true;

                    spr->m_iLastMouseDownX = m_pEngine->m_iMouseX;
                    spr->m_iLastMouseDownY = m_pEngine->m_iMouseY;

                    spr->m_iLastW =  m_pEngine->m_iMouseX - spr->m_iPosX;
                    spr->m_iLastH =  m_pEngine->m_iMouseY - spr->m_iPosY;

                    //spr->MouseDown();
                    m_pCurrentSpr = spr;
                    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    spr->CallEvent(Event_OnMouseDown);

                    CogeTextInputter* pTextInputter = spr->GetTextInputter();

                    if(!pTextInputter) m_pEngine->DisableIM();

                    if (spr->m_bEnableFocus)
                    {
                        if(m_pFocusSpr && m_pFocusSpr != spr)
                        {
                            m_pCurrentSpr = m_pFocusSpr;
                            m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                            m_pFocusSpr->CallEvent(Event_OnLoseFocus);
                        }

                        if(pTextInputter) pTextInputter->Focus();

                        if(m_pFocusSpr != spr)
                        {
                            m_pFocusSpr = spr;
                            m_pCurrentSpr = spr;
                            m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                            spr->CallEvent(Event_OnGetFocus);
                        }
                    }
                }
                else
                {
                    if(m_pEngine->m_iMouseX != spr->m_iLastMouseDownX || m_pEngine->m_iMouseY != spr->m_iLastMouseDownY)
                    {


                        if (spr->m_bEnableDrag
                            //&& spr->m_pParent==NULL
                            //&& brz::lib::PointInRect(spr->GetRelativePoint(pt), spr->m_DragRect)
                            )
                        {
                            spr->m_iPosX = m_pEngine->m_iMouseX - spr->m_iLastW;
                            spr->m_iPosY = m_pEngine->m_iMouseY - spr->m_iLastH;

                            if(spr->m_pParent && spr->m_bIsRelative)
                            {
                                spr->m_iRelativeX = spr->m_iPosX - spr->m_pParent->m_iPosX;
                                spr->m_iRelativeY = spr->m_iPosY - spr->m_pParent->m_iPosY;
                            }

                            if(!spr->m_bMouseDrag)
                            {
                                spr->m_bMouseDrag = true;
                                if(m_pDraggingSpr != spr) m_pDraggingSpr = spr;
                            }

                            m_pCurrentSpr = spr;
                            m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                            spr->CallEvent(Event_OnMouseDrag);

                            spr->Update();
                            AutoAddDirtyRect(spr);

                            spr->ChildrenAdjust();

                        }
                    }

                }
            }
            else
            {
                if(spr->m_bMouseDown)
                {
                    spr->m_bMouseDown = false;

                    spr->m_iLastMouseDownX = -1;
                    spr->m_iLastMouseDownY = -1;

                    //spr->MouseUp();
                    m_pCurrentSpr = spr;
                    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    spr->CallEvent(Event_OnMouseUp);

                }
            }
		}
		else
		{

#ifdef __OGE_WITH_MOUSE__

            if(spr->m_bMouseIn)
            {
                spr->m_bMouseIn = false;
                //spr->MouseOut();

                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnMouseOut);

                if(spr->m_bMouseDown)
                {
                    spr->m_bMouseDown = false;
                    spr->m_iLastMouseDownX = -1;
                    spr->m_iLastMouseDownY = -1;

                    //m_pCurrentSpr = spr;
                    //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    //spr->CallEvent(Event_OnMouseUp);
                }

            }

#endif

            m_SpritesForTouches.push_back(spr);

		}

		count--;

	}

	if(m_pModalSpr && m_pModalSpr->m_bActive && m_pModalSpr->m_bVisible)
	{
	    if(!m_bIsMouseEventHandled) m_bIsMouseEventHandled = true;
	}

	//if(m_bIsMouseEventHandled) m_bMouseDown = false;


#ifndef __OGE_WITH_MOUSE__

    int iMainMouseX = m_pEngine->m_iMouseX;
    int iMainMouseY = m_pEngine->m_iMouseY;
    int iMainMouseLeft = m_pEngine->m_iMouseLeft;
    int iMainMouseRight = m_pEngine->m_iMouseRight;

	if(m_pEngine->m_iMainVMouseId >= 0)
    {
        if(m_bIsMouseEventHandled)
        {
            m_pEngine->m_VMouseList[m_pEngine->m_iMainVMouseId].done = true;
            m_pEngine->m_iActiveVMouseCount = m_pEngine->m_iActiveVMouseCount - 1;
        }
    }

    count = m_SpritesForTouches.size();

    it = m_SpritesForTouches.begin();

	while (count > 0 && m_pEngine->m_iActiveVMouseCount > 0)
    {
        spr = *it;

        for(int i=_OGE_MAX_VMOUSE_-1; i>=0; i--)
        {
            if(m_pEngine->m_VMouseList[i].done) continue;
            if(!m_pEngine->m_VMouseList[i].pressed && !m_pEngine->m_VMouseList[i].active) continue;

            if (PointInRect(m_pEngine->m_VMouseList[i].x, m_pEngine->m_VMouseList[i].y, spr->m_Body))
            {
                m_pEngine->m_VMouseList[i].done = true;

                spr->m_bGetMouse = true;

                if(m_pEngine->m_VMouseList[i].pressed)
                {
                    m_pEngine->m_iMouseX = m_pEngine->m_VMouseList[i].x;
                    m_pEngine->m_iMouseY = m_pEngine->m_VMouseList[i].y;
                    m_pEngine->m_iMouseLeft = 1;
                    m_pEngine->m_iMouseRight = 0;

                    if(!spr->m_bMouseIn)
                    {
                        spr->m_bMouseIn = true;

                        m_pCurrentSpr = spr;
                        m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                        spr->CallEvent(Event_OnMouseIn);

                    }

                    if(!spr->m_bMouseDown)
                    {
                        spr->m_bMouseDown = true;

                        spr->m_iLastMouseDownX = -1;
                        spr->m_iLastMouseDownY = -1;

                        m_pCurrentSpr = spr;
                        m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                        spr->CallEvent(Event_OnMouseDown);
                    }

                    m_pCurrentSpr = spr;
                    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    spr->CallEvent(Event_OnMouseOver);

                }
                else
                {
                    m_pEngine->m_iMouseX = m_pEngine->m_VMouseList[i].x;
                    m_pEngine->m_iMouseY = m_pEngine->m_VMouseList[i].y;
                    m_pEngine->m_iMouseLeft = 0;
                    m_pEngine->m_iMouseRight = 0;

                    if(spr->m_bMouseDown)
                    {
                        spr->m_bMouseDown = false;

                        spr->m_iLastMouseDownX = -1;
                        spr->m_iLastMouseDownY = -1;

                        m_pCurrentSpr = spr;
                        m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                        spr->CallEvent(Event_OnMouseUp);
                    }
                }

            }

        }

        it++;
        count--;
    }


    count = m_SpritesForTouches.size();
    it = m_SpritesForTouches.begin();

	while (count > 0)
    {
        spr = *it;

        if(!spr->m_bGetMouse)
        {
            if(spr->m_bMouseIn)
            {
                spr->m_bMouseIn = false;
                //spr->MouseOut();

                m_pCurrentSpr = spr;
                m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                spr->CallEvent(Event_OnMouseOut);

                if(spr->m_bMouseDown)
                {
                    spr->m_bMouseDown = false;
                    spr->m_iLastMouseDownX = -1;
                    spr->m_iLastMouseDownY = -1;

                    //m_pCurrentSpr = spr;
                    //m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
                    //spr->CallEvent(Event_OnMouseUp);
                }

            }
        }

        it++;
        count--;
    }

    m_pEngine->m_iMouseX = iMainMouseX;
    m_pEngine->m_iMouseY = iMainMouseY;
    m_pEngine->m_iMouseLeft = iMainMouseLeft;
    m_pEngine->m_iMouseRight = iMainMouseRight;

#endif


}

void CogeScene::SkipCurrentMouseEvent()
{
    m_bIsMouseEventHandled = false;
}

void CogeScene::CheckInteraction()
{

#ifdef __OGE_WITH_MOUSE__

    if (!m_bIsMouseEventHandled)
    {
        m_bIsMouseEventHandled = true;
        CallMouseEvent(Event_OnMouseOver);

        if(m_pEngine->m_iMouseLeft > 0 || m_pEngine->m_iMouseRight > 0)
        {
            if(!m_bMouseDown)
            {
                m_bMouseDown = true;
                CallMouseEvent(Event_OnMouseDown);
            }
        }
        else
        {
            if(m_bMouseDown)
            {
                m_bMouseDown = false;
                CallMouseEvent(Event_OnMouseUp);
            }
        }
    }

#else

    if (m_pEngine->m_iActiveVMouseCount > 0)
    {
        int iMainMouseX = m_pEngine->m_iMouseX;
        int iMainMouseY = m_pEngine->m_iMouseY;
        int iMainMouseLeft = m_pEngine->m_iMouseLeft;
        int iMainMouseRight = m_pEngine->m_iMouseRight;

        for(int i=_OGE_MAX_VMOUSE_-1; i>=0; i--)
        {
            if(m_pEngine->m_VMouseList[i].done) continue;
            if(!m_pEngine->m_VMouseList[i].pressed && !m_pEngine->m_VMouseList[i].active) continue;

            m_pEngine->m_VMouseList[i].done = true;

            if(m_pEngine->m_VMouseList[i].moving)
            {
                m_pEngine->m_iMouseX = m_pEngine->m_VMouseList[i].x;
                m_pEngine->m_iMouseY = m_pEngine->m_VMouseList[i].y;
                m_pEngine->m_iMouseLeft = 1;
                m_pEngine->m_iMouseRight = 0;

                CallMouseEvent(Event_OnMouseOver);

                if(!m_bMouseDown)
                {
                    m_bMouseDown = true;
                    CallMouseEvent(Event_OnMouseDown);
                }

            }
            else if(m_pEngine->m_VMouseList[i].pressed)
            {
                m_pEngine->m_iMouseX = m_pEngine->m_VMouseList[i].x;
                m_pEngine->m_iMouseY = m_pEngine->m_VMouseList[i].y;
                m_pEngine->m_iMouseLeft = 1;
                m_pEngine->m_iMouseRight = 0;

                if(!m_bMouseDown)
                {
                    m_bMouseDown = true;
                    CallMouseEvent(Event_OnMouseDown);
                }

            }
            else
            {
                m_pEngine->m_iMouseX = m_pEngine->m_VMouseList[i].x;
                m_pEngine->m_iMouseY = m_pEngine->m_VMouseList[i].y;
                m_pEngine->m_iMouseLeft = 0;
                m_pEngine->m_iMouseRight = 0;

                if(m_bMouseDown)
                {
                    m_bMouseDown = false;
                    CallMouseEvent(Event_OnMouseUp);
                }

            }

        }

        m_pEngine->m_iMouseX = iMainMouseX;
        m_pEngine->m_iMouseY = iMainMouseY;
        m_pEngine->m_iMouseLeft = iMainMouseLeft;
        m_pEngine->m_iMouseRight = iMainMouseRight;

    }


#endif


    if(m_pLastSpr)
	{
	    m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallCustomEvents();
	}

}

int CogeScene::CheckSpriteLifeCycles()
{
    // impl activate spr ..

	int count = m_PrepareActive.size();

	ogeSpriteList::iterator it = m_PrepareActive.begin();

	while (count>0)
	{
		(*it)->SetActive(true);
		it++;
		count--;
	}

	m_PrepareActive.clear();

	// impl deactivate spr ..

	count = m_PrepareDeactive.size();
	it  = m_PrepareDeactive.begin();
	while (count>0)
	{
		(*it)->SetActive(false);
		it++;
		count--;
	}

    m_PrepareDeactive.clear();

    // check activating event ..

	count = m_Activating.size();
	it  = m_Activating.begin();
	while (count>0)
	{
	    m_pCurrentSpr = (*it);
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		m_pCurrentSpr->CallEvent(Event_OnActive);
		it++;
		count--;
	}

	m_Activating.clear();

	// check deactivating event ..

	count = m_Deactivating.size();
	it  = m_Deactivating.begin();
	while (count>0)
	{
	    m_pCurrentSpr = (*it);
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		m_pCurrentSpr->CallEvent(Event_OnDeactive);
		it++;
		count--;
	}

	m_Deactivating.clear();

	if(m_pLosingFocusSpr)
    {
        m_pCurrentSpr = m_pLosingFocusSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		m_pCurrentSpr->CallEvent(Event_OnLoseFocus);
		m_pLosingFocusSpr = NULL;
    }

	if(m_pGettingFocusSpr)
    {
        m_pCurrentSpr = m_pGettingFocusSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		m_pCurrentSpr->CallEvent(Event_OnGetFocus);
		m_pGettingFocusSpr = NULL;
    }

	int iLeftCount = m_PrepareActive.size() + m_PrepareDeactive.size();
	if(m_pLosingFocusSpr) iLeftCount++;
	if(m_pGettingFocusSpr) iLeftCount++;

	return iLeftCount;

}

void CogeScene::CheckSpriteCustomEvents(bool bIncludeLastSpr)
{
    if(m_pFirstSpr)
	{
	    m_pCurrentSpr = m_pFirstSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->CallCustomEvents();
	}

    ogeSpriteMap::iterator it = m_ActiveSprites.begin();

    int iCount = m_ActiveSprites.size();

	CogeSprite* spr = NULL;

	//for ( it = itb; it != ite; it++ )
	while(iCount > 0)
	{
	    spr = it->second;
		m_pCurrentSpr = spr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		spr->CallCustomEvents();

		it++;

		iCount--;
	}

	if(bIncludeLastSpr && m_pLastSpr != NULL)
    {
        m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallCustomEvents();
    }

}

void CogeScene::CallUpdateEvent()
{
    if(m_pLastSpr)
	{
	    m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->Update();
	}

    CallEvent(Event_OnUpdate);

}

void CogeScene::CallMouseEvent(int iEventCode)
{
    if(m_Events[iEventCode] >= 0) CallEvent(iEventCode);
    else if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(iEventCode);
    }
}

void CogeScene::CallProxySprEvent(int iEventCode)
{
    if(m_pLastSpr)
    {
        m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->CallEvent(iEventCode);
    }
}

void CogeScene::UpdateSprites()
{
    if(m_pFirstSpr)
	{
	    m_pCurrentSpr = m_pFirstSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pFirstSpr->Update();
	}

    ogeSpriteMap::iterator it = m_ActiveSprites.begin();

    int iCount = m_ActiveSprites.size();

	CogeSprite* spr = NULL;

	//for ( it = itb; it != ite; it++ )
	while(iCount > 0)
	{
	    spr = it->second;

        /*
	    if(spr == m_pMouseSpr || spr == m_pFirstSpr || spr == m_pLastSpr)
        {
            it++;
            iCount--;
            continue;
        }
        */

		m_pCurrentSpr = spr;
		m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
		spr->Update();

		it++;

		iCount--;
	}


	if(m_pMouseSpr)
	{
	    m_pMouseSpr->SetPos(m_pEngine->m_iMouseX, m_pEngine->m_iMouseY);
	    m_pCurrentSpr = m_pMouseSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pMouseSpr->Update();
	}

    /*
	if(m_pLastSpr)
	{
	    m_pCurrentSpr = m_pLastSpr;
	    m_pEngine->m_pCurrentGameSprite = m_pCurrentSpr;
        m_pLastSpr->Update();
	}
	*/
}

void CogeScene::DrawSprites()
{
    PrepareLightMap();

    ogeSpriteList::iterator it;

	int count = m_SpritesInView.size();

	if(count == 0)
	{
	    if(m_pFirstSpr)
        {
            m_pFirstSpr->Draw();
            //AutoAddDirtyRect(m_pFirstSpr);
        }

	    m_pEngine->m_pCurrentGameScene = this;
        CallEvent(Event_OnDrawSprFin);
        BlendSpriteLight();
        DrawLightMap();
        m_pEngine->m_pCurrentGameScene = this;
        CallEvent(Event_OnDrawWinFin);
        if(m_pMouseSpr)
        {
            m_pMouseSpr->Draw();
            AutoAddDirtyRect(m_pMouseSpr);
        }

        if(m_pLastSpr)
        {
            m_pLastSpr->Draw();
            //AutoAddDirtyRect(m_pLastSpr);
        }

        return;
	}

	//then draw every spript in screen ...

	if(m_pFirstSpr)
    {
        m_pFirstSpr->Draw();
        //AutoAddDirtyRect(m_pFirstSpr);
    }

	bool bMeetWindow = false;
	bool bIsWindow = false;

	it  = m_SpritesInView.begin();

	while (count>0)
	{
	    CogeSprite* spr = *it;

	    bIsWindow = spr->m_iUnitType >= Spr_Window && spr->m_iUnitType <= Spr_InputText;

	    if(!bMeetWindow && bIsWindow)
	    {
	        m_pEngine->m_pCurrentGameScene = this;
	        CallEvent(Event_OnDrawSprFin);
	        BlendSpriteLight();
	        DrawLightMap();
        }

        if(bIsWindow) bMeetWindow = true;

		if(spr->m_bVisible) spr->Draw();
		AutoAddDirtyRect(spr);

		it++;

		count--;

		if(count == 0)
		{
		    m_pEngine->m_pCurrentGameScene = this;
		    if(bMeetWindow) CallEvent(Event_OnDrawWinFin);
		    else
		    {
                CallEvent(Event_OnDrawSprFin);
                BlendSpriteLight();
                DrawLightMap();
		    }
        }
	}

	if(m_pMouseSpr)
    {
        m_pMouseSpr->Draw();
        AutoAddDirtyRect(m_pMouseSpr);
    }

    if(m_pLastSpr)
    {
        m_pLastSpr->Draw();
        //AutoAddDirtyRect(m_pLastSpr);
    }

}

void CogeScene::DrawInfo()
{
    if (m_iState < 0) return;

	//int iTop = m_SceneViewRect.top;

	if (m_pEngine->m_bShowFPS)
	{
		m_pEngine->DrawFPS(m_FPSInfoRect.left+1, m_FPSInfoRect.top);
	}

	if (m_pEngine->m_bShowVideoMode)
	{
		m_pEngine->DrawVideoMode(m_VideoModeInfoRect.left+1, m_VideoModeInfoRect.top);
	}

	if (m_pEngine->m_bShowMousePos)
	{
		m_pEngine->DrawMousePos(m_MousePosInfoRect.left+1, m_MousePosInfoRect.top);
	}
}

void CogeScene::PrepareLightMap()
{
    if(m_pEngine->m_bUseDirtyRect) return;

    switch(m_iLightMode)
    {
    case Light_M_View:
        //if(m_pLightMap) m_pEngine->m_pVideo->m_pDefaultScreen->Draw(m_pLightMap, 0, 0, 0, 0,
        if(m_pLightMap) m_pEngine->m_pVideo->GetDefaultBg()->CopyRect(m_pLightMap, 0, 0, 0, 0,
        m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
    break;
    case Light_M_Map:
        //if(m_pLightMap) m_pEngine->m_pVideo->m_pDefaultScreen->Draw(m_pLightMap, 0, 0,
        if(m_pLightMap) m_pEngine->m_pVideo->GetDefaultBg()->CopyRect(m_pLightMap, 0, 0,
        m_SceneViewRect.left, m_SceneViewRect.top,
        m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
    break;
    case Light_M_None:
    break;

    }
}
void CogeScene::BlendSpriteLight()
{
    if(m_pEngine->m_bUseDirtyRect) return;

    if(m_iLightMode == Light_M_None ||
       m_bEnableSpriteLight == false ||
       m_pLightMap  == NULL )
        return;

    CogeSprite* pSprite = NULL;

    //int count = m_SpritesInView.size();
	//ogeSpriteList::iterator it  = m_SpritesInView.begin();

	int count = m_ActiveSprites.size();
	ogeSpriteMap::iterator it  = m_ActiveSprites.begin();

	while (count>0)
	{
	    //pSprite = (*it);

	    pSprite = it->second;

	    bool bIsWindow = pSprite->m_iUnitType >= Spr_Window && pSprite->m_iUnitType <= Spr_InputText;

	    if(pSprite->m_bEnableLightMap && pSprite->m_pLightMap && pSprite->m_bActive && pSprite->m_bVisible && !bIsWindow)
	    {
	        int iMaskWidth = pSprite->m_pLightMap->GetWidth();
	        int iMaskHeight = pSprite->m_pLightMap->GetHeight();

	        int x = (pSprite->m_iPosX - m_SceneViewRect.left) - iMaskWidth / 2;
	        int y = (pSprite->m_iPosY - m_SceneViewRect.top) - iMaskHeight / 2;

	        m_pEngine->m_pVideo->GetDefaultBg()->LightMaskBlend(pSprite->m_pLightMap, x, y, 0, 0, iMaskWidth, iMaskHeight);

        }

		it++;

		count--;

	}

}
void CogeScene::DrawLightMap()
{
    if(m_pEngine->m_bUseDirtyRect) return;

    if(m_iLightMode == Light_M_None || m_pLightMap == NULL) return;

    //m_pScreen->LightMaskBlend( m_pEngine->m_pVideo->m_pDefaultScreen,
    m_pScreen->LightMaskBlend( m_pEngine->m_pVideo->GetDefaultBg(),
    m_SceneViewRect.left, m_SceneViewRect.top, 0, 0,
    m_pEngine->m_iVideoWidth, m_pEngine->m_iVideoHeight);
}

int CogeScene::GetLightMode()
{
    return m_iLightMode;
}
CogeImage* CogeScene::GetLightMap()
{
    return m_pLightMap;
}

void CogeScene::SetLightMap(CogeImage* pLightMapImage, int iLightMode)
{
    m_iLightMode = iLightMode;

    if(iLightMode != 0 && pLightMapImage)
    {
        if(m_pLightMap != pLightMapImage)
        {
            if(m_pLightMap)
            {
                m_pLightMap->Fire();
                //m_pLightMap->GetVideoEngine()->DelImage(m_pLightMap->GetName());
                m_pEngine->m_pVideo->DelImage(m_pLightMap);
            }
            m_pLightMap = pLightMapImage;
            m_pLightMap->Hire();
        }
    }
    else m_iLightMode = 0;

    if(m_iLightMode == 0)
    {
        if(m_pLightMap)
        {
            m_pLightMap->Fire();
            //m_pLightMap->GetVideoEngine()->DelImage(m_pLightMap->GetName());
            m_pEngine->m_pVideo->DelImage(m_pLightMap);
        }
        m_pLightMap = NULL;
    }
}

void CogeScene::UpdateScreen()
{
    if (m_iState < 0) return;
    m_pEngine->UpdateScreen(m_SceneViewRect.left, m_SceneViewRect.top);
}

void CogeScene::UpdatePendingOffset(bool bUpdateRelSpr)
{
    if(m_iPendingScrollX != 0 || m_iPendingScrollY != 0)
    {
        ScrollView(m_iPendingScrollX, m_iPendingScrollY, true, bUpdateRelSpr);
        m_iPendingScrollX = 0;
        m_iPendingScrollY = 0;
    }

    if(m_iPendingPosX != _OGE_INVALID_POS_ || m_iPendingPosY != _OGE_INVALID_POS_)
    {
        if(m_iPendingPosX == _OGE_INVALID_POS_) m_iPendingPosX = m_SceneViewRect.left;
        if(m_iPendingPosY == _OGE_INVALID_POS_) m_iPendingPosY = m_SceneViewRect.top;
        SetViewPos(m_iPendingPosX, m_iPendingPosY, true, bUpdateRelSpr);
        m_iPendingPosX = _OGE_INVALID_POS_;
        m_iPendingPosY = _OGE_INVALID_POS_;
    }
}

void CogeScene::ClearDirtyRects()
{
    m_iTotalDirtyRects = 0;
    m_DirtyRects.clear();
}

void CogeScene::ValidateViewRect()
{
    if(m_pEngine && m_pBackground)
    {
        if(m_SceneViewRect.left < 0) m_SceneViewRect.left = 0;
        if(m_SceneViewRect.top < 0) m_SceneViewRect.top = 0;

        m_SceneViewRect.right = m_SceneViewRect.left + m_pEngine->m_iVideoWidth;
        m_SceneViewRect.bottom = m_SceneViewRect.top + m_pEngine->m_iVideoHeight;

        if(m_SceneViewRect.right > m_iBackgroundWidth)
        {
            m_SceneViewRect.right = m_iBackgroundWidth;
            m_SceneViewRect.left = m_SceneViewRect.right - m_pEngine->m_iVideoWidth;
            //if(m_SceneViewRect.left < 0) m_SceneViewRect.left = 0;
        }

        if(m_SceneViewRect.bottom > m_iBackgroundHeight)
        {
            m_SceneViewRect.bottom = m_iBackgroundHeight;
            m_SceneViewRect.top = m_SceneViewRect.bottom - m_pEngine->m_iVideoHeight;
            //if(m_SceneViewRect.top < 0) m_SceneViewRect.top = 0;
        }
    }
    else
    {
        m_SceneViewRect.left = 0;
        m_SceneViewRect.top = 0;

        m_SceneViewRect.right = 0;
        m_SceneViewRect.bottom = 0;
    }

}

int CogeScene::Update()
{
    if (m_iState < 0) return -1;

    //ClearDirtyRects();

    m_pGettingFocusSpr = NULL;
    m_pLosingFocusSpr = NULL;

    m_Activating.clear();
    m_Deactivating.clear();

    m_PrepareActive.clear();
    m_PrepareDeactive.clear();

    if(m_iState == 0)
    {
        //m_SpritesInView.clear();

        if(m_iScrollState > 0) DoScroll();

        //UpdatePendingOffset(false);

        // first, let srpites update themselves ...
        UpdateSprites(); // update position, status... etc

        // then check srpites' new status after they updated ...
        CheckSpriteInteraction(); // check Collision, Mouse Events... etc

        // check sprites' custom events ..
        CheckSpriteCustomEvents();

        // time & timer ...
        int iCurrentTick = SDL_GetTicks();
        m_iCurrentTime = iCurrentTick - (m_iActiveTime + m_iPauseTime);
        if(m_bEnableDefaultTimer)
        {
            if(iCurrentTick - m_iTimerLastTick >= m_iTimerInterval)
            {
                m_iTimerLastTick = iCurrentTick;
                m_iTimerEventCount++;
                CallEvent(Event_OnTimerTime);

                /*
                if(m_pPlotSpr && m_iPlotTimerTriggerFlag > 0)
                {
                    m_iPlotTimerTriggerFlag = 0;
                    m_pPlotSpr->ResumeUpdateScript();
                }
                */

                if(m_pPlotSpr && m_pPlotSpr->GetActive())
                {
                    m_pPlotSpr->ResumeUpdateScript();
                }

            }
        }

    }

    // handle global events ...
    HandleEvent();

    // call scene's own update event ...
    CallUpdateEvent();

    // check scene's interaction events ...
    CheckInteraction();

    //if(m_iScrollState > 0) DoScroll();

	// refresh screen ...
	//UpdatePendingOffset(true);

	// prepare info to show
	int iTop = m_SceneViewRect.top;

	if (m_pEngine->m_bShowFPS)
	{
		m_FPSInfoRect.left = m_SceneViewRect.left;
		m_FPSInfoRect.top = iTop;
		m_FPSInfoRect.right = m_FPSInfoRect.left + 64;
		m_FPSInfoRect.bottom = m_FPSInfoRect.top + _OGE_DOT_FONT_SIZE_;
		if(m_pEngine->m_bUseDirtyRect) AddDirtyRect(&m_FPSInfoRect);
		iTop = m_FPSInfoRect.bottom + 1;
	}

	if (m_pEngine->m_bShowVideoMode)
	{
		m_VideoModeInfoRect.left = m_SceneViewRect.left;
		m_VideoModeInfoRect.top = iTop;
		m_VideoModeInfoRect.right = m_VideoModeInfoRect.left + 96;
		m_VideoModeInfoRect.bottom = m_VideoModeInfoRect.top + _OGE_DOT_FONT_SIZE_;
		if(m_pEngine->m_bUseDirtyRect) AddDirtyRect(&m_VideoModeInfoRect);
		iTop = m_VideoModeInfoRect.bottom + 1;
	}

	if (m_pEngine->m_bShowMousePos)
	{
		m_MousePosInfoRect.left = m_SceneViewRect.left;
		m_MousePosInfoRect.top = iTop;
		m_MousePosInfoRect.right = m_MousePosInfoRect.left + 160;
		m_MousePosInfoRect.bottom = m_MousePosInfoRect.top + _OGE_DOT_FONT_SIZE_;
		if(m_pEngine->m_bUseDirtyRect) AddDirtyRect(&m_MousePosInfoRect);
		iTop = m_MousePosInfoRect.bottom + 1;
	}

	// draw bg ...
	DrawBackground();

    //...
	ClearDirtyRects();

    // draw spr ...
	DrawSprites();

	// draw global info ...
	DrawInfo();


	// test - begin
	/*
	CogeImage* pMainScreen = m_pEngine->GetScreen();
    CogeImage* pBoyImg = m_pEngine->GetImage("boy");
    if(pMainScreen != NULL && pBoyImg != NULL)
    {
        //pMainScreen->CopyRect(pBoyImg, 320, 240, 3, 7, 29, 31);
        //pMainScreen->Blt(pBoyImg, 320, 240, pBoyImg->GetRawColorKey(), 3, 7, 29, 31);
        //pMainScreen->BltChangedRGB(pBoyImg, 0x00f00a0a, 300, 200);
        //pMainScreen->BltAlphaBlend(pBoyImg, 128, 300, 200);
        //pMainScreen->BltWithColor(pBoyImg, 0x00ff0000, 128, 300, 200);
        pMainScreen->BltWithEdge(pBoyImg, 0x00ff0000, 300, 200);

        //pMainScreen->SubLight(300, 200, 200, 200, 128);

        //pMainScreen->Lightness(300, 200, 200, 200, 128);



    }
    */
	// test - end


	// let user have a chance to modify the screen ...
    CallEvent(Event_OnDraw);

    //if(m_bNeedRedrawBg)
    //{
    //    m_iTotalDirtyRects = _OGE_MAX_DIRTY_RECT_NUMBER_ + 8;
    //    m_bNeedRedrawBg = false;
    //}


    bool bIsFading = false;

	if(m_iFadeState == 0)
	{
	    /*
	    if(m_pNextScene)
	    {
            m_pEngine->SetActiveScene(m_pNextScene);
            return 1;
	    }
	    */

	    if(m_pEngine->m_pNextActiveScene)
	    {
            m_pEngine->SetActiveScene(m_pEngine->m_pNextActiveScene);
            return 1;
	    }
    }
    else
    {
        DoFade();
        bIsFading = true;
    }

    // check sprites' lives ..
    // this will always be the last step in every update cycle ...
    int iMaxCycles = 5;
    int iLeftCount = CheckSpriteLifeCycles();
    while(iLeftCount > 0 && iMaxCycles > 0)
    {
        iLeftCount = CheckSpriteLifeCycles();
        iMaxCycles--;
    }

    if(bIsFading)
    {
        if(m_iFadeState == 0)
        {
            /*
            if(m_pNextScene)
            {
                m_pEngine->SetActiveScene(m_pNextScene);
                return 1;
            }
            */

            if(m_pEngine->m_pNextActiveScene)
            {
                m_pEngine->SetActiveScene(m_pEngine->m_pNextActiveScene);
                return 1;
            }
        }
    }

    // flip screen ...
	UpdateScreen();

    return 0;

}


/*---------------------------------- GameMap -----------------------------------------*/

CogeGameMap::CogeGameMap(const std::string& sTheName, CogeEngine* pTheEngine):
m_pEngine(pTheEngine),
m_pCurrentScene(NULL),
m_pBackground(NULL),
m_sName(sTheName)
{
    m_iBgWidth = 0;
    m_iBgHeight = 0;

    m_iTileRootX = 0;
    m_iTileRootY = 0;

    m_iTileWidth = 0;
    m_iTileHeight = 0;

    m_iColCount = 0;
    m_iRowCount = 0;

    m_iFirstX = 0;
    m_iFirstY = 0;

    m_iMapType = 0;

    m_iTotalUsers = 0;
}
CogeGameMap::~CogeGameMap()
{
    m_Tiles.clear();
    m_OriginalTiles.clear();
    m_Data.clear();

    //if(m_pBackground) m_pEngine->m_pVideo->DelImage(m_pBackground->GetName());

    SetBgImage(NULL);
}

const std::string& CogeGameMap::GetName()
{
    return m_sName;
}

bool CogeGameMap::LoadConfigFile(const std::string& sConfigFileName)
{
    return m_pEngine->LoadIniFile(&m_IniFile, sConfigFileName, m_pEngine->m_bPackedScene, m_pEngine->m_bEncryptedScene);
}

/*
bool CogeGameMap::Load(const std::string& sConfigFileName)
{
    //CogeIniFile ini;

    if (m_IniFile.Load(sConfigFileName))
    {
        // load ...

        std::string sBgName = m_IniFile.ReadString("GameMap", "Background", "");
        if(sBgName.length() > 0)
        {
            CogeImage* pBackground = m_pEngine->GetImage(sBgName);
            if(pBackground) SetBgImage(pBackground);
            else return false;

            m_iMapType = m_IniFile.ReadInteger("GameMap", "MapType", 0);
            m_iTileWidth = m_IniFile.ReadInteger("GameMap", "TileWidth", 0);
            m_iTileHeight = m_IniFile.ReadInteger("GameMap", "TileHeight", 0);
            //m_iTileDepth = m_IniFile.ReadInteger("GameMap", "TileDepth", 0);
            m_iTileRootX = m_IniFile.ReadInteger("GameMap", "TileRootX", 0);
            m_iTileRootY = m_IniFile.ReadInteger("GameMap", "TileRootY", 0);
            m_iFirstX = m_IniFile.ReadInteger("GameMap", "FirstX", 0);
            m_iFirstY = m_IniFile.ReadInteger("GameMap", "FirstY", 0);
            int iColumnCount = m_IniFile.ReadInteger("GameMap", "ColumnCount", 0);
            int iRowCount = m_IniFile.ReadInteger("GameMap", "RowCount", 0);

            SetMapSize(iColumnCount, iRowCount);

            int iTileValue = 0;

            for(int j=0; j<iRowCount; j++)
            for(int i=0; i<iColumnCount; i++)
            {
                iTileValue = m_IniFile.ReadInteger("Tiles", "X"+OGE_itoa(i)+"Y"+OGE_itoa(j), 0);
                m_Tiles[j*iColumnCount + i] = iTileValue;
                m_OriginalTiles[j*iColumnCount + i] = iTileValue;
                m_Data[j*iColumnCount + i] = 0;
            }


            InitFinders();

            return true;

        }
        else return false;

    }
    else return false;
}
*/

void CogeGameMap::InitFinders()
{
    m_PathFinder.Initialize(&m_Tiles, m_iColCount, m_iRowCount);
    m_RangeFinder.Initialize(&m_Tiles, m_iColCount, m_iRowCount);
}

bool CogeGameMap::PixelToTile(int iPixelX, int iPixelY, int& iTileX, int& iTileY)
{
    switch(m_iMapType)
    {
    case GameMap_Scrolling:
    {
        iTileX = 0; iTileY = 0;
        return true;
    }
    break;
    case GameMap_RectTiles:
    {
        iTileX = (iPixelX-m_iFirstX)/m_iTileWidth;
        iTileY = (iPixelY-m_iFirstY)/m_iTileHeight;
        return true;
    }
    break;
    case GameMap_Isometric:
    {
        int iWidth  = m_iTileWidth/2;
        int iHeight = m_iTileHeight/2;

        int iRealStartX = m_iFirstX;
        int iRealStartY = m_iFirstY - m_iColCount * iHeight;

        int iRealPosX = iPixelX - iRealStartX;
        int iRealPosY = iPixelY - iRealStartY;

        int iSmallCellX = iRealPosX/iWidth;
        int iSmallCellY = iRealPosY/iHeight;

        int iStartCellX = 0;
        int iStartCellY = m_iColCount - 1;

        int iPointX = iWidth;
        int iPointY = m_iColCount * iHeight;

        int iStartPointX = iPointX;
        int iStartPointY = iPointY;

        int iOddFlag = ((iSmallCellX+iSmallCellY) - (iStartCellX+iStartCellY)) % 2;

        if(iOddFlag == 0)  // "/"
        {
            int iPointX1 = iSmallCellX * iWidth;
            int iPointY1 = (iSmallCellY+1) * iHeight;

            int iPointX2 = iPointX1 + iWidth;
            int iPointY2 = iPointY1 - iHeight;

            double fDstX = abs(iPointX2 - iPointX1);
            double fDstY = abs(iPointY2 - iPointY1);

            double fDstPX = abs(iRealPosX - iPointX1);
            double fDstPY = abs(iRealPosY - iPointY1);

            if(fDstPX/fDstPY < fDstX/fDstY)  // top-left
            {
                iPointX = iPointX1;
                iPointY = iPointY1 - iHeight;
            }
            else      // bottom-right
            {

                iPointX = iPointX1 + iWidth;
                iPointY = iPointY1;
            }

        }
        else  // "\"
        {

            int iPointX1 = iSmallCellX * iWidth;
            int iPointY1 = iSmallCellY * iHeight;

            int iPointX2 = iPointX1 + iWidth;
            int iPointY2 = iPointY1 + iHeight;

            double fDstX = abs(iPointX1 - iPointX2);
            double fDstY = abs(iPointY1 - iPointY2);

            double fDstPX = abs(iRealPosX - iPointX2);
            double fDstPY = abs(iRealPosY - iPointY2);

            if(fDstPX/fDstPY < fDstX/fDstY)  // top-right
            {

                iPointX = iPointX1 + iWidth;
                iPointY = iPointY1;
            }
            else          // bottom-left
            {

                iPointX = iPointX1;
                iPointY = iPointY1 + iHeight;
            }
        }

        iTileX = ((iPointX-iStartPointX)/iWidth - (iPointY-iStartPointY)/iHeight)/2;
        iTileY = ((iPointX-iStartPointX)/iWidth + (iPointY-iStartPointY)/iHeight)/2;

        return true;
    }
    break;
    default:
    return false;
    }
}
bool CogeGameMap::TileToPixel(int iTileX, int iTileY, int& iPixelX, int& iPixelY)
{
    switch(m_iMapType)
    {
    case GameMap_Scrolling:
        iPixelX = 0; iPixelY = 0;
        return true;
    break;
    case GameMap_RectTiles:
        iPixelX = m_iFirstX + iTileX * m_iTileWidth  + m_iTileRootX;
        iPixelY = m_iFirstY + iTileY * m_iTileHeight + m_iTileRootY;
        return true;
    break;
    case GameMap_Isometric:
        iPixelX = m_iFirstX + iTileX * m_iTileWidth/2  + iTileY * m_iTileWidth/2  + m_iTileRootX;
        iPixelY = m_iFirstY - iTileX * m_iTileHeight/2 + iTileY * m_iTileHeight/2 - m_iTileHeight/2 + m_iTileRootY;
        return true;
    break;
    default:
    return false;
    }
}

bool CogeGameMap::AlignPixel(int iInPixelX, int iInPixelY, int& iOutPixelX, int& iOutPixelY)
{
    switch(m_iMapType)
    {
    case GameMap_Scrolling:
        iOutPixelX = iInPixelX; iOutPixelY = iInPixelY;
        return true;
    break;
    case GameMap_RectTiles:
    case GameMap_Isometric:
    {
        int iTileX = 0; int iTileY = 0;
        if(PixelToTile(iInPixelX, iInPixelY, iTileX, iTileY))
        return TileToPixel(iTileX, iTileY, iOutPixelX, iOutPixelY);
        else return false;
        break;
    }
    default:
    return false;
    }
}

int CogeGameMap::SetType(int iType)
{
    m_iMapType = iType;
    return m_iMapType;
}
int CogeGameMap::GetType()
{
    return m_iMapType;
}

int CogeGameMap::SetColumnCount(int iCount)
{
    if(iCount < 0 || iCount == m_iColCount) return iCount;
    m_iColCount = iCount;
    m_Tiles.resize(m_iColCount*m_iRowCount);
    m_OriginalTiles.resize(m_iColCount*m_iRowCount);
    m_Data.resize(m_iColCount*m_iRowCount);
    return m_iColCount;
}
int CogeGameMap::SetRowCount(int iCount)
{
    if(iCount < 0 || iCount == m_iRowCount) return iCount;
    m_iRowCount = iCount;
    m_Tiles.resize(m_iColCount*m_iRowCount);
    m_OriginalTiles.resize(m_iColCount*m_iRowCount);
    m_Data.resize(m_iColCount*m_iRowCount);
    return m_iRowCount;
}
int CogeGameMap::GetColumnCount()
{
    return m_iColCount;
}
int CogeGameMap::GetRowCount()
{
    return m_iRowCount;
}

int CogeGameMap::SetMapSize(int iColumnCount, int iRowCount)
{
    if(iColumnCount < 0 || iRowCount < 0 ) return -1;
    if(iColumnCount == m_iColCount && iRowCount == m_iRowCount) return 0;
    m_iColCount = iColumnCount;
    m_iRowCount = iRowCount;
    m_Tiles.resize(m_iColCount*m_iRowCount);
    m_OriginalTiles.resize(m_iColCount*m_iRowCount);
    m_Data.resize(m_iColCount*m_iRowCount);
    return m_Tiles.size();
}

bool CogeGameMap::SetBgImage(CogeImage* pBackground)
{
    if(pBackground == NULL && m_pBackground != NULL)
    {
        m_pBackground->Fire();
        //m_pBackground->GetVideoEngine()->DelImage(m_pBackground->GetName());
        m_pBackground->GetVideoEngine()->DelImage(m_pBackground);
        return true;
    }

    if(pBackground && pBackground != m_pBackground)
    {
        if(m_pBackground)
        {
            m_pBackground->Fire();
            //m_pBackground->GetVideoEngine()->DelImage(m_pBackground->GetName());
            m_pBackground->GetVideoEngine()->DelImage(m_pBackground);
        }

        m_pBackground = pBackground;
        m_pBackground->Hire();

        m_iBgWidth = m_pBackground->GetWidth();
        m_iBgHeight = m_pBackground->GetHeight();
        return true;
    }
    else return false;

}

CogeImage* CogeGameMap::GetBgImage()
{
    return m_pBackground;
}
int CogeGameMap::GetBgWidth()
{
    return m_iBgWidth;
}
int CogeGameMap::GetBgHeight()
{
    return m_iBgHeight;
}

void CogeGameMap::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}

void CogeGameMap::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}

int CogeGameMap::GetTileValue(int iTileX, int iTileY)
{
    if(iTileX < 0 || iTileY < 0) return -1;
    size_t idx = iTileY*m_iColCount+iTileX;
    if(idx >= m_Tiles.size()) return -1;
    else return m_Tiles[idx];
}
int CogeGameMap::SetTileValue(int iTileX, int iTileY, int iValue)
{
    if(iTileX < 0 || iTileY < 0) return -1;
    size_t idx = iTileY*m_iColCount+iTileX;
    if(idx >= m_Tiles.size()) return -1;
    else m_Tiles[idx] = iValue;
    return iValue;
}
void CogeGameMap::ReloadTileValues()
{
    if(m_pEngine)
    {
        /*
        for(int j=0; j<m_iRowCount; j++)
        for(int i=0; i<m_iColCount; i++)
        {
            m_Tiles[j*m_iColCount + i] = m_pEngine->m_GameMapIniFile.ReadInteger(m_sName, "X"+OGE_itoa(i)+"Y"+OGE_itoa(j), 0);
        }
        */

        int iCount = m_Tiles.size();
        for(int i=0; i<iCount; i++) m_Tiles[i] = m_OriginalTiles[i];

    }
}

void CogeGameMap::Reset()
{
    int iCount = m_Tiles.size();
    for(int i=0; i<iCount; i++)
    {
        m_Tiles[i] = m_OriginalTiles[i];
        m_Data[i] = 0;
    }
}

void CogeGameMap::InitTileValues(int iInitValue)
{
    int iCount = m_Tiles.size();
    for(int i=0; i<iCount; i++) m_Tiles[i] = iInitValue;
}
int CogeGameMap::GetTileData(int iTileX, int iTileY)
{
    if(iTileX < 0 || iTileY < 0) return -1;
    size_t idx = iTileY*m_iColCount+iTileX;
    if(idx >= m_Data.size()) return -1;
    else return m_Data[idx];
}
int CogeGameMap::SetTileData(int iTileX, int iTileY, int iValue)
{
    if(iTileX < 0 || iTileY < 0) return -1;
    size_t idx = iTileY*m_iColCount+iTileX;
    if(idx >= m_Data.size()) return -1;
    else m_Data[idx] = iValue;
    return iValue;
}
void CogeGameMap::InitTileData(int iInitValue)
{
    int iCount = m_Data.size();
    for(int i=0; i<iCount; i++) m_Data[i] = iInitValue;
}

bool CogeGameMap::GetEightDirectionMode()
{
    return m_PathFinder.GetEightDirectionMode();
}
void CogeGameMap::SetEightDirectionMode(bool value)
{
    m_PathFinder.SetEightDirectionMode(value);
}

ogePointArray* CogeGameMap::FindWay(int iStartX, int iStartY, int iEndX, int iEndY, ogePointArray* pWay)
{
    return m_PathFinder.FindWay(iStartX, iStartY, iEndX, iEndY, pWay);
}
ogePointArray* CogeGameMap::FindRange(int iPosX, int iPosY, int iMovementPoints, ogePointArray* pRange)
{
    return m_RangeFinder.FindRange(iPosX, iPosY, iMovementPoints, pRange);
}


/*---------------------------------- Sprite Group -----------------------------------------*/

CogeSpriteGroup::CogeSpriteGroup(const std::string& sTheName, CogeEngine* pTheEngine):
m_pEngine(pTheEngine),
m_pCurrentScene(NULL),
m_sName(sTheName)
{
    //m_iTotalUsers = 0;

    m_pRootSprite = NULL;

    m_sTemplate = "";
    //m_sRoot = "";
}

CogeSpriteGroup::~CogeSpriteGroup()
{
	Clear();
}

/*
void CogeSpriteGroup::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}
void CogeSpriteGroup::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}
*/

const std::string& CogeSpriteGroup::GetName()
{
    return m_sName;
}

const std::string& CogeSpriteGroup::GetTemplateSpriteName()
{
    return m_sTemplate;
}
/*
std::string CogeSpriteGroup::GetRootSpriteBaseName()
{
    return m_sRoot;
}
*/

CogeSprite* CogeSpriteGroup::GetRootSprite()
{
    return m_pRootSprite;
}

CogeSprite* CogeSpriteGroup::AddSprite(CogeSprite* pSprite)
{
    if (pSprite == NULL) return NULL;

    /*
    std::string sSpriteName = sCurrentName;
    if(sSpriteName.length() <= 0) sSpriteName = pSprite->GetName();
    if(sSpriteName.length() <= 0) sSpriteName = pSprite->GetBaseName();

	ogeSpriteMap::iterator its;

	CogeSprite* pMatchedSprite = NULL;
    its = m_SpriteMap.find(sSpriteName);
	if (its != m_SpriteMap.end())
	{
	    pMatchedSprite = its->second;
	    if(pSprite == pMatchedSprite) return pSprite;
        else RemoveSprite(pMatchedSprite);

	}
	*/

	std::string sSpriteName = pSprite->GetName();
	if(sSpriteName.length() <= 0) sSpriteName = pSprite->GetBaseName();

    pSprite->m_pCurrentGroup = this;
	m_SpriteMap.insert(ogeSpriteMap::value_type(sSpriteName, pSprite));

	if(m_pRootSprite == NULL) m_pRootSprite = pSprite;

	int iCount = pSprite->m_Children.size();
    if(iCount > 0)
    {
        ogeSpriteMap::iterator it = pSprite->m_Children.begin();
        while(iCount > 0)
        {
            //AddSprite(it->second, sSpriteName + "_" + it->second->GetBaseName());
            AddSprite(it->second);
            it++;
            iCount--;
        }
    }

    return pSprite;
}

CogeSprite* CogeSpriteGroup::AddSprite(const std::string& sSpriteName, const std::string& sBaseName)
{
	if (sBaseName.length() == 0)
    {
        OGE_Log("Error in CogeSpriteGroup::AddSprite(): Missing sprite template name.\n");
        return NULL;
    }

    std::string sRealName = sSpriteName;

    if (sRealName.length() <= 0) sRealName = m_sName + "_" + sBaseName; // auto name it ...
    //else sRealName = m_sName + "_" + sRealName; // auto name it ...

    ogeSpriteMap::iterator its = m_SpriteMap.find(sSpriteName);
	if (its != m_SpriteMap.end()) return its->second;

    //CogeSprite* pTheSprite = m_pEngine->GetSprite(sRealName, sBaseName);
    CogeSprite* pTheSprite = m_pEngine->NewSprite(sRealName, sBaseName);

	if (pTheSprite == NULL) return NULL;

	//sRealName = sSpriteName;
    //if (sRealName.length() <= 0) sRealName = m_sName + "_" + sBaseName; // auto name it ...

	//return AddSprite(pTheSprite, sRealName);

	return AddSprite(pTheSprite);

}
int CogeSpriteGroup::RemoveSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator its;

	CogeSprite* pMatchedSprite = NULL;
    its = m_SpriteMap.find(sSpriteName);
	if (its != m_SpriteMap.end())
	{
	    pMatchedSprite = its->second;

	    int iCount = pMatchedSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pMatchedSprite->m_Children.begin();
            while(iCount > 0)
            {
                RemoveSprite(it->second);
                it++;
                iCount--;
            }
        }

	    pMatchedSprite->m_pCurrentGroup = NULL;
        m_SpriteMap.erase(its);

        if(m_pRootSprite == pMatchedSprite) m_pRootSprite = NULL;

        //pMatchedSprite->Fire();
        if(pMatchedSprite->m_pCurrentScene == NULL) m_pEngine->DelSprite(pMatchedSprite);

        return 1;
	}
	else return 0;
}

int CogeSpriteGroup::RemoveSprite(CogeSprite* pSprite)
{
    if(!pSprite) return 0;

    bool bFound = false;
    ogeSpriteMap::iterator its;

    for(its = m_SpriteMap.begin(); its != m_SpriteMap.end(); its++)
    {
        if(its->second == pSprite)
        {
            bFound = true;
            break;
        }
    }

	if (bFound)
	{
	    CogeSprite* pMatchedSprite = its->second;

	    int iCount = pMatchedSprite->m_Children.size();
        if(iCount > 0)
        {
            ogeSpriteMap::iterator it = pMatchedSprite->m_Children.begin();
            while(iCount > 0)
            {
                RemoveSprite(it->second);
                it++;
                iCount--;
            }
        }

	    pMatchedSprite->m_pCurrentGroup = NULL;
        m_SpriteMap.erase(its);

        if(m_pRootSprite == pMatchedSprite) m_pRootSprite = NULL;

        //pMatchedSprite->Fire();
        if(pMatchedSprite->m_pCurrentScene == NULL) m_pEngine->DelSprite(pMatchedSprite);

        return 1;
	}
	else return 0;
}

CogeSprite* CogeSpriteGroup::FindSprite(const std::string& sSpriteName)
{
    ogeSpriteMap::iterator it;

    it = m_SpriteMap.find(sSpriteName);
	if (it != m_SpriteMap.end()) return it->second;
	else return NULL;
}

CogeSprite* CogeSpriteGroup::GetFreeSprite(int iSprType, int iSprClassTag, int iSprTag)
{
    if(m_pCurrentScene)
    {
        int iCount = m_SpriteMap.size();
        if(iCount <= 0) return NULL;

        CogeSprite* pMatchedSprite = NULL;
        ogeSpriteMap::iterator its = m_SpriteMap.begin();

        while (iCount > 0)
        {
            pMatchedSprite = its->second;
            if(pMatchedSprite->m_pCurrentScene == m_pCurrentScene &&
               //pMatchedSprite->m_iUnitType != Spr_Plot &&
               //pMatchedSprite->m_iUnitType != Spr_Timer &&
               pMatchedSprite->m_bBusy == false)
            {
                //printf("Free Sprite Found.\n");
                if( (iSprType < 0 || iSprType == pMatchedSprite->m_iUnitType)
                   && (iSprClassTag < 0 || iSprClassTag == pMatchedSprite->m_iClassTag)
                   && (iSprTag < 0 || iSprTag == pMatchedSprite->m_iObjectTag) )
                return pMatchedSprite;
            }

            its++;
            iCount--;
        }
    }
    //printf("Free Sprite NOT Found.\n");
    return NULL;
}

/*
CogeSprite* CogeSpriteGroup::GetFreePlot()
{
    if(m_pCurrentScene)
    {
        int iCount = m_SpriteMap.size();
        if(iCount <= 0) return NULL;

        CogeSprite* pMatchedSprite = NULL;
        ogeSpriteMap::iterator its = m_SpriteMap.begin();

        while (iCount > 0)
        {
            pMatchedSprite = its->second;
            if(pMatchedSprite->m_pCurrentScene == m_pCurrentScene &&
               pMatchedSprite->m_iUnitType == Spr_Plot &&
               pMatchedSprite->m_pMasterSpr == NULL)
            {
                //printf("Free Sprite Found.\n");
                return pMatchedSprite;
            }

            its++;
            iCount--;
        }
    }
    //printf("Free Sprite NOT Found.\n");
    return NULL;
}

CogeSprite* CogeSpriteGroup::GetFreeTimer()
{
    if(m_pCurrentScene)
    {
        int iCount = m_SpriteMap.size();
        if(iCount <= 0) return NULL;

        CogeSprite* pMatchedSprite = NULL;
        ogeSpriteMap::iterator its = m_SpriteMap.begin();

        while (iCount > 0)
        {
            pMatchedSprite = its->second;
            if(pMatchedSprite->m_pCurrentScene == m_pCurrentScene &&
               pMatchedSprite->m_iUnitType == Spr_Timer &&
               pMatchedSprite->m_bEnableTimer == false)
            {
                //printf("Free Sprite Found.\n");
                return pMatchedSprite;
            }

            its++;
            iCount--;
        }
    }
    //printf("Free Sprite NOT Found.\n");
    return NULL;
}
*/

int CogeSpriteGroup::GetSpriteCount()
{
    return m_SpriteMap.size();
}

CogeSprite* CogeSpriteGroup::GetSpriteByIndex(int idx)
{
    CogeSprite* pSprite = NULL;
    int iCount = m_SpriteMap.size();
    ogeSpriteMap::iterator it = m_SpriteMap.begin();
    ogeSpriteMap::iterator ite = m_SpriteMap.end();
    for(int i=0; i<iCount; i++)
    {
        if(it == ite) break;
        if(idx == i)
        {
            pSprite = it->second;
            break;
        }
        it++;
    }
    return pSprite;
}

CogeSprite* CogeSpriteGroup::GetNpcSprite(size_t iNpcId)
{
    if(iNpcId >= m_Npcs.size()) return NULL;
    else return m_Npcs[iNpcId];
}

int CogeSpriteGroup::LoadSpritesFromScene(const std::string& sSceneName)
{
    std::string sConfigFileName = m_pEngine->m_ObjectIniFile.ReadFilePath("Scenes", sSceneName, "");

    if (sConfigFileName.length() <= 0) return 0;

    int rsl = 0;

    CogeIniFile iniFile;

    bool bLoadedConfig = m_pEngine->LoadIniFile(&iniFile, sConfigFileName, m_pEngine->m_bPackedScene, m_pEngine->m_bEncryptedScene);

    if (bLoadedConfig)
    {
        iniFile.SetCurrentPath(OGE_GetAppGameDir());

        // section sprite ...

        m_Npcs.clear();

        ogeIntStrMap SpriteParentMap;

        int iSpriteCount = iniFile.ReadInteger("Sprites", "Count",  0);

        std::string sConfigGroup = "G_" + m_sName;

        if(m_pEngine->m_bBinaryScript == false && iSpriteCount > 0)
        {
            m_pEngine->m_pScripter->BeginConfigGroup(sConfigGroup);

            m_pEngine->m_pScripter->RegisterEnum(m_sName);

            for(int k=1; k<=iSpriteCount; k++)
            {
                std::string sIdx = "Sprite_" + OGE_itoa(k);
                std::string sSpriteName = iniFile.ReadString(sIdx, "Name", "");
                if(sSpriteName.length() > 0)
                    m_pEngine->m_pScripter->RegisterEnumValue(m_sName, sSpriteName, k);
            }

            //m_pEngine->m_pScripter->EndConfigGroup();

        }

        for(int k=1; k<=iSpriteCount; k++)
        {

            std::string sIdx = "Sprite_" + OGE_itoa(k);
            std::string sSpriteName = iniFile.ReadString(sIdx, "Name", "");

            std::string sBaseSprite = iniFile.ReadString(sIdx, "Base", "");

            if(sBaseSprite.length() <= 0) continue;

            CogeSprite* pTheSprite = NULL;

            pTheSprite = AddSprite(sSpriteName, sBaseSprite);

            if (pTheSprite == NULL) continue;
            //else SetNpcSprite(pTheSprite, k-1);

            m_Npcs.push_back(pTheSprite);

            sSpriteName = pTheSprite->GetName();

            std::string sLocalScript = iniFile.ReadString(sIdx, "Script", "");
            if(sLocalScript.length() > 0) pTheSprite->LoadLocalScript(&iniFile, sIdx);

            // the new image of the sprite
            std::string sNewImage = iniFile.ReadString(sIdx, "AltImage", "");
            if(sNewImage.length() > 0)
            {
                CogeImage* pNewImage = m_pEngine->GetImage(sNewImage);
                if(pNewImage) pTheSprite->SetImage(pNewImage);
            }

            // and set status of the sprite

            int iDir     = iniFile.ReadInteger(sIdx, "Dir",   Dir_Down);
            int iAct     = iniFile.ReadInteger(sIdx, "Act",   Act_Stand);

            pTheSprite->SetAnima(iDir, iAct);

            int iSpeedX  = iniFile.ReadInteger(sIdx, "SpeedX", -1);
            int iSpeedY  = iniFile.ReadInteger(sIdx, "SpeedY", -1);

            int iTimerInterval = iniFile.ReadInteger(sIdx, "TimerInterval", -1);
            if(iTimerInterval > 0)
            {
                if(pTheSprite->m_iUnitType == Spr_Timer)
                    pTheSprite->m_iDefaultTimerInterval = iTimerInterval;
            }

            int iObjectTag = iniFile.ReadInteger(sIdx, "Tag", -1);
            if(iObjectTag > 0) pTheSprite->SetObjectTag(iObjectTag);

            if( iSpeedX >= 0 && iSpeedY >= 0 ) pTheSprite->SetSpeed(iSpeedX, iSpeedY);

            //int iActive  = iniFile.ReadInteger(sIdx, "Active",  0);
            int iPosX    = iniFile.ReadInteger(sIdx, "PosX",    0);
            int iPosY    = iniFile.ReadInteger(sIdx, "PosY",    0);
            int iPosZ    = iniFile.ReadInteger(sIdx, "PosZ",    0);

            //if(iActive > 0) ActivateSprite(sSpriteName);

            pTheSprite->SetPos(iPosX, iPosY, iPosZ);

            pTheSprite->m_iRelativeX  = iniFile.ReadInteger(sIdx, "RelativeX", 0);
            pTheSprite->m_iRelativeY  = iniFile.ReadInteger(sIdx, "RelativeY", 0);

            int iRelative = iniFile.ReadInteger(sIdx, "Relative",   -1);
            if(iRelative >= 0) pTheSprite->m_bIsRelative = iRelative > 0;

            int iVisible = iniFile.ReadInteger(sIdx, "Visible",   -1);
            if(iVisible >= 0) pTheSprite->m_bVisible = iVisible > 0;

            int iEnableAnima = iniFile.ReadInteger(sIdx, "EnableAnima", -1);
            if(iEnableAnima >= 0) pTheSprite->m_bEnableAnima = iEnableAnima > 0;

            int iEnableMovement = iniFile.ReadInteger(sIdx, "EnableMovement", -1);
            if(iEnableMovement >= 0) pTheSprite->m_bEnableMovement = iEnableMovement > 0;

            int iEnableCollision = iniFile.ReadInteger(sIdx, "EnableCollision", -1);
            if(iEnableCollision >= 0) pTheSprite->m_bEnableCollision = iEnableCollision > 0;

            int iEnableInput = iniFile.ReadInteger(sIdx, "EnableInput", -1);
            if(iEnableInput >= 0) pTheSprite->m_bEnableInput = iEnableInput > 0;

            int iEnableFocus = iniFile.ReadInteger(sIdx, "EnableFocus", -1);
            if(iEnableFocus >= 0) pTheSprite->m_bEnableFocus = iEnableFocus > 0;

            int iEnableDrag = iniFile.ReadInteger(sIdx, "EnableDrag", -1);
            if(iEnableDrag >= 0) pTheSprite->m_bEnableDrag = iEnableDrag > 0;

            //  reset game data ...
            CogeGameData* pGameData = pTheSprite->GetGameData();

            if(pGameData)
            {
                std::string sKeyName = "";
                std::string sValue = "";
                int iValue = 0;
                double fValue = 0;

                int iIntCount = pGameData->GetIntVarCount();
                for(int i=1; i<=iIntCount; i++)
                {
                    sKeyName = "IntVar" + OGE_itoa(i);
                    sValue = iniFile.ReadString(sIdx, sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        iValue = iniFile.ReadInteger(sIdx, sKeyName, 0);
                        pGameData->SetIntVar(i, iValue);
                    }
                }

                int iFloatCount = pGameData->GetFloatVarCount();
                for(int i=1; i<=iFloatCount; i++)
                {
                    sKeyName = "FloatVar" + OGE_itoa(i);
                    sValue = iniFile.ReadString(sIdx, sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        fValue = iniFile.ReadFloat(sIdx, sKeyName, 0);
                        pGameData->SetFloatVar(i, fValue);
                    }
                }

                int iStrCount = pGameData->GetStrVarCount();
                for(int i=1; i<=iStrCount; i++)
                {
                    sKeyName = "StrVar" + OGE_itoa(i);
                    sValue = iniFile.ReadString(sIdx, sKeyName, "\n");
                    if(sValue.length() == 1 && sValue[0] == '\n') continue;
                    else
                    {
                        pGameData->SetStrVar(i, sValue);
                    }
                }

            }

            // bind parent in scene ...
            std::string sSpriteParentName = iniFile.ReadString(sIdx, "Parent", "");
            if(sSpriteParentName.length() > 0)
            {
                int iSprId = (int) pTheSprite;
                SpriteParentMap.insert(ogeIntStrMap::value_type(iSprId, sSpriteParentName));
            }

            rsl++;

        }


        if(m_pEngine->m_bBinaryScript == false && iSpriteCount > 0)
        {
            m_pEngine->m_pScripter->EndConfigGroup();
            m_pEngine->m_pScripter->RemoveConfigGroup(sConfigGroup);
        }

        // bind parent in scene ...

        iSpriteCount = SpriteParentMap.size();

        if(iSpriteCount > 0)
        {
            //CogeSprite* pMatchedSprite = NULL;
            ogeIntStrMap::iterator its = SpriteParentMap.begin();

            while (iSpriteCount > 0)
            {
                CogeSprite* pMatchedSprite = (CogeSprite*) its->first;
                std::string sSpriteParentName = its->second;

                pMatchedSprite->SetParent(sSpriteParentName, pMatchedSprite->m_iRelativeX, pMatchedSprite->m_iRelativeY);

                its++;

                iSpriteCount--;

            }
        }

        SpriteParentMap.clear();

    }

    return rsl;

}

CogeScene* CogeSpriteGroup::GetCurrentScene()
{
    return m_pCurrentScene;
}
void CogeSpriteGroup::SetCurrentScene(CogeScene* pCurrentScene)
{
    if(m_pCurrentScene && m_pCurrentScene != pCurrentScene)
    {
        int iCount = m_SpriteMap.size();
        ogeSpriteMap::iterator its = m_SpriteMap.begin();
        while (iCount > 0)
        {
            m_pCurrentScene->RemoveSprite(its->second);
            its++;
            iCount--;
        }
    }

    if(pCurrentScene && m_pCurrentScene != pCurrentScene)
    {
        int iCount = m_SpriteMap.size();
        ogeSpriteMap::iterator its = m_SpriteMap.begin();
        while (iCount > 0)
        {
            pCurrentScene->AddSprite(its->second);
            its++;
            iCount--;
        }
    }

    if(m_pCurrentScene != pCurrentScene) m_pCurrentScene = pCurrentScene;
}
void CogeSpriteGroup::SetCurrentScene(const std::string& sSceneName)
{
    CogeScene* pCurrentScene = m_pEngine->FindScene(sSceneName);
    SetCurrentScene(pCurrentScene);
}

void CogeSpriteGroup::Clear()
{
    ogeSpriteMap::iterator its;
	CogeSprite* pMatchedSprite = NULL;

	its = m_SpriteMap.begin();

	while (its != m_SpriteMap.end())
	{
		pMatchedSprite = its->second;
		if(pMatchedSprite->m_pCurrentScene)
		{
		    pMatchedSprite->m_pCurrentScene->RemoveSprite(pMatchedSprite);
		    pMatchedSprite->m_pCurrentScene = NULL;
		}
		pMatchedSprite->m_pCurrentGroup = NULL;
		m_SpriteMap.erase(its);
		if(pMatchedSprite->m_pCurrentScene == NULL) m_pEngine->DelSprite(pMatchedSprite);
		its = m_SpriteMap.begin();
	}

	m_sTemplate = "";
    //m_sRoot = "";

    m_pRootSprite = NULL;

    m_Npcs.clear();
}

int CogeSpriteGroup::GenSprites(const std::string& sTempletSpriteName, int iCount)
{
    std::string sConfigFileName = m_pEngine->m_ObjectIniFile.ReadFilePath("Sprites", sTempletSpriteName, "");
    if (sConfigFileName.length() <= 0) return 0;

    int iStartIdx = m_SpriteMap.size();

    int k = 0;

    for(int i=1; i<=iCount; i++)
    {
        std::string sSpriteName = m_sName + "_" + sTempletSpriteName + "_" + OGE_itoa(iStartIdx+i);
        CogeSprite* pNewSprite = AddSprite(sSpriteName, sTempletSpriteName);
        if(pNewSprite) k++;
    }

    m_sTemplate = sTempletSpriteName;

    return k;
}

int CogeSpriteGroup::LoadMembers(const std::string& sConfigFileName)
{
    CogeIniFile ini;

    if (ini.Load(sConfigFileName))
    {
        // load ...
        return m_SpriteMap.size();
    }
    else return -1;
}

/*---------------- Sprite -----------------*/

bool CogeSprite::operator < (const CogeSprite& OtherSpr)
{
	if (m_iPosZ < OtherSpr.m_iPosZ) return true;
	else if (m_iPosZ == OtherSpr.m_iPosZ)
	{
		if (m_iPosY < OtherSpr.m_iPosY) return true;
		else if (m_iPosY == OtherSpr.m_iPosY)
		{
			if (m_iPosX < OtherSpr.m_iPosX) return true;
			else return false;
		}
		else return false;
	}
	else return false;
}

CogeSprite::CogeSprite(const std::string& sName, CogeEngine* pEngine, int iUnitType):
m_pEngine(pEngine),
m_pCommonScript(NULL),
m_pLocalScript(NULL),
m_pActiveUpdateScript(NULL),
m_pCurrentScene(NULL),
m_pCurrentGroup(NULL),
m_pParent(NULL),
m_pRoot(NULL),
m_pCollidedSpr(NULL),
m_pPlotSpr(NULL),
m_pMasterSpr(NULL),
m_pReplacedSpr(NULL),
m_pCurrentAnima(NULL),
m_pCurrentPath(NULL),
m_pCustomEvents(NULL),
m_pGameData(NULL),
m_pCustomData(NULL),
//m_pCurrentSound(NULL),
m_pLightMap(NULL),
//m_pAnimaEffect(NULL),
m_pTextInputter(NULL),
m_sName(sName),
m_iUnitType(iUnitType),
m_iState(-1)
{
    memset((void*)&m_CommonEvents[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);
    memset((void*)&m_LocalEvents[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);

    memset((void*)&m_PlotTriggers[0],  0, sizeof(int) * _OGE_MAX_EVENT_COUNT_);

    memset((void*)&m_RelatedSprites[0],  0, sizeof(int) * _OGE_MAX_REL_SPR_);
    memset((void*)&m_RelatedGroups[0],  0, sizeof(int) * _OGE_MAX_REL_SPR_);

    //memset(&m_AnimaEffect, 0, sizeof(m_AnimaEffect));

    m_pAnimaEffect = new CogeFrameEffect();

    m_pLocalPath = new CogePath();
    m_pLocalPath->m_iType = 3; // ... special type ?
    m_pLocalPath->m_iSegmentLength = 0;
    m_pLocalPath->m_sName = m_sName + "_LocalPath";

    m_sBaseName = "";

    m_iScriptState = -1;
    //m_iTotalUsers  = 0;

    m_bVisible = true;
    //m_bEnabled = true;

    m_bEnableCollision = false;

    m_bEnableInput = false;

    m_bEnableFocus = false;
    m_bEnableDrag = false;

    m_bEnableAnima = false;

    m_bEnableMovement = false;

    m_bIsRelative = false;

    m_bDefaultDraw = true;

    m_bHasDefaultData = false;

    m_bOwnCustomData = false;

    m_bBusy = false;

    m_bActive = false;

    //m_iLastSoundCode = -1;

    m_iPathStep = 0;
    m_bPathAutoStepIn = false;
    m_bPathStepIn = false;
    m_iPathStepInterval = 0;
    m_iVirtualStepCount = -1;

    m_iMovingIntervalX = 50;
    m_iMovingTimeX = 0;
    m_iMovingIntervalY = 50;
    m_iMovingTimeY = 0;

    m_iInputWinPosX = 10;
    m_iInputWinPosY = 10;

    m_iDir = 1;
    m_iAct = 1;

    m_iPosX = 0;
    m_iPosY = 0;
    m_iPosZ = 0;

    m_iClassTag = 0;
    m_iObjectTag = 0;

    //m_iLightLocalX = 0;
    //m_iLightLocalY = 0;
    //m_iLightLocalWidth = 0;
    //m_iLightLocalHeight = 0;
    //m_iLightRelativeX = 0;
    //m_iLightRelativeY = 0;

    m_iDefaultWidth  = 32;
    m_iDefaultHeight = 32;
    m_iDefaultRootX  = 16;
    m_iDefaultRootY  = 16;

    m_bEnableLightMap = false;

    m_bGetMouse = false;
    m_bMouseIn = false;
    m_bMouseDown = false;
    m_bMouseDrag = false;

    m_bEnableTimer   = false;
    m_bEnablePlot = false;
    //m_bJoinedPlot = true;

    m_iTimerLastTick = 0;
    m_iTimerInterval = 1000;
    m_iTimerEventCount = 0;

    m_iDefaultTimerInterval = 0;

    m_iActiveUpdateFunctionId = -1;

    m_iTotalPlotRounds = 0;
    m_iCurrentPlotRound = 0;

    //m_iThinkStepTime = 0;
    //m_iThinkStepInterval = 0;

    m_iLastDisplayFlag = 0;

}

CogeSprite::~CogeSprite()
{
    Finalize();

    if(m_pAnimaEffect) {delete m_pAnimaEffect;m_pAnimaEffect=NULL;}

    if(m_pLocalPath) {delete m_pLocalPath; m_pLocalPath = NULL;}

    if(m_pCustomEvents != NULL) {delete m_pCustomEvents; m_pCustomEvents = NULL;}

}

const std::string& CogeSprite::GetName()
{
    return m_sName;
}

const std::string& CogeSprite::GetBaseName()
{
    return m_sBaseName;
}

int CogeSprite::CallEvent(int iEventCode)
{
    if (m_iScriptState < 0) return -1;

    if(m_pEngine->m_bFreeze)
    {
        //if(iEventCode == Event_OnUpdate) return -1;
        //if(m_iUnitType < Spr_Window || m_iUnitType > Spr_InputText)
        if(m_iUnitType < Spr_Plot || m_iUnitType > Spr_InputText)
        {
            if(iEventCode >= Event_OnMouseOver && iEventCode <= Event_OnKeyUp) return -1;
        }

    }

    //if(m_pCurrentScene) m_pCurrentScene->m_pCurrentSpr = this;
    //m_pEngine->m_pCurrentGameSprite = this;

    //if(iEventCode < 0 || iEventCode >= _OGE_MAX_EVENT_COUNT_) return;

    if(iEventCode == Event_OnUpdate && m_iUnitType == Spr_Plot)
    {
        if(!m_bEnablePlot || m_pActiveUpdateScript == NULL) return -1;

        //if(m_pActiveUpdateScript->IsSuspended() || m_pActiveUpdateScript->IsFinished()) return -1;
        if(m_pActiveUpdateScript->IsSuspended()) return -1;

        if(m_iActiveUpdateFunctionId < 0) return -1;

        if(m_iCurrentPlotRound == 0)
        {
            if(m_pCurrentScene)
            {
                m_pCurrentScene->m_pCurrentSpr = this;
                m_pEngine->m_pCurrentGameSprite = this;
                CallEvent(Event_OnGetFocus);
            }
            m_pActiveUpdateScript->Reset();
        }

        if(!m_pActiveUpdateScript->IsFunctionReady(m_iActiveUpdateFunctionId))
        {
            //printf("init function\n");
            m_pActiveUpdateScript->PrepareFunction(m_iActiveUpdateFunctionId);
        }

        CogeScript* pCurrentScript = m_pActiveUpdateScript;

        int rsl = pCurrentScript->Execute();

        if(pCurrentScript->IsFinished())
        {
            //pCurrentScript->CallGC();
            m_iCurrentPlotRound++;
            if((!m_bEnablePlot) || (m_iTotalPlotRounds > 0 && m_iCurrentPlotRound >= m_iTotalPlotRounds))
            {
                DisablePlot();
                if(m_pCurrentScene)
                {
                    m_pCurrentScene->m_pCurrentSpr = this;
                    m_pEngine->m_pCurrentGameSprite = this;
                    CallEvent(Event_OnLoseFocus);
                }
            }
        }

        return rsl;

    }

    m_bCallingCustomEvents = false;

    m_pCurrentScript = NULL;
    m_iCurrentEventCode = -1;

    if(m_pLocalScript)
    {
        int iEventId = m_LocalEvents[iEventCode];
        if(iEventId >= 0)
        {
            m_pCurrentScript = m_pLocalScript;
            m_iCurrentEventCode = iEventCode;

            m_pLocalScript->CallFunction(iEventId);
            //m_pLocalScript->CallGC();
        }
    }

    if(m_pCommonScript && !m_pCurrentScript)
    {
        int iEventId = m_CommonEvents[iEventCode];
        if(iEventId >= 0)
        {
            m_pCommonScript->CallFunction(iEventId);
            //m_pCommonScript->CallGC();
        }
    }

    if(m_iUnitType != Spr_Plot && m_pPlotSpr && m_PlotTriggers[iEventCode] > 0)
    {
        m_PlotTriggers[iEventCode] = 0;
        m_pPlotSpr->ResumeUpdateScript();
    }

    return -1;
}

void CogeSprite::CallBaseEvent()
{
    if (m_iScriptState < 0 || m_iUnitType == Spr_Plot) return;

    if(m_bCallingCustomEvents && m_pCustomEvents)
    {
        m_pCustomEvents->CallBaseEvent();
        return;
    }

    if(m_pCurrentScript && m_pCurrentScript == m_pLocalScript)
    {
        if(m_pCommonScript && m_iCurrentEventCode >= 0)
        {
            int iEventId = m_CommonEvents[m_iCurrentEventCode];
            if(iEventId >= 0)
            {
                m_pCommonScript->CallFunction(iEventId);
                ////m_pCommonScript->CallGC();
            }
        }
    }
}


int CogeSprite::BindAnima(int iDir, int iAct, CogeAnima* pTheAnima)
{
    if(!pTheAnima) return -1;
    int idx = iAct<<8 | iDir;
    ogeAnimaMap::iterator it;
	CogeAnima* pMatchedAnima = NULL;
    it = m_Animation.find(idx);
    if(it != m_Animation.end())
	{
		pMatchedAnima = it->second;
		if(pTheAnima == pMatchedAnima) return 0;
		m_Animation.erase(it);
		delete pMatchedAnima;
	}

    m_Animation.insert(ogeAnimaMap::value_type(idx, pTheAnima));
	pTheAnima->m_pSprite = this;
	return 1;
}

CogeAnima* CogeSprite::FindAnima(int iAnimaCode)
{
    ogeAnimaMap::iterator it = m_Animation.find(iAnimaCode);

    if(it != m_Animation.end()) return it->second;
    else return NULL;
}

int CogeSprite::SetAnima(int iDir, int iAct)
{
    if(m_iDir == iDir && m_iAct == iAct && m_pCurrentAnima) return 0;
    else
    {
        CogeAnima* pMatchedAnima = FindAnima( iAct<<8 | iDir );
        if(pMatchedAnima)
        {
            m_iDir = iDir;
            m_iAct = iAct;
            m_pCurrentAnima = pMatchedAnima;
            m_pCurrentAnima->Reset();
            return 1;
        }
        else return -1;
    }
}

void CogeSprite::ResetAnima()
{
    if(m_pCurrentAnima) m_pCurrentAnima->Reset();
}

CogeAnima* CogeSprite::GetCurrentAnima()
{
    return m_pCurrentAnima;
}

CogeImage* CogeSprite::GetCurrentImage()
{
    if(m_pCurrentAnima) return m_pCurrentAnima->GetImage();
    else return NULL;
}

CogeImage* CogeSprite::GetAnimaImage(int iDir, int iAct)
{
    CogeAnima* pMatchedAnima = FindAnima( iAct<<8 | iDir );
    if(pMatchedAnima) return pMatchedAnima->GetImage();
    else return NULL;
}

int CogeSprite::SetImage(CogeImage* pImage)
{
    int iRsl = 0;
	CogeAnima* pMatchedAnima = NULL;
    for(ogeAnimaMap::iterator it = m_Animation.begin(); it != m_Animation.end(); it++)
    {
        pMatchedAnima = it->second;
        if(pMatchedAnima->SetImage(pImage)) iRsl++;
    }
	return iRsl;
}
bool CogeSprite::SetAnimaImage(int iDir, int iAct, CogeImage* pImage)
{
    CogeAnima* pMatchedAnima = FindAnima( iAct<<8 | iDir );
    if(pMatchedAnima) return pMatchedAnima->SetImage(pImage);
    else return false;
}

bool CogeSprite::GetDefaultDraw()
{
    return m_bDefaultDraw;
}
void CogeSprite::SetDefaultDraw(bool value)
{
    m_bDefaultDraw = value;
}

CogeScene* CogeSprite::GetCurrentScene()
{
    return m_pCurrentScene;
}
CogeSpriteGroup* CogeSprite::GetCurrentGroup()
{
    return m_pCurrentGroup;
}

void CogeSprite::SetCustomData(CogeGameData* pGameData, bool bOwnIt)
{
    m_pCustomData = pGameData;
    if(m_pCustomData && bOwnIt) m_bOwnCustomData = true;
    else m_bOwnCustomData = false;
}
CogeGameData* CogeSprite::GetCustomData()
{
    return m_pCustomData;
}

CogeSprite* CogeSprite::GetCollidedSpr()
{
    return m_pCollidedSpr;
}
CogeSprite* CogeSprite::GetPlotSpr()
{
    return m_pPlotSpr;
}
void CogeSprite::SetPlotSpr(CogeSprite* pSprite)
{
    m_pPlotSpr = pSprite;
    //if(pSprite) pSprite->SetMasterSpr(this);
}
CogeSprite* CogeSprite::GetMasterSpr()
{
    return m_pMasterSpr;
}
void CogeSprite::SetMasterSpr(CogeSprite* pSprite)
{
    m_pMasterSpr = pSprite;
}

CogeSprite* CogeSprite::GetReplacedSpr()
{
    return m_pReplacedSpr;
}
void CogeSprite::SetReplacedSpr(CogeSprite* pSprite)
{
    m_pReplacedSpr = pSprite;
}

CogeSprite* CogeSprite::GetParentSpr()
{
    return m_pParent;
}

CogeSprite* CogeSprite::GetRootSpr()
{
    return m_pRoot;
}

CogeSprite* CogeSprite::GetRelatedSpr(int iRelIdx)
{
    if(iRelIdx < 0 || iRelIdx >= _OGE_MAX_REL_SPR_) return NULL;
    else return m_RelatedSprites[iRelIdx];
}
void CogeSprite::SetRelatedSpr(CogeSprite* pSprite, int iRelIdx)
{
    //m_pRelatedSpr = pSprite;
    if(iRelIdx < 0 || iRelIdx >= _OGE_MAX_REL_SPR_) return;
    else m_RelatedSprites[iRelIdx] = pSprite;
}

CogeSpriteGroup* CogeSprite::GetRelatedGroup(int iRelIdx)
{
    if(iRelIdx < 0 || iRelIdx >= _OGE_MAX_REL_SPR_) return NULL;
    else return m_RelatedGroups[iRelIdx];
}
void CogeSprite::SetRelatedGroup(CogeSpriteGroup* pGroup, int iRelIdx)
{
    if(iRelIdx < 0 || iRelIdx >= _OGE_MAX_REL_SPR_) return;
    else m_RelatedGroups[iRelIdx] = pGroup;
}

int CogeSprite::GetChildCount()
{
    return m_Children.size();
}
CogeSprite* CogeSprite::GetChildByIndex(int idx)
{
    CogeSprite* pSprite = NULL;
    int iCount = m_Children.size();
    ogeSpriteMap::iterator it = m_Children.begin();
    ogeSpriteMap::iterator ite = m_Children.end();
    for(int i=0; i<iCount; i++)
    {
        if(it == ite) break;
        if(idx == i)
        {
            pSprite = it->second;
            break;
        }
        it++;
    }
    return pSprite;
}

CogeImage* CogeSprite::GetLightMap()
{
    return m_pLightMap;
}
void CogeSprite::SetLightMap(CogeImage* pLightMapImage)
{
    if(pLightMapImage)
    {
        if(m_pLightMap != pLightMapImage)
        {
            if(m_pLightMap)
            {
                m_pLightMap->Fire();
                //m_pLightMap->GetVideoEngine()->DelImage(m_pLightMap->GetName());
                m_pEngine->m_pVideo->DelImage(m_pLightMap);
            }
            m_pLightMap = pLightMapImage;
            m_pLightMap->Hire();
        }

    }
    else
    {
        if(m_pLightMap)
        {
            m_pLightMap->Fire();
            //m_pLightMap->GetVideoEngine()->DelImage(m_pLightMap->GetName());
            m_pEngine->m_pVideo->DelImage(m_pLightMap);
        }
        m_pLightMap = NULL;
    }
}

ogePointArray* CogeSprite::GetTileList()
{
    return &m_TileList;
}

CogeTextInputter* CogeSprite::GetTextInputter()
{
    return m_pTextInputter;
}
bool CogeSprite::ConnectTextInputter(CogeTextInputter* pTextInputter)
{
    if(!pTextInputter) return false;
    m_pTextInputter = pTextInputter;
    m_pTextInputter->Hire();
    m_iInputWinPosX = m_iPosX;
    m_iInputWinPosY = m_iPosY;
    pTextInputter->SetInputWinPos(m_iInputWinPosX, m_iInputWinPosY);
    return true;

}
bool CogeSprite::ConnectTextInputter(const std::string& sInputterName)
{
    bool bSucceed = false;
    if(m_pEngine && m_pEngine->m_pIM)
    {
        CogeTextInputter* pTextInputter = m_pEngine->m_pIM->FindInputter(sInputterName);
        if(pTextInputter)
        {
            bSucceed = ConnectTextInputter(pTextInputter);
        }
    }
    return bSucceed;
}
void CogeSprite::DisconnectTextInputter()
{
    if(!m_pTextInputter) return;
    if(m_pEngine && m_pEngine->m_pIM)
    {
        CogeTextInputter* pTextInputter = m_pEngine->m_pIM->GetActiveInputter();
        if(pTextInputter == m_pTextInputter) m_pEngine->DisableIM();
        m_pTextInputter->Fire();
        m_pEngine->m_pIM->DelInputter(m_pTextInputter->GetName());
        m_pTextInputter = NULL;
    }
}

char* CogeSprite::GetInputTextBuffer()
{
    if(m_pTextInputter) return m_pTextInputter->GetBufferText();
    else return NULL;
}

void CogeSprite::ClearInputTextBuffer()
{
    if(m_pTextInputter) return m_pTextInputter->Clear();
}

int CogeSprite::GetInputTextCharCount()
{
    if(m_pTextInputter) return m_pTextInputter->GetTextLength();
    else return 0;
}

int CogeSprite::SetInputTextCharCount(int iSize)
{
    if(m_pTextInputter) return m_pTextInputter->SetTextLength(iSize);
    else return 0;
}

int CogeSprite::GetInputterCaretPos()
{
    if(m_pTextInputter) return m_pTextInputter->GetCaretPos();
    else return -9;
}

void CogeSprite::ResetInputterCaretPos()
{
    if(m_pTextInputter) return m_pTextInputter->Reset();
}

void CogeSprite::SetInputWinPos(int iPosX, int iPosY)
{
    m_iInputWinPosX = iPosX;
    m_iInputWinPosY = iPosY;
}

//int CogeSprite::GetInputTextWidth(int iPos)
//{
//    if(m_pTextInputter)
//    {
//        return m_pTextInputter->GetTextWidth();
//    }
//    else return -1;
//}


int CogeSprite::SetDir(int iDir)
{
    if(m_iDir == iDir && m_pCurrentAnima) return 0;
    else
    {
        CogeAnima* pMatchedAnima = FindAnima( m_iAct<<8 | iDir );
        if(pMatchedAnima)
        {
            m_iDir = iDir;
            //m_iAct = iAct;
            m_pCurrentAnima = pMatchedAnima;
            m_pCurrentAnima->Reset();
            return 1;
        }
        else return -1;
    }
}

int CogeSprite::SetAct(int iAct)
{
    if(m_iAct == iAct && m_pCurrentAnima) return 0;
    else
    {
        CogeAnima* pMatchedAnima = FindAnima( iAct<<8 | m_iDir );
        if(pMatchedAnima)
        {
            //m_iDir = iDir;
            m_iAct = iAct;
            m_pCurrentAnima = pMatchedAnima;
            m_pCurrentAnima->Reset();
            return 1;
        }
        else return -1;
    }
}

void CogeSprite::SetDefaultSize(int iWidth, int iHeight)
{
    m_iDefaultWidth  = iWidth;
    m_iDefaultHeight = iHeight;
}
void CogeSprite::SetDefaultRoot(int iRootX, int iRootY)
{
    m_iDefaultRootX = iRootX;
    m_iDefaultRootY = iRootY;
}

bool CogeSprite::OpenTimer(int iInterval)
{
    //if(iInterval <= 0 || m_iUnitType != Spr_Timer) return false;
    if(iInterval <= 0) return false;
    m_iTimerInterval = iInterval;
    m_iTimerLastTick = SDL_GetTicks();
    m_iTimerEventCount = 0;
    m_bEnableTimer   = true;
    return m_bEnableTimer;
}
void CogeSprite::CloseTimer()
{
    m_bEnableTimer = false;
}
int CogeSprite::GetTimerInterval()
{
    if(m_bEnableTimer) return m_iTimerInterval;
    else return -1;
}
bool CogeSprite::IsTimerWaiting()
{
    return m_bEnableTimer;
}

int CogeSprite::SuspendUpdateScript()
{
    if(m_pActiveUpdateScript && m_pActiveUpdateScript->GetState() >= 0)
    {
        return m_pActiveUpdateScript->Pause();
    }
    return -1;
}
int CogeSprite::ResumeUpdateScript()
{
    if(m_pActiveUpdateScript && m_pActiveUpdateScript->GetState() >= 0)
    {
        CogeScript* pCurrentScript = m_pActiveUpdateScript;

        int rsl = pCurrentScript->Resume();

        if(pCurrentScript->IsFinished())
        {
            //pCurrentScript->CallGC();
            m_iCurrentPlotRound++;
            if((!m_bEnablePlot) || (m_iTotalPlotRounds > 0 && m_iCurrentPlotRound >= m_iTotalPlotRounds))
            {
                DisablePlot();
                if(m_pCurrentScene)
                {
                    m_pCurrentScene->m_pCurrentSpr = this;
                    m_pEngine->m_pCurrentGameSprite = this;
                    CallEvent(Event_OnLoseFocus);
                }
            }
        }

        return rsl;
    }
    return -1;
}

int CogeSprite::GetUpdateScriptState()
{
    if(m_pActiveUpdateScript && m_pActiveUpdateScript->GetState() >= 0)
    {
        return m_pActiveUpdateScript->GetRuntimeState();
    }
    return -1;
}

void CogeSprite::EnablePlot(int iRounds)
{
    if(iRounds == 0) return;

    m_pActiveUpdateScript = NULL;

    if(m_pLocalScript)
    {
        int iEventId = m_LocalEvents[Event_OnUpdate];
        if(iEventId >= 0)
        {
            m_iActiveUpdateFunctionId = iEventId;
            m_pActiveUpdateScript = m_pLocalScript;
            //m_pActiveUpdateScript->Reset(); // reset it ...
        }
    }

    if(m_pCommonScript && m_pActiveUpdateScript == NULL)
    {
        int iEventId = m_CommonEvents[Event_OnUpdate];
        if(iEventId >= 0)
        {
            m_iActiveUpdateFunctionId = iEventId;
            m_pActiveUpdateScript = m_pCommonScript;
            //m_pActiveUpdateScript->Reset(); // reset it ...
        }
    }

    if(m_pActiveUpdateScript)
    {
        m_pActiveUpdateScript->Reset();

        m_bEnablePlot = true;

        m_iTotalPlotRounds = iRounds;
        m_iCurrentPlotRound = 0;
    }


}
void CogeSprite::DisablePlot()
{
    m_bEnablePlot = false;
    m_pActiveUpdateScript = NULL;
    m_iActiveUpdateFunctionId = -1;

    m_iTotalPlotRounds = 0;
    m_iCurrentPlotRound = 0;
}

bool CogeSprite::IsPlayingPlot()
{
    return m_bEnablePlot && m_pActiveUpdateScript && m_iActiveUpdateFunctionId >= 0;
}

int CogeSprite::GetPlotTriggerFlag(int iEventCode)
{
    if(iEventCode < 0 || iEventCode >= _OGE_MAX_EVENT_COUNT_) return 0;
    else return m_PlotTriggers[iEventCode];
}
void CogeSprite::SetPlotTriggerFlag(int iEventCode, int iFlag)
{
    if(iEventCode < 0 || iEventCode >= _OGE_MAX_EVENT_COUNT_) return;
    else m_PlotTriggers[iEventCode] = iFlag;
}

/*
void CogeSprite::JoinPlot()
{
    m_bJoinedPlot = true;
}
void CogeSprite::DisjoinPlot()
{
    m_bJoinedPlot = false;
}
*/

/*
int CogeSprite::AddAnimaEffect(int iEffectType, double fEffectValue, bool bIncludeChildren)
{
    bool bNewEffect = true;
    switch (iEffectType)
    {
    case Effect_Alpha:
        if(fEffectValue < 0) return -1;
        if(m_AnimaEffect.use_alpha) bNewEffect = false;
        else m_AnimaEffect.use_alpha = true;
        m_AnimaEffect.value_alpha = (int)fEffectValue;
        break;
    case Effect_Rota:
        if(fEffectValue < 0) return -1;
        if(m_AnimaEffect.use_rota) bNewEffect = false;
        else m_AnimaEffect.use_rota = true;
        m_AnimaEffect.value_rota = (int)fEffectValue;
        break;
    case Effect_RGB:
        //if(fEffectValue < 0) return -1;
        if(m_AnimaEffect.use_light) bNewEffect = false;
        else m_AnimaEffect.use_light = true;
        m_AnimaEffect.value_light = (int)fEffectValue;
        break;
    case Effect_Edge:
        //if(fEffectValue < 0) return -1;
        if(m_AnimaEffect.use_edge) bNewEffect = false;
        else m_AnimaEffect.use_edge = true;
        m_AnimaEffect.value_edge = (int)fEffectValue;
        break;
    case Effect_Color:
        //if(fEffectValue < 0) return -1;
        if(m_AnimaEffect.use_color) bNewEffect = false;
        else m_AnimaEffect.use_color = true;
        m_AnimaEffect.value_color = (int)fEffectValue;
        break;
    case Effect_Scale:
        if(fEffectValue <= 0) return -1;
        if(m_AnimaEffect.use_scale) bNewEffect = false;
        else m_AnimaEffect.use_scale = true;
        m_AnimaEffect.value_scale = fEffectValue;
        break;
    default:
        bNewEffect = false;
    }

    if(bNewEffect) m_AnimaEffect.count = m_AnimaEffect.count + 1;

    if(m_AnimaEffect.count < 0) m_AnimaEffect.count = 0;
    if(m_AnimaEffect.count > 6) m_AnimaEffect.count = 6;

    if(bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->AddAnimaEffect(iEffectType, fEffectValue, bIncludeChildren);
            its++;
        }
    }

    return m_AnimaEffect.count;

}
*/

int CogeSprite::AddAnimaEffect(int iEffectType, double fEffectValue, bool bIncludeChildren)
{
    if(m_pAnimaEffect == NULL) return -1;

    int iRsl =  m_pAnimaEffect->AddEffect(iEffectType, fEffectValue);

    if(iRsl > 0 && bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->AddAnimaEffect(iEffectType, fEffectValue, bIncludeChildren);
            its++;
        }
    }

    return iRsl;
}

int CogeSprite::AddAnimaEffectEx(int iEffectType, double fStart, double fEnd, double fIncrement,  int iStepInterval, int iRepeat, bool bIncludeChildren)
{
    if(m_pAnimaEffect == NULL) return -1;

    int iRsl =  m_pAnimaEffect->AddEffectEx(iEffectType, fStart, fEnd, fIncrement, iStepInterval, iRepeat);

    if(iRsl > 0 && bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->AddAnimaEffectEx(iEffectType, fStart, fEnd, fIncrement, iStepInterval, iRepeat, bIncludeChildren);
            its++;
        }
    }

    return iRsl;
}

/*
int CogeSprite::RemoveAnimaEffect(int iEffectType, bool bIncludeChildren)
{
    bool bOldEffect = false;

    switch (iEffectType)
    {
    case Effect_Alpha:
        if(m_AnimaEffect.use_alpha) bOldEffect = true;
        m_AnimaEffect.use_alpha = false;
        break;
    case Effect_Rota:
        if(m_AnimaEffect.use_rota) bOldEffect = true;
        m_AnimaEffect.use_rota = false;
        break;
    case Effect_RGB:
        if(m_AnimaEffect.use_light) bOldEffect = true;
        m_AnimaEffect.use_light = false;
        break;
    case Effect_Edge:
        if(m_AnimaEffect.use_edge) bOldEffect = true;
        m_AnimaEffect.use_edge = false;
        break;
    case Effect_Color:
        if(m_AnimaEffect.use_color) bOldEffect = true;
        m_AnimaEffect.use_color = false;
        break;
    case Effect_Scale:
        if(m_AnimaEffect.use_scale) bOldEffect = true;
        m_AnimaEffect.use_scale = false;
        break;
    default:
        bOldEffect = false;

    }

    if(bOldEffect) m_AnimaEffect.count = m_AnimaEffect.count - 1;

    if(m_AnimaEffect.count < 0) m_AnimaEffect.count = 0;
    if(m_AnimaEffect.count > 6) m_AnimaEffect.count = 6;

    if(bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->RemoveAnimaEffect(iEffectType, bIncludeChildren);
            its++;
        }
    }

    return m_AnimaEffect.count;
}


void CogeSprite::RemoveAllAnimaEffect(bool bIncludeChildren)
{
    memset(&m_AnimaEffect, 0, sizeof(m_AnimaEffect));

    if(bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->RemoveAllAnimaEffect(bIncludeChildren);
            its++;
        }
    }
}
*/


int CogeSprite::RemoveAnimaEffect(int iEffectType, bool bIncludeChildren)
{
    if(m_pAnimaEffect == NULL) return -1;

    int iRsl =  m_pAnimaEffect->DisableEffect(iEffectType);

    if(bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->RemoveAnimaEffect(iEffectType, bIncludeChildren);
            its++;
        }
    }

    return iRsl;

}


void CogeSprite::RemoveAllAnimaEffect(bool bIncludeChildren)
{
    DelAllEffects();

    if(bIncludeChildren)
    {
        ogeSpriteMap::iterator its, ite;
        its = m_Children.begin();
        ite = m_Children.end();
        while (its != ite)
        {
            its->second->RemoveAllAnimaEffect(bIncludeChildren);
            its++;
        }
    }
}

bool CogeSprite::HasEffect(int iEffectType)
{
    if(m_pAnimaEffect == NULL) return false;
    CogeEffect* pEffect = m_pAnimaEffect->FindEffect(iEffectType);
    return pEffect && pEffect->active;
}

bool CogeSprite::IsEffectCompleted(int iEffectType)
{
    if(m_pAnimaEffect == NULL) return true;
    CogeEffect* pEffect = m_pAnimaEffect->FindEffect(iEffectType);
    if(pEffect == NULL) return true;
    if(pEffect->active == false) return true;
    if(pEffect->effect_value == pEffect->end_value) return true;
    return false;
}

double CogeSprite::GetEffectValue(int iEffectType)
{
    if(m_pAnimaEffect == NULL) return 0;

    CogeEffect* pEffect = m_pAnimaEffect->FindEffect(iEffectType);
    if(pEffect && pEffect->active)
    {
        return pEffect->effect_value;
    }

    return 0;
}

double CogeSprite::GetEffectStepValue(int iEffectType)
{
    if(m_pAnimaEffect == NULL) return 0;

    CogeEffect* pEffect = m_pAnimaEffect->FindEffect(iEffectType);
    if(pEffect && pEffect->active)
    {
        return pEffect->step_value;
    }

    return 0;
}

void CogeSprite::IncX(int x)
{
    if(!m_bEnableMovement) return;

    //m_iMovingTimeX += m_pEngine->m_iFrameInterval;

    if(m_iMovingIntervalX > 0)
    {
        m_iMovingTimeX += m_pEngine->m_iCurrentInterval;

        if (m_iMovingTimeX <= m_iMovingIntervalX) return;
        else m_iMovingTimeX = 0;
    }

    if(m_bIsRelative)
    {
        m_iRelativeX += x;
        if(m_pParent) AdjustParent();
        else AdjustSceneView();
    }
    else
    {
        m_iPosX += x;
        ChildrenAdjust();
    }

}
void CogeSprite::IncY(int y)
{
    if(!m_bEnableMovement) return;

    //m_iMovingTimeY += m_pEngine->m_iFrameInterval;

    if(m_iMovingIntervalY > 0)
    {
        m_iMovingTimeY += m_pEngine->m_iCurrentInterval;

        if (m_iMovingTimeY <= m_iMovingIntervalY) return;
        else m_iMovingTimeY = 0;
    }

    if(m_bIsRelative)
    {
        m_iRelativeY += y;
        if(m_pParent) AdjustParent();
        else AdjustSceneView();
    }
    else
    {
        m_iPosY += y;
        ChildrenAdjust();
    }

}

void CogeSprite::SetSpeed(int iSpeedX, int iSpeedY)
{
    if(iSpeedX < 0) iSpeedX = 0;
    if(iSpeedY < 0) iSpeedY = 0;
    if(iSpeedX > 100) iSpeedX = 100;
    if(iSpeedY > 100) iSpeedY = 100;

    m_iMovingIntervalX = 100 - iSpeedX;
    m_iMovingIntervalY = 100 - iSpeedY;
    m_iMovingTimeX = 0;
    m_iMovingTimeY = 0;
}

void CogeSprite::SetPosZ(int iPosZ)
{
    m_iPosZ = iPosZ;
    if(m_pCurrentScene)
    {
        if(m_iPosZ > m_pCurrentScene->m_iTopZ)
            m_pCurrentScene->m_iTopZ = m_iPosZ;
    }
    ChildrenAdjust();
}

void CogeSprite::SetPos(int iPosX, int iPosY, int iPosZ)
{
    m_iPosX = iPosX;
    m_iPosY = iPosY;
    m_iPosZ = iPosZ;

    if(m_pCurrentScene)
    {
        if(m_iPosZ > m_pCurrentScene->m_iTopZ)
            m_pCurrentScene->m_iTopZ = m_iPosZ;
    }

    ChildrenAdjust();
}

void CogeSprite::SetPos(int iPosX, int iPosY)
{
    m_iPosX = iPosX;
    m_iPosY = iPosY;

    ChildrenAdjust();
}

void CogeSprite::SetRelativePos(int iRelativeX, int iRelativeY)
{
    m_iRelativeX = iRelativeX;
    m_iRelativeY = iRelativeY;

    if(m_bIsRelative)
    {
        if(m_pParent) AdjustParent();
        else AdjustSceneView();

        ChildrenAdjust();
    }

}

bool CogeSprite::GetRelativePosMode()
{
    return m_bIsRelative;
}
void CogeSprite::SetRelativePosMode(bool bValue)
{
    m_bIsRelative = bValue;
}

void CogeSprite::SetBusy(bool bValue)
{
    m_bBusy = bValue;
}
bool CogeSprite::GetBusy()
{
    return m_bBusy;
}

int CogeSprite::SetActive(bool bValue)
{
    //if(bValue == m_bActive) return 0;
    if(m_pCurrentScene)
    {
        if(bValue) return m_pCurrentScene->ActivateSprite(m_sName);
        else
        {
            m_iLastDisplayFlag = 0;
            return m_pCurrentScene->DeactivateSprite(m_sName);
        }
    }
    else return -1;
}
bool CogeSprite::GetActive()
{
    return m_bActive;
}

int CogeSprite::PrepareActive()
{
    if(m_pCurrentScene)
    {
        return m_pCurrentScene->PrepareActivateSprite(this);
    }
    else return -1;
}
int CogeSprite::PrepareDeactive()
{
    if(m_pCurrentScene)
    {
        return m_pCurrentScene->PrepareDeactivateSprite(this);
    }
    else return -1;
}

void CogeSprite::SetVisible(bool bValue)
{
    if(bValue != m_bVisible) m_bVisible = bValue;
    if(!m_bVisible) m_iLastDisplayFlag = 0;

    int iCount = m_Children.size();
    if(iCount > 0)
    {
        ogeSpriteMap::iterator it = m_Children.begin();
        while(iCount > 0)
        {
            it->second->SetVisible(bValue);
            it++;
            iCount--;
        }
    }

}
bool CogeSprite::GetVisible()
{
    return m_bVisible;
}

void CogeSprite::SetInput(bool bValue)
{
    if(bValue != m_bEnableInput) m_bEnableInput = bValue;
}
bool CogeSprite::GetInput()
{
    return m_bEnableInput;
}

void CogeSprite::SetDrag(bool bValue)
{
    if(bValue != m_bEnableDrag) m_bEnableDrag = bValue;
}
bool CogeSprite::GetDrag()
{
    return m_bEnableDrag;
}

void CogeSprite::SetCollide(bool bValue)
{
    if(bValue != m_bEnableCollision) m_bEnableCollision = bValue;
}
bool CogeSprite::GetCollide()
{
    return m_bEnableCollision;
}

int CogeSprite::SetType(int iType)
{
    if(iType < Spr_Empty || iType > Spr_Player) return -1;
    m_iUnitType = iType;

    m_bVisible        = (m_iUnitType != Spr_Empty) && (m_iUnitType != Spr_Timer) && (m_iUnitType != Spr_Plot);
    m_bEnableAnima    = m_bVisible && (m_iUnitType >= Spr_Anima);
    m_bEnableMovement = (m_iUnitType >= Spr_Anima) && (m_iUnitType != Spr_Timer) && (m_iUnitType != Spr_Plot);
    m_bEnableCollision= (m_iUnitType != Spr_Anima) && (m_iUnitType != Spr_Timer) && (m_iUnitType != Spr_Plot);
    m_bEnableInput    = m_iUnitType >= Spr_Window;
    m_bEnableDrag     = m_iUnitType == Spr_Window;

    m_bEnableFocus    = m_iUnitType >= Spr_Window;

    return m_iUnitType;
}
int CogeSprite::GetType()
{
    return m_iUnitType;
}

void CogeSprite::SetClassTag(int iTag)
{
    m_iClassTag = iTag;
}
int CogeSprite::GetClassTag()
{
    return m_iClassTag;
}
void CogeSprite::SetObjectTag(int iTag)
{
    m_iObjectTag = iTag;
}
int CogeSprite::GetObjectTag()
{
    return m_iObjectTag;
}

int CogeSprite::GetPosX()
{
    return m_iPosX;
}
int CogeSprite::GetPosY()
{
    return m_iPosY;
}
int CogeSprite::GetPosZ()
{
    return m_iPosZ;
}

int CogeSprite::GetRelativeX()
{
    return m_iRelativeX;
}
int CogeSprite::GetRelativeY()
{
    return m_iRelativeY;
}

int CogeSprite::GetAnimaRootX()
{
    if(m_pCurrentAnima && m_bDefaultDraw) return m_pCurrentAnima->m_iRootX;
    else return m_iDefaultRootX;
}
int CogeSprite::GetAnimaRootY()
{
    if(m_pCurrentAnima && m_bDefaultDraw) return m_pCurrentAnima->m_iRootY;
    else return m_iDefaultRootY;
}
int CogeSprite::GetAnimaWidth()
{
    if(m_pCurrentAnima && m_bDefaultDraw) return m_pCurrentAnima->m_iFrameWidth;
    else return m_iDefaultWidth;
}
int CogeSprite::GetAnimaHeight()
{
    if(m_pCurrentAnima && m_bDefaultDraw) return m_pCurrentAnima->m_iFrameHeight;
    else return m_iDefaultHeight;
}

int CogeSprite::GetDir()
{
    return m_iDir;
}
int CogeSprite::GetAct()
{
    return m_iAct;
}

/*
int CogeSprite::UseThinker(const std::string& sThinkerName)
{
    CogeThinker* pThinker = m_pEngine->GetThinker(sThinkerName);
    if(pThinker)
    {
        m_pCurrentThinker = pThinker;
        return 0;
    }
    else return -1;
}

void CogeSprite::RemoveThinker()
{
    m_pCurrentThinker = NULL;
}
*/

/*
CogeCustomEventSet* CogeSprite::GetCustomEventSet()
{
    return m_pCustomEvents;
}
*/

void CogeSprite::CallCustomEvents()
{
    if(m_pCustomEvents && m_iUnitType != Spr_Plot)
    {
        m_bCallingCustomEvents = true;
        m_pCustomEvents->SetTrigger(&(m_PlotTriggers[0]));
        m_pCustomEvents->SetPlotSpr(m_pPlotSpr);
        m_pCustomEvents->CallCustomEvents();
        m_bCallingCustomEvents = false;
        CallEvent(Event_OnCustomEnd);
    }
}
void CogeSprite::PushCustomEvent(int iCustomEventId)
{
    if(m_pCustomEvents && m_iUnitType != Spr_Plot) m_pCustomEvents->PushEvent(iCustomEventId);
}
void CogeSprite::ClearRunningCustomEvents()
{
    if(m_pCustomEvents && m_iUnitType != Spr_Plot) m_pCustomEvents->ClearPendingEvents();
}

/*
int CogeSprite::BindGameData(const std::string& sGameDataName)
{
    CogeGameData* pGameData = m_pEngine->GetGameData(sGameDataName);
    if(pGameData && pGameData!=m_pGameData)
    {
        m_pGameData = pGameData;
        pGameData->Hire();
        return 1;
    }
    else if(pGameData==m_pGameData) return 0;
    return -1;
}
*/

int CogeSprite::BindGameData(CogeGameData* pGameData)
{
    if(pGameData == NULL)
    {
        RemoveGameData();
        return 0;
    }

    if(pGameData != m_pGameData)
    {
        if(m_pGameData) RemoveGameData();
        m_pGameData = pGameData;
        //pGameData->Hire();
        return 1;
    }
    else return 0;

}

void CogeSprite::RemoveGameData()
{
    //if(m_pGameData) m_pGameData->Fire();
    m_pGameData = NULL;
}

CogeGameData* CogeSprite::GetGameData()
{
    return m_pGameData;
}

CogePath* CogeSprite::GetCurrentPath()
{
    return m_pCurrentPath;
}
CogePath* CogeSprite::GetLocalPath()
{
    return m_pLocalPath;
}
int CogeSprite::UseLocalPath(int iStep, int iStepInterval, int iVirtualStepCount, bool bAutoStepIn)
{
    return UsePath(m_pLocalPath, iStep, iStepInterval, iVirtualStepCount, bAutoStepIn);
}

int CogeSprite::UsePath(CogePath* pPath, int iStep, int iStepInterval, int iVirtualStepCount, bool bAutoStepIn)
{
    if(pPath)
    {
        m_pCurrentPath = pPath;
        m_iPathStepInterval = iStepInterval;
        m_iVirtualStepCount = iVirtualStepCount;
        return ResetPath(iStep, bAutoStepIn);
    }
    else return -1;
}

int CogeSprite::ResetPath(int iStep, int iStepInterval, int iVirtualStepCount, bool bAutoStepIn)
{
    if(m_pCurrentPath && iStep)
    {
        m_iPathStepTime = 0;

        m_iPathStepInterval = iStepInterval;

        m_iVirtualStepCount = iVirtualStepCount;

        m_bPathAutoStepIn = bAutoStepIn;

        m_iPathLastIdx = m_pCurrentPath->m_Points.size()-1;
        m_iPathLastKey = m_pCurrentPath->m_Keys.size()-1;

        if(m_iPathLastIdx < 0)
        {
            m_iPathLastIdx = 0;
            m_iPathLastKey = 0;
            m_iPathStep = 0;
            return -1;
        }

        //m_iPathOffsetX = iOffsetX;
        //m_iPathOffsetY = iOffsetY;

        if(m_bIsRelative && !m_pParent)
        {
            m_iPathOffsetX = m_iRelativeX;
            m_iPathOffsetY = m_iRelativeY;
        }
        else
        {
            m_iPathOffsetX = m_iPosX;
            m_iPathOffsetY = m_iPosY;
        }

        m_iPathStep = iStep;

        if(m_iPathStep > 0)
        {
            m_iPathStepIdx = 0;
            m_iPathKeyIdx = 0;
        }
        else
        {
            m_iPathStepIdx = m_iPathLastIdx;
            m_iPathKeyIdx = m_iPathLastKey;
        }


        if(m_bIsRelative && !m_pParent)
        {
            m_iRelativeX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
            m_iRelativeY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
        }
        else
        {
            m_iPosX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
            m_iPosY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
        }

        //return m_pCurrentPath->m_iDir;

        return 0;
    }
    else
    {
        m_iPathStepTime = 0;
        m_iPathStep = 0;
        m_iPathStepInterval = 0;
        m_iVirtualStepCount = -1;
        m_bPathAutoStepIn = false;
        return -1;
    }
}
int CogeSprite::NextPathStep()
{
    if(m_pCurrentPath && m_iPathStep)
    {

        //m_iPathStepTime += m_pEngine->m_iFrameInterval;
        m_iPathStepTime += m_pEngine->m_iCurrentInterval;

        if (m_iPathStepTime > m_iPathStepInterval)
        {
            m_iPathStepTime = 0;
        }
        else return -1;

        if(m_iPathStep > 0)
        {
            int iValidStepCount = m_pCurrentPath->GetTotalStepCount();
            if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

            if(m_iPathStepIdx >= m_iPathLastIdx || m_iPathStepIdx >= iValidStepCount - 1)
            {
                return -1;
            }
            else
            {
                int iCurrentStepIdx = m_iPathStepIdx;
                int iNextStepIdx = m_iPathStepIdx + m_iPathStep;
                int iMaxKeyIdx = m_pCurrentPath->m_Keys.size() - 1;
                int iNextKeyIdx = m_iPathKeyIdx + 1; // ...
                if(iNextKeyIdx > iMaxKeyIdx) iNextKeyIdx = iMaxKeyIdx;

                for(int i=iCurrentStepIdx; i<iNextStepIdx; i++)
                {
                    if(m_pCurrentPath->m_Points[i]->x == m_pCurrentPath->m_Keys[iNextKeyIdx]->x
                       && m_pCurrentPath->m_Points[i]->y == m_pCurrentPath->m_Keys[iNextKeyIdx]->y)
                    {
                       m_iPathKeyIdx = iNextKeyIdx;
                       break;
                    }
                }

                m_iPathStepIdx = m_iPathStepIdx + m_iPathStep;
                if(m_iPathStepIdx > m_iPathLastIdx) m_iPathStepIdx = m_iPathLastIdx;

                if(m_bIsRelative && !m_pParent)
                {
                    m_iRelativeX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
                    m_iRelativeY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
                }
                else
                {
                    m_iPosX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
                    m_iPosY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
                }

                m_bPathStepIn = true;
                return m_iPathStepIdx;
            }
        }
        else
        {
            int iTotalStepCount = m_pCurrentPath->GetTotalStepCount();
            int iValidStepCount = iTotalStepCount;
            if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

            if(m_iPathStepIdx <= 0 || m_iPathStepIdx <= iTotalStepCount - iValidStepCount)
            {
                return -1;
            }
            else
            {
                int iCurrentStepIdx = m_iPathStepIdx;
                int iNextStepIdx = m_iPathStepIdx + m_iPathStep;
                //int iCurrentKeyIdx = m_iPathKeyIdx;
                int iNextKeyIdx = m_iPathKeyIdx - 1; // ...
                if(iNextKeyIdx < 0) iNextKeyIdx = 0;

                for(int i=iCurrentStepIdx; i>iNextStepIdx; i--)
                {
                    if(m_pCurrentPath->m_Points[i]->x == m_pCurrentPath->m_Keys[iNextKeyIdx]->x
                       && m_pCurrentPath->m_Points[i]->y == m_pCurrentPath->m_Keys[iNextKeyIdx]->y)
                    {
                       m_iPathKeyIdx = iNextKeyIdx;
                       break;
                    }
                }

                m_iPathStepIdx = m_iPathStepIdx + m_iPathStep;
                if(m_iPathStepIdx < 0) m_iPathStepIdx = 0;

                if(m_bIsRelative && !m_pParent)
                {
                    m_iRelativeX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
                    m_iRelativeY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
                }
                else
                {
                    m_iPosX = m_iPathOffsetX + m_pCurrentPath->m_Points[m_iPathStepIdx]->x;
                    m_iPosY = m_iPathOffsetY + m_pCurrentPath->m_Points[m_iPathStepIdx]->y;
                }

                m_bPathStepIn = true;
                return m_iPathStepIdx;
            }
        }
    }
    else return -1;
}
void CogeSprite::AbortPath()
{
    //m_iPathStep = 0;
    m_iPathStepTime = 0;
    m_iPathStep = 0;
    m_bPathAutoStepIn = false;
}

int CogeSprite::GetCurrentStepIndex()
{
    return m_iPathStepIdx;
}

int CogeSprite::GetPathStepLength()
{
    return m_iPathStep;
}
void CogeSprite::SetPathStepLength(int iStepLength)
{
    //if(iStepLength >= 0) m_iPathStep = iStepLength;
    m_iPathStep = iStepLength;
}

int CogeSprite::GetPathStepInterval()
{
    return m_iPathStepInterval;
}
void CogeSprite::SetPathStepInterval(int iStepInterval)
{
    if(iStepInterval >= 0) m_iPathStepInterval = iStepInterval;
}

int CogeSprite::GetPathStepCount()
{
    int iValidStepCount = m_iVirtualStepCount;
    if(m_pCurrentPath) iValidStepCount = m_pCurrentPath->GetTotalStepCount();
    if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;
    return iValidStepCount;
}
void CogeSprite::SetPathStepCount(int iVirtualStepCount)
{
    if(iVirtualStepCount < 0)
    {
        m_iVirtualStepCount = -1;
        return;
    }
    if(m_pCurrentPath)
    {
        int iMaxStepCount = m_pCurrentPath->GetTotalStepCount();
        if(iVirtualStepCount > iMaxStepCount) m_iVirtualStepCount = iMaxStepCount;
        else m_iVirtualStepCount = iVirtualStepCount;
        return;
    }
    m_iVirtualStepCount = iVirtualStepCount;
}

int CogeSprite::GetPathDir()
{
    if(m_pCurrentPath == NULL) return -1;

    int iNextStepIdx = -1;

    if(m_iPathStep > 0)
    {
        int iValidStepCount = m_pCurrentPath->GetTotalStepCount();
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx >= m_iPathLastIdx || m_iPathStepIdx >= iValidStepCount - 1)
        {
            return -1;
        }
        else
        {
            iNextStepIdx = m_iPathStepIdx + m_iPathStep;
            if(iNextStepIdx > m_iPathLastIdx) iNextStepIdx = m_iPathLastIdx;
        }
    }
    else
    {
        int iTotalStepCount = m_pCurrentPath->GetTotalStepCount();
        int iValidStepCount = iTotalStepCount;
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx <= 0 || m_iPathStepIdx <= iTotalStepCount - iValidStepCount)
        {
            return -1;
        }
        else
        {
            iNextStepIdx = m_iPathStepIdx + m_iPathStep;
            if(iNextStepIdx < 0) iNextStepIdx = 0;
        }
    }

    if(iNextStepIdx < 0) return -1;

    int iNextPosX = m_iPathOffsetX + m_pCurrentPath->m_Points[iNextStepIdx]->x;
    int iNextPosY = m_iPathOffsetY + m_pCurrentPath->m_Points[iNextStepIdx]->y;

    int iDeltaX = iNextPosX - m_iPosX;
    int iDeltaY = iNextPosY - m_iPosY;

    if(m_bIsRelative && !m_pParent)
    {
        iDeltaX = iNextPosX - m_iRelativeX;
        iDeltaY = iNextPosY - m_iRelativeY;
    }

    if(iDeltaX == 0)
    {
        if(iDeltaY > 0) return Dir_Down;
        else if(iDeltaY < 0) return Dir_Up;
        else return -1;
    }

    if(iDeltaY == 0)
    {
        if(iDeltaX > 0) return Dir_Right;
        else if(iDeltaX < 0) return Dir_Left;
        else return -1;
    }

    if(iDeltaX > 0 && iDeltaY > 0) return Dir_DownRight;
    else if(iDeltaX > 0 && iDeltaY < 0) return Dir_UpRight;
    else if(iDeltaX < 0 && iDeltaY > 0) return Dir_DownLeft;
    else if(iDeltaX < 0 && iDeltaY < 0) return Dir_UpLeft;
    else return -1;

}

int CogeSprite::GetPathDirAvg()
{
    if(m_pCurrentPath == NULL) return -1;

    int iNextStepIdx = -1;

    if(m_iPathStep > 0)
    {
        int iValidStepCount = m_pCurrentPath->GetTotalStepCount();
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx >= m_iPathLastIdx || m_iPathStepIdx >= iValidStepCount - 1)
        {
            return -1;
        }
        else
        {
            iNextStepIdx = m_iPathStepIdx + m_iPathStep;
            if(iNextStepIdx > m_iPathLastIdx) iNextStepIdx = m_iPathLastIdx;
        }
    }
    else
    {
        int iTotalStepCount = m_pCurrentPath->GetTotalStepCount();
        int iValidStepCount = iTotalStepCount;
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx <= 0 || m_iPathStepIdx <= iTotalStepCount - iValidStepCount)
        {
            return -1;
        }
        else
        {
            iNextStepIdx = m_iPathStepIdx + m_iPathStep;
            if(iNextStepIdx < 0) iNextStepIdx = 0;
        }
    }

    if(iNextStepIdx < 0) return -1;

    int iNextPosX = m_iPathOffsetX + m_pCurrentPath->m_Points[iNextStepIdx]->x;
    int iNextPosY = m_iPathOffsetY + m_pCurrentPath->m_Points[iNextStepIdx]->y;

    int iDeltaX = iNextPosX - m_iPosX;
    int iDeltaY = iNextPosY - m_iPosY;

    if(m_bIsRelative && !m_pParent)
    {
        iDeltaX = iNextPosX - m_iRelativeX;
        iDeltaY = iNextPosY - m_iRelativeY;
    }

    if(iDeltaX == 0)
    {
        if(iDeltaY > 0) return Dir_Down;
        else if(iDeltaY < 0) return Dir_Up;
        else return -1;
    }

    if(iDeltaY == 0)
    {
        if(iDeltaX > 0) return Dir_Right;
        else if(iDeltaX < 0) return Dir_Left;
        else return -1;
    }

    double tg225 = 0.41421;
    double tg675 = 2.41421;

    double fRate = (iDeltaY+0.00)/iDeltaX; //tg(alfa);

    if( (fRate > 0 && fRate < tg225) || (fRate < 0 && fRate > 0-tg225))
    {
        if(iDeltaX > 0) return Dir_Right;
        else return Dir_Left;
    }

    if( (fRate > 0 && fRate > tg675) || (fRate < 0 && fRate < 0-tg675))
    {
        if(iDeltaY > 0) return Dir_Down;
        else return Dir_Up;
    }

    if(iDeltaX > 0 && iDeltaY > 0) return Dir_DownRight;
    else if(iDeltaX > 0 && iDeltaY < 0) return Dir_UpRight;
    else if(iDeltaX < 0 && iDeltaY > 0) return Dir_DownLeft;
    else if(iDeltaX < 0 && iDeltaY < 0) return Dir_UpLeft;
    else return -1;

}

int CogeSprite::GetPathKeyDir()
{
    if(m_pCurrentPath == NULL) return -1;

    int iNextKeyIdx = -1;

    if(m_iPathStep > 0)
    {
        int iValidStepCount = m_pCurrentPath->GetTotalStepCount();
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx >= m_iPathLastIdx || m_iPathStepIdx >= iValidStepCount - 1)
        {
            return -1;
        }
        else
        {
            iNextKeyIdx = m_iPathKeyIdx + 1;
            if(iNextKeyIdx > m_iPathLastKey) iNextKeyIdx = m_iPathLastKey;
        }
    }
    else
    {
        int iTotalStepCount = m_pCurrentPath->GetTotalStepCount();
        int iValidStepCount = iTotalStepCount;
        if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;

        if(m_iPathStepIdx <= 0 || m_iPathStepIdx <= iTotalStepCount - iValidStepCount)
        {
            return -1;
        }
        else
        {
            iNextKeyIdx = m_iPathKeyIdx - 1;
            if(iNextKeyIdx < 0) iNextKeyIdx = 0;
        }
    }

    if(iNextKeyIdx < 0) return -1;

    int iLastPosX = m_iPathOffsetX + m_pCurrentPath->m_Keys[m_iPathKeyIdx]->x;
    int iLastPosY = m_iPathOffsetY + m_pCurrentPath->m_Keys[m_iPathKeyIdx]->y;

    int iNextPosX = m_iPathOffsetX + m_pCurrentPath->m_Keys[iNextKeyIdx]->x;
    int iNextPosY = m_iPathOffsetY + m_pCurrentPath->m_Keys[iNextKeyIdx]->y;

    int iDeltaX = iNextPosX - iLastPosX;
    int iDeltaY = iNextPosY - iLastPosY;

    //if(m_bIsRelative && !m_pParent)
    //{
    //    iDeltaX = iNextPosX - m_iRelativeX;
    //    iDeltaY = iNextPosY - m_iRelativeY;
    //}

    if(iDeltaX == 0)
    {
        if(iDeltaY > 0) return Dir_Down;
        else if(iDeltaY < 0) return Dir_Up;
        else return -1;
    }

    if(iDeltaY == 0)
    {
        if(iDeltaX > 0) return Dir_Right;
        else if(iDeltaX < 0) return Dir_Left;
        else return -1;
    }

    double tg225 = 0.41421;
    double tg675 = 2.41421;

    double fRate = (iDeltaY+0.00)/iDeltaX; //tg(alfa);

    if( (fRate > 0 && fRate < tg225) || (fRate < 0 && fRate > 0-tg225))
    {
        if(iDeltaX > 0) return Dir_Right;
        else return Dir_Left;
    }

    if( (fRate > 0 && fRate > tg675) || (fRate < 0 && fRate < 0-tg675))
    {
        if(iDeltaY > 0) return Dir_Down;
        else return Dir_Up;
    }

    if(iDeltaX > 0 && iDeltaY > 0) return Dir_DownRight;
    else if(iDeltaX > 0 && iDeltaY < 0) return Dir_UpRight;
    else if(iDeltaX < 0 && iDeltaY > 0) return Dir_DownLeft;
    else if(iDeltaX < 0 && iDeltaY < 0) return Dir_UpLeft;
    else return -1;

}

void CogeSprite::UpdateDirtyRect()
{
    //CogeRect rc;
	m_DirtyRect.left   = m_OldPosRect.left   < m_DrawPosRect.left   ? m_OldPosRect.left   : m_DrawPosRect.left;
	m_DirtyRect.top    = m_OldPosRect.top    < m_DrawPosRect.top    ? m_OldPosRect.top    : m_DrawPosRect.top;
	m_DirtyRect.right  = m_OldPosRect.right  > m_DrawPosRect.right  ? m_OldPosRect.right  : m_DrawPosRect.right;
	m_DirtyRect.bottom = m_OldPosRect.bottom > m_DrawPosRect.bottom ? m_OldPosRect.bottom : m_DrawPosRect.bottom;
	//return rc;
}

int CogeSprite::AddChild(CogeSprite* pChild, int iClientX, int iClientY)
{
    if(!pChild) return -1;
    std::string sName = pChild->m_sName;
    ogeSpriteMap::iterator its;
	its = m_Children.find(sName);
	if (its != m_Children.end())
	{
	    CogeSprite* pChildSpr = its->second;

        pChildSpr->m_pCurrentScene = m_pCurrentScene;

        pChildSpr->m_pParent = this;

        pChildSpr->UpdateRoot(m_pRoot);

        pChildSpr->m_bIsRelative = true;

        pChildSpr->m_iRelativeX = iClientX;
        pChildSpr->m_iRelativeY = iClientY;

        pChildSpr->AdjustParent();

        pChildSpr->SetActive(m_bActive);

        return 0;
    }
	else
	{
		m_Children.insert(ogeSpriteMap::value_type(sName, pChild));

		pChild->m_pCurrentScene = m_pCurrentScene;

		pChild->m_pParent = this;

        pChild->UpdateRoot(m_pRoot);

        pChild->m_bIsRelative = true;

		pChild->m_iRelativeX = iClientX;
		pChild->m_iRelativeY = iClientY;

		//if(pChild->m_iUnitType==Spr_Window)
		pChild->AdjustParent();

		pChild->SetActive(m_bActive);

		return 1;
	}
}
int CogeSprite::AddChild(const std::string& sName, int iClientX, int iClientY)
{
    //if(!m_pEngine) return -1;
    //std::string sName = pChild->GetName();
    ogeSpriteMap::iterator its;
	its = m_Children.find(sName);
	if (its != m_Children.end())
	{
	    CogeSprite* pChild = its->second;

        pChild->m_pCurrentScene = m_pCurrentScene;

        pChild->m_pParent = this;

        pChild->UpdateRoot(m_pRoot);

        pChild->m_bIsRelative = true;

        pChild->m_iRelativeX = iClientX;
        pChild->m_iRelativeY = iClientY;

        pChild->AdjustParent();

        pChild->SetActive(m_bActive);

        return 0;
    }
	else
	{
	    //CogeSprite* pChild = m_pEngine->FindSprite(sName);
	    CogeSprite* pChild = NULL;
	    if(m_pCurrentScene) pChild = m_pCurrentScene->FindActiveSprite(sName);
	    else return -1;

	    if(!pChild) return -1;

		m_Children.insert(ogeSpriteMap::value_type(sName, pChild));

		pChild->m_pCurrentScene = m_pCurrentScene;

		pChild->m_pParent = this;

		pChild->UpdateRoot(m_pRoot);

        pChild->m_bIsRelative = true;

		pChild->m_iRelativeX = iClientX;
		pChild->m_iRelativeY = iClientY;

		//if(pChild->m_iUnitType==Spr_Window)
		pChild->AdjustParent();

		pChild->SetActive(m_bActive);

		return 1;
	}

}

int CogeSprite::SetParent(CogeSprite* pParent, int iClientX, int iClientY)
{
    if(pParent == NULL)
    {
        if(m_pParent != NULL)
        {
            m_pParent->RemoveChild(m_sName);
            m_bIsRelative = false;
            m_pParent = NULL;
            m_pRoot = NULL;
            UpdateRoot(NULL);
        }
        return 0;
    }
    else
    {
        if(m_pParent != pParent)
        {
            if(m_pParent != NULL)
            {
                m_pParent->RemoveChild(m_sName);
                m_bIsRelative = false;
                m_pParent = NULL;
                m_pRoot = NULL;
                UpdateRoot(NULL);
            }
            return pParent->AddChild(this, iClientX, iClientY);
        }
        else SetRelativePos(iClientX, iClientY);
        return 0;
    }

}

int CogeSprite::SetParent(const std::string& sParentName, int iClientX, int iClientY)
{
    //CogeSprite* pParent = m_pEngine->FindSprite(sParentName);
    CogeSprite* pParent = NULL;
    //if(m_pCurrentScene) pParent = m_pCurrentScene->FindActiveSprite(sParentName);
    //else return -1;

    if(m_pCurrentScene) pParent = m_pCurrentScene->FindSprite(sParentName);
    else return -1;

    if(!pParent) return -1;
    else return SetParent(pParent, iClientX, iClientY);

}

void CogeSprite::UpdateRoot(CogeSprite* pRoot)
{
    if(pRoot != NULL) m_pRoot = pRoot;
    else if(m_pParent != NULL) m_pRoot = m_pParent;
    else m_pRoot = NULL;

    int iCount = m_Children.size();
    if(iCount <= 0) return;
    ogeSpriteMap::iterator it = m_Children.begin();
    while(iCount > 0)
    {
        it->second->UpdateRoot(pRoot);
        it++;
        iCount--;
    }
}

int CogeSprite::RemoveChild(const std::string& sName)
{
    ogeSpriteMap::iterator its;
	its = m_Children.find(sName);
	if (its != m_Children.end())
	{
		its->second->m_pParent = NULL;
		its->second->m_pRoot = NULL;
		its->second->m_bIsRelative = false;
		its->second->UpdateRoot(NULL);
		m_Children.erase(its);
		return 1;
	}
	else return -1;
}

void CogeSprite::AdjustParent()
{
    if(!m_pParent || !m_bIsRelative) return;

    if(m_pParent) {

        m_iPosZ = m_pParent->m_iPosZ + 1;

        if(m_pCurrentScene)
        {
            if(m_iPosZ > m_pCurrentScene->m_iTopZ)
                m_pCurrentScene->m_iTopZ = m_iPosZ;
        }

        m_iPosX = m_pParent->m_iPosX + m_iRelativeX;
        m_iPosY = m_pParent->m_iPosY + m_iRelativeY;

        ChildrenAdjust();
    }
    else if(m_bIsRelative) {
        //...
    }
}

void CogeSprite::ChildrenAdjust()
{
    int iCount = m_Children.size();
    if(iCount <= 0) return;

    ogeSpriteMap::iterator it = m_Children.begin();

    while(iCount > 0)
    {
        it->second->AdjustParent();
        it++;
        iCount--;
    }
}

void CogeSprite::AdjustSceneView()
{
    if(m_bIsRelative && m_pCurrentScene && !m_pParent)
    {
        m_iPosX = m_pCurrentScene->m_SceneViewRect.left + m_iRelativeX;
        m_iPosY = m_pCurrentScene->m_SceneViewRect.top + m_iRelativeY;
        ChildrenAdjust();
    }
}

void CogeSprite::UpdatePosZ(int iValuePosZ)
{
    m_iPosZ = iValuePosZ;

    if(m_pCurrentScene)
    {
        if(m_iPosZ > m_pCurrentScene->m_iTopZ)
            m_pCurrentScene->m_iTopZ = m_iPosZ;
    }

    int iCount = m_Children.size();
    if(iCount <= 0) return;

    ogeSpriteMap::iterator it = m_Children.begin();

    while(iCount > 0)
    {
        if(it->second->m_bIsRelative) it->second->UpdatePosZ(m_iPosZ + 1);
        it++; iCount--;
    }
}

void CogeSprite::PrepareNextFrame()
{
    if (m_pCurrentAnima && m_bDefaultDraw)
	{
		m_DrawPosRect.left   = m_iPosX - m_pCurrentAnima->m_iRootX;
		m_DrawPosRect.top    = m_iPosY - m_pCurrentAnima->m_iRootY;
		m_DrawPosRect.right  = m_DrawPosRect.left + m_pCurrentAnima->m_iFrameWidth;
		m_DrawPosRect.bottom = m_DrawPosRect.top  + m_pCurrentAnima->m_iFrameHeight;

		m_Body.left   = m_DrawPosRect.left + m_pCurrentAnima->m_Body.left;
		m_Body.top    = m_DrawPosRect.top  + m_pCurrentAnima->m_Body.top;

		m_Body.right  = m_DrawPosRect.left + m_pCurrentAnima->m_Body.right;
		m_Body.bottom = m_DrawPosRect.top  + m_pCurrentAnima->m_Body.bottom;

	}
	else
	{
	    m_DrawPosRect.left   = m_iPosX - m_iDefaultRootX;
		m_DrawPosRect.top    = m_iPosY - m_iDefaultRootY;
		m_DrawPosRect.right  = m_DrawPosRect.left + m_iDefaultWidth;
		m_DrawPosRect.bottom = m_DrawPosRect.top  + m_iDefaultHeight;

		memcpy(&m_Body, &m_DrawPosRect, sizeof(m_DrawPosRect));

    }
}

void CogeSprite::Update()
{
    if(m_iState < 0) return;

    //if(m_pCurrentThinker) m_pCurrentThinker->Think();

    if (m_pCurrentAnima)
    {
        if(m_bEnableAnima &&
           m_pCurrentAnima->m_bFinished &&
           m_pCurrentAnima->m_iTotalFrames > 0 &&
           !m_pCurrentAnima->m_bAutoReplay)
        {
            CallEvent(Event_OnAnimaFin);
        }
    }

    bool bAllowPath = (!m_pEngine->m_bFreeze) || (m_iUnitType < Spr_Npc);

    if(!m_pEngine->m_bFreeze)
    {
        if(m_bEnableTimer)
        {
            int iCurrentTick = SDL_GetTicks();
            if(iCurrentTick - m_iTimerLastTick >= m_iTimerInterval)
            {
                m_iTimerLastTick = iCurrentTick;
                m_iTimerEventCount++;
                CallEvent(Event_OnTimerTime);
            }
        }

        if(m_bPathAutoStepIn && m_pCurrentPath && m_iPathStep) NextPathStep();

        //if(m_bEnableInput) HandleEvent();

        CallEvent(Event_OnUpdate);

    }
    else
    {
        if (m_iUnitType == Spr_Plot
           || (m_iUnitType >= Spr_Window && m_iUnitType <= Spr_InputText))
        {
            if(m_bEnableTimer)
            {
                int iCurrentTick = SDL_GetTicks();
                if(iCurrentTick - m_iTimerLastTick >= m_iTimerInterval)
                {
                    m_iTimerLastTick = iCurrentTick;
                    m_iTimerEventCount++;
                    CallEvent(Event_OnTimerTime);
                }
            }
            //CallEvent(Event_OnUpdate);
        }

        if(m_iUnitType < Spr_Npc)
        {
            if(m_bPathAutoStepIn && m_pCurrentPath && m_iPathStep) NextPathStep();
            CallEvent(Event_OnUpdate);
        }
    }


    if(m_bIsRelative && !m_pParent) AdjustSceneView();

    memcpy(&m_OldPosRect, &m_DrawPosRect, sizeof(m_DrawPosRect));

	if (m_pCurrentAnima)
	{
		m_pCurrentAnima->Update();
		PrepareNextFrame();

		/*
		if(m_bEnableAnima &&
		   m_pCurrentAnima->m_bFinished &&
		   m_pCurrentAnima->m_iTotalFrames > 0 &&
		   !m_pCurrentAnima->m_bAutoReplay)
        {
            //if()
            CallEvent(Event_OnAnimaFin);
        }
        */

	}
	else PrepareNextFrame();

	if(m_bPathStepIn && bAllowPath)
	{
	    CallEvent(Event_OnPathStep);
	    m_bPathStepIn = false;
    }

	if(m_iPathStep != 0 && bAllowPath)
	{
        if(m_iPathStep > 0)
        {
            int iMaxIdx = m_iPathLastIdx;
            if(m_pCurrentPath)
            {
                int iValidStepCount = m_pCurrentPath->GetTotalStepCount();
                if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;
                iMaxIdx = iValidStepCount - 1;
            }

            if(m_iPathStepIdx >= m_iPathLastIdx || m_iPathStepIdx >= iMaxIdx)
            {
                AbortPath();
                CallEvent(Event_OnPathFin);

                /*
                if(m_bJoinedPlot && m_pCurrentScene)
                {
                    CogeSprite* pPlotSpr = m_pCurrentScene->GetPlotSprite();
                    if(pPlotSpr) pPlotSpr->ResumeUpdateScript();
                }
                */
            }
        }
        else
        {
            int iMinIdx = 0;
            if(m_pCurrentPath)
            {
                int iTotalStepCount = m_pCurrentPath->GetTotalStepCount();
                int iValidStepCount = iTotalStepCount;
                if(m_iVirtualStepCount >= 0) iValidStepCount = m_iVirtualStepCount;
                iMinIdx = iTotalStepCount - iValidStepCount;
            }

            if(m_iPathStepIdx <= 0 || m_iPathStepIdx <= iMinIdx)
            {
                AbortPath();
                CallEvent(Event_OnPathFin);

                /*
                if(m_bJoinedPlot && m_pCurrentScene)
                {
                    CogeSprite* pPlotSpr = m_pCurrentScene->GetPlotSprite();
                    if(pPlotSpr) pPlotSpr->ResumeUpdateScript();
                }
                */
            }
        }
	}

    //if(m_pTextInputter) m_pTextInputter->SetInputWinPos(m_iPosX, m_iPosY);
    if(m_pTextInputter) m_pTextInputter->SetInputWinPos(m_iInputWinPosX, m_iInputWinPosY);

	//if(m_pCurrentThinker) m_pCurrentThinker->Think();

	//CallCustomEvents();

	// make OnIdle event be the last event in every update round
	//CallEvent(Event_OnIdle);

}

void CogeSprite::DefaultDraw()
{
    if(m_iState < 0) return;
    if (m_pCurrentAnima)
    {
        if(m_pAnimaEffect)
            m_pCurrentAnima->Draw(m_DrawPosRect.left, m_DrawPosRect.top, m_pAnimaEffect);
        else
            m_pCurrentAnima->Draw(m_DrawPosRect.left, m_DrawPosRect.top);
    }
}

void CogeSprite::Draw()
{
    if(m_iState < 0) return;

    if(m_bDefaultDraw)
    {
        if (m_pCurrentAnima)
        {
            if(m_pAnimaEffect)
                m_pCurrentAnima->Draw(m_DrawPosRect.left, m_DrawPosRect.top, m_pAnimaEffect);
            else
                m_pCurrentAnima->Draw(m_DrawPosRect.left, m_DrawPosRect.top);
        }

        /*
        if(m_iUnitType == Spr_InputText)
        {
            CogeImage* img = GetCurrentImage();
            if(img && m_pTextInputter)
            {
                std::string sText = m_pTextInputter->GetText();
                img->FillRect( ((255 << 16) & 0x00ff0000) | ((255 << 8) & 0x0000ff00) | (255&0xff) );
                img->PrintText(sText, 1, 1);
            }
        }
        */

    }

    //if(m_pEngine->m_pActiveScene) m_pEngine->m_pActiveScene->m_pCurrentSpr = this;
    //CallEvent(Event_OnDraw);

    if(m_pCurrentScene)
    {
        m_pCurrentScene->m_pCurrentSpr = this;
        m_pEngine->m_pCurrentGameSprite = this;
        CallEvent(Event_OnDraw);
    }

}

int CogeSprite::Initialize(const std::string& sConfigFileName)
{
    Finalize();

    if(!m_pEngine) return -1;

    CogeIniFile ini;

    //bool bLoadedConfig = ini.Load(sConfigFileName);
    bool bLoadedConfig = m_pEngine->LoadIniFile(&ini, sConfigFileName, m_pEngine->m_bPackedSprite, m_pEngine->m_bEncryptedSprite);

    if (bLoadedConfig)
    {
        ini.SetCurrentPath(OGE_GetAppGameDir());

        // base info ...

        m_sBaseName = ini.ReadString("Sprite", "Name",  "");

        SetType(ini.ReadInteger("Sprite", "Type",  1));

        int iDir    = ini.ReadInteger("Sprite", "Dir",   Dir_Down);
        int iAct    = ini.ReadInteger("Sprite", "Act",   Act_Stand);
        int iSpeedX = ini.ReadInteger("Sprite", "SpeedX", 100);
        int iSpeedY = ini.ReadInteger("Sprite", "SpeedY", 100);

        int iTimerInterval = ini.ReadInteger("Sprite", "TimerInterval", 0);

        int iClassTag = ini.ReadInteger("Sprite", "ClassTag", 0);
        int iObjectTag = ini.ReadInteger("Sprite", "Tag", 0);

        int iDefaultWidth   = ini.ReadInteger("Sprite", "DefaultWidth",  32);
        int iDefaultHeight  = ini.ReadInteger("Sprite", "DefaultHeight", 32);
        int iDefaultRootX   = ini.ReadInteger("Sprite", "DefaultRootX",  16);
        int iDefaultRootY   = ini.ReadInteger("Sprite", "DefaultRootY",  16);

        int iEnableAnima = ini.ReadInteger("Sprite", "EnableAnima", -1);
        if(iEnableAnima >= 0) m_bEnableAnima = iEnableAnima > 0;

        int iEnableMovement = ini.ReadInteger("Sprite", "EnableMovement", -1);
        if(iEnableMovement >= 0) m_bEnableMovement = iEnableMovement > 0;

        int iEnableCollision = ini.ReadInteger("Sprite", "EnableCollision", -1);
        if(iEnableCollision >= 0) m_bEnableCollision = iEnableCollision > 0;

        int iEnableInput = ini.ReadInteger("Sprite", "EnableInput", -1);
        if(iEnableInput >= 0) m_bEnableInput = iEnableInput > 0;

        int iEnableFocus = ini.ReadInteger("Sprite", "EnableFocus", -1);
        if(iEnableFocus >= 0) m_bEnableFocus = iEnableFocus > 0;

        int iEnableDrag = ini.ReadInteger("Sprite", "EnableDrag", -1);
        if(iEnableDrag >= 0) m_bEnableDrag = iEnableDrag > 0;


        int iRelative = ini.ReadInteger("Sprite", "Relative",   -1);

        //std::string sAIName = ini.ReadString("Sprite", "AI",  "");

        // section script ...
        if(m_pEngine->m_pScripter && m_pEngine->m_iScriptState >= 0)
        {
            CogeScript* pScript = NULL;

            std::string sScriptName = ini.ReadString("Script", "Name", "");
            if(sScriptName.length() > 0) pScript = m_pEngine->GetScript(sScriptName);

            if(pScript && pScript->GetState() >= 0)
            {
                m_pCommonScript = pScript;
                m_pCommonScript->Hire();

                std::map<std::string, int>::iterator it = m_pEngine->m_EventIdMap.begin(); //, itb, ite;

                int iCount = m_pEngine->m_EventIdMap.size();

                int iEventId  = -1;
                std::string sEventFunctionName = "";

                while(iCount > 0)
                {
                    iEventId  = -1;
                    sEventFunctionName = ini.ReadString("Script", it->first, "");
                    if(sEventFunctionName.length() > 0)
                    {
                        iEventId  = m_pCommonScript->PrepareFunction(sEventFunctionName);
                        //if(iEventId >= 0) m_Events[it->second] = iEventId;
                    }

                    m_CommonEvents[it->second] = iEventId;

                    it++;

                    iCount--;
                }

                m_iScriptState = 0;


                // setup custom events ...

                int iCustomEventNameCount = ini.ReadInteger("CustomEventNames", "Count", 0);
                if (iCustomEventNameCount > 64) iCustomEventNameCount = 64;
                if (iCustomEventNameCount > 0)
                {
                    m_CustomEventNames.resize(iCustomEventNameCount);

                    for(int i=0; i<iCustomEventNameCount; i++)
                    {
                        std::string sIdx = "CustomEventName" + OGE_itoa(i+1);
                        std::string sCustomEventName = ini.ReadString("CustomEventNames", sIdx, "");
                        m_CustomEventNames[i] = sCustomEventName;
                    }
                }

                iCustomEventNameCount = m_CustomEventNames.size();

                if(iCustomEventNameCount > 0)
                {
                    if(m_pCustomEvents != NULL) delete m_pCustomEvents;
                    m_pCustomEvents = new CogeCustomEventSet(m_sName + "_CustomEvents", m_pCommonScript);
                }

                for(int i=0; i<iCustomEventNameCount; i++)
                {
                    std::string sIdx = m_CustomEventNames[i];
                    if (sIdx.length() <= 0) continue;
                    std::string sCustomEvent = ini.ReadString("CustomEvents", sIdx, "");
                    if (sCustomEvent.length() <= 0) continue;

                    int iScriptEntryId  = m_pCommonScript->PrepareFunction(sCustomEvent);
                    if(iScriptEntryId >= 0) m_pCustomEvents->m_CommonEvents[i] = iScriptEntryId;

                }


            }
        }

        // mask section
        if(m_pEngine->m_pVideo)
        {
            //m_bEnableLightMap   = ini.ReadInteger("Mask", "UseMask",  0) == 1;
            //m_iLightLocalX      = ini.ReadInteger("Mask", "LocalX",  0);
            //m_iLightLocalY      = ini.ReadInteger("Mask", "LocalY",  0);
            //m_iLightLocalWidth  = ini.ReadInteger("Mask", "LocalWidth",  0);
            //m_iLightLocalHeight = ini.ReadInteger("Mask", "LocalHeight",  0);
            //m_iLightRelativeX   = ini.ReadInteger("Mask", "DrawRelativeX",  0);
            //m_iLightRelativeY   = ini.ReadInteger("Mask", "DrawRelativeY",  0);

            std::string sMaskName = ini.ReadString("Mask", "LightMaskImage", "");

            if(sMaskName.length() > 0)
            {
                CogeImage* pImage = m_pEngine->GetImage(sMaskName);
                SetLightMap(pImage);
            }

            m_bEnableLightMap  =  m_pLightMap != NULL;

        }

        // section effect
        int iEffectCount = ini.ReadInteger("Effect", "Count",  0);
        for(int i=1; i<=iEffectCount; i++)
        {
            std::string sEffectIdx = "Effect_" + OGE_itoa(i);
            int iEffectType = ini.ReadInteger(sEffectIdx, "EffectType",  Effect_Alpha);
            double fEffectValue = ini.ReadFloat(sEffectIdx, "EffectValue",  0);

            AddAnimaEffect(iEffectType, fEffectValue, true);

        }


        // section anima ...

        int iAnimaCount = ini.ReadInteger("Animation", "Count",  0);

        if(iAnimaCount <= 0)
        {
            //m_iUnitType = Spr_Empty;
            m_bVisible  = false;
            m_bEnableAnima = false;
            //m_bEnableCollision = true;
        }
        else
        for(int i=1; i<=iAnimaCount; i++)
        {

            std::string sIdx = "Anima_" + OGE_itoa(i);
            std::string sImageName = ini.ReadString(sIdx, "Image", "");
            int iAnimaDir = ini.ReadInteger(sIdx, "Dir",  Dir_Down);
            int iAnimaAct = ini.ReadInteger(sIdx, "Act",  Act_Stand);
            int iLocalX = ini.ReadInteger(sIdx, "LocalX",  0);
            int iLocalY = ini.ReadInteger(sIdx, "LocalY",  0);
            int iLocalWidth = ini.ReadInteger(sIdx, "LocalWidth",  0);
            int iLocalHeight = ini.ReadInteger(sIdx, "LocalHeight",  0);
            int iFrameWidth = ini.ReadInteger(sIdx, "FrameWidth",  0);
            int iFrameHeight = ini.ReadInteger(sIdx, "FrameHeight",  0);
            int iTotalFrames = ini.ReadInteger(sIdx, "FrameCount",  0);
            int iRootX = ini.ReadInteger(sIdx, "RootX",  iFrameWidth/2);
            int iRootY = ini.ReadInteger(sIdx, "RootY",  iFrameHeight);

            int iBodyLeft = ini.ReadInteger(sIdx, "BodyLeft",  0);
            int iBodyTop = ini.ReadInteger(sIdx, "BodyTop",  0);
            int iBodyRight = ini.ReadInteger(sIdx, "BodyRight",  iFrameWidth);
            int iBodyBottom = ini.ReadInteger(sIdx, "BodyBottom",  iFrameHeight);

            int iAutoReplay = ini.ReadInteger(sIdx, "AutoReplay",  1);
            int iReplayInterval = ini.ReadInteger(sIdx, "ReplayInterval",  10);
            int iFrameInterval = ini.ReadInteger(sIdx, "FrameInterval",  10);
            int iEffect = ini.ReadInteger(sIdx, "Effect",  -1);

            int iEffectMode = iEffect;

            if (iEffect <= -1) iEffectMode = Effect_M_None;
            if (iEffect == 0)  iEffectMode = Effect_M_Sprite;
            if (iEffect > 0)   iEffectMode = Effect_M_Frame;

            CogeAnima* pAnima = new CogeAnima(m_sName + "_" + sIdx, m_pEngine);
            int iRsl = pAnima->InitImages(sImageName, iLocalX, iLocalY, iLocalWidth, iLocalHeight);
            if(pAnima->m_bImageOK) iRsl = pAnima->InitFrames(iTotalFrames, iFrameWidth, iFrameHeight, iRootX, iRootY);
            if(pAnima->m_bFrameOK) iRsl = pAnima->InitPlay(iFrameInterval, iAutoReplay!=0, iReplayInterval, iEffectMode);
            if (iRsl < 0)
            {
                delete pAnima;
                pAnima = NULL;
                continue;
            }

            pAnima->InitBody(iBodyLeft, iBodyTop, iBodyRight, iBodyBottom);

            if (iEffectMode == Effect_M_Sprite)
            {
                /*
                std::string sEffectIdx = sIdx + "_Effect_0";
                //int iFrameID = ini.ReadInteger(sEffectIdx, "FrameID",  1);
                int iEffectType = ini.ReadInteger(sEffectIdx, "EffectType",  6);
                int iEffectValue = ini.ReadInteger(sEffectIdx, "EffectValue",  0);

                pAnima->SetAnimaEffect(iEffectType, iEffectValue);
                */
            }

            if (iEffectMode == Effect_M_Frame)
            for(int j=1; j<=iEffect; j++)
            {
                std::string sEffectIdx = sIdx + "_Effect_" + OGE_itoa(j);
                int iFrameID = ini.ReadInteger(sEffectIdx, "FrameID",  1);
                int iEffectType = ini.ReadInteger(sEffectIdx, "EffectType",  Effect_Alpha);
                double fEffectValue = ini.ReadFloat(sEffectIdx, "EffectValue",  0);

                pAnima->AddFrameEffect(iFrameID, iEffectType, fEffectValue);
            }


            // bind anima ...
            BindAnima(iAnimaDir, iAnimaAct, pAnima);


        }

        // section data ...

        m_bHasDefaultData = ini.ReadInteger("Data", "Default", 0) != 0;

        if(m_bHasDefaultData)
        {
            //std::string sDataName = m_sName;
            int iIntCount  = ini.ReadInteger("Data", "IntVarCount", 0);
            int iFloatCount  = ini.ReadInteger("Data", "FloatVarCount", 0);
            int iStrCount  = ini.ReadInteger("Data", "StrVarCount", 0);

            CogeGameData* pGameData = m_pEngine->NewFixedGameData(m_sName, iIntCount, iFloatCount, iStrCount);

            if(pGameData)
            {
                std::string sKeyName = "";
                std::string sValue = "";
                int iValue = 0;
                double fValue = 0;

                int iIntCount = pGameData->GetIntVarCount();
                for(int i=1; i<=iIntCount; i++)
                {
                    sKeyName = "IntVar" + OGE_itoa(i);
                    sValue = ini.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        iValue = ini.ReadInteger("Data", sKeyName, 0);
                        pGameData->SetIntVar(i, iValue);
                    }
                }

                int iFloatCount = pGameData->GetFloatVarCount();
                for(int i=1; i<=iFloatCount; i++)
                {
                    sKeyName = "FloatVar" + OGE_itoa(i);
                    sValue = ini.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        fValue = ini.ReadFloat("Data", sKeyName, 0);
                        pGameData->SetFloatVar(i, fValue);
                    }
                }

                int iStrCount = pGameData->GetStrVarCount();
                for(int i=1; i<=iStrCount; i++)
                {
                    sKeyName = "StrVar" + OGE_itoa(i);
                    sValue = ini.ReadString("Data", sKeyName, "");
                    if(sValue.length() > 0)
                    {
                        pGameData->SetStrVar(i, sValue);
                    }
                }

                BindGameData(pGameData);
            }
        }


        // if all is ok then set status is ok ...
        m_iState = 0;

        SetDefaultSize(iDefaultWidth, iDefaultHeight);
        SetDefaultRoot(iDefaultRootX, iDefaultRootY);

        SetAnima(iDir, iAct);

        SetSpeed(iSpeedX, iSpeedY);

        SetClassTag(iClassTag);
        SetObjectTag(iObjectTag);

        //if(sAIName.length() > 0) UseThinker(sAIName);


        if(iRelative >= 0) m_bIsRelative = iRelative > 0;


        if(m_iUnitType == Spr_InputText)
        {
            CogeTextInputter* inputter = m_pEngine->GetTextInputter(m_sName + "_IM");
            if(inputter) ConnectTextInputter(inputter->GetName());
        }

        if(m_iUnitType == Spr_Timer)
        {
            m_iDefaultTimerInterval = iTimerInterval;
        }


        /*
        if(m_pEngine->m_pLoadingScene)
        {

            CogeSprite* pCurrentSprite = m_pEngine->m_pCurrentGameSprite;
            m_pEngine->m_pCurrentGameSprite = this;
            CallEvent(Event_OnInit);
            m_pEngine->m_pCurrentGameSprite = pCurrentSprite;

        }
        else
        */

        m_pEngine->m_InitSprites.insert(ogeSpriteMap::value_type(m_sName, this));

        //CallEvent(Event_OnInit); // not work when create in script ...

        //m_iMovingInterval = 100;
        //m_iMovingTime = 0;


    }
    else
    {
        //printf("Load Sprite Config File Failed.\n");
        //if (m_iState >= 0) DoInit(this);
    }

    if(m_iState < 0 ) OGE_Log("Load Sprite Config File Failed.\n");
    if(m_iScriptState < 0) OGE_Log("Load Sprite Script Failed.\n");

    if(m_iState >= 0 && m_iScriptState >= 0)
    {
        //m_pEngine->m_SpriteMap.insert(ogeSpriteMap::value_type(m_sName, this));


        // add children ...

        int iChildCount = ini.ReadInteger("Child", "Count",  0);

        for(int i=1; i<=iChildCount; i++)
        {
            std::string sIdx = "Child_" + OGE_itoa(i);
            std::string sBaseName = ini.ReadString(sIdx, "Name", "");
            std::string sChildName = m_sName + "_" + sBaseName;

            /*
            size_t pos = m_sName.find('_', 0);
            if(pos != std::string::npos)
            {
                std::string sPrefix = m_sName.substr(0, pos) + "_";
                if(sChildName.find(sPrefix, 0) == std::string::npos)
                sChildName = sPrefix + sBaseName;
            }
            */

            int iRelativeX = ini.ReadInteger(sIdx, "RelativeX",  0);
            int iRelativeY = ini.ReadInteger(sIdx, "RelativeY",  0);

            //std::string sConfigFileName = m_pEngine->m_ObjectIniFile.ReadFilePath("Sprites", sBaseName, "");
            //if (sConfigFileName.length() == 0) continue;
            ////CogeSprite* pChild = m_pEngine->GetSpriteWithConfig(sChildName, sConfigFileName);
            //CogeSprite* pChild = m_pEngine->NewSpriteWithConfig(sChildName, sConfigFileName);

            CogeSprite* pChild = m_pEngine->NewSprite(sChildName, sBaseName);

            if(pChild) this->AddChild(pChild, iRelativeX, iRelativeY);

        }

    }


    return m_iState;
}

int CogeSprite::LoadLocalScript(CogeIniFile* pIniFile, const std::string& sSectionName)
{
    CogeScript* pScript = NULL;

    std::string sScriptName = pIniFile->ReadString(sSectionName, "Script", "");

    if(sScriptName.length() > 0)
    {
        std::string sScriptFileName  = pIniFile->ReadFilePath("Scripts", sScriptName, "");
        if(sScriptFileName.length() > 0) pScript = m_pEngine->NewFixedScript(sScriptFileName);
    }

    //if(sScriptName.length() > 0) pScript = m_pEngine->GetScript(sScriptName);
    if(pScript && pScript->GetState() >= 0)
    {
        m_pLocalScript = pScript;
        m_pLocalScript->Hire();

        std::map<std::string, int>::iterator it = m_pEngine->m_EventIdMap.begin();

        int iCount = m_pEngine->m_EventIdMap.size();

        int iEventId  = -1;
        std::string sEventFunctionName = "";

        while(iCount > 0)
        {
            iEventId  = -1;
            sEventFunctionName = pIniFile->ReadString(sSectionName, it->first, "");
            if(sEventFunctionName.length() > 0)
            {
                iEventId  = m_pLocalScript->PrepareFunction(sEventFunctionName);
                if(iEventId >= 0) m_LocalEvents[it->second] = iEventId; // override it only when it is valid ...
            }

            //m_Events[it->second] = iEventId;

            it++;

            iCount--;
        }


        // custom ...

        int iOverride = 0;
        int iCustomEventNameCount = m_CustomEventNames.size();
        for(int i=0; i<iCustomEventNameCount; i++)
        {
            std::string sIdx = m_CustomEventNames[i];
            if (sIdx.length() <= 0) continue;
            std::string sCustomEvent = pIniFile->ReadString(sSectionName, sIdx, "");
            if (sCustomEvent.length() <= 0) continue;

            iOverride++;
            break;
        }

        if(iOverride > 0)
        {
            if(m_pCustomEvents)
            {
                int CustomCommonEvents[_OGE_MAX_EVENT_COUNT_];
                memcpy((void*)&CustomCommonEvents[0],  (void*)&(m_pCustomEvents->m_CommonEvents[0]), sizeof(int) * _OGE_MAX_EVENT_COUNT_);
                delete m_pCustomEvents;
                m_pCustomEvents = new CogeCustomEventSet(m_sName + "_CustomEvents", m_pCommonScript, m_pLocalScript);
                memcpy((void*)&(m_pCustomEvents->m_CommonEvents[0]),  (void*)&CustomCommonEvents[0], sizeof(int) * _OGE_MAX_EVENT_COUNT_);
            }

            for(int i=0; i<iCustomEventNameCount; i++)
            {
                std::string sIdx = m_CustomEventNames[i];
                if (sIdx.length() <= 0) continue;
                std::string sCustomEvent = pIniFile->ReadString(sSectionName, sIdx, "");
                if (sCustomEvent.length() <= 0) continue;

                int iScriptEntryId  = m_pLocalScript->PrepareFunction(sCustomEvent);
                if(iScriptEntryId >= 0) m_pCustomEvents->m_LocalEvents[i] = iScriptEntryId;
            }

        }

        m_iScriptState = 0;
    }

    return m_iScriptState;

}

void CogeSprite::DelAllEffects()
{
    if(m_pAnimaEffect == NULL) return;

    m_pAnimaEffect->Clear();

}

void CogeSprite::DelAllAnimation()
{
    //m_iState = -1;

    // free all Effects
	ogeAnimaMap::iterator it;
	CogeAnima* pMatchedAnima = NULL;

	it = m_Animation.begin();
	int iCount = m_Animation.size();
	for(int i=0; i<iCount; i++)
	{
	    pMatchedAnima = it->second;
	    delete pMatchedAnima;
	    it++;
	}
	m_Animation.clear();

}

void CogeSprite::Finalize()
{
    m_iScriptState = -1;
    m_iState = -1;
    m_Children.clear();
    //m_EventMap.clear();

    m_TileList.clear();

    m_CustomEventNames.clear();

    //if(m_pLightMap) m_pEngine->m_pVideo->DelImage(m_pLightMap->GetName());
    if(m_pLightMap)
    {
        m_pLightMap->Fire();
        //m_pEngine->m_pVideo->DelImage(m_pLightMap->GetName());
        m_pEngine->m_pVideo->DelImage(m_pLightMap);
    }
    m_pLightMap = NULL;

    //m_pCommonScript = NULL;
    //m_pLocalScript = NULL;

    if (m_pLocalScript)
    {
        //m_pLocalScript->Fire();
        //m_pEngine->m_pScripter->DelScript(m_pLocalScript->GetName());

        m_pEngine->m_pScripter->DelScript(m_pLocalScript);

        m_pLocalScript = NULL;
    }

    if (m_pCommonScript)
    {
        //m_pCommonScript->Fire();
        //m_pEngine->m_pScripter->DelScript(m_pCommonScript->GetName());
        m_pCommonScript = NULL;
    }

    if(m_pTextInputter) DisconnectTextInputter();

    DelAllEffects();

    DelAllAnimation();

	//CogeGameData* pData = GetGameData();
	//RemoveGameData();
	//if(pData) m_pEngine->DelGameData(pData->GetName()); // normally we may delete it ...

	if(m_bHasDefaultData && m_pGameData)
    {
        //std::string sDataName = m_sName + "_Data";
        //m_pEngine->DelGameData(sDataName);
        m_pEngine->DelGameData(m_pGameData);
        m_pGameData = NULL;
    }

    if(m_bOwnCustomData && m_pCustomData)
    {
        std::string sDataName = m_pCustomData->GetName();
        m_pEngine->DelGameData(sDataName);
    }

    m_pCustomData = NULL;

}



/*---------------- Animation -----------------*/

CogeAnima::CogeAnima(const std::string& sName, CogeEngine* pEngine):
m_pEngine(pEngine),
m_pVideo(NULL),
m_pImage(NULL),
m_pSprite(NULL),
m_sName(sName)
{
    if(pEngine && pEngine->m_pVideo) m_pVideo = pEngine->m_pVideo;

    memset(&m_Local, 0, sizeof(m_Local));
    memset(&m_FrameRect, 0, sizeof(m_FrameRect));
    memset(&m_Body, 0, sizeof(m_Body));

    //memset(&m_AnimaEffect, 0, sizeof(m_AnimaEffect));

    m_iEffectMode = Effect_M_None;

    m_bFinished = false;

    m_bAutoReplay = false;

    m_iCurrentFrame = 0;

    m_iPlayTime = 0;

    m_iTotalFrames = 0;

    m_iFrameInterval = 0;

    m_iReplayInterval = 0;

    m_iRootX = 0;
    m_iRootY = 0;

    m_iLocalRight = 0;
    m_iLocalBottom = 0;

    m_iFrameWidth = 0;
    m_iFrameHeight = 0;

    m_bImageOK = false;
    m_bFrameOK = false;

    //m_sImageName = "";

    m_iState = -1;

}

CogeAnima::~CogeAnima()
{
    DelAllEffects();
    if(m_pVideo && m_pImage)
    {
        m_pImage->Fire();
        //m_pVideo->DelImage(m_sImageName);
        m_pVideo->DelImage(m_pImage);
    }
}

void CogeAnima::DelAllEffects()
{
    //m_iState = -1;

    // free all Effects

	ogeFrameEffectMap::iterator it;
	CogeFrameEffect* pMatchedEffect = NULL;

	it = m_FrameEffectMap.begin();

	while (it != m_FrameEffectMap.end())
	{
		pMatchedEffect = it->second;
		m_FrameEffectMap.erase(it);
		delete pMatchedEffect;
		it = m_FrameEffectMap.begin();
	}

}

int CogeAnima::InitImages(const std::string& sImageName,
                          int iLocalX, int iLocalY, int iLocalWidth, int iLocalHeight)
{
    if(m_pEngine && m_pVideo)
    {
        CogeImage* pImage = m_pEngine->GetImage(sImageName);
        if(pImage == NULL) return -1;

        if(iLocalX < 0 || iLocalY < 0) return -1;
        if(iLocalWidth < 0 || iLocalHeight < 0) return -1;

        if(iLocalX+iLocalWidth  > pImage->GetWidth() ||
           iLocalY+iLocalHeight > pImage->GetHeight()) return -1;

        m_Local.x  = iLocalX;
        m_Local.y  = iLocalY;
        m_Local.w  = iLocalWidth;
        m_Local.h  = iLocalHeight;

        m_iLocalRight = m_Local.x + m_Local.w;
        m_iLocalBottom = m_Local.y + m_Local.h;

        if(m_pVideo && m_pImage)
        {
            m_pImage->Fire();
            //m_pVideo->DelImage(m_sImageName);
            m_pVideo->DelImage(m_pImage);
        }

        m_pImage = pImage;

        m_bImageOK = true;

        m_pImage->Hire();
        //m_sImageName = m_pImage->GetName();

        return 0;
    }

    return -1;
}

int CogeAnima::InitFrames(int iTotalFrames,
                          int iFrameWidth, int iFrameHeight, int iRootX, int iRootY)
{
    if (!m_bImageOK) return -1;

    if(iTotalFrames <= 0) return -1;
    if(iFrameWidth < 0 || iFrameHeight < 0) return -1;
    if(iRootX < 0) iRootX = iFrameWidth/2;
    if(iRootY < 0) iRootY = iFrameHeight;
    if(iRootX > iFrameWidth || iRootY > iFrameHeight) return -1;

    int a = m_Local.w * m_Local.h;
    int b = iFrameWidth * iFrameHeight;

    if (a < b * iTotalFrames) return -1;
    if (a%b != 0) return -1;

    m_iTotalFrames = iTotalFrames;
    m_iFrameWidth  = iFrameWidth;
    m_iFrameHeight = iFrameHeight;
    m_iRootX       = iRootX;
    m_iRootY       = iRootY;

    m_Body.left = 0;
    m_Body.top = 0;
    m_Body.right = iFrameWidth;
    m_Body.bottom = iFrameHeight;

    m_FrameRect.w = iFrameWidth;
    m_FrameRect.h = iFrameHeight;

    m_bFrameOK = true;

    return 0;
}

void CogeAnima::InitBody(int iLeft, int iTop, int iRight, int iBottom)
{
    m_Body.left = iLeft;
    m_Body.top = iTop;
    m_Body.right = iRight;
    m_Body.bottom = iBottom;

}

int CogeAnima::InitPlay(int iFrameInterval, bool bAutoReplay,
                        int iReplayInterval, int iEffectMode)
{
    if (!m_bImageOK) return -1;
    if (!m_bFrameOK) return -1;

    if(iFrameInterval < 0) iFrameInterval = 0;
    if(iReplayInterval < 0) iReplayInterval = 0;
    if(iEffectMode < 0) iEffectMode = 0;

    m_iFrameInterval  = iFrameInterval;
    m_iReplayInterval = iReplayInterval;

    m_bAutoReplay = bAutoReplay;
    m_iEffectMode = iEffectMode;

    m_iState = 0;

    return m_iState;
}

int CogeAnima::Init(const std::string& sImageName,
                    int iLocalX, int iLocalY, int iLocalWidth, int iLocalHeight,
                    int iTotalFrames, int iFrameWidth, int iFrameHeight, int iFrameInterval)
{
    int iRsl = InitImages(sImageName, iLocalX, iLocalY, iLocalWidth, iLocalHeight);
    if(m_bImageOK) iRsl = InitFrames(iTotalFrames, iFrameWidth, iFrameHeight);
    if(m_bFrameOK) iRsl = InitPlay(iFrameInterval);
    return iRsl;
}

/*
void CogeAnima::SetAnimaEffect(int iEffectType, double fEffectValue)
{
    memset(&m_AnimaEffect, 0, sizeof(m_AnimaEffect));

    switch (iEffectType)
    {
    case Effect_Alpha:
    m_AnimaEffect.use_alpha = true;
    m_AnimaEffect.value_alpha = (int)fEffectValue;
    break;
    case Effect_Rota:
    m_AnimaEffect.use_rota = true;
    m_AnimaEffect.value_rota = (int)fEffectValue;
    break;
    case Effect_RGB:
    m_AnimaEffect.use_light = true;
    m_AnimaEffect.value_light = (int)fEffectValue;
    break;
    case Effect_Edge:
    m_AnimaEffect.use_edge = true;
    m_AnimaEffect.value_edge = (int)fEffectValue;
    break;
    case Effect_Color:
    m_AnimaEffect.use_color = true;
    m_AnimaEffect.value_color = (int)fEffectValue;
    break;
    case Effect_Zoom:
    m_AnimaEffect.use_zoom = true;
    m_AnimaEffect.value_zoom = fEffectValue;
    break;
    }
}
*/

/*
int CogeAnima::AddFrameEffect(int iFrameIdx, int iEffectType, double fEffectValue)
{
    if (iFrameIdx < 1) return -1;
    if (iFrameIdx > m_iTotalFrames) return -1;

    ogeEffectMap::iterator it;

    CogeEffect* pEffect = NULL;

    it = m_EffectMap.find(iFrameIdx);
	if (it != m_EffectMap.end()) pEffect = it->second;

	if(pEffect == NULL)
	{
	    pEffect = new CogeEffect();
	    memset(pEffect, 0, sizeof(CogeEffect));
	    m_EffectMap.insert(ogeEffectMap::value_type(iFrameIdx, pEffect));
    }

    bool bNewEffect = true;

    switch (iEffectType)
    {
    case Effect_Alpha:
        if(fEffectValue < 0) return -1;
        if(pEffect->use_alpha) bNewEffect = false;
        else pEffect->use_alpha = true;
        pEffect->value_alpha = (int)fEffectValue;
        break;
    case Effect_Rota:
        if(fEffectValue < 0) return -1;
        if(pEffect->use_rota) bNewEffect = false;
        else pEffect->use_rota = true;
        //pEffect->use_rota = true;
        pEffect->value_rota = (int)fEffectValue;
        break;
    case Effect_RGB:
        //if(fEffectValue < 0) return -1;
        if(pEffect->use_light) bNewEffect = false;
        else pEffect->use_light = true;
        //pEffect->use_light = true;
        pEffect->value_light = (int)fEffectValue;
        break;
    case Effect_Edge:
        //if(fEffectValue < 0) return -1;
        if(pEffect->use_edge) bNewEffect = false;
        else pEffect->use_edge = true;
        //pEffect->use_edge = true;
        pEffect->value_edge = (int)fEffectValue;
        break;
    case Effect_Color:
        //if(fEffectValue < 0) return -1;
        if(pEffect->use_color) bNewEffect = false;
        else pEffect->use_color = true;
        //pEffect->use_color = true;
        pEffect->value_color = (int)fEffectValue;
        break;
    case Effect_Scale:
        if(fEffectValue < 0) return -1;
        if(pEffect->use_scale) bNewEffect = false;
        else pEffect->use_scale = true;
        //pEffect->use_zoom = true;
        pEffect->value_scale = fEffectValue;
        break;
    default:
        bNewEffect = false;
    }

    if(bNewEffect) pEffect->count = pEffect->count + 1;

    if(pEffect->count < 0) pEffect->count = 0;
    if(pEffect->count > 6) pEffect->count = 6;

    //pEffect->idx = 0;

    //return m_EffectMap.size();

    return iFrameIdx;

}

CogeEffect* CogeAnima::GetFrameEffect(int iFrameIdx)
{
    ogeEffectMap::iterator it;
	it = m_EffectMap.find(iFrameIdx);
	if(it != m_EffectMap.end()) return it->second;
	else return NULL;
}
*/

int CogeAnima::AddFrameEffect(int iFrameIdx, int iEffectType, double fEffectValue)
{
    if (iFrameIdx < 1) return -1;
    if (iFrameIdx > m_iTotalFrames) return -1;

    ogeFrameEffectMap::iterator it;

    CogeFrameEffect* pFrameEffect = NULL;

    it = m_FrameEffectMap.find(iFrameIdx);
	if (it != m_FrameEffectMap.end()) pFrameEffect = it->second;

	if(pFrameEffect == NULL)
	{
	    pFrameEffect = new CogeFrameEffect();
	    pFrameEffect->AddEffect(iEffectType, fEffectValue);
	    pFrameEffect->SetFrameId(iFrameIdx);
	    m_FrameEffectMap.insert(ogeFrameEffectMap::value_type(iFrameIdx, pFrameEffect));
    }
    else
    {
        pFrameEffect->AddEffect(iEffectType, fEffectValue);
    }


    return iFrameIdx;

}

CogeFrameEffect* CogeAnima::GetFrameEffect(int iFrameIdx)
{
    ogeFrameEffectMap::iterator it;
	it = m_FrameEffectMap.find(iFrameIdx);
	if(it != m_FrameEffectMap.end()) return it->second;
	else return NULL;
}

void CogeAnima::Reset()
{
    //m_iCurrentFrame = 0;

    //m_iCurrentFrame = 1;
    //m_FrameRect.x = m_Local.x;
    //m_FrameRect.y = m_Local.y;
	//m_iPlayTime = 0;
	//m_bFinished = false;

	m_iPlayTime = 0;
    m_iCurrentFrame = 1;
    m_FrameRect.x = m_Local.x;
    m_FrameRect.y = m_Local.y;
    m_bFinished = false;
}

CogeImage* CogeAnima::GetImage()
{
    return m_pImage;
}

bool CogeAnima::SetImage(CogeImage* pImage)
{
    if(m_iState < 0 || m_pImage == NULL || pImage == NULL) return false;
    if(m_pImage->GetWidth() != pImage->GetWidth() || m_pImage->GetHeight() != pImage->GetHeight()) return false;
    m_pImage = pImage;
    return true;
}

void CogeAnima::Draw(int iPosX, int iPosY, CogeFrameEffect* pGlobalEffect)
{
    if(m_iState < 0 || m_iCurrentFrame <= 0) return;

    int iGlobalEffectCount = 0;
    int iDrawWidth = -1;
    int iDrawHeight = -1;

    CogeImage* pMainScreen = m_pVideo->GetScreen();
    CogeImage* pClipboardA = m_pVideo->GetClipboardA();
    CogeImage* pClipboardB = m_pVideo->GetClipboardB();

    //if(pGlobalEffect) iGlobalEffectCount = pGlobalEffect->GetEffectCount();

    if(pGlobalEffect) iGlobalEffectCount = pGlobalEffect->GetActiveEffectCount();

    if(iGlobalEffectCount > 0)
    {
        if(m_pImage->HasLocalClipboard())
            m_pImage->PrepareLocalClipboards(iGlobalEffectCount);

        if(iGlobalEffectCount == 1)
        {
            //ogeEffectMap Effects = pGlobalEffect->GetEffects();
            //ogeEffectMap::iterator it;

            //it = Effects.begin();

            //CogeEffect* pEffect = it->second;

            CogeEffect* pEffect = pGlobalEffect->GetFirstActiveEffect();

            int iEffectValue = (int)pEffect->effect_value;

            switch(pEffect->effect_type)
            {
            case Effect_Edge:
                pMainScreen->BltWithEdge( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;

            case Effect_RGB:
                pMainScreen->BltChangedRGB( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;

            case Effect_Color:
            {
                uint32_t iRealColor =  iEffectValue & 0x00ffffff;
                uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
                pMainScreen->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            }
            break;

            case Effect_Rota:
                pMainScreen->BltRotate( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;

            case Effect_Wave:
                pMainScreen->BltWave( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;

            case Effect_Lightness:
                pMainScreen->BltWithLightness( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;

            case Effect_Scale:
            {
                iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);
                int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);
                pMainScreen->BltStretch( m_pImage,
                m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
            }
            break;

            case Effect_Alpha:
                pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
            break;
            }

            if(pEffect->effect_value != pEffect->end_value)
            {
                //pEffect->time += m_pEngine->m_iFrameInterval;
                pEffect->time += m_pEngine->m_iCurrentInterval;
                if (pEffect->time >= pEffect->interval)
                {
                    if(pEffect->step_value > 0)
                    {
                        if(pEffect->effect_value < pEffect->end_value)
                        pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                        if(pEffect->effect_value > pEffect->end_value)
                        pEffect->effect_value = pEffect->end_value;
                    }
                    else if(pEffect->step_value < 0)
                    {
                        if(pEffect->effect_value > pEffect->end_value)
                        pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                        if(pEffect->effect_value < pEffect->end_value)
                        pEffect->effect_value = pEffect->end_value;
                    }

                    pEffect->time = 0;
                }
            }
            else
            {
                if(pEffect->repeat_times != 0)
                {
                    pEffect->effect_value = pEffect->start_value;
                    if(pEffect->repeat_times > 0) pEffect->repeat_times = pEffect->repeat_times - 1;
                    //else pEffect->repeat_times = 0;
                }
                else
                {
                    pEffect->active = false;

                    /*
                    if(m_pSprite && m_pSprite->m_bJoinedPlot && m_pSprite->m_pCurrentScene)
                    {
                        CogeSprite* pPlotSpr = m_pSprite->m_pCurrentScene->GetPlotSprite();
                        if(pPlotSpr) pPlotSpr->ResumeUpdateScript();
                    }
                    */
                }
            }

        }

        if(iGlobalEffectCount > 1 && pClipboardA && pClipboardB)
        {
            CogeImage* pCurrentClipboard = NULL;
            CogeImage* pFreeClipboard = NULL;
            int iHandled = 0;

            int iSrcColorKey = m_pImage->GetColorKey();

            if(iSrcColorKey != -1)
            {
                pClipboardA->SetColorKey(iSrcColorKey);
                pClipboardB->SetColorKey(iSrcColorKey);
            }

            CogeEffect* pEffect = NULL;

            bool bFin = false;

            ogeEffectList Effects = pGlobalEffect->GetEffects();
            ogeEffectList::iterator it;

            it = Effects.begin();

            while (it != Effects.end() && !bFin)
            {
                //pEffect = it->second;
                pEffect = *it;

                if(pEffect->active == false)
                {
                    it++;
                    continue;
                }

                int iEffectValue = (int)pEffect->effect_value;

                switch(pEffect->effect_type)
                {
                case Effect_Edge:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithEdge( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltWithEdge( m_pImage, iEffectValue,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithEdge( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltWithEdge( pCurrentClipboard, iEffectValue,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;

                        }
                    }
                break;

                case Effect_RGB:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltChangedRGB( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltChangedRGB( m_pImage, iEffectValue,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltChangedRGB( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltChangedRGB( pCurrentClipboard, iEffectValue,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;
                        }
                    }
                break;

                case Effect_Color:
                {
                    iHandled++;
                    uint32_t iRealColor =  iEffectValue & 0x00ffffff;
                    uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithColor( pCurrentClipboard, iRealColor, iRealAlpha,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltWithColor( pCurrentClipboard, iRealColor, iRealAlpha,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;
                        }
                    }
                }
                break;

                case Effect_Rota:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount || m_pImage->HasLocalClipboard() )
                        {
                            pMainScreen->BltRotate( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltRotate( m_pImage, iEffectValue,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                        }

                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltRotate( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltRotate( pCurrentClipboard, iEffectValue,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;

                        }
                    }

                break;


                case Effect_Wave:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWave( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltWave( m_pImage, iEffectValue,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                        }

                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWave( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltWave( pCurrentClipboard, iEffectValue,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;

                        }
                    }

                break;

                case Effect_Lightness:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithLightness( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pCurrentClipboard->BltWithLightness( m_pImage, iEffectValue,
                            0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                        }

                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iHandled == iGlobalEffectCount)
                        {
                            pMainScreen->BltWithLightness( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                            pFreeClipboard->BltWithLightness( pCurrentClipboard, iEffectValue,
                            0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                            pCurrentClipboard = pFreeClipboard;

                        }
                    }

                break;

                case Effect_Scale:
                    iHandled++;
                    if(pCurrentClipboard == NULL)
                    {
                        iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                        iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);

                        if(iHandled == iGlobalEffectCount || m_pImage->HasLocalClipboard())
                        {
                            int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                            int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                            pMainScreen->BltStretch( m_pImage,
                             m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                            iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
                        }
                        else
                        {
                            pCurrentClipboard = pClipboardA;
                            if(iSrcColorKey != -1)
                            pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, iDrawWidth, iDrawHeight);
                            pCurrentClipboard->BltStretch( m_pImage,
                             m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                            0,  0, iDrawWidth,  iDrawHeight);

                            //fScaleValue = pEffect->effect_value;
                        }

                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                        iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);

                        if(iHandled == iGlobalEffectCount)
                        {
                            int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                            int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                            pMainScreen->BltStretch( pCurrentClipboard,
                             0,  0,  m_FrameRect.w,  m_FrameRect.h,
                            iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
                        }
                        else
                        {
                            if(iSrcColorKey != -1)
                            pFreeClipboard->FillRect(iSrcColorKey, 0, 0, iDrawWidth, iDrawHeight);

                            pFreeClipboard->BltStretch( pCurrentClipboard,
                            0,  0, m_FrameRect.w, m_FrameRect.h,
                            0,  0, iDrawWidth, iDrawHeight);

                            pCurrentClipboard = pFreeClipboard;

                        }
                    }
                    //bFin = true;
                break;

                case Effect_Alpha:
                    iHandled++;
                    if(pCurrentClipboard == NULL || m_pImage->HasLocalClipboard())
                    {
                        //pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                        //iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                        if(iDrawWidth < 0)
                        {
                            pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                            iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                            int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                            pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                            iDrawX,  iDrawY, 0,  0,  iDrawWidth,  iDrawHeight);

                        }
                    }
                    else
                    {
                        if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                        else pFreeClipboard = pClipboardA;

                        if(iDrawWidth < 0)
                        {
                            pMainScreen->BltAlphaBlend( pCurrentClipboard, iEffectValue,
                            iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                        }
                        else
                        {
                            int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                            int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                            pMainScreen->BltAlphaBlend( pCurrentClipboard, iEffectValue,
                            iDrawX,  iDrawY, 0,  0,  iDrawWidth,  iDrawHeight);

                        }

                    }
                    bFin = true;
                break;
                }

                if(pEffect->effect_value != pEffect->end_value)
                {
                    //pEffect->time += m_pEngine->m_iFrameInterval;
                    pEffect->time += m_pEngine->m_iCurrentInterval;
                    if (pEffect->time >= pEffect->interval)
                    {
                        if(pEffect->step_value > 0)
                        {
                            if(pEffect->effect_value < pEffect->end_value)
                            pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                            if(pEffect->effect_value > pEffect->end_value)
                            pEffect->effect_value = pEffect->end_value;
                        }
                        else if(pEffect->step_value < 0)
                        {
                            if(pEffect->effect_value > pEffect->end_value)
                            pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                            if(pEffect->effect_value < pEffect->end_value)
                            pEffect->effect_value = pEffect->end_value;
                        }

                        pEffect->time = 0;
                    }
                }
                else
                {
                    if(pEffect->repeat_times != 0)
                    {
                        pEffect->effect_value = pEffect->start_value;
                        if(pEffect->repeat_times > 0) pEffect->repeat_times = pEffect->repeat_times - 1;
                        //else pEffect->repeat_times = 0;
                    }
                    else pEffect->active = false;
                }

                it++;
            }


            if(iSrcColorKey != -1)
            {
                pClipboardA->SetColorKey(-1);
                pClipboardB->SetColorKey(-1);
            }

            return;
        }

        return;
    }

    switch (m_iEffectMode)
    {
    case Effect_M_None:    // no effect ...
    case Effect_M_Sprite:  // has effects but now no one is active ...
        if(m_pSprite && m_pSprite->m_pParent && m_pSprite->m_bIsRelative && !m_pSprite->m_bMouseDrag)
        {
            SDL_Rect rcSrc = m_FrameRect;
            SDL_Rect rcDst = {0, 0,
                              m_pSprite->m_pParent->m_DrawPosRect.right - m_pSprite->m_pParent->m_DrawPosRect.left,
                              m_pSprite->m_pParent->m_DrawPosRect.bottom - m_pSprite->m_pParent->m_DrawPosRect.top};

            int iRelX = m_pSprite->m_iRelativeX;
            int iRelY = m_pSprite->m_iRelativeY;
            if(m_pSprite->m_pParent->m_pCurrentAnima && m_pSprite->m_pParent->m_bDefaultDraw)
            {
                iRelX = m_pSprite->m_pParent->m_pCurrentAnima->m_iRootX + iRelX;
                iRelY = m_pSprite->m_pParent->m_pCurrentAnima->m_iRootY + iRelY;
            }
            else
            {
                iRelX = m_pSprite->m_pParent->m_iDefaultRootX + iRelX;
                iRelY = m_pSprite->m_pParent->m_iDefaultRootY + iRelY;
            }

            if(GetValidRect(iRelX, iRelY, rcSrc, rcDst))
            pMainScreen->Draw(m_pImage, m_pSprite->m_pParent->m_DrawPosRect.left+rcDst.x,
                                        m_pSprite->m_pParent->m_DrawPosRect.top+rcDst.y,
                                        rcSrc.x,  rcSrc.y,  rcSrc.w,  rcSrc.h);

        }
        else
        pMainScreen->Draw(m_pImage, iPosX,  iPosY,
                                    m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
    break;
    case Effect_M_Frame:
    {
        int iFrameEffectCount = 0;
        CogeFrameEffect* pFrameEffect = NULL;

        if(m_iCurrentFrame > m_iTotalFrames) pFrameEffect = GetFrameEffect(m_iCurrentFrame-1);
        else pFrameEffect = GetFrameEffect(m_iCurrentFrame);

        //if(pFrameEffect) iFrameEffectCount = pFrameEffect->GetEffectCount();
        if(pFrameEffect) iFrameEffectCount = pFrameEffect->GetActiveEffectCount();

        if(pFrameEffect && iFrameEffectCount > 0)
        {
            if(m_pImage->HasLocalClipboard())
                m_pImage->PrepareLocalClipboards(iFrameEffectCount);

            if(iFrameEffectCount == 1)
            {
                //ogeEffectMap::iterator it;
                //it = pFrameEffect->m_Effects.begin();

                //ogeEffectMap Effects = pFrameEffect->GetEffects();
                //ogeEffectMap::iterator it;

                //it = Effects.begin();

                //CogeEffect* pEffect = it->second;

                CogeEffect* pEffect = pFrameEffect->GetFirstActiveEffect();

                int iEffectValue = (int)pEffect->effect_value;

                switch(pEffect->effect_type)
                {
                case Effect_Edge:
                    pMainScreen->BltWithEdge( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;

                case Effect_RGB:
                    pMainScreen->BltChangedRGB( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;

                case Effect_Color:
                {
                    uint32_t iRealColor =  iEffectValue & 0x00ffffff;
                    uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
                    pMainScreen->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                }
                break;

                case Effect_Rota:
                    pMainScreen->BltRotate( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;

                case Effect_Wave:
                    pMainScreen->BltWave( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;

                case Effect_Lightness:
                    pMainScreen->BltWithLightness( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;

                case Effect_Scale:
                {
                    iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                    iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);
                    int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                    int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);
                    pMainScreen->BltStretch( m_pImage,
                    m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                    iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
                }
                break;

                case Effect_Alpha:
                    pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                    iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                break;
                }

                if(pEffect->effect_value != pEffect->end_value)
                {
                    //pEffect->time += m_pEngine->m_iFrameInterval;
                    pEffect->time += m_pEngine->m_iCurrentInterval;
                    if (pEffect->time >= pEffect->interval)
                    {
                        if(pEffect->step_value > 0)
                        {
                            if(pEffect->effect_value < pEffect->end_value)
                            pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                            if(pEffect->effect_value > pEffect->end_value)
                            pEffect->effect_value = pEffect->end_value;
                        }
                        else if(pEffect->step_value < 0)
                        {
                            if(pEffect->effect_value > pEffect->end_value)
                            pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                            if(pEffect->effect_value < pEffect->end_value)
                            pEffect->effect_value = pEffect->end_value;
                        }

                        pEffect->time = 0;
                    }
                }
                else
                {
                    if(pEffect->repeat_times != 0)
                    {
                        pEffect->effect_value = pEffect->start_value;
                        if(pEffect->repeat_times > 0) pEffect->repeat_times = pEffect->repeat_times - 1;
                        //else pEffect->repeat_times = 0;
                    }
                    else pEffect->active = false;
                }

            }

            if(iFrameEffectCount > 1 && pClipboardA && pClipboardB)
            {
                CogeImage* pCurrentClipboard = NULL;
                CogeImage* pFreeClipboard = NULL;
                int iHandled = 0;

                int iSrcColorKey = m_pImage->GetColorKey();

                if(iSrcColorKey != -1)
                {
                    pClipboardA->SetColorKey(iSrcColorKey);
                    pClipboardB->SetColorKey(iSrcColorKey);
                }

                CogeEffect* pEffect = NULL;

                bool bFin = false;

                //ogeEffectMap::iterator it;
                //it = pFrameEffect->m_Effects.begin();

                ogeEffectList Effects = pFrameEffect->GetEffects();
                ogeEffectList::iterator it;

                it = Effects.begin();

                while (it != Effects.end() && !bFin)
                {
                    //pEffect = it->second;
                    pEffect = *it;

                    if(pEffect->active == false)
                    {
                        it++;
                        continue;
                    }

                    int iEffectValue = (int)pEffect->effect_value;

                    switch(pEffect->effect_type)
                    {
                    case Effect_Edge:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithEdge( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltWithEdge( m_pImage, iEffectValue,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithEdge( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltWithEdge( pCurrentClipboard, iEffectValue,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;

                            }
                        }
                    break;

                    case Effect_RGB:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltChangedRGB( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltChangedRGB( m_pImage, iEffectValue,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltChangedRGB( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltChangedRGB( pCurrentClipboard, iEffectValue,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;
                            }
                        }
                    break;

                    case Effect_Color:
                    {
                        iHandled++;
                        uint32_t iRealColor =  iEffectValue & 0x00ffffff;
                        uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltWithColor( m_pImage, iRealColor, iRealAlpha,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithColor( pCurrentClipboard, iRealColor, iRealAlpha,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltWithColor( pCurrentClipboard, iRealColor, iRealAlpha,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;
                            }
                        }
                    }
                    break;

                    case Effect_Rota:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount || m_pImage->HasLocalClipboard())
                            {
                                pMainScreen->BltRotate( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltRotate( m_pImage, iEffectValue,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                            }


                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltRotate( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltRotate( pCurrentClipboard, iEffectValue,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;

                            }
                        }

                    break;


                    case Effect_Wave:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWave( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltWave( m_pImage, iEffectValue,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                            }


                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWave( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltWave( pCurrentClipboard, iEffectValue,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;

                            }
                        }

                    break;

                    case Effect_Lightness:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithLightness( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pCurrentClipboard->BltWithLightness( m_pImage, iEffectValue,
                                0,  0, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                            }


                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iHandled == iFrameEffectCount)
                            {
                                pMainScreen->BltWithLightness( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, m_FrameRect.w, m_FrameRect.h);
                                pFreeClipboard->BltWithLightness( pCurrentClipboard, iEffectValue,
                                0,  0, 0,  0,  m_FrameRect.w,  m_FrameRect.h);

                                pCurrentClipboard = pFreeClipboard;

                            }
                        }

                    break;

                    case Effect_Scale:
                        iHandled++;
                        if(pCurrentClipboard == NULL)
                        {
                            iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                            iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);

                            if(iHandled == iFrameEffectCount || m_pImage->HasLocalClipboard())
                            {
                                int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                                int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                                pMainScreen->BltStretch( m_pImage,
                                 m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                                iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
                            }
                            else
                            {
                                pCurrentClipboard = pClipboardA;
                                if(iSrcColorKey != -1)
                                pCurrentClipboard->FillRect(iSrcColorKey, 0, 0, iDrawWidth, iDrawHeight);
                                pCurrentClipboard->BltStretch( m_pImage,
                                 m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.x+m_FrameRect.w,  m_FrameRect.y+m_FrameRect.h,
                                0,  0, iDrawWidth,  iDrawHeight);

                            }

                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            iDrawWidth  = lround(m_FrameRect.w * pEffect->effect_value);
                            iDrawHeight = lround(m_FrameRect.h * pEffect->effect_value);

                            if(iHandled == iFrameEffectCount)
                            {
                                int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                                int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                                pMainScreen->BltStretch( pCurrentClipboard,
                                 0,  0,  m_FrameRect.w,  m_FrameRect.h,
                                iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
                            }
                            else
                            {
                                if(iSrcColorKey != -1)
                                pFreeClipboard->FillRect(iSrcColorKey, 0, 0, iDrawWidth, iDrawHeight);

                                pFreeClipboard->BltStretch( pCurrentClipboard,
                                0,  0, m_FrameRect.w, m_FrameRect.h,
                                0,  0, iDrawWidth, iDrawHeight);

                                pCurrentClipboard = pFreeClipboard;

                            }
                        }
                        //bFin = true;
                    break;

                    case Effect_Alpha:
                        iHandled++;
                        if(pCurrentClipboard == NULL || m_pImage->HasLocalClipboard())
                        {
                            //pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                            //iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);

                            if(iDrawWidth < 0)
                            {
                                pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                                iPosX,  iPosY, m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                                int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                                pMainScreen->BltAlphaBlend( m_pImage, iEffectValue,
                                iDrawX,  iDrawY, 0,  0,  iDrawWidth,  iDrawHeight);

                            }
                        }
                        else
                        {
                            if(pCurrentClipboard == pClipboardA) pFreeClipboard = pClipboardB;
                            else pFreeClipboard = pClipboardA;

                            if(iDrawWidth < 0)
                            {
                                pMainScreen->BltAlphaBlend( pCurrentClipboard, iEffectValue,
                                iPosX,  iPosY, 0,  0,  m_FrameRect.w,  m_FrameRect.h);
                            }
                            else
                            {
                                int iDrawX  = iPosX + (m_FrameRect.w >> 1) - (iDrawWidth  >> 1);
                                int iDrawY  = iPosY + (m_FrameRect.h >> 1) - (iDrawHeight >> 1);

                                pMainScreen->BltAlphaBlend( pCurrentClipboard, iEffectValue,
                                iDrawX,  iDrawY, 0,  0,  iDrawWidth,  iDrawHeight);

                            }
                        }
                        bFin = true;
                    break;
                    }

                    if(pEffect->effect_value != pEffect->end_value)
                    {
                        //pEffect->time += m_pEngine->m_iFrameInterval;
                        pEffect->time += m_pEngine->m_iCurrentInterval;
                        if (pEffect->time >= pEffect->interval)
                        {

                            if(pEffect->step_value > 0)
                            {
                                if(pEffect->effect_value < pEffect->end_value)
                                pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                                if(pEffect->effect_value > pEffect->end_value)
                                pEffect->effect_value = pEffect->end_value;
                            }
                            else if(pEffect->step_value < 0)
                            {
                                if(pEffect->effect_value > pEffect->end_value)
                                pEffect->effect_value = pEffect->effect_value + pEffect->step_value;

                                if(pEffect->effect_value < pEffect->end_value)
                                pEffect->effect_value = pEffect->end_value;
                            }

                            pEffect->time = 0;
                        }
                    }
                    else
                    {
                        if(pEffect->repeat_times != 0)
                        {
                            pEffect->effect_value = pEffect->start_value;
                            if(pEffect->repeat_times > 0) pEffect->repeat_times = pEffect->repeat_times - 1;
                            //else pEffect->repeat_times = 0;
                        }
                        else pEffect->active = false;
                    }

                    it++;
                }


                if(iSrcColorKey != -1)
                {
                    pClipboardA->SetColorKey(-1);
                    pClipboardB->SetColorKey(-1);
                }

                return;

            }

            return;
        }
        else // if no active effect for current frame ...
        {
            if(m_pSprite && m_pSprite->m_pParent && m_pSprite->m_bIsRelative && !m_pSprite->m_bMouseDrag)
            {
                SDL_Rect rcSrc = m_FrameRect;
                SDL_Rect rcDst = {0, 0,
                                  m_pSprite->m_pParent->m_DrawPosRect.right - m_pSprite->m_pParent->m_DrawPosRect.left,
                                  m_pSprite->m_pParent->m_DrawPosRect.bottom - m_pSprite->m_pParent->m_DrawPosRect.top};

                int iRelX = m_pSprite->m_iRelativeX;
                int iRelY = m_pSprite->m_iRelativeY;
                if(m_pSprite->m_pParent->m_pCurrentAnima && m_pSprite->m_pParent->m_bDefaultDraw)
                {
                    iRelX = m_pSprite->m_pParent->m_pCurrentAnima->m_iRootX + iRelX;
                    iRelY = m_pSprite->m_pParent->m_pCurrentAnima->m_iRootY + iRelY;
                }
                else
                {
                    iRelX = m_pSprite->m_pParent->m_iDefaultRootX + iRelX;
                    iRelY = m_pSprite->m_pParent->m_iDefaultRootY + iRelY;
                }

                if(GetValidRect(iRelX, iRelY, rcSrc, rcDst))
                pMainScreen->Draw(m_pImage, m_pSprite->m_pParent->m_DrawPosRect.left+rcDst.x,
                                            m_pSprite->m_pParent->m_DrawPosRect.top+rcDst.y,
                                            rcSrc.x,  rcSrc.y,  rcSrc.w,  rcSrc.h);

            }
            else
            pMainScreen->Draw(m_pImage, iPosX,  iPosY,
                                        m_FrameRect.x,  m_FrameRect.y,  m_FrameRect.w,  m_FrameRect.h);
        }


    }
    break;
    }

}

int CogeAnima::Update()
{
    if(m_iState < 0) return -1;

    if (m_iCurrentFrame == 0)
    {
        m_iCurrentFrame = 1;
        m_FrameRect.x = m_Local.x;
        m_FrameRect.y = m_Local.y;

        //if(m_pImage) m_pImage->UpdateRect(m_FrameRect.x, m_FrameRect.y, m_FrameRect.w, m_FrameRect.h);
    }
    else
    {
        if(m_pSprite && !m_pSprite->m_bEnableAnima) return m_iCurrentFrame;

        if (m_iCurrentFrame > m_iTotalFrames)
        {
            if (m_bAutoReplay)
            {
                //m_iPlayTime += m_pEngine->m_iFrameInterval;
                m_iPlayTime += m_pEngine->m_iCurrentInterval;
                if (m_iPlayTime <= m_iReplayInterval) return -1;
                else
                {
                    m_iPlayTime = 0;
                    m_iCurrentFrame = 1;
                    m_FrameRect.x = m_Local.x;
                    m_FrameRect.y = m_Local.y;
                    m_bFinished = false;
                }
            }
            else
            {
                m_bFinished = true;
                //if(m_pSprite) m_pSprite->AnimaFin();
            }
        }
        else
        {
            //m_iPlayTime += m_pEngine->m_iFrameInterval;
            m_iPlayTime += m_pEngine->m_iCurrentInterval;
            if (m_iPlayTime <= m_iFrameInterval) return -1;
            else m_iPlayTime = 0;
            m_iCurrentFrame++;

            if(m_iCurrentFrame <= m_iTotalFrames)
            {
                m_FrameRect.x = m_FrameRect.x + m_iFrameWidth;
                if(m_FrameRect.x + m_iFrameWidth > m_iLocalRight)
                {
                    m_FrameRect.x = m_Local.x;
                    m_FrameRect.y = m_FrameRect.y + m_iFrameHeight;
                }
                if(m_FrameRect.y > m_iLocalBottom) m_FrameRect.y = m_Local.y;
            }
            else m_iCurrentFrame = m_iTotalFrames + 1;

        }
    }

    return m_iCurrentFrame;
}


/*---------------- Rander Effect Set -----------------*/

int CogeFrameEffect::AddEffect(int iEffectType, double fEffectValue)
{
    //CogeEffect* pEffect = NULL;
    //ogeEffectMap::iterator it = m_Effects.find(iEffectType);
    //if(it == m_Effects.end())
    CogeEffect* pEffect = FindEffect(iEffectType);
    if(pEffect == NULL)
    {
        pEffect = new CogeEffect();
        pEffect->active = true;
        pEffect->effect_type = iEffectType;
        pEffect->effect_value = fEffectValue;
        pEffect->start_value = fEffectValue;
        pEffect->end_value = fEffectValue;
        pEffect->repeat_times = -1;
        pEffect->step_value = 0;
        pEffect->time = 0;
        pEffect->interval = 5;
        //m_Effects.insert(ogeEffectMap::value_type(iEffectType, pEffect));

        if(iEffectType != Effect_Alpha)
        {
            // find proper position ...
            ogeEffectList::iterator it = m_Effects.begin();
            ogeEffectList::iterator its = m_Effects.end();
            ogeEffectList::iterator ita = m_Effects.end();
            while(it != m_Effects.end())
            {
                if((*it)->effect_type == Effect_Scale) its = it;
                else if((*it)->effect_type == Effect_Alpha) ita = it;

                it++;
            }

            if(iEffectType == Effect_Scale)
            {
                if(ita != m_Effects.end())
                {
                    m_Effects.insert(ita, pEffect);
                    return m_Effects.size();
                }
            }
            else
            {
                if(its != m_Effects.end())
                {
                    m_Effects.insert(its, pEffect);
                    return m_Effects.size();
                }
                else if(ita != m_Effects.end())
                {
                    m_Effects.insert(ita, pEffect);
                    return m_Effects.size();
                }
            }
        }

        m_Effects.push_back(pEffect);

    }
    else
    {
        //pEffect = it->second;
        pEffect->active = true;
        pEffect->effect_type = iEffectType;
        pEffect->effect_value = fEffectValue;
        pEffect->start_value = fEffectValue;
        pEffect->end_value = fEffectValue;
        pEffect->repeat_times = -1;
        pEffect->step_value = 0;
        pEffect->time = 0;
        pEffect->interval = 5;

    }

    return m_Effects.size();

}

int CogeFrameEffect::AddEffectEx(int iEffectType, double fStart, double fEnd, double fIncrement, int iStepInterval, int iRepeat)
{
    //CogeEffect* pEffect = NULL;
    //ogeEffectMap::iterator it = m_Effects.find(iEffectType);

    if(fStart > fEnd)
    {
        if(fIncrement > 0) fIncrement = 0 - fIncrement;
    }
    if(fStart < fEnd)
    {
        if(fIncrement < 0) fIncrement = 0 - fIncrement;
    }

    CogeEffect* pEffect = FindEffect(iEffectType);
    if(pEffect == NULL)
    {
        pEffect = new CogeEffect();
        pEffect->active = true;
        pEffect->time = 0;
        pEffect->interval = iStepInterval;
        pEffect->effect_type = iEffectType;
        pEffect->effect_value = fStart;
        pEffect->start_value = fStart;
        pEffect->end_value = fEnd;
        pEffect->repeat_times = iRepeat;
        pEffect->step_value = fIncrement;
        //m_Effects.insert(ogeEffectMap::value_type(iEffectType, pEffect));

        if(iEffectType != Effect_Alpha)
        {
            // find proper position ...
            ogeEffectList::iterator it = m_Effects.begin();
            ogeEffectList::iterator its = m_Effects.end();
            ogeEffectList::iterator ita = m_Effects.end();
            while(it != m_Effects.end())
            {
                if((*it)->effect_type == Effect_Scale) its = it;
                else if((*it)->effect_type == Effect_Alpha) ita = it;

                it++;
            }

            if(iEffectType == Effect_Scale)
            {
                if(ita != m_Effects.end())
                {
                    m_Effects.insert(ita, pEffect);
                    return m_Effects.size();
                }
            }
            else
            {
                if(its != m_Effects.end())
                {
                    m_Effects.insert(its, pEffect);
                    return m_Effects.size();
                }
                else if(ita != m_Effects.end())
                {
                    m_Effects.insert(ita, pEffect);
                    return m_Effects.size();
                }
            }
        }

        m_Effects.push_back(pEffect);

    }
    else
    {
        //pEffect = it->second;
        pEffect->active = true;
        pEffect->time = 0;
        pEffect->interval = iStepInterval;
        pEffect->effect_type = iEffectType;
        pEffect->effect_value = fStart;
        pEffect->start_value = fStart;
        pEffect->end_value = fEnd;
        pEffect->repeat_times = iRepeat;
        pEffect->step_value = fIncrement;

    }

    return m_Effects.size();
}

int CogeFrameEffect::DelEffect(int iEffectType)
{
    //CogeEffect* pEffect = NULL;
    //ogeEffectMap::iterator it = m_Effects.find(iEffectType);
    //if(it == m_Effects.end()) return -1;
    //else m_Effects.erase(it);
    //return m_Effects.size();

    CogeEffect* pEffect = NULL;
    ogeEffectList::iterator it = m_Effects.begin();
    while(it != m_Effects.end())
    {
        pEffect = *it;
        if(pEffect->effect_type == iEffectType)
        {
            m_Effects.erase(it);
            delete pEffect;
            return m_Effects.size();
        }

        it++;
    }

    return -1;

}

int CogeFrameEffect::DisableEffect(int iEffectType)
{
    //ogeEffectMap::iterator it = m_Effects.find(iEffectType);
    //if(it == m_Effects.end()) return -1;
    //else it->second->active = false;
    //return m_Effects.size();

    CogeEffect* pEffect = FindEffect(iEffectType);
    if(pEffect)
    {
        pEffect->active = false;
        return m_Effects.size();
    }
    else return -1;
}

CogeEffect* CogeFrameEffect::FindEffect(int iEffectType)
{
    //ogeEffectMap::iterator it = m_Effects.find(iEffectType);
    //if(it == m_Effects.end()) return NULL;
    //else return it->second;

    CogeEffect* pEffect = NULL;
    for(size_t i=0; i<m_Effects.size(); i++)
    {
        pEffect = m_Effects[i];
        if(pEffect->effect_type == iEffectType) return pEffect;
    }

    return NULL;
}

const ogeEffectList& CogeFrameEffect::GetEffects() const
{
    return m_Effects;
}

int CogeFrameEffect::GetFrameId()
{
    return m_iFrameId;
}
void CogeFrameEffect::SetFrameId(int value)
{
    m_iFrameId = value;
}

int CogeFrameEffect::GetEffectCount()
{
    return m_Effects.size();
}

int CogeFrameEffect::GetActiveEffectCount()
{
    int iTotal = 0;
    //ogeEffectMap::iterator it = m_Effects.begin();
    ogeEffectList::iterator it = m_Effects.begin();
    while (it != m_Effects.end())
    {
        if((*it)->active) iTotal++;
        it++;
    }
    return iTotal;
}
CogeEffect* CogeFrameEffect::GetFirstActiveEffect()
{
    CogeEffect* pMatchedEffect = NULL;
    //ogeEffectMap::iterator it = m_Effects.begin();
    ogeEffectList::iterator it = m_Effects.begin();
    while (it != m_Effects.end())
    {
        pMatchedEffect = *it;
        if(pMatchedEffect->active) return pMatchedEffect;
        it++;
    }
    return pMatchedEffect;
}

void CogeFrameEffect::Clear()
{
    ogeEffectList::iterator it;
    CogeEffect* pMatchedEffect = NULL;

    it = m_Effects.begin();

    while (it != m_Effects.end())
    {
        pMatchedEffect = *it;
        m_Effects.erase(it);
        delete pMatchedEffect;
        it = m_Effects.begin();
    }
}

CogeFrameEffect::CogeFrameEffect()
{
    m_iFrameId = 0;
}

CogeFrameEffect::~CogeFrameEffect()
{
    Clear();
}


/*----------------------------------- Custom Event Set --------------------------------------*/

CogeCustomEventSet::CogeCustomEventSet(const std::string& sName, CogeScript* pCommonScript, CogeScript* pLocalScript):
m_pCommonScript(pCommonScript),
m_pLocalScript(pLocalScript),
m_pCurrentScript(NULL),
m_pPlotSpr(NULL),
m_sName(sName),
m_iCurrentEventCode(0),
m_pTrigger(NULL)
{
    memset((void*)&m_CommonEvents[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);
    memset((void*)&m_LocalEvents[0],  -1, sizeof(int) * _OGE_MAX_EVENT_COUNT_);
    m_PendingEvents.clear();
}
CogeCustomEventSet::~CogeCustomEventSet()
{
    m_PendingEvents.clear();
    m_pCommonScript = NULL;
    m_pLocalScript = NULL;
}

void CogeCustomEventSet::SetCommonEvent(int iCustomEventCode, int iScriptEntryId)
{
    m_CommonEvents[iCustomEventCode] = iScriptEntryId;
}

void CogeCustomEventSet::SetLocalEvent(int iCustomEventCode, int iScriptEntryId)
{
    m_LocalEvents[iCustomEventCode] = iScriptEntryId;
}

void CogeCustomEventSet::SetTrigger(int* pTrigger)
{
    m_pTrigger = pTrigger;
}
void CogeCustomEventSet::SetPlotSpr(CogeSprite* pPlotSpr)
{
    m_pPlotSpr = pPlotSpr;
}

void CogeCustomEventSet::CallEvent(int iCustomEventCode)
{
    m_pCurrentScript = NULL;
    m_iCurrentEventCode = -1;

    if(m_pLocalScript)
    {
        int iEventId = m_LocalEvents[iCustomEventCode];
        if(iEventId >= 0)
        {
            m_pCurrentScript = m_pLocalScript;
            m_iCurrentEventCode = iCustomEventCode;

            m_pLocalScript->CallFunction(iEventId);
            //m_pLocalScript->CallGC();
        }
    }

    if(m_pCommonScript && !m_pCurrentScript)
    {
        int iEventId = m_CommonEvents[iCustomEventCode];
        if(iEventId >= 0)
        {
            m_pCommonScript->CallFunction(iEventId);
            //m_pCommonScript->CallGC();
        }
    }

    if(m_pPlotSpr && m_pTrigger)
    {
        int* pTriggers = m_pTrigger;

        if(pTriggers[iCustomEventCode] > 0)
        {
            pTriggers[iCustomEventCode] = 0;
            m_pPlotSpr->ResumeUpdateScript();
        }
    }

}

void CogeCustomEventSet::CallBaseEvent()
{
    if(m_pCurrentScript && m_pCurrentScript == m_pLocalScript)
    {
        if(m_pCommonScript && m_iCurrentEventCode >= 0)
        {
            int iEventId = m_CommonEvents[m_iCurrentEventCode];
            if(iEventId >= 0)
            {
                m_pCommonScript->CallFunction(iEventId);
            }
        }
    }
}

void CogeCustomEventSet::CallCustomEvents()
{
    for(size_t i=0; i<m_PendingEvents.size(); i++)
    {
        CallEvent(m_PendingEvents[i]);
    }
    m_PendingEvents.clear();
}

void CogeCustomEventSet::PushEvent(int iCustomEventCode)
{
    if(iCustomEventCode >= _OGE_MAX_EVENT_COUNT_) return;
    m_PendingEvents.push_back(iCustomEventCode);
}

int CogeCustomEventSet::GetCustomEventCount()
{
    int total = 0;
    for(int i=0; i<_OGE_MAX_EVENT_COUNT_; i++)
    {
        if(m_CommonEvents[i] >= 0 || m_LocalEvents[i] >= 0) total++;
    }
    return total;
}

int CogeCustomEventSet::GetPendingEventCount()
{
    return m_PendingEvents.size();
}

void CogeCustomEventSet::ClearPendingEvents()
{
    m_PendingEvents.clear();
}

/*---------------------------------- Path -------------------------------------*/

void CogePath::ClearKeys()
{
    ogePoints::iterator it;
    CogePoint* p = NULL;

    it = m_Keys.begin();

    while (it != m_Keys.end())
    {
        p = *it;
        m_Keys.erase(it);
        delete p;
        it = m_Keys.begin();
    }
}
void CogePath::ClearPoints()
{
    ogePoints::iterator it;
    CogePoint* p = NULL;

    it = m_Points.begin();

    while (it != m_Points.end())
    {
        p = *it;
        m_Points.erase(it);
        delete p;
        it = m_Points.begin();
    }
}

CogePath::CogePath()
{
    //m_iDir = 0;
    //m_iStep = 0;

    m_iType = 0;
    m_iSegmentLength = 0;

    //m_iStepInterval = 0;
    //m_iStepTime = 0;
    //m_iValidStepCount = 0;
}

CogePath::~CogePath()
{
    ClearKeys();
    ClearPoints();
}

void CogePath::AddLine(int x1, int y1, int x2, int y2)
{
    int i,deltax,deltay,numpixels,d,dinc1,dinc2,x,xinc1,xinc2,y,yinc1,yinc2,x0,y0;

    //if (iMaxX < 0 || iMaxY < 0) return;
	//if (std::max(x1,x2) < 0 || std::max(y1,y2) < 0) return;

	x0 = std::max(std::min(x1,x2), 0);
	y0 = std::max(std::min(y1,y2), 0);

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	// Initialize all vars based on which is the independent variable
	if (deltax >= deltay)
	{

		// x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		// y is independent variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	// Make sure x and y move in the right directions
	if (x1 > x2)
	{
		xinc1 = 0 - xinc1;
		xinc2 = 0 - xinc2;
	}
	if (y1 > y2)
	{
		yinc1 = 0 - yinc1;
		yinc2 = 0 - yinc2;
	}

	// Start drawing at
	x = x1;
	y = y1;

	//for (i = 1; i <= numpixels; i++)
	for (i = 1; i < numpixels; i++)
    {
        //SAFE_SET_32_PX(x, y);
        //*(uint32_t*)(pDstData + (y-y0) * iDstLineSize + 4*(x-x0)) = (uint32_t)iColor;

        CogePoint* pPoint = new CogePoint();
        pPoint->x = x;
        pPoint->y = y;

        m_Points.push_back(pPoint);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }

}

void CogePath::GenShortLine(int x1, int y1, int x2, int y2)
{
    ClearKeys();
    ClearPoints();

    int i,deltax,deltay,numpixels,d,dinc1,dinc2,x,xinc1,xinc2,y,yinc1,yinc2,x0,y0;

    //if (iMaxX < 0 || iMaxY < 0) return;
	//if (std::max(x1,x2) < 0 || std::max(y1,y2) < 0) return;

	int rootx = x1;
	int rooty = y1;

	x0 = std::max(std::min(x1,x2), 0);
	y0 = std::max(std::min(y1,y2), 0);

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	// Initialize all vars based on which is the independent variable
	if (deltax >= deltay)
	{

		// x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		// y is independent variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	// Make sure x and y move in the right directions
	if (x1 > x2)
	{
		xinc1 = 0 - xinc1;
		xinc2 = 0 - xinc2;
	}
	if (y1 > y2)
	{
		yinc1 = 0 - yinc1;
		yinc2 = 0 - yinc2;
	}

	// Start drawing at
	x = x1;
	y = y1;

	//iStepCount++; // start point is step 0 ...

	for (i = 1; i < numpixels; i++)
    {
        //SAFE_SET_32_PX(x, y);
        //*(uint32_t*)(pDstData + (y-y0) * iDstLineSize + 4*(x-x0)) = (uint32_t)iColor;

        CogePoint* pPoint = new CogePoint();
        pPoint->x = x - rootx;
        pPoint->y = y - rooty;

        m_Points.push_back(pPoint);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }

    // add last point ...
    if(m_Points.size() > 0)
    {
        CogePoint* pPoint = new CogePoint();
        pPoint->x = x2 - rootx;
        pPoint->y = y2 - rooty;
        m_Points.push_back(pPoint);
    }

    // update keys ...
    if(m_Points.size() > 0)
    {
        CogePoint* pPoint = new CogePoint();
        pPoint->x = m_Points[0]->x;
        pPoint->y = m_Points[0]->y;
        m_Keys.push_back(pPoint);

        //printf("\n first point: %d, %d \n", pPoint->x, pPoint->y);
    }

    if(m_Points.size() > 1)
    {
        int iLastIdx = m_Points.size() - 1;
        CogePoint* pPoint = new CogePoint();
        pPoint->x = m_Points[iLastIdx]->x;
        pPoint->y = m_Points[iLastIdx]->y;
        m_Keys.push_back(pPoint);

        //printf("\n last point: %d, %d \n", pPoint->x, pPoint->y);
    }

    //m_iValidStepCount = m_Points.size();

}

int CogePath::GetTotalStepCount()
{
    return m_Points.size();
}

/*
int CogePath::GetValidStepCount()
{
    return m_iValidStepCount;
}
void CogePath::SetValidStepCount(int iCount)
{
    if(iCount >= 0) m_iValidStepCount = iCount;
    else m_iValidStepCount = m_Points.size();
}

int CogePath::GetStepInterval()
{
    return m_iStepInterval;
}
void CogePath::SetStepInterval(int iInterval)
{
    if(iInterval >= 0) m_iStepInterval = iInterval;
}
*/

void CogePath::ParseLineRoad()
{
    ClearPoints();
    CogePoint* pPoint = NULL;
    int x1=-1; int y1=-1; int x2=-1; int y2=-1;
    int iCount = m_Keys.size();
    if(iCount <= 0) return;
    for(int i=0; i<iCount; i++)
    {
        pPoint = m_Keys[i];
        x2 = pPoint->x; y2 = pPoint->y;
        //if(x1>=0 && y1>=0 && x2>=0 && y2>=0)
        if(i>0) AddLine(x1, y1, x2, y2);
        x1=x2; y1=y2;
    }

    // add last point ...
    //if(x1>=0 && y1>=0 && x2>=0 && y2>=0)
    //if(iCount > 0)
    //{
        pPoint = new CogePoint();
        pPoint->x = x2;
        pPoint->y = y2;

        m_Points.push_back(pPoint);
    //}

    //m_iValidStepCount = m_Points.size();
}

void CogePath::ParseBezierRoad()
{
    ClearPoints();
    CogePoint* pPoint = NULL;

    double t0,t1,t2,t3,t4;
    int n, iCount;

    iCount = (m_Keys.size()-1) / 3;

    if(iCount > 0)
    {
        for(int idx=0; idx<iCount; idx++)
        {
            for(int i=1; i<=m_iSegmentLength; i++)
            {
                t0 = (i+0.00)/m_iSegmentLength;
                t1 = 1-t0;
                t2 = t1 * t1;
                t3 = t0 * t0 * t0;
                t4 = t1*t1*t1;
                n  = idx*3;

                pPoint = new CogePoint();
                pPoint->x = lround(t4 * m_Keys[n]->x +
                            3  * t0 * t2 * m_Keys[n+1]->x +
                            3  * t0 * t0 * t1 * m_Keys[n+2]->x +
                            t3 * m_Keys[n+3]->x);
                pPoint->y = lround(t4 * m_Keys[n]->y +
                            3  * t0 * t2 * m_Keys[n+1]->y +
                            3  * t0 * t0 * t1 * m_Keys[n+2]->y +
                            t3 * m_Keys[n+3]->y);

                m_Points.push_back(pPoint);

                //printf("x%d: %d, y%d: %d\n", i, pPoint->x, i, pPoint->y);
            }
        }
    }

    //m_iValidStepCount = m_Points.size();

}

/*------------------- Data ---------------------*/

CogeGameData::CogeGameData(const std::string& sName):
m_iIntVarCount(0),
m_iFloatVarCount(0),
m_iStrVarCount(0),
m_iBufVarCount(0)
{
    m_sName = sName;
    //m_iTotalUsers = 0;
}
CogeGameData::~CogeGameData()
{
    //ints.clear();
    //floats.clear();
    //strs.clear();

    //bufs.clear();

    Clear(true);
}

const std::string& CogeGameData::GetName()
{
    return m_sName;
}

void CogeGameData::SetName(const std::string& sName)
{
    m_sName = sName;
}

/*
void CogeGameData::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}
void CogeGameData::Fire()
{
    m_iTotalUsers--;
    if(m_iTotalUsers < 0) m_iTotalUsers = 0;
}
*/

int CogeGameData::GetIntVarCount()
{
    return m_iIntVarCount;
}
int CogeGameData::GetFloatVarCount()
{
    return m_iFloatVarCount;
}
int CogeGameData::GetStrVarCount()
{
    return m_iStrVarCount;
}
int CogeGameData::GetBufVarCount()
{
    return m_iBufVarCount;
}

void CogeGameData::SetIntVarCount(int iCount)
{
    if(iCount <= 0)
    {
        ints.clear();
        m_iIntVarCount = 0;
    }
    else if(m_iIntVarCount != iCount)
    {
        ints.resize(iCount + 1);
        m_iIntVarCount = iCount;
    }
}
void CogeGameData::SetFloatVarCount(int iCount)
{
    if(iCount <= 0)
    {
        floats.clear();
        m_iFloatVarCount = 0;
    }
    else if(m_iFloatVarCount != iCount)
    {
        floats.resize(iCount + 1);
        m_iFloatVarCount = iCount;
    }
}
void CogeGameData::SetStrVarCount(int iCount)
{
    if(iCount <= 0)
    {
        strs.clear();
        m_iStrVarCount = 0;
    }
    else if(m_iStrVarCount != iCount)
    {
        strs.resize(iCount + 1);
        m_iStrVarCount = iCount;
    }
}
void CogeGameData::SetBufVarCount(int iCount)
{
    if(iCount <= 0)
    {
        bufs.clear();
        m_iBufVarCount = 0;
    }
    else if(m_iBufVarCount != iCount)
    {
        bufs.resize(iCount + 1);
        m_iBufVarCount = iCount;
    }
}

void CogeGameData::Clear(bool bIncludeBuffer)
{
    SetIntVarCount(0);
    SetFloatVarCount(0);
    SetStrVarCount(0);

    if(bIncludeBuffer) SetBufVarCount(0);

}

int CogeGameData::GetIntVar(int idx)
{
    if(idx <= m_iIntVarCount) return ints[idx];
    else return 0;
}
double CogeGameData::GetFloatVar(int idx)
{
    if(idx <= m_iFloatVarCount) return floats[idx];
    else return 0;
}
const std::string& CogeGameData::GetStrVar(int idx)
{
    if(idx <= m_iStrVarCount) return strs[idx];
    else return _OGE_EMPTY_STR_;
}
char* CogeGameData::GetBufVar(int idx)
{
    if(idx <= m_iBufVarCount) return &(bufs[idx].buf[0]);
    else return NULL;
}

void CogeGameData::SetIntVar(int idx, int iValue)
{
    if(idx <= m_iIntVarCount) ints[idx] = iValue;
}
void CogeGameData::SetFloatVar(int idx, double fValue)
{
    if(idx <= m_iFloatVarCount) floats[idx] = fValue;
}
void CogeGameData::SetStrVar(int idx, const std::string& sValue)
{
    if(idx <= m_iStrVarCount) strs[idx] = sValue;
}
void CogeGameData::SetBufVar(int idx, const std::string& sValue)
{
    if(idx <= m_iStrVarCount)
    {
        char* pChar = &(bufs[idx].buf[0]);
        int iLen = sValue.length();
        if(iLen > _OGE_MAX_TEXT_LEN_) iLen = _OGE_MAX_TEXT_LEN_;
        memcpy(pChar, sValue.c_str(), iLen);
    }
}

void CogeGameData::SortInt()
{
    if(ints.size() <= 1) return;
    sort(ints.begin()+1, ints.end());
}
void CogeGameData::SortFloat()
{
    if(floats.size() <= 1) return;
    sort(floats.begin()+1, floats.end());
}
void CogeGameData::SortStr()
{
    if(strs.size() <= 1) return;
    sort(strs.begin()+1, strs.end());
}

int CogeGameData::IndexOfInt(int value)
{
    int iCount = ints.size();
    for(int i=1; i<iCount; i++)
    {
        if(ints[i] == value) return i;
    }
    return -1;
}
int CogeGameData::IndexOfFloat(double value)
{
    int iCount = floats.size();
    for(int i=1; i<iCount; i++)
    {
        if(floats[i] == value) return i;
    }
    return -1;
}
int CogeGameData::IndexOfStr(const std::string& value)
{
    int iCount = strs.size();
    for(int i=1; i<iCount; i++)
    {
        if(strs[i] == value) return i;
    }
    return -1;
}

void CogeGameData::Copy(CogeGameData* pData)
{
    if(pData == NULL) return;

    SetIntVarCount(pData->GetIntVarCount());
    SetFloatVarCount(pData->GetFloatVarCount());
    SetStrVarCount(pData->GetStrVarCount());

    SetBufVarCount(pData->GetBufVarCount());


    for(size_t i = 0; i < pData->ints.size(); i++) ints[i] = pData->ints[i];
    for(size_t i = 0; i < pData->floats.size(); i++) floats[i] = pData->floats[i];
    for(size_t i = 0; i < pData->strs.size(); i++) strs[i] = pData->strs[i];


    for(size_t i = 0; i < pData->bufs.size(); i++)
    {
        char* pSrcChar = &(pData->bufs[i].buf[0]);
        char* pDstChar = &(bufs[i].buf[0]);
        memcpy(pSrcChar, pDstChar, _OGE_MAX_TEXT_LEN_);
    }


}


/*---------------- Path Finder -----------------*/

int CogePathFinder::SetSize(int iColumnCount, int iRowCount)
{
    if(m_iColumnCount == iColumnCount && m_iRowCount == iRowCount) return 0;
    int iTotal = iColumnCount * iRowCount;

    m_History.resize(iTotal);
    for(int i=0; i<iTotal; i++) m_History[i] = 0;

    m_iColumnCount = iColumnCount;
    m_iRowCount = iRowCount;

    return iTotal;
}

bool CogePathFinder::GetEightDirectionMode()
{
    return m_bIsEight;
}
void CogePathFinder::SetEightDirectionMode(bool value)
{
    m_bIsEight = value;
}

int CogePathFinder::Initialize(std::vector<int>* pTileValues, int iColumnCount, int iRowCount)
{
    m_pTileValues = pTileValues;
    SetSize(iColumnCount, iRowCount);
    m_iState = 0;
    m_bIsEight = false;
    return m_iState;
}

void CogePathFinder::Clear()
{
    std::vector<CogeStepNode*>::iterator its;
	CogeStepNode* pStep = NULL;

	its = m_Steps.begin();

	while (its != m_Steps.end())
	{
		pStep = *its;
		m_Steps.erase(its);
		if(pStep) delete pStep;
		its = m_Steps.begin();
	}

	std::vector<CogeTraceNode*>::iterator itt;
	CogeTraceNode* pTrace = NULL;

	itt = m_Traces.begin();

	while (itt != m_Traces.end())
	{
		pTrace = *itt;
		m_Traces.erase(itt);
		if(pTrace) delete pTrace;
		itt = m_Traces.begin();
	}

	m_pOpenQueue = NULL;
	m_pClosedQueue = NULL;

	int iSize = m_History.size();
	for(int i=0; i<iSize; i++) m_History[i] = 0;

	//m_Way.clear();
}

void CogePathFinder::Finalize()
{
    Clear();

	m_History.clear();
	//m_Way.clear();

	m_iState = -1;

    return;
}

CogePathFinder::CogePathFinder()
{
    m_iState = -1;

    m_iColumnCount = 0;
    m_iRowCount = 0;

    m_bIsEight = false;

    m_pOpenQueue = NULL;
    m_pClosedQueue = NULL;

    m_pTileValues = NULL;

    return;
}
CogePathFinder::~CogePathFinder()
{
    Finalize();
    return;
}

int CogePathFinder::PosToIndex(int iPosX, int iPosY)
{
    return m_iColumnCount * iPosY + iPosX;
}

int CogePathFinder::EstDist(int iStartX, int iStartY, int iEndX, int iEndY)
{
    return abs(iEndX - iStartX) + abs(iEndY - iStartY);
}
void CogePathFinder::InsertTrace(CogeTraceNode* pTraceNode)
{
    if(pTraceNode == NULL) return;

    CogeTraceNode* pPriorNode = NULL;
    CogeTraceNode* pCurrentNode = m_pOpenQueue;
    while(pCurrentNode != NULL
            && pTraceNode->lastidx + pTraceNode->dist > pCurrentNode->lastidx + pCurrentNode->dist)
    {
        pPriorNode = pCurrentNode;
        pCurrentNode = pCurrentNode->next;
    }
    if(pPriorNode) pPriorNode->next = pTraceNode;
    else m_pOpenQueue = pTraceNode;
    pTraceNode->next = pCurrentNode;

    return;
}
void CogePathFinder::ArchiveTrace(CogeTraceNode* pTraceNode)
{
    if(pTraceNode == NULL) return;

    CogeTraceNode* pPriorNode = NULL;
    CogeTraceNode* pCurrentNode = m_pClosedQueue;
    while(pCurrentNode != NULL)
    {
        bool bIsBetter = pTraceNode->dist > pCurrentNode->dist;
        if(!bIsBetter)
        {
            if(pTraceNode->dist == pCurrentNode->dist)
            bIsBetter = pTraceNode->lastidx > pCurrentNode->lastidx;
        }
        if(bIsBetter)
        {
            pPriorNode = pCurrentNode;
            pCurrentNode = pCurrentNode->next;
        }
        else break;
    }
    if(pPriorNode) pPriorNode->next = pTraceNode;
    else m_pClosedQueue = pTraceNode;
    pTraceNode->next = pCurrentNode;

    return;
}
CogeTraceNode* CogePathFinder::PopTrace()
{
    CogeTraceNode* pTopTrace = m_pOpenQueue;
    if(m_pOpenQueue != NULL) m_pOpenQueue = m_pOpenQueue->next;
    if(pTopTrace != NULL) pTopTrace->next = NULL;
    return pTopTrace;
}
CogeStepNode* CogePathFinder::NewStep(CogeStepNode* pPriorStep, int iPosX, int iPosY)
{
    CogeStepNode* pNewStep = new CogeStepNode();
    if(pPriorStep != NULL) pNewStep->index = pPriorStep->index + 1;
    else pNewStep->index = 1;
    pNewStep->posx = iPosX;
    pNewStep->posy = iPosY;
    pNewStep->prior = pPriorStep;
    m_Steps.push_back(pNewStep);
    return pNewStep;
}
CogeTraceNode* CogePathFinder::NewTrace(CogeStepNode* pStepNode, int iDist)
{
    CogeTraceNode* pNewTrace = new CogeTraceNode();
    if(pStepNode) pNewTrace->lastidx = pStepNode->index;
    else pNewTrace->lastidx = 0;
    pNewTrace->step = pStepNode;
    pNewTrace->dist = iDist;
    pNewTrace->next = NULL;
    m_Traces.push_back(pNewTrace);
    return pNewTrace;
}
bool CogePathFinder::TestStep(int iStartX, int iStartY, int iEndX, int iEndY, CogeStepNode* pPriorStep)
{
    if(iStartX < 0 || iStartY < 0) return false;
    if(iStartX >= m_iColumnCount || iStartY >= m_iRowCount) return false;
    int idx = PosToIndex(iStartX, iStartY);
    if(m_pTileValues == NULL || m_pTileValues->at(idx) < 0) return false;

    int iCurrentIdx = 1;
    if(pPriorStep) iCurrentIdx = pPriorStep->index + 1;
    if(m_History[idx] > 0 && iCurrentIdx >= m_History[idx]) return false;
    else m_History[idx] = iCurrentIdx;

    CogeStepNode* pNewStep = NewStep(pPriorStep, iStartX, iStartY);
    CogeTraceNode* pNewTrace = NewTrace(pNewStep, EstDist(iStartX, iStartY, iEndX, iEndY));

    InsertTrace(pNewTrace);

    return true;
}

CogeTraceNode* CogePathFinder::FindTrace(int iStartX, int iStartY, int iEndX, int iEndY)
{
    if(m_iState < 0) return NULL;
    if(!TestStep(iStartX, iStartY, iEndX, iEndY, NULL)) return NULL;

    CogeTraceNode* pBestTrace = PopTrace();
    while(pBestTrace)
    {
        CogeStepNode* pCurrentStep = pBestTrace->step;
        if(pCurrentStep)
        {
            int iCurrentX = pCurrentStep->posx;
            int iCurrentY = pCurrentStep->posy;
            if(iCurrentX == iEndX && iCurrentY == iEndY)
            {
                ArchiveTrace(pBestTrace);
                break;
            }

            bool bIsUpDirOK    = TestStep(iCurrentX,   iCurrentY-1, iEndX, iEndY, pCurrentStep);
            bool bIsRightDirOK = TestStep(iCurrentX+1, iCurrentY,   iEndX, iEndY, pCurrentStep);
            bool bIsDownDirOK  = TestStep(iCurrentX,   iCurrentY+1, iEndX, iEndY, pCurrentStep);
            bool bIsLeftDirOK  = TestStep(iCurrentX-1, iCurrentY,   iEndX, iEndY, pCurrentStep);

            if(m_bIsEight)
            {
                bool bIsUpRightOK   = false;
                bool bIsDownRightOK = false;
                bool bIsDownLeftOK  = false;
                bool bIsUpLeftOK    = false;

                if(bIsUpDirOK || bIsRightDirOK) bIsUpRightOK = TestStep(iCurrentX+1, iCurrentY-1, iEndX, iEndY, pCurrentStep);
                if(bIsDownDirOK || bIsRightDirOK) bIsDownRightOK = TestStep(iCurrentX+1, iCurrentY+1, iEndX, iEndY, pCurrentStep);
                if(bIsDownDirOK || bIsLeftDirOK) bIsDownLeftOK = TestStep(iCurrentX-1, iCurrentY+1, iEndX, iEndY, pCurrentStep);
                if(bIsUpDirOK || bIsLeftDirOK) bIsUpLeftOK = TestStep(iCurrentX-1, iCurrentY-1, iEndX, iEndY, pCurrentStep);

                bool bNeedToArchive = !bIsUpDirOK || !bIsRightDirOK || !bIsDownDirOK || !bIsLeftDirOK;

                if((!bNeedToArchive) && (bIsUpDirOK || bIsRightDirOK)) bNeedToArchive = bNeedToArchive || !bIsUpRightOK;
                if((!bNeedToArchive) && (bIsDownDirOK || bIsRightDirOK)) bNeedToArchive = bNeedToArchive || !bIsDownRightOK;
                if((!bNeedToArchive) && (bIsDownDirOK || bIsLeftDirOK)) bNeedToArchive = bNeedToArchive || !bIsDownLeftOK;
                if((!bNeedToArchive) && (bIsUpDirOK || bIsLeftDirOK)) bNeedToArchive = bNeedToArchive || !bIsUpLeftOK;

                if(bNeedToArchive) ArchiveTrace(pBestTrace);

            }
            else
            {
                if(!bIsUpDirOK || !bIsRightDirOK || !bIsDownDirOK || !bIsLeftDirOK)
                {
                    ArchiveTrace(pBestTrace);
                }
            }

        }

        pBestTrace = PopTrace();

    }

    return m_pClosedQueue;

}

ogePointArray* CogePathFinder::FindWay(int iStartX, int iStartY, int iEndX, int iEndY, ogePointArray* pWay)
{
    if(m_iState < 0 || pWay == NULL) return NULL;

    Clear();

    pWay->clear();

    CogeTraceNode* pBestTrace = FindTrace(iStartX, iStartY, iEndX, iEndY);

    if(pBestTrace == NULL) return NULL;

    CogeStepNode* pCurrentStep = pBestTrace->step;

    if(pCurrentStep == NULL) return NULL;

    //m_Way.resize(pCurrentStep->index);
    pWay->resize(pCurrentStep->index);

    int idx = pCurrentStep->index;
    while(pCurrentStep)
    {
        idx--;
        if(idx >= 0)
        {
            pWay->at(idx).x = pCurrentStep->posx;
            pWay->at(idx).y = pCurrentStep->posy;
        }
        pCurrentStep = pCurrentStep->prior;
    }

    //return &m_Way;

    return pWay;

}

/*---------------- Range Finder -----------------*/

int CogeRangeFinder::SetSize(int iColumnCount, int iRowCount)
{
    if(m_iColumnCount == iColumnCount && m_iRowCount == iRowCount) return 0;
    int iTotal = iColumnCount * iRowCount;

    m_History.resize(iTotal);
    ClearHistory();

    m_iColumnCount = iColumnCount;
    m_iRowCount = iRowCount;

    return iTotal;
}

int CogeRangeFinder::SetTileHistory(int idx, int value)
{
    if(m_History[idx] < 0 && value >= 0) m_iTotalChanges++;
    m_History[idx] = value;
    return value;
}

int CogeRangeFinder::Initialize(std::vector<int>* pTileValues, int iColumnCount, int iRowCount)
{
    m_pTileValues = pTileValues;
    SetSize(iColumnCount, iRowCount);
    m_iState = 0;
    return m_iState;
}

void CogeRangeFinder::ClearHistory()
{
    int iSize = m_History.size();
	for(int i=0; i<iSize; i++) m_History[i] = -1;
	m_iTotalChanges = 0;
}

void CogeRangeFinder::Clear()
{
    std::vector<CogeEdgeNode*>::iterator its;
	CogeEdgeNode* pEdge = NULL;

	its = m_Edges.begin();

	while (its != m_Edges.end())
	{
		pEdge = *its;
		m_Edges.erase(its);
		if(pEdge) delete pEdge;
		its = m_Edges.begin();
	}

	m_pCurrentEdge = NULL;
	m_pNextEdge = NULL;

	ClearHistory();

	//m_Range.clear();
}

void CogeRangeFinder::Finalize()
{
    Clear();

	m_History.clear();
	//m_Range.clear();

	m_iState = -1;

    return;
}

CogeRangeFinder::CogeRangeFinder()
{
    m_iState = -1;

    m_iColumnCount = 0;
    m_iRowCount = 0;

    m_iTotalChanges = 0;

    m_pCurrentEdge = NULL;
    m_pNextEdge = NULL;

    m_pTileValues = NULL;

    return;
}
CogeRangeFinder::~CogeRangeFinder()
{
    Finalize();
    return;
}

int CogeRangeFinder::PosToIndex(int iPosX, int iPosY)
{
    return m_iColumnCount * iPosY + iPosX;
}

CogeEdgeNode* CogeRangeFinder::AddEdge(int iPosX, int iPosY, int iRest)
{
    CogeEdgeNode* pNewEdge = new CogeEdgeNode();
    pNewEdge->posx = iPosX;
    pNewEdge->posy = iPosY;
    pNewEdge->rest = iRest;
    pNewEdge->next = m_pNextEdge;
    m_pNextEdge = pNewEdge;
    return m_pNextEdge;
}

bool CogeRangeFinder::TestTile(int iPosX, int iPosY, int iRest)
{
    if(iPosX < 0 || iPosY < 0) return false;
    if(iPosX >= m_iColumnCount || iPosY >= m_iRowCount) return false;
    int idx = PosToIndex(iPosX, iPosY);
    if(m_pTileValues == NULL || m_pTileValues->at(idx) > iRest || m_pTileValues->at(idx) < 0) return false;

    int iNewRest = iRest - m_pTileValues->at(idx);

    if(m_History[idx] > iNewRest) return false;
    else SetTileHistory(idx, iNewRest);

    AddEdge(iPosX, iPosY, iNewRest);

    return true;
}

void CogeRangeFinder::Travel(int iPosX, int iPosY, int iMovementPoints)
{
    if(m_iState < 0) return;

    AddEdge(iPosX, iPosY, iMovementPoints);
    SetTileHistory(PosToIndex(iPosX, iPosY), iMovementPoints);

    while(m_pNextEdge)
    {
    	m_pCurrentEdge = m_pNextEdge;
    	m_pNextEdge = NULL;

	    while(m_pCurrentEdge)
	    {
	        int iCurrentX = m_pCurrentEdge->posx;
	        int iCurrentY = m_pCurrentEdge->posy;
	        int iRest     = m_pCurrentEdge->rest;

	        TestTile(iCurrentX,   iCurrentY-1, iRest);
	        TestTile(iCurrentX+1, iCurrentY,   iRest);
	        TestTile(iCurrentX,   iCurrentY+1, iRest);
	        TestTile(iCurrentX-1, iCurrentY,   iRest);

	        m_pCurrentEdge = m_pCurrentEdge->next;
	    }
    }

}

ogePointArray* CogeRangeFinder::FindRange(int iPosX, int iPosY, int iMovementPoints, ogePointArray* pRange)
{
    if(m_iState < 0 || pRange == NULL) return NULL;

    Clear();

    pRange->clear();

    Travel(iPosX, iPosY, iMovementPoints);

    //m_Range.resize(m_iTotalChanges);
    pRange->resize(m_iTotalChanges);

    int idx = 0;

    for(int i=0; i<m_iColumnCount; i++)
    for(int j=0; j<m_iRowCount; j++)
    {
    	int value = m_History[PosToIndex(i, j)];
    	if(value >= 0)
    	{
            pRange->at(idx).x = i;
            pRange->at(idx).y = j;
            idx++;
    	}
    }

    return pRange;
}

