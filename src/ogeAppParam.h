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

#ifndef __OGE_APP_PARAM_H_INCLUDED__
#define __OGE_APP_PARAM_H_INCLUDED__

#include <string>

static const std::string _OGE_APP_CMD_GAME_   = "game";
static const std::string _OGE_APP_CMD_PACK_   = "pack";
static const std::string _OGE_APP_CMD_DEBUG_  = "debug";
static const std::string _OGE_APP_CMD_SCENE_  = "scene";
static const std::string _OGE_APP_CMD_SCRIPT_ = "script";

static const std::string _OGE_DEFAULT_GAME_CONFIG_FILE_ = "app.ini";
static const std::string _OGE_DEFAULT_FUNC_CONFIG_FILE_ = "func.conf";
static const std::string _OGE_DEFAULT_KEY_MAP_FILE_     = "keymap.ini";

void OGE_InitAppParams(int argc, char** argv);

int OGE_GetAppParamCount();
char** OGE_GetAppParams();

const std::string& OGE_GetAppGameCmd();

const std::string& OGE_GetAppInputFile();

const std::string& OGE_GetAppConfigFile();

const std::string& OGE_GetAppMainDir();
std::string OGE_GetAppMainPath();

const std::string& OGE_GetAppGameDir();
std::string OGE_GetAppGamePath();

const std::string& OGE_GetAppDebugSceneName();
const std::string& OGE_GetAppDebugServerAddr();

#endif // __OGE_APP_PARAM_H_INCLUDED__
