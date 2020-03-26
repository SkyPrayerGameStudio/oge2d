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

#ifndef __OGE_ENGINE_H_INCLUDED__
#define __OGE_ENGINE_H_INCLUDED__

#include "ogeCommon.h"
#include "ogeBuffer.h"
#include "ogePack.h"

#include "ogeVideo.h"
#include "ogeAudio.h"
#include "ogeMovie.h"
#include "ogeScript.h"
#include "ogeIniFile.h"
#include "ogeNet.h"
#include "ogeIM.h"
#include "ogeDatabase.h"

#include <string>
#include <vector>
#include <stack>
#include <list>
#include <map>

#define _OGE_MAX_JOY_COUNT_          8
#define _OGE_MAX_PLAYER_COUNT_       32
#define _OGE_MAX_EVENT_COUNT_        40
#define _OGE_MAX_JOY_KEY_NUMBER_     64
#define _OGE_MAX_DIRTY_RECT_NUMBER_  128
#define _OGE_MAX_NPC_COUNT_          128

#define _OGE_MAX_SCANCODE_           256

#ifdef __OGE_WITH_SDL2__
#define _OGE_MAX_N_KEY_NUMBER_       200
#define _OGE_MAX_S_KEY_NUMBER_       300
#else
#define _OGE_MAX_KEY_NUMBER_         350
#endif

#define _OGE_MAX_REL_SPR_            8

#define _OGE_MAX_VMOUSE_             4

#define _OGE_MAX_SOCK_COUNT_         64

#define _OGE_MAX_INT_VAR_            256
#define _OGE_MAX_FLOAT_VAR_          256
#define _OGE_MAX_STR_VAR_            256

#define _OGE_GLOBAL_VAR_INT_COUNT_   128
#define _OGE_GLOBAL_VAR_FLOAT_COUNT_ 64
#define _OGE_GLOBAL_VAR_STR_COUNT_   32


enum ogeOSType
{
    OS_Unknown        = 0,

	OS_Win32          = 1,
	OS_Linux          = 2,
	OS_MacOS          = 3,

	OS_WinCE          = 11,
	OS_Android        = 12,
	OS_iPhone         = 13
};

enum ogeInfo
{
	Info_FPS          = 0,
	Info_VideoMode    = 1,
	Info_MousePos     = 2
};

enum ogeGameMapType
{
    GameMap_Scrolling = 0,
    GameMap_RectTiles = 1,
    GameMap_Isometric = 2
};

enum ogeEvent
{
	Event_OnInit      = 0,
	Event_OnFinalize  = 1,
	Event_OnUpdate    = 2,
	Event_OnCollide   = 3,
	Event_OnDraw      = 4,
	Event_OnPathStep  = 5,
	Event_OnPathFin   = 6,
	Event_OnAnimaFin  = 7,

	Event_OnGetFocus  = 8,
	Event_OnLoseFocus = 9,

	Event_OnMouseOver = 10,
	Event_OnMouseIn   = 11,
	Event_OnMouseOut  = 12,
	Event_OnMouseDown = 13,
	Event_OnMouseUp   = 14,
	Event_OnMouseDrag = 15,
	Event_OnMouseDrop = 16,

	Event_OnKeyDown   = 17,    // Event_OnPathStepIn
	Event_OnKeyUp     = 18,    // Event_OnPathFin

	Event_OnLeave     = 19,
	Event_OnEnter     = 20,

	Event_OnActive    = 21,
	Event_OnDeactive  = 22,

	Event_OnOpen      = 23,    // Event_OnGetFocus
	Event_OnClose     = 24,    // Event_OnLoseFocus

	Event_OnDrawSprFin= 25,
	Event_OnDrawWinFin= 26,

	Event_OnSceneActive    = 27,
	Event_OnSceneDeactive  = 28,

	Event_OnTimerTime = 29,

	Event_OnCustomEnd = 30,

	Event_OnAcceptClient = 31,            // Event_OnCollide
    Event_OnReceiveFromRemoteClient = 32, // Event_OnMouseIn
    Event_OnRemoteClientDisconnect  = 33, // Event_OnMouseOut
    Event_OnRemoteServerDisconnect  = 34, // Event_OnLeave
    Event_OnReceiveFromRemoteServer = 35  // Event_OnEnter


};

enum ogeUnitType
{
    Spr_Empty         = 0,  // invisible but check collision
	Spr_Anima         = 1,  // has animation, but normally no collision
	Spr_Timer         = 2,  // invisible and no collision, it will auto-check timer event once be active
	Spr_Plot          = 3,  // invisible and no collision, user can pause/resume the event script in its update event
	Spr_Mouse         = 4,  // just like animation but always be drawn lastly in every frame
	Spr_Window        = 5,  // normal UI who can be dragged and dropped
	Spr_Panel         = 6,  // common UI just like window but normally will not support drag and drop
	Spr_InputText     = 7,  // UI for input text
	Spr_Npc           = 8,  // common sprite just like player but normally will not accept any input
	Spr_Player        = 9   // the player sprite who can accept input from user


};

enum ogeTagType
{
    Tag_Nothing       = 0,
    Tag_Player        = 1,
    Tag_Npc           = 2,
    Tag_Friend        = 3,
    Tag_Enemy         = 4,
    Tag_Effect        = 5,
    Tag_Skill         = 6,
    Tag_Gold          = 7,
    Tag_Tool          = 8,
    Tag_Equip         = 9,
    Tag_Medal         = 10,
    Tag_Poison        = 11,
    Tag_Magic         = 12,
    Tag_Bullet        = 13,
    Tag_EnemyBullet   = 14,
    Tag_Component     = 15,
    Tag_Text          = 16,
    Tag_Button        = 17,
    Tag_Container     = 18,
    Tag_Input         = 19
};

enum ogeDir
{
    Dir_Down          = 1,
	Dir_Up            = 2,
	Dir_Right         = 4,
	Dir_DownRight     = 5,
	Dir_UpRight       = 6,
	Dir_Left          = 8,
	Dir_DownLeft      = 9,
	Dir_UpLeft        = 10
};

enum ogeAction
{
	Act_Stand         = 1,
	Act_Move          = 2,
	Act_Walk          = 3,
	Act_Run           = 4,
	Act_Jump          = 5,
	Act_Hunker        = 6,
	Act_Fall          = 7,
	Act_Climb         = 8,
	Act_Appear        = 9,
	Act_Disappear     = 10,
	Act_Attack        = 11,
	Act_Defend        = 12,
	Act_Fire          = 13,
	Act_Throw         = 14,
	Act_Say           = 15,
	Act_Magic         = 16,
	Act_Cure          = 17,
	Act_Hurt          = 18,
	Act_Die           = 19,
	Act_Birth         = 20,
	Act_PowerUp       = 21,
	Act_PowerDown     = 22,
	Act_Get           = 23,
	Act_Lose          = 24,
	Act_Explode       = 25,
	Act_Pass          = 26,
	Act_Fail          = 27,
	Act_Push          = 28,
	Act_Drag          = 29,

	Act_WinNormal     = 30,
	Act_WinMouseOver  = 31,
	Act_WinMouseClick = 32

};

enum ogeFadeType
{
    Fade_None         = 0,
    Fade_Lightness    = 1,
    Fade_Bright       = 2,
    Fade_Dark         = 3,
    Fade_Alpha        = 4,
    Fade_Mask         = 5,
    Fade_RandomMask   = 6
};

enum ogeLightMode
{
	Light_M_None      = 0,  // no light
	Light_M_View      = 1,  // global light on view window only
	Light_M_Map       = 2   // light with whole map
};


enum ogeEffectMode
{
	Effect_M_None     = 0,  // no effect
	Effect_M_Sprite   = 1,  // global effect
	Effect_M_Frame    = 2   // one effect to one frame
};

enum ogeEffectType
{

	Effect_Lightness  = 1,
	Effect_RGB        = 2,
	Effect_Color      = 3,
	Effect_Rota       = 4,
	Effect_Wave       = 5,
	Effect_Edge       = 6,

	Effect_Scale      = 127,
	Effect_Alpha      = 128

};

/*---------------- Virtual Mouse -----------------*/

struct CogeVMouse
{
    int x;
    int y;
    bool pressed;
    bool moving;
    bool active;
    bool done;
};

/*---------------- Joystick -----------------*/

struct CogeJoystick
{
    int id;
    SDL_Joystick* joy;
    int m_KeyTime[_OGE_MAX_JOY_KEY_NUMBER_];
    bool m_KeyState[_OGE_MAX_JOY_KEY_NUMBER_];
};


/*---------------- RenderEffect -----------------*/

struct CogeEffect
{
    bool active;
    int effect_type;
    int repeat_times;
    int time;
    int interval;
    double effect_value;
    double start_value;
    double end_value;
    double step_value;
};

//typedef std::map<int, CogeEffect*> ogeEffectMap;
typedef std::vector<CogeEffect*> ogeEffectList;

class CogeFrameEffect;

typedef std::map<int, CogeFrameEffect*> ogeFrameEffectMap;

class CogeEngine;
class CogeScene;

class CogeGameMap;
class CogeSpriteGroup;

class CogeSprite;
class CogeAnima;


/*---------------- Custom Event Set -----------------*/

class CogeCustomEventSet
{
private:

    CogeScript* m_pCommonScript;
    CogeScript* m_pLocalScript;

    CogeScript* m_pCurrentScript;

    CogeSprite* m_pPlotSpr;

    std::string m_sName;

    int m_iCurrentEventCode;

    int* m_pTrigger;


    int m_CommonEvents[_OGE_MAX_EVENT_COUNT_];
    int m_LocalEvents[_OGE_MAX_EVENT_COUNT_];

    std::vector<int> m_PendingEvents;

public:

    CogeCustomEventSet(const std::string& sName, CogeScript* pCommonScript, CogeScript* pLocalScript = NULL);
    ~CogeCustomEventSet();

    void SetCommonEvent(int iCustomEventCode, int iScriptEntryId);
    void SetLocalEvent(int iCustomEventCode, int iScriptEntryId);

    void SetTrigger(int* pTrigger);
    void SetPlotSpr(CogeSprite* pPlotSpr);

    void CallEvent(int iCustomEventCode);
    void CallBaseEvent();

    void CallCustomEvents();
    void PushEvent(int iCustomEventCode);

    int GetCustomEventCount(); // check whether there is any Custom Event missing loading.
    int GetPendingEventCount();

    void ClearPendingEvents();

    friend class CogeSprite;

};

/*---------------- Path -----------------*/

class CogePath
{
private:

    //int m_iDir;
    //int m_iStep;

    int m_iType;

    int m_iSegmentLength;  // for Bezier ...

    //int m_iStepInterval;
    //int m_iStepTime;
    //int m_iValidStepCount;

    std::string m_sName;

    ogePoints m_Keys;

    ogePoints m_Points;

    void AddLine(int x1, int y1, int x2, int y2);

public:

    void ClearKeys();
    void ClearPoints();

    void GenShortLine(int x1, int y1, int x2, int y2);

    int GetTotalStepCount();

    //int GetValidStepCount();
    //void SetValidStepCount(int iCount);
    //int GetStepInterval();
    //void SetStepInterval(int iInterval);

    CogePath();
    ~CogePath();

    void ParseLineRoad();

    void ParseBezierRoad();

    friend class CogeEngine;
    friend class CogeSprite;

};

/*---------------- Path Finder -----------------*/

struct CogeStepNode
{
    int index;
    int posx;
    int posy;
    CogeStepNode* prior;
};

struct CogeTraceNode
{
    int lastidx;
    int dist;
    CogeStepNode* step;
    CogeTraceNode* next;
};

class CogePathFinder
{
private:

    int m_iState;

    int m_iColumnCount;
    int m_iRowCount;

    bool m_bIsEight;

    std::vector<CogeTraceNode*> m_Traces;
    std::vector<CogeStepNode*> m_Steps;

    CogeTraceNode* m_pOpenQueue;
    CogeTraceNode* m_pClosedQueue;

    std::vector<int>* m_pTileValues;

    std::vector<int> m_History;

    //ogePointArray m_Way;

    int PosToIndex(int iPosX, int iPosY);

    int SetSize(int iColumnCount, int iRowCount);
    int EstDist(int iStartX, int iStartY, int iEndX, int iEndY);
    void InsertTrace(CogeTraceNode* pTraceNode);
    void ArchiveTrace(CogeTraceNode* pTraceNode);
    CogeTraceNode* PopTrace();
    CogeStepNode* NewStep(CogeStepNode* pPriorStep, int iPosX, int iPosY);
    CogeTraceNode* NewTrace(CogeStepNode* pStepNode, int iDist);
    bool TestStep(int iStartX, int iStartY, int iEndX, int iEndY, CogeStepNode* pPriorStep);

    CogeTraceNode* FindTrace(int iStartX, int iStartY, int iEndX, int iEndY);

    void Clear();

protected:

public:

    int Initialize(std::vector<int>* pTileValues, int iColumnCount, int iRowCount);
    void Finalize();

    bool GetEightDirectionMode();
    void SetEightDirectionMode(bool value);

    ogePointArray* FindWay(int iStartX, int iStartY, int iEndX, int iEndY, ogePointArray* pWay);

    CogePathFinder();
    ~CogePathFinder();

};


/*---------------- Range Finder -----------------*/

struct CogeEdgeNode
{
    int rest;
    int posx;
    int posy;
    CogeEdgeNode* next;
};

class CogeRangeFinder
{
private:

    int m_iState;

    int m_iColumnCount;
    int m_iRowCount;

    int m_iTotalChanges;

    std::vector<CogeEdgeNode*> m_Edges;

    CogeEdgeNode* m_pCurrentEdge;
    CogeEdgeNode* m_pNextEdge;

    std::vector<int>* m_pTileValues;

    std::vector<int> m_History;

    //ogePointArray m_Range;

    int PosToIndex(int iPosX, int iPosY);

    int SetSize(int iColumnCount, int iRowCount);

    int SetTileHistory(int idx, int value);
    void ClearHistory();

    CogeEdgeNode* AddEdge(int iPosX, int iPosY, int iRest);

    bool TestTile(int iPosX, int iPosY, int iRest);

    void Travel(int iPosX, int iPosY, int iMovementPoints);

    void Clear();

protected:

public:

    int Initialize(std::vector<int>* pTileValues, int iColumnCount, int iRowCount);
    void Finalize();

    ogePointArray* FindRange(int iPosX, int iPosY, int iMovementPoints, ogePointArray* pRange);

    CogeRangeFinder();
    ~CogeRangeFinder();

};

/*------------------- Data ---------------------*/

class CogeGameData
{
private:

    std::string m_sName;

    int m_iIntVarCount;
    int m_iFloatVarCount;
    int m_iStrVarCount;

    int m_iBufVarCount;

    //int m_iTotalUsers;

    std::vector<int> ints;
    std::vector<double> floats;
    std::vector<std::string> strs;

    std::vector<CogeCharBuf> bufs;

    //void Hire();
    //void Fire();

protected:

public:

    explicit CogeGameData(const std::string& sName);
    ~CogeGameData();

    const std::string& GetName();
    void SetName(const std::string& sName);

    int GetIntVarCount();
    int GetFloatVarCount();
    int GetStrVarCount();
    int GetBufVarCount();

    void SetIntVarCount(int iCount);
    void SetFloatVarCount(int iCount);
    void SetStrVarCount(int iCount);
    void SetBufVarCount(int iCount);

    void Clear(bool bIncludeBuffer = false);

    int GetIntVar(int idx);
    double GetFloatVar(int idx);
    const std::string& GetStrVar(int idx);
    char* GetBufVar(int idx);

    void SetIntVar(int idx, int iValue);
    void SetFloatVar(int idx, double fValue);
    void SetStrVar(int idx, const std::string& sValue);
    void SetBufVar(int idx, const std::string& sValue);

    void SortInt();
    void SortFloat();
    void SortStr();

    int IndexOfInt(int value);
    int IndexOfFloat(double value);
    int IndexOfStr(const std::string& value);

    void Copy(CogeGameData* pData);

    friend class CogeEngine;
    friend class CogeScene;
    friend class CogeSprite;

};

typedef std::map<int, CogeAnima*> ogeAnimaMap;

typedef std::map<std::string, CogeGameData*> ogeGameDataMap;
typedef std::list<CogeGameData*> ogeGameDataList;

typedef std::map<std::string, CogePath*> ogePathMap;

//typedef std::map<std::string, CogeThinker*> ogeThinkerMap;

typedef std::map<std::string, CogeGameMap*> ogeGameMapMap;

typedef std::list<CogeSprite*> ogeSpriteList;
typedef std::map<std::string, CogeSprite*> ogeSpriteMap;

typedef std::map<std::string, CogeScene*> ogeSceneMap;

typedef std::map<std::string, CogeSpriteGroup*> ogeGroupMap;

typedef std::map<int, CogeIniFile*> ogeIniFileMap;

typedef std::map<int, CogeStreamBuffer*> ogeStreamBufMap;

//class CogeChapter;
//typedef std::map<std::string, CogeChapter*> ogeChapterMap;

/*---------------- Engine -----------------*/

class CogeEngine
{
private:

    CogeBufferManager* m_pBuffer;

    CogePacker*      m_pPacker;

    CogeVideo*       m_pVideo;

    CogeAudio*       m_pAudio;

    CogeMoviePlayer* m_pPlayer;

    CogeScripter*    m_pScripter;

    CogeScript*      m_pScript;

    CogeScene*       m_pActiveScene;
    CogeScene*       m_pLastActiveScene;
    CogeScene*       m_pNextActiveScene;

    CogeImage*       m_pCurrentMask;

    CogeGameData*    m_pAppGameData;
    CogeGameData*    m_pCustomData;

    CogeNetwork*     m_pNetwork;

    CogeIM*          m_pIM;

    CogeDatabase*    m_pDatabase;


    //CogeImage*       m_pLightMap;

    //CogeScene*       m_pLoadingScene;

    CogeScene*       m_pCurrentGameScene;
    CogeSprite*      m_pCurrentGameSprite;

    CogeSocket*      m_Socks[_OGE_MAX_SOCK_COUNT_];

    CogeIniFile      m_AppIniFile;

    CogeIniFile      m_ImageIniFile;
    CogeIniFile      m_FontIniFile;
    CogeIniFile      m_MediaIniFile;
    CogeIniFile      m_ObjectIniFile;
    CogeIniFile      m_PathIniFile;
    //CogeIniFile      m_EventIniFile;
    CogeIniFile      m_GroupIniFile;
    //CogeIniFile      m_GameMapIniFile;
    CogeIniFile      m_GameDataIniFile;

    CogeIniFile      m_LibraryIniFile;

    CogeIniFile      m_ConstIntIniFile;

    CogeIniFile      m_UserConfigFile;

    std::string      m_sAppDefaultDbFile;


    SDL_Event        m_GlobalEvent;

    //PogeEvent        m_OnInit;
    //PogeEvent        m_OnUpdateScreen;

    //ogeEventIdMap    m_EventMap;

    //ogeChapterMap    m_ChapterMap;
    //CogeChapter*     m_pActiveChapter;

    std::map<std::string, int> m_EventIdMap;

    ogeSceneMap      m_SceneMap;
    //ogeSpriteMap     m_SpriteMap;
    ogePathMap       m_PathMap;

    ogeIniFileMap    m_IniFileMap;

    ogeStreamBufMap  m_StreamBufMap;

    //ogeSoundCodeMap  m_SoundCodeMap;
    //ogeThinkerMap    m_ThinkerMap;

    std::vector<CogeSound*> m_Sounds;
    std::vector<CogeSpriteGroup*> m_Groups;
    std::vector<CogePath*> m_Paths;

    ogeGroupMap      m_GroupMap;

    ogeGameMapMap    m_GameMapMap;

    ogeGameDataMap   m_GameDataMap;
    ogeGameDataList  m_GameDataList;

    ogeImageMap      m_TransMasks;

    ogeSceneMap      m_InitScenes;
    ogeSpriteMap     m_InitSprites;

    int              m_Events[_OGE_MAX_EVENT_COUNT_];

#ifdef __OGE_WITH_SDL2__
    int              m_NKeyTime[_OGE_MAX_N_KEY_NUMBER_];
    bool             m_NKeyState[_OGE_MAX_N_KEY_NUMBER_];
    int              m_SKeyTime[_OGE_MAX_S_KEY_NUMBER_];
    bool             m_SKeyState[_OGE_MAX_S_KEY_NUMBER_];
#else
    int              m_KeyTime[_OGE_MAX_KEY_NUMBER_];
    bool             m_KeyState[_OGE_MAX_KEY_NUMBER_];
#endif

    Uint32           m_KeyMap[_OGE_MAX_SCANCODE_];

    int              m_iJoystickCount;
    CogeJoystick     m_Joysticks[_OGE_MAX_JOY_COUNT_];

    CogeVMouse       m_VMouseList[_OGE_MAX_VMOUSE_];

    //int              m_IntVars[_OGE_MAX_INT_VAR_];
    //float            m_FloatVars[_OGE_MAX_FLOAT_VAR_];
    //std::string      m_StrVars[_OGE_MAX_STR_VAR_];

    CogeSprite*      m_Players[_OGE_MAX_PLAYER_COUNT_];
    CogeSprite*      m_Npcs[_OGE_MAX_NPC_COUNT_];

    CogeGameData*    m_UserData[_OGE_MAX_NPC_COUNT_];
    CogePath*        m_UserPaths[_OGE_MAX_NPC_COUNT_];
    CogeImage*       m_UserImages[_OGE_MAX_NPC_COUNT_];
    CogeSpriteGroup* m_UserGroups[_OGE_MAX_NPC_COUNT_];

    void*            m_UserObjects[_OGE_MAX_NPC_COUNT_];

    std::string m_sName;

    std::string m_sActiveSceneName;
    std::string m_sNextSceneName;

    std::string m_sTitle;

    int m_iVideoWidth;
    int m_iVideoHeight;
    int m_iVideoBPP;

    int m_iFrameInterval;

    bool m_bShowFPS;
    bool m_bShowVideoMode;
    bool m_bShowMousePos;

    bool m_bUseDirtyRect;

    bool m_bEnableInput;

    bool m_bKeyEventHappened;

    bool m_bPackedIndex;
    bool m_bPackedImage;
    //bool m_bPackedMask;
    bool m_bPackedMedia;
    bool m_bPackedFont;
    bool m_bPackedScript;
    bool m_bPackedSprite;
    bool m_bPackedScene;

    bool m_bEncryptedIndex;
    bool m_bEncryptedImage;
    bool m_bEncryptedMedia;
    bool m_bEncryptedFont;
    bool m_bEncryptedScript;
    bool m_bEncryptedSprite;
    bool m_bEncryptedScene;

    bool m_bBinaryScript;

    int m_iMouseX;
    int m_iMouseY;

    int m_iMouseLeft;
    int m_iMouseRight;
    int m_iMouseOldLeft;
    int m_iMouseOldRight;

    int m_iMainVMouseId;
    int m_iActiveVMouseCount;

    int m_iKeyDelay;
    int m_iKeyInterval;

    Uint8 m_iMouseState;

    int m_iScriptState;

    int m_iState;

    //int m_iThreadForInput;


    int m_iLastCycleTime;
    int m_iCycleRate;
    int m_iOldCycleRate;
    int m_iCycleRateWanted;
    int m_iCycleCount;
    int m_iGlobalTime;

    bool m_bFreeze;

    int m_iLockedCPS;

    int  m_iCycleInterval;
    int  m_iCurrentInterval;


    int LoadLibraries();

    int LoadConstInt();

    int LoadKeyMap();

    void DelAllScenes();
    //void DelAllSprites();

    void DelAllPaths();

    //void DelAllThinkers();

    void DelAllGroups();

    void DelAllMaps();

    void DelAllGameData();

    void DelAllIniFiles();

    void DelAllStreamBufs();

    void RegisterEventNames();

    void HandleAppEvents();
    void UpdateMouseInput();
    bool HandleIMEvents();
    void HandleEngineEvents();
    void HandleNetworkEvents();
    void UpdateActiveScene();

    int GetValidPackedFilePathLength(const std::string& sResFilePath);

    bool LoadIniFile(CogeIniFile* pIniFile, const std::string& sIniFile, bool bPacked, bool bEncrypted = false);

    //int SDLCALL FilterEvents(const SDL_Event *event);
    //int SDLCALL HandleMouse(void *unused);
    //int SDLCALL HandleKeyboard(void *unused);

protected:

    //void DoInit(void* Sender);
    //void DoUpdateScreen(void* Sender);

    //void DoInit();
    //void DoDraw();


public:

    CogeEngine();
    explicit CogeEngine(CogeScripter* pScripter);
    ~CogeEngine();

    CogeVideo* GetVideo();

    CogeImage* GetScreen();

    CogeNetwork* GetNetwork();

    CogeScripter* GetScripter();

    CogeIM* GetIM();

    CogeDatabase* GetDatabase();

    int GetOS();

    int CallEvent(int iEventCode);

    bool LibraryExists(const std::string& sLibraryName);
    bool ServiceExists(const std::string& sLibraryName, const std::string& sServiceName);
    int CallService(const std::string& sLibraryName, const std::string& sServiceName, void* pParam);

    int Initialize(const std::string& sConfigFileName);
    int Initialize(int iWidth, int iHeight, int iBPP, bool bFullScreen = false);

    void Finalize();

    int Terminate();

    int Run();

    int PrepareFirstChapter();

    void StopBroadcastKeyEvent();
    void SkipCurrentMouseEvent();

    void ResetVMouseState();

    int GetLockedCPS();
    void LockCPS(int iCPS);
    void UnlockCPS();
    int GetCPS();
    int GetGameUpdateInterval();
    int GetCurrentInterval();

    void DrawFPS(int x=0, int y=0);
    void DrawMousePos(int x=0, int y=0);
    void DrawVideoMode(int x=0, int y=0);

    int GetState();

    const std::string& GetName();
    const std::string& GetTitle();

    void ShowInfo(int iInfoType);
    void HideInfo(int iInfoType);

    bool GetFreeze();
    void SetFreeze(bool bValue);

    void EnableKeyRepeat();
    void DisableKeyRepeat();

    bool IsDirtyRectMode();
    void SetDirtyRectMode(bool bValue);

    bool IsUnicodeIM();
    bool IsInputMethodReady();

    void UpdateScreen(int x, int y);

    void ScrollScene(int iIncX, int iIncY, bool bImmediately);
    void SetSceneViewPos(int iLeft, int iTop, bool bImmediately);
    int GetSceneViewPosX();
    int GetSceneViewPosY();
    int GetSceneViewWidth();
    int GetSceneViewHeight();

    int GetCurrentKey();
    int GetCurrentScanCode();

    int SetCurrentMask(const std::string& sMaskName);

    CogeImage* RandomMask();

    //void SetEvent_OnInit(void* pEvent);
    //void SetEvent_OnUpdateScreen(void* pEvent);

    //CogeChapter* NewChapter(const std::string& sChapterName, const std::string& sConfigFileName);
    //CogeChapter* FindChapter(const std::string& sChapterName);
    //CogeChapter* GetChapter(const std::string& sChapterName, const std::string& sConfigFileName);
    //int DelChapter(const std::string& sChapterName);

    CogeScene* NewScene(const std::string& sSceneName, const std::string& sConfigFileName);
    CogeScene* FindScene(const std::string& sSceneName);
    CogeScene* GetScene(const std::string& sSceneName, const std::string& sConfigFileName);
    CogeScene* GetScene(const std::string& sSceneName);
    int DelScene(const std::string& sSceneName);
    int DelScene(CogeScene* pScene);
    int GetSceneCount();
    CogeScene* GetSceneByIndex(int idx);

    int SetActiveScene(const std::string& sSceneName);
    int SetActiveScene(CogeScene* pActiveScene);

    int SetNextScene(const std::string& sSceneName);
    int SetNextScene(CogeScene* pNextScene);

    void FadeIn (int iFadeType);
    void FadeOut(int iFadeType);

    CogeSprite* NewSpriteWithConfig(const std::string& sSpriteName, const std::string& sConfigFileName);
    CogeSprite* NewSprite(const std::string& sSpriteName, const std::string& sBaseName);
    //CogeSprite* FindSprite(const std::string& sSpriteName);
    //CogeSprite* GetSpriteWithConfig(const std::string& sSpriteName, const std::string& sConfigFileName);
    //CogeSprite* GetSprite(const std::string& sSpriteName, const std::string& sBaseName);
    //int DelSprite(const std::string& sSpriteName);
    int DelSprite(CogeSprite* pSprite);

    CogePath* NewPath(const std::string& sPathName);
    CogePath* FindPath(const std::string& sPathName);
    CogePath* GetPath(const std::string& sPathName);
    CogePath* GetPath(int iPathCode);
    int DelPath(const std::string& sPathName);

    CogeGameData* NewFixedGameData(const std::string& sGameDataName, int iIntCount = 0, int iFloatCount = 0, int iStrCount = 0);
    CogeGameData* NewGameData(const std::string& sGameDataName, const std::string& sTemplateName);
    CogeGameData* NewGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount);
    CogeGameData* FindGameData(const std::string& sGameDataName);
    CogeGameData* GetGameData(const std::string& sGameDataName, const std::string& sTemplateName);
    CogeGameData* GetGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount);
    int DelGameData(const std::string& sGameDataName);
    int DelGameData(CogeGameData* pGameData);

    int SaveGameDataToIniFile(CogeGameData* pGameData, CogeIniFile* pIniFile);
    int LoadGameDataFromIniFile(CogeGameData* pGameData, CogeIniFile* pIniFile);

    int SaveGameDataToDb(CogeGameData* pGameData, int iSessionId);
    int LoadGameDataFromDb(CogeGameData* pGameData, int iSessionId);

    CogeGameData* GetAppGameData();

    void SetCustomData(CogeGameData* pGameData);
    CogeGameData* GetCustomData();

    //CogeThinker* NewThinker(const std::string& sThinkerName);
    //CogeThinker* FindThinker(const std::string& sThinkerName);
    //CogeThinker* GetThinker(const std::string& sThinkerName);
    //int DelThinker(const std::string& sThinkerName);

    CogeScript* NewFixedScript(const std::string& sScriptFile);

    CogeScript* NewScript(const std::string& sScriptName);
    CogeScript* FindScript(const std::string& sScriptName);
    CogeScript* GetScript(const std::string& sScriptName);
    int DelScript(const std::string& sScriptName);

    CogeSpriteGroup* NewGroup(const std::string& sGroupName);
    CogeSpriteGroup* FindGroup(const std::string& sGroupName);
    CogeSpriteGroup* GetGroup(const std::string& sGroupName);
    CogeSpriteGroup* GetGroup(int iGroupCode);
    int DelGroup(const std::string& sGroupName);
    int GetGroupCount();
    CogeSpriteGroup* GetGroupByIndex(int idx);


    CogeGameMap* NewGameMap(const std::string& sGameMapName);
    CogeGameMap* FindGameMap(const std::string& sGameMapName);
    CogeGameMap* GetGameMap(const std::string& sGameMapName);
    int DelGameMap(const std::string& sGameMapName);
    int DelGameMap(CogeGameMap* pGameMap);

    CogeIniFile* NewIniFile(const std::string& sFilePath);
    CogeIniFile* FindIniFile(int iIniFileId);
    //CogeIniFile* GetIniFile(const std::string& sIniFileName, const std::string& sFilePath);
    int DelIniFile(int iIniFileId);

    CogeStreamBuffer* NewStreamBuf(char* pBuf, int iBufSize, int iReadWritePos = 0);
    CogeStreamBuffer* FindStreamBuf(int iStreamBufId);
    int DelStreamBuf(int iStreamBufId);

    CogeImage* NewImage(const std::string& sImageName);
    CogeImage* GetImage(const std::string& sImageName);
    CogeImage* FindImage(const std::string& sImageName);
    bool DelImage(const std::string& sImageName);

    CogeSound* NewSound(const std::string& sSoundName);
    CogeSound* GetSound(const std::string& sSoundName);
    CogeSound* FindSound(const std::string& sSoundName);
    int DelSound(const std::string& sSoundName);

    CogeMusic* NewMusic(const std::string& sMusicName);
    CogeMusic* GetMusic(const std::string& sMusicName);
    CogeMusic* FindMusic(const std::string& sMusicName);
    int DelMusic(const std::string& sMusicName);

    CogeMovie* NewMovie(const std::string& sMovieName);
    CogeMovie* GetMovie(const std::string& sMovieName);
    CogeMovie* FindMovie(const std::string& sMovieName);
    int DelMovie(const std::string& sMovieName);

    //int UseMusic(const std::string& sMusicName);
    int SetBgMusic(const std::string& sMusicName);
    int PlayBgMusic(int iLoopTimes = 0);

    int PlayMusic(int iLoopTimes = 0);
    int StopMusic();
    int PauseMusic();
    int ResumeMusic();

    int StopAudio();
    int PlaySound(const std::string& sSoundName, int iLoopTimes = 0);
    int PlaySound(int iSoundCode, int iLoopTimes = 0);
    int StopSound(const std::string& sSoundName);
    int StopSound(int iSoundCode);
    int PlayMusic(const std::string& sMusicName, int iLoopTimes = 0);
    int FadeInMusic(const std::string& sMusicName, int iLoopTimes = 0, int iFadingTime = 1000);
    int FadeOutMusic(int iFadingTime = 1000);

    //int BindSoundCode(int iSoundCode, CogeSound* pTheSound);
    //CogeSound* FindSoundByCode(int iSoundCode);

    int PlayMovie(const std::string& sMovieName, int iLoopTimes = 0);
    int StopMovie();
    bool IsPlayingMovie();

    int GetSoundVolume();
    int GetMusicVolume();
    void SetSoundVolume(int iValue);
    void SetMusicVolume(int iValue);

    bool GetFullScreen();
    void SetFullScreen(bool bValue);

    //bool InstallFont(const std::string& sFontName, int iFontSize, const std::string& sFontFileName);
    //void UninstallFont(const std::string& sFontName);
    //bool UseFont(const std::string& sFontName);

    CogeFont* NewFont(const std::string& sFontName, int iFontSize);
    CogeFont* FindFont(const std::string& sFontName, int iFontSize);
    CogeFont* GetFont(const std::string& sFontName, int iFontSize);
    int DelFont(const std::string& sFontName, int iFontSize);

    CogeFont* GetDefaultFont();
    void SetDefaultFont(CogeFont* pFont);

    const std::string& GetDefaultCharset();
    void SetDefaultCharset(const std::string& sDefaultCharset);

    const std::string& GetSystemCharset();
    void SetSystemCharset(const std::string& sSystemCharset);

    int StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer);
    std::string UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName);

    CogeTextInputter* NewTextInputter(const std::string& sInputterName);
    CogeTextInputter* FindTextInputter(const std::string& sInputterName);
    CogeTextInputter* GetTextInputter(const std::string& sInputterName);

    void DisableIM();
    void CloseIM();

    void EnableInput();
    void DisableInput();

    void SetKeyDown(int iKey, bool bDown = true);
    void SetKeyUp(int iKey);

    bool IsKeyDown(int iKey);
    bool IsKeyDown(int iKey, int iInterval);

    bool IsMouseLeftButtonDown();
    bool IsMouseRightButtonDown();

    bool IsMouseLeftButtonUp();
    bool IsMouseRightButtonUp();

    int GetMouseX();
    int GetMouseY();

    void HideSystemMouseCursor();
    void ShowSystemMouseCursor();

    int OpenJoysticks();
    int GetJoystickCount();
    bool IsJoyKeyDown(int iJoystickId, int iKey);
    bool IsJoyKeyDown(int iJoystickId, int iKey, int iInterval);
    void CloseJoysticks();

    void Screenshot();
    CogeImage* GetLastScreenshot();
    void SaveScreenshot(const std::string& sBmpFileName);

    CogeScene* GetCurrentScene();
    CogeScene* GetActiveScene();
    CogeScene* GetLastActiveScene();
    CogeScene* GetNextActiveScene();

    CogeSprite* GetCurrentSprite();
    void CallCurrentSpriteBaseEvent();

    CogeSprite* GetFocusSprite();
    void SetFocusSprite(CogeSprite* pSprite);

    CogeSprite* GetModalSprite();
    void SetModalSprite(CogeSprite* pSprite);

    CogeSprite* GetMouseSprite();
    void SetMouseSprite(CogeSprite* pSprite);

    CogeSprite* GetFirstSprite();
    void SetFirstSprite(CogeSprite* pSprite);

    CogeSprite* GetLastSprite();
    void SetLastSprite(CogeSprite* pSprite);

    int GetTopZ();
    int BringSpriteToTop(CogeSprite* pSprite);

    CogeSprite* GetPlayerSprite(int iPlayerId);
    void SetPlayerSprite(CogeSprite* pSprite, int iPlayerId);

    CogeSprite* GetUserSprite(int iNpcId);
    void SetUserSprite(CogeSprite* pSprite, int iNpcId);

    CogeSprite* GetScenePlayerSprite(int iPlayerId);
    void SetScenePlayerSprite(CogeSprite* pSprite, int iPlayerId);

    CogeSprite* GetSceneDefaultPlayerSpr();
    void SetSceneDefaultPlayerSpr(CogeSprite* pSprite);

    CogeSprite* GetSceneNpcSprite(int iNpcId);
    //void SetSceneNpcSprite(CogeSprite* pSprite, int iNpcId);

    //CogeSprite* GetPlotSprite();
    //CogeSprite* OpenPlot(CogeSprite* pSprite);
    //void ClosePlot(CogeSprite* pSprite);
    //bool IsPlayingPlot(CogeSprite* pSprite);

    void ActivateGroup(CogeSpriteGroup* pGroup);
    void DeactivateGroup(CogeSpriteGroup* pGroup);

    CogeSpriteGroup* GetUserGroup(int iNpcId);
    void SetUserGroup(CogeSpriteGroup* pGroup, int iNpcId);

    CogePath* GetUserPath(int iNpcId);
    void SetUserPath(CogePath* pPath, int iNpcId);

    CogeImage* GetUserImage(int iNpcId);
    void SetUserImage(CogeImage* pImage, int iNpcId);

    CogeGameData* GetUserData(int iNpcId);
    void SetUserData(CogeGameData* pData, int iNpcId);

    void* GetUserObject(int iNpcId);
    void SetUserObject(void* pObject, int iNpcId);

    void Pause();
    void Resume();
    bool IsPlaying();

    int GetSceneTime();

    int OpenLocalServer(int iPort);
    int CloseLocalServer();
    CogeSocket* ConnectServer(const std::string& sAddr);
    int DisconnectServer(const std::string& sAddr);
    int DisconnectClient(const std::string& sAddr);

    const std::string& GetCurrentSocketAddr();

    int GetRecvDataSize();
    int GetRecvDataType();
    //std::string GetRecvMsg();
    //int SendMsg(const std::string& sMsg);
    char* GetRecvData();
    int SendData(char* pData, int iLen, int iMsgType);

    int RefreshClientList();
    int RefreshServerList();
    CogeSocket* GetClientByIndex(int idx);
    CogeSocket* GetServerByIndex(int idx);
    CogeSocket* GetCurrentSocket();

    CogeSocket* GetPlayerSocket(int idx);
    void SetPlayerSocket(CogeSocket* pSocket, int idx);

    CogeSocket* GetDebugSocket();

    const std::string& GetLocalIp();
    CogeSocket* FindClient(const std::string& sAddr);
    CogeSocket* FindServer(const std::string& sAddr);
    CogeSocket* GetLocalServer();

    int StartRemoteDebug(const std::string& sAddr);
    int StopRemoteDebug();
    bool IsRemoteDebugMode();
    int ScanRemoteDebugRequest();

    int StartLocalDebug();

    CogeScript* GetCurrentScript();

    int OpenDefaultDb();

    CogeIniFile* GetUserConfig();
    CogeIniFile* GetAppConfig();

    /*
    std::string ReadString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
    std::string ReadPath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
    int ReadInteger(const std::string& sSectionName, const std::string& sFieldName, int iDefault);
    double ReadFloat(const std::string& sSectionName, const std::string& sFieldName, double fDefault);

    bool WriteString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue);
    bool WriteInteger(const std::string& sSectionName, const std::string& sFieldName, int iValue);
    bool WriteFloat(const std::string& sSectionName, const std::string& sFieldName, double fValue);

    bool SaveConfig();
    */


    friend class CogeScene;
    friend class CogeGameMap;
    friend class CogeSpriteGroup;
    friend class CogeSprite;
    friend class CogeAnima;

};


/*---------------- Chapter -----------------*/
/*
class CogeChapter
{
private:

    CogeEngine*     m_pEngine;

    ogeSceneMap     m_SceneMap;

    CogeScene*      m_pGameScene;
    CogeScene*      m_pGUIScene;

    std::string     m_sName;

protected:
public:

    CogeChapter(const std::string& sName, CogeEngine* pTheEngine);
    ~CogeChapter();


    int Initialize(const std::string& sConfigFileName);
    void Finalize();

    CogeScene* NewScene(const std::string& sSceneName, const std::string& sConfigFileName);
    CogeScene* FindScene(const std::string& sSceneName);
    CogeScene* GetScene(const std::string& sSceneName, const std::string& sConfigFileName);
    int DelScene(const std::string& sSceneName);

    int PrepareFirstScene();

    friend class CogeScene;
    friend class CogeEngine;


};
*/


/*---------------- Scene -----------------*/

class CogeScene
{
private:

    CogeEngine*      m_pEngine;

    //CogeAudio*       m_pAudio;

    CogeScript*      m_pScript;

    CogeImage*       m_pScreen;

    CogeImage*       m_pBackground;

    CogeImage*       m_pLightMap;

    CogeImage*       m_pFadeMask;

    CogeGameMap*     m_pMap;

    CogeMusic*       m_pBackgroundMusic;

    CogeSprite*      m_pCurrentSpr;

    CogeSprite*      m_pDraggingSpr;

    CogeSprite*      m_pModalSpr;

    CogeSprite*      m_pFocusSpr;

    CogeSprite*      m_pMouseSpr;

    CogeSprite*      m_pPlotSpr;

    CogeSprite*      m_pGettingFocusSpr;
    CogeSprite*      m_pLosingFocusSpr;

    CogeSprite*      m_pPlayerSpr;

    CogeSprite*      m_pFirstSpr;
    CogeSprite*      m_pLastSpr;

    CogeSprite*      m_Players[_OGE_MAX_PLAYER_COUNT_];
    //CogeSprite*      m_Npcs[_OGE_MAX_NPC_COUNT_];
    std::vector<CogeSprite*> m_Npcs;

    //CogeScene*       m_pNextScene;
    //CogeScene*       m_pSocketScene;

    CogeGameData*    m_pGameData;
    CogeGameData*    m_pCustomData;

    //CogeSound*       m_pSound;

    CogeIniFile      m_IniFile;

    std::string      m_sName;
    std::string      m_sChapterName;

    std::string      m_sTagName;

    //std::string      m_sNextSceneName;

    ogeSpriteMap     m_SpriteMap;

    ogeSpriteMap     m_ActiveSprites;
    ogeSpriteMap     m_DeactiveSprites;

    ogeSpriteList    m_SpritesInView;
    ogeSpriteList    m_SpritesForTouches;

    ogeSpriteList    m_Activating;
    ogeSpriteList    m_Deactivating;

    ogeSpriteList    m_PrepareActive;
    ogeSpriteList    m_PrepareDeactive;

    std::vector<CogeSpriteGroup*> m_Groups;

    int              m_Events[_OGE_MAX_EVENT_COUNT_];

    ogeRectList      m_DirtyRects;

    CogeRect         m_SceneViewRect;

    CogeRect         m_FPSInfoRect;
    CogeRect         m_VideoModeInfoRect;
    CogeRect         m_MousePosInfoRect;

    int              m_iSceneType;

    int              m_iTopZ;

    int              m_iScriptState;
    int              m_iState;

    //bool             m_bUseDirtyRect;
    int              m_iTotalDirtyRects;
    //bool             m_bNeedRedrawBg;

    int              m_iBackgroundWidth;
    int              m_iBackgroundHeight;

    int              m_iBackgroundColor;
    int              m_iBackgroundColorRGB;

    bool             m_bMouseDown;
    bool             m_bIsMouseEventHandled;

    bool             m_bBusy;

    int              m_iFadeState;
    int              m_iFadeEffectValue;
    int              m_iFadeStep;
    int              m_iFadeEffectCurrentValue;
    int              m_iFadeEffectStartValue;
    int              m_iFadeType;
    int              m_iFadeInType;
    int              m_iFadeOutType;
    int              m_iFadeInterval;
    int              m_iFadeTime;
    bool             m_bAutoFade;

    int              m_iScaleValue;

    int              m_iLightMode;
    bool             m_bEnableSpriteLight;

    int              m_iScrollStepX;
    int              m_iScrollStepY;
    int              m_iScrollCurrentX;
    int              m_iScrollCurrentY;
    int              m_iScrollTargetX;
    int              m_iScrollTargetY;
    int              m_iScrollInterval;
    int              m_iScrollTime;
    int              m_iScrollState; // 0: manual scroll; 1: auto scrolling
    int              m_bScrollLoop;

    bool             m_bHasDefaultData;
    bool             m_bOwnCustomData;

    bool             m_bEnableDefaultTimer;
    int              m_iTimerLastTick;
    int              m_iTimerInterval;
    int              m_iTimerEventCount;
    //int              m_iPlotTimerTriggerFlag;

    int              m_iActiveTime;
    int              m_iCurrentTime;
    int              m_iTimePoint;
    int              m_iPauseTime;

    int              m_iPendingPosX;
    int              m_iPendingPosY;
    int              m_iPendingScrollX;
    int              m_iPendingScrollY;


    void HandleEvent();

    void SkipCurrentMouseEvent();

    void CheckSpriteInteraction();

    void CheckInteraction();

    int CheckSpriteLifeCycles();

    void CheckSpriteCustomEvents(bool bIncludeLastSpr = false);

    bool IsDirtyRectMode();

    void ClearDirtyRects();

    void ValidateViewRect();

    void DrawBackground();

    void UpdateSprites();
    void DrawSprites();
    void DrawInfo();

    void PrepareLightMap();
    void BlendSpriteLight();
    void DrawLightMap();

    void UpdateScreen();

    void UpdatePendingOffset(bool bUpdateRelSpr);

    void Activate();
    void Deactivate();

    int BindGameData(CogeGameData* pGameData);
    void RemoveGameData();

    int DoFade();

    void DoScroll();


protected:

public:

    CogeScene(const std::string& sName, CogeEngine* pTheEngine);
    ~CogeScene();

    int Initialize(const std::string& sConfigFileName);
    void Finalize();

    const std::string& GetName();

    const std::string& GetChapterName();
    void SetChapterName(const std::string& sChapterName);

    const std::string& GetTagName();
    void SetTagName(const std::string& sTagName);

    CogeGameData* GetGameData();

    CogeGameData* GetCustomData();
    void SetCustomData(CogeGameData* pGameData, bool bOwnIt);

    CogeScript* NewScript(const std::string& sScriptName);

    int CallEvent(int iEventCode);

    void CallUpdateEvent();
    void CallMouseEvent(int iEventCode);
    void CallProxySprEvent(int iEventCode);

    int SetNextScene(CogeScene* pNextScene);
    int SetNextScene(const std::string& sSceneName);

    void FadeIn (int iFadeType, int iStartValue, int iEndValue, int iStep);
    void FadeOut(int iFadeType, int iStartValue, int iEndValue, int iStep);

    void FadeIn (int iFadeType = -1);
    void FadeOut(int iFadeType = -1);

    int GetFadeInSpeed();
    int SetFadeInSpeed(int iSpeed);
    int GetFadeOutSpeed();
    int SetFadeOutSpeed(int iSpeed);

    int GetFadeInType();
    int SetFadeInType(int iType);
    int GetFadeOutType();
    int SetFadeOutType(int iType);

    bool GetAutoFade();
    void SetAutoFade(bool value);

    CogeImage* GetFadeMask();
    void SetFadeMask(CogeImage* pMaskImage);

    int GetLightMode();
    CogeImage* GetLightMap();
    void SetLightMap(CogeImage* pLightMapImage, int iLightMode);

    //void EnableAutoScroll(int iStepX, int iStepY);
    //void DisableAutoScroll();
    int GetAutoScrollState();
    void StopAutoScroll();
    void StartAutoScroll(bool bResetParams, int iStepX, int iStepY, int iInterval, bool bLoop);
    void SetScrollSpeed(int iStepX, int iStepY, int iInterval);
    void SetScrollTarget(int iTargetX, int iTargetY);
    int GetScrollTargetX();
    int GetScrollTargetY();
    int GetScrollStepX();
    int GetScrollStepY();
    int GetScrollX();
    int GetScrollY();

    void ShowInfo(int iInfoType);
    void HideInfo(int iInfoType);

    int AddDirtyRect(CogeRect* rc);
    int AutoAddDirtyRect(CogeSprite* pSprite);

    int Prepare();

    int PlayBgMusic(int iLoopTimes = 0);
    const std::string& GetBgMusicName();
    int SetBgMusic(const std::string& sMusicName);

    int SetBgColor(int iRGBColor);

    int SetMap(const std::string& sMapName, int iNewLeft = 0, int iNewTop = 0);

    CogeGameMap* GetMap();
    CogeImage* GetBackground();

    //int SetBackground(const std::string& sImageName);

    CogeSprite* AddSprite(CogeSprite* pSprite);
    int RemoveSprite(const std::string& sSpriteName);
    int RemoveSprite(CogeSprite* pSprite);
    CogeSprite* FindSprite(const std::string& sSpriteName);
    CogeSprite* FindActiveSprite(const std::string& sSpriteName);
    CogeSprite* FindDeactiveSprite(const std::string& sSpriteName);

    CogeSprite* AddSprite(const std::string& sSpriteName, const std::string& sBaseName);

    int ActivateSprite(const std::string& sSpriteName);
    int DeactivateSprite(const std::string& sSpriteName);

    int PrepareActivateSprite(CogeSprite* pSprite);
    int PrepareDeactivateSprite(CogeSprite* pSprite);

    CogeSprite* GetCurrentSprite();

    CogeSprite* GetFocusSprite();
    void SetFocusSprite(CogeSprite* pSprite);

    CogeSprite* GetModalSprite();
    void SetModalSprite(CogeSprite* pSprite);

    CogeSprite* GetMouseSprite();
    void SetMouseSprite(CogeSprite* pSprite);

    CogeSprite* GetFirstSprite();
    void SetFirstSprite(CogeSprite* pSprite);

    CogeSprite* GetLastSprite();
    void SetLastSprite(CogeSprite* pSprite);

    int GetTopZ();
    int BringSpriteToTop(CogeSprite* pSprite);

    int GetSpriteCount(int iState);
    CogeSprite* GetSpriteByIndex(int iIndex, int iState);

    CogeSprite* GetDefaultPlayerSprite();
    void SetDefaultPlayerSprite(CogeSprite* pSprite);

    CogeSprite* GetPlayerSprite(int iPlayerId);
    void SetPlayerSprite(CogeSprite* pSprite, int iPlayerId);

    CogeSprite* GetNpcSprite(size_t iNpcId);
    //void SetNpcSprite(CogeSprite* pSprite, int iNpcId);

    CogeSprite* GetPlotSprite();
    void SetPlotSprite(CogeSprite* pSprite);

    //CogeSprite* GetPlotSprite();
    //CogeSprite* OpenPlot(CogeSprite* pSprite);
    //void ClosePlot(CogeSprite* pSprite);
    //bool IsPlayingPlot(CogeSprite* pSprite);

    void Pause();
    void Resume();
    bool IsPlaying();

    int GetActiveTime();

    void ScrollView(int iIncX, int iIncY, bool bImmediately, bool bUpdateRelSpr = true);
    void SetViewPos(int iPosX, int iPosY, bool bImmediately, bool bUpdateRelSpr = true);

    //bool OpenTimer(int iInterval, bool bEnablePlotTrigger);
    bool OpenTimer(int iInterval);
    void CloseTimer();
    int GetTimerInterval();
    bool IsTimerWaiting();


    int Update();


    friend class CogeEngine;
    friend class CogeGameMap;
    friend class CogeSprite;



};

/*---------------- GameMap -----------------*/

class CogeGameMap
{
private:

    CogeEngine*      m_pEngine;

    CogeScene*       m_pCurrentScene;

    CogeImage*       m_pBackground;

    CogePathFinder   m_PathFinder;

    CogeRangeFinder  m_RangeFinder;

    CogeIniFile      m_IniFile;

    std::vector<int> m_Tiles;
    std::vector<int> m_OriginalTiles;

    std::vector<int> m_Data;

    std::string      m_sName;

    int m_iBgWidth;
    int m_iBgHeight;

    int m_iTileRootX;
    int m_iTileRootY;

    int m_iTileWidth;
    int m_iTileHeight;
    //int m_iTileDepth;

    int m_iColCount;
    int m_iRowCount;

    int m_iFirstX;
    int m_iFirstY;

    int m_iMapType;

    int m_iTotalUsers;



public:

    CogeGameMap(const std::string& sTheName, CogeEngine* pTheEngine);
    ~CogeGameMap();

    const std::string& GetName();

    bool LoadConfigFile(const std::string& sConfigFileName);

    //bool Load(const std::string& sConfigFileName);

    void InitFinders();

    bool PixelToTile(int iPixelX, int iPixelY, int& iTileX, int& iTileY);
    bool TileToPixel(int iTileX, int iTileY, int& iPixelX, int& iPixelY);

    bool AlignPixel(int iInPixelX, int iInPixelY, int& iOutPixelX, int& iOutPixelY);

    int SetType(int iType);
    int GetType();

    int SetColumnCount(int iCount);
    int SetRowCount(int iCount);
    int GetColumnCount();
    int GetRowCount();

    int SetMapSize(int iColumnCount, int iRowCount);

    bool SetBgImage(CogeImage* pBackground);
    CogeImage* GetBgImage();
    int GetBgWidth();
    int GetBgHeight();

    void Hire();
    void Fire();

    int GetTileValue(int iTileX, int iTileY);
    int SetTileValue(int iTileX, int iTileY, int iValue);
    void ReloadTileValues();
    void InitTileValues(int iInitValue);

    void Reset();

    int GetTileData(int iTileX, int iTileY);
    int SetTileData(int iTileX, int iTileY, int iValue);
    void InitTileData(int iInitValue);

    bool GetEightDirectionMode();
    void SetEightDirectionMode(bool value);

    ogePointArray* FindWay(int iStartX, int iStartY, int iEndX, int iEndY, ogePointArray* pWay);
    ogePointArray* FindRange(int iPosX, int iPosY, int iMovementPoints, ogePointArray* pRange);


    friend class CogeEngine;
    friend class CogeScene;

};


/*-------------- Sprite Group ---------------*/

class CogeSpriteGroup
{
private:

    CogeEngine*      m_pEngine;

    CogeScene*       m_pCurrentScene;

    CogeSprite*      m_pRootSprite;

    ogeSpriteMap     m_SpriteMap;

    std::string      m_sName;

    std::string      m_sTemplate;
    //std::string      m_sRoot;

    //int              m_iTotalUsers;

    std::vector<CogeSprite*> m_Npcs;

    //void Hire();
    //void Fire();

public:

    CogeSpriteGroup(const std::string& sTheName, CogeEngine* pTheEngine);
    ~CogeSpriteGroup();

    const std::string& GetName();

    const std::string& GetTemplateSpriteName();
    //std::string GetRootSpriteBaseName();

    CogeSprite* GetRootSprite();

    CogeSprite* AddSprite(CogeSprite* pSprite);
    CogeSprite* AddSprite(const std::string& sSpriteName, const std::string& sBaseName);
    int RemoveSprite(const std::string& sSpriteName);
    int RemoveSprite(CogeSprite* pSprite);
    CogeSprite* FindSprite(const std::string& sSpriteName);

    CogeSprite* GetFreeSprite(int iSprType = -1, int iSprClassTag = -1, int iSprTag = -1);
    //CogeSprite* GetFreePlot();
    //CogeSprite* GetFreeTimer();

    int GetSpriteCount();
    CogeSprite* GetSpriteByIndex(int idx);

    CogeSprite* GetNpcSprite(size_t iNpcId);

    int LoadSpritesFromScene(const std::string& sSceneName);

    CogeScene* GetCurrentScene();
    void SetCurrentScene(CogeScene* pCurrentScene);
    void SetCurrentScene(const std::string& sSceneName);

    void Clear();

    int GenSprites(const std::string& sTempletSpriteName, int iCount);


    int LoadMembers(const std::string& sConfigFileName);


    friend class CogeEngine;
    friend class CogeScene;



};

/*---------------- Sprite -----------------*/

// Sprite Interface
class CogeSprite
{
private:

    CogeEngine*       m_pEngine;

    CogeScript*       m_pCommonScript;
    CogeScript*       m_pLocalScript;

    CogeScript*       m_pActiveUpdateScript;

    CogeScript*       m_pCurrentScript;

    CogeScene*        m_pCurrentScene;
    CogeSpriteGroup*  m_pCurrentGroup;

    CogeSprite*       m_pParent;
    CogeSprite*       m_pRoot;

    CogeSprite*       m_pCollidedSpr;
    CogeSprite*       m_pPlotSpr;
    CogeSprite*       m_pMasterSpr;

    CogeSprite*       m_pReplacedSpr;

    CogeSprite*       m_RelatedSprites[_OGE_MAX_REL_SPR_];
    CogeSpriteGroup*  m_RelatedGroups[_OGE_MAX_REL_SPR_];

    CogeAnima*        m_pCurrentAnima;

    CogePath*         m_pCurrentPath;

    CogePath*         m_pLocalPath; // dynamic path ..

    //CogeThinker*      m_pCurrentThinker; // ai

    CogeCustomEventSet* m_pCustomEvents;

    CogeGameData*     m_pGameData;
    CogeGameData*     m_pCustomData;

    //CogeSound*        m_pCurrentSound;

    CogeImage*        m_pLightMap;

    CogeFrameEffect*  m_pAnimaEffect;

    CogeTextInputter* m_pTextInputter;

    ogeSpriteMap     m_Children;

    ogeAnimaMap      m_Animation;

    //ogeActionSound   m_SoundMap;

    ogePointArray    m_TileList;

    std::vector<std::string> m_CustomEventNames;

    std::string      m_sName;
    std::string      m_sBaseName; // Sprite Template Name ...

    //ogeEventIdMap    m_EventMap;

    int              m_CommonEvents[_OGE_MAX_EVENT_COUNT_];
    int              m_LocalEvents[_OGE_MAX_EVENT_COUNT_];

    int              m_PlotTriggers[_OGE_MAX_EVENT_COUNT_];

    /*
    std::string      m_Text;
    std::string      m_FontName;
    int              m_FontSize;
    int              m_FontColor;
    int              m_FontBgColor;

    int              m_iCopyCount;
    int              m_iCopyNumber;
    */

    bool             m_bEnableAnima;
    bool             m_bEnableMovement;
    bool             m_bEnableCollision;
    bool             m_bEnableInput;

    bool             m_bEnableFocus;
    bool             m_bEnableDrag;

    bool             m_bEnableLightMap;

    bool             m_bEnableTimer;

    bool             m_bEnablePlot;

    //bool             m_bJoinedPlot;

    bool             m_bIsRelative;

    //int              m_iLightRelativeX;
    //int              m_iLightRelativeY;
    //int              m_iLightLocalX;
    //int              m_iLightLocalY;
    //int              m_iLightLocalWidth;
    //int              m_iLightLocalHeight;

    bool             m_bGetMouse;
    bool             m_bMouseIn;
    bool             m_bMouseDown;
    bool             m_bMouseDrag;

    int              m_iLastMouseDownX;
    int              m_iLastMouseDownY;

    int              m_iLastW;
    int              m_iLastH;

    int              m_iRelativeX;
    int              m_iRelativeY;

    int              m_iInputWinPosX;
    int              m_iInputWinPosY;


    int              m_iUnitType;

    int              m_iClassTag;
    int              m_iObjectTag;

    int              m_iWaitTime;
    int              m_iUpdateInterval;

    int              m_iDir;
    int              m_iAct;

    int              m_iTimerLastTick;
    int              m_iTimerInterval;
    int              m_iTimerEventCount;

    int              m_iDefaultTimerInterval;

    int              m_iActiveUpdateFunctionId;
    int              m_iTotalPlotRounds;
    int              m_iCurrentPlotRound;

    bool             m_bBusy;

    bool             m_bGetCollided;

    //bool             m_bEnableCollision;

    bool             m_bDefaultDraw;

    bool             m_bHasDefaultData;
    bool             m_bOwnCustomData;

    //bool dirty;
    bool             m_bGetInView;

    bool             m_bActive;
    bool             m_bVisible;
    //bool             m_bEnabled;

    bool             m_bCallingCustomEvents;

    int              m_iCurrentEventCode;

    //int              m_iXCount;
    //int              m_iYCount;
    //int              m_iXDelay;
    //int              m_iYDelay;

    int              m_iMovingTimeX;
    int              m_iMovingTimeY;
    int              m_iMovingIntervalX;
    int              m_iMovingIntervalY;

    //int              m_iPathDir;
    int              m_iPathOffsetX;
    int              m_iPathOffsetY;
    int              m_iPathStepIdx;
    int              m_iPathLastIdx;
    int              m_iPathKeyIdx;
    int              m_iPathLastKey;
    int              m_iPathStep;     // 0: no path;  > 0: low to high;  < 0: high to low;
    int              m_iPathStepTime;
    bool             m_bPathAutoStepIn;
    bool             m_bPathStepIn; // for step-in event;
    int              m_iPathStepInterval;
    int              m_iVirtualStepCount;

    //int              m_iThinkStepTime;
    //int              m_iThinkStepInterval;

    //int              m_iBodyWidth;
    //int              m_iBodyHeight;

    //int              m_iRootX;
    //int              m_iRootY;

    //int              m_iLastSoundCode;

    int              m_iLastDisplayFlag;

    int              m_iPosX;
    int              m_iPosY;
    int              m_iPosZ;

    int              m_iDefaultWidth;   // for (empty) sprite with no anima ...
    int              m_iDefaultHeight;
    int              m_iDefaultRootX;
    int              m_iDefaultRootY;

    int              m_iState;
    int              m_iScriptState;

    CogeRect         m_DrawPosRect;
    CogeRect         m_OldPosRect;

    //CogeRect         m_TempPosRect;

    CogeRect         m_DirtyRect;

    CogeRect         m_Body;

    //int              m_iTotalUsers;

    bool operator < (const CogeSprite& OtherSpr); // for list::sort() function

    //void Hire();
    //void Fire();

    void DelAllEffects();

    void DelAllAnimation();

    //void HandleEvent();

    void UpdateDirtyRect();

    //int BindGameData(const std::string& sGameDataName);
    int BindGameData(CogeGameData* pGameData);
    void RemoveGameData();

protected:

/*
    virtual void Draw();

    virtual bool Update();

    virtual void Collide();

    virtual void AnimaFin();

    virtual void Focus();

    virtual void MouseIn();
    virtual void MouseOut();
    virtual void MouseDown();
    virtual void MouseUp();
    virtual void MouseDrag();
    virtual void MouseDrop();
*/


public:

    CogeSprite(const std::string& sName, CogeEngine* pEngine, int iUnitType = Spr_Npc);
    ~CogeSprite();

    int Initialize(const std::string& sConfigFileName);
    void Finalize();

    const std::string& GetName();

    const std::string& GetBaseName();

    int CallEvent(int iEventCode);
    void CallBaseEvent();

    //int LoadScript(const std::string& sScriptName);
    int LoadLocalScript(CogeIniFile* pIniFile, const std::string& sSectionName);

    int BindAnima(int iDir, int iAct, CogeAnima* pTheAnima);
    CogeAnima* FindAnima(int iAnimaCode);

    CogeAnima* GetCurrentAnima();
    CogeImage* GetCurrentImage();
    CogeImage* GetAnimaImage(int iDir, int iAct);
    int SetImage(CogeImage* pImage);
    bool SetAnimaImage(int iDir, int iAct, CogeImage* pImage);

    bool GetDefaultDraw();
    void SetDefaultDraw(bool value);

    CogeScene* GetCurrentScene();
    CogeSpriteGroup*  GetCurrentGroup();

    void SetCustomData(CogeGameData* pGameData, bool bOwnIt);
    CogeGameData* GetCustomData();

    CogeSprite* GetCollidedSpr();

    CogeSprite* GetPlotSpr();
    void SetPlotSpr(CogeSprite* pSprite);

    CogeSprite* GetMasterSpr();
    void SetMasterSpr(CogeSprite* pSprite);

    CogeSprite* GetReplacedSpr();
    void SetReplacedSpr(CogeSprite* pSprite);

    CogeSprite* GetParentSpr();
    CogeSprite* GetRootSpr();

    CogeSprite* GetRelatedSpr(int iRelIdx);
    void SetRelatedSpr(CogeSprite* pSprite, int iRelIdx);

    CogeSpriteGroup* GetRelatedGroup(int iRelIdx);
    void SetRelatedGroup(CogeSpriteGroup* pGroup, int iRelIdx);

    int GetChildCount();
    CogeSprite* GetChildByIndex(int idx);

    CogeImage* GetLightMap();
    void SetLightMap(CogeImage* pLightMapImage);

    ogePointArray* GetTileList();

    CogeTextInputter* GetTextInputter();
    bool ConnectTextInputter(const std::string& sInputterName);
    bool ConnectTextInputter(CogeTextInputter* pTextInputter);
    void DisconnectTextInputter();
    char* GetInputTextBuffer();
    void ClearInputTextBuffer();
    int GetInputTextCharCount();
    int SetInputTextCharCount(int iSize);
    int GetInputterCaretPos();
    void ResetInputterCaretPos();
    void SetInputWinPos(int iPosX, int iPosY);

    int SetAnima(int iDir, int iAct);
    int SetDir(int iDir);
    int SetAct(int iAct);
    void ResetAnima();

    void SetDefaultSize(int iWidth, int iHeight);
    void SetDefaultRoot(int iRootX, int iRootY);

    bool OpenTimer(int iInterval);
    void CloseTimer();
    int GetTimerInterval();
    bool IsTimerWaiting();

    int SuspendUpdateScript();
    int ResumeUpdateScript();
    int GetUpdateScriptState();
    void EnablePlot(int iRounds);
    void DisablePlot();
    bool IsPlayingPlot();
    //void JoinPlot();
    //void DisjoinPlot();
    int GetPlotTriggerFlag(int iEventCode);
    void SetPlotTriggerFlag(int iEventCode, int iFlag);

    int AddAnimaEffect(int iEffectType, double fEffectValue, bool bIncludeChildren = false);
    int AddAnimaEffectEx(int iEffectType, double fStart, double fEnd, double fIncrement,  int iStepInterval = 5, int iRepeat = 0, bool bIncludeChildren = false);
    int RemoveAnimaEffect(int iEffectType, bool bIncludeChildren = false);
    void RemoveAllAnimaEffect(bool bIncludeChildren = false);
    bool HasEffect(int iEffectType);
    bool IsEffectCompleted(int iEffectType);
    double GetEffectValue(int iEffectType);
    double GetEffectStepValue(int iEffectType);

    void IncX(int x);
    void IncY(int y);

    void SetSpeed(int iSpeedX, int iSpeedY);

    void SetPos(int iPosX, int iPosY);
    void SetPos(int iPosX, int iPosY, int iPosZ);
    void SetPosZ(int iPosZ);

    void SetRelativePos(int iRelativeX, int iRelativeY);

    bool GetRelativePosMode();
    void SetRelativePosMode(bool bValue);

    void SetBusy(bool bValue);
    bool GetBusy();

    int SetActive(bool bValue);
    bool GetActive();

    int PrepareActive();
    int PrepareDeactive();

    void SetVisible(bool bValue);
    bool GetVisible();

    void SetInput(bool bValue);
    bool GetInput();

    void SetDrag(bool bValue);
    bool GetDrag();

    void SetCollide(bool bValue);
    bool GetCollide();

    int SetType(int iType);
    int GetType();

    void SetClassTag(int iTag);
    int GetClassTag();
    void SetObjectTag(int iTag);
    int GetObjectTag();

    int GetPosX();
    int GetPosY();
    int GetPosZ();
    int GetDir();
    int GetAct();

    int GetRelativeX();
    int GetRelativeY();

    int GetAnimaRootX();
    int GetAnimaRootY();
    int GetAnimaWidth();
    int GetAnimaHeight();

    //int UseThinker(const std::string& sThinkerName);
    //void RemoveThinker();

    //CogeCustomEventSet* GetCustomEventSet();

    void CallCustomEvents();
    void PushCustomEvent(int iCustomEventId);
    void ClearRunningCustomEvents();

    CogeGameData* GetGameData();

    CogePath* GetCurrentPath();
    CogePath* GetLocalPath();
    int UseLocalPath(int iStep = 1, int iStepInterval = 0, int iVirtualStepCount = -1, bool bAutoStepIn = true);
    int UsePath(CogePath* pPath, int iStep = 1, int iStepInterval = 0, int iVirtualStepCount = -1, bool bAutoStepIn = true);
    int ResetPath(int iStep = 1, int iStepInterval = 0, int iVirtualStepCount = -1, bool bAutoStepIn = true);
    int NextPathStep();
    void AbortPath();
    int GetCurrentStepIndex();
    int GetPathStepLength();
    void SetPathStepLength(int iStepLength);
    int GetPathStepInterval();
    void SetPathStepInterval(int iStepInterval);
    int GetPathStepCount();
    void SetPathStepCount(int iVirtualStepCount);
    int GetPathDir();
    int GetPathDirAvg();
    int GetPathKeyDir();

    int AddChild(CogeSprite* pChild, int iClientX, int iClientY);
    int AddChild(const std::string& sName, int iClientX, int iClientY);
    int RemoveChild(const std::string& sName);

    int SetParent(CogeSprite* pParent, int iClientX, int iClientY);
    int SetParent(const std::string& sParentName, int iClientX, int iClientY);

    void UpdateRoot(CogeSprite* pRoot);

    void AdjustParent();

    void ChildrenAdjust();

    void AdjustSceneView();

    void UpdatePosZ(int iValuePosZ);

    void PrepareNextFrame();

    void Update();

    void Draw();

    void DefaultDraw();

    friend class CogeEngine;
    friend class CogeScene;
    friend class CogeAnima;
    friend class CogeSpriteGroup;
    friend class ogeSprPosCompFunc;

};

struct ogeSprPosCompFunc
:std::binary_function<CogeSprite*, CogeSprite*, bool>
{
    bool operator()(CogeSprite* spr1, CogeSprite* spr2) const
    {
        if (spr1->m_iPosZ < spr2->m_iPosZ) return true;
        else if (spr1->m_iPosZ == spr2->m_iPosZ)
        {
            if (spr1->m_iPosY < spr2->m_iPosY) return true;
            else if (spr1->m_iPosY == spr2->m_iPosY)
            {
                if (spr1->m_iPosX < spr2->m_iPosX) return true;
                else return false;
            }
            else return false;
        }
        else return false;
    }
};



/*---------------- Animation -----------------*/

// Animation Interface
class CogeAnima
{
private:

    CogeEngine*       m_pEngine;

    CogeVideo*        m_pVideo;

    CogeImage*        m_pImage;

    CogeSprite*       m_pSprite;

    std::string       m_sName;
    //std::string       m_sImageName;

    SDL_Rect          m_Local;

    SDL_Rect          m_FrameRect;

    CogeRect          m_Body;

    //ogeEffectMap     m_EffectMap;
    //CogeEffect       m_AnimaEffect;

    ogeFrameEffectMap m_FrameEffectMap;

    int m_iEffectMode;

    bool m_bFinished;

    int m_iCurrentFrame;

    int m_iPlayTime;

    int m_iTotalFrames;

    int m_iFrameInterval;

    int m_iReplayInterval;

    bool m_bAutoReplay;

    int m_iRootX;
    int m_iRootY;

    int m_iLocalRight;
    int m_iLocalBottom;

    int m_iFrameWidth;
    int m_iFrameHeight;

    bool m_bImageOK;
    bool m_bFrameOK;

    int m_iState;


    int InitImages(const std::string& sImageName,
                   int iLocalX, int iLocalY, int iLocalWidth, int iLocalHeight);

    int InitFrames(int iTotalFrames,
                   int iFrameWidth, int iFrameHeight, int iRootX=-1, int iRootY=-1);

    void InitBody(int iLeft, int iTop, int iRight, int iBottom);

    int InitPlay(int iFrameInterval, bool bAutoReplay = true,
                 int iReplayInterval = 0, int iEffectMode = Effect_M_None);

    void DelAllEffects();


protected:

    //bool m_bActive;

public:

    CogeAnima(const std::string& sName, CogeEngine* pEngine);
    ~CogeAnima();

    int Init(const std::string& sImageName,
             int iLocalX, int iLocalY, int iLocalWidth, int iLocalHeight,
             int iTotalFrames, int iFrameWidth, int iFrameHeight, int iFrameInterval);

    //void SetAnimaEffect(int iEffectType, double fEffectValue);
    int AddFrameEffect(int iFrameIdx, int iEffectType, double fEffectValue);

    CogeFrameEffect* GetFrameEffect(int iFrameIdx);

    CogeImage* GetImage();
    bool SetImage(CogeImage* pImage);

    void Reset();

    int Update();

    void Draw(int iPosX, int iPosY, CogeFrameEffect* pGlobalEffect = NULL);


    friend class CogeVideo;
    friend class CogeSprite;
    friend class CogeScene;

};

class CogeFrameEffect
{
private:

    int m_iFrameId;

    ogeEffectList m_Effects;

public:

    int AddEffect(int iEffectType, double fEffectValue);
    int AddEffectEx(int iEffectType, double fStart, double fEnd, double fIncrement, int iStepInterval = 5, int iRepeat = 0);
    int DelEffect(int iEffectType);
    CogeEffect* FindEffect(int iEffectType);

    int DisableEffect(int iEffectType);

    const ogeEffectList& GetEffects() const;

    int GetFrameId();
    void SetFrameId(int value);

    int GetEffectCount();

    int GetActiveEffectCount();
    CogeEffect* GetFirstActiveEffect();

    void Clear();

    CogeFrameEffect();
    ~CogeFrameEffect();

    //friend class CogeAnima;
    //friend class CogeSprite;
};




#endif // __OGE_ENGINE_H_INCLUDED__
