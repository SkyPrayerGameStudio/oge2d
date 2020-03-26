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

#include "ogeIM.h"

#include "ogeCommon.h"

#include "SDL.h"
#include "SDL_syswm.h"

static CogeIM* g_im = NULL;
static bool g_IsUnicodeIM = false;
static bool g_IsUsingIM = false;
static void* g_MainImWindow = NULL;

#if SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0)
static int g_ImUnicodeFlag = 0;
#endif

#if defined(__IPHONE__) && defined(__OGE_WITH_GLWIN__) && (SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0))
static SDL_GLContext g_glctx = NULL;
#endif


static int g_CharShiftMap[128];

int  OGE_InitIM();
void OGE_ResetInputWin(CogeTextInputter* inputter);
void OGE_HideInputWin();
void OGE_FiniIM();

bool OGE_HandleInputEvent(SDL_Event* pEvent, int iMappedKey, int iKeyStateFlag);

void OGE_HandleInputKey(CogeTextInputter* inputter, int iParam);
void OGE_HandleInputChar(CogeTextInputter* inputter, int iParam);
void OGE_HandleInputChange(CogeTextInputter* inputter);

bool OGE_IsDBCSLeadByte(char ch);
//bool OGE_IsDBCSTrailByte(char *base, char *p);

#if defined(__IPHONE__)
extern "C" DECLSPEC int SDLCALL SDL_iPhoneKeyboardShow(SDL_Window * window);
extern "C" DECLSPEC int SDLCALL SDL_iPhoneKeyboardHide(SDL_Window * window);
extern "C" DECLSPEC SDL_bool SDLCALL SDL_iPhoneKeyboardIsShown(SDL_Window * window);
//extern DECLSPEC int SDLCALL SDL_iPhoneKeyboardToggle(SDL_Window * window);
#endif

static int OGE_EnableUNICODE(int enable)
{

#if SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0)

    int previous = g_ImUnicodeFlag;

    switch (enable) {
    case 1:
        g_ImUnicodeFlag = 1;
        SDL_StartTextInput();
        break;
    case 0:
        g_ImUnicodeFlag = 0;
        SDL_StopTextInput();
        break;
    }
    return previous;

#else

    return SDL_EnableUNICODE(enable);

#endif

}


static void OGE_InitCharShiftMap()
{
    memset((void*)&g_CharShiftMap[0],  0, sizeof(int) * 128 );

    g_CharShiftMap['`'] = '~';
    g_CharShiftMap['1'] = '!';
    g_CharShiftMap['2'] = '@';
    g_CharShiftMap['3'] = '#';
    g_CharShiftMap['4'] = '$';
    g_CharShiftMap['5'] = '%';
    g_CharShiftMap['6'] = '^';
    g_CharShiftMap['7'] = '&';
    g_CharShiftMap['8'] = '*';
    g_CharShiftMap['9'] = '(';
    g_CharShiftMap['0'] = ')';

    g_CharShiftMap['-'] = '_';
    g_CharShiftMap['='] = '+';
    g_CharShiftMap['['] = '{';
    g_CharShiftMap[']'] = '}';
    g_CharShiftMap['\\'] = '|';
    g_CharShiftMap[';'] = ':';
    g_CharShiftMap['\''] = '"';
    g_CharShiftMap[','] = '<';
    g_CharShiftMap['.'] = '>';
    g_CharShiftMap['/'] = '?';

    for(int i=97; i<=122; i++) g_CharShiftMap[i] = i - 32;

    for(int i=0; i<128; i++)
    {
        if(g_CharShiftMap[i] != 0) g_CharShiftMap[g_CharShiftMap[i]] = i;
    }

}


#ifdef __OGE_WITH_IME__

#if defined(__WIN32__)

#include <assert.h>
#include <windows.h>
#include <imm.h>

static HWND wnd = 0;
static WNDPROC proc = 0;
#ifdef __IME_WITH_HOOK__
static HHOOK hook = 0;
#endif
static HIMC imc = 0;
static COMPOSITIONFORM comp = {0};
static DWORD prop = 0;
static BOOL opened = FALSE;

#ifdef __IME_WITH_HOOK__
static LRESULT CALLBACK GetMessageHook(int code, WPARAM wp, LPARAM lp)
{
    CogeTextInputter* inputter = NULL;

    if(g_im) inputter = g_im->GetActiveInputter();

    // take back useful messages, such as WM_CHAR, WM_INPUTLANGCHANGE ...
    if(inputter && code == HC_ACTION && wp == PM_REMOVE)
    {
        MSG* msg = (MSG*)lp;
        TranslateMessage(msg);
    }
    return CallNextHookEx(hook, code, wp, lp);
}
#endif

static LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CogeTextInputter* inputter = NULL;

    if(g_im) inputter = g_im->GetActiveInputter();

    if(inputter)
    switch(msg)
    {
        case WM_CREATE:
            break;

        case WM_MOVE:
            OGE_ResetInputWin(inputter);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            //DestroyWindow(hwnd);
            break;

        case WM_SETFOCUS:
            break;

        case WM_KILLFOCUS:
            break;

        case WM_KEYDOWN:
            //printf("WM_KEYDOWN\n");
            OGE_HandleInputKey(inputter, wp);
            break;

        case WM_CHAR:
            //printf("WM_CHAR\n");
            OGE_HandleInputChar(inputter, wp);
            break;

        case WM_INPUTLANGCHANGE:
            //printf("WM_INPUTLANGCHANGE\n");
            OGE_HandleInputChange(inputter);
            OGE_ResetInputWin(inputter);
            break;

        default:
            break;
    }

    return CallWindowProc(proc, hwnd, msg, wp, lp);

}


int OGE_InitIM()
{
    g_IsUnicodeIM = false; //...

    g_IsUsingIM = true;

    OGE_InitCharShiftMap();

#if defined(__OGE_WITH_SDL2__) && (SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0))

    SDL_StopTextInput();
    //SDL_Window *window = SDL_GetKeyboardFocus();
    SDL_Window* window = (SDL_Window*)g_MainImWindow;
    if (window)
    {
        SDL_SysWMinfo windowInfo;
        SDL_VERSION(&windowInfo.version);
        SDL_GetWindowWMInfo( window, &windowInfo );
        wnd = windowInfo.info.win.window;
    }
    else
    {
        wnd = 0;
    }

#endif

    if(wnd == 0)
    {
        // get main win handle
        SDL_SysWMinfo sysinfo;
        SDL_VERSION(&sysinfo.version);

#if !SDL_VERSION_ATLEAST(1,3,0) && !SDL_VERSION_ATLEAST(2,0,0)
        SDL_GetWMInfo(&sysinfo);
        wnd = sysinfo.window;

#elif SDL_VERSION_ATLEAST(1,3,0) && !SDL_VERSION_ATLEAST(2,0,0)

        SDL_GetWMInfo(&sysinfo);
        wnd = sysinfo.info.win.window;

#else
        wnd = 0;

#endif

    }


    // get main win's imc
    imc = ImmGetContext(wnd);
    if(!imc)
    {
        imc = ImmCreateContext();
        ImmAssociateContext(wnd, imc);
    }
    if(!imc) return -1;

#ifdef __IME_WITH_HOOK__
    // use hook to take back useful messages, such as WM_CHAR, WM_INPUTLANGCHANGE ...
    hook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMessageHook, NULL, GetCurrentThreadId());
    // it has been supported by SDL since version 1.2.14 ??
#endif

    // reset message handler ...
    proc = (WNDPROC)GetWindowLong(wnd, GWL_WNDPROC);
    if(proc) SetWindowLong(wnd, GWL_WNDPROC, (LONG)WndProcHook);
    else
    {
        int iError = GetLastError();
        OGE_Log("Win32 IME Error: %d \n", iError);
        return -2;
    }

    // if all is ok ...
    return 0;

}

void OGE_ResetInputWin(CogeTextInputter* inputter)
{
    //int x = 10; int y = 10;

    if(!inputter) return;

    //CogeSprite* spr = (CogeSprite*) inputter->GetRelatedSprId();
    //if(!spr) return;

    int x = inputter->GetInputWinPosX();
    int y = inputter->GetInputWinPosY();

    if(!imc) return;

    //if(!imc){
    //    imc = ImmCreateContext();
    //    ImmAssociateContext(wnd, imc);
    //}

    //COMPOSITIONFORM cf;
    //memset(&cf, 0, sizeof(cf));
    comp.dwStyle = CFS_POINT;
    comp.ptCurrentPos.x = x;
    comp.ptCurrentPos.y = y;
    ImmSetCompositionWindow(imc, &comp);
    if(ImmGetOpenStatus(imc) == FALSE) ImmSetOpenStatus(imc, TRUE);
    opened = ImmGetOpenStatus(imc);
}

void OGE_HideInputWin()
{
    if(!opened) return;
    if(!imc) return;
    if(ImmGetOpenStatus(imc))
    {
        ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmSetOpenStatus(imc, FALSE);
    }
    opened = ImmGetOpenStatus(imc);
}

void OGE_FiniIM()
{
    return;
}

bool OGE_IsDBCSLeadByte(char ch)
{
    return IsDBCSLeadByte((BYTE)ch);
    //return (BYTE)ch > 0x00a0;
}

/*
bool OGE_IsDBCSTrailByte(char *base, char *p)
{
    int lbc = 0;    // lead byte count

    assert(base <= p);

    while ( p > base )
    {
    	if ( !OGE_IsDBCSLeadByte(*(--p)) ) break;
    	lbc++;
    }

    return (lbc & 1);
}
*/

bool OGE_HandleInputEvent(SDL_Event* pEvent, int iMappedKey, int iKeyStateFlag)
{
    return false; // no need to handle it in windows ...
}

void OGE_HandleInputKey(CogeTextInputter* inputter, int iParam)
{
    if(inputter)
    switch( iParam )
    {
        case VK_HOME:   // beginning of line
            inputter->MoveCaret2Head();
            OGE_ResetInputWin(inputter);
            break;

        case VK_END:    // end of line
            inputter->MoveCaret2Tail();
            OGE_ResetInputWin(inputter);
            break;

        case VK_RIGHT:
            inputter->MoveCaret2Right();
            OGE_ResetInputWin(inputter);
            break;

        case VK_LEFT:
            inputter->MoveCaret2Left();
            OGE_ResetInputWin(inputter);
            break;

        case VK_UP:
            break;

        case VK_DOWN:
            break;

        case VK_INSERT:
            break;

        case VK_BACK:   // backspace
            inputter->DeleteLeftChar();
            OGE_ResetInputWin(inputter);
            break;

        case VK_DELETE:
            inputter->DeleteRightChar();
            OGE_ResetInputWin(inputter);
            break;

        case VK_TAB:    // tab  -- tabs are column allignment not character
            break;

        case VK_RETURN: // linefeed
            break;
    }
}

void OGE_HandleInputChar(CogeTextInputter* inputter, int iParam)
{
    if(inputter==NULL || wnd==0) return;

    unsigned char ch = (unsigned char)iParam;

    //
    // Because DBCS characters are usually generated by IMEs (as two
    // PostMessages), if a lead byte comes in, the trail byte should
    // arrive very soon after.  We wait here for the trail byte and
    // store them into the text buffer together.
    if ( OGE_IsDBCSLeadByte( ch ) )
    {
        //
        // Wait an arbitrary amount of time for the trail byte to
        // arrive.  If it doesn't, then discard the lead byte.
        //
        // This could happen if the IME screwed up.  Or, more likely,
        // the user generated the lead byte through ALT-numpad.
        //
        MSG msg = {0};
        int i = 10;

        while (!PeekMessage((LPMSG)&msg, wnd, WM_CHAR, WM_CHAR, PM_REMOVE))
        {
            if ( --i == 0 )
            {
                return;
            }
            Yield();
        }

        //StoreDBCSChar( hWnd, (WORD)(((unsigned)(msg.wParam)<<8) | (unsigned)ch ));
        inputter->StoreDBCSChar( (WORD)(((unsigned)(msg.wParam)<<8) | (unsigned)ch ) );
        OGE_ResetInputWin(inputter);
    }
    else
    {
        switch( ch )
        {
            case '\r': /* fall-through */
            case '\t': /* fall-through */
            case '\b':
                //
                // Throw away.  Already handled at WM_KEYDOWN time.
                //
                break;

            default:
                //StoreChar( hWnd, ch );
                inputter->StoreChar( ch );
                OGE_ResetInputWin(inputter);
                break;
        }
    }
}

void OGE_HandleInputChange(CogeTextInputter* inputter)
{
    if(inputter==NULL || wnd==0) return;

    prop = ImmGetProperty( GetKeyboardLayout(0), IGP_PROPERTY );

    HIMC hIMC = ImmGetContext( wnd );

    // if this application set the candidate position, it need to set
    // it to default for the near caret IME
    if ( hIMC )
    {
        UINT i;

        for (i = 0; i < 4; i++)
        {
            CANDIDATEFORM CandForm = {0};

            if ( prop & IME_PROP_AT_CARET )
            {
                CandForm.dwIndex = i;
                CandForm.dwStyle = CFS_CANDIDATEPOS;

                // This application do not want to set candidate window to
                // any position. Anyway, if an application need to set the
                // candiadet position, it should remove the if 0 code

                // the position you want to set
                //CandForm.ptCurrentPos.x = ptAppWantPosition[i].x;
                //CandForm.ptCurrentPos.y = ptAppWantPosition[i].y;

                //ImmSetCandidateWindow( hIMC, &CandForm );
            }
            else
            {
                if ( !ImmGetCandidateWindow( hIMC, i, &CandForm ) )
                {
                    continue;
                }

                if ( CandForm.dwStyle == CFS_DEFAULT )
                {
                    continue;
                }

                CandForm.dwStyle = CFS_DEFAULT;

                ImmSetCandidateWindow( hIMC, &CandForm );
            }
        }

        ImmReleaseContext( wnd, hIMC );
    }

    return;
}


#elif defined(__LINUX__)

// do something for linux ...

int OGE_InitIM()
{
    // we always assume xim will use unicode in linux ...
    g_IsUnicodeIM = true;

    g_IsUsingIM = true;

    OGE_EnableUNICODE(1);

    OGE_InitCharShiftMap();

    return 0;
}

void OGE_ResetInputWin(CogeTextInputter* inputter)
{
    return;
}

void OGE_HideInputWin()
{
    return;
}

void OGE_FiniIM()
{
    return;
}

bool OGE_IsDBCSLeadByte(char ch)
{
    return g_IsUnicodeIM;
}

/*
bool OGE_IsDBCSTrailByte(char *base, char *p)
{
    return g_IsUnicodeIM;
}
*/

bool OGE_HandleInputEvent(SDL_Event* pEvent, int iMappedKey, int iKeyStateFlag)
{
    //return; // no need to handle it in windows ...

    //printf( "Key '%d' (keysym==%d) has been %s\n",
    //        pEvent->key.keysym.unicode,
    //        (int) (pEvent->key.keysym.sym),
    //        (pEvent->key.state == SDL_PRESSED) ? "pressed" : "released" );

    CogeTextInputter* inputter = NULL;

    if(g_im) inputter = g_im->GetActiveInputter();

    if(inputter == NULL) return false;

    wchar_t code = pEvent->key.keysym.unicode;
    int key = pEvent->key.keysym.sym;

    if(code >= 32 && code < 127 && key > 0)
    {
        char ch = code;
        inputter->StoreChar(ch);
        return true;
    }
    else if(key == 0 && code != 0)
    {
        inputter->StoreDBCSChar(code);
        return true;
    }
    else if (key != 0)
    {
        bool bHandled = false;

        switch(pEvent->key.keysym.sym)
        {
        case SDLK_UP: break;
        case SDLK_DOWN: break;
        case SDLK_LEFT: inputter->MoveCaret2Left(); bHandled = true; break;
        case SDLK_RIGHT: inputter->MoveCaret2Right(); bHandled = true; break;
        case SDLK_HOME: inputter->MoveCaret2Head(); bHandled = true; break;
        case SDLK_END: inputter->MoveCaret2Tail(); bHandled = true; break;
        case SDLK_PAGEUP: break;
        case SDLK_PAGEDOWN: break;
        case SDLK_BACKSPACE: inputter->DeleteLeftChar(); bHandled = true; break;
        case SDLK_DELETE: inputter->DeleteRightChar(); bHandled = true; break; // '\177' == 127
        //case 127: inputter->DeleteRightChar(); bHandled = true; break;
        default: break;
        }

        return bHandled;
    }

    return false;

}

void OGE_HandleInputKey(CogeTextInputter* inputter, int iParam)
{
    return;
}

void OGE_HandleInputChar(CogeTextInputter* inputter, int iParam)
{
    return;
}

void OGE_HandleInputChange(CogeTextInputter* inputter)
{
    return;
}


#else
// assume no IME is available ...
#undef __OGE_WITH_IME__

#endif


// end of #ifdef __OGE_WITH_IME__
#endif


// if no defined IME ...
#ifndef __OGE_WITH_IME__

int OGE_InitIM()
{
    g_IsUnicodeIM = false; // only handle ascii char ...

    g_IsUsingIM = false;

    OGE_EnableUNICODE(0);

    OGE_InitCharShiftMap();

    return 0;
}

void OGE_ResetInputWin(CogeTextInputter* inputter)
{
    return;
}

void OGE_HideInputWin()
{
    return;
}

void OGE_FiniIM()
{
    return;
}

bool OGE_IsDBCSLeadByte(char ch)
{
    if ((ch & 0x80) == 0x00) return false;
    if ((ch & 0xC0) == 0xC0) return true;
    return false;
}

/*
bool OGE_IsDBCSTrailByte(char *base, char *p)
{
    if (((*p) & 0x80) == 0x00) return false;
    if (((*p) & 0xC0) == 0x80) return true;
    return false;
}
*/

bool OGE_HandleInputEvent(SDL_Event* pEvent, int iMappedKey, int iKeyStateFlag)
{
    CogeTextInputter* inputter = NULL;

    if(g_im) inputter = g_im->GetActiveInputter();

    if(inputter == NULL) return false;

    //wchar_t code = pEvent->key.keysym.unicode;
    int key = pEvent->key.keysym.sym;

    if(iMappedKey != 0) key = iMappedKey;

    //if(code >= 32 && code < 127 && key > 0)
    if(key >= 32 && key < 127)
    {
        //char ch = code;
        bool bIsShiftPressed = (iKeyStateFlag & 1) != 0 || (iKeyStateFlag & 64) != 0;
        if(bIsShiftPressed) key = g_CharShiftMap[key];

        char ch = key;
        inputter->StoreChar(ch);
        return true;
    }
    //else if(key == 0 && code != 0)
    //{
    //    inputter->StoreDBCSChar(code);
    //    return true;
    //}
    else if (key != 0)
    {
        bool bHandled = false;

        switch(key)
        {
        case SDLK_UP: break;
        case SDLK_DOWN: break;
        case SDLK_LEFT: inputter->MoveCaret2Left(); bHandled = true; break;
        case SDLK_RIGHT: inputter->MoveCaret2Right(); bHandled = true; break;
        case SDLK_HOME: inputter->MoveCaret2Head(); bHandled = true; break;
        case SDLK_END: inputter->MoveCaret2Tail(); bHandled = true; break;
        case SDLK_PAGEUP: break;
        case SDLK_PAGEDOWN: break;
        case SDLK_BACKSPACE: inputter->DeleteLeftChar(); bHandled = true; break;
        case SDLK_DELETE: inputter->DeleteRightChar(); bHandled = true; break; // '\177' == 127
        //case 127: inputter->DeleteRightChar(); bHandled = true; break;
        default: break;
        }

        return bHandled;
    }

    return false;
}

void OGE_HandleInputKey(CogeTextInputter* inputter, int iParam)
{
    return;
}

void OGE_HandleInputChar(CogeTextInputter* inputter, int iParam)
{
    return;
}

void OGE_HandleInputChange(CogeTextInputter* inputter)
{
    return;
}


// end of #ifndef __OGE_WITH_IME__
#endif



/***************** CogeIM ********************/

CogeIM::CogeIM(void* pMainWindow)
{
    m_pMainWindow = pMainWindow;
    g_MainImWindow = m_pMainWindow;
    m_pActiveInputter = NULL;
    m_iState = -1;
    m_iPlatform = 0;
    if(m_pMainWindow) OGE_EnableUNICODE(0);
}
CogeIM::~CogeIM()
{
    Finalize();
}

void CogeIM::DelAllInputters()
{
    // free all Inputters
	ogeTextInputterMap::iterator it;
	CogeTextInputter* pMatched = NULL;

	it = m_InputterMap.begin();

	while (it != m_InputterMap.end())
	{
		pMatched = it->second;
		m_InputterMap.erase(it);
		delete pMatched;
		it = m_InputterMap.begin();
	}
}

int CogeIM::GetState()
{
    return m_iState;
}

void CogeIM::DisableIM()
{
    SetActiveInputter(NULL);
    OGE_HideInputWin();
}

bool CogeIM::HandleInput(void* pEventData, int iMappedKey, int iKeyStateFlag)
{
    if (pEventData == NULL) return false;

    SDL_Event* pEvent = (SDL_Event*)pEventData;

    return OGE_HandleInputEvent(pEvent, iMappedKey, iKeyStateFlag);
}

int CogeIM::Initialize()
{

    m_iState = OGE_InitIM();

    if(m_iState >= 0) g_im = this;

    return m_iState;

}
void CogeIM::Finalize()
{
    m_pActiveInputter = NULL;
    DelAllInputters();
    g_im = NULL;
    OGE_FiniIM();
}

void CogeIM::SetUnicodeMode(bool bIsAllUnicode)
{
    g_IsUnicodeIM = bIsAllUnicode;
}
bool CogeIM::GetUnicodeMode()
{
    return g_IsUnicodeIM;
}

bool CogeIM::IsInputMethodReady()
{
    return g_IsUsingIM;
}

void CogeIM::ShowSoftKeyboard(bool bShow)
{
#if defined(__IPHONE__) && defined(__OGE_WITH_GLWIN__) && (SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0))

    SDL_Window* window = (SDL_Window*) m_pMainWindow;
    if(window == NULL) return;

    if(bShow)
    {
        //if(g_glctx == NULL) g_glctx = SDL_GL_CreateContext(window);
        SDL_iPhoneKeyboardShow(window);
    }
    else
    {
        SDL_iPhoneKeyboardHide(window);
        if(g_glctx)
        {
            //SDL_GL_DeleteContext(g_glctx);
            g_glctx = NULL;
        }
    }

#else
    return;
#endif

}
bool CogeIM::IsSoftKeyboardShown()
{
#if defined(__IPHONE__) && defined(__OGE_WITH_GLWIN__) && (SDL_VERSION_ATLEAST(1,3,0) || SDL_VERSION_ATLEAST(2,0,0))
    SDL_Window* window = (SDL_Window*) m_pMainWindow;
    if(window == NULL) return false;
    return SDL_iPhoneKeyboardIsShown(window);
#else
    return false;
#endif
}

CogeTextInputter* CogeIM::GetActiveInputter()
{
    return m_pActiveInputter;
}

void CogeIM::SetActiveInputter(CogeTextInputter* inputter)
{
    if(inputter == NULL)
    {
        m_pActiveInputter = NULL;
        OGE_HideInputWin();
    }
    else
    {
        CogeTextInputter* pMatched = FindInputter(inputter->GetName());
        if(pMatched == NULL || pMatched == m_pActiveInputter) return;
        else m_pActiveInputter = pMatched;
        if(m_pActiveInputter)
        {
            OGE_ResetInputWin(m_pActiveInputter);
            m_pActiveInputter->Reset();
        }
    }
}

CogeTextInputter* CogeIM::NewInputter(const std::string& sInputterName)
{
    CogeTextInputter* pTheNewInputter = NULL;

	ogeTextInputterMap::iterator it;

    it = m_InputterMap.find(sInputterName);
	if (it != m_InputterMap.end())
	{
        OGE_Log("The Text Inputter name '%s' is in use.\n", sInputterName.c_str());
        return NULL;
	}

	pTheNewInputter = new CogeTextInputter(this, sInputterName);

	m_InputterMap.insert(ogeTextInputterMap::value_type(sInputterName, pTheNewInputter));

	return pTheNewInputter;

}

CogeTextInputter* CogeIM::FindInputter(const std::string& sInputterName)
{
    ogeTextInputterMap::iterator it;

    it = m_InputterMap.find(sInputterName);
	if (it != m_InputterMap.end()) return it->second;
	else return NULL;
}

CogeTextInputter* CogeIM::GetInputter(const std::string& sInputterName)
{
    CogeTextInputter* pMatched = FindInputter(sInputterName);
    if (!pMatched) pMatched = NewInputter(sInputterName);
    return pMatched;
}

int CogeIM::DelInputter(const std::string& sInputterName)
{
    ogeTextInputterMap::iterator it = m_InputterMap.find(sInputterName);
	if (it == m_InputterMap.end()) return -1;

    CogeTextInputter* pMatched = it->second;
    if(!pMatched) return -1;

    if(pMatched->m_iTotalUsers > 0) return 0;

    m_InputterMap.erase(it);

    delete pMatched;

    return 1;
}


/***************** CogeTextInputter ********************/

CogeTextInputter::CogeTextInputter(CogeIM* pIM, const std::string& sName)
{
    m_pIM = pIM;
    m_sName = sName;
    m_iCaretPos = -1;
    //m_iCharWidth = 1;

    //m_iRelatedSprId = 0;

    m_iTextLength = 0;

    m_iInputWinPosX = 10;
    m_iInputWinPosY = 10;

    m_iTotalUsers = 0;

    Clear();
}
CogeTextInputter::~CogeTextInputter()
{
    Clear();
}

const std::string& CogeTextInputter::GetName()
{
    return m_sName;
}

void CogeTextInputter::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}

void CogeTextInputter::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}



//std::string CogeTextInputter::GetText()
//{
//    return std::string(m_text);
//}

char* CogeTextInputter::GetBufferText()
{
    return &(m_text[0]);
}


int CogeTextInputter::GetTextLength()
{
    //if(m_text[_OGE_MAX_IM_BUF_-1] != 0)
    //int iLen = strlen(m_text);

    //int iLen = m_iTextLength;
    //if(iLen > _OGE_MAX_IM_BUF_) iLen = _OGE_MAX_IM_BUF_;
    //return iLen;

    if(m_iTextLength > _OGE_MAX_IM_BUF_) m_iTextLength = _OGE_MAX_IM_BUF_;
    if(m_iTextLength < 0) m_iTextLength = 0;
    return m_iTextLength;
}

int CogeTextInputter::SetTextLength(int iLen)
{
    m_iTextLength = iLen;
    if(m_iTextLength > _OGE_MAX_IM_BUF_) m_iTextLength = _OGE_MAX_IM_BUF_;
    if(m_iTextLength < 0) m_iTextLength = 0;
    return m_iTextLength;
}

//int CogeTextInputter::GetTextWidth()
//{
//    return m_iCharWidth * GetTextLength();
//}

int CogeTextInputter::GetCaretPos()
{
    return m_iCaretPos;
}

void CogeTextInputter::Clear()
{
    memset((void*)&m_text[0], 0, sizeof(char) * _OGE_MAX_IM_BUF_ );
    m_iCaretPos = -1;
    m_iTextLength = 0;
}

void CogeTextInputter::Focus()
{
    m_pIM->SetActiveInputter(this);
}

void CogeTextInputter::Reset()
{
    //if(iCharWidth > 0) m_iCharWidth = iCharWidth;
    MoveCaret2Tail();
}

/*
int CogeTextInputter::GetRelatedSprId()
{
    return m_iRelatedSprId;
}
void CogeTextInputter::SetRelatedSprId(int iSprId)
{
    m_iRelatedSprId = iSprId;
}
*/

int CogeTextInputter::GetInputWinPosX()
{
    return m_iInputWinPosX;
}
int CogeTextInputter::GetInputWinPosY()
{
    return m_iInputWinPosY;
}

void CogeTextInputter::SetInputWinPos(int iPosX, int iPosY)
{
    m_iInputWinPosX = iPosX;
    m_iInputWinPosY = iPosY;
}

void CogeTextInputter::StoreChar(char ch)
{
    if (m_pIM->GetUnicodeMode())
    {
        wchar_t wch = ch;
        //printf(" Char: %d\n", wch);
        StoreDBCSChar(wch);
    }
    else
    {
        int iLastPos = _OGE_MAX_IM_BUF_ - 1;

        if (m_iCaretPos >= iLastPos) return;

        for (int i = iLastPos-1; i > m_iCaretPos; i-- )
        {
            m_text[i+1] = m_text[i];
        }

        m_text[m_iCaretPos+1] = ch;

        m_iCaretPos++;

        m_iTextLength++;
    }
}

void CogeTextInputter::StoreDBCSChar(wchar_t ch)
{
    int iLastPos = _OGE_MAX_IM_BUF_ - 1;

    if (m_iCaretPos >= iLastPos-1) return;

    for (int i = iLastPos-2; i > m_iCaretPos; i-- )
    {
        m_text[i+2] = m_text[i];
    }

    m_text[m_iCaretPos+1] = ch & 0x00ff;
    m_text[m_iCaretPos+2] = (ch & 0xff00) >> 8;

    m_iCaretPos += 2;

    m_iTextLength += 2;
}

void CogeTextInputter::MoveCaret2Left()
{
    if(m_iCaretPos < -1)
    {
        m_iCaretPos = -1;
        return;
    }
    if(m_iCaretPos == -1) return;
    if(m_iCaretPos == 0)
    {
        m_iCaretPos--;
        return;
    }
    if(m_iCaretPos > 0)
    {
        if(OGE_IsDBCSLeadByte(m_text[m_iCaretPos])) m_iCaretPos -= 2;
        else m_iCaretPos--;
        return;
    }

}
void CogeTextInputter::MoveCaret2Right()
{
    int iLastPos = GetTextLength()-1;
    if(iLastPos < 0)
    {
        m_iCaretPos = -1;
        return;
    }
    if(m_iCaretPos > iLastPos)
    {
        m_iCaretPos = iLastPos;
        return;
    }
    if(m_iCaretPos == iLastPos) return;
    if(m_iCaretPos == iLastPos-1)
    {
        m_iCaretPos++;
        return;
    }
    if(m_iCaretPos < iLastPos-1)
    {
        if(OGE_IsDBCSLeadByte(m_text[m_iCaretPos+1])) m_iCaretPos += 2;
        else m_iCaretPos++;
        return;
    }

}
void CogeTextInputter::MoveCaret2Head()
{
    m_iCaretPos = -1;
}
void CogeTextInputter::MoveCaret2Tail()
{
    int iLastPos = GetTextLength()-1;
    if(iLastPos < 0)
    {
        m_iCaretPos = -1;
        return;
    }
    else m_iCaretPos = iLastPos;
}
void CogeTextInputter::DeleteLeftChar()
{
    if(m_iCaretPos < -1)
    {
        m_iCaretPos = -1;
        return;
    }
    if(m_iCaretPos == -1) return;
    if(m_iCaretPos == 0)
    {
        m_text[m_iCaretPos] = 0;

        int iLastPos = _OGE_MAX_IM_BUF_ - 1;

        for (int i = m_iCaretPos+1; i <= iLastPos; i++ )
        {
            m_text[i-1] = m_text[i];
        }

        m_iCaretPos--;
        m_iTextLength--;

        return;
    }
    if(m_iCaretPos > 0)
    {
        if(OGE_IsDBCSLeadByte(m_text[m_iCaretPos]))
        {
            m_text[m_iCaretPos] = 0;
            m_text[m_iCaretPos-1] = 0;

            int iLastPos = _OGE_MAX_IM_BUF_ - 1;

            for (int i = m_iCaretPos+1; i <= iLastPos; i++ )
            {
                m_text[i-2] = m_text[i];
            }
            m_iCaretPos -= 2;
            m_iTextLength -= 2;
        }
        else
        {
            m_text[m_iCaretPos] = 0;

            int iLastPos = _OGE_MAX_IM_BUF_ - 1;

            for (int i = m_iCaretPos+1; i <= iLastPos; i++ )
            {
                m_text[i-1] = m_text[i];
            }
            m_iCaretPos--;
            m_iTextLength--;
        }
        return;
    }
}
void CogeTextInputter::DeleteRightChar()
{
    int iLastPos = GetTextLength()-1;
    if(iLastPos < 0)
    {
        m_iCaretPos = -1;
        return;
    }
    if(m_iCaretPos > iLastPos)
    {
        m_iCaretPos = iLastPos;
        return;
    }
    //if(m_iCaretPos == iLastPos) return;
    if(m_iCaretPos == iLastPos) return DeleteLeftChar();
    if(m_iCaretPos == iLastPos-1)
    {
        m_text[m_iCaretPos+1] = 0;
        m_iTextLength--;
        return;
    }
    if(m_iCaretPos < iLastPos-1)
    {
        if(OGE_IsDBCSLeadByte(m_text[m_iCaretPos+1]))
        {
            m_text[m_iCaretPos+1] = 0;
            m_text[m_iCaretPos+2] = 0;

            int iLastBufPos = _OGE_MAX_IM_BUF_ - 1;

            for (int i = m_iCaretPos+3; i <= iLastBufPos; i++ )
            {
                m_text[i-2] = m_text[i];
            }

            m_iTextLength -= 2;
        }
        else
        {
            m_text[m_iCaretPos+1] = 0;

            int iLastBufPos = _OGE_MAX_IM_BUF_ - 1;

            for (int i = m_iCaretPos+2; i <= iLastBufPos; i++ )
            {
                m_text[i-1] = m_text[i];
            }

            m_iTextLength--;
        }
        return;
    }
}




