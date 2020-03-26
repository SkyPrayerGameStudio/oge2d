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

#include "ogeFont.h"

#include "ogeCommon.h"

#include "iconv.h"

#define _CHECK_LEN_STEP_          8

/*
static TTF_Font* OGE_OpenFont(const char* filepath, int fontsize)
{
    TTF_Font* pResult = NULL;

#ifdef __OGE_WITH_SDL2__

    SDL_RWops *file = SDL_RWFromFile(filepath, "rb");
    if(file) pResult = TTF_OpenFontRW(file, 1, fontsize);

#else

    pResult = TTF_OpenFont(filepath, fontsize);

#endif

    return pResult;
}
*/

static Uint16 unicode_text[BUFSIZ*2];
static char src_text[BUFSIZ*4];
static char dst_text[BUFSIZ*4];
static char line_text[BUFSIZ*4];

static const char* g_UTF8  = "UTF-8";
static const char* g_UTF16 = "UTF-16";

/*------------------ CogeFontStock ------------------*/

CogeFontStock::CogeFontStock(std::string sDefaultCharset)
{
    m_iState = -1;

    //m_bFontSystemIsOK = false;

    m_sDefaultCharset = sDefaultCharset;
    m_sSystemCharset = "";

    m_bFontSystemIsOK = TTF_Init() >= 0;

    if(!m_bFontSystemIsOK) OGE_Log("Could not initialize TTF: %s\n",SDL_GetError());
    else m_iState = 0;

}

CogeFontStock::~CogeFontStock()
{
    DelAllFont();

    m_iState = -1;

    if(m_bFontSystemIsOK) TTF_Quit();
}

bool CogeFontStock::IsFontSystemOK()
{
    return m_bFontSystemIsOK;
}

const std::string& CogeFontStock::GetDefaultCharset()
{
    return m_sDefaultCharset;
}
void CogeFontStock::SetDefaultCharset(const std::string& sDefaultCharset)
{
    m_sDefaultCharset = sDefaultCharset;
}

const std::string& CogeFontStock::GetSystemCharset()
{
    return m_sSystemCharset;
}
void CogeFontStock::SetSystemCharset(const std::string& sSystemCharset)
{
    m_sSystemCharset = sSystemCharset;
}

int CogeFontStock::StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer)
{
    const char* pCharsetName = sCharsetName.c_str();
    const char* pBufferText = sText.c_str();

    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    bool bIsUnicode = strcmp(g_UTF16, pCharsetName) == 0;

    if (!bIsUnicode)
    {
        iconv_t cd;

        if ((cd = iconv_open(g_UTF16, pCharsetName)) == (iconv_t)-1)
        {
            return 0;
        }

        const char *from_str = pBufferText;
        char *to_str = (char*)unicode_text;
        size_t from_sz = iBufferSize;
        size_t to_sz = sizeof(unicode_text);

        //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

        int res = -1;

#ifdef ICONV_CONST_INPUT
        res = iconv(cd, &from_str, &from_sz, &to_str, &to_sz);
#else
        res = iconv(cd, (char**)(&from_str), &from_sz, &to_str, &to_sz);
#endif

        iconv_close(cd);

        if (res == -1) return 0; //printf("Could not use iconv\n");

        to_sz = sizeof(unicode_text) - to_sz;

        memcpy(pUnicodeBuffer, (char*)unicode_text, to_sz);

        /*
        printf("\nStringToUnicode: ");
        for(size_t i=0; i<to_sz; i++)
        {
            printf("%d, ", pUnicodeBuffer[i]);
        }
        printf("\n");
        */

        return to_sz;

    }
    else
    {
        return 0;
    }

}

std::string CogeFontStock::UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName)
{
    std::string rsl = "";

    if(iBufferSize < 2) return rsl;

    const unsigned char* pBufferText = (unsigned char*)pUnicodeBuffer;

    bool bHasHead = (pBufferText[0] == 0xfe && pBufferText[1] == 0xff) || (pBufferText[0] == 0xff && pBufferText[1] == 0xfe);
    bool bHasTail = pBufferText[iBufferSize-2] == '\0' && pBufferText[iBufferSize-1] == '\0';

    if(!bHasHead)
    {
        char* p = &(src_text[2]);
        memcpy(p, pBufferText, iBufferSize);
        src_text[0] = 0xff; src_text[1] = 0xfe; // use "0xff, 0xfe" as default order ...
        iBufferSize = iBufferSize + 2;
    }
    else memcpy(src_text, pBufferText, iBufferSize);

    if(!bHasTail)
    {
        src_text[iBufferSize] = '\0';
        src_text[iBufferSize+1] = '\0';
        iBufferSize = iBufferSize + 2;
    }

    /*
        printf("\n UnicodeToString: ");
        for(size_t i=0; i<iBufferSize; i++)
        {
            printf("%d, ", pUnicodeBuffer[i]);
        }
        printf("\n");
    */


    const char* pCharsetName = sCharsetName.c_str();

    bool bIsUnicode = strcmp(g_UTF16, pCharsetName) == 0;

    if (!bIsUnicode)
    {
        iconv_t cd;

        if ((cd = iconv_open(pCharsetName, g_UTF16)) == (iconv_t)-1)
        {
            return rsl;
        }

        const char *from_str = src_text;
        char *to_str = (char*)dst_text;
        size_t from_sz = iBufferSize;
        size_t to_sz = sizeof(dst_text);

        //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

        int res = -1;

#ifdef ICONV_CONST_INPUT
        res = iconv(cd, &from_str, &from_sz, &to_str, &to_sz);
#else
        res = iconv(cd, (char**)(&from_str), &from_sz, &to_str, &to_sz);
#endif

        iconv_close(cd);

        if (res == -1) return rsl; //printf("Could not use iconv\n");

        to_sz = sizeof(dst_text) - to_sz;

        dst_text[to_sz] = '\0';

        rsl = dst_text;

        return rsl;

    }
    else
    {
        return rsl;
    }
}

CogeFont* CogeFontStock::NewFont(const std::string& sFontName, int iFontSize, const std::string& sFileName)
{
    if(!m_bFontSystemIsOK) return NULL;

    CogeFont* pNewFont = NULL;

    ogeFontMap::iterator it;

    char c[8];
    sprintf(c,"%d",iFontSize);
    std::string sFontSize(c);

    std::string sRealFontName = sFontName + "_" + sFontSize;

    it = m_FontMap.find(sRealFontName);
	if (it != m_FontMap.end())
	{
        OGE_Log("The Font name (with size) '%s' is in use.\n", sRealFontName.c_str());
        return NULL;
	}

	if (sFileName.length() > 0 && OGE_IsFileExisted(sFileName.c_str()))
	{
	    pNewFont = new CogeFont(sRealFontName, iFontSize, this);
	    //pNewFont->m_pFont = OGE_OpenFont(sFileName.c_str(), iFontSize);
	    pNewFont->m_pFont = TTF_OpenFont(sFileName.c_str(), iFontSize);
		if(pNewFont->m_pFont == NULL)
        {
            delete pNewFont;
            pNewFont = NULL;
            OGE_Log("Unable to load font file(%s): %s\n", sFileName.c_str(), SDL_GetError());
            return NULL;
        }

	}
	else return NULL;

	pNewFont->m_iState = 0;

	//TTF_SetFontStyle(pNewFont->m_pFont, TTF_STYLE_BOLD);

	m_FontMap.insert(ogeFontMap::value_type(sRealFontName, pNewFont));

	//if(m_pCurrentFont == NULL) m_pCurrentFont = pNewFont;

	return pNewFont;

}

CogeFont* CogeFontStock::NewFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize)
{
    if(!m_bFontSystemIsOK) return NULL;

    CogeFont* pNewFont = NULL;

    ogeFontMap::iterator it;

    char c[8];
    sprintf(c,"%d",iFontSize);
    std::string sFontSize(c);

    std::string sRealFontName = sFontName + "_" + sFontSize;

    it = m_FontMap.find(sRealFontName);
	if (it != m_FontMap.end())
	{
        OGE_Log("The Font name (with size) '%s' is in use.\n", sRealFontName.c_str());
        return NULL;
	}

	if (pBuffer && iBufferSize > 0)
	{
	    pNewFont = new CogeFont(sRealFontName, iFontSize, this);
	    pNewFont->m_pFont = TTF_OpenFontRW(SDL_RWFromMem((void*)pBuffer, iBufferSize), 0, iFontSize);
		if(pNewFont->m_pFont == NULL)
        {
            delete pNewFont;
            pNewFont = NULL;
            OGE_Log("Unable to load font from buffer: %s\n", SDL_GetError());
            return NULL;
        }

	}
	else return NULL;

	pNewFont->m_iState = 0;

	m_FontMap.insert(ogeFontMap::value_type(sRealFontName, pNewFont));

	//if(m_pCurrentFont == NULL) m_pCurrentFont = pNewFont;

	return pNewFont;
}

CogeFont* CogeFontStock::FindFont(const std::string& sFontName)
{
    ogeFontMap::iterator it;

    it = m_FontMap.find(sFontName);
	if (it != m_FontMap.end()) return it->second;
	else return NULL;

}

CogeFont* CogeFontStock::FindFont(const std::string& sFontName, int iFontSize)
{
    char c[8];
    sprintf(c,"%d",iFontSize);
    std::string sFontSize(c);

    std::string sRealFontName = sFontName + "_" + sFontSize;
    return FindFont(sRealFontName);
}

CogeFont* CogeFontStock::GetFont(const std::string& sFontName, int iFontSize, const std::string& sFileName)
{
    CogeFont* pMatchedFont = FindFont(sFontName, iFontSize);
    if (!pMatchedFont) pMatchedFont = NewFont(sFontName, iFontSize, sFileName);
    //if (pMatchedImage) pMatchedImage->Hire();
    return pMatchedFont;
}

CogeFont* CogeFontStock::GetFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize)
{
    CogeFont* pMatchedFont = FindFont(sFontName, iFontSize);
    if (!pMatchedFont) pMatchedFont = NewFontFromBuffer(sFontName, iFontSize, pBuffer, iBufferSize);
    return pMatchedFont;
}

int CogeFontStock::DelFont(const std::string& sFontName, int iFontSize)
{
    CogeFont* pMatchedFont = FindFont(sFontName, iFontSize);
	if (pMatchedFont)
	{
	    //pMatchedFont->Stop();
	    //if(m_pCurrentFont == pMatchedFont) m_pCurrentFont = NULL;

        if (pMatchedFont->m_iTotalUsers > 0) return 0;
		m_FontMap.erase(sFontName);
		delete pMatchedFont;
		return 1;
	}
	return -1;
}

void CogeFontStock::DelAllFont()
{
    //m_pCurrentFont = NULL;

    ogeFontMap::iterator it;
	CogeFont* pMatchedFont = NULL;

	it = m_FontMap.begin();

	while (it != m_FontMap.end())
	{
		pMatchedFont = it->second;
		m_FontMap.erase(it);
		delete pMatchedFont;
		it = m_FontMap.begin();
	}

}

/*
int CogeFontStock::PrepareFont(const std::string& sFontName)
{
    CogeFont* pFont = FindFont(sFontName);
    if(pFont == NULL) return -1;
    if(m_pCurrentFont == pFont) return 0;
    m_pCurrentFont = pFont;
    return 1;

}

SDL_Surface* CogeFontStock::PrintText(const std::string& sText, SDL_Color TheColor, int iRGBColor)
{
    if(m_pCurrentFont)
    {
        return m_pCurrentFont->PrintText( sText, TheColor, iRGBColor );
    }
    else return NULL;
}
*/


/*------------------ CogeFont ------------------*/

CogeFont::CogeFont(const std::string& sName, int iFontSize, CogeFontStock* pFontStock):
m_pFontStock(pFontStock),
m_pFont(NULL),
m_pTextImg(NULL),
m_sName(sName),
//m_sLastText(""),
m_iFontSize(iFontSize)
{
    //m_iLastColorRGB = 0;

    m_iState = -1;

    m_iTotalUsers = 0;

    //m_iPlayState = -1;
}

CogeFont::~CogeFont()
{
    //if(m_pFontStock && m_pFont)
    //{
    //    if(m_pFontStock->m_pCurrentFont == this)
    //    {
    //        m_pFontStock->m_pCurrentFont = NULL;
    //    }
    //}

    if(m_pFont)
    {
        TTF_CloseFont(m_pFont);
        m_pFont = NULL;
    }

    if(m_pTextImg)
    {
        SDL_FreeSurface( m_pTextImg );
        m_pTextImg  = NULL;
    }

    for(size_t i = 0; i < m_TextImgs.size(); i++)
    {
        SDL_Surface* tmpimg = m_TextImgs[i];
        if(tmpimg) SDL_FreeSurface(tmpimg);
        m_TextImgs[i] = NULL;
    }
    m_TextImgs.clear();

    m_iState = -1;

}

void CogeFont::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}
void CogeFont::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}

const std::string& CogeFont::GetName()
{
    return m_sName;
}

int CogeFont::GetState()
{
    return m_iState;
}

int CogeFont::GetFontSize()
{
    return m_iFontSize;
}

void* CogeFont::GetDataBuffer()
{
    return m_pDataBuf;
}
void CogeFont::SetDataBuffer(void* pData)
{
    m_pDataBuf = pData;
}

int CogeFont::TestLineLength(char* src, char* line, int iStart, int iStep, int iMaxLen, int iMaxWidth)
{
    //if(m_pFont == NULL) return 0;

    if(iStep == 0 || iStart==iMaxLen) // can not add more ...
    {
        return iStart;
    }

    if(iStart + iStep*2 > iMaxLen)
    {
        iStep = iStep / 2;
        return TestLineLength(src, line, iStart, iStep, iMaxLen, iMaxWidth);
    }
    else
    {
        //if (iStart + iStep == iMaxLen)
        //{
        //}
    }
    memcpy(line+iStart, src+iStart, iStep * 2);
    int iCurrentWidth = 0; int iCurrentHeight = 0;
    TTF_SizeUNICODE(m_pFont, (Uint16*)line, &iCurrentWidth, &iCurrentHeight);

    if(iCurrentWidth < iMaxWidth)
    {
        iStart = iStart + iStep*2;
        return TestLineLength(src, line, iStart, iStep, iMaxLen, iMaxWidth);
    }
    else if(iCurrentWidth == iMaxWidth)
    {
        return iStart + iStep * 2;
    }
    else // if(iCurrentWidth > iMaxWidth)
    {
        memset(line+iStart, 0, iStep * 2);
        iStep = iStep / 2;
        return TestLineLength(src, line, iStart, iStep, iMaxLen, iMaxWidth);
    }

}

bool CogeFont::GetBufferTextSize(const char* pBufferText, int iBufferSize, const char* pCharsetName, int* iWidth, int* iHeight)
{
    if(m_pFont)
    {
        bool bIsUnicode = strcmp(g_UTF16, pCharsetName) == 0;

        if (!bIsUnicode)
        {
            iconv_t cd;

            //if ((cd = iconv_open("UTF-16", "char")) == (iconv_t)-1 &&
            //    (cd = iconv_open("UTF-16", ""))     == (iconv_t)-1)
            if ((cd = iconv_open(g_UTF16, pCharsetName)) == (iconv_t)-1)
            {
                //printf("Could not open iconv.\n");
                return false;
                //m_bIconvIsOpen = false;
            }
            //else m_bIconvIsOpen = true;

            //memset(&unicode_text[0], 0, sizeof(unicode_text));

            const char *from_str = pBufferText;
            char *to_str = (char*)unicode_text;
            size_t from_sz = iBufferSize;
            size_t to_sz = sizeof(unicode_text);

            if(from_str[iBufferSize-1] != '\0')
            {
                memcpy(src_text, pBufferText, iBufferSize);
                src_text[iBufferSize] = '\0';
                from_str = src_text;
                from_sz = from_sz + 1;
            }

            //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

            int res = -1;

#ifdef ICONV_CONST_INPUT
            res = iconv(cd, &from_str, &from_sz, &to_str, &to_sz);
#else
            res = iconv(cd, (char**)(&from_str), &from_sz, &to_str, &to_sz);
#endif

            iconv_close(cd);

            if (res == -1) return false; //printf("Could not use iconv\n");

        }
        else
        {
            const char *from_str = pBufferText;
            char *to_str = (char*)unicode_text;

            size_t from_sz = iBufferSize;

            memcpy(to_str, from_str, from_sz);

            to_str[iBufferSize] = '\0';
            to_str[iBufferSize + 1] = '\0'; // ensure it has end chars ...

        }

        TTF_SizeUNICODE(m_pFont, unicode_text, iWidth, iHeight);

        return true;


    }
    else return false;
}

SDL_Surface* CogeFont::PrintBufferText(const char* pBufferText, int iBufferSize, const char* pCharsetName, SDL_Color TheColor)
{
    if(m_pFont)
    {
        //return TTF_RenderText_Solid( m_pFont, sText.c_str(), TheColor );
        //return TTF_RenderUTF8_Solid( m_pFont, sText.c_str(), TheColor );

        //if (m_pTextImg == NULL || m_iLastColorRGB != iRGBColor || m_sLastText != sText)
        //{

        //memset(&unicode_text[0], 0, sizeof(unicode_text));

        //printf("CharsetName: %s\n", pCharsetName);

        bool bIsUnicode = strcmp(g_UTF16, pCharsetName) == 0;

        if (!bIsUnicode)
        {
            iconv_t cd;

            //if ((cd = iconv_open("UTF-8", "char")) == (iconv_t)-1 &&
            //    (cd = iconv_open("UTF-8", ""))     == (iconv_t)-1)
            //if ((cd = iconv_open("UTF-16", "char")) == (iconv_t)-1 &&
            //    (cd = iconv_open("UTF-16", ""))     == (iconv_t)-1)
            if ((cd = iconv_open(g_UTF16, pCharsetName)) == (iconv_t)-1)
            {
                //printf("Could not open iconv.\n");
                return NULL;
                //m_bIconvIsOpen = false;
            }
            //else m_bIconvIsOpen = true;

            //memset(&unicode_text[0], 0, sizeof(unicode_text));

            const char *from_str = pBufferText;
            char *to_str = (char*)unicode_text;
            size_t from_sz = iBufferSize;
            size_t to_sz = sizeof(unicode_text);

            if(from_str[iBufferSize-1] != '\0')
            {
                memcpy(src_text, pBufferText, iBufferSize);
                src_text[iBufferSize] = '\0';
                from_str = src_text;
                from_sz = from_sz + 1;
            }

            //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

            int res = -1;

#ifdef ICONV_CONST_INPUT
            res = iconv(cd, &from_str, &from_sz, &to_str, &to_sz);
#else
            res = iconv(cd, (char**)(&from_str), &from_sz, &to_str, &to_sz);
#endif

            iconv_close(cd);

            if (res == -1)
            {
                //printf("Could not use iconv\n");
                return NULL;
            }

        }
        else
        {

            const char *from_str = pBufferText;
            char *to_str = (char*)unicode_text;

            size_t from_sz = iBufferSize;

            memcpy(to_str, from_str, from_sz);

            to_str[iBufferSize] = '\0';
            to_str[iBufferSize + 1] = '\0'; // ensure it has end chars ...

        }

        //int iCurrentWidth = 0; int iCurrentHeight = 0;
        //TTF_SizeUNICODE(m_pFont, unicode_text, &iCurrentWidth, &iCurrentHeight);
        //printf("text w: %d, h: %d", iCurrentWidth, iCurrentHeight);

        if(m_pTextImg) SDL_FreeSurface( m_pTextImg );
        //m_pTextImg = TTF_RenderUNICODE_Solid(m_pFont, unicode_text, TheColor);
        //m_pTextImg = TTF_RenderUTF8_Solid(m_pFont, pBufferText, TheColor);
        m_pTextImg = TTF_RenderUNICODE_Blended(m_pFont, unicode_text, TheColor);

        //if(m_pTextImg)
        //{
        //    m_sLastText = sText;
        //    m_iLastColorRGB = iRGBColor;
        //}

        return m_pTextImg;

        //}
        //else return m_pTextImg;

    }
    else return NULL;
}

bool CogeFont::PrintBufferTextWrap(const char* pBufferText, int iBufferSize, const char* pCharsetName, int iMaxWidth, SDL_Color TheColor)
{
    if(m_pFont == NULL) return false;

    size_t bufsize = sizeof(unicode_text);

    size_t to_sz = bufsize;

    bool bIsUnicode = strcmp(g_UTF16, pCharsetName) == 0;

    if (!bIsUnicode)
    {
        iconv_t cd;

        //if ((cd = iconv_open("UTF-16", "char")) == (iconv_t)-1 &&
        //    (cd = iconv_open("UTF-16", ""))     == (iconv_t)-1)
        if ((cd = iconv_open(g_UTF16, pCharsetName)) == (iconv_t)-1)
        {
            //printf("Could not open iconv.\n");
            return false;
            //m_bIconvIsOpen = false;
        }

        const char *from_str = pBufferText;
        char *to_str = (char*)unicode_text;
        size_t from_sz = iBufferSize;

        if(from_str[iBufferSize-1] != '\0')
        {
            memcpy(src_text, pBufferText, iBufferSize);
            src_text[iBufferSize] = '\0';
            from_str = src_text;
            from_sz = from_sz + 1;
        }

        //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

#ifdef ICONV_CONST_INPUT
        int res = iconv(cd, &from_str, &from_sz, &to_str, &to_sz);
#else
        int res = iconv(cd, (char**)(&from_str), &from_sz, &to_str, &to_sz);
#endif

        iconv_close(cd);

        if (res == -1) return false; //printf("Could not use iconv\n");
    }
    else
    {
        const char *from_str = pBufferText;
        char *to_str = (char*)unicode_text;

        size_t from_sz = iBufferSize;

        memcpy(to_str, from_str, from_sz);

        to_str[iBufferSize] = '\0';
        to_str[iBufferSize + 1] = '\0'; // ensure it has end chars ...

        to_sz = from_sz;
    }

    //size_t to_sz = from_sz*2; //: sizeof(unicode_text) ? sizeof(unicode_text) > from_sz*2;

    //int iCurrentWidth = 0; int iCurrentHeight = 0;
    //TTF_SizeUNICODE(m_pFont, unicode_text, &iCurrentWidth, &iCurrentHeight);
    //printf("text w: %d, h: %d", iCurrentWidth, iCurrentHeight);

    char* buftxt = (char*)unicode_text;

    //char* src = new char(bufsize);
    //char* line = new char(bufsize);

    //std::vector<SDL_Surface*> tmpimgs;

    int len = bufsize - to_sz;
    int start = 0;
    SDL_Surface* tmp = NULL;

    for(size_t i = 0; i < m_TextImgs.size(); i++)
    {
        tmp = m_TextImgs[i];
        if(tmp) SDL_FreeSurface(tmp);
        m_TextImgs[i] = NULL;
    }
    m_TextImgs.clear();

    tmp = NULL;

    if(len <= _CHECK_LEN_STEP_*2)
    {
        //tmp = TTF_RenderUNICODE_Solid(m_pFont, unicode_text, TheColor);
        tmp = TTF_RenderUNICODE_Blended(m_pFont, unicode_text, TheColor);
        if(tmp) m_TextImgs.push_back(tmp);
        return (tmp != NULL);
    }

    int iLines = 0;

    while(len > 0)
    {
        memset(src_text, 0, bufsize);
        memset(line_text, 0, bufsize);

        iLines++;

        if(iLines == 1) memcpy(src_text, buftxt+start, len);
        else
        {
            memcpy(src_text, buftxt, 2);
            memcpy(src_text+2, buftxt+start, len);
            len = len + 2;
        }

        //int iCurrentWidth = 0; int iCurrentHeight = 0;
        //TTF_SizeUNICODE(m_pFont, unicode_text, &iCurrentWidth, &iCurrentHeight);
        //printf("text w: %d, h: %d", iCurrentWidth, iCurrentHeight);

        //break;

        int pos = TestLineLength(src_text, line_text, 0, _CHECK_LEN_STEP_, len, iMaxWidth);

        //tmp = TTF_RenderUNICODE_Solid(m_pFont, (Uint16*)line_text, TheColor);
        tmp = TTF_RenderUNICODE_Blended(m_pFont, (Uint16*)line_text, TheColor);
        if(tmp == NULL) break;
        m_TextImgs.push_back(tmp);

        start = start + pos;
        len = len - pos;

    }

    return (tmp != NULL);
}

bool CogeFont::GetTextSize(const std::string& sText, int* iWidth, int* iHeight)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    if(m_pFontStock->m_sDefaultCharset.length() > 0)
        return GetBufferTextSize(pBufferText, iBufferSize, m_pFontStock->m_sDefaultCharset.c_str(), iWidth, iHeight);
    else
        return GetBufferTextSize(pBufferText, iBufferSize, g_UTF8, iWidth, iHeight);
}

SDL_Surface* CogeFont::PrintText(const std::string& sText, SDL_Color TheColor)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    if(m_pFontStock->m_sDefaultCharset.length() > 0)
        return PrintBufferText(pBufferText, iBufferSize, m_pFontStock->m_sDefaultCharset.c_str(), TheColor);
    else
        return PrintBufferText(pBufferText, iBufferSize, g_UTF8, TheColor);
}

bool CogeFont::PrintTextWrap(const std::string& sText, int iMaxWidth, SDL_Color TheColor)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    if(m_pFontStock->m_sDefaultCharset.length() > 0)
        return PrintBufferTextWrap(pBufferText, iBufferSize, m_pFontStock->m_sDefaultCharset.c_str(), iMaxWidth, TheColor);
    else
        return PrintBufferTextWrap(pBufferText, iBufferSize, g_UTF8, iMaxWidth, TheColor);
}

const std::vector<SDL_Surface*>& CogeFont::GetTextImages() const
{
    return m_TextImgs;
}













