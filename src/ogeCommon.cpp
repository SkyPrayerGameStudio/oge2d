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

#include "ogeCommon.h"

#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <ctime>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>

#include "ogeAppParam.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifndef OGE_M_GetCurrentDir
#ifdef __WIN32__
    #include <direct.h>
    #define OGE_M_GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define OGE_M_GetCurrentDir getcwd
#endif
#endif

static const int _OGE_MAX_CMM_TEXT_LEN_  = 256;

static char g_cmmtmpbuf[_OGE_MAX_CMM_TEXT_LEN_];

static char g_cmmlogbuf[_OGE_MAX_CMM_TEXT_LEN_];

/* format color */
/*
int OGE_FormatColor(int iRGBColor, int iBPP)
{
    switch(iBPP)
	{
	case 32:
		return iRGBColor;
		break;
	case 24:
		return iRGBColor & 0x00ffffff;
		break;
	case 16:
		return ((((iRGBColor&0x00ff0000)>>16) << 5 >> 8) << 11) |
			   ((((iRGBColor&0x0000ff00)>>8) << 6 >> 8) << 5)   |
			   ((iRGBColor&0x000000ff) << 5 >> 8);
		break;
	case 15:
		return ((((iRGBColor&0x00ff0000)>>16) << 5 >> 8) << 10) |
			   ((((iRGBColor&0x0000ff00)>>8) << 5 >> 8) << 5)   |
			   ((iRGBColor&0x000000ff) << 5 >> 8);
		break;
	default:
		return 0;

	}
}
*/

char* OGE_itoa( int value, char* result, int base )
{
    // check that the base if valid

	if (base < 2 || base > 16) { *result = 0; return result; }

	char* out = result;

	int quotient = value;

	do {

		*out = "0123456789abcdef"[ std::abs( quotient % base ) ];

		++out;

		quotient /= base;

	} while ( quotient );

	// Only apply negative sign for base 10

	if ( value < 0 && base == 10) *out++ = '-';

	std::reverse( result, out );

	*out = 0;

	return result;

}

std::string OGE_itoa(int value, int base)
{
	enum { kMaxDigits = 35 };

	std::string buf;

	buf.reserve( kMaxDigits ); // Pre-allocate enough space.

	// check that the base if valid

	if (base < 2 || base > 16) return buf;

	int quotient = value;

	// Translating number to string with base:

	do {

		buf += "0123456789abcdef"[ std::abs( quotient % base ) ];

		quotient /= base;

	} while ( quotient );

	// Append the negative sign for base 10

	if ( value < 0 && base == 10) buf += '-';

	std::reverse( buf.begin(), buf.end() );

	return buf;

}

char* OGE_ftoa(float f, char* buf, int prec)
{
	register char *bp;
	register int exp, digit;

	prec = (prec <= 0) ? 0 : (prec <= 9) ? prec : 9;
	bp = buf;
	//f = ffptof(fl);	/* get floating point value */
	if (f < 0.0) {		/* negative float */
		*bp++ = '-';
		f = -f;		/* make it positive */
	}
	if (f == 0.0) {
		*bp++ = '0';
		if(prec) {
			*bp++ = '.';
			while (prec--)
				*bp++ = '0';
		}
		*bp = 0;
		return(buf);
	}
	for (exp=0; f < 1.0; f = f * 10.0)	/* get negative exp */
		exp--;
	for ( ; f >= 1.0; f = f / 10.0)		/* 0.XXXXXXE00 * 10^exp */
		exp++;

	if (exp<=0)	/* one significant digit */
		*bp++ = '0';
	for ( ; exp>0; exp--) {	/* get significant digits */
		f = f * 10.0;
		digit = (int)f;	/* get one digit */
		f = f - digit;
		*bp++ = digit + '0';
	}
	if(prec) {
		*bp++ = '.';
		for( ; exp<0 && prec; prec--, exp++)	/* exp < 0 ? */
			*bp++ = '0';
		while(prec-- > 0) {
			f = f * 10.0;
			digit = (int)f;	/* get one digit */
			f = f - digit;
			*bp++ = digit + '0';
		}
	}
	*bp = 0;
	return(buf);
}

std::string OGE_ftoa(double f)
{
    std::ostringstream o; o << f;
    return o.str();
}

int OGE_htoi(std::string &str)
{
    if(str.length() <= 0) return 0;

    std::string s = str;

    if(s[0] == '#' || s[0] == '$') s = s.substr(1);
    if(s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        s = s.substr(2);
    }

    if(s.length() <= 0) return 0;

    unsigned int x;
    std::stringstream ss;
    ss << std::hex << s;
    ss >> x;

    return x;
}

void OGE_StrReplace(std::string &str, const char* string_to_replace, const char* new_string)
{
    // Find the first string to replace
    unsigned int index = str.find(string_to_replace);
    // while there is one
    while(index != std::string::npos)
    {
        // Replace it
        str.replace(index, strlen(string_to_replace), new_string);
        // Find the next one
        index = str.find(string_to_replace, index + strlen(new_string));
    }
}

void OGE_StrTrimLeft(std::string& str)
{
    const char* c = str.c_str();
    const char* s = c + str.size();
    while (c < s && isspace(*c)) ++c;
    str.erase(0,c-str.c_str());
}
void OGE_StrTrimRight(std::string& str)
{
    const char* c = str.c_str();
    const char* s = c + str.size() - 1;
    while (c < s && isspace(*s)) --s;
    str.erase(s-c+1);
}
void OGE_StrTrim(std::string& str)
{
    OGE_StrTrimLeft(str);
    OGE_StrTrimRight(str);
}

std::string OGE_TrimSpecifiedStr(std::string const& source, char const* delims)
{
    std::string result(source);
    std::string::size_type index = result.find_last_not_of(delims);
    if(index != std::string::npos) result.erase(++index);
    index = result.find_first_not_of(delims);
    if(index != std::string::npos) result.erase(0, index);
    else result.erase();
    return result;
}

int OGE_StrSplit(const std::string& src, const std::string& delims, std::vector<std::string>& strs)
{
    strs.clear();

    int iLen = delims.size();
    int iLastPos = 0, iIndex = -1;
    while (-1 != (iIndex = src.find(delims,iLastPos)))
    {
        strs.push_back(src.substr(iLastPos, iIndex - iLastPos));
        iLastPos = iIndex + iLen;
    }

    std::string sLastString = src.substr(iLastPos);

    //if (!sLastString.empty())
    strs.push_back(sLastString);

    return strs.size();
}

bool OGE_IsFileExisted(const char* sFileName)
{
    std::ifstream infile;
    infile.open(sFileName);
    if (infile.is_open())
    {
        infile.close();
        return true;
    }
    else return false;
}

int OGE_GetFileSize(const char* sFileName)
{
    int iRsl = 0;
    std::ifstream infile;
    infile.open(sFileName);
    if (infile.is_open())
    {
        infile.seekg(0, std::ios::end);
		iRsl = infile.tellg();
		infile.close();
    }
    return iRsl;
}

void OGE_AppendTextToFile(const char* sFileName, const char* sText)
{
    std::ofstream logfile(sFileName, std::ios::out | std::ios::app);
	if (!logfile.fail())
	{
		logfile << sText << "\n";
		logfile.close();
	}
}

std::string OGE_GetCurrentTimeStr()
{
    std::string rsl;

    time_t rawtime;
    struct tm * timeinfo;
    char buffer [20];

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

    rsl = buffer;

    return rsl;

}

bool OGE_GetCurrentTime(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    year  = timeinfo->tm_year + 1900;
    month = timeinfo->tm_mon  + 1;
    day   = timeinfo->tm_mday;
    hour  = timeinfo->tm_hour;
    min   = timeinfo->tm_min;
    sec   = timeinfo->tm_sec;

    if(sec > 59) sec = 59;

    return true;
}

std::string OGE_GetCurrentDir()
{
    std::string rsl;
    char buffer [FILENAME_MAX];
    if(OGE_M_GetCurrentDir(buffer, sizeof(buffer))) rsl = buffer;
    else rsl = "";
    return rsl;
}


/*
void OGE_ResetStdIO()
{
#if defined(__WIN32__)
    freopen("CON","r",stdin);
    freopen("CON","w",stdout);
    freopen("CON","w",stderr);
#elif defined(__LINUX__)
    freopen("/dev/tty","r",stdin);
    freopen("/dev/tty","w",stdout);
    freopen("/dev/tty","w",stderr);
#else
// do nothing ...
#endif

}
*/

int OGE_BufPutByte(char* buf, int idx, char value)
{
    buf[idx] = value;
    return idx+1;
}
int OGE_BufPutInt(char* buf, int idx, int value)
{
    int len = sizeof(int);
    memcpy((void*)(buf+idx), (void*)(&value), len);
    return idx+len;
}
int OGE_BufPutStr(char* buf, int idx, const std::string& value)
{
    int len = value.length();
    memcpy((void*)(buf+idx), (void*)(value.c_str()), len);
    return idx+len;
}
char OGE_BufGetByte(char* buf, int idx)
{
    if(buf == NULL) return 0;
    return *(buf + idx);
}
int OGE_BufGetInt(char* buf, int idx)
{
    if(buf == NULL) return 0;
    int dst = 0; int len = sizeof(int);
    memcpy((void*)(&dst), (void*)(buf + idx), len);
    return dst;
}
std::string OGE_BufGetStr(char* buf, int idx, int len)
{
    if(buf == NULL) return "";

    //char dst[len+1];
    //memcpy((void*)(&dst), (void*)(buf + idx), len);
    //dst[len] = '\0';

    memcpy((void*)(&g_cmmtmpbuf), (void*)(buf + idx), len);
    g_cmmtmpbuf[len] = '\0';

    return std::string(g_cmmtmpbuf);
}

void OGE_Log(const char *fmt, ...)
{
    va_list args;
	va_start(args, fmt);

	vsprintf(g_cmmlogbuf, fmt, args);

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "OGE2D", g_cmmlogbuf);
#else
    printf(g_cmmlogbuf);
    fflush(stdout);
#endif

#ifdef __LOG_TO_FILE__
    std::string sLogFile = OGE_GetAppGameDir() + "/" + _OGE_DF_LOG_FILE_;
    OGE_AppendTextToFile(sLogFile.c_str(), g_cmmlogbuf);
#endif

	va_end(args);

}
