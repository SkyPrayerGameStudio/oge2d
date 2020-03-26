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

#ifndef __OGE_COMMON_H_INCLUDED__
#define __OGE_COMMON_H_INCLUDED__

#include <string>
#include <vector>
#include <list>
#include <map>

/*
#ifndef OGE_Log
#ifdef __OGE_WITH_SDL2__
#include <SDL.h>
#define OGE_Log SDL_Log
#else
#include <cstdio>
#define OGE_Log printf
#endif
#endif
*/

static const int _OGE_MAX_TEXT_LEN_  = 256;
static const int _OGE_MAX_LONG_TEXT_LEN_  = 1024;

static const std::string _OGE_DF_LOG_FILE_ = "log.txt";
static const std::string _OGE_EMPTY_STR_   = "";

/*================= Common definitions =======================*/

struct CogePoint
{
    int x;
    int y;
};
typedef std::vector<CogePoint> ogePointArray;
typedef std::vector<CogePoint*> ogePoints;

struct CogeRect
{
    int left;
    int top;
    int right;
    int bottom;
};

typedef std::list<CogeRect*> ogeRectList;

struct CogeCharBuf
{
    char buf[_OGE_MAX_TEXT_LEN_];
};

//typedef std::vector<CogeCharBuf> ogeCharBufArray;

typedef int (*ogeCommonFunction)();

typedef std::map<std::string, int> ogeStrIntMap;
typedef std::map<int, std::string> ogeIntStrMap;


/*================= Global functions =======================*/
/* format color */
//int OGE_FormatColor(int iRGBColor, int iBPP);

/* itoa ... */
char* OGE_itoa( int value, char* result, int base=10);
std::string OGE_itoa(int value, int base=10);

char* OGE_ftoa(float f, char* buf, int prec);
std::string OGE_ftoa(double f);

int OGE_htoi(std::string &str);

void OGE_StrReplace(std::string &str, const char* string_to_replace, const char* new_string);

void OGE_StrTrimLeft(std::string& str);
void OGE_StrTrimRight(std::string& str);
void OGE_StrTrim(std::string& str);
std::string OGE_TrimSpecifiedStr(std::string const& source, char const* delims = " \t\r\n");

int OGE_StrSplit(const std::string& src, const std::string& delims, std::vector<std::string>& strs);

bool OGE_IsFileExisted(const char* sFileName);
int OGE_GetFileSize(const char* sFileName);

void OGE_AppendTextToFile(const char* sFileName, const char* sText);

std::string OGE_GetCurrentTimeStr();

bool OGE_GetCurrentTime(int& year, int& month, int& day, int& hour, int& min, int& sec);

std::string OGE_GetCurrentDir();

//void OGE_ResetStdIO();

int OGE_BufPutByte(char* buf, int idx, char value);
int OGE_BufPutInt(char* buf, int idx, int value);
int OGE_BufPutStr(char* buf, int idx, const std::string& value);
char OGE_BufGetByte(char* buf, int idx);
int OGE_BufGetInt(char* buf, int idx);
std::string OGE_BufGetStr(char* buf, int idx, int len);

void OGE_Log(const char *fmt, ...);

#endif // __OGE_COMMON_H_INCLUDED__
