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

#include "ogeAppParam.h"
#include "ogeCommon.h"

static int g_paramcount = 0;
static char** g_params = NULL;

static std::string g_maindir = ""; // end without "/"
static std::string g_gamedir = ""; // end without "/"

static std::string g_gamecmd = "";

static std::string g_configfile = "";

static std::string g_inputfile = "";

static std::string g_debugscene = "";

static std::string g_debugip = ""; // format: xxx.xxx.xxx.xxx:xxxx

void OGE_InitAppParams(int argc, char** argv)
{
    //OGE_Log("%s \n", "OGE_InitAppParams: - BEGIN -");

    g_paramcount = argc;
    g_params = argv;

    g_maindir = std::string(argv[0]);

    OGE_StrReplace(g_maindir, "\\", "/");

//#if defined(__MACOSX__) && !defined(__IPHONE__)
//    size_t iTailPos = g_maindir.rfind(".app/Contents/MacOS/");
//    if(iTailPos > 0) g_maindir = g_maindir.substr(0, iTailPos);
//#endif

#if defined(__MACOSX__) || defined(__IPHONE__)
    size_t iTailPos = g_maindir.rfind(".app/");
    if(iTailPos > 0) g_maindir = g_maindir.substr(0, iTailPos);
#endif

    size_t iPos = g_maindir.rfind("/");

    if(iPos != std::string::npos) g_maindir = g_maindir.substr(0, iPos);
    else g_maindir = OGE_GetCurrentDir(); // assume current dir is main dir ...

    if(g_paramcount <= 1)
    {
        g_gamedir = g_maindir;

#if defined(__IPHONE__)

        std::string sBinDir = std::string(argv[0]);
        iPos = sBinDir.rfind("/");
        if(iPos != std::string::npos) sBinDir = sBinDir.substr(0, iPos);
        std::string sConfigFileName = sBinDir + "/" + _OGE_DEFAULT_GAME_CONFIG_FILE_;
        if(OGE_IsFileExisted(sConfigFileName.c_str()))
        {
            g_configfile = sConfigFileName;
            g_maindir = sBinDir;
            g_gamedir = g_maindir;
        }
        else
        {
            sConfigFileName = g_maindir + "/" + _OGE_DEFAULT_GAME_CONFIG_FILE_;
            if(OGE_IsFileExisted(sConfigFileName.c_str()))
            {
                g_configfile = sConfigFileName;
            }
            else
            {
                sConfigFileName = g_maindir + "/Documents/" + _OGE_DEFAULT_GAME_CONFIG_FILE_;
                if(OGE_IsFileExisted(sConfigFileName.c_str()))
                {
                    g_configfile = sConfigFileName;
                    g_gamedir = g_maindir + "/Documents";
                }
            }
        }

#endif

    }


    if(g_paramcount == 2)
    {
        g_configfile = std::string(argv[1]);

        g_gamedir = std::string(argv[1]);
        OGE_StrReplace(g_gamedir, "\\", "/");
        iPos = g_gamedir.rfind("/");
        if(iPos != std::string::npos) g_gamedir = g_gamedir.substr(0, iPos);
        else g_gamedir = g_maindir;
    }

    g_gamecmd = _OGE_APP_CMD_GAME_; // by default ...

    if(g_paramcount > 2)
    {
        g_gamecmd = std::string(argv[1]);

        if(g_gamecmd.compare(_OGE_APP_CMD_GAME_) == 0)
        {
            g_configfile = std::string(argv[2]);

            g_gamedir = std::string(argv[2]);
            OGE_StrReplace(g_gamedir, "\\", "/");
            iPos = g_gamedir.rfind("/");
            if(iPos != std::string::npos) g_gamedir = g_gamedir.substr(0, iPos);
            else g_gamedir = g_maindir;
        }

        if(g_gamecmd.compare(_OGE_APP_CMD_DEBUG_) == 0)
        {
            g_configfile = std::string(argv[2]);

            g_gamedir = std::string(argv[2]);
            OGE_StrReplace(g_gamedir, "\\", "/");
            iPos = g_gamedir.rfind("/");
            if(iPos != std::string::npos) g_gamedir = g_gamedir.substr(0, iPos);
            else g_gamedir = g_maindir;

            if(g_paramcount > 3) g_debugip = std::string(argv[3]);
            if(g_paramcount > 4) g_inputfile = std::string(argv[4]);

        }

        if(g_gamecmd.compare(_OGE_APP_CMD_SCENE_) == 0)
        {
            g_configfile = std::string(argv[2]);

            g_gamedir = std::string(argv[2]);
            OGE_StrReplace(g_gamedir, "\\", "/");
            iPos = g_gamedir.rfind("/");
            if(iPos != std::string::npos) g_gamedir = g_gamedir.substr(0, iPos);
            else g_gamedir = g_maindir;

            if(g_paramcount > 3) g_debugscene = std::string(argv[3]);
            if(g_paramcount > 4) g_debugip = std::string(argv[4]);
            if(g_paramcount > 5) g_inputfile = std::string(argv[5]);

        }

        if(g_gamecmd.compare(_OGE_APP_CMD_SCRIPT_) == 0) g_inputfile = std::string(argv[2]);

        if(g_gamecmd.compare(_OGE_APP_CMD_PACK_) == 0) g_inputfile = std::string(argv[2]);

    }

    //OGE_Log("Main Dir: %s\n", g_maindir.c_str());
    //OGE_Log("Game Dir: %s\n", g_gamedir.c_str());
    //OGE_Log("%s \n", "OGE_InitAppParams: - END -");
}

int OGE_GetAppParamCount()
{
    return g_paramcount;
}
char** OGE_GetAppParams()
{
    return g_params;
}

const std::string& OGE_GetAppGameCmd()
{
    return g_gamecmd;
}

const std::string& OGE_GetAppInputFile()
{
    return g_inputfile;
}

const std::string& OGE_GetAppConfigFile()
{
    return g_configfile;
}

const std::string& OGE_GetAppMainDir()
{
    return g_maindir;
}

std::string OGE_GetAppMainPath()
{
    if(g_maindir.length() > 0) return g_maindir + "/";
    else return g_maindir;
}


const std::string& OGE_GetAppGameDir()
{
    return g_gamedir;
}

std::string OGE_GetAppGamePath()
{
    if(g_gamedir.length() > 0) return g_gamedir + "/";
    else return g_gamedir;
}

const std::string& OGE_GetAppDebugServerAddr()
{
    return g_debugip;
}

const std::string& OGE_GetAppDebugSceneName()
{
    return g_debugscene;
}
