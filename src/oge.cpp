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

#include "oge.h"
#include "ogeAppParam.h"
#include "ogeCommon.h"
#include "ogeBuffer.h"
#include "ogeScript.h"
#include "ogePack.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <cmath>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <sstream>


static const int _OGE_MAX_FUNC_COUNT_ = 800;
static const int _OGE_MAX_BUF_COUNT_  = 32;

static CogeScripter* g_script = NULL;
static CogeEngine*   g_engine = NULL;
static bool          g_init   = false;

static char g_buf[_OGE_MAX_TEXT_LEN_ * 4];

static char g_tmpbuf[_OGE_MAX_TEXT_LEN_];
//static char g_tmplongbuf[_OGE_MAX_LONG_TEXT_LEN_];

static char g_bufs[_OGE_MAX_BUF_COUNT_][_OGE_MAX_TEXT_LEN_];

static SDL_Rect g_rect;

// Inner functions ...
void OGE_RegisterScriptTypedefs(CogeScripter* pScripter);
void OGE_RegisterScriptEnumValues(CogeScripter* pScripter);
void OGE_RegisterScriptConstInts(CogeScripter* pScripter, const std::string& sConstFile);
void OGE_RegisterScriptFunctions(CogeScripter* pScripter, const std::string& sFuncFile = "");
int OGE_HandleInputScript(const std::string& sInputConfigFile);
int OGE_PackInputFiles(const std::string& sInputConfigFile);
// end

/*
int OGE_Init(int iWidth, int iHeight, int iBPP, bool bFullScreen)
{
    int iRsl = -1;

    if(!g_script) g_script = new CogeScripter();

    if(!g_engine) g_engine = new CogeEngine(g_script);

    if(g_engine) iRsl = g_engine->Initialize(iWidth, iHeight, iBPP, bFullScreen);

    if(iRsl >= 0) g_init = true; else g_init = false;

    return iRsl;
}
*/

int OGE_Init(int argc, char** argv)
{
    int iRsl = -1;

    OGE_InitAppParams(argc, argv);

    std::string sAppCmd = OGE_GetAppGameCmd();

    bool bIsDebugMode = sAppCmd.compare(_OGE_APP_CMD_DEBUG_) == 0 || sAppCmd.compare(_OGE_APP_CMD_SCENE_) == 0;
    bool bIsToolMode = sAppCmd.compare(_OGE_APP_CMD_SCRIPT_) == 0 || sAppCmd.compare(_OGE_APP_CMD_PACK_) == 0;

    if(bIsDebugMode)
    {

//#if SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0)

        bool bNeedResetOutput = true;

        std::string sDebugServerAddr = OGE_GetAppDebugServerAddr();
        if(sDebugServerAddr.compare("local") == 0) bNeedResetOutput = false;

        if(bNeedResetOutput) OGE_ResetStdOutput();

//#else
//        OGE_ResetStdOutput(); // always reset output ...
//#endif

    }

    OGE_Log("Main Dir: %s\n", OGE_GetAppMainDir().c_str());
    OGE_Log("Game Dir: %s\n", OGE_GetAppGameDir().c_str());

    std::string sConfigFileName = OGE_GetAppConfigFile();

    if(sConfigFileName.length() == 0)
    {
        sConfigFileName = OGE_GetAppGameDir() + "/" + _OGE_DEFAULT_GAME_CONFIG_FILE_;
    }

    //printf(sConfigFileName.c_str());
    //printf("\n");

    std::string sInputFile = OGE_GetAppInputFile();
    if(sInputFile.length() > 0)
    {
        if(OGE_IsFileExisted(sInputFile.c_str())) iRsl = 0;
        else printf("Can not find input file: %s", sInputFile.c_str());

    }

    if(!g_script) g_script = new CogeScripter();

    if(!bIsToolMode)
    {
        if(OGE_IsFileExisted(sConfigFileName.c_str()))
        {
            CogeIniFile ini;
            if(ini.Load(sConfigFileName))
            {
                int iScriptBinMode = ini.ReadInteger("Script", "BinMode",  0);
                g_script->SetBinaryMode(iScriptBinMode);
            }
        }

    }

    // begin to register

    OGE_RegisterScriptTypedefs(g_script);

    OGE_RegisterScriptEnumValues(g_script);

    std::string sFuncConfigFile = "";
    if(!bIsToolMode) sFuncConfigFile = OGE_GetAppGameDir() + "/" + _OGE_DEFAULT_FUNC_CONFIG_FILE_;

    OGE_RegisterScriptFunctions(g_script, sFuncConfigFile);

    // end of register


    if(!g_engine) g_engine = new CogeEngine(g_script);

    if(!bIsToolMode)
    {
        if(OGE_IsFileExisted(sConfigFileName.c_str()))
        {
            if(g_engine) iRsl = g_engine->Initialize(sConfigFileName);
        }
        else
        {
            printf("Can not find config file: %s", sConfigFileName.c_str());
        }
    }

    g_init = iRsl >= 0;

    return iRsl;
}

void OGE_RegisterScriptConstInts(CogeScripter* pScripter, const std::string& sConstFile)
{
    CogeIniFile ini;

    if(!pScripter) return;
    if(!ini.Load(sConstFile)) return;

    int iCount = ini.GetSectionCount();
    for(int i=0; i<iCount; i++)
    {
        std::string sType = ini.GetSectionNameByIndex(i);
        std::map<std::string, std::string>* pSection = ini.GetSectionByIndex(i);
        if(sType.length() == 0 || pSection == NULL) continue;

        size_t iSceneFlagPos = sType.find('_');
        bool bIsScene = iSceneFlagPos != std::string::npos;

        std::string sTypeName = "ogeUser" + sType;
        std::string sPrefix = sType + "_";

        if(bIsScene)
        {
            //sTypeName = "oge" + sType;
            sTypeName = sType.substr(iSceneFlagPos+1);
            sPrefix = "";
        }

        pScripter->RegisterEnum(sTypeName);

        std::map<std::string, std::string>::iterator it = pSection->begin();
        while(it != pSection->end())
        {
            std::string sName = it->first;
            std::string sValue = it->second;
            int iValue = atoi(sValue.c_str());

            pScripter->RegisterEnumValue(sTypeName, sPrefix + sName, iValue);

            it++;
        }
    }

}

void OGE_RegisterScriptTypedefs(CogeScripter* pScripter)
{
    if(pScripter == NULL || pScripter->GetBinaryMode() > 0) return;

    pScripter->RegisterTypedef("char", "int8");

}

void OGE_RegisterScriptEnumValues(CogeScripter* pScripter)
{
    if(!pScripter) return;

    if(pScripter->GetBinaryMode() > 0) return;

    // register const int ...

    pScripter->RegisterEnum("ogeOSType");

    pScripter->RegisterEnumValue("ogeOSType", "OS_Unknown",  OS_Unknown);

    pScripter->RegisterEnumValue("ogeOSType", "OS_Win32",    OS_Win32);
    pScripter->RegisterEnumValue("ogeOSType", "OS_Linux",    OS_Linux);
    pScripter->RegisterEnumValue("ogeOSType", "OS_MacOS",    OS_MacOS);

    pScripter->RegisterEnumValue("ogeOSType", "OS_WinCE",    OS_WinCE);
    pScripter->RegisterEnumValue("ogeOSType", "OS_Android",  OS_Android);
    pScripter->RegisterEnumValue("ogeOSType", "OS_iPhone",   OS_iPhone);


#ifdef __OGE_WITH_SDL2__

    pScripter->RegisterEnum("ogeNKey");

    pScripter->RegisterEnumValue("ogeNKey", "Key_Back",  8);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Enter", 13);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Esc",   27);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Space", 32);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Comma", 44);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Dot",   46);

    pScripter->RegisterEnumValue("ogeNKey", "Key_0",     48);
    pScripter->RegisterEnumValue("ogeNKey", "Key_1",     49);
    pScripter->RegisterEnumValue("ogeNKey", "Key_2",     50);
    pScripter->RegisterEnumValue("ogeNKey", "Key_3",     51);
    pScripter->RegisterEnumValue("ogeNKey", "Key_4",     52);
    pScripter->RegisterEnumValue("ogeNKey", "Key_5",     53);
    pScripter->RegisterEnumValue("ogeNKey", "Key_6",     54);
    pScripter->RegisterEnumValue("ogeNKey", "Key_7",     55);
    pScripter->RegisterEnumValue("ogeNKey", "Key_8",     56);
    pScripter->RegisterEnumValue("ogeNKey", "Key_9",     57);

    pScripter->RegisterEnumValue("ogeNKey", "Key_A",     97);
    pScripter->RegisterEnumValue("ogeNKey", "Key_B",     98);
    pScripter->RegisterEnumValue("ogeNKey", "Key_C",     99);
    pScripter->RegisterEnumValue("ogeNKey", "Key_D",     100);
    pScripter->RegisterEnumValue("ogeNKey", "Key_E",     101);
    pScripter->RegisterEnumValue("ogeNKey", "Key_F",     102);
    pScripter->RegisterEnumValue("ogeNKey", "Key_G",     103);
    pScripter->RegisterEnumValue("ogeNKey", "Key_H",     104);
    pScripter->RegisterEnumValue("ogeNKey", "Key_I",     105);
    pScripter->RegisterEnumValue("ogeNKey", "Key_J",     106);
    pScripter->RegisterEnumValue("ogeNKey", "Key_K",     107);
    pScripter->RegisterEnumValue("ogeNKey", "Key_L",     108);
    pScripter->RegisterEnumValue("ogeNKey", "Key_M",     109);
    pScripter->RegisterEnumValue("ogeNKey", "Key_N",     110);
    pScripter->RegisterEnumValue("ogeNKey", "Key_O",     111);
    pScripter->RegisterEnumValue("ogeNKey", "Key_P",     112);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Q",     113);
    pScripter->RegisterEnumValue("ogeNKey", "Key_R",     114);
    pScripter->RegisterEnumValue("ogeNKey", "Key_S",     115);
    pScripter->RegisterEnumValue("ogeNKey", "Key_T",     116);
    pScripter->RegisterEnumValue("ogeNKey", "Key_U",     117);
    pScripter->RegisterEnumValue("ogeNKey", "Key_V",     118);
    pScripter->RegisterEnumValue("ogeNKey", "Key_W",     119);
    pScripter->RegisterEnumValue("ogeNKey", "Key_X",     120);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Y",     121);
    pScripter->RegisterEnumValue("ogeNKey", "Key_Z",     122);

    pScripter->RegisterEnumValue("ogeNKey", "Key_Del",   127);

    pScripter->RegisterEnum("ogeSKey");

    pScripter->RegisterEnumValue("ogeSKey", "Key_F1",    0x103A);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F2",    0x103B);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F3",    0x103C);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F4",    0x103D);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F5",    0x103E);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F6",    0x103F);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F7",    0x1040);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F8",    0x1041);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F9",    0x1042);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F10",   0x1043);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F11",   0x1044);
    pScripter->RegisterEnumValue("ogeSKey", "Key_F12",   0x1045);

    pScripter->RegisterEnumValue("ogeSKey", "Key_Insert",0x1049);
    pScripter->RegisterEnumValue("ogeSKey", "Key_Home",  0x104A);
    pScripter->RegisterEnumValue("ogeSKey", "Key_PageUp",0x104B);
    pScripter->RegisterEnumValue("ogeSKey", "Key_End",   0x104D);
    pScripter->RegisterEnumValue("ogeSKey", "Key_PageDown", 0x104E);

    pScripter->RegisterEnumValue("ogeSKey", "Key_Right", 0x104F);
    pScripter->RegisterEnumValue("ogeSKey", "Key_Left",  0x1050);
    pScripter->RegisterEnumValue("ogeSKey", "Key_Down",  0x1051);
    pScripter->RegisterEnumValue("ogeSKey", "Key_Up",    0x1052);

    pScripter->RegisterEnumValue("ogeSKey", "Key_LCtrl", 0x10E0);
    pScripter->RegisterEnumValue("ogeSKey", "Key_LShift",0x10E1);
    pScripter->RegisterEnumValue("ogeSKey", "Key_LAlt",  0x10E2);

    pScripter->RegisterEnumValue("ogeSKey", "Key_RCtrl", 0x10E4);
    pScripter->RegisterEnumValue("ogeSKey", "Key_RShift",0x10E5);
    pScripter->RegisterEnumValue("ogeSKey", "Key_RAlt",  0x10E6);

    pScripter->RegisterEnumValue("ogeSKey", "AKey_Search",0x110C);
    pScripter->RegisterEnumValue("ogeSKey", "AKey_Home",  0x110D);
    pScripter->RegisterEnumValue("ogeSKey", "AKey_Back",  0x110E);

#else

    pScripter->RegisterEnum("ogeKey");

    pScripter->RegisterEnumValue("ogeKey", "Key_Back",  8);
    pScripter->RegisterEnumValue("ogeKey", "Key_Enter", 13);
    pScripter->RegisterEnumValue("ogeKey", "Key_Esc",   27);
    pScripter->RegisterEnumValue("ogeKey", "Key_Space", 32);
    pScripter->RegisterEnumValue("ogeKey", "Key_Comma", 44);
    pScripter->RegisterEnumValue("ogeKey", "Key_Dot",   46);

    pScripter->RegisterEnumValue("ogeKey", "Key_0",     48);
    pScripter->RegisterEnumValue("ogeKey", "Key_1",     49);
    pScripter->RegisterEnumValue("ogeKey", "Key_2",     50);
    pScripter->RegisterEnumValue("ogeKey", "Key_3",     51);
    pScripter->RegisterEnumValue("ogeKey", "Key_4",     52);
    pScripter->RegisterEnumValue("ogeKey", "Key_5",     53);
    pScripter->RegisterEnumValue("ogeKey", "Key_6",     54);
    pScripter->RegisterEnumValue("ogeKey", "Key_7",     55);
    pScripter->RegisterEnumValue("ogeKey", "Key_8",     56);
    pScripter->RegisterEnumValue("ogeKey", "Key_9",     57);

    pScripter->RegisterEnumValue("ogeKey", "Key_A",     97);
    pScripter->RegisterEnumValue("ogeKey", "Key_B",     98);
    pScripter->RegisterEnumValue("ogeKey", "Key_C",     99);
    pScripter->RegisterEnumValue("ogeKey", "Key_D",     100);
    pScripter->RegisterEnumValue("ogeKey", "Key_E",     101);
    pScripter->RegisterEnumValue("ogeKey", "Key_F",     102);
    pScripter->RegisterEnumValue("ogeKey", "Key_G",     103);
    pScripter->RegisterEnumValue("ogeKey", "Key_H",     104);
    pScripter->RegisterEnumValue("ogeKey", "Key_I",     105);
    pScripter->RegisterEnumValue("ogeKey", "Key_J",     106);
    pScripter->RegisterEnumValue("ogeKey", "Key_K",     107);
    pScripter->RegisterEnumValue("ogeKey", "Key_L",     108);
    pScripter->RegisterEnumValue("ogeKey", "Key_M",     109);
    pScripter->RegisterEnumValue("ogeKey", "Key_N",     110);
    pScripter->RegisterEnumValue("ogeKey", "Key_O",     111);
    pScripter->RegisterEnumValue("ogeKey", "Key_P",     112);
    pScripter->RegisterEnumValue("ogeKey", "Key_Q",     113);
    pScripter->RegisterEnumValue("ogeKey", "Key_R",     114);
    pScripter->RegisterEnumValue("ogeKey", "Key_S",     115);
    pScripter->RegisterEnumValue("ogeKey", "Key_T",     116);
    pScripter->RegisterEnumValue("ogeKey", "Key_U",     117);
    pScripter->RegisterEnumValue("ogeKey", "Key_V",     118);
    pScripter->RegisterEnumValue("ogeKey", "Key_W",     119);
    pScripter->RegisterEnumValue("ogeKey", "Key_X",     120);
    pScripter->RegisterEnumValue("ogeKey", "Key_Y",     121);
    pScripter->RegisterEnumValue("ogeKey", "Key_Z",     122);

    pScripter->RegisterEnumValue("ogeKey", "Key_Del",   127);

    pScripter->RegisterEnumValue("ogeKey", "Key_F1",    282);
    pScripter->RegisterEnumValue("ogeKey", "Key_F2",    283);
    pScripter->RegisterEnumValue("ogeKey", "Key_F3",    284);
    pScripter->RegisterEnumValue("ogeKey", "Key_F4",    285);
    pScripter->RegisterEnumValue("ogeKey", "Key_F5",    286);
    pScripter->RegisterEnumValue("ogeKey", "Key_F6",    287);
    pScripter->RegisterEnumValue("ogeKey", "Key_F7",    288);
    pScripter->RegisterEnumValue("ogeKey", "Key_F8",    289);
    pScripter->RegisterEnumValue("ogeKey", "Key_F9",    290);
    pScripter->RegisterEnumValue("ogeKey", "Key_F10",   291);
    pScripter->RegisterEnumValue("ogeKey", "Key_F11",   292);
    pScripter->RegisterEnumValue("ogeKey", "Key_F12",   293);

    pScripter->RegisterEnumValue("ogeKey", "Key_Up",    273);
    pScripter->RegisterEnumValue("ogeKey", "Key_Down",  274);
    pScripter->RegisterEnumValue("ogeKey", "Key_Right", 275);
    pScripter->RegisterEnumValue("ogeKey", "Key_Left",  276);
    pScripter->RegisterEnumValue("ogeKey", "Key_Insert",277);
    pScripter->RegisterEnumValue("ogeKey", "Key_Home",  278);
    pScripter->RegisterEnumValue("ogeKey", "Key_End",   279);
    pScripter->RegisterEnumValue("ogeKey", "Key_PageUp",280);
    pScripter->RegisterEnumValue("ogeKey", "Key_PageDown", 281);

    pScripter->RegisterEnumValue("ogeKey", "Key_RShift",303);
    pScripter->RegisterEnumValue("ogeKey", "Key_LShift",304);
    pScripter->RegisterEnumValue("ogeKey", "Key_RCtrl", 305);
    pScripter->RegisterEnumValue("ogeKey", "Key_LCtrl", 306);
    pScripter->RegisterEnumValue("ogeKey", "Key_RAlt",  307);
    pScripter->RegisterEnumValue("ogeKey", "Key_LAlt",  308);

    pScripter->RegisterEnumValue("ogeKey", "AKey_Search",341);
    pScripter->RegisterEnumValue("ogeKey", "AKey_Home",  342);
    pScripter->RegisterEnumValue("ogeKey", "AKey_Back",  343);

#endif

    pScripter->RegisterEnum("ogeJKey");

    pScripter->RegisterEnumValue("ogeJKey", "JKey_Down", Dir_Down);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_Up",   Dir_Up);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_Right",Dir_Right);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_Left", Dir_Left);

    pScripter->RegisterEnumValue("ogeJKey", "JKey_1",    10);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_2",    11);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_3",    12);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_4",    13);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_5",    14);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_6",    15);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_7",    16);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_8",    17);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_9",    18);
    pScripter->RegisterEnumValue("ogeJKey", "JKey_10",   19);

    pScripter->RegisterEnum("ogeSockMsg");

    pScripter->RegisterEnumValue("ogeSockMsg", "SockMsg_HB",   0);
    pScripter->RegisterEnumValue("ogeSockMsg", "SockMsg_Bin",  1);
    pScripter->RegisterEnumValue("ogeSockMsg", "SockMsg_Text", 2);

    pScripter->RegisterEnum("ogeSprState");
    pScripter->RegisterEnumValue("ogeSprState", "SprState_Inactive", -1);
    pScripter->RegisterEnumValue("ogeSprState", "SprState_All",       0);
    pScripter->RegisterEnumValue("ogeSprState", "SprState_Active",    1);

    pScripter->RegisterEnum("ogeSprType");
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Empty",  Spr_Empty);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Anima",  Spr_Anima);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Timer",  Spr_Timer);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Plot",   Spr_Plot);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Mouse",  Spr_Mouse);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Window", Spr_Window);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Panel",  Spr_Panel);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_InputText", Spr_InputText);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Npc",    Spr_Npc);
    pScripter->RegisterEnumValue("ogeSprType", "Spr_Player", Spr_Player);

    pScripter->RegisterEnum("ogeSprTag");
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Undefined",   Tag_Nothing);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Player",      Tag_Player);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Npc",         Tag_Npc);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Friend",      Tag_Friend);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Enemy",       Tag_Enemy);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Effect",      Tag_Effect);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Skill",       Tag_Skill);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Gold",        Tag_Gold);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Tool",        Tag_Tool);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Equip",       Tag_Equip);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Medal",       Tag_Medal);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Poison",      Tag_Poison);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Magic",       Tag_Magic);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Bullet",      Tag_Bullet);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_EnemyBullet", Tag_EnemyBullet);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Component",   Tag_Component);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Text",        Tag_Text);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Button",      Tag_Button);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Container",   Tag_Container);
    pScripter->RegisterEnumValue("ogeSprTag", "Tag_Input",       Tag_Input);


    pScripter->RegisterEnum("ogeEvent");

    pScripter->RegisterEnumValue("ogeEvent", "Event_OnInit",     Event_OnInit);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnFinalize", Event_OnFinalize);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnUpdate",   Event_OnUpdate);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnCollide",  Event_OnCollide);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnDraw",     Event_OnDraw);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnPathStep", Event_OnPathStep);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnPathFin",  Event_OnPathFin);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnAnimaFin", Event_OnAnimaFin);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnGetFocus", Event_OnGetFocus);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnLoseFocus",Event_OnLoseFocus);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseOver",Event_OnMouseOver);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseIn",  Event_OnMouseIn);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseOut", Event_OnMouseOut);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseDown",Event_OnMouseDown);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseUp",  Event_OnMouseUp);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseDrag",Event_OnMouseDrag);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnMouseDrop",Event_OnMouseDrop);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnKeyDown",  Event_OnKeyDown);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnKeyUp",    Event_OnKeyUp);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnLeave",    Event_OnLeave);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnEnter",    Event_OnEnter);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnActive",   Event_OnActive);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnDeactive", Event_OnDeactive);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnOpen",     Event_OnOpen);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnClose",    Event_OnClose);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnDrawSprFin", Event_OnDrawSprFin);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnDrawWinFin", Event_OnDrawWinFin);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnSceneActive",  Event_OnSceneActive);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnSceneDeactive",Event_OnSceneDeactive);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnTimerTime",  Event_OnTimerTime);
    pScripter->RegisterEnumValue("ogeEvent", "Event_OnCustomEnd",  Event_OnCustomEnd);

    pScripter->RegisterEnum("ogeDirection");

    pScripter->RegisterEnumValue("ogeDirection", "Dir_Down",     Dir_Down);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_Up",       Dir_Up);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_Right",    Dir_Right);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_DownRight",Dir_DownRight);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_UpRight",  Dir_UpRight);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_Left",     Dir_Left);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_DownLeft", Dir_DownLeft);
    pScripter->RegisterEnumValue("ogeDirection", "Dir_UpLeft",   Dir_UpLeft);

    pScripter->RegisterEnum("ogeAction");

    pScripter->RegisterEnumValue("ogeAction", "Act_Stand",   Act_Stand);
    pScripter->RegisterEnumValue("ogeAction", "Act_Move",    Act_Move);
    pScripter->RegisterEnumValue("ogeAction", "Act_Walk",    Act_Walk);
    pScripter->RegisterEnumValue("ogeAction", "Act_Run",     Act_Run);
    pScripter->RegisterEnumValue("ogeAction", "Act_Jump",    Act_Jump);
    pScripter->RegisterEnumValue("ogeAction", "Act_Hunker",  Act_Hunker);
    pScripter->RegisterEnumValue("ogeAction", "Act_Fall",    Act_Fall);
    pScripter->RegisterEnumValue("ogeAction", "Act_Climb",   Act_Climb);
    pScripter->RegisterEnumValue("ogeAction", "Act_Appear",  Act_Appear);
    pScripter->RegisterEnumValue("ogeAction", "Act_Disappear",Act_Disappear);
    pScripter->RegisterEnumValue("ogeAction", "Act_Attack",  Act_Attack);
    pScripter->RegisterEnumValue("ogeAction", "Act_Defend",  Act_Defend);
    pScripter->RegisterEnumValue("ogeAction", "Act_Fire",    Act_Fire);
    pScripter->RegisterEnumValue("ogeAction", "Act_Throw",   Act_Throw);
    pScripter->RegisterEnumValue("ogeAction", "Act_Say",     Act_Say);
    pScripter->RegisterEnumValue("ogeAction", "Act_Magic",   Act_Magic);
    pScripter->RegisterEnumValue("ogeAction", "Act_Cure",    Act_Cure);
    pScripter->RegisterEnumValue("ogeAction", "Act_Hurt",    Act_Hurt);
    pScripter->RegisterEnumValue("ogeAction", "Act_Die",     Act_Die);
    pScripter->RegisterEnumValue("ogeAction", "Act_Birth",   Act_Birth);
    pScripter->RegisterEnumValue("ogeAction", "Act_PowerUp", Act_PowerUp);
    pScripter->RegisterEnumValue("ogeAction", "Act_PowerDown",Act_PowerDown);
    pScripter->RegisterEnumValue("ogeAction", "Act_Get",     Act_Get);
    pScripter->RegisterEnumValue("ogeAction", "Act_Lose",    Act_Lose);
    pScripter->RegisterEnumValue("ogeAction", "Act_Explode", Act_Explode);
    pScripter->RegisterEnumValue("ogeAction", "Act_Pass",    Act_Pass);
    pScripter->RegisterEnumValue("ogeAction", "Act_Fail",    Act_Fail);
    pScripter->RegisterEnumValue("ogeAction", "Act_Push",    Act_Push);
    pScripter->RegisterEnumValue("ogeAction", "Act_Drag",    Act_Drag);

    pScripter->RegisterEnumValue("ogeAction", "Act_WinNormal",    Act_WinNormal);
    pScripter->RegisterEnumValue("ogeAction", "Act_WinMouseOver", Act_WinMouseOver);
    pScripter->RegisterEnumValue("ogeAction", "Act_WinMouseClick",Act_WinMouseClick);

    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action0", 40);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action1", 41);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action2", 42);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action3", 43);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action4", 44);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action5", 45);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action6", 46);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action7", 47);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action8", 48);
    pScripter->RegisterEnumValue("ogeAction", "Act_X_Action9", 49);

    pScripter->RegisterEnum("ogeEffect");

    pScripter->RegisterEnumValue("ogeEffect", "Effect_Lightness",  Effect_Lightness);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_RGB",   Effect_RGB);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Color", Effect_Color);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Rota",  Effect_Rota);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Wave",  Effect_Wave);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Edge",  Effect_Edge);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Scale", Effect_Scale);
    pScripter->RegisterEnumValue("ogeEffect", "Effect_Alpha", Effect_Alpha);

    pScripter->RegisterEnum("ogeFade");

    pScripter->RegisterEnumValue("ogeFade", "Fade_None",      Fade_None);
    pScripter->RegisterEnumValue("ogeFade", "Fade_Lightness", Fade_Lightness);
    pScripter->RegisterEnumValue("ogeFade", "Fade_Bright",    Fade_Bright);
    pScripter->RegisterEnumValue("ogeFade", "Fade_Dark",      Fade_Dark);
    pScripter->RegisterEnumValue("ogeFade", "Fade_Alpha",     Fade_Alpha);
    pScripter->RegisterEnumValue("ogeFade", "Fade_Mask",      Fade_Mask);
    pScripter->RegisterEnumValue("ogeFade", "Fade_RandomMask",Fade_RandomMask);

    pScripter->RegisterEnum("ogeLight");

    pScripter->RegisterEnumValue("ogeLight", "Light_None",    Light_M_None);
    pScripter->RegisterEnumValue("ogeLight", "Light_View",    Light_M_View);
    pScripter->RegisterEnumValue("ogeLight", "Light_Map",     Light_M_Map);

    pScripter->RegisterEnum("ogeDbType");

    pScripter->RegisterEnumValue("ogeDbType", "DB_SQLITE3",   DB_SQLITE3);
    pScripter->RegisterEnumValue("ogeDbType", "DB_MYSQL",     DB_MYSQL);
    pScripter->RegisterEnumValue("ogeDbType", "DB_ODBC",      DB_ODBC);
    pScripter->RegisterEnumValue("ogeDbType", "DB_ORACLE",    DB_ORACLE);
    pScripter->RegisterEnumValue("ogeDbType", "DB_POSTGRESQL",DB_POSTGRESQL);

}

void OGE_RegisterScriptFunctions(CogeScripter* pScripter, const std::string& sFuncFile)
{

    if(!pScripter) return;

    char flags[_OGE_MAX_FUNC_COUNT_ + 1];
    memset((void*)&flags[0], 0, sizeof(flags));
    if(sFuncFile.length() > 0 && OGE_IsFileExisted(sFuncFile.c_str()))
    {
        FILE *f;
        std::string sLine = "";
        char line[100+1]; int len = 0; int idx = 0;

        if( (f = fopen(sFuncFile.c_str(), "r" )) != NULL )
        {
            while( !feof( f ) )
            {
                if( fgets( line, 100, f ) != NULL )
                {
                    sLine = line;
                    sLine = OGE_TrimSpecifiedStr(sLine);
                    len = sLine.length();
                    if(idx + len > _OGE_MAX_FUNC_COUNT_) break;
                    memcpy((void*)&flags[idx], (void*)(sLine.c_str()), len);
                    idx = idx + len;
                }
            }

            fclose( f );
        }
    }

    int i = 0;

    // begin to register functions

    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Print, "void OGE_Print(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IntToStr, "string OGE_IntToStr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StrToInt, "int OGE_StrToInt(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_HexToInt, "int OGE_HexToInt(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FloatToStr, "string OGE_FloatToStr(double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StrToFloat, "double OGE_StrToFloat(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StrToUnicode, "int OGE_StrToUnicode(string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_UnicodeToStr, "string OGE_UnicodeToStr(int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StrLen, "int OGE_StrLen(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindStr, "int OGE_FindStr(string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CopyStr, "string OGE_CopyStr(string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DeleteStr, "string OGE_DeleteStr(string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReplaceStr, "string OGE_ReplaceStr(string &in, string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_TrimStr, "string OGE_TrimStr(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTicks, "int OGE_GetTicks()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetOS, "int OGE_GetOS()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDateTimeStr, "string OGE_GetDateTimeStr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDateTime, "bool OGE_GetDateTime(int &out, int &out, int &out, int &out, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Delay, "void OGE_Delay(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Random, "int OGE_Random(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RandomFloat, "double OGE_RandomFloat()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LibraryExists, "bool OGE_LibraryExists(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ServiceExists, "bool OGE_ServiceExists(string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CallService, "int OGE_CallService(string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGameMainDir, "const string& OGE_GetGameMainDir()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_MakeColor, "int OGE_MakeColor(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImage, "int OGE_GetImage(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LoadImage, "int OGE_LoadImage(string &in, int, int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImageName, "const string& OGE_GetImageName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetColorKey, "int OGE_GetColorKey(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetColorKey, "void OGE_SetColorKey(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPenColor, "int OGE_GetPenColor(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPenColor, "void OGE_SetPenColor(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFont, "int OGE_GetFont(string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFontName, "const string& OGE_GetFontName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFontSize, "int OGE_GetFontSize(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDefaultFont, "int OGE_GetDefaultFont()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetDefaultFont, "void OGE_SetDefaultFont(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImageFont, "int OGE_GetImageFont(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetImageFont, "void OGE_SetImageFont(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDefaultCharset, "const string& OGE_GetDefaultCharset()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetDefaultCharset, "void OGE_SetDefaultCharset(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSystemCharset, "const string& OGE_GetSystemCharset()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSystemCharset, "void OGE_SetSystemCharset(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImageWidth, "int OGE_GetImageWidth(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImageHeight, "int OGE_GetImageHeight(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetImageRect, "int OGE_GetImageRect(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRect, "int OGE_GetRect(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Draw, "void OGE_Draw(int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawWithEffect, "void OGE_DrawWithEffect(int, int, int, int, int, int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FillColor, "void OGE_FillColor(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FillRect, "void OGE_FillRect(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FillRectAlpha, "void OGE_FillRectAlpha(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawText, "void OGE_DrawText(int, string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawTextWithDotFont, "void OGE_DrawTextWithDotFont(int, string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTextWidth, "int OGE_GetTextWidth(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTextSize, "bool OGE_GetTextSize(int, string &in, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsInputUnicode, "bool OGE_IsInputUnicode()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawBufferText, "void OGE_DrawBufferText(int, int, int, int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufferTextWidth, "int OGE_GetBufferTextWidth(int, int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufferTextSize, "bool OGE_GetBufferTextSize(int, int, int, string &in, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawLine, "void OGE_DrawLine(int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawCircle, "void OGE_DrawCircle(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetClipboardB, "int OGE_GetClipboardB()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetClipboardC, "int OGE_GetClipboardC()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Screenshot, "void OGE_Screenshot()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLastScreenshot, "int OGE_GetLastScreenshot()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetScreenImage, "int OGE_GetScreenImage()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ImageRGB, "void OGE_ImageRGB(int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ImageLightness, "void OGE_ImageLightness(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ImageGrayscale, "void OGE_ImageGrayscale(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ImageBlur, "void OGE_ImageBlur(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StretchBltToScreen, "void OGE_StretchBltToScreen(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SaveImageAsBMP, "void OGE_SaveImageAsBMP(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ShowFPS, "void OGE_ShowFPS(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LockFPS, "void OGE_LockFPS(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFPS, "int OGE_GetFPS()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLockedFPS, "int OGE_GetLockedFPS()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LockCPS, "void OGE_LockCPS(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCPS, "int OGE_GetCPS()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLockedCPS, "int OGE_GetLockedCPS()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ShowMousePos, "void OGE_ShowMousePos(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetDirtyRectMode, "void OGE_SetDirtyRectMode(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_Scroll, "void OGE_Scroll(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetViewPos, "void OGE_SetViewPos(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetViewPosX, "int OGE_GetViewPosX()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetViewPosY, "int OGE_GetViewPosY()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetViewWidth, "int OGE_GetViewWidth()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetViewHeight, "int OGE_GetViewHeight()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopAutoScroll, "void OGE_StopAutoScroll()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StartAutoScroll, "void OGE_StartAutoScroll(int, int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetAutoScrollSpeed, "void OGE_SetAutoScrollSpeed(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetScrollTarget, "void OGE_SetScrollTarget(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneMap, "int OGE_SetSceneMap(int, string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMap, "int OGE_GetMap()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMapByName, "int OGE_GetMapByName(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMapName, "const string& OGE_GetMapName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMapWidth, "int OGE_GetMapWidth(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMapHeight, "int OGE_GetMapHeight(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMapImage, "int OGE_GetMapImage(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetEightDirectionMode, "bool OGE_GetEightDirectionMode(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetEightDirectionMode, "void OGE_SetEightDirectionMode(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetColumnCount, "int OGE_GetColumnCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRowCount, "int OGE_GetRowCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileValue, "int OGE_GetTileValue(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetTileValue, "int OGE_SetTileValue(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReloadTileValues, "void OGE_ReloadTileValues(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResetGameMap, "void OGE_ResetGameMap(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileData, "int OGE_GetTileData(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetTileData, "int OGE_SetTileData(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_InitTileData, "void OGE_InitTileData(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_InitTileValues, "void OGE_InitTileValues(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PixelToTile, "bool OGE_PixelToTile(int, int, int, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_TileToPixel, "bool OGE_TileToPixel(int, int, int, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_AlignPixel, "bool OGE_AlignPixel(int, int, int, int &out, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindWay, "int OGE_FindWay(int, int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindRange, "int OGE_FindRange(int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileListSize, "int OGE_GetTileListSize(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileXFromList, "int OGE_GetTileXFromList(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileYFromList, "int OGE_GetTileYFromList(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTileIndexInList, "int OGE_GetTileIndexInList(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_AddTileToList, "int OGE_AddTileToList(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveTileFromList, "int OGE_RemoveTileFromList(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ClearTileList, "void OGE_ClearTileList(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetKey, "int OGE_GetKey()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_HoldKeyEvent, "void OGE_HoldKeyEvent()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SkipMouseEvent, "void OGE_SkipMouseEvent()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetScanCode, "int OGE_GetScanCode()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ShowKeyboard, "void OGE_ShowKeyboard(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsKeyboardShown, "bool OGE_IsKeyboardShown()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetKeyDown, "void OGE_SetKeyDown(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetKeyUp, "void OGE_SetKeyUp(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsKeyDown, "bool OGE_IsKeyDown(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsKeyDownTime, "bool OGE_IsKeyDownTime(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetJoystickCount, "int OGE_GetJoystickCount()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsJoyKeyDown, "bool OGE_IsJoyKeyDown(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsJoyKeyDownTime, "bool OGE_IsJoyKeyDownTime(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsMouseLeftDown, "bool OGE_IsMouseLeftDown()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsMouseRightDown, "bool OGE_IsMouseRightDown()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsMouseLeftUp, "bool OGE_IsMouseLeftUp()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsMouseRightUp, "bool OGE_IsMouseRightUp()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMouseX, "int OGE_GetMouseX()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMouseY, "int OGE_GetMouseY()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetIntVar, "int OGE_GetIntVar(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetIntVar, "void OGE_SetIntVar(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFloatVar, "double OGE_GetFloatVar(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFloatVar, "void OGE_SetFloatVar(int, int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetStrVar, "const string& OGE_GetStrVar(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetStrVar, "void OGE_SetStrVar(int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufVar, "int OGE_GetBufVar(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetBufVar, "void OGE_SetBufVar(int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetIntVarCount, "int OGE_GetIntVarCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetIntVarCount, "void OGE_SetIntVarCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFloatVarCount, "int OGE_GetFloatVarCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFloatVarCount, "void OGE_SetFloatVarCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetStrVarCount, "int OGE_GetStrVarCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetStrVarCount, "void OGE_SetStrVarCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufVarCount, "int OGE_GetBufVarCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetBufVarCount, "void OGE_SetBufVarCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SortIntVar, "void OGE_SortIntVar(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SortFloatVar, "void OGE_SortFloatVar(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SortStrVar, "void OGE_SortStrVar(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindIntVar, "int OGE_FindIntVar(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindFloatVar, "int OGE_FindFloatVar(int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindStrVar, "int OGE_FindStrVar(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SplitStrIntoData, "int OGE_SplitStrIntoData(string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprCurrentScene, "int OGE_GetSprCurrentScene(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprCurrentGroup, "int OGE_GetSprCurrentGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSpr, "int OGE_GetSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprName, "const string& OGE_GetSprName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CallBaseEvent, "void OGE_CallBaseEvent()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindGroupSpr, "int OGE_FindGroupSpr(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindSceneSpr, "int OGE_FindSceneSpr(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFocusSpr, "int OGE_GetFocusSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFocusSpr, "void OGE_SetFocusSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetModalSpr, "int OGE_GetModalSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetModalSpr, "void OGE_SetModalSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMouseSpr, "int OGE_GetMouseSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetMouseSpr, "void OGE_SetMouseSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFirstSpr, "int OGE_GetFirstSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFirstSpr, "void OGE_SetFirstSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLastSpr, "int OGE_GetLastSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetLastSpr, "void OGE_SetLastSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetProxySpr, "int OGE_GetProxySpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetProxySpr, "void OGE_SetProxySpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FreezeSprites, "void OGE_FreezeSprites(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ShowMouseCursor, "void OGE_ShowMouseCursor(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTopZ, "int OGE_GetTopZ()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_BringSprToTop, "int OGE_BringSprToTop(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneSpr, "int OGE_GetSceneSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDefaultPlayerSpr, "int OGE_GetDefaultPlayerSpr()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetDefaultPlayerSpr, "void OGE_SetDefaultPlayerSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneLightMap, "int OGE_GetSceneLightMap(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneLightMap, "int OGE_SetSceneLightMap(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPlayerSpr, "int OGE_GetPlayerSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPlayerSpr, "void OGE_SetPlayerSpr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserSpr, "int OGE_GetUserSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserSpr, "void OGE_SetUserSpr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserGroup, "int OGE_GetUserGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserGroup, "void OGE_SetUserGroup(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserPath, "int OGE_GetUserPath(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserPath, "void OGE_SetUserPath(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserImage, "int OGE_GetUserImage(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserImage, "void OGE_SetUserImage(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserData, "int OGE_GetUserData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserData, "void OGE_SetUserData(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserObject, "int OGE_GetUserObject(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserObject, "void OGE_SetUserObject(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCaretPos, "int OGE_GetCaretPos(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResetCaretPos, "void OGE_ResetCaretPos(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetInputTextBuffer, "int OGE_GetInputTextBuffer(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ClearInputTextBuffer, "void OGE_ClearInputTextBuffer(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetInputCharCount, "int OGE_GetInputCharCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetInputText, "string OGE_GetInputText(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetInputText, "int OGE_SetInputText(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetInputWinPos, "void OGE_SetInputWinPos(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprActive, "bool OGE_GetSprActive(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprActive, "void OGE_SetSprActive(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprVisible, "bool OGE_GetSprVisible(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprVisible, "void OGE_SetSprVisible(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprInput, "bool OGE_GetSprInput(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprInput, "void OGE_SetSprInput(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprDrag, "bool OGE_GetSprDrag(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprDrag, "void OGE_SetSprDrag(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprCollide, "bool OGE_GetSprCollide(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprCollide, "void OGE_SetSprCollide(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprBusy, "bool OGE_GetSprBusy(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprBusy, "void OGE_SetSprBusy(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprDefaultDraw, "bool OGE_GetSprDefaultDraw(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprDefaultDraw, "void OGE_SetSprDefaultDraw(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprLightMap, "int OGE_GetSprLightMap(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprLightMap, "void OGE_SetSprLightMap(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCollidedSpr, "int OGE_GetCollidedSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprPlot, "int OGE_GetSprPlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprPlot, "void OGE_SetSprPlot(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprMaster, "int OGE_GetSprMaster(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprMaster, "void OGE_SetSprMaster(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetReplacedSpr, "int OGE_GetReplacedSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetReplacedSpr, "void OGE_SetReplacedSpr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprParent, "int OGE_GetSprParent(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprParent, "int OGE_SetSprParent(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRelatedSpr, "int OGE_GetRelatedSpr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetRelatedSpr, "void OGE_SetRelatedSpr(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRelatedGroup, "int OGE_GetRelatedGroup(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetRelatedGroup, "void OGE_SetRelatedGroup(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprTileList, "int OGE_GetSprTileList(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PushEvent, "void OGE_PushEvent(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprCurrentImage, "int OGE_GetSprCurrentImage(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprImage, "int OGE_SetSprImage(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprAnimationImg, "int OGE_GetSprAnimationImg(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprAnimationImg, "bool OGE_SetSprAnimationImg(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_MoveX, "void OGE_MoveX(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_MoveY, "void OGE_MoveY(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetDirection, "int OGE_SetDirection(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetAction, "int OGE_SetAction(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetAnimation, "int OGE_SetAnimation(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResetAnimation, "void OGE_ResetAnimation(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DrawSpr, "void OGE_DrawSpr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPosX, "int OGE_GetPosX(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPosY, "int OGE_GetPosY(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPosZ, "int OGE_GetPosZ(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPosXYZ, "void OGE_SetPosXYZ(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPos, "void OGE_SetPos(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPosZ, "void OGE_SetPosZ(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetRelPos, "void OGE_SetRelPos(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRelPosX, "int OGE_GetRelPosX(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRelPosY, "int OGE_GetRelPosY(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRelMode, "bool OGE_GetRelMode(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetRelMode, "void OGE_SetRelMode(int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprRootX, "int OGE_GetSprRootX(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprRootY, "int OGE_GetSprRootY(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprWidth, "int OGE_GetSprWidth(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprHeight, "int OGE_GetSprHeight(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprDefaultRoot, "void OGE_SetSprDefaultRoot(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprDefaultSize, "void OGE_SetSprDefaultSize(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDirection, "int OGE_GetDirection(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetAction, "int OGE_GetAction(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprType, "int OGE_GetSprType(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprClassTag, "int OGE_GetSprClassTag(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprTag, "int OGE_GetSprTag(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprTag, "void OGE_SetSprTag(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprChildCount, "int OGE_GetSprChildCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprChildByIndex, "int OGE_GetSprChildByIndex(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_AddEffect, "int OGE_AddEffect(int, int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_AddEffectEx, "int OGE_AddEffectEx(int, int, double, double, double, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveEffect, "int OGE_RemoveEffect(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_HasEffect, "bool OGE_HasEffect(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsEffectCompleted, "bool OGE_IsEffectCompleted(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetEffectValue, "double OGE_GetEffectValue(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetEffectIncrement, "double OGE_GetEffectIncrement(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCurrentPath, "int OGE_GetCurrentPath(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLocalPath, "int OGE_GetLocalPath(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_UsePath, "int OGE_UsePath(int, int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_UseLocalPath, "int OGE_UseLocalPath(int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResetPath, "int OGE_ResetPath(int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_NextPathStep, "int OGE_NextPathStep(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_AbortPath, "void OGE_AbortPath(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathNodeIndex, "int OGE_GetPathNodeIndex(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathStepLength, "int OGE_GetPathStepLength(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPathStepLength, "void OGE_SetPathStepLength(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathStepInterval, "int OGE_GetPathStepInterval(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPathStepInterval, "void OGE_SetPathStepInterval(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetValidPathNodeCount, "int OGE_GetValidPathNodeCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetValidPathNodeCount, "void OGE_SetValidPathNodeCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathDirection, "int OGE_GetPathDirection(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPath, "int OGE_GetPath(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathByCode, "int OGE_GetPathByCode(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PathGenLine, "void OGE_PathGenLine(int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PathGenExtLine, "void OGE_PathGenExtLine(int, int, int, int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPathNodeCount, "int OGE_GetPathNodeCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenTimer, "bool OGE_OpenTimer(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseTimer, "void OGE_CloseTimer(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTimerInterval, "int OGE_GetTimerInterval(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsTimerWaiting, "bool OGE_IsTimerWaiting(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenSceneTimer, "bool OGE_OpenSceneTimer(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseSceneTimer, "void OGE_CloseSceneTimer()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneTimerInterval, "int OGE_GetSceneTimerInterval()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsSceneTimerWaiting, "bool OGE_IsSceneTimerWaiting()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetScenePlot, "int OGE_GetScenePlot()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetScenePlot, "void OGE_SetScenePlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlayPlot, "void OGE_PlayPlot(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenPlot, "void OGE_OpenPlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ClosePlot, "void OGE_ClosePlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PausePlot, "void OGE_PausePlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResumePlot, "void OGE_ResumePlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsPlayingPlot, "bool OGE_IsPlayingPlot(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetPlotTriggerFlag, "int OGE_GetPlotTriggerFlag(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetPlotTriggerFlag, "void OGE_SetPlotTriggerFlag(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PauseGame, "void OGE_PauseGame()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResumeGame, "void OGE_ResumeGame()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsPlayingGame, "bool OGE_IsPlayingGame()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_QuitGame, "int OGE_QuitGame()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprGameData, "int OGE_GetSprGameData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprCustomData, "int OGE_GetSprCustomData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSprCustomData, "void OGE_SetSprCustomData(int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetAppCustomData, "int OGE_GetAppCustomData()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetAppCustomData, "void OGE_SetAppCustomData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneCustomData, "int OGE_GetSceneCustomData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneCustomData, "void OGE_SetSceneCustomData(int, int, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindGameData, "int OGE_FindGameData(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGameData, "int OGE_GetGameData(string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGameDataName, "const string& OGE_GetGameDataName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCustomGameData, "int OGE_GetCustomGameData(string &in, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetAppGameData, "int OGE_GetAppGameData()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CopyGameData, "void OGE_CopyGameData(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlayBgMusic, "int OGE_PlayBgMusic(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlayMusic, "int OGE_PlayMusic(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopMusic, "int OGE_StopMusic()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PauseMusic, "int OGE_PauseMusic()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ResumeMusic, "int OGE_ResumeMusic()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlayMusicByName, "int OGE_PlayMusicByName(string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FadeInMusic, "int OGE_FadeInMusic(string &in, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FadeOutMusic, "int OGE_FadeOutMusic(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSound, "int OGE_GetSound(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlaySound, "int OGE_PlaySound(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlaySoundById, "int OGE_PlaySoundById(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlaySoundByCode, "int OGE_PlaySoundByCode(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlaySoundByName, "int OGE_PlaySoundByName(string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopSoundById, "int OGE_StopSoundById(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopSoundByCode, "int OGE_StopSoundByCode(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopSoundByName, "int OGE_StopSoundByName(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopAudio, "int OGE_StopAudio()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PlayMovie, "int OGE_PlayMovie(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_StopMovie, "int OGE_StopMovie()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsPlayingMovie, "bool OGE_IsPlayingMovie()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSoundVolume, "int OGE_GetSoundVolume()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetMusicVolume, "int OGE_GetMusicVolume()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSoundVolume, "void OGE_SetSoundVolume(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetMusicVolume, "void OGE_SetMusicVolume(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFullScreen, "bool OGE_GetFullScreen()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFullScreen, "void OGE_SetFullScreen(bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetScene, "int OGE_GetScene()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetActiveScene, "int OGE_GetActiveScene()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLastActiveScene, "int OGE_GetLastActiveScene()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetNextActiveScene, "int OGE_GetNextActiveScene()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneByName, "int OGE_GetSceneByName(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindScene, "int OGE_FindScene(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneName, "const string& OGE_GetSceneName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneChapter, "const string& OGE_GetSceneChapter(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneChapter, "void OGE_SetSceneChapter(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneTag, "const string& OGE_GetSceneTag(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneTag, "void OGE_SetSceneTag(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneGameData, "int OGE_GetSceneGameData(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneBgMusic, "int OGE_SetSceneBgMusic(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneBgMusic, "const string& OGE_GetSceneBgMusic(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneTime, "int OGE_GetSceneTime()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CallScene, "int OGE_CallScene(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneFadeIn, "int OGE_SetSceneFadeIn(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneFadeOut, "int OGE_SetSceneFadeOut(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetSceneFadeMask, "int OGE_SetSceneFadeMask(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroup, "int OGE_GetGroup(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupByCode, "int OGE_GetGroupByCode(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupName, "const string& OGE_GetGroupName(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenGroup, "void OGE_OpenGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseGroup, "void OGE_CloseGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GroupGenSpr, "int OGE_GroupGenSpr(int, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRootSprFromGroup, "int OGE_GetRootSprFromGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFreeSprFromGroup, "int OGE_GetFreeSprFromGroup(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFreeSprWithType, "int OGE_GetFreeSprWithType(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFreeSprWithClassTag, "int OGE_GetFreeSprWithClassTag(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFreeSprWithTag, "int OGE_GetFreeSprWithTag(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupSprCount, "int OGE_GetGroupSprCount(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupSprByIndex, "int OGE_GetGroupSprByIndex(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupSpr, "int OGE_GetGroupSpr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneSprCount, "int OGE_GetSceneSprCount(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneSprByIndex, "int OGE_GetSceneSprByIndex(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupCount, "int OGE_GetGroupCount()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetGroupByIndex, "int OGE_GetGroupByIndex(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneCount, "int OGE_GetSceneCount()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSceneByIndex, "int OGE_GetSceneByIndex(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprForScene, "int OGE_GetSprForScene(int, string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSprForGroup, "int OGE_GetSprForGroup(int, string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveSprFromScene, "int OGE_RemoveSprFromScene(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveSprFromGroup, "int OGE_RemoveSprFromGroup(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveScene, "void OGE_RemoveScene(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveGroup, "void OGE_RemoveGroup(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveGameData, "void OGE_RemoveGameData(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveGameMap, "void OGE_RemoveGameMap(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveImage, "void OGE_RemoveImage(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveMusic, "void OGE_RemoveMusic(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveMovie, "void OGE_RemoveMovie(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveFont, "void OGE_RemoveFont(string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetStaticBuf, "int OGE_GetStaticBuf(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ClearBuf, "int OGE_ClearBuf(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CopyBuf, "int OGE_CopyBuf(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufByte, "int8 OGE_GetBufByte(int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufInt, "int OGE_GetBufInt(int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufFloat, "double OGE_GetBufFloat(int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBufStr, "string OGE_GetBufStr(int, int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutBufByte, "int OGE_PutBufByte(int, int, int8)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutBufInt, "int OGE_PutBufInt(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutBufFloat, "int OGE_PutBufFloat(int, int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutBufStr, "int OGE_PutBufStr(int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenLocalServer, "int OGE_OpenLocalServer(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseLocalServer, "int OGE_CloseLocalServer()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ConnectServer, "int OGE_ConnectServer(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DisconnectServer, "int OGE_DisconnectServer(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_DisconnectClient, "int OGE_DisconnectClient(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetSocketAddr, "const string& OGE_GetSocketAddr(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetCurrentSocket, "int OGE_GetCurrentSocket()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetUserSocket, "int OGE_GetUserSocket(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetUserSocket, "void OGE_SetUserSocket(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvDataSize, "int OGE_GetRecvDataSize()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvDataType, "int OGE_GetRecvDataType()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvByte, "int8 OGE_GetRecvByte(int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvInt, "int OGE_GetRecvInt(int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvFloat, "double OGE_GetRecvFloat(int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvBuf, "int OGE_GetRecvBuf(int, int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetRecvStr, "string OGE_GetRecvStr(int, int, int &out)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutSendingByte, "int OGE_PutSendingByte(int, int, int8)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutSendingInt, "int OGE_PutSendingInt(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutSendingFloat, "int OGE_PutSendingFloat(int, int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutSendingBuf, "int OGE_PutSendingBuf(int, int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_PutSendingStr, "int OGE_PutSendingStr(int, int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SendData, "int OGE_SendData(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLocalIP, "const string& OGE_GetLocalIP()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindClient, "int OGE_FindClient(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FindServer, "int OGE_FindServer(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetLocalServer, "int OGE_GetLocalServer()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FileSize, "int OGE_FileSize(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FileExists, "bool OGE_FileExists(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CreateEmptyFile, "bool OGE_CreateEmptyFile(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RemoveFile, "bool OGE_RemoveFile(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenFile, "int OGE_OpenFile(string &in, bool)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseFile, "int OGE_CloseFile(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SaveFile, "int OGE_SaveFile(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFileEof, "bool OGE_GetFileEof(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFileReadPos, "int OGE_GetFileReadPos(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFileReadPos, "int OGE_SetFileReadPos(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFileReadPosFromEnd, "int OGE_SetFileReadPosFromEnd(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFileWritePos, "int OGE_GetFileWritePos(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFileWritePos, "int OGE_SetFileWritePos(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SetFileWritePosFromEnd, "int OGE_SetFileWritePosFromEnd(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadFileByte, "int8 OGE_ReadFileByte(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadFileInt, "int OGE_ReadFileInt(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadFileFloat, "double OGE_ReadFileFloat(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadFileBuf, "int OGE_ReadFileBuf(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadFileStr, "string OGE_ReadFileStr(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteFileByte, "int OGE_WriteFileByte(int, int8)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteFileInt, "int OGE_WriteFileInt(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteFileFloat, "int OGE_WriteFileFloat(int, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteFileBuf, "int OGE_WriteFileBuf(int, int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteFileStr, "int OGE_WriteFileStr(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenCustomConfig, "int OGE_OpenCustomConfig(string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseCustomConfig, "int OGE_CloseCustomConfig(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetDefaultConfig, "int OGE_GetDefaultConfig()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetAppConfig, "int OGE_GetAppConfig()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadConfigStr, "string OGE_ReadConfigStr(int, string &in, string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadConfigInt, "int OGE_ReadConfigInt(int, string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_ReadConfigFloat, "double OGE_ReadConfigFloat(int, string &in, string &in, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteConfigStr, "bool OGE_WriteConfigStr(int, string &in, string &in, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteConfigInt, "bool OGE_WriteConfigInt(int, string &in, string &in, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_WriteConfigFloat, "bool OGE_WriteConfigFloat(int, string &in, string &in, double)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SaveConfig, "bool OGE_SaveConfig(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SaveDataToConfig, "int OGE_SaveDataToConfig(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LoadDataFromConfig, "int OGE_LoadDataFromConfig(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_SaveDataToDB, "int OGE_SaveDataToDB(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_LoadDataFromDB, "int OGE_LoadDataFromDB(int, int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_IsValidDBType, "bool OGE_IsValidDBType(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenDefaultDB, "int OGE_OpenDefaultDB()");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenDB, "int OGE_OpenDB(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseDB, "void OGE_CloseDB(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_RunSql, "int OGE_RunSql(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_OpenQuery, "int OGE_OpenQuery(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_CloseQuery, "void OGE_CloseQuery(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_FirstRecord, "int OGE_FirstRecord(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_NextRecord, "int OGE_NextRecord(int)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetBoolFieldValue, "bool OGE_GetBoolFieldValue(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetIntFieldValue, "int OGE_GetIntFieldValue(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetFloatFieldValue, "double OGE_GetFloatFieldValue(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetStrFieldValue, "string OGE_GetStrFieldValue(int, string &in)");
    if(flags[i++]<'1') pScripter->RegisterFunction((void*)OGE_GetTimeFieldValue, "string OGE_GetTimeFieldValue(int, string &in)");




    // end of register

}

#ifdef __OGE_AS_TOOL__

int OGE_HandleInputScript(const std::string& sInputConfigFile)
{
    if(!g_script) return -1;

    CogeIniFile ini;
    if(!ini.Load(sInputConfigFile)) return -1;

    std::string sCurrentPath = ini.ReadString("Input", "CurrentPath", "");
    if(sCurrentPath.length() > 0) ini.SetCurrentPath(sCurrentPath);
    else ini.SetCurrentPath(OGE_GetAppMainDir());

    std::string sLogFile = ini.ReadString("Output", "LogFile", "");
    if(sLogFile.length() > 0)
    {
        int iFileHandle = (int) freopen( sLogFile.c_str(), "w", stdout );
        if(iFileHandle == 0)
        {
            printf("Fail to open log file: %s \n", sLogFile.c_str());
            return -1;
        }
    }

    std::string sAction = ini.ReadString("Input", "Action", "Check");

    std::string sConstFile = ini.ReadString("Input", "ConstInt", "");
    if(sConstFile.length() > 0) OGE_RegisterScriptConstInts(g_script, sConstFile);

    int iFileCount = ini.ReadInteger("Input", "FileCount", 0);

    if(iFileCount <= 0) return -1;

    if(sAction.compare("Check") == 0)
    {
        for(int i=1; i<=iFileCount; i++)
        {
            std::string sIdx = "File" + OGE_itoa(i);
            std::string sFileName = ini.ReadPath("Input", sIdx, "");
            if(sFileName.length() > 0)
            {
                CogeScript* p = g_script->GetScript(sIdx, sFileName);
                if(p) printf("Validated script file: %s \n", sFileName.c_str());
                else printf("Fail to validate script file: %s \n", sFileName.c_str());
            }
        }
    }
    else if(sAction.compare("Parse") == 0)
    {
        std::string sReportFile = ini.ReadString("Output", "ReportFile", "");
        if(sReportFile.length() <= 0) return -1;

        CogeIniFile iniReport;
        if(!iniReport.Rewrite(sReportFile)) return -1;

        std::string sScriptName = "File1";
        std::string sFileName = ini.ReadPath("Input", sScriptName, "");
        if(sFileName.length() <= 0) return -1;

        CogeScript* p = g_script->GetScript(sScriptName, sFileName);

        if(p)
        {
            iniReport.WriteInteger("Error", "Count", 0);

            int iVarCount = p->GetVarCount();
            iniReport.WriteInteger("Variable", "Count", iVarCount);
            for(int i=0; i<iVarCount; i++)
            {
                std::string sName = p->GetVarNameByIndex(i);
                iniReport.WriteString("Variable", "Var"+OGE_itoa(i+1), sName);
            }

            int iFunctionCount = p->GetFunctionCount();
            iniReport.WriteInteger("Function", "Count", iFunctionCount);
            for(int i=0; i<iFunctionCount; i++)
            {
                std::string sName = p->GetFunctionNameByIndex(i);
                iniReport.WriteString("Function", "Func"+OGE_itoa(i+1), sName);
            }
        }
        else
        {
            FILE *stream;
            if( (stream = fopen(sLogFile.c_str(), "r" )) != NULL )
            {
                //int iMaxLineLen = 256;
                int iLineIdx = 0;

                char line[_OGE_MAX_LONG_TEXT_LEN_];
                std::string sLine = "";
                while( !feof( stream ) )
                {
                    if( fgets( line, _OGE_MAX_LONG_TEXT_LEN_, stream ) != NULL )
                    {
                        iLineIdx++;
                        sLine = line;
                        iniReport.WriteString("Error", "Error"+OGE_itoa(iLineIdx), sLine);
                    }
                }

                iniReport.WriteInteger("Error", "Count", iLineIdx);
            }
        }

        iniReport.Save();

    }
    else if(sAction.compare("Compile") == 0)
    {
        // compile ...

        CogeBufferManager* memory = new CogeBufferManager();

        int iBufSize = 1024 * 1024 * ini.ReadInteger("Output", "Buffer", 1);

        CogeBuffer* buf = memory->GetFreeBuffer(iBufSize);
        if(buf != NULL && buf->GetState() >= 0)
        {
            buf->Hire();

            for(int i=1; i<=iFileCount; i++)
            {
                std::string sIdx = "File" + OGE_itoa(i);
                std::string sFileName = ini.ReadPath("Input", sIdx, "");
                std::string sBinFileName = sFileName + ".bin";
                if(sFileName.length() > 0)
                {
                    CogeScript* code = g_script->GetScript(sIdx, sFileName);

                    if(code)
                    {
                        printf("Validated script file: %s \n", sFileName.c_str());

                        char* pFileData = buf->GetBuffer();

                        int iRealWrite = code->SaveBinaryCode(pFileData, iBufSize);
                        if(iRealWrite > 0)
                        {
                            std::ofstream ofile(sBinFileName.c_str(), std::ios::out | std::ios::binary);
                            if (!ofile.fail())
                            {
                                ofile.write(pFileData, iRealWrite);
                                ofile.close();
                                printf("Compiled script file: %s \n", sBinFileName.c_str());
                            }
                        }
                    }
                    else printf("Fail to validate script file: %s \n", sFileName.c_str());
                }
            }

            buf->Fire();
        }


        if (memory) delete memory;
    }

    return 0;
}

int OGE_PackInputFiles(const std::string& sInputConfigFile)
{
    CogeIniFile ini;
    if(!ini.Load(sInputConfigFile)) return -1;

    //ini.Show();

    //CogeScript* code = g_script->GetScript("SAMPLE_APP", "test.as");
    //code->CallFunction("App_Run");


    std::string sCurrentPath = ini.ReadString("Input", "CurrentPath", "");
    if(sCurrentPath.length() > 0) ini.SetCurrentPath(sCurrentPath);
    else ini.SetCurrentPath(OGE_GetAppMainDir());

    std::string sLogFile = ini.ReadString("Output", "LogFile", "");
    if(sLogFile.length() > 0)
    {
        int foo = (int) freopen( sLogFile.c_str(), "w", stdout );
        if(foo == 0)
        {
            printf("Fail to open log file: %s \n", sLogFile.c_str());
            return -1;
        }
    }

    std::string sAction = ini.ReadString("Input", "Action", "Pack");

    bool bNeedEncryptData = ini.ReadInteger("Input", "Encrypt", 0) != 0;

    int iFileCount = ini.ReadInteger("Input", "FileCount", 0);

    if(iFileCount <= 0) return -1;

    if(sAction.compare("Pack") == 0)
    {
        std::string sPackFile = ini.ReadString("Output", "PackFile", "");
        if(sPackFile.length() == 0)
        {
            printf("Fail to get output file in config file: %s \n", sInputConfigFile.c_str());
            return -1;
        }

        std::map<std::string, std::string> files;

        for(int i=1; i<=iFileCount; i++)
        {
            std::string sIdx = OGE_itoa(i);
            std::string sFileIdx = "File" + sIdx;
            std::string sNameIdx = "FileName" + sIdx;
            std::string sFileName = ini.ReadPath("Input", sFileIdx, "");
            std::string sSaveFileName = ini.ReadString("Input", sNameIdx, "");

            if(sFileName.length() > 0 && sSaveFileName.length() > 0)
                files.insert(std::map<std::string, std::string>::value_type(sFileName, sSaveFileName));

        }

        if(files.size() > 0)
        {
            CogeBufferManager* memory = new CogeBufferManager();
            CogePacker* packer = new CogePacker(memory);

            packer->PackFiles(files, sPackFile, bNeedEncryptData);

            delete packer;
            delete memory;

            printf("Packed %d files in to output file: %s \n", files.size(), sPackFile.c_str());
        }

    }
    else if(sAction.compare("Unpack") == 0)
    {
        // unpack ...

    }

    return 0;

}

#else

int OGE_HandleInputScript(const std::string& sInputConfigFile)
{
    return 0;
}

int OGE_PackInputFiles(const std::string& sInputConfigFile)
{
    return 0;
}

#endif

void OGE_Run()
{
    std::string sAppCmd = OGE_GetAppGameCmd();
    if(sAppCmd.compare(_OGE_APP_CMD_GAME_) == 0
       || sAppCmd.compare(_OGE_APP_CMD_DEBUG_) == 0
       || sAppCmd.compare(_OGE_APP_CMD_SCENE_) == 0)
    {
        if(g_init && g_engine) g_engine->Run();
    }
    else if (sAppCmd.compare(_OGE_APP_CMD_SCRIPT_) == 0) OGE_HandleInputScript(OGE_GetAppInputFile());
    else if (sAppCmd.compare(_OGE_APP_CMD_PACK_) == 0) OGE_PackInputFiles(OGE_GetAppInputFile());
}

void OGE_Fini()
{
    if(g_engine)
    {
        delete g_engine;
        g_engine = NULL;
    }

    if(g_script)
    {
        delete g_script;
        g_script = NULL;
    }

    g_init = false;


/*
#if defined(__WIN32__)
    std::string sAppCmd = OGE_GetAppGameCmd();
    if(sAppCmd.compare(_OGE_APP_CMD_DEBUG_) == 0 || sAppCmd.compare(_OGE_APP_CMD_SCENE_) == 0)
    {
        printf("Press any key to quit ... \n\n");
        //getchar();
        system("PAUSE");
    }
#endif
*/

}

void OGE_ResetStdOutput()
{

#if defined(__WIN32__) && !defined(__WINCE__)

    freopen( "CON", "w", stdout );

    //freopen( "CON", "r", stdin  );

#elif defined(__LINUX__)

    //freopen( "/dev/console", "w", stdout );
    //freopen( "/dev/tty", "w", stdout );

    //freopen( "/dev/console", "r", stdin );

    return; // seems no need to do anything ...

#else

    return;

#endif

}

int OGE_GetParamCount()
{
    return OGE_GetAppParamCount();
}

char** OGE_GetParams()
{
    return OGE_GetAppParams();
}

const std::string& OGE_GetMainDir()
{
    return OGE_GetAppMainDir();
}

std::string OGE_GetMainPath()
{
    return OGE_GetAppMainPath();
}

const std::string& OGE_GetGameDir()
{
    return OGE_GetAppGameDir();
}

std::string OGE_GetGamePath()
{
    return OGE_GetAppGamePath();
}

const std::string& OGE_GetDebugServerAddr()
{
    return OGE_GetAppDebugServerAddr();
}

int OGE_GetWindowID()
{
    char* pValue = getenv("SDL_WINDOWID");
    int iRsl = -1;
    if (pValue)
    {
        iRsl = atoi(pValue);
        delete pValue;
    }
    return iRsl;
}

CogeEngine* OGE_GetEngine()
{
    return g_engine;
}

CogeVideo* OGE_GetVideo()
{
    return g_engine->GetVideo();
}

CogeImage* OGE_GetScreen()
{
    return g_engine->GetScreen();
}


// api for script ...

void OGE_Print(const std::string& sInfo)
{
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "OGE2D", sInfo.c_str());
#else
    printf("%s", sInfo.c_str());
    fflush(stdout);
#endif

#ifdef __LOG_TO_FILE__
    std::string sLogFile = OGE_GetAppGameDir() + "/" + _OGE_DF_LOG_FILE_;
    OGE_AppendTextToFile(sLogFile.c_str(), sInfo.c_str());
#endif

}

std::string OGE_IntToStr(int iValue)
{
    //const asCScriptString& s = asCScriptString(OGE_itoa(iValue));
    return OGE_itoa(iValue);
}
int  OGE_StrToInt(const std::string& sValue)
{
    std::string s = sValue;
    OGE_StrTrim(s);
    return atoi(s.c_str());
}

int  OGE_HexToInt(std::string& sValue)
{
    std::string s = sValue;
    OGE_StrTrim(s);
    return OGE_htoi(s);
}

std::string OGE_FloatToStr(double fValue)
{
    return OGE_ftoa(fValue);
}
double  OGE_StrToFloat(const std::string& sValue)
{
    std::string s = sValue;
    OGE_StrTrim(s);
    return atof(s.c_str());
}

int OGE_StrLen(const std::string& sText)
{
    return sText.length();
}
int OGE_FindStr(const std::string& sText, const std::string& sSubText, int iStartPos)
{
    return sText.find(sSubText, iStartPos);
}
std::string OGE_CopyStr(const std::string& sText, int iStartPos, int iCount)
{
    if(iCount < 0) return sText.substr(iStartPos);
    else return sText.substr(iStartPos, iCount);
}
std::string OGE_DeleteStr(const std::string& sText, int iStartPos, int iCount)
{
    std::string s = sText;
    if(iCount < 0) return s.erase(iStartPos);
    else return s.erase(iStartPos, iCount);
}

std::string OGE_ReplaceStr(const std::string& sText, const std::string& sOldSubText, const std::string& sNewSubText)
{
	std::string s = sText;
	OGE_StrReplace(s, sOldSubText.c_str(), sNewSubText.c_str());
	return s;
}

std::string OGE_TrimStr(const std::string& sText)
{
    std::string s = sText;
    OGE_StrTrim(s);
    return s;
}

int  OGE_GetTicks()
{
    return SDL_GetTicks();
}

int  OGE_GetOS()
{
    return g_engine->GetOS();
}

std::string OGE_GetDateTimeStr()
{
    return OGE_GetCurrentTimeStr();
}
bool OGE_GetDateTime(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    return OGE_GetCurrentTime(year, month, day, hour, min, sec);
}

void OGE_Delay(int iMilliSeconds)
{
    SDL_Delay(iMilliSeconds);
}

int  OGE_Random(int iMax)
{
    return rand()%iMax;
}
double OGE_RandomFloat()
{
    return rand()/double(RAND_MAX);
}

bool OGE_LibraryExists(const std::string& sLibraryName)
{
    return g_engine->LibraryExists(sLibraryName);
}
bool OGE_ServiceExists(const std::string& sLibraryName, const std::string& sServiceName)
{
    return g_engine->ServiceExists(sLibraryName, sServiceName);
}

int OGE_CallService(const std::string& sLibraryName, const std::string& sServiceName, int iParam)
{
    return g_engine->CallService(sLibraryName, sServiceName, (void*)iParam);
}

const std::string& OGE_GetGameMainDir()
{
    return OGE_GetAppGameDir();
}

int OGE_MakeColor(int iRed, int iGreen, int iBlue)
{
    return ((iRed<< 16) & 0x00ff0000) | ((iGreen << 8) & 0x0000ff00) | (iBlue&0xff);
}

int OGE_GetImage(const std::string& sName)
{
    return (int)g_engine->GetImage(sName);
}

int OGE_LoadImage(const std::string& sName,
                  int iWidth, int iHeight, int iColorKeyRGB,
                  const std::string& sFileName)
{
    bool bLoadAlphaChannel = false;
    if(sFileName.find_last_of(".png") != std::string::npos) bLoadAlphaChannel = true;
    return (int)g_engine->GetVideo()->GetImage(sName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, false, sFileName);
}

int OGE_GetImageWidth(int iImageId)
{
    return ((CogeImage*)iImageId)->GetWidth();
}
int OGE_GetImageHeight(int iImageId)
{
    return ((CogeImage*)iImageId)->GetHeight();
}

int OGE_GetImageRect(int iImageId)
{
    g_rect.x = 0;
    g_rect.y = 0;
    g_rect.w = ((CogeImage*)iImageId)->GetWidth();
    g_rect.h = ((CogeImage*)iImageId)->GetHeight();

    return (int) (&g_rect);
}

int OGE_GetRect(int x, int y, int w, int h)
{
    g_rect.x = x;
    g_rect.y = y;
    g_rect.w = w;
    g_rect.h = h;

    return (int) (&g_rect);
}

/*
void OGE_DrawImage(int iDstImageId, int x, int y, int iSrcImageId)
{
    ((CogeImage*)iDstImageId)->Draw((CogeImage*)iSrcImageId, x, y);
}

void OGE_DrawImageWithEffect(int iDstImageId, int x, int y, int iSrcImageId, int iEffectType, double fEffectValue)
{
    CogeImage* pDstImage = (CogeImage*) iDstImageId;
    CogeImage* pSrcImage = (CogeImage*) iSrcImageId;

    int iEffectValue = (int)fEffectValue;

    switch(iEffectType)
    {
    case Effect_Edge:
        pDstImage->BltWithEdge(pSrcImage, iEffectValue, x, y);
    break;

    case Effect_RGB:
        pDstImage->BltChangedRGB(pSrcImage, iEffectValue, x, y);
    break;

    case Effect_Color:
    {
        uint32_t iRealColor =  iEffectValue & 0x00ffffff;
        uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
        pDstImage->BltWithColor(pSrcImage, iRealColor, iRealAlpha, x, y);
    }
    break;

    case Effect_Rota:
        pDstImage->BltRotate( pSrcImage, iEffectValue,
        x, y, 0, 0, pSrcImage->GetWidth(), pSrcImage->GetHeight() );
    break;

    case Effect_Wave:
        pDstImage->BltWave( pSrcImage, iEffectValue,
        x, y, 0, 0, pSrcImage->GetWidth(), pSrcImage->GetHeight() );
    break;

    case Effect_Lightness:
        pDstImage->BltWithLightness( pSrcImage, iEffectValue,
        x, y, 0, 0, pSrcImage->GetWidth(), pSrcImage->GetHeight() );
    break;

    case Effect_Scale:
    {
        int iWidth = pSrcImage->GetWidth();
        int iHeight = pSrcImage->GetHeight();
        int iDrawWidth  = lround(iWidth * fEffectValue);
        int iDrawHeight = lround(iHeight * fEffectValue);
        int iDrawX      = x + (iWidth >> 1) - (iDrawWidth  >> 1);
        int iDrawY      = y + (iHeight >> 1) - (iDrawHeight >> 1);
        pDstImage->BltStretch( pSrcImage,
        0,  0,  iWidth,  iHeight,
        iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
    }
    break;

    case Effect_Alpha:
        pDstImage->BltAlphaBlend( pSrcImage, iEffectValue,
        x, y, 0, 0, pSrcImage->GetWidth(), pSrcImage->GetHeight() );
    break;
    }
}
*/

void OGE_Draw(int iDstImageId, int x, int y, int iSrcImageId, int iRectId)
{
    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iDstImageId)->Draw((CogeImage*)iSrcImageId, x, y, pRect->x, pRect->y, pRect->w, pRect->h);
}
void OGE_DrawWithEffect(int iDstImageId, int x, int y, int iSrcImageId, int iRectId, int iEffectType, double fEffectValue)
{
    CogeImage* pDstImage = (CogeImage*) iDstImageId;
    CogeImage* pSrcImage = (CogeImage*) iSrcImageId;

    SDL_Rect* pRect = (SDL_Rect*) iRectId;

    int iEffectValue = (int)fEffectValue;

    switch(iEffectType)
    {
    case Effect_Edge:
        pDstImage->BltWithEdge( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;

    case Effect_RGB:
        pDstImage->BltChangedRGB( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;

    case Effect_Color:
    {
        uint32_t iRealColor =  iEffectValue & 0x00ffffff;
        uint32_t iRealAlpha = (iEffectValue & 0xff000000) >> 24;
        pDstImage->BltWithColor( pSrcImage, iRealColor, iRealAlpha,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    }
    break;

    case Effect_Rota:
        pDstImage->BltRotate( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;

    case Effect_Wave:
        pDstImage->BltWave( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;

    case Effect_Lightness:
        pDstImage->BltWithLightness( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;

    case Effect_Scale:
    {
        int iWidth = pRect->w;
        int iHeight = pRect->h;
        int iDrawWidth  = lround(iWidth * fEffectValue);
        int iDrawHeight = lround(iHeight * fEffectValue);
        int iDrawX      = x + (iWidth >> 1) - (iDrawWidth  >> 1);
        int iDrawY      = y + (iHeight >> 1) - (iDrawHeight >> 1);
        pDstImage->BltStretch( pSrcImage,
        pRect->x,  pRect->y,  pRect->x+iWidth,  pRect->y+iHeight,
        iDrawX,  iDrawY, iDrawX + iDrawWidth,  iDrawY + iDrawHeight);
    }
    break;

    case Effect_Alpha:
        pDstImage->BltAlphaBlend( pSrcImage, iEffectValue,
        x, y, pRect->x, pRect->y, pRect->w, pRect->h );
    break;
    }
}

void OGE_FillRect(int iImageId, int iRGBColor, int iRectId)
{
    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iImageId)->FillRect(iRGBColor, pRect->x, pRect->y, pRect->w, pRect->h);
}

void OGE_FillRectAlpha(int iImageId, int iRGBColor, int iRectId, int iAlpha)
{
    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    CogeImage* pClipboardB = g_engine->GetVideo()->GetClipboardB();
    if(pClipboardB)
    {
        pClipboardB->FillRect(iRGBColor, 0, 0, pRect->w, pRect->h);
        ((CogeImage*)iImageId)->BltAlphaBlend(pClipboardB, iAlpha, pRect->x, pRect->y, 0, 0, pRect->w, pRect->h);
    }
}

void OGE_ShowMousePos(bool bShow)
{
    //if(g_engine)
    if(bShow) g_engine->ShowInfo(Info_MousePos);
    else g_engine->HideInfo(Info_MousePos);
}

/*
void OGE_HideMousePos()
{
    //if(g_engine)
    g_engine->HideInfo(Info_MousePos);
}
*/

void OGE_ShowFPS(bool bShow)
{
    //if(g_engine)
    if(bShow) g_engine->ShowInfo(Info_FPS);
    else g_engine->HideInfo(Info_FPS);
}

/*
void OGE_HideFPS()
{
    //if(g_engine)
    g_engine->HideInfo(Info_FPS);
}
*/

void OGE_LockFPS(int iFPS)
{
    if(iFPS > 0) g_engine->GetVideo()->LockFPS(iFPS);
    else g_engine->GetVideo()->UnlockFPS();
}

/*
void OGE_UnlockFPS()
{
    //if(g_engine)
    g_engine->GetVideo()->UnlockFPS();
}

bool OGE_IsFPSLocked()
{
    return g_engine->GetVideo()->GetLockedFPS() > 0;
}
*/
int  OGE_GetFPS()
{
    return g_engine->GetVideo()->GetFPS();
}
int  OGE_GetLockedFPS()
{
    return g_engine->GetVideo()->GetLockedFPS();
}

void OGE_LockCPS(int iCPS)
{
    if(iCPS > 0) g_engine->LockCPS(iCPS);
    else g_engine->UnlockCPS();
}

/*
void OGE_UnlockCPS()
{
    g_engine->UnlockCPS();
}
bool OGE_IsCPSLocked()
{
    return g_engine->IsCPSLocked();
}
*/

int  OGE_GetCPS()
{
    return g_engine->GetCPS();
}
int  OGE_GetLockedCPS()
{
    return g_engine->GetLockedCPS();
}

void OGE_SetDirtyRectMode(bool bValue)
{
    //if(g_engine)
    g_engine->SetDirtyRectMode(bValue);
}

void OGE_FreezeSprites(bool bValue)
{
    g_engine->SetFreeze(bValue);
}

void OGE_Scroll(int iIncX, int iIncY)
{
    g_engine->ScrollScene(iIncX, iIncY, true);
}
void OGE_SetViewPos(int iLeft, int iTop)
{
    g_engine->SetSceneViewPos(iLeft, iTop, true);
}

int  OGE_GetViewPosX()
{
    return g_engine->GetSceneViewPosX();
}
int  OGE_GetViewPosY()
{
    return g_engine->GetSceneViewPosY();
}

int  OGE_GetViewWidth()
{
    return g_engine->GetSceneViewWidth();
}
int  OGE_GetViewHeight()
{
    return g_engine->GetSceneViewHeight();
}

void OGE_StopAutoScroll()
{
    g_engine->GetCurrentScene()->StopAutoScroll();
}
void OGE_StartAutoScroll(int iStepX, int iStepY, int iInterval, bool bLoop)
{
    g_engine->GetCurrentScene()->StartAutoScroll(true, iStepX, iStepY, iInterval, bLoop);
}

void OGE_SetAutoScrollSpeed(int iStepX, int iStepY, int iInterval)
{
    g_engine->GetCurrentScene()->SetScrollSpeed(iStepX, iStepY, iInterval);
}

void OGE_SetScrollTarget(int iTargetX, int iTargetY)
{
    g_engine->GetCurrentScene()->SetScrollTarget(iTargetX, iTargetY);
}

/*
int OGE_GetScrollX()
{
    return g_engine->GetCurrentScene()->GetScrollX();
}
int OGE_GetScrollY()
{
    return g_engine->GetCurrentScene()->GetScrollY();
}
*/


int  OGE_GetSceneLightMap(int iSceneId)
{
    return (int) ((CogeScene*)iSceneId)->GetLightMap();
}

int OGE_SetSceneLightMap(int iSceneId, int iImageId, int iLightMode)
{
    ((CogeScene*)iSceneId)->SetLightMap((CogeImage*)iImageId, iLightMode);
    return ((CogeScene*)iSceneId)->GetLightMode();
}

int  OGE_SetSceneMap(int iSceneId, const std::string& sNewMap, int iNewLeft, int iNewTop)
{
    int rsl = ((CogeScene*)iSceneId)->SetMap(sNewMap, iNewLeft, iNewTop);
    if(rsl >= 0) return (int) ((CogeScene*)iSceneId)->GetMap();
    else return 0;
}

int  OGE_GetMap()
{
    return (int) g_engine->GetCurrentScene()->GetMap();
}

int  OGE_GetMapByName(const std::string& sMapName)
{
    return (int)g_engine->GetGameMap(sMapName);
}

const std::string& OGE_GetMapName(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetName();
}
int  OGE_GetMapWidth(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetBgWidth();
}
int  OGE_GetMapHeight(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetBgHeight();
}
int  OGE_GetMapImage(int iMapId)
{
    return (int) ((CogeGameMap*)iMapId)->GetBgImage();
}
bool OGE_GetEightDirectionMode(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetEightDirectionMode();
}
void OGE_SetEightDirectionMode(int iMapId, bool value)
{
    ((CogeGameMap*)iMapId)->SetEightDirectionMode(value);
}
int  OGE_GetColumnCount(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetColumnCount();
}
int  OGE_GetRowCount(int iMapId)
{
    return ((CogeGameMap*)iMapId)->GetRowCount();
}
int  OGE_GetTileValue(int iMapId, int iTileX, int iTileY)
{
    return ((CogeGameMap*)iMapId)->GetTileValue(iTileX, iTileY);
}
int  OGE_SetTileValue(int iMapId, int iTileX, int iTileY, int iValue)
{
    return ((CogeGameMap*)iMapId)->SetTileValue(iTileX, iTileY, iValue);
}
void OGE_ReloadTileValues(int iMapId)
{
    ((CogeGameMap*)iMapId)->ReloadTileValues();
}
void OGE_ResetGameMap(int iMapId)
{
    ((CogeGameMap*)iMapId)->Reset();
}
int  OGE_GetTileData(int iMapId, int iTileX, int iTileY)
{
    return ((CogeGameMap*)iMapId)->GetTileData(iTileX, iTileY);
}
int  OGE_SetTileData(int iMapId, int iTileX, int iTileY, int iValue)
{
    return ((CogeGameMap*)iMapId)->SetTileData(iTileX, iTileY, iValue);
}
void OGE_InitTileData(int iMapId, int iInitValue)
{
    ((CogeGameMap*)iMapId)->InitTileData(iInitValue);
}
void OGE_InitTileValues(int iMapId, int iInitValue)
{
    ((CogeGameMap*)iMapId)->InitTileValues(iInitValue);
}
bool OGE_PixelToTile(int iMapId, int iPixelX, int iPixelY, int& iTileX, int& iTileY)
{
    return ((CogeGameMap*)iMapId)->PixelToTile(iPixelX, iPixelY, iTileX, iTileY);
}
bool OGE_TileToPixel(int iMapId, int iTileX, int iTileY, int& iPixelX, int& iPixelY)
{
    return ((CogeGameMap*)iMapId)->TileToPixel(iTileX, iTileY, iPixelX, iPixelY);
}
bool OGE_AlignPixel(int iMapId, int iInPixelX, int iInPixelY, int& iOutPixelX, int& iOutPixelY)
{
    return ((CogeGameMap*)iMapId)->AlignPixel(iInPixelX, iInPixelY, iOutPixelX, iOutPixelY);
}
int  OGE_FindWay(int iMapId, int iStartX, int iStartY, int iEndX, int iEndY, int iTileListId)
{
    return (int)((CogeGameMap*)iMapId)->FindWay(iStartX, iStartY, iEndX, iEndY, (ogePointArray*)iTileListId);
}
int  OGE_FindRange(int iMapId, int iPosX, int iPosY, int iMovementPoints, int iTileListId)
{
    return (int)((CogeGameMap*)iMapId)->FindRange(iPosX, iPosY, iMovementPoints, (ogePointArray*)iTileListId);
}


int  OGE_GetTileListSize(int iTileListId)
{
    return ((ogePointArray*)iTileListId)->size();
}
int  OGE_GetTileXFromList(int iTileListId, int iIndex)
{
    return ((ogePointArray*)iTileListId)->at(iIndex-1).x;
}
int  OGE_GetTileYFromList(int iTileListId, int iIndex)
{
    return ((ogePointArray*)iTileListId)->at(iIndex-1).y;
}
int  OGE_GetTileIndexInList(int iTileListId, int iTileX, int iTileY)
{
    int idx = -1;

    ogePointArray* pList = (ogePointArray*)iTileListId;
    int count = pList->size();

    for(int i=0; i<count; i++)
    {
        if(pList->at(i).x == iTileX && pList->at(i).y == iTileY)
        {
            idx = i;
            break;
        }
    }

    return idx + 1;
}
int  OGE_AddTileToList(int iTileListId, int iTileX, int iTileY)
{
    CogePoint pt;
    pt.x = iTileX; pt.y = iTileY;
    ogePointArray* pPointList = (ogePointArray*)iTileListId;
    pPointList->push_back(pt);
    return pPointList->size();
}
int  OGE_RemoveTileFromList(int iTileListId, int iIndex)
{
    ogePointArray* pPointList = (ogePointArray*)iTileListId;
    pPointList->erase(pPointList->begin()+(iIndex-1));
    return pPointList->size();
}
void OGE_ClearTileList(int iTileListId)
{
    ((ogePointArray*)iTileListId)->clear();
}

int  OGE_GetKey()
{
    return g_engine->GetCurrentKey();
}
void OGE_HoldKeyEvent()
{
    g_engine->StopBroadcastKeyEvent();
}

void OGE_SkipMouseEvent()
{
    g_engine->SkipCurrentMouseEvent();
}

int  OGE_GetScanCode()
{
    return g_engine->GetCurrentScanCode();
}

void OGE_ShowKeyboard(bool bShow)
{
    if(g_engine->GetIM()) g_engine->GetIM()->ShowSoftKeyboard(bShow);
}
bool OGE_IsKeyboardShown()
{
    if(g_engine->GetIM()) return g_engine->GetIM()->IsSoftKeyboardShown();
    else return false;
}

void OGE_SetKeyDown(int iKey)
{
    g_engine->SetKeyDown(iKey);
}
void OGE_SetKeyUp(int iKey)
{
    g_engine->SetKeyUp(iKey);
}

bool OGE_IsKeyDown(int iKey)
{
    return g_engine->IsKeyDown(iKey);
}

bool OGE_IsKeyDownTime(int iKey, int iInterval)
{
    return g_engine->IsKeyDown(iKey, iInterval);
}
int  OGE_GetJoystickCount()
{
    return g_engine->GetJoystickCount();
}

bool OGE_IsJoyKeyDown(int iJoyId, int iKey)
{
    return g_engine->IsJoyKeyDown(iJoyId - 1, iKey);
}
bool OGE_IsJoyKeyDownTime(int iJoyId, int iKey, int iInterval)
{
    return g_engine->IsJoyKeyDown(iJoyId - 1, iKey, iInterval);
}

bool OGE_IsMouseLeftDown()
{
    return g_engine->IsMouseLeftButtonDown();
}
bool OGE_IsMouseRightDown()
{
    return g_engine->IsMouseRightButtonDown();
}
bool OGE_IsMouseLeftUp()
{
    return g_engine->IsMouseLeftButtonUp();
}
bool OGE_IsMouseRightUp()
{
    return g_engine->IsMouseRightButtonUp();
}
int  OGE_GetMouseX()
{
    return g_engine->GetMouseX();
}
int  OGE_GetMouseY()
{
    return g_engine->GetMouseY();
}

int OGE_GetIntVar(int iDataId, int iIndex)
{
    return ((CogeGameData*)iDataId)->GetIntVar(iIndex);
}
void OGE_SetIntVar(int iDataId, int iIndex, int iValue)
{
    ((CogeGameData*)iDataId)->SetIntVar(iIndex, iValue);
}
double OGE_GetFloatVar(int iDataId, int iIndex)
{
    return ((CogeGameData*)iDataId)->GetFloatVar(iIndex);
}
void OGE_SetFloatVar(int iDataId, int iIndex, double fValue)
{
    ((CogeGameData*)iDataId)->SetFloatVar(iIndex, fValue);
}
const std::string& OGE_GetStrVar(int iDataId, int iIndex)
{
    return ((CogeGameData*)iDataId)->GetStrVar(iIndex);
}
void OGE_SetStrVar(int iDataId, int iIndex, const std::string& sValue)
{
    ((CogeGameData*)iDataId)->SetStrVar(iIndex, sValue);
}
int OGE_GetBufVar(int iDataId, int iIndex)
{
    return (int) (((CogeGameData*)iDataId)->GetBufVar(iIndex));
}
void OGE_SetBufVar(int iDataId, int iIndex, const std::string& sValue)
{
    ((CogeGameData*)iDataId)->SetBufVar(iIndex, sValue);
}

int  OGE_GetIntVarCount(int iDataId)
{
    return ((CogeGameData*)iDataId)->GetIntVarCount();
}
void OGE_SetIntVarCount(int iDataId, int iCount)
{
    ((CogeGameData*)iDataId)->SetIntVarCount(iCount);
}
int  OGE_GetFloatVarCount(int iDataId)
{
    return ((CogeGameData*)iDataId)->GetFloatVarCount();
}
void OGE_SetFloatVarCount(int iDataId, int iCount)
{
    ((CogeGameData*)iDataId)->SetFloatVarCount(iCount);
}
int  OGE_GetStrVarCount(int iDataId)
{
    return ((CogeGameData*)iDataId)->GetStrVarCount();
}
void OGE_SetStrVarCount(int iDataId, int iCount)
{
    ((CogeGameData*)iDataId)->SetStrVarCount(iCount);
}
int  OGE_GetBufVarCount(int iDataId)
{
    return ((CogeGameData*)iDataId)->GetBufVarCount();
}
void OGE_SetBufVarCount(int iDataId, int iCount)
{
    ((CogeGameData*)iDataId)->SetBufVarCount(iCount);
}

void OGE_SortIntVar(int iDataId)
{
    ((CogeGameData*)iDataId)->SortInt();
}
void OGE_SortFloatVar(int iDataId)
{
    ((CogeGameData*)iDataId)->SortFloat();
}
void OGE_SortStrVar(int iDataId)
{
    ((CogeGameData*)iDataId)->SortStr();
}

int OGE_FindIntVar(int iDataId, int iValue)
{
    return ((CogeGameData*)iDataId)->IndexOfInt(iValue);
}
int OGE_FindFloatVar(int iDataId, double fValue)
{
    return ((CogeGameData*)iDataId)->IndexOfFloat(fValue);
}
int OGE_FindStrVar(int iDataId, const std::string& sValue)
{
    return ((CogeGameData*)iDataId)->IndexOfStr(sValue);
}

int OGE_SplitStrIntoData(const std::string& sStr, const std::string& sDelim, int iDataId)
{
    CogeGameData* pData = (CogeGameData*)iDataId;
    if(pData == NULL) return 0;

    //OGE_Log("OGE_SplitStrIntoData(): [%s], [%s]", sStr.c_str(), sDelim.c_str());

    std::vector<std::string> arrStr;
    int iCount = OGE_StrSplit(sStr, sDelim, arrStr);
    if(iCount <= 0) return 0;

    pData->SetStrVarCount(iCount);
    //pData->SetStrVar(0, sStr);

    for(int i=1; i<=iCount; i++)
    {
        pData->SetStrVar(i, arrStr[i-1]);
    }

    return iCount;
}

/*
int  OGE_GetGameCurrentScene()
{
    return (int)g_engine->GetCurrentScene();
}
int  OGE_GetGameCurrentSprite()
{
    return (int)g_engine->GetCurrentSprite();
}
*/

int  OGE_GetSprCurrentScene(int iSprId)
{
    return (int) (((CogeSprite*)iSprId)->GetCurrentScene());
}
int  OGE_GetSprCurrentGroup(int iSprId)
{
    return (int) (((CogeSprite*)iSprId)->GetCurrentGroup());
}

int OGE_GetSpr()
{
    return (int)g_engine->GetCurrentSprite();
}

int OGE_GetFocusSpr()
{
    return (int)g_engine->GetFocusSprite();
}

void OGE_SetFocusSpr(int iSprId)
{
    g_engine->SetFocusSprite((CogeSprite*)iSprId);
}

int  OGE_GetModalSpr()
{
    return (int)g_engine->GetModalSprite();
}
void OGE_SetModalSpr(int iSprId)
{
    g_engine->SetModalSprite((CogeSprite*)iSprId);
}

int  OGE_GetMouseSpr()
{
    return (int)g_engine->GetMouseSprite();
}
void OGE_SetMouseSpr(int iSprId)
{
    g_engine->SetMouseSprite((CogeSprite*)iSprId);
}

int  OGE_GetFirstSpr()
{
    return (int)g_engine->GetFirstSprite();
}
void OGE_SetFirstSpr(int iSprId)
{
    g_engine->SetFirstSprite((CogeSprite*)iSprId);
}

int  OGE_GetLastSpr()
{
    return (int)g_engine->GetLastSprite();
}
void OGE_SetLastSpr(int iSprId)
{
    g_engine->SetLastSprite((CogeSprite*)iSprId);
}
int  OGE_GetProxySpr()
{
    return (int)g_engine->GetLastSprite();
}
void OGE_SetProxySpr(int iSprId)
{
    g_engine->SetLastSprite((CogeSprite*)iSprId);
}

/*
void OGE_HideMouseCursor()
{
    g_engine->HideSystemMouseCursor();
}
*/
void OGE_ShowMouseCursor(bool bShow)
{
    if(bShow) g_engine->ShowSystemMouseCursor();
    else g_engine->HideSystemMouseCursor();
}

int  OGE_GetTopZ()
{
    return g_engine->GetTopZ();
}

int  OGE_BringSprToTop(int iSprId)
{
    return g_engine->BringSpriteToTop((CogeSprite*)iSprId);
}

/*
ogeScriptString OGE_GetCurrentSprName()
{
    return NEW_SCRIPT_STR(g_engine->GetCurrentSprite()->GetName());
}
*/

const std::string& OGE_GetSprName(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetName();
}

int  OGE_SetSceneBgMusic(int iSceneId, const std::string& sMusicName)
{
    return ((CogeScene*)iSceneId)->SetBgMusic(sMusicName);
}
const std::string& OGE_GetSceneBgMusic(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetBgMusicName();
}

int OGE_GetSceneTime()
{
    return g_engine->GetSceneTime();
}

void OGE_CallBaseEvent()
{
    g_engine->CallCurrentSpriteBaseEvent();
}

//int OGE_FindSpr(const std::string& sSprName, int iState)
//{
//    if(iState==0) return (int)g_engine->GetCurrentScene()->FindSprite(sSprName);
//    else if(iState>0) return (int)g_engine->GetCurrentScene()->FindActiveSprite(sSprName);
//    else return (int)g_engine->GetCurrentScene()->FindDeactiveSprite(sSprName);
//}

int  OGE_FindGroupSpr(int iGroupId, const std::string& sSprName)
{
    return (int) ((CogeSpriteGroup*)iGroupId)->FindSprite(sSprName);
}

int OGE_FindSceneSpr(int iSceneId, const std::string& sSprName)
{
    return (int) ((CogeScene*)iSceneId)->FindSprite(sSprName);
}

/*
int OGE_FindGameSpr(const std::string& sSprName)
{
    return (int)g_engine->FindSprite(sSprName);
}
*/

int  OGE_GetCaretPos(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetInputterCaretPos();
}

void OGE_ResetCaretPos(int iSprId)
{
    ((CogeSprite*)iSprId)->ResetInputterCaretPos();
}

int OGE_GetInputTextBuffer(int iSprId)
{
    return (int) ((CogeSprite*)iSprId)->GetInputTextBuffer();
}

void OGE_ClearInputTextBuffer(int iSprId)
{
    ((CogeSprite*)iSprId)->ClearInputTextBuffer();
}

int OGE_GetInputCharCount(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetInputTextCharCount();
}
/*
int  OGE_SetInputCharCount(int iSprId, int iSize)
{
    return ((CogeSprite*)iSprId)->SetInputTextCharCount(iSize);
}
*/

std::string OGE_GetInputText(int iSprId)
{
    std::string sText = "";

    CogeSprite* pSprite = (CogeSprite*)iSprId;
    if(pSprite == NULL) return 0;

    char* pInputBuf = pSprite->GetInputTextBuffer();
    if (pInputBuf == NULL) return 0;

    int iInputSize = pSprite->GetInputTextCharCount();

    if(OGE_IsInputUnicode())
    {
        sText = g_engine->UnicodeToString(pInputBuf, iInputSize, "UTF-8");
    }
    else
    {
        char* src = pInputBuf;

        //char dst[iInputSize+1];
        //memcpy((void*)(&dst[0]), (void*)src, iInputSize);
        //dst[iInputSize] = '\0';
        //sText = dst;

        memcpy((void*)(&g_tmpbuf[0]), (void*)src, iInputSize);
        g_tmpbuf[iInputSize] = '\0';
        sText = g_tmpbuf;

        int iLen = OGE_StrToUnicode(sText, g_engine->GetSystemCharset(), (int)g_buf);
        sText = g_engine->UnicodeToString(g_buf, iLen, "UTF-8");
    }

    return sText;

}

int  OGE_SetInputText(int iSprId, const std::string& sText)
{
    int iSize = 0;

    CogeSprite* pSprite = (CogeSprite*)iSprId;
    if(pSprite == NULL) return 0;

    char* pInputBuf = pSprite->GetInputTextBuffer();
    if (pInputBuf == NULL) return 0;

    if(OGE_IsInputUnicode())
    {
        int iLen = OGE_StrToUnicode(sText, "UTF-8", (int)g_buf);
        if(iLen < 4) return 0;

        char* p = &(g_buf[2]);
        memcpy(pInputBuf, p, iLen - 2);

        iSize = iLen - 4;
    }
    else
    {
        int iLen = OGE_StrToUnicode(sText, "UTF-8", (int)g_buf);
        std::string sContent = g_engine->UnicodeToString(g_buf, iLen, g_engine->GetSystemCharset());
        OGE_PutBufStr((int)pInputBuf, 0, sContent);

        iSize = sContent.length();
    }

    pSprite->SetInputTextCharCount(iSize);

    return iSize;
}

void OGE_SetInputWinPos(int iSprId, int iPosX, int iPosY)
{
    ((CogeSprite*)iSprId)->SetInputWinPos(iPosX, iPosY);
}

//int  OGE_GetInputTextWidth(int iSprId)
//{
//    return ((CogeSprite*)iSprId)->GetInputTextWidth();
//}

bool OGE_GetSprActive(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetActive();
}
void OGE_SetSprActive(int iSprId, bool bValue)
{
    //((CogeSprite*)iSprId)->SetActive(bValue);
    if(bValue) ((CogeSprite*)iSprId)->PrepareActive();
    else ((CogeSprite*)iSprId)->PrepareDeactive();
}

bool OGE_GetSprVisible(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetVisible();
}
void OGE_SetSprVisible(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetVisible(bValue);
}

bool OGE_GetSprInput(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetInput();
}
void OGE_SetSprInput(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetInput(bValue);
}

bool OGE_GetSprDrag(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetDrag();
}
void OGE_SetSprDrag(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetDrag(bValue);
}

bool OGE_GetSprCollide(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetCollide();
}
void OGE_SetSprCollide(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetCollide(bValue);
}

bool OGE_GetSprBusy(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetBusy();
}
void OGE_SetSprBusy(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetBusy(bValue);
}

bool OGE_GetSprDefaultDraw(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetDefaultDraw();
}
void OGE_SetSprDefaultDraw(int iSprId, bool bValue)
{
    ((CogeSprite*)iSprId)->SetDefaultDraw(bValue);
}

int  OGE_GetSprLightMap(int iSprId)
{
    return (int) ((CogeSprite*)iSprId)->GetLightMap();
}
void OGE_SetSprLightMap(int iSprId, int iImageId)
{
    ((CogeSprite*)iSprId)->SetLightMap((CogeImage*)iImageId);
}

int OGE_GetRelatedSpr(int iSprId, int idx)
{
    return (int)((CogeSprite*)iSprId)->GetRelatedSpr(idx - 1);
}
void OGE_SetRelatedSpr(int iSprId, int iRelatedSprId, int idx)
{
    ((CogeSprite*)iSprId)->SetRelatedSpr((CogeSprite*)iRelatedSprId, idx - 1);
}

int  OGE_GetRelatedGroup(int iSprId, int idx)
{
    return (int)((CogeSprite*)iSprId)->GetRelatedGroup(idx - 1);
}
void OGE_SetRelatedGroup(int iSprId, int iRelatedGroupId, int idx)
{
    ((CogeSprite*)iSprId)->SetRelatedGroup((CogeSpriteGroup*)iRelatedGroupId, idx - 1);
}


int OGE_GetCollidedSpr(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetCollidedSpr();
}
int OGE_GetSprPlot(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetPlotSpr();
}
void OGE_SetSprPlot(int iSprId, int iPlotSprId)
{
    ((CogeSprite*)iSprId)->SetPlotSpr((CogeSprite*)iPlotSprId);
}
int OGE_GetSprMaster(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetMasterSpr();
}
void OGE_SetSprMaster(int iSprId, int iMasterSprId)
{
    ((CogeSprite*)iSprId)->SetMasterSpr((CogeSprite*)iMasterSprId);
}

int OGE_GetReplacedSpr(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetReplacedSpr();
}
void OGE_SetReplacedSpr(int iSprId, int iReplacedSprId)
{
    ((CogeSprite*)iSprId)->SetReplacedSpr((CogeSprite*)iReplacedSprId);
}

int OGE_GetSprParent(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetParentSpr();
}

int OGE_SetSprParent(int iSprId, int iParentId, int iClientX, int iClientY)
{
    return ((CogeSprite*)iSprId)->SetParent((CogeSprite*)iParentId, iClientX, iClientY);
}

//int  OGE_GetScenePlayerSpr(int iPlayerId)
//{
//    return (int)g_engine->GetScenePlayerSprite(iPlayerId - 1);
//}
//void OGE_SetScenePlayerSpr(int iSprId, int iPlayerId)
//{
//    g_engine->SetScenePlayerSprite((CogeSprite*)iSprId, iPlayerId - 1);
//}

int  OGE_GetSceneSpr(int idx)
{
    return (int)g_engine->GetSceneNpcSprite(idx - 1);
}
/*
void OGE_SetSceneSpr(int iSprId, int idx)
{
    g_engine->SetSceneNpcSprite((CogeSprite*)iSprId, idx - 1);
}
*/

int  OGE_GetDefaultPlayerSpr()
{
    return (int)g_engine->GetSceneDefaultPlayerSpr();
}
void OGE_SetDefaultPlayerSpr(int iSprId)
{
    g_engine->SetSceneDefaultPlayerSpr((CogeSprite*)iSprId);
}

int  OGE_GetPlayerSpr(int iPlayerId)
{
    return (int)g_engine->GetPlayerSprite(iPlayerId - 1);
}
void OGE_SetPlayerSpr(int iSprId, int iPlayerId)
{
    g_engine->SetPlayerSprite((CogeSprite*)iSprId, iPlayerId - 1);
}

int  OGE_GetUserSpr(int idx)
{
    return (int)g_engine->GetUserSprite(idx - 1);
}
void OGE_SetUserSpr(int iSprId, int idx)
{
    g_engine->SetUserSprite((CogeSprite*)iSprId, idx - 1);
}

int  OGE_GetUserGroup(int idx)
{
    return (int)g_engine->GetUserGroup(idx - 1);
}
void OGE_SetUserGroup(int iSprGroupId, int idx)
{
    g_engine->SetUserGroup((CogeSpriteGroup*)iSprGroupId, idx - 1);
}

int  OGE_GetUserPath(int idx)
{
    return (int)g_engine->GetUserPath(idx - 1);
}
void OGE_SetUserPath(int iSprPathId, int idx)
{
    g_engine->SetUserPath((CogePath*)iSprPathId, idx - 1);
}

int  OGE_GetUserImage(int idx)
{
    return (int)g_engine->GetUserImage(idx - 1);
}
void OGE_SetUserImage(int iImageId, int idx)
{
    g_engine->SetUserImage((CogeImage*)iImageId, idx - 1);
}

int  OGE_GetUserData(int idx)
{
    return (int)g_engine->GetUserData(idx - 1);
}
void OGE_SetUserData(int iDataId, int idx)
{
    g_engine->SetUserData((CogeGameData*)iDataId, idx - 1);
}

int  OGE_GetUserObject(int idx)
{
    return (int)g_engine->GetUserObject(idx - 1);
}
void OGE_SetUserObject(int iObjId, int idx)
{
    g_engine->SetUserObject((void*)iObjId, idx - 1);
}

int  OGE_GetSprTileList(int iSprId)
{
    return (int) ((CogeSprite*)iSprId)->GetTileList();
}

void OGE_PushEvent(int iSprId, int iCustomEventId)
{
    if(iCustomEventId < 1 || iCustomEventId > _OGE_MAX_EVENT_COUNT_) return;
    ((CogeSprite*)iSprId)->PushCustomEvent(iCustomEventId - 1);
}

int  OGE_GetSprCurrentImage(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetCurrentImage();
}

int  OGE_SetSprImage(int iSprId, int iImageId)
{
    return ((CogeSprite*)iSprId)->SetImage((CogeImage*)iImageId);
}

int  OGE_GetSprAnimationImg(int iSprId, int iDir, int iAct)
{
    return (int)((CogeSprite*)iSprId)->GetAnimaImage(iDir, iAct);
}

bool OGE_SetSprAnimationImg(int iSprId, int iDir, int iAct, int iImageId)
{
    return ((CogeSprite*)iSprId)->SetAnimaImage(iDir, iAct, (CogeImage*)iImageId);
}

void OGE_MoveX(int iSprId, int iIncX)
{
    ((CogeSprite*)iSprId)->IncX(iIncX);
}
void OGE_MoveY(int iSprId, int iIncY)
{
    ((CogeSprite*)iSprId)->IncY(iIncY);
}

int OGE_SetDirection(int iSprId, int iDir)
{
    return ((CogeSprite*)iSprId)->SetDir(iDir);
}
int OGE_SetAction(int iSprId, int iAct)
{
    return ((CogeSprite*)iSprId)->SetAct(iAct);
}
int  OGE_SetAnimation(int iSprId, int iDir, int iAct)
{
    return ((CogeSprite*)iSprId)->SetAnima(iDir, iAct);
}
void OGE_ResetAnimation(int iSprId)
{
    ((CogeSprite*)iSprId)->ResetAnima();
}

void OGE_DrawSpr(int iSprId)
{
    ((CogeSprite*)iSprId)->DefaultDraw();
}

int OGE_GetPosX(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPosX();
}
int OGE_GetPosY(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPosY();
}
int OGE_GetPosZ(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPosZ();
}

void OGE_SetPosXYZ(int iSprId, int iPosX, int iPosY, int iPosZ)
{
    ((CogeSprite*)iSprId)->SetPos(iPosX, iPosY, iPosZ);
}

void OGE_SetPos(int iSprId, int iPosX, int iPosY)
{
    ((CogeSprite*)iSprId)->SetPos(iPosX, iPosY);
}

void OGE_SetPosZ(int iSprId, int iPosZ)
{
    ((CogeSprite*)iSprId)->SetPosZ(iPosZ);
}

void OGE_SetRelPos(int iSprId, int iRelativeX, int iRelativeY)
{
    ((CogeSprite*)iSprId)->SetRelativePos(iRelativeX, iRelativeY);
}

int OGE_GetRelPosX(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetRelativeX();
}
int OGE_GetRelPosY(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetRelativeY();
}

bool OGE_GetRelMode(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetRelativePosMode();
}
void OGE_SetRelMode(int iSprId, bool bEnable)
{
    ((CogeSprite*)iSprId)->SetRelativePosMode(bEnable);
}

bool OGE_HasEffect(int iSprId, int iEffectType)
{
    return ((CogeSprite*)iSprId)->HasEffect(iEffectType);
}
bool OGE_IsEffectCompleted(int iSprId, int iEffectType)
{
    return ((CogeSprite*)iSprId)->IsEffectCompleted(iEffectType);
}
double OGE_GetEffectValue(int iSprId, int iEffectType)
{
    return ((CogeSprite*)iSprId)->GetEffectValue(iEffectType);
}

double OGE_GetEffectIncrement(int iSprId, int iEffectType)
{
    return ((CogeSprite*)iSprId)->GetEffectStepValue(iEffectType);
}

int  OGE_GetSprRootX(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetAnimaRootX();
}
int  OGE_GetSprRootY(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetAnimaRootY();
}
int  OGE_GetSprWidth(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetAnimaWidth();
}
int  OGE_GetSprHeight(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetAnimaHeight();
}

void OGE_SetSprDefaultRoot(int iSprId, int iRootX, int iRootY)
{
    ((CogeSprite*)iSprId)->SetDefaultRoot(iRootX, iRootY);
}
void OGE_SetSprDefaultSize(int iSprId, int iWidth, int iHeight)
{
    ((CogeSprite*)iSprId)->SetDefaultSize(iWidth, iHeight);
}

int OGE_GetDirection(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetDir();
}
int OGE_GetAction(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetAct();
}

int  OGE_GetSprType(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetType();
}

int  OGE_GetSprClassTag(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetClassTag();
}
int  OGE_GetSprTag(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetObjectTag();
}
void OGE_SetSprTag(int iSprId, int iTag)
{
    ((CogeSprite*)iSprId)->SetObjectTag(iTag);
}

int  OGE_GetSprChildCount(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetChildCount();
}
int  OGE_GetSprChildByIndex(int iSprId, int iIndex)
{
    return (int) ((CogeSprite*)iSprId)->GetChildByIndex(iIndex - 1);
}

int OGE_AddEffect(int iSprId, int iEffectType, double fEffectValue)
{
    return ((CogeSprite*)iSprId)->AddAnimaEffect(iEffectType, fEffectValue);
}
int OGE_AddEffectEx(int iSprId, int iEffectType, double fStart, double fEnd, double fIncrement, int iStepInterval, int iRepeat)
{
    return ((CogeSprite*)iSprId)->AddAnimaEffectEx(iEffectType, fStart, fEnd, fIncrement, iStepInterval, iRepeat, false);
}
int OGE_RemoveEffect(int iSprId, int iEffectType)
{
    return ((CogeSprite*)iSprId)->RemoveAnimaEffect(iEffectType, false);
}

int  OGE_GetCurrentPath(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetCurrentPath();
}
int  OGE_GetLocalPath(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetLocalPath();
}
int  OGE_UseLocalPath(int iSprId, int iStep, bool bAutoStepping)
{
    return ((CogeSprite*)iSprId)->UseLocalPath(iStep, 0, -1, bAutoStepping);
}
int OGE_UsePath(int iSprId, int iPathId, int iStep, bool bAutoStepping)
{
    return ((CogeSprite*)iSprId)->UsePath((CogePath*)iPathId, iStep, 0, -1, bAutoStepping);
}
int OGE_ResetPath(int iSprId, int iStep, bool bAutoStepping)
{
    return ((CogeSprite*)iSprId)->ResetPath(iStep, 0, -1, bAutoStepping);
}
int OGE_NextPathStep(int iSprId)
{
    return ((CogeSprite*)iSprId)->NextPathStep();
}
void OGE_AbortPath(int iSprId)
{
    return ((CogeSprite*)iSprId)->AbortPath();
}
int  OGE_GetPathNodeIndex(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetCurrentStepIndex();
}
int OGE_GetPathStepLength(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPathStepLength();
}
void OGE_SetPathStepLength(int iSprId, int iStepLength)
{
    ((CogeSprite*)iSprId)->SetPathStepLength(iStepLength);
}

int OGE_GetPathStepInterval(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPathStepInterval();
}
void OGE_SetPathStepInterval(int iSprId, int iStepInterval)
{
    ((CogeSprite*)iSprId)->SetPathStepInterval(iStepInterval);
}
int OGE_GetValidPathNodeCount(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetPathStepCount();
}
void OGE_SetValidPathNodeCount(int iSprId, int iStepCount)
{
    ((CogeSprite*)iSprId)->SetPathStepCount(iStepCount);
}

int OGE_GetPathDirection(int iSprId)
{
    //return ((CogeSprite*)iSprId)->GetPathDir();
    //return ((CogeSprite*)iSprId)->GetPathDirAvg();
    return ((CogeSprite*)iSprId)->GetPathKeyDir();
}

//int OGE_GetPathAvgDirection(int iSprId)
//{
//    return ((CogeSprite*)iSprId)->GetPathDirAvg();
//}

int  OGE_GetPath(const std::string& sPathName)
{
    return (int) g_engine->GetPath(sPathName);
}
int  OGE_GetPathByCode(int iCode)
{
    return (int) g_engine->GetPath(iCode);
}
void OGE_PathGenLine(int iPathId, int x1, int y1, int x2, int y2)
{
    ((CogePath*)iPathId)->GenShortLine(x1, y1, x2, y2);
}

void OGE_PathGenExtLine(int iPathId, int iSrcX, int iSrcY, int iDstX, int iDstY, int iExtraWidth, int iExtraHeight)
{
    int iWidth = OGE_GetViewWidth();
    int iHeight = OGE_GetViewHeight();

    if(iSrcX == iDstX && iSrcY == iDstY) return;

    if(iDstX == iSrcX)
    {
        if(iDstY > iSrcY) iDstY = iHeight + iExtraHeight;
        else iDstY = 0 - iExtraHeight;
    }
    else if(iDstY == iSrcY)
    {
        if(iDstX > iSrcX) iDstX = iWidth + iExtraWidth;
        else iDstX = 0 - iExtraWidth;
    }
    else
    {
        if(iDstX < iSrcX && iDstY < iSrcY)
        {
            int x = iSrcX - iDstX;
            int y = iSrcY - iDstY;

            if(x >= y)
            {
                iDstX = 0 - iExtraWidth;
                iDstY = (iSrcX - iDstX) * y / x;
                iDstY = iSrcY - iDstY;
            }
            else
            {
                iDstY = 0 - iExtraHeight;
                iDstX = (iSrcY - iDstY) * x / y;
                iDstX = iSrcX - iDstX;
            }
        }
        else if(iDstX > iSrcX && iDstY < iSrcY)
        {
            int x = iDstX - iSrcX;
            int y = iSrcY - iDstY;

            if(x >= y)
            {
                iDstX = iWidth + iExtraWidth;
                iDstY = (iDstX - iSrcX) * y / x;
                iDstY = iSrcY - iDstY;
            }
            else
            {
                iDstY = 0 - iExtraHeight;
                iDstX = (iSrcY - iDstY) * x / y;
                iDstX = iSrcX + iDstX;
            }
        }
        else if(iDstX > iSrcX && iDstY > iSrcY)
        {
            int x = iDstX - iSrcX;
            int y = iDstY - iSrcY;

            if(x >= y)
            {
                iDstX = iWidth + iExtraWidth;
                iDstY = (iDstX - iSrcX) * y / x;
                iDstY = iSrcY + iDstY;
            }
            else
            {
                iDstY = iHeight + iExtraHeight;
                iDstX = (iDstY - iSrcY) * x / y;
                iDstX = iSrcX + iDstX;
            }
        }
        else if(iDstX < iSrcX && iDstY > iSrcY)
        {
            int x = iSrcX - iDstX;
            int y = iDstY - iSrcY;

            if(x >= y)
            {
                iDstX = 0 - iExtraWidth;
                iDstY = (iSrcX - iDstX) * y / x;
                iDstY = iSrcY + iDstY;
            }
            else
            {
                iDstY = iHeight + iExtraHeight;
                iDstX = (iDstY - iSrcY) * x / y;
                iDstX = iSrcX - iDstX;
            }
        }
    }

    OGE_PathGenLine(iPathId, iSrcX, iSrcY, iDstX, iDstY);
}

int  OGE_GetPathNodeCount(int iPathId)
{
    return ((CogePath*)iPathId)->GetTotalStepCount();
}

/*
int  OGE_GetValidStepCount(int iPathId)
{
    return ((CogePath*)iPathId)->GetValidStepCount();
}
void OGE_SetValidStepCount(int iPathId, int iCount)
{
    ((CogePath*)iPathId)->SetValidStepCount(iCount);
}
*/

bool OGE_OpenTimer(int iSprId, int iInterval)
{
    return ((CogeSprite*)iSprId)->OpenTimer(iInterval);
}
void OGE_CloseTimer(int iSprId)
{
    ((CogeSprite*)iSprId)->CloseTimer();
}
int OGE_GetTimerInterval(int iSprId)
{
    return ((CogeSprite*)iSprId)->GetTimerInterval();
}
bool OGE_IsTimerWaiting(int iSprId)
{
    return ((CogeSprite*)iSprId)->IsTimerWaiting();
}

bool OGE_OpenSceneTimer(int iInterval)
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    return pCurrentScene && pCurrentScene->OpenTimer(iInterval);
}
void OGE_CloseSceneTimer()
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    if(pCurrentScene) pCurrentScene->CloseTimer();
}
int  OGE_GetSceneTimerInterval()
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    if(pCurrentScene) return pCurrentScene->GetTimerInterval();
    else return -1;
}

bool OGE_IsSceneTimerWaiting()
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    if(pCurrentScene) return pCurrentScene->IsTimerWaiting();
    else return false;
}

int  OGE_GetScenePlot()
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    if(pCurrentScene) return (int) pCurrentScene->GetPlotSprite();
    else return 0;
}
void OGE_SetScenePlot(int iSprId)
{
    CogeScene* pCurrentScene = g_engine->GetCurrentScene();
    if(pCurrentScene) pCurrentScene->SetPlotSprite((CogeSprite*)iSprId);
}
void OGE_PlayPlot(int iSprId, int iLoopTimes)
{
    //return (int)g_engine->OpenPlot((CogeSprite*)iSprId);
    return ((CogeSprite*)iSprId)->EnablePlot(iLoopTimes);

}
void OGE_OpenPlot(int iSprId)
{
    //return (int)g_engine->OpenPlot((CogeSprite*)iSprId);
    return ((CogeSprite*)iSprId)->EnablePlot(1);

}
void OGE_ClosePlot(int iSprId)
{
    //g_engine->ClosePlot();
    ((CogeSprite*)iSprId)->DisablePlot();
}
void OGE_PausePlot(int iSprId)
{
    ((CogeSprite*)iSprId)->SuspendUpdateScript();
}
void OGE_ResumePlot(int iSprId)
{
    ((CogeSprite*)iSprId)->ResumeUpdateScript();
}
bool OGE_IsPlayingPlot(int iSprId)
{
    return ((CogeSprite*)iSprId)->IsPlayingPlot();
}

int OGE_GetPlotTriggerFlag(int iSprId, int iEventCode)
{
    return ((CogeSprite*)iSprId)->GetPlotTriggerFlag(iEventCode);
}
void OGE_SetPlotTriggerFlag(int iSprId, int iEventCode, int iFlag)
{
    ((CogeSprite*)iSprId)->SetPlotTriggerFlag(iEventCode, iFlag);
}

int  OGE_GetSprGameData(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetGameData();
}

int  OGE_GetSprCustomData(int iSprId)
{
    return (int)((CogeSprite*)iSprId)->GetCustomData();
}
void OGE_SetSprCustomData(int iSprId, int iGameDataId, bool bOwnIt)
{
    ((CogeSprite*)iSprId)->SetCustomData((CogeGameData*)iGameDataId, bOwnIt);
}

int  OGE_GetAppCustomData()
{
    return (int) (g_engine->GetCustomData());
}
void OGE_SetAppCustomData(int iGameDataId)
{
    g_engine->SetCustomData((CogeGameData*)iGameDataId);
}

int  OGE_GetSceneCustomData(int iSceneId)
{
    return (int)((CogeScene*)iSceneId)->GetCustomData();
}

void OGE_SetSceneCustomData(int iSceneId, int iGameDataId, bool bOwnIt)
{
    ((CogeScene*)iSceneId)->SetCustomData((CogeGameData*)iGameDataId, bOwnIt);
}



void OGE_PauseGame()
{
    g_engine->Pause();
}
void OGE_ResumeGame()
{
    g_engine->Resume();
}
bool OGE_IsPlayingGame()
{
    return g_engine->IsPlaying();
}
int OGE_QuitGame()
{
    return g_engine->Terminate();
}

/*
void OGE_SetSprGameData(int iSprId, const std::string& sGameDataName)
{
    ((CogeSprite*)iSprId)->BindGameData(sGameDataName);
}
*/

int  OGE_FindGameData(const std::string& sGameDataName)
{
    return (int)g_engine->FindGameData(sGameDataName);
}

int  OGE_GetGameData(const std::string& sGameDataName, const std::string& sTemplateName)
{
    return (int)g_engine->GetGameData(sGameDataName, sTemplateName);
}

const std::string& OGE_GetGameDataName(int iGameDataId)
{
    return ((CogeGameData*)iGameDataId)->GetName();
}

int  OGE_GetCustomGameData(const std::string& sGameDataName, int iIntCount, int iFloatCount, int iStrCount)
{
    return (int)g_engine->GetGameData(sGameDataName, iIntCount, iFloatCount, iStrCount);
}

int  OGE_GetAppGameData()
{
    return (int)g_engine->GetAppGameData();
}

void OGE_CopyGameData(int iFromGameDataId, int iToGameDataId)
{
    return ((CogeGameData*)iToGameDataId)->Copy((CogeGameData*)iFromGameDataId);
}

bool OGE_IsInputUnicode()
{
    return g_engine->IsUnicodeIM();
}

void OGE_FillColor(int iImageId, int iColor)
{
    ((CogeImage*)iImageId)->FillRect(iColor);
}

void OGE_DrawText(int iImageId, const std::string& sText, int x, int y)
{
    bool bUseAlign = false;
    if(!bUseAlign) ((CogeImage*)iImageId)->PrintText(sText, x, y);
    else ((CogeImage*)iImageId)->PrintTextAlign(sText, x, y);
}

void OGE_DrawTextWithDotFont(int iImageId, const std::string& sText, int x, int y)
{
    ((CogeImage*)iImageId)->DotTextOut(sText.c_str(), x, y);
}

int  OGE_GetTextWidth(int iImageId, const std::string& sText)
{
    int iMaxLen = -1;
    int iWidth = 0; int iHeight = 0;
    if(iMaxLen == 0) return 0;
    else if(iMaxLen < 0)
    {
        if(sText.length() > 0 && ((CogeImage*)iImageId)->GetTextSize(sText, &iWidth, &iHeight)) return iWidth;
        else return 0;
    }
    else
    {
        if(((CogeImage*)iImageId)->GetTextSize(sText.substr(0, iMaxLen), &iWidth, &iHeight)) return iWidth;
        else return 0;
    }
}

bool OGE_GetTextSize(int iImageId, const std::string& sText, int& w, int& h)
{
    int iWidth = 0; int iHeight = 0;
    if(sText.length() == 0)
    {
        w = iWidth; h = iHeight;
        return true;
    }
    else
    {
        bool rsl = ((CogeImage*)iImageId)->GetTextSize(sText, &iWidth, &iHeight);
        if(rsl) { w = iWidth; h = iHeight; }
        return rsl;
    }
}

void OGE_DrawBufferText(int iImageId, int iBufferId, int iBufferSize, int x, int y, const std::string& sCharsetName)
{
    bool bUseAlign = false;
    if(!bUseAlign) ((CogeImage*)iImageId)->PrintBufferText((char*)iBufferId, iBufferSize, x, y, sCharsetName.c_str());
    else ((CogeImage*)iImageId)->PrintBufferTextAlign((char*)iBufferId, iBufferSize, x, y, sCharsetName.c_str());
}

int  OGE_GetBufferTextWidth(int iImageId, int iBufferId, int iBufferSize, const std::string& sCharsetName)
{
    int iWidth = 0; int iHeight = 0;
    bool bIsUnicode = sCharsetName == "UTF-16";
    if (!bIsUnicode)
    {
        char buf[_OGE_MAX_TEXT_LEN_];
        memset((void*)buf, 0, _OGE_MAX_TEXT_LEN_);
        memcpy((void*)buf, (char*)iBufferId, iBufferSize);

        if(iBufferSize > 0 && ((CogeImage*)iImageId)->GetBufferTextSize(buf, iBufferSize, &iWidth, &iHeight, sCharsetName.c_str())) return iWidth;
        else return 0;
    }
    else
    {
        if(iBufferSize > 0 && ((CogeImage*)iImageId)->GetBufferTextSize((char*)iBufferId, iBufferSize, &iWidth, &iHeight, sCharsetName.c_str())) return iWidth;
        else return 0;
    }

}

bool OGE_GetBufferTextSize(int iImageId, int iBufferId, int iBufferSize, const std::string& sCharsetName, int& w, int& h)
{
    int iWidth = 0; int iHeight = 0;

    if(iBufferSize <= 0)
    {
        w = iWidth; h = iHeight;
        return true;
    }

    bool rsl = false;

    bool bIsUnicode = sCharsetName == "UTF-16";
    if (!bIsUnicode)
    {
        char buf[_OGE_MAX_TEXT_LEN_];
        memset((void*)buf, 0, _OGE_MAX_TEXT_LEN_);
        memcpy((void*)buf, (char*)iBufferId, iBufferSize);

        rsl = ((CogeImage*)iImageId)->GetBufferTextSize(buf, iBufferSize, &iWidth, &iHeight, sCharsetName.c_str());
    }
    else
    {
        rsl = ((CogeImage*)iImageId)->GetBufferTextSize((char*)iBufferId, iBufferSize, &iWidth, &iHeight, sCharsetName.c_str());
    }

    if(rsl) { w = iWidth; h = iHeight; }

    return rsl;
}

const std::string& OGE_GetImageName(int iImageId)
{
    return ((CogeImage*)iImageId)->GetName();
}

int  OGE_GetColorKey(int iImageId)
{
    return ((CogeImage*)iImageId)->GetColorKey();
}
void OGE_SetColorKey(int iImageId, int iColor)
{
    ((CogeImage*)iImageId)->SetColorKey(iColor);
}

int  OGE_GetPenColor(int iImageId)
{
    return ((CogeImage*)iImageId)->GetPenColor();
}
void OGE_SetPenColor(int iImageId, int iColor)
{
    ((CogeImage*)iImageId)->SetPenColor(iColor);
}

int OGE_GetFont(const std::string& sName, int iFontSize)
{
    return (int)g_engine->GetFont(sName, iFontSize);
}

const std::string& OGE_GetFontName(int iFontId)
{
    return ((CogeFont*)iFontId)->GetName();
}
int OGE_GetFontSize(int iFontId)
{
    return ((CogeFont*)iFontId)->GetFontSize();
}

int OGE_GetDefaultFont()
{
    return (int)g_engine->GetDefaultFont();
}
void OGE_SetDefaultFont(int iFontId)
{
    g_engine->SetDefaultFont((CogeFont*)iFontId);
}
int OGE_GetImageFont(int iImageId)
{
    return (int) ((CogeImage*)iImageId)->GetDefaultFont();
}
void OGE_SetImageFont(int iImageId, int iFontId)
{
    ((CogeImage*)iImageId)->SetDefaultFont((CogeFont*)iFontId);
}

const std::string& OGE_GetDefaultCharset()
{
    return g_engine->GetDefaultCharset();
}
void OGE_SetDefaultCharset(const std::string& sCharsetName)
{
    g_engine->SetDefaultCharset(sCharsetName);
}

const std::string& OGE_GetSystemCharset()
{
    return g_engine->GetSystemCharset();
}
void OGE_SetSystemCharset(const std::string& sCharsetName)
{
    g_engine->SetSystemCharset(sCharsetName);
}

void OGE_DrawLine(int iImageId, int iStartX, int iStartY, int iEndX, int iEndY)
{
    ((CogeImage*)iImageId)->DrawLine(iStartX, iStartY, iEndX, iEndY);
}

void OGE_DrawCircle(int iImageId, int iCenterX, int iCenterY, int iRadius)
{
    ((CogeImage*)iImageId)->DrawCircle(iCenterX, iCenterY, iRadius);
}

int OGE_GetClipboardB()
{
    return (int) g_engine->GetVideo()->GetClipboardB();
}
int OGE_GetClipboardC()
{
    return (int) g_engine->GetVideo()->GetClipboardC();
}

void OGE_Screenshot()
{
    g_engine->Screenshot();
}
int OGE_GetLastScreenshot()
{
    return (int) g_engine->GetLastScreenshot();
}

int OGE_GetScreenImage()
{
    return (int) g_engine->GetScreen();
}
void OGE_ImageRGB(int iImageId, int iRectId, int iRed, int iGreen, int iBlue)
{
    //g_engine->GetScreen()->ChangeColorRGB(
    //g_engine->GetSceneViewPosX(), g_engine->GetSceneViewPosY(),
    //g_engine->GetSceneViewWidth(), g_engine->GetSceneViewHeight(),
    //iRed, iGreen, iBlue);

    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iImageId)->ChangeColorRGB(pRect->x, pRect->y, pRect->w, pRect->h, iRed, iGreen, iBlue);
}

/*
void OGE_ScreenBltColor(int iColor, int iAlpha)
{
    int w = g_engine->GetSceneViewWidth();
    int h = g_engine->GetSceneViewHeight();

    int x = g_engine->GetSceneViewPosX();
    int y = g_engine->GetSceneViewPosY();

    CogeImage* pClipboardB = g_engine->GetVideo()->GetClipboardB();
    if(pClipboardB)
    {
        pClipboardB->FillRect(iColor, 0, 0, w, h);
        g_engine->GetScreen()->BltAlphaBlend(pClipboardB, iAlpha, x, y, 0, 0, w, h);
    }
}
*/

void OGE_ImageLightness(int iImageId, int iRectId, int iAmount)
{
    //g_engine->GetScreen()->Lightness(
    //g_engine->GetSceneViewPosX(), g_engine->GetSceneViewPosY(),
    //g_engine->GetSceneViewWidth(), g_engine->GetSceneViewHeight(),
    //iAmount);

    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iImageId)->Lightness(pRect->x, pRect->y, pRect->w, pRect->h, iAmount);
}

void OGE_ImageGrayscale(int iImageId, int iRectId, int iAmount)
{
    //g_engine->GetScreen()->Grayscale(
    //g_engine->GetSceneViewPosX(), g_engine->GetSceneViewPosY(),
    //g_engine->GetSceneViewWidth(), g_engine->GetSceneViewHeight(),
    //iAmount);

    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iImageId)->Grayscale(pRect->x, pRect->y, pRect->w, pRect->h, iAmount);
}

void OGE_ImageBlur(int iImageId, int iRectId, int iAmount)
{
    SDL_Rect* pRect = (SDL_Rect*) iRectId;
    ((CogeImage*)iImageId)->Blur(pRect->x, pRect->y, pRect->w, pRect->h, iAmount);
}

void OGE_StretchBltToScreen(int iSrcImageId, int iRectId)
{
    SDL_Rect* pRect = (SDL_Rect*) iRectId;

    int iDstX = OGE_GetViewPosX();
    int iDstY = OGE_GetViewPosY();
    int iDstW = OGE_GetViewWidth();
    int iDstH = OGE_GetViewHeight();

#if !defined(__WINCE__) && !defined(__ANDROID__) && !defined(__IPHONE__)
    g_engine->GetScreen()->BltStretchSmoothly((CogeImage*)iSrcImageId, pRect->x, pRect->y, pRect->x+pRect->w, pRect->y+pRect->h,
                                              iDstX, iDstY, iDstX+iDstW, iDstY+iDstH);
#else
    g_engine->GetScreen()->BltStretch((CogeImage*)iSrcImageId, pRect->x, pRect->y, pRect->x+pRect->w, pRect->y+pRect->h,
                                              iDstX, iDstY, iDstX+iDstW, iDstY+iDstH);
#endif

}

void OGE_SaveImageAsBMP(int iImageId, const std::string& sBmpFileName)
{
    if(iImageId != 0 && sBmpFileName.length() > 0) ((CogeImage*)iImageId)->SaveAsBMP(sBmpFileName);
}

/*
int OGE_UseMusic(std::string& sMusicName)
{
    return g_engine->UseMusic(sMusicName);
}
int OGE_PlayMusic()
{
    return g_engine->PlayMusic();
}
int OGE_StopMusic()
{
    return g_engine->StopMusic();
}
int OGE_StopAudio()
{
    return g_engine->StopAudio();
}
int OGE_PlaySound(std::string& sSoundName)
{
    return g_engine->PlaySound(sSoundName);
}
int OGE_PlaySomeMusic(std::string& sMusicName)
{
    return g_engine->PlayMusic(sMusicName);
}
*/


int  OGE_PlayBgMusic(int iLoopTimes)
{
    return g_engine->PlayBgMusic(iLoopTimes);
}
int  OGE_PlayMusic(int iLoopTimes)
{
    return g_engine->PlayMusic(iLoopTimes);
}
int OGE_StopMusic()
{
    return g_engine->StopMusic();
}

int  OGE_PauseMusic()
{
    return g_engine->PauseMusic();

}
int  OGE_ResumeMusic()
{
    return g_engine->ResumeMusic();
}

int  OGE_PlayMusicByName(const std::string& sMusicName, int iLoopTimes)
{
    return g_engine->PlayMusic(sMusicName, iLoopTimes);
}

int  OGE_FadeInMusic(const std::string& sMusicName, int iLoopTimes, int iFadingTime)
{
    return g_engine->FadeInMusic(sMusicName, iLoopTimes, iFadingTime);
}
int  OGE_FadeOutMusic(int iFadingTime)
{
    return g_engine->FadeOutMusic(iFadingTime);
}

int  OGE_GetSound(const std::string& sSoundName)
{
    return (int) g_engine->GetSound(sSoundName);
}
int  OGE_PlaySoundByName(const std::string& sSoundName, int iLoopTimes)
{
    return g_engine->PlaySound(sSoundName, iLoopTimes);
}
int  OGE_PlaySoundByCode(int iSoundCode, int iLoopTimes)
{
    return g_engine->PlaySound(iSoundCode, iLoopTimes);
}
int  OGE_PlaySoundById(int iSoundId, int iLoopTimes)
{
    return ((CogeSound*)iSoundId)->Play(iLoopTimes);
}
int  OGE_PlaySound(int iSoundCode)
{
    return g_engine->PlaySound(iSoundCode);
}
int  OGE_StopSoundById(int iSoundId)
{
    return ((CogeSound*)iSoundId)->Stop();
}
int  OGE_StopSoundByCode(int iSoundCode)
{
    return g_engine->StopSound(iSoundCode);
}
int  OGE_StopSoundByName(const std::string& sSoundName)
{
    return g_engine->StopSound(sSoundName);
}

int  OGE_StopAudio()
{
    return g_engine->StopAudio();
}

int  OGE_PlayMovie(const std::string& sMovieName)
{
    return g_engine->PlayMovie(sMovieName, 0);
}
int  OGE_StopMovie()
{
    return g_engine->StopMovie();
}

bool OGE_IsPlayingMovie()
{
    return g_engine->IsPlayingMovie();
}

int OGE_GetSoundVolume()
{
    return g_engine->GetSoundVolume();
}
int OGE_GetMusicVolume()
{
    return g_engine->GetMusicVolume();
}
void OGE_SetSoundVolume(int iValue)
{
    g_engine->SetSoundVolume(iValue);
}
void OGE_SetMusicVolume(int iValue)
{
    g_engine->SetMusicVolume(iValue);
}

bool OGE_GetFullScreen()
{
    return g_engine->GetFullScreen();
}
void OGE_SetFullScreen(bool bValue)
{
    g_engine->SetFullScreen(bValue);
}

int  OGE_GetScene()
{
    return (int)g_engine->GetCurrentScene();
}
int  OGE_GetActiveScene()
{
    return (int)g_engine->GetActiveScene();
}
int  OGE_GetLastActiveScene()
{
    return (int)g_engine->GetLastActiveScene();
}
int  OGE_GetNextActiveScene()
{
    return (int)g_engine->GetNextActiveScene();
}
int  OGE_FindScene(const std::string& sSceneName)
{
    return (int)g_engine->FindScene(sSceneName);
}
int  OGE_GetSceneByName(const std::string& sSceneName)
{
    return (int)g_engine->GetScene(sSceneName);
}
const std::string& OGE_GetSceneName(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetName();
}
const std::string& OGE_GetSceneChapter(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetChapterName();
}
void OGE_SetSceneChapter(int iSceneId, const std::string& sChapterName)
{
    ((CogeScene*)iSceneId)->SetChapterName(sChapterName);
}
const std::string& OGE_GetSceneTag(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetTagName();
}
void OGE_SetSceneTag(int iSceneId, const std::string& sTagName)
{
    ((CogeScene*)iSceneId)->SetTagName(sTagName);
}

int  OGE_GetSceneGameData(int iSceneId)
{
    return (int)((CogeScene*)iSceneId)->GetGameData();
}

int  OGE_CallScene(const std::string& sSceneName)
{
    return g_engine->SetNextScene(sSceneName);
}

/*
void OGE_FadeIn (int iFadeType)
{
    return g_engine->FadeIn(iFadeType);
}
void OGE_FadeOut(int iFadeType)
{
    return g_engine->FadeOut(iFadeType);
}

int OGE_GetSceneFadeInSpeed(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetFadeInSpeed();
}

int OGE_GetSceneFadeOutSpeed(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetFadeOutSpeed();
}

int OGE_SetSceneFadeInSpeed(int iSceneId, int iSpeed)
{
    return ((CogeScene*)iSceneId)->SetFadeInSpeed(iSpeed);
}
int OGE_SetSceneFadeOutSpeed(int iSceneId, int iSpeed)
{
    return ((CogeScene*)iSceneId)->SetFadeOutSpeed(iSpeed);
}
int OGE_GetSceneFadeInType(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetFadeInType();
}
int OGE_GetSceneFadeOutType(int iSceneId)
{
    return ((CogeScene*)iSceneId)->GetFadeOutType();
}
int OGE_SetSceneFadeInType(int iSceneId, int iType)
{
    return ((CogeScene*)iSceneId)->SetFadeInType(iType);
}
int OGE_SetSceneFadeOutType(int iSceneId, int iType)
{
    return ((CogeScene*)iSceneId)->SetFadeOutType(iType);
}
*/

int OGE_SetSceneFadeIn(int iSceneId, int iType, int iSpeed)
{
    ((CogeScene*)iSceneId)->SetFadeInType(iType);
    return ((CogeScene*)iSceneId)->SetFadeInSpeed(iSpeed);
}
int OGE_SetSceneFadeOut(int iSceneId, int iType, int iSpeed)
{
    ((CogeScene*)iSceneId)->SetFadeOutType(iType);
    return ((CogeScene*)iSceneId)->SetFadeOutSpeed(iSpeed);
}

int OGE_SetSceneFadeMask(int iSceneId, int iImageId)
{
    ((CogeScene*)iSceneId)->SetFadeMask((CogeImage*)iImageId);
    return iImageId;
}

int OGE_GetGroup(const std::string& sGroupName)
{
    return (int)g_engine->GetGroup(sGroupName);
}
int  OGE_GetGroupByCode(int iCode)
{
    return (int)g_engine->GetGroup(iCode);
}

const std::string& OGE_GetGroupName(int iSprGroupId)
{
    return ((CogeSpriteGroup*)iSprGroupId)->GetName();
}

/*
void OGE_BindSprGroupToScene(int iSprGroupId, int iSceneId)
{
    ((CogeSpriteGroup*)iSprGroupId)->SetCurrentScene((CogeScene*)iSceneId);
}
*/

void OGE_OpenGroup(int iSprGroupId)
{
    g_engine->ActivateGroup((CogeSpriteGroup*)iSprGroupId);
}
void OGE_CloseGroup(int iSprGroupId)
{
    g_engine->DeactivateGroup((CogeSpriteGroup*)iSprGroupId);
}

int OGE_GroupGenSpr(int iSprGroupId, const std::string& sTempletSprName, int iCount)
{
    return ((CogeSpriteGroup*)iSprGroupId)->GenSprites(sTempletSprName, iCount);
}
int OGE_GetRootSprFromGroup(int iSprGroupId)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetRootSprite();
}
int OGE_GetFreeSprFromGroup(int iSprGroupId)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreeSprite();
}
/*
int  OGE_GetFreePlotFromGroup(int iSprGroupId)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreePlot();
}
int  OGE_GetFreeTimerFromGroup(int iSprGroupId)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreeTimer();
}
*/

int  OGE_GetFreeSprWithType(int iSprGroupId, int iSprType)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreeSprite(iSprType);
}
int  OGE_GetFreeSprWithClassTag(int iSprGroupId, int iSprClassTag)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreeSprite(-1, iSprClassTag);
}
int  OGE_GetFreeSprWithTag(int iSprGroupId, int iSprTag)
{
    return (int)((CogeSpriteGroup*)iSprGroupId)->GetFreeSprite(-1, -1, iSprTag);
}

int  OGE_GetGroupSprCount(int iSprGroupId)
{
    return ((CogeSpriteGroup*)iSprGroupId)->GetSpriteCount();
}

int  OGE_GetGroupSprByIndex(int iGroupId, int iIndex)
{
    return (int) ((CogeSpriteGroup*)iGroupId)->GetSpriteByIndex(iIndex-1);
}

int  OGE_GetGroupSpr(int iGroupId, int idx)
{
    return (int) ((CogeSpriteGroup*)iGroupId)->GetNpcSprite(idx - 1);
}

int  OGE_GetSceneSprCount(int iSceneId, int iState)
{
    return ((CogeScene*)iSceneId)->GetSpriteCount(iState);
}
int  OGE_GetSceneSprByIndex(int iSceneId, int iIndex, int iState)
{
    return (int) ((CogeScene*)iSceneId)->GetSpriteByIndex(iIndex-1, iState);
}

int  OGE_GetGroupCount()
{
    return g_engine->GetGroupCount();
}
int  OGE_GetGroupByIndex(int iIndex)
{
    return (int) g_engine->GetGroupByIndex(iIndex-1);
}

int  OGE_GetSceneCount()
{
    return g_engine->GetSceneCount();
}
int  OGE_GetSceneByIndex(int iIndex)
{
    return (int) g_engine->GetSceneByIndex(iIndex-1);
}

int OGE_GetSprForScene(int iSceneId, const std::string& sSprName, const std::string& sClassName)
{
    return (int) ((CogeScene*)iSceneId)->AddSprite(sSprName, sClassName);
}
int OGE_GetSprForGroup(int iGroupId, const std::string& sSprName, const std::string& sClassName)
{
    return (int) ((CogeSpriteGroup*)iGroupId)->AddSprite(sSprName, sClassName);
}

int OGE_RemoveSprFromScene(int iSceneId, int iSprId)
{
    return ((CogeScene*)iSceneId)->RemoveSprite((CogeSprite*)iSprId);
}
int OGE_RemoveSprFromGroup(int iGroupId, int iSprId)
{
    return ((CogeSpriteGroup*)iGroupId)->RemoveSprite((CogeSprite*)iSprId);
}

void OGE_RemoveScene(const std::string& sName)
{
    g_engine->DelScene(sName);
}
void OGE_RemoveGroup(const std::string& sName)
{
    g_engine->DelGroup(sName);
}
void OGE_RemoveGameData(const std::string& sName)
{
    g_engine->DelGameData(sName);
}
void OGE_RemoveGameMap(const std::string& sName)
{
    g_engine->DelGameMap(sName);
}
void OGE_RemoveImage(const std::string& sName)
{
    g_engine->DelImage(sName);
}
void OGE_RemoveMusic(const std::string& sName)
{
    g_engine->DelMusic(sName);
}
void OGE_RemoveMovie(const std::string& sName)
{
    g_engine->DelMovie(sName);
}
void OGE_RemoveFont(const std::string& sName, int iFontSize)
{
    g_engine->DelFont(sName, iFontSize);
}


int OGE_GetStaticBuf(int iIndex)
{
    int idx = iIndex - 1;
    if(idx < 0 || idx >= _OGE_MAX_BUF_COUNT_) return 0;

    char* rsl = &(g_bufs[idx][0]);

    return (int) rsl;
}
int OGE_ClearBuf(int iBufferId)
{
    void* pBuf = (void*) iBufferId;
    memset(pBuf, 0, _OGE_MAX_TEXT_LEN_);
    return _OGE_MAX_TEXT_LEN_;
}
int OGE_CopyBuf(int iFromId, int iToId, int iStart, int iLen)
{
    char* pFrom = (char*) iFromId;
    pFrom = pFrom + iStart;
    char* pTo = (char*) iToId;
    memcpy(pTo, pFrom, iLen);
    return iLen;
}

int OGE_StrToUnicode(const std::string& sText, const std::string& sCharsetName, int iBufferId)
{
    char* pUnicodeBuffer = (char*) iBufferId;
    return g_engine->StringToUnicode(sText, sCharsetName, pUnicodeBuffer);
}

std::string OGE_UnicodeToStr(int iUnicodeBufferId, int iBufferSize, const std::string& sCharsetName)
{
    char* pUnicodeBuffer = (char*) iUnicodeBufferId;
    return g_engine->UnicodeToString(pUnicodeBuffer, iBufferSize, sCharsetName);
}

/*
int OGE_OpenStreamBuf(int iBufferId)
{
    return (int) g_engine->NewStreamBuf((char*)iBufferId, _OGE_MAX_TEXT_LEN_);
}
int OGE_CloseStreamBuf(int iStreamBufId)
{
    return g_engine->DelStreamBuf(iStreamBufId);
}
int OGE_GetBufReadPos(int iStreamBufId)
{
    return ((CogeStreamBuffer*)iStreamBufId)->GetReadPos();
}
int OGE_SetBufReadPos(int iStreamBufId, int iPos)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetReadPos(iPos);
}
int OGE_GetBufWritePos(int iStreamBufId)
{
    return ((CogeStreamBuffer*)iStreamBufId)->GetWritePos();
}
int OGE_SetBufWritePos(int iStreamBufId, int iPos)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetWritePos(iPos);
}


char OGE_GetBufByte(int iStreamBufId)
{
    return ((CogeStreamBuffer*)iStreamBufId)->GetByte();
}

int OGE_GetBufInt(int iStreamBufId)
{
    return ((CogeStreamBuffer*)iStreamBufId)->GetInt();
}
double OGE_GetBufFloat(int iStreamBufId)
{
    return ((CogeStreamBuffer*)iStreamBufId)->GetFloat();
}
ogeScriptString OGE_GetBufStr(int iStreamBufId)
{
    std::string rsl = ((CogeStreamBuffer*)iStreamBufId)->GetStr();
    return NEW_SCRIPT_STR(rsl);
}

int OGE_PutBufByte(int iStreamBufId, char value)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetByte(value);
}
int OGE_PutBufInt(int iStreamBufId, int value)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetInt(value);
}
int OGE_PutBufFloat(int iStreamBufId, double value)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetFloat(value);
}
int OGE_PutBufStr(int iStreamBufId, const std::string& value)
{
    return ((CogeStreamBuffer*)iStreamBufId)->SetStr(value);
}
*/



char OGE_GetBufByte(int iBufId, int pos, int& next)
{
    char* src = (char*)iBufId;
    if(src == NULL) return 0;
    char rsl = *(src + pos);
    next = pos + 1;
    return rsl;
}
int OGE_GetBufInt(int iBufId, int pos, int& next)
{
    char* src = (char*)iBufId;
    if(src == NULL) return 0;
    int dst = 0; int len = sizeof(int);
    memcpy((void*)(&dst), (void*)(src + pos), len);
    next = pos + len;
    return dst;
}
double OGE_GetBufFloat(int iBufId, int pos, int& next)
{
    char* src = (char*)iBufId;
    if(src == NULL) return 0;
    double dst = 0; int len = sizeof(double);
    memcpy((void*)(&dst), (void*)(src + pos), len);
    next = pos + len;
    return dst;
}
std::string OGE_GetBufStr(int iBufId, int pos, int len, int& next)
{
    std::string s = "";

    char* src = (char*)iBufId;
    if(src == NULL) return s;

    //char dst[len+1];
    //memcpy((void*)(&dst[0]), (void*)(src + pos), len);
    //dst[len] = '\0';
    //next = pos + len;
    //return NEW_SCRIPT_STR(dst);

    memcpy((void*)(&g_tmpbuf[0]), (void*)(src + pos), len);
    g_tmpbuf[len] = '\0';
    next = pos + len;
    s = g_tmpbuf;
    return s;
}

int OGE_PutBufByte(int iBufId, int pos, char value)
{
    char* dst = (char*)iBufId;
    dst[pos] = value;
    return pos+1;
}
int OGE_PutBufInt(int iBufId, int pos, int value)
{
    char* dst = (char*)iBufId;
    int len = sizeof(int);
    memcpy((void*)(dst+pos), (void*)(&value), len);
    return pos+len;
}
int OGE_PutBufFloat(int iBufId, int pos, double value)
{
    char* dst = (char*)iBufId;
    int len = sizeof(double);
    memcpy((void*)(dst+pos), (void*)(&value), len);
    return pos+len;
}
int OGE_PutBufStr(int iBufId, int pos, const std::string& value)
{
    char* dst = (char*)iBufId;
    int len = value.length();
    memcpy((void*)(dst+pos), (void*)(value.c_str()), len);
    return pos+len;
}


int OGE_OpenLocalServer(int iPort)
{
    return g_engine->OpenLocalServer(iPort);
}
int OGE_CloseLocalServer()
{
    return g_engine->CloseLocalServer();
}

int OGE_ConnectServer(const std::string& sAddr)
{
    return (int)g_engine->ConnectServer(sAddr);
}

int OGE_DisconnectServer(const std::string& sAddr)
{
    return g_engine->DisconnectServer(sAddr);
}

int OGE_DisconnectClient(const std::string& sAddr)
{
    return g_engine->DisconnectClient(sAddr);
}

const std::string& OGE_GetSocketAddr(int iSockId)
{
    if(iSockId == 0) return g_engine->GetCurrentSocketAddr();
    else return ((CogeSocket*)iSockId)->GetAddr();
}

/*
int OGE_RefreshClientList()
{
    return g_engine->RefreshClientList();
}
int OGE_RefreshServerList()
{
    return g_engine->RefreshServerList();
}
int OGE_GetClientByIndex(int idx)
{
    return (int) g_engine->GetClientByIndex(idx-1);
}
int OGE_GetServerByIndex(int idx)
{
    return (int) g_engine->GetServerByIndex(idx-1);
}
*/

int OGE_GetUserSocket(int idx)
{
    return (int) g_engine->GetPlayerSocket(idx-1);
}
void OGE_SetUserSocket(int iSockId, int idx)
{
    g_engine->SetPlayerSocket((CogeSocket*)iSockId, idx-1);
}

int OGE_GetCurrentSocket()
{
    return (int) g_engine->GetCurrentSocket();
}

int OGE_GetRecvDataSize()
{
    return g_engine->GetRecvDataSize();
}

int OGE_GetRecvDataType()
{
    return g_engine->GetRecvDataType();
}

char OGE_GetRecvByte(int pos, int& next)
{
    char* src = g_engine->GetRecvData();
    if(src == NULL) return 0;
    char rsl = *(src + pos);
    next = pos + 1;
    return rsl;
}
int OGE_GetRecvInt(int pos, int& next)
{
    char* src = g_engine->GetRecvData();
    if(src == NULL) return 0;
    int dst = 0; int len = sizeof(int);
    memcpy((void*)(&dst), (void*)(src + pos), len);
    next = pos + len;
    return dst;
}

double OGE_GetRecvFloat(int pos, int& next)
{
    char* src = g_engine->GetRecvData();
    if(src == NULL) return 0;
    double dst = 0; int len = sizeof(double);
    memcpy((void*)(&dst), (void*)(src + pos), len);
    next = pos + len;
    return dst;
}
int OGE_GetRecvBuf(int pos, int len, int buf, int& next)
{
    char* src = g_engine->GetRecvData();
    if(src == NULL) return 0;
    char* dst = (char*)buf;
    memcpy((void*)dst, (void*)(src + pos), len);
    next = pos + len;
    return (int) dst;
}

std::string OGE_GetRecvStr(int pos, int len, int& next)
{
    std::string s = "";

    char* src = g_engine->GetRecvData();
    if(src == NULL) return s;

    //char dst[len+1];
    //memcpy((void*)(&dst), (void*)(src + pos), len);
    //dst[len] = '\0';
    //next = pos + len;
    //return NEW_SCRIPT_STR(dst);

    memcpy((void*)(&g_tmpbuf), (void*)(src + pos), len);
    g_tmpbuf[len] = '\0';
    next = pos + len;
    s = g_tmpbuf;
    return s;
}

int OGE_PutSendingByte(int iSockId, int pos, char value)
{
    return ((CogeSocket*)iSockId)->PutByte(pos, value);
}
int OGE_PutSendingInt(int iSockId, int pos, int value)
{
    return ((CogeSocket*)iSockId)->PutInt(pos, value);
}
int OGE_PutSendingFloat(int iSockId, int pos, double value)
{
    return ((CogeSocket*)iSockId)->PutFloat(pos, value);
}
int OGE_PutSendingBuf(int iSockId, int pos, int buf, int len)
{
    return ((CogeSocket*)iSockId)->PutBuf(pos, (char*)buf, len);
}

int OGE_PutSendingStr(int iSockId, int pos, const std::string& value)
{
    return ((CogeSocket*)iSockId)->PutStr(pos, value);
}
int OGE_SendData(int iSockId, int iDataSize)
{
    if(iSockId == 0)
    {
        CogeSocket* sockCurrent = g_engine->GetCurrentSocket();
        if(sockCurrent) return sockCurrent->SendData(iDataSize);
        else return 0;
    }
    else return ((CogeSocket*)iSockId)->SendData(iDataSize);
}

/*
int OGE_SendMsg(const std::string& sMsg)
{
    return g_engine->SendMsg(sMsg);
}
ogeScriptString OGE_GetRecvMsg()
{
    return NEW_SCRIPT_STR(g_engine->GetRecvMsg());
}
*/

const std::string& OGE_GetLocalIP()
{
    return g_engine->GetLocalIp();
}
int OGE_FindClient(const std::string& sAddr)
{
    return (int) g_engine->FindClient(sAddr);
}
int OGE_FindServer(const std::string& sAddr)
{
    return (int) g_engine->FindServer(sAddr);
}
int OGE_GetLocalServer()
{
    return (int) g_engine->GetLocalServer();
}

/*
ogeScriptByteArray OGE_GetRecvData()
{
    return NEW_SCRIPT_BYTE_ARRAY(g_engine->GetRecvData(), g_engine->GetRecvDataSize());
}


int OGE_SendData(ogeScriptByteArray data)
{
    return g_engine->SendData(data->c_str(), data->size());
}


ogeScriptByteArray OGE_TestGetByteArray()
{
    ogeScriptByteArray arr = NEW_SCRIPT_BYTE_ARRAY(g_engine->GetCurrentScene()->GetName().c_str(), g_engine->GetCurrentScene()->GetName().length());
    printf("Get Byte Array: %d, %d, %d", arr->get(0), arr->get(1), arr->get(2));
    return arr;
}
int OGE_TestSetByteArray(ogeScriptByteArray arr)
{
    printf("Set Byte Array: %d, %d, %d", arr->get(0), arr->get(1), arr->get(2));
    return arr->size();

}

int OGE_GetIntData(ogeScriptByteArray data, int idx)
{
    char* src = data->c_str();
    int dst = 0;
    memcpy((void*)(&dst), (void*)(src + idx), 4);
    return dst;
}
ogeScriptString OGE_GetStrData(ogeScriptByteArray data, int idx, int len)
{
    char* src = data->c_str();
    char dst[len+1];
    memcpy((void*)(&dst), (void*)(src + idx), len);
    dst[len] = '\0';
    return NEW_SCRIPT_STR(dst);
}
int OGE_SetIntData(ogeScriptByteArray data, int idx, int value)
{
    char* dst = data->c_str();
    memcpy((void*)(dst+idx), (void*)(&value), 4);
    return 4;
}
int OGE_SetStrData(ogeScriptByteArray data, int idx, std::string& value)
{
    char* dst = data->c_str();
    int len = value.length();
    memcpy((void*)(dst+idx), (void*)(value.c_str()), len);
    return len;
}
*/

int OGE_OpenCustomConfig(const std::string& sConfigFile)
{
    return (int)g_engine->NewIniFile(sConfigFile);
}

int OGE_CloseCustomConfig(int iConfigId)
{
    return (int)g_engine->DelIniFile(iConfigId);
}

int OGE_GetDefaultConfig()
{
    return (int)g_engine->GetUserConfig();
}

int OGE_GetAppConfig()
{
    return (int)g_engine->GetAppConfig();
}

std::string OGE_ReadConfigStr(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    return ((CogeIniFile*)iConfigId)->ReadString(sSectionName, sFieldName, sDefault);
}
/*
ogeScriptString OGE_ReadConfigPath(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    return NEW_SCRIPT_STR( ((CogeIniFile*)iConfigId)->ReadPath(sSectionName, sFieldName, sDefault) );
}
*/
int OGE_ReadConfigInt(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, int iDefault)
{
    return ((CogeIniFile*)iConfigId)->ReadInteger(sSectionName, sFieldName, iDefault);
}
double OGE_ReadConfigFloat(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, double fDefault)
{
    return ((CogeIniFile*)iConfigId)->ReadFloat(sSectionName, sFieldName, fDefault);
}

bool OGE_WriteConfigStr(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue)
{
    return ((CogeIniFile*)iConfigId)->WriteString(sSectionName, sFieldName, sValue);
}
bool OGE_WriteConfigInt(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, int iValue)
{
    return ((CogeIniFile*)iConfigId)->WriteInteger(sSectionName, sFieldName, iValue);
}
bool OGE_WriteConfigFloat(int iConfigId, const std::string& sSectionName, const std::string& sFieldName, double fValue)
{
    return ((CogeIniFile*)iConfigId)->WriteFloat(sSectionName, sFieldName, fValue);
}
bool OGE_SaveConfig(int iConfigId)
{
    return ((CogeIniFile*)iConfigId)->Save();
}

int OGE_SaveDataToConfig(int iDataId, int iConfigId)
{
    return g_engine->SaveGameDataToIniFile((CogeGameData*)iDataId, (CogeIniFile*)iConfigId);
}
int OGE_LoadDataFromConfig(int iDataId, int iConfigId)
{
    return g_engine->LoadGameDataFromIniFile((CogeGameData*)iDataId, (CogeIniFile*)iConfigId);
}

int OGE_SaveDataToDB(int iDataId, int iSessionId)
{
    return g_engine->SaveGameDataToDb((CogeGameData*)iDataId, iSessionId);
}
int OGE_LoadDataFromDB(int iDataId, int iSessionId)
{
    return g_engine->LoadGameDataFromDb((CogeGameData*)iDataId, iSessionId);
}

bool OGE_IsValidDBType(int iDbType)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->IsValidType(iDbType);
    else return false;
}
int OGE_OpenDefaultDB()
{
    return g_engine->OpenDefaultDb();
}
int OGE_OpenDB(int iDbType, const std::string& sCnnStr)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->OpenDb(iDbType, sCnnStr);
    else return 0;
}
int OGE_RunSql(int iSessionId, const std::string& sSql)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->RunSql(iSessionId, sSql);
    else return -1;
}
int OGE_OpenQuery(int iSessionId, const std::string& sQuerySql)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->OpenQuery(iSessionId, sQuerySql);
    else return 0;
}
void OGE_CloseQuery(int iQueryId)
{
    if(g_engine->GetDatabase()) g_engine->GetDatabase()->CloseQuery(iQueryId);
}
int OGE_FirstRecord(int iQueryId)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->FirstRecord(iQueryId);
    else return 0;
}
int OGE_NextRecord(int iQueryId)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->NextRecord(iQueryId);
    else return 0;
}
bool OGE_GetBoolFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->GetBoolFieldValue(iQueryId, sFieldName);
    else return false;
}
int OGE_GetIntFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->GetIntFieldValue(iQueryId, sFieldName);
    else return 0;
}
double OGE_GetFloatFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->GetFloatFieldValue(iQueryId, sFieldName);
    else return 0;
}
std::string OGE_GetStrFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->GetStrFieldValue(iQueryId, sFieldName);
    else return "";
}
std::string OGE_GetTimeFieldValue(int iQueryId, const std::string& sFieldName)
{
    if(g_engine->GetDatabase()) return g_engine->GetDatabase()->GetTimeFieldValue(iQueryId, sFieldName);
    else return "";
}
void OGE_CloseDB(int iSessionId)
{
    if(g_engine->GetDatabase()) g_engine->GetDatabase()->CloseDb(iSessionId);
}


bool OGE_FileExists(const std::string& sFileName)
{
    return OGE_IsFileExisted(sFileName.c_str());
}

bool OGE_CreateEmptyFile(const std::string& sFileName)
{
    std::fstream * fs = new std::fstream();
    fs->open( sFileName.c_str(), std::ios::out | std::ios::binary );

    bool rsl = fs->is_open();

    if(rsl) fs->close();

    delete fs;

    return rsl;
}

bool OGE_RemoveFile(const std::string& sFileName)
{
    return remove( sFileName.c_str() ) == 0;
}

int OGE_FileSize(const std::string& sFileName)
{
    int iRsl = -1;
    std::ifstream infile;
    infile.open(sFileName.c_str());
    if (infile.is_open())
    {
        infile.seekg(0, std::ios::end);
		iRsl = infile.tellg();
		infile.close();
    }
    return iRsl;
}

int OGE_OpenFile(const std::string& sFileName, bool bTryCreate)
{
    bool bExist = OGE_IsFileExisted(sFileName.c_str());

    if(!bExist)
    {
        if(!bTryCreate) return 0;
        else
        {
            if(!OGE_CreateEmptyFile(sFileName)) return 0;
        }
    }

    std::fstream * fs = new std::fstream();
    fs->open( sFileName.c_str(), std::ios::in | std::ios::out | std::ios::ate | std::ios::binary );
    if(fs->is_open()) return (int) fs;
    else delete fs;

    return 0;

}
int OGE_CloseFile(int iFileId)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    if(fs->is_open()) fs->close();
    delete fs;
    return 0;
}

int OGE_SaveFile(int iFileId)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->flush();
    return 0;
}

bool OGE_GetFileEof(int iFileId)
{
    if(iFileId == 0) return true;
    std::fstream * fs = (std::fstream *) iFileId;
    return fs->eof();
}
int OGE_GetFileReadPos(int iFileId)
{
    if(iFileId == 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    return fs->tellg();
}
int OGE_SetFileReadPos(int iFileId, int iPos)
{
    if(iFileId == 0 || iPos < 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->seekg(iPos, std::ios::beg);
    return fs->tellg();
}

int OGE_SetFileReadPosFromEnd(int iFileId, int iPos)
{
    if(iFileId == 0 || iPos < 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->seekg(iPos, std::ios::end);
    return fs->tellg();
}

int OGE_GetFileWritePos(int iFileId)
{
    if(iFileId == 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    return fs->tellp();
}
int OGE_SetFileWritePos(int iFileId, int iPos)
{
    if(iFileId == 0 || iPos < 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->seekp(iPos, std::ios::beg);
    return fs->tellp();
}

int OGE_SetFileWritePosFromEnd(int iFileId, int iPos)
{
    if(iFileId == 0 || iPos < 0) return -1;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->seekp(iPos, std::ios::end);
    return fs->tellp();
}

char OGE_ReadFileByte(int iFileId)
{
    if(iFileId == 0) return '\0';
    std::fstream * fs = (std::fstream *) iFileId;
    return fs->get();
}
int OGE_ReadFileInt(int iFileId)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    int value = 0;
    fs->read((char*)&value, sizeof(int));
    return value;
}
double OGE_ReadFileFloat(int iFileId)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    double value = 0;
    fs->read((char*)&value, sizeof(double));
    return value;
}
int OGE_ReadFileBuf(int iFileId, int iLen, int iBufId)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    char* value = (char*)iBufId;
    fs->read(value, iLen);
    return (int) value;
}
std::string OGE_ReadFileStr(int iFileId, int iLen)
{
    std::string s = "";
    if(iFileId == 0) return s;
    std::fstream * fs = (std::fstream *) iFileId;

    //char value[iLen+1];
    //fs->read(&(value[0]), iLen);
    //value[iLen] = '\0';
    //return NEW_SCRIPT_STR(value);

    fs->read(&(g_tmpbuf[0]), iLen);
    g_tmpbuf[iLen] = '\0';
    s = g_tmpbuf;
    return s;
}

int OGE_WriteFileByte(int iFileId, char value)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->put(value);
    return 1;
}
int OGE_WriteFileInt(int iFileId, int value)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    int len = sizeof(int);
    fs->write((char*)&value, len);
    return len;
}
int OGE_WriteFileFloat(int iFileId, double value)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    int len = sizeof(double);
    fs->write((char*)&value, len);
    return len;
}
int OGE_WriteFileBuf(int iFileId, int buf, int len)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    fs->write((char*)buf, len);
    return len;
}
int OGE_WriteFileStr(int iFileId, const std::string& value)
{
    if(iFileId == 0) return 0;
    std::fstream * fs = (std::fstream *) iFileId;
    int len = value.length();
    fs->write(value.c_str(), len);
    return len;
}




