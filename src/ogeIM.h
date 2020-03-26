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

#ifndef __OGE_IM_H_INCLUDED__
#define __OGE_IM_H_INCLUDED__

#include <string>
#include <map>

#define _OGE_MAX_IM_BUF_  256

class CogeTextInputter;

class CogeIM
{
private:

    typedef std::map<std::string, CogeTextInputter*> ogeTextInputterMap;

    ogeTextInputterMap   m_InputterMap;
    CogeTextInputter*    m_pActiveInputter;

    void* m_pMainWindow;

    int m_iState;

    int m_iPlatform;

    void DelAllInputters();


protected:

public:

    explicit CogeIM(void* pMainWindow);
    ~CogeIM();

    int Initialize();
    void Finalize();

    int GetState();

    void DisableIM();

    void SetUnicodeMode(bool bIsAllUnicode);
    bool GetUnicodeMode();

    bool IsInputMethodReady();

    void ShowSoftKeyboard(bool bShow);
    bool IsSoftKeyboardShown();

    bool HandleInput(void* pEventData, int iMappedKey, int iKeyStateFlag);

    CogeTextInputter* GetActiveInputter();
    void SetActiveInputter(CogeTextInputter* inputter);

    CogeTextInputter* NewInputter(const std::string& sInputterName);
    CogeTextInputter* FindInputter(const std::string& sInputterName);
    CogeTextInputter* GetInputter(const std::string& sInputterName);

    int DelInputter(const std::string& sInputterName);

    friend class CogeTextInputter;

};

class CogeTextInputter
{
private:

    CogeIM*        m_pIM;

    std::string    m_sName;

    char m_text[_OGE_MAX_IM_BUF_];

    //int m_iCharWidth;
    int m_iCaretPos;

    int m_iTextLength;

    int m_iInputWinPosX;
    int m_iInputWinPosY;

    int m_iTotalUsers;


protected:

public:

    CogeTextInputter(CogeIM* pIM, const std::string& sName);
    ~CogeTextInputter();

    const std::string& GetName();

    void Hire();
    void Fire();

    //std::string GetText();

    char* GetBufferText();

    int GetTextLength();
    int SetTextLength(int iLen);

    int GetCaretPos();

    //int GetRelatedSprId();
    //void SetRelatedSprId(int iSprId);

    void SetInputWinPos(int iPosX, int iPosY);
    int GetInputWinPosX();
    int GetInputWinPosY();

    void Clear();

    void Reset();

    void Focus();

    void StoreChar(char ch);
    void StoreDBCSChar(wchar_t ch);

    void MoveCaret2Left();
    void MoveCaret2Right();
    void MoveCaret2Head();
    void MoveCaret2Tail();
    void DeleteLeftChar();
    void DeleteRightChar();


    friend class CogeIM;

};


#endif // __OGE_IM_H_INCLUDED__
