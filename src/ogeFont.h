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

#ifndef __OGE_FONT_H_INCLUDED__
#define __OGE_FONT_H_INCLUDED__

#include "SDL.h"
#include "SDL_ttf.h"

#include <string>
#include <vector>
#include <map>

class CogeFont;

class CogeFontStock
{
private:

    typedef std::map<std::string, CogeFont*> ogeFontMap;

    ogeFontMap m_FontMap;

    //CogeFont* m_pCurrentFont;

    int m_iState;

    std::string m_sDefaultCharset;
    std::string m_sSystemCharset;

    bool m_bFontSystemIsOK;
    bool m_bIconvIsOpen;

    CogeFont* FindFont(const std::string& sFontName);

    void DelAllFont();


protected:

public:

    explicit CogeFontStock(std::string sDefaultCharset = "");
    ~CogeFontStock();

    //int Initalize();
    //void Finalize();

    bool IsFontSystemOK();

    const std::string& GetDefaultCharset();
    void SetDefaultCharset(const std::string& sDefaultCharset);

    const std::string& GetSystemCharset();
    void SetSystemCharset(const std::string& sSystemCharset);

    int StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer);
    std::string UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName);

    CogeFont* NewFont(const std::string& sFontName, int iFontSize, const std::string& sFileName);
    CogeFont* NewFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize);
    CogeFont* FindFont(const std::string& sFontName, int iFontSize);
    CogeFont* GetFont(const std::string& sFontName, int iFontSize, const std::string& sFileName);
    CogeFont* GetFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize);
    int DelFont(const std::string& sFontName, int iFontSize);

    //int PrepareFont(const std::string& sFontName);
    //SDL_Surface* PrintText(const std::string& sText, SDL_Color TheColor, int iRGBColor);


    friend class CogeFont;

};


class CogeFont
{
private:

    CogeFontStock*  m_pFontStock;

    TTF_Font*       m_pFont;

    SDL_Surface*    m_pTextImg;

    std::vector<SDL_Surface*> m_TextImgs;

    std::string     m_sName;

    //std::string     m_sLastText;
    //char m_LastText[_OGE_MAX_TXT_BUF_];
    //int m_iLastColorRGB;

    void* m_pDataBuf;

    int m_iState;

    int m_iFontSize;

    int m_iTotalUsers;

    int TestLineLength(char* src, char* line, int iStart, int iStep, int iMaxLen, int iMaxWidth);


protected:

public:

    CogeFont(const std::string& sName, int iFontSize, CogeFontStock* pFontStock);
    ~CogeFont();

    void Hire();
    void Fire();

    int GetState();
    int GetFontSize();
    const std::string& GetName();

    void* GetDataBuffer();
    void SetDataBuffer(void* pData);

    bool GetBufferTextSize(const char* pBufferText, int iBufferSize, const char* pCharsetName, int* iWidth, int* iHeight);
    SDL_Surface* PrintBufferText(const char* pBufferText, int iBufferSize, const char* pCharsetName, SDL_Color TheColor);
    bool PrintBufferTextWrap(const char* pBufferText, int iBufferSize, const char* pCharsetName, int iMaxWidth, SDL_Color TheColor);

    bool GetTextSize(const std::string& sText, int* iWidth, int* iHeight);
    SDL_Surface* PrintText(const std::string& sText, SDL_Color TheColor);
    bool PrintTextWrap(const std::string& sText, int iMaxWidth, SDL_Color TheColor);

    const std::vector<SDL_Surface*>& GetTextImages() const;


    friend class CogeFontStock;

};


#endif // __OGE_FONT_H_INCLUDED__
