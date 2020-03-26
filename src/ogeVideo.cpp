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

#include "ogeVideo.h"

#include "ogeCommon.h"

#include "ogeGraphicFX.h"

#include "SDL_image.h"

#include <cmath>
#include <fstream>

#if SDL_VERSION_ATLEAST(2,0,0)
#define OGE_SRCALPHA        0x00010000
#define OGE_SRCCOLORKEY     0x00020000
#define OGE_ANYFORMAT       0x00100000
#define OGE_DOUBLEBUF       0x00400000
#define OGE_FULLSCREEN      0x00800000
#else
#define OGE_SRCALPHA        SDL_SRCALPHA
#define OGE_SRCCOLORKEY     SDL_SRCCOLORKEY
#define OGE_ANYFORMAT       SDL_ANYFORMAT
#define OGE_DOUBLEBUF       SDL_DOUBLEBUF
#define OGE_FULLSCREEN      SDL_FULLSCREEN
#endif


// global internal functions
/*
int FormatColor(int iRGBColor, int iBPP)
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


static SDL_Surface* g_pMainSurface = NULL;

static int OGE_SetAlpha(SDL_Surface * surface, Uint32 flag, Uint8 value)
{
#if defined(__OGE_WITH_GLWIN__) || SDL_VERSION_ATLEAST(2,0,0)

    if (flag & OGE_SRCALPHA) {
        /* According to the docs, value is ignored for alpha surfaces */
        if (surface->format->Amask) {
            value = 0xFF;
        }
        SDL_SetSurfaceAlphaMod(surface, value);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    } else {
        SDL_SetSurfaceAlphaMod(surface, 0xFF);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    }
    SDL_SetSurfaceRLE(surface, (flag & SDL_RLEACCEL));

    return 0;

#else

    return SDL_SetAlpha(surface, flag, value);

#endif


}

static SDL_Surface* OGE_DisplayFormat(SDL_Surface* pSurface)
{
    SDL_Surface* pResult = NULL;

#if defined(__OGE_WITH_GLWIN__) || SDL_VERSION_ATLEAST(2,0,0)

    if(g_pMainSurface != NULL)
    {
        SDL_PixelFormat* format = g_pMainSurface->format;
        pResult = SDL_ConvertSurface(pSurface, format, 0);
    }


#else


    pResult = SDL_DisplayFormat(pSurface);

#ifdef __OGE_WITH_SDL2__
    if(pResult) SDL_SetSurfaceRLE(pResult, 0);
#endif


#endif

    return pResult;
}


static SDL_Surface* OGE_DisplayFormatAlpha(SDL_Surface* pSurface)
{
    SDL_Surface* pResult = NULL;

#if defined(__OGE_WITH_GLWIN__) || SDL_VERSION_ATLEAST(2,0,0)

    SDL_PixelFormat *vf;
    SDL_PixelFormat *format;

    /* default to ARGB8888 */
    Uint32 amask = 0xff000000;
    Uint32 rmask = 0x00ff0000;
    Uint32 gmask = 0x0000ff00;
    Uint32 bmask = 0x000000ff;

    if(g_pMainSurface == NULL) return pResult;
    vf = g_pMainSurface->format;

    switch (vf->BytesPerPixel) {
    case 2:
        /* For XGY5[56]5, use, AXGY8888, where {X, Y} = {R, B}.
           For anything else (like ARGB4444) it doesn't matter
           since we have no special code for it anyway */
        if ((vf->Rmask == 0x1f) &&
            (vf->Bmask == 0xf800 || vf->Bmask == 0x7c00)) {
            rmask = 0xff;
            bmask = 0xff0000;
        }
        break;

    case 3:
    case 4:
        /* Keep the video format, as long as the high 8 bits are
           unused or alpha */
        if ((vf->Rmask == 0xff) && (vf->Bmask == 0xff0000)) {
            rmask = 0xff;
            bmask = 0xff0000;
        }
        break;

    default:
        /* We have no other optimised formats right now. When/if a new
           optimised alpha format is written, add the converter here */
        break;
    }
    format = SDL_AllocFormat(SDL_MasksToPixelFormatEnum(32, rmask, gmask, bmask, amask));
    if (format == NULL) return NULL;

    pResult = SDL_ConvertSurface(pSurface, format, 0);
    SDL_FreeFormat(format);

#else

    pResult = SDL_DisplayFormatAlpha(pSurface);

#ifdef __OGE_WITH_SDL2__
    if(pResult) SDL_SetSurfaceRLE(pResult, 0);
#endif

#endif

    return pResult;
}



/*---------------- Video -----------------*/

void CogeVideo::OGE_WarpMouse(unsigned short x, unsigned short y)
{

#ifdef __OGE_WITH_GLWIN__
    if(m_pMainWindow != NULL)
    {
        SDL_WarpMouseInWindow(m_pMainWindow, x, y);
    }
    else
    {
#if SDL_VERSION_ATLEAST(2,0,0)
        return;
#else
        SDL_WarpMouse(x, y);
#endif
    }
#else
    SDL_WarpMouse(x, y);
#endif


    //SDL_WarpMouse(x, y);

}


/*
SDL_Surface* CogeVideo::OGE_LoadBMP(const char* filepath)
{
    SDL_Surface* pResult = NULL;

#ifdef __OGE_WITH_SDL2__

    SDL_RWops *file = SDL_RWFromFile(filepath, "rb");
    if(file) pResult = SDL_LoadBMP_RW(file, 1);

#else

    pResult = SDL_LoadBMP(filepath);

#endif

    return pResult;
}

SDL_Surface* CogeVideo::OGE_LoadIMG(const char* filepath)
{
    SDL_Surface* pResult = NULL;

#ifdef __OGE_WITH_SDL2__

    SDL_RWops *file = SDL_RWFromFile(filepath, "rb");
    if(file) pResult = IMG_Load_RW(file, 1);

#else

    pResult = IMG_Load(filepath);

#endif

    return pResult;
}
*/


int CogeVideo::CheckSystemScreenSize(int iRequiredWidth, int iRequiredHeight, int iRequiredBPP)
{
    m_iRealWidth = m_iWidth;
    m_iRealHeight = m_iHeight;
    m_iRealBPP = m_iBPP;
    m_bNeedStretch = false;

#if (defined __OGE_WITH_SDL2__)

    int iDisplayCount = SDL_GetNumVideoDisplays();
    OGE_Log("Video Display Count: %d \n", iDisplayCount);

    if(iDisplayCount <= 0) return -1;

    bool bNeedWideScreen = iRequiredWidth > iRequiredHeight;

    int defaultdisplay, nummodes, bestmode, diff, bestdiff, bpp;
	SDL_DisplayMode mode;

	defaultdisplay = 0;

	bestdiff = 999999;
	bestmode = -1;

	nummodes = SDL_GetNumDisplayModes(defaultdisplay);

	for (int i = 0; i < nummodes; i++)
    {
		SDL_GetDisplayMode(defaultdisplay, i, &mode);

		bpp = SDL_BITSPERPIXEL(mode.format);
		if(bpp >= 24) bpp = 32;
        else bpp = 16;

		OGE_Log("Display Mode %d:  %dx%d %dHz %d(%d) BPP \n", i, mode.w, mode.h, mode.refresh_rate, bpp, SDL_BITSPERPIXEL(mode.format));

        if(bNeedWideScreen)
        {
            if(mode.w > mode.h && bpp == iRequiredBPP && mode.w >= iRequiredWidth && mode.h >= iRequiredHeight)
            {
                diff = abs(iRequiredWidth - mode.w) + abs(iRequiredHeight - mode.h);
                if(diff < bestdiff)
                {
                    bestdiff = diff;
                    bestmode = i;
                }
            }
        }
        else
        {
            if(mode.w < mode.h && bpp == iRequiredBPP && mode.w >= iRequiredWidth && mode.h >= iRequiredHeight)
            {
                diff = abs(iRequiredWidth - mode.w) + abs(iRequiredHeight - mode.h);
                if(diff < bestdiff)
                {
                    bestdiff = diff;
                    bestmode = i;
                }
            }

        }
	}

	if(bestmode < 0)
    {
        for (int i = 0; i < nummodes; i++)
        {

            SDL_GetDisplayMode(defaultdisplay, i, &mode);

            bpp = SDL_BITSPERPIXEL(mode.format);
            if(bpp >= 24) bpp = 32;
            else bpp = 16;

            //OGE_Log("Display Mode %d:  %dx%d %dHz %d BPP \n", i, mode.w, mode.h, mode.refresh_rate, SDL_BITSPERPIXEL(mode.format));

            if(bNeedWideScreen)
            {
                if(mode.w > mode.h && bpp == iRequiredBPP)
                {
                    diff = abs(iRequiredWidth - mode.w) + abs(iRequiredHeight - mode.h);
                    if(diff < bestdiff)
                    {
                        bestdiff = diff;
                        bestmode = i;
                    }
                }
            }
            else
            {
                if(mode.w < mode.h && bpp == iRequiredBPP)
                {
                    diff = abs(iRequiredWidth - mode.w) + abs(iRequiredHeight - mode.h);
                    if(diff < bestdiff)
                    {
                        bestdiff = diff;
                        bestmode = i;
                    }
                }

            }
        }
    }

	if(bestmode >= 0)
    {
        SDL_GetDisplayMode(defaultdisplay, bestmode, &m_DisplayMode);
        m_iRealWidth  = m_DisplayMode.w;
        m_iRealHeight = m_DisplayMode.h;
        m_iRealBPP = SDL_BITSPERPIXEL(m_DisplayMode.format);
        if(m_iRealBPP >= 24) m_iRealBPP = 32;
        else m_iRealBPP = 16;
    }
	else
    {
        SDL_GetCurrentDisplayMode(defaultdisplay, &m_DisplayMode);

        SDL_Rect bounds;

        SDL_GetDisplayBounds(defaultdisplay, &bounds);
        //SDL_GetCurrentDisplayMode(defaultdisplay, &m_DisplayMode);

        m_iRealWidth = bounds.w;
        m_iRealHeight = bounds.h;

        m_iRealBPP = SDL_BITSPERPIXEL(m_DisplayMode.format);
        if(m_iRealBPP >= 24) m_iRealBPP = 32;
        else m_iRealBPP = 16;
    }

	OGE_Log("The best mode for %dx%dx%d is: %d (%dx%dx%d).\n",
         iRequiredWidth, iRequiredHeight, iRequiredBPP, bestmode,
         m_iRealWidth, m_iRealHeight, m_iRealBPP);

    //OGE_Log("Display %d: %dx%d at %d,%d \n", d, bounds.w, bounds.h, bounds.x, bounds.y);
    //OGE_Log("Display %d: %dx%d at %d,%d (BPP: %d)\n", defaultdisplay, bounds.w, bounds.h, bounds.x, bounds.y, m_iRealBPP);

    return bestmode;

#else

    const SDL_VideoInfo* pVideoInfo = SDL_GetVideoInfo();
    if(pVideoInfo)
    {
        m_iRealWidth = pVideoInfo->current_w;
        m_iRealHeight = pVideoInfo->current_h;
        m_iRealBPP = pVideoInfo->vfmt->BitsPerPixel;

        if(m_iRealBPP >= 24) m_iRealBPP = 32;
        else m_iRealBPP = 16;
    }

    return 0;

#endif


}

CogeVideo::CogeVideo():
m_pFrontBuffer(NULL),
m_pMainWindow(NULL),
m_pMainTexture(NULL),
m_pMainRenderer(NULL),
m_pDefaultScreen(NULL),
m_pDefaultBg(NULL),
m_pMainScreen(NULL),
m_pLastScreen(NULL),
m_pTextArea(NULL),
m_pClipboardA(NULL),
m_pClipboardB(NULL),
m_pClipboardC(NULL),
m_pDotFont(NULL),
m_pFontStock(NULL),
m_pDefaultFont(NULL),
m_iWidth(0),
m_iHeight(0),
m_iBPP(0),
m_iMode(0)
{
    m_pWindowIcon = NULL;

    m_bShowFPS         = true;
	m_iLockedFPS       = 0;
	m_iFrameInterval   = 0;

	m_bIsBGRA          = false;

	//m_bInDirtyRectMode = true;

	//m_iFrameInterval = 0;
	m_iLastFrameTime = 0;
	m_iFrameRate     = 0;
	m_iOldFrameRate  = 0;
    m_iFrameCount    = 0;
	m_iGlobalTime    = 0;

	m_ViewRect.x = 0;
	m_ViewRect.y = 0;
	m_ViewRect.w = 0;
	m_ViewRect.h = 0;

	m_sDefaultResPath = "";

#ifdef __OGE_WITH_SDL2__
    memset((void*)&m_DisplayMode, 0, sizeof(SDL_DisplayMode));
#endif

	OGE_FX_Init();

    int iMMXFlag = OGE_FX_CheckMMX();
	m_bSupportMMX = iMMXFlag > 0;

#if SDL_VERSION_ATLEAST(2,0,0)
	OGE_Log( "Video Flags: SDL = 2.0, MMX = %d\n", iMMXFlag );
#elif SDL_VERSION_ATLEAST(1,3,0)
	OGE_Log( "Video Flags: SDL = 1.3, MMX = %d\n", iMMXFlag );
#else
    OGE_Log( "Video Flags: SDL = 1.2, MMX = %d\n", iMMXFlag );
#endif

	m_iState = 0;

    // initialize SDL video
    //m_iState = SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
    //if ( m_iState < 0 )
    //{
    //    printf( "Unable to init SDL: %s\n", SDL_GetError() );
    //    return;
    //}

    // make sure SDL cleans up before exit
    //atexit(SDL_Quit);

    return;
}

CogeVideo::~CogeVideo()
{
    Finalize();
    return;
}

int CogeVideo::Initialize(int iWidth, int iHeight, int iBPP, bool bFullscreen, int iFlags)
{
    Finalize();

    //SDL_Surface* pBackScreen = NULL;

    if(m_iState >= 0) return m_iState;

#ifdef __WINCE__
    iBPP = 16;
#elif defined __ANDROID__
    iBPP = 16;
#elif defined __IPHONE__
    iBPP = 32;
#endif

#if defined(__ANDROID__) || defined(__IPHONE__)
    bFullscreen = false;
#endif


#ifdef __OGE_WITH_SDL2__
    if(bFullscreen) iFlags = iFlags|OGE_FULLSCREEN;
#else
    if(bFullscreen) iFlags = iFlags|OGE_FULLSCREEN;
    else iFlags = iFlags|OGE_ANYFORMAT;
#endif



    //if(bFullscreen) iFlags = iFlags|SDL_FULLSCREEN;
    //else iFlags = iFlags|SDL_ANYFORMAT;

#if !defined(__WINCE__) && !defined(__ANDROID__) && !defined(__IPHONE__)
#if !SDL_VERSION_ATLEAST(2,0,0)
    if (SDL_VideoModeOK(iWidth, iHeight, iBPP, iFlags) <= 0)
    {
        if(bFullscreen)
            OGE_Log("Not Support The Video Mode: %d x %d x %d (Fullscreen) \n", iWidth, iHeight, iBPP);
        else
            OGE_Log("Not Support The Video Mode: %d x %d x %d (Window) \n", iWidth, iHeight, iBPP);

        return -1;
    }
#endif
#endif

#if defined(__OGE_WITH_SDL2__)
    int iBestMode = CheckSystemScreenSize(iWidth, iHeight, iBPP);
#else
    CheckSystemScreenSize(iWidth, iHeight, iBPP);
#endif

#if (defined __WINCE__)
    if((iWidth > iHeight && m_iRealWidth < m_iRealHeight)
       || (iWidth < iHeight && m_iRealWidth > m_iRealHeight))
    {
        int iTemp = m_iRealWidth;
        m_iRealWidth = m_iRealHeight;
        m_iRealHeight = iTemp;
    }
#endif

#if defined(__ANDROID__) || defined(__WINCE__) || defined(__IPHONE__)
    m_bNeedStretch = (m_iRealWidth != iWidth) || (m_iRealHeight != iHeight);
#else
    m_bNeedStretch = false;
#endif


#if defined(__OGE_WITH_GLWIN__) && defined(__OGE_WITH_SDL2__)

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 1);

//#if (defined __ANDROID__)
//    bFullscreen = false;
//#endif

    int iWinFlag = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

#if defined(__ANDROID__) || defined(__IPHONE__)
    iWinFlag = iWinFlag | SDL_WINDOW_BORDERLESS;
#else
    if(bFullscreen) iWinFlag = iWinFlag | SDL_WINDOW_BORDERLESS;
#endif

    //m_pFrontBuffer = SDL_SetVideoMode(iWidth, iHeight, m_iRealBPP, iFlags);

    m_pMainWindow = SDL_CreateWindow("OGE2D",
    		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    		iWidth, iHeight, iWinFlag);

    if(iBestMode >= 0)
    {
        int defaultdisplay = 0;
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(defaultdisplay, iBestMode, &mode);
        SDL_SetWindowDisplayMode(m_pMainWindow, &mode);
    }

#if defined(__ANDROID__) || defined(__IPHONE__)
    SDL_SetWindowFullscreen(m_pMainWindow, SDL_TRUE);
#else
    if(bFullscreen) SDL_SetWindowFullscreen(m_pMainWindow, SDL_TRUE);
#endif

#if defined(__MACOSX__) || defined(__IPHONE__)

    if(m_iRealBPP >= 24)
        m_pFrontBuffer = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight, m_iRealBPP,
                                              0x0000ff00, 0x00ff0000, 0xff000000, 0x00000000);
    else
        m_pFrontBuffer = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight, m_iRealBPP,
                                              0x001f, 0x07e0, 0xf800, 0x0000);

#else

    //m_pFrontBuffer = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight, m_iRealBPP, 0, 0, 0, 0);

    if(m_iRealBPP >= 24)
        m_pFrontBuffer = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight, m_iRealBPP,
                                              0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
    else
        m_pFrontBuffer = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight, m_iRealBPP,
                                              0xf800, 0x07e0, 0x001f, 0x0000);

#endif

    //m_pFrontBuffer = SDL_GetWindowSurface(m_pMainWindow);
    //m_pFrontBuffer = SDL_SetVideoMode(iWidth, iHeight, m_iRealBPP, iFlags);

    //SDL_RaiseWindow(m_pMainWindow);

    m_pMainRenderer = SDL_CreateRenderer(m_pMainWindow, -1, 0);
    m_pMainTexture = SDL_CreateTexture(m_pMainRenderer, m_pFrontBuffer->format->format, SDL_TEXTUREACCESS_STREAMING, iWidth, iHeight);

#else

    if(m_bNeedStretch) m_pFrontBuffer = SDL_SetVideoMode(m_iRealWidth, m_iRealHeight, iBPP, iFlags);
    else m_pFrontBuffer = SDL_SetVideoMode(iWidth, iHeight, iBPP, iFlags);

#endif


    g_pMainSurface = m_pFrontBuffer;

    if(m_pFrontBuffer)
    {
        m_iWidth = iWidth;

        m_iHeight = iHeight;

        m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_iMode = iFlags;


        if(m_pFrontBuffer->format->Bmask > m_pFrontBuffer->format->Rmask)
        {
            m_bIsBGRA = true;
        }
        else
        {
            m_bIsBGRA = false;
        }

    }


#ifdef __OGE_WITH_GLWIN__

    if(bFullscreen)
        OGE_Log("Video Mode: %d x %d x %d (Fullscreen + OpenGL) \n", m_iWidth, m_iHeight, m_iBPP);
    else
        OGE_Log("Video Mode: %d x %d x %d (Window + OpenGL) \n", m_iWidth, m_iHeight, m_iBPP);

#else

    if(bFullscreen)
        OGE_Log("Video Mode: %d x %d x %d (Fullscreen) \n", m_iWidth, m_iHeight, m_iBPP);
    else
        OGE_Log("Video Mode: %d x %d x %d (Window) \n", m_iWidth, m_iHeight, m_iBPP);

#endif


    if(m_pFrontBuffer)
    {
        //std::string sEnFontFilePath = OGE_GetGamePath();
        //std::string sCnFontFilePath = OGE_GetGamePath();

        std::string sEnFontFilePath = m_sDefaultResPath;
        std::string sCnFontFilePath = m_sDefaultResPath;

        sEnFontFilePath = sEnFontFilePath + _OGE_EN_FONT_FILE_;
        sCnFontFilePath = sCnFontFilePath + _OGE_CN_FONT_FILE_;

        //printf(sEnFontFilePath.c_str());
        //printf("\n");


        m_pDotFont = new CogeDotFont();
        if (!(m_pDotFont->InstallFont(sEnFontFilePath.c_str(), 0)) ||
            !(m_pDotFont->InstallFont(sCnFontFilePath.c_str(), 1)))
        {
            delete m_pDotFont;
            m_pDotFont = NULL;
            OGE_Log("Init Video Error: Can not load default dot font file.\n");
        }

        m_pFontStock = new CogeFontStock();
        if(!m_pFontStock->IsFontSystemOK())
        {
            delete m_pFontStock;
            m_pFontStock = NULL;
            OGE_Log("Init Video Error: Can not init ttf font system.\n");
        }
        else
        {
            //std::string sFontFilePath = OGE_GetGamePath();
            //if(m_pFontStock->NewFont("Default", _OGE_DOT_FONT_SIZE_, sFontFilePath + _OGE_TTF_FONT_FILE_) == NULL)
            //printf("Init Video Error: Can not load default ttf font file.\n");
        }
    }

    if (m_pFrontBuffer && !m_pDefaultScreen)
	{
		m_pDefaultScreen = new CogeImage("DEFAULT_MAIN_SCREEN");
		m_pDefaultScreen->m_pDotFont = this->m_pDotFont;

		m_pDefaultScreen->m_pVideo = this;

        m_pDefaultScreen->m_iWidth = iWidth;
        m_pDefaultScreen->m_iHeight = iHeight;

        m_pDefaultScreen->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pDefaultScreen->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pDefaultScreen->m_iWidth, m_pDefaultScreen->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pDefaultScreen->m_pSurface)
        SDL_FillRect(m_pDefaultScreen->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

        m_pDefaultScreen->SetPenColor(-1);
	}

    if (!m_pDefaultScreen) return -1;

    // clear it
    //SDL_FillRect(pBackScreen, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

    if (!m_pDefaultBg)
	{
		m_pDefaultBg = new CogeImage("MAIN_SCREEN_DEFAULT_BG");
		m_pDefaultBg->m_pDotFont = this->m_pDotFont;

		m_pDefaultBg->m_pVideo = this;

        m_pDefaultBg->m_iWidth = iWidth;
        m_pDefaultBg->m_iHeight = iHeight;

        m_pDefaultBg->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pDefaultBg->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pDefaultBg->m_iWidth, m_pDefaultBg->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pDefaultBg->m_pSurface)
        SDL_FillRect(m_pDefaultBg->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

        m_pDefaultBg->SetPenColor(-1);
	}

	if (!m_pLastScreen)
	{
		m_pLastScreen = new CogeImage("LAST_MAIN_SCREEN");
		m_pLastScreen->m_pDotFont = this->m_pDotFont;

		m_pLastScreen->m_pVideo = this;

        m_pLastScreen->m_iWidth = iWidth;
        m_pLastScreen->m_iHeight = iHeight;

        m_pLastScreen->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pLastScreen->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pLastScreen->m_iWidth, m_pLastScreen->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pLastScreen->m_pSurface)
        SDL_FillRect(m_pLastScreen->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

        m_pLastScreen->SetPenColor(-1);
	}


    if (!m_pTextArea)
	{
		m_pTextArea = new CogeImage("MAIN_SCREEN_TEXT_AREA");
		m_pTextArea->m_pDotFont = this->m_pDotFont;

		m_pTextArea->m_pVideo = this;

        m_pTextArea->m_iWidth = 128;
        m_pTextArea->m_iHeight = 16;

        m_pTextArea->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pTextArea->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pTextArea->m_iWidth, m_pTextArea->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pTextArea->m_pSurface)
        SDL_SetColorKey(m_pTextArea->m_pSurface,
                        OGE_SRCCOLORKEY,
                        FormatColor(0xff00ff));

        m_pTextArea->SetPenColor(-1);
	}

	if (!m_pClipboardA)
	{
		m_pClipboardA = new CogeImage("MAIN_SCREEN_CLIPBOARD_A");
		m_pClipboardA->m_pDotFont = this->m_pDotFont;

		m_pClipboardA->m_pVideo = this;

        m_pClipboardA->m_iWidth = iWidth;
        m_pClipboardA->m_iHeight = iHeight;

        m_pClipboardA->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pClipboardA->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pClipboardA->m_iWidth, m_pClipboardA->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pClipboardA->m_pSurface)
        SDL_FillRect(m_pClipboardA->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

        m_pClipboardA->SetPenColor(-1);
	}

	if (!m_pClipboardB)
	{
		m_pClipboardB = new CogeImage("MAIN_SCREEN_CLIPBOARD_B");
		m_pClipboardB->m_pDotFont = this->m_pDotFont;

		m_pClipboardB->m_pVideo = this;

        m_pClipboardB->m_iWidth = iWidth;
        m_pClipboardB->m_iHeight = iHeight;

        m_pClipboardB->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pClipboardB->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pClipboardB->m_iWidth, m_pClipboardB->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);
        if(m_pClipboardB->m_pSurface)
        SDL_FillRect(m_pClipboardB->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));

        m_pClipboardB->SetPenColor(-1);
	}

	if (!m_pClipboardC)
	{
		m_pClipboardC = new CogeImage("MAIN_SCREEN_CLIPBOARD_C");
		m_pClipboardC->m_pDotFont = this->m_pDotFont;

		m_pClipboardC->m_pVideo = this;

        m_pClipboardC->m_iWidth = iWidth;
        m_pClipboardC->m_iHeight = iHeight;

        m_pClipboardC->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

        m_pClipboardC->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_,
                                         m_pClipboardC->m_iWidth, m_pClipboardC->m_iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);

        if(m_pClipboardC->m_pSurface)
            SDL_FillRect(m_pClipboardC->m_pSurface, 0, SDL_MapRGB(m_pFrontBuffer->format, 0, 0, 0));
        else
            OGE_Log("Fail to create Surface for ClipboardC\n");

        m_pClipboardC->SetPenColor(-1);
	}


    if (!m_pMainScreen)
	{
		//m_pMainScreen = new CogeImage("MAIN_SCREEN");
		//m_pMainScreen->m_pDotFont = this->m_pDotFont;
		m_pMainScreen = m_pDefaultScreen;

	}

	//dynamic_cast<CbrzImage*>(m_pMainScreen)->setSurface(m_pddBackBuffer);
	//((CbrzImage*)m_pMainScreen)->setSurface(m_pddBackBuffer);

    m_ViewRect.w = m_iWidth;
    m_ViewRect.h = m_iHeight;

    //LockFPS(33);
    //LockFPS(100);
    UnlockFPS();

    m_iState = 0;

    return m_iState;
}

void CogeVideo::Finalize()
{
    m_iState = -1;

    DelAllImages();

    if (m_pClipboardA)
    {
        delete m_pClipboardA;
        m_pClipboardA = NULL;
    }

    if (m_pClipboardB)
    {
        delete m_pClipboardB;
        m_pClipboardB = NULL;
    }

    if (m_pClipboardC)
    {
        delete m_pClipboardC;
        m_pClipboardC = NULL;
    }

    if (m_pTextArea)
    {
        delete m_pTextArea;
        m_pTextArea = NULL;
    }

    if (m_pDefaultBg)
    {
        delete m_pDefaultBg;
        m_pDefaultBg = NULL;
    }

    if (m_pLastScreen)
    {
        delete m_pLastScreen;
        m_pLastScreen = NULL;
    }

    m_pDefaultFont = NULL;

    if (m_pFontStock)
    {
        delete m_pFontStock;
        m_pFontStock = NULL;
    }

    if (m_pDotFont)
    {
        delete m_pDotFont;
        m_pDotFont = NULL;
    }

    if (m_pMainScreen) m_pMainScreen = NULL;

    if (m_pDefaultScreen) delete m_pDefaultScreen;

    if (m_pFrontBuffer)
    {
        SDL_FreeSurface(m_pFrontBuffer);
        m_pFrontBuffer = NULL;
    }

    g_pMainSurface = NULL;

    if (m_pWindowIcon)
    {
        SDL_FreeSurface(m_pWindowIcon);
        m_pWindowIcon = NULL;
    }

#ifdef __OGE_WITH_GLWIN__

    if (m_pMainTexture)
    {
        SDL_DestroyTexture(m_pMainTexture);
        m_pMainTexture = NULL;
    }

#endif

    //printf("\n End of CogeVideo::Finalize(): %d \n", SDL_GetTicks());
    //return 0;
}

int CogeVideo::Resume()
{
	if (m_iState < 0) return m_iState;
	if (m_iState == 0)
    {
        m_iState = 1;

#ifdef __OGE_WITH_GLWIN__
        if(m_pMainWindow)
        {
            //SDL_ShowWindow(m_pMainWindow);
            //SDL_RaiseWindow(m_pMainWindow);
        }
#endif

    }
	return m_iState;
}

int CogeVideo::Pause()
{
	if (m_iState < 0) return m_iState;
	if (m_iState > 0)
    {
        m_iState = 0;

#ifdef __OGE_WITH_GLWIN__
        if(m_pMainWindow)
        {
            //SDL_HideWindow(m_pMainWindow);
        }
#endif

    }
	return m_iState;
}

// Width getter
int CogeVideo::GetWidth()
{
	return m_iWidth;
}

// Height getter
int CogeVideo::GetHeight()
{
	return m_iHeight;
}

// BPP getter
int CogeVideo::GetBPP()
{
	return m_iBPP;
}

int CogeVideo::GetWindowID()
{
    char* pValue = getenv("SDL_WINDOWID");
    int iRsl = -1;
    if (pValue)
    {
        iRsl = atoi(pValue);
    }
    return iRsl;
}

bool CogeVideo::GetFullScreen()
{
    return (m_iMode & OGE_FULLSCREEN) != 0;
}

void CogeVideo::SetFullScreen(bool bValue)
{
    if(GetFullScreen() != bValue)
    {

#if !defined(__WINCE__) && !defined(__ANDROID__) && !defined(__IPHONE__)


#ifdef __OGE_WITH_GLWIN__

        bool bOK = false;
        if(m_pMainWindow)
        {
            if(bValue) bOK = SDL_SetWindowFullscreen(m_pMainWindow, SDL_TRUE) == 0;
            else bOK = SDL_SetWindowFullscreen(m_pMainWindow, SDL_FALSE) == 0;
        }
        if(bOK)
        {
            if(bValue) m_iMode = m_iMode | OGE_FULLSCREEN;
            else m_iMode = m_iMode & (~OGE_FULLSCREEN);
        }


#else

#if defined(__LINUX__)
        int iOK = 0;
        if(m_pFrontBuffer)
        {
            iOK = SDL_WM_ToggleFullScreen(m_pFrontBuffer);
        }
        if(iOK)
        {
            if(bValue) m_iMode = m_iMode | SDL_FULLSCREEN;
            else m_iMode = m_iMode & (~SDL_FULLSCREEN);
        }
#else

        if(m_pFrontBuffer)
        {
            int iWidth  = m_pFrontBuffer->w;
            int iHeight = m_pFrontBuffer->h;
            int iBPP = m_pFrontBuffer->format->BitsPerPixel;

            int iFlags = m_iMode;

            SDL_FreeSurface(m_pFrontBuffer);

            if(bValue) iFlags = iFlags | SDL_FULLSCREEN;
            else iFlags = iFlags & (~SDL_FULLSCREEN);

            m_pFrontBuffer = SDL_SetVideoMode(iWidth, iHeight, iBPP, iFlags);

            if(m_pFrontBuffer) m_iMode = iFlags;

        }

#endif

#endif

#endif

    }
}

// Window ID getter
//void* CogeVideo::GetWindowID()
//{
//	return m_iWindowID;
//}

// Mode Flag getter
int CogeVideo::GetMode()
{
	return m_iMode;
}

// Active Flag getter
int CogeVideo::GetState()
{
	return m_iState;
}

// FPS Flag getter
int CogeVideo::GetFPS()
{
	return m_iFrameRate;
}

int CogeVideo::GetScreenUpdateInterval()
{
    return m_iFrameInterval;
}

bool CogeVideo::GetRealPos(double in_x, double in_y, int* out_x, int* out_y)
{
    if(!m_bNeedStretch) return false;
    else
    {
        *out_x = lround(in_x * m_iWidth / m_iRealWidth);
        *out_y = lround(in_y * m_iHeight / m_iRealHeight);
        return true;
    }
}

void CogeVideo::SetDefaultResPath(const std::string& sPath)
{
    m_sDefaultResPath = sPath;
}

int CogeVideo::FormatColor(int iRGBColor)
{
    if(m_bIsBGRA)
    {
        switch(m_iBPP)
        {
        case 32:
            return ((iRGBColor&0xff000000)>>24) |
                   (((iRGBColor&0x00ff0000)>>16) << 8) |
                   (((iRGBColor&0x0000ff00)>>8) << 16) |
                   ((iRGBColor&0x000000ff) << 24);
        default:
            //return 0;
            int iR = (iRGBColor&0x00ff0000)>>16;
            int iG = (iRGBColor&0x0000ff00)>>8;
            int iB = iRGBColor&0x000000ff;

            return SDL_MapRGB(m_pFrontBuffer->format, iR, iG, iB);

        }
    }
    else
    {
        switch(m_iBPP)
        {
        case 32:
            return iRGBColor;
            break;
        //SDL_BYTEORDER == SDL_LIL_ENDIAN  ??
        //case 24:
        //	return iRGBColor & 0x00ffffff;
        //	break;
        case 16:
            return ((((iRGBColor&0x00ff0000)>>16) << 5 >> 8) << 11) |
                   ((((iRGBColor&0x0000ff00)>>8) << 6 >> 8) << 5)   |
                   ((iRGBColor&0x000000ff) << 5 >> 8);
            break;
        case 15:
            return ((((iRGBColor&0x00ff0000)>>16)<< 5 >> 8) << 10) |
                   ((((iRGBColor&0x0000ff00)>>8) << 5 >> 8) << 5)  |
                   ((iRGBColor&0x000000ff) << 5 >> 8);
            break;
        default:
            //return 0;
            int iR = (iRGBColor&0x00ff0000)>>16;
            int iG = (iRGBColor&0x0000ff00)>>8;
            int iB = iRGBColor&0x000000ff;

            return SDL_MapRGB(m_pFrontBuffer->format, iR, iG, iB);

        }
    }

}

int CogeVideo::StringToColor(const std::string& sValue)
{
    size_t iFirst = sValue.find(',');
    if(iFirst == std::string::npos) return atoi(sValue.c_str());

    std::string sRed = sValue.substr(0, iFirst);
    if(sRed.length() <= 0) return -1;

    size_t iLast = sValue.find_last_of(',');
    if(iFirst >= iLast) return -1;

    std::string sGreen = sValue.substr(iFirst+1, iLast-iFirst-1);
    if(sGreen.length() <= 0) return -1;

    std::string sBlue = sValue.substr(iLast+1);
    if(sBlue.length() <= 0) return -1;

    int iRed   = atoi(sRed.c_str());
    int iGreen = atoi(sGreen.c_str());
    int iBlue  = atoi(sBlue.c_str());

    return iRed << 16 | iGreen << 8 | iBlue;


}

int CogeVideo::StringToColor(const std::string& sValue, int* iRed, int* iGreen, int* iBlue)
{
    size_t iFirst = sValue.find(',');
    if(iFirst == std::string::npos) return atoi(sValue.c_str());

    std::string sRed = sValue.substr(0, iFirst);
    if(sRed.length() <= 0) return -1;

    size_t iLast = sValue.find_last_of(',');
    if(iFirst >= iLast) return -1;

    std::string sGreen = sValue.substr(iFirst+1, iLast-iFirst-1);
    if(sGreen.length() <= 0) return -1;

    std::string sBlue = sValue.substr(iLast+1);
    if(sBlue.length() <= 0) return -1;

    *iRed   = atoi(sRed.c_str());
    *iGreen = atoi(sGreen.c_str());
    *iBlue  = atoi(sBlue.c_str());

    return (*iRed) << 16 | (*iGreen) << 8 | (*iBlue);

}

int CogeVideo::StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer)
{
    if(m_pFontStock) return m_pFontStock->StringToUnicode(sText, sCharsetName, pUnicodeBuffer);
    else return 0;
}

std::string CogeVideo::UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName)
{
    if(m_pFontStock) return m_pFontStock->UnicodeToString(pUnicodeBuffer, iBufferSize, sCharsetName);
    else return "";
}

void CogeVideo::SetWindowCaption(const std::string& sCaption)
{

#if defined(__ANDROID__) || defined(__IPHONE__)

    return;

#else


#ifdef __OGE_WITH_GLWIN__
    if(m_pMainWindow)
    {
        SDL_SetWindowTitle(m_pMainWindow, sCaption.c_str());
    }
    else
    {
#if SDL_VERSION_ATLEAST(2,0,0)
        return;
#else
        SDL_WM_SetCaption(sCaption.c_str(), NULL);
#endif
    }
#else
    SDL_WM_SetCaption(sCaption.c_str(), NULL);
#endif


#endif

    //SDL_WM_SetCaption(sCaption.c_str(), NULL);

}

void CogeVideo::SetWindowIcon(const std::string& sIconFilePath, const std::string& sIconMask)
{

/*
#ifdef __OGE_WITH_SDL2__

    if (m_pWindowIcon)
    {
        SDL_FreeSurface(m_pWindowIcon);
        m_pWindowIcon = NULL;
    }

    int iRed = 0; int iGreen = 0; int iBlue = 0;
    int colorkey = StringToColor(sIconMask, &iRed, &iGreen, &iBlue);

    m_pWindowIcon = SDL_LoadBMP(sIconFilePath.c_str());

    if (colorkey >= 0)
    {
        colorkey = SDL_MapRGB(m_pWindowIcon->format, iRed, iGreen, iBlue);
        SDL_SetColorKey(m_pWindowIcon, SDL_SRCCOLORKEY, colorkey);
    }

    if(m_pMainWindow) SDL_SetWindowIcon(m_pMainWindow, m_pWindowIcon);

#else



    if(m_pFrontBuffer) return; // if already called SDL_SetVideoMode() then do nothing ...

    if (m_pWindowIcon)
    {
        SDL_FreeSurface(m_pWindowIcon);
        m_pWindowIcon = NULL;
    }

    int iRed = 0; int iGreen = 0; int iBlue = 0;
    int colorkey = StringToColor(sIconMask, &iRed, &iGreen, &iBlue);

    m_pWindowIcon = SDL_LoadBMP(sIconFilePath.c_str());

    if (colorkey >= 0)
    {
        colorkey = SDL_MapRGB(m_pWindowIcon->format, iRed, iGreen, iBlue);
        SDL_SetColorKey(m_pWindowIcon, SDL_SRCCOLORKEY, colorkey);
    }

    SDL_WM_SetIcon(m_pWindowIcon, NULL);

#endif

*/


#if defined(__ANDROID__) || defined(__IPHONE__)

    return;

#else


#ifndef __OGE_WITH_SDL2__
    if(m_pFrontBuffer) return; // if already called SDL_SetVideoMode() then do nothing ...
#endif

    if (m_pWindowIcon)
    {
        SDL_FreeSurface(m_pWindowIcon);
        m_pWindowIcon = NULL;
    }

    int iRed = 0; int iGreen = 0; int iBlue = 0;
    int colorkey = StringToColor(sIconMask, &iRed, &iGreen, &iBlue);

    //m_pWindowIcon = OGE_LoadBMP(sIconFilePath.c_str());
    m_pWindowIcon = SDL_LoadBMP(sIconFilePath.c_str());

    if (m_pWindowIcon && colorkey >= 0)
    {
        colorkey = SDL_MapRGB(m_pWindowIcon->format, iRed, iGreen, iBlue);
        SDL_SetColorKey(m_pWindowIcon, OGE_SRCCOLORKEY, colorkey);
    }

#ifdef __OGE_WITH_GLWIN__
    if(m_pMainWindow && m_pWindowIcon)
    {
        SDL_SetWindowIcon(m_pMainWindow, m_pWindowIcon);
    }
    else if(m_pWindowIcon)
    {
#if SDL_VERSION_ATLEAST(2,0,0)
        return;
#else
        SDL_WM_SetIcon(m_pWindowIcon, NULL);
#endif
    }
#else
    if(m_pWindowIcon) SDL_WM_SetIcon(m_pWindowIcon, NULL);
#endif


#endif

}

void* CogeVideo::GetRawSurface()
{
    return (void*) m_pFrontBuffer;
}

void* CogeVideo::GetMainWindow()
{
    return (void*) m_pMainWindow;
}

CogeImage* CogeVideo::GetScreen()
{
	return m_pMainScreen;
}

void CogeVideo::SetScreen(CogeImage* pTheScreen)
{
    if(m_iState < 0 ||
       pTheScreen == NULL ||
       m_pMainScreen == pTheScreen) return;

    int iOldState = GetState();
	if(iOldState > 0) Pause();
	m_pMainScreen = pTheScreen;
	if(iOldState > 0) Resume();
}

void CogeVideo::DefaultScreen()
{
    if(m_iState < 0 ||
       m_pDefaultScreen == NULL ||
       m_pMainScreen == m_pDefaultScreen) return;

    int iOldState = GetState();
	if(iOldState > 0) Pause();
	m_pMainScreen = m_pDefaultScreen;
	if(iOldState > 0) Resume();
}

CogeImage* CogeVideo::GetDefaultScreen()
{
    return m_pDefaultScreen;
}

CogeImage* CogeVideo::GetLastScreen()
{
    return m_pLastScreen;
}

CogeImage* CogeVideo::GetClipboardA()
{
    return m_pClipboardA;
}
CogeImage* CogeVideo::GetClipboardB()
{
    return m_pClipboardB;
}
CogeImage* CogeVideo::GetClipboardC()
{
    return m_pClipboardC;
}

CogeImage* CogeVideo::GetDefaultBg()
{
    return m_pDefaultBg;
}

void CogeVideo::ClearDefaultBg(int iRGBColor)
{
    if(m_iState < 0 || m_pDefaultBg == NULL ) return;

    size_t iColor = iRGBColor;
    SDL_FillRect(m_pDefaultBg->m_pSurface, 0, iColor);

    //m_pDefaultBg->FillRect(iRGBColor);
}

void CogeVideo::SetView(int x, int y)
{
    m_ViewRect.x = x;
    m_ViewRect.y = y;
}

// Active Flag getter
void CogeVideo::DrawFPS(int x, int y)
{
    char sFPS[10];

    if (m_iFrameRate >= 1000)
        sprintf(sFPS,"FPS:%d",999);
    else
        sprintf(sFPS,"FPS:%d", m_iFrameRate);

    /*
    m_pTextArea->FillRect(0xff00ff);
    m_pTextArea->DotTextOut(sFPS, x, y);
    m_pMainScreen->Draw(m_pTextArea, x, y);
    */

    m_pMainScreen->DotTextOut(sFPS, x, y);

}

int CogeVideo::GetLockedFPS()
{
	return m_iLockedFPS;
}

bool CogeVideo::IsFPSChanged()
{
    return (m_iOldFrameRate != m_iFrameRate);
}

void CogeVideo::LockFPS(int iFPS)
{
    m_iFrameRateWanted = iFPS;
	m_iFrameInterval = 1000/iFPS;
	m_iLockedFPS = iFPS;
}
void CogeVideo::UnlockFPS()
{
	m_iLockedFPS = 0;
}

bool CogeVideo::IsBGRAMode()
{
    return m_bIsBGRA;
}

void CogeVideo::FillRect(int iRGBColor, int iLeft, int iTop, int iWidth, int iHeight)
{
    //if (m_iState < 0) return;
    //if (!m_pMainScreen) return;
    m_pMainScreen->FillRect(iRGBColor, iLeft, iTop, iWidth, iHeight);
}

void CogeVideo::DrawImage(CogeImage* pSrcImage,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    //if (m_iState < 0) return;

    //if (m_pMainScreen && pSrcImage)
    //{

    //iDstLeft = m_iViewX + iDstLeft;
    //iDstTop  = m_iViewY + iDstTop;


    m_pMainScreen->Draw(pSrcImage,
                        iDstLeft,  iDstTop,  //iDstWidth,  iDstHeight,
                        iSrcLeft,  iSrcTop,  iSrcWidth,  iSrcHeight);
    //}

}

void CogeVideo::DrawImage(const std::string& sSrcImageName,
                           int iDstLeft, int iDstTop,
                           int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    //if (m_iState < 0) return;
    //if (!m_pMainScreen) return;

    CogeImage* pSrcImage = FindImage(sSrcImageName);

    if (pSrcImage)
    {

    //iDstLeft = m_iViewX + iDstLeft;
    //iDstTop  = m_iViewY + iDstTop;


    m_pMainScreen->Draw(pSrcImage,
                        iDstLeft,  iDstTop,  //iDstWidth,  iDstHeight,
                        iSrcLeft,  iSrcTop,  iSrcWidth,  iSrcHeight);
    }


}

void CogeVideo::DotTextOut(char* pText, int x, int y,
                            int iMaxWidth, int iFontSpace , int iLineSpace)
{
    //if (m_iState < 0) return;
    //if (!m_pDotFont) return;
    //if (!m_pMainScreen) return;

    if(m_pMainScreen)
    m_pMainScreen->DotTextOut(pText, x, y, iMaxWidth, iFontSpace, iLineSpace);
}

void CogeVideo::PrintText(const std::string& sText, int x, int y, CogeFont* pFont, int iAlpha)
{
    if(m_pMainScreen)
    {
        if(pFont == NULL) pFont = m_pDefaultFont;
        m_pMainScreen->PrintText(sText, x, y, pFont, iAlpha);
    }
}

int CogeVideo::Update(int x, int y)
{
    if (m_iState < 0) return 0;

    // get FPS

	int iCurrentTime = SDL_GetTicks();

	if (m_iLockedFPS > 0 && m_iFrameInterval > 0)
	{
	    /*
		while (iCurrentTime - m_iGlobalTime < m_iFrameInterval)
		{
			SDL_Delay(10);  // cool down cpu time ...
			iCurrentTime = SDL_GetTicks();
		}
		*/

		if (iCurrentTime - m_iGlobalTime < m_iFrameInterval)
		{
			//SDL_Delay(10);  // cool down cpu time ...
			//iCurrentTime = SDL_GetTicks();
			return 0;
		}

		//if(iCurrentTime - m_iGlobalTime < m_iFrameInterval) return 0;
		// QueryPerformanceCounter(iNowTime);

	}
	else m_iFrameInterval = iCurrentTime - m_iGlobalTime;

	m_iFrameCount++;

	if (iCurrentTime - m_iLastFrameTime >= 1000)
	{
	    m_iOldFrameRate = m_iFrameRate;
		m_iFrameRate = m_iFrameCount;
		m_iFrameCount = 0;
		m_iLastFrameTime = iCurrentTime;

		//if(m_iFrameRate < m_iFrameRateWanted-5) m_iFrameInterval -= 2;
		//if(m_iFrameRate > m_iFrameRateWanted+5) m_iFrameInterval += 2;
		//if (m_bTimerActive) m_iTimerTime++;
	}

	m_iGlobalTime = iCurrentTime;

	// flip

    m_ViewRect.x = x;
    m_ViewRect.y = y;


#ifdef __OGE_WITH_GLWIN__

    //if(m_iState == 0 && m_pMainWindow)
    //{
    //    //SDL_Flip(m_pFrontBuffer);
    //    SDL_UpdateWindowSurface(m_pMainWindow);
    //}

    /*
    if (m_iState > 0 && m_pMainTexture && m_pMainScreen && (m_pMainScreen->m_pSurface))
	{
	    void* pSrc = NULL;
        void* pDst = NULL;
        int iSrcLineSize = 0;
        int iDstLineSize = 0;

        //OGE_Log("Flip: --- Begin --- \n");

        //SDL_LockSurface(m_pMainScreen->m_pSurface);
        SDL_LockTexture(m_pMainTexture, NULL, &pDst, &iDstLineSize);

        pSrc = (void *)m_pMainScreen->m_pSurface->pixels;
        iSrcLineSize = m_pMainScreen->m_pSurface->pitch;

        OGE_FX_CopyRect((uint8_t*)pDst, iDstLineSize, 0, 0,
                        (uint8_t*)pSrc, iSrcLineSize,
                        m_ViewRect.x, m_ViewRect.y, m_ViewRect.w, m_ViewRect.h, m_iBPP);

        SDL_UnlockTexture(m_pMainTexture);
        //SDL_UnlockSurface(m_pMainScreen->m_pSurface);

        SDL_RenderCopy(m_pMainRenderer, m_pMainTexture, NULL, NULL);

        SDL_RenderPresent(m_pMainRenderer);

        //OGE_Log("Flip: --- End --- \n");

	}
	*/

	if (m_iState > 0) UpdateRenderer(m_pMainScreen->m_pSurface);


#else

	if (m_iState > 0 && m_pFrontBuffer && m_pMainScreen && (m_pMainScreen->m_pSurface))
	{
	    if(m_bNeedStretch)
        {
            /*
            SDL_Rect rcDst = {0};

            rcDst.x = 0;
            rcDst.y = 0;
            rcDst.w = m_iRealWidth;
            rcDst.y = m_iRealHeight;

            SDL_SoftStretch(m_pMainScreen->m_pSurface, &m_ViewRect, m_pFrontBuffer, &rcDst);
            */


            //bool bNeedLockSrc = SDL_MUSTLOCK(m_pMainScreen->m_pSurface);
            //if(bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(m_pMainScreen->m_pSurface) == 0;
            //bool bNeedLockDst = SDL_MUSTLOCK(m_pFrontBuffer);
            //if(bNeedLockDst) bNeedLockDst = SDL_LockSurface(m_pFrontBuffer) == 0;

            uint8_t* pSrc = (Uint8 *)m_pMainScreen->m_pSurface->pixels;
            int iSrcLineSize = m_pMainScreen->m_pSurface->pitch;

            uint8_t* pDst = (Uint8 *)m_pFrontBuffer->pixels;
            int iDstLineSize = m_pFrontBuffer->pitch;

            OGE_FX_BltStretch(pDst, iDstLineSize,
                                0, 0, m_iRealWidth, m_iRealHeight,
                                pSrc, iSrcLineSize, -1,
                                m_ViewRect.x, m_ViewRect.y, m_ViewRect.w, m_ViewRect.h, m_iBPP);

            //if (bNeedLockSrc) SDL_UnlockSurface(m_pMainScreen->m_pSurface);
            //if (bNeedLockDst) SDL_UnlockSurface(m_pFrontBuffer);

        }
        else
        {

#if defined(__ANDROID__) || defined(__IPHONE__)

            //bool bNeedLockSrc = SDL_MUSTLOCK(m_pMainScreen->m_pSurface);
            //if(bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(m_pMainScreen->m_pSurface) == 0;
            //bool bNeedLockDst = SDL_MUSTLOCK(m_pFrontBuffer);
            //if(bNeedLockDst) bNeedLockDst = SDL_LockSurface(m_pFrontBuffer) == 0;

            uint8_t* pSrc = (Uint8 *)m_pMainScreen->m_pSurface->pixels;
            int iSrcLineSize = m_pMainScreen->m_pSurface->pitch;

            uint8_t* pDst = (Uint8 *)m_pFrontBuffer->pixels;
            int iDstLineSize = m_pFrontBuffer->pitch;

            OGE_FX_CopyRect(pDst, iDstLineSize,
                        0, 0,
                        pSrc, iSrcLineSize,
                        m_ViewRect.x, m_ViewRect.y,
                        m_ViewRect.w, m_ViewRect.h, m_iBPP);

            //if (bNeedLockSrc) SDL_UnlockSurface(m_pMainScreen->m_pSurface);
            //if (bNeedLockDst) SDL_UnlockSurface(m_pFrontBuffer);

#else

            SDL_BlitSurface(m_pMainScreen->m_pSurface, &m_ViewRect, m_pFrontBuffer, 0);

#endif

        }


        //SDL_UpdateRect(m_pFrontBuffer, 0, 0, m_iWidth, m_iHeight);

        SDL_Flip(m_pFrontBuffer);

	}

#endif

	return 1;

}

void CogeVideo::UpdateRenderer(void* pSurface, int x, int y, unsigned int w, unsigned int h)
{

#ifdef __OGE_WITH_GLWIN__

    //OGE_Log("Calling UpdateRenderer(): %d ... ", (int)pSurface);

    if(pSurface == (void*)(m_pMainScreen->m_pSurface))
    {
        if (m_pMainTexture && m_pMainScreen)
        {
            void* pSrc = NULL;
            void* pDst = NULL;
            int iSrcLineSize = 0;
            int iDstLineSize = 0;

            //OGE_Log("Flip: --- Begin --- \n");

            //SDL_LockSurface(m_pMainScreen->m_pSurface);
            SDL_LockTexture(m_pMainTexture, NULL, &pDst, &iDstLineSize);

            pSrc = (void *)m_pMainScreen->m_pSurface->pixels;
            iSrcLineSize = m_pMainScreen->m_pSurface->pitch;

            OGE_FX_CopyRect((uint8_t*)pDst, iDstLineSize, 0, 0,
                            (uint8_t*)pSrc, iSrcLineSize,
                            m_ViewRect.x, m_ViewRect.y, m_ViewRect.w, m_ViewRect.h, m_iBPP);

            SDL_UnlockTexture(m_pMainTexture);
            //SDL_UnlockSurface(m_pMainScreen->m_pSurface);

            SDL_RenderCopy(m_pMainRenderer, m_pMainTexture, NULL, NULL);

            SDL_RenderPresent(m_pMainRenderer);

            //OGE_Log("Flip: --- End --- \n");

        }

    }
    else
    {
        SDL_Surface* pCurrentSurface = (SDL_Surface*) pSurface;

        if (m_pMainTexture && pCurrentSurface)
        {
            void* pSrc = NULL;
            void* pDst = NULL;
            int iSrcLineSize = 0;
            int iDstLineSize = 0;

            //SDL_LockSurface(pCurrentSurface->m_pSurface);
            SDL_LockTexture(m_pMainTexture, NULL, &pDst, &iDstLineSize);

            pSrc = (void *)pCurrentSurface->pixels;
            iSrcLineSize = pCurrentSurface->pitch;

            OGE_FX_CopyRect((uint8_t*)pDst, iDstLineSize, 0, 0,
                            (uint8_t*)pSrc, iSrcLineSize,
                            0, 0, w, h, m_iBPP);

            SDL_UnlockTexture(m_pMainTexture);
            //SDL_UnlockSurface(pCurrentSurface->m_pSurface);

            SDL_RenderCopy(m_pMainRenderer, m_pMainTexture, NULL, NULL);

            SDL_RenderPresent(m_pMainRenderer);
        }

    }

#endif

}

CogeImage* CogeVideo::NewImage(const std::string& sName,
                               int iWidth, int iHeight, int iColorKeyRGB,
                               bool bLoadAlphaChannel, bool bCreateLocalClipboard,
                               const std::string& sFileName)
{
    CogeImage* pTheNewImage = NULL;

	ogeImageMap::iterator it;

    it = m_ImageMap.find(sName);
	if (it != m_ImageMap.end())
	{
        OGE_Log("The Image name '%s' is in use.\n", sName.c_str());
        return NULL;
	}

	//if (m_ImageMap.find(sName) != m_ImageMap.end()) return NULL;

	pTheNewImage = new CogeImage(sName);
	// set parent
	pTheNewImage->m_pVideo = this;

	// create surface
	if (sFileName.length() > 0) pTheNewImage->LoadData(sFileName, bLoadAlphaChannel, bCreateLocalClipboard);

	// create empty surface if in need
	if (pTheNewImage->m_pSurface == NULL)
	{
	    pTheNewImage->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);

    }

    if (pTheNewImage->m_pSurface == NULL)
	{
	    delete pTheNewImage;
		return NULL;
	}

	// set size
	pTheNewImage->m_iWidth  = pTheNewImage->m_pSurface->w;
    pTheNewImage->m_iHeight = pTheNewImage->m_pSurface->h;

    pTheNewImage->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

	// set parent
	//pTheNewImage->m_pVideo = this;

	// set default font drawer
	pTheNewImage->m_pDotFont = this->m_pDotFont;

	//pTheNewImage->m_pFontStock = this->m_pFontStock;

	// set default pen color
	pTheNewImage->SetPenColor(-1);

	pTheNewImage->SetColorKey(iColorKeyRGB);

	// set background color
	//pTheNewImage->SetBgColor(iBgColorRGB);

	m_ImageMap.insert(ogeImageMap::value_type(sName, pTheNewImage));

	return pTheNewImage;

}

CogeImage* CogeVideo::NewImageFromBuffer(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB,
                        bool bLoadAlphaChannel, bool bCreateLocalClipboard,
                        char* pBuffer, int iBufferSize)
{
    CogeImage* pTheNewImage = NULL;

	ogeImageMap::iterator it;

    it = m_ImageMap.find(sName);
	if (it != m_ImageMap.end())
	{
        OGE_Log("The Image name '%s' is in use.\n", sName.c_str());
        return NULL;
	}

	//if (m_ImageMap.find(sName) != m_ImageMap.end()) return NULL;

	pTheNewImage = new CogeImage(sName);

	// create surface
	if (pBuffer && iBufferSize > 0)
        pTheNewImage->LoadDataFromBuffer(pBuffer, iBufferSize, bLoadAlphaChannel, bCreateLocalClipboard);

	// create empty surface if in need
	if (pTheNewImage->m_pSurface == NULL)
	{
	    pTheNewImage->m_pSurface = SDL_CreateRGBSurface(_OGE_VIDEO_DF_MODE_, iWidth, iHeight,
                                         m_pFrontBuffer->format->BitsPerPixel,
                                         m_pFrontBuffer->format->Rmask,
                                         m_pFrontBuffer->format->Gmask,
                                         m_pFrontBuffer->format->Bmask,
                                         m_pFrontBuffer->format->Amask);

    }

    if (pTheNewImage->m_pSurface == NULL)
	{
	    delete pTheNewImage;
		return NULL;
	}

	// set size
	pTheNewImage->m_iWidth  = pTheNewImage->m_pSurface->w;
    pTheNewImage->m_iHeight = pTheNewImage->m_pSurface->h;

    pTheNewImage->m_iBPP = m_pFrontBuffer->format->BitsPerPixel;

	// set parent
	pTheNewImage->m_pVideo = this;

	// set default font drawer
	pTheNewImage->m_pDotFont = this->m_pDotFont;

	// set default pen color
	pTheNewImage->SetPenColor(-1);

	pTheNewImage->SetColorKey(iColorKeyRGB);

	m_ImageMap.insert(ogeImageMap::value_type(sName, pTheNewImage));

	return pTheNewImage;
}

CogeImage* CogeVideo::FindImage(const std::string& sName)
{
    ogeImageMap::iterator it;

    it = m_ImageMap.find(sName);
	if (it != m_ImageMap.end()) return it->second;
	else return NULL;

}

CogeImage* CogeVideo::GetImage(const std::string& sName,
                               int iWidth, int iHeight, int iColorKeyRGB,
                               bool bLoadAlphaChannel, bool bCreateLocalClipboard,
                               const std::string& sFileName)
{
    CogeImage* pMatchedImage = FindImage(sName);
    if (!pMatchedImage)
        pMatchedImage = NewImage(sName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, bCreateLocalClipboard, sFileName);
    //if (pMatchedImage) pMatchedImage->Hire();
    return pMatchedImage;

}

CogeImage* CogeVideo::GetImageFromBuffer(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB,
                        bool bLoadAlphaChannel, bool bCreateLocalClipboard,
                        char* pBuffer, int iBufferSize)
{
    CogeImage* pMatchedImage = FindImage(sName);
    if (!pMatchedImage)
        pMatchedImage = NewImageFromBuffer(sName, iWidth, iHeight, iColorKeyRGB, bLoadAlphaChannel, bCreateLocalClipboard, pBuffer, iBufferSize);
    return pMatchedImage;
}

bool CogeVideo::DelImage(const std::string& sName)
{
    CogeImage* pMatchedImage = NULL;
    ogeImageMap::iterator it = m_ImageMap.find(sName);
	if (it != m_ImageMap.end()) pMatchedImage = it->second;

	if (pMatchedImage)
	{
	    //pMatchedImage->Fire();
	    if (pMatchedImage->m_iTotalUsers > 0) return false;
		m_ImageMap.erase(it);
		delete pMatchedImage;
		return true;

	}
	return false;

}

bool CogeVideo::DelImage(CogeImage* pImage)
{
    if(!pImage) return false;

    CogeImage* pMatchedImage = NULL;
    ogeImageMap::iterator it = m_ImageMap.begin();

    while(it != m_ImageMap.end())
    {
        pMatchedImage = it->second;

        if(pMatchedImage == pImage)
        {
            if (pMatchedImage->m_iTotalUsers > 0) return false;
            else
            {
                m_ImageMap.erase(it);
                delete pMatchedImage;
                return true;
            }
        }

        it++;
    }

    return false;

}

void CogeVideo::DelAllImages()
{
    // free all Images
	ogeImageMap::iterator it;
	CogeImage* pMatchedImage = NULL;

	it = m_ImageMap.begin();
	int iCount = m_ImageMap.size();
	for(int i=0; i<iCount; i++)
	{
	    pMatchedImage = it->second;
	    delete pMatchedImage;
	    it++;
	}
	m_ImageMap.clear();

}

/*
bool CogeVideo::InstallFont(const std::string& sFontName, int iFontSize, const std::string& sFontFileName)
{

    if(m_pFontStock &&
       m_pFontStock->NewFont(sFontName, iFontSize, sFontFileName) != NULL)
    return true;

    printf("Install TTF Font Error: Can not load ttf font file: ");
    printf(sFontFileName.c_str());
    printf("\n");

    return false;

}
void CogeVideo::UninstallFont(const std::string& sFontName)
{
    if(m_pFontStock) m_pFontStock->DelFont(sFontName);
}

bool CogeVideo::UseFont(const std::string& sFontName)
{
    if(m_pFontStock) return m_pFontStock->PrepareFont(sFontName) >= 0;
    else return false;
}
*/

CogeFont* CogeVideo::NewFont(const std::string& sFontName, int iFontSize, const std::string& sFileName)
{
    if(m_pFontStock) return m_pFontStock->NewFont(sFontName, iFontSize, sFileName);
    else return NULL;
}
CogeFont* CogeVideo::NewFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize)
{
    if(m_pFontStock) return m_pFontStock->NewFontFromBuffer(sFontName, iFontSize, pBuffer, iBufferSize);
    else return NULL;
}
CogeFont* CogeVideo::FindFont(const std::string& sFontName, int iFontSize)
{
    if(m_pFontStock) return m_pFontStock->FindFont(sFontName, iFontSize);
    else return NULL;
}
CogeFont* CogeVideo::GetFont(const std::string& sFontName, int iFontSize, const std::string& sFileName)
{
    if(m_pFontStock) return m_pFontStock->GetFont(sFontName, iFontSize, sFileName);
    else return NULL;
}
int CogeVideo::DelFont(const std::string& sFontName, int iFontSize)
{
    if(m_pFontStock) return m_pFontStock->DelFont(sFontName, iFontSize);
    else return -1;
}

CogeFont* CogeVideo::GetDefaultFont()
{
    return m_pDefaultFont;
}
void CogeVideo::SetDefaultFont(CogeFont* pFont)
{
    //if(m_pDefaultFont != pFont) m_pDefaultFont = pFont;

    if(m_pDefaultFont == pFont) return;

    if(m_pDefaultFont) m_pDefaultFont->Fire();
    m_pDefaultFont = pFont;
    if(m_pDefaultFont) m_pDefaultFont->Hire();
}

const std::string& CogeVideo::GetDefaultCharset()
{
    if(m_pFontStock) return m_pFontStock->GetDefaultCharset();
    else return _OGE_EMPTY_STR_;
}
void CogeVideo::SetDefaultCharset(const std::string& sDefaultCharset)
{
    if(m_pFontStock) m_pFontStock->SetDefaultCharset(sDefaultCharset);
}

const std::string& CogeVideo::GetSystemCharset()
{
    if(m_pFontStock) return m_pFontStock->GetSystemCharset();
    else return _OGE_EMPTY_STR_;
}
void CogeVideo::SetSystemCharset(const std::string& sSystemCharset)
{
    if(m_pFontStock) m_pFontStock->SetSystemCharset(sSystemCharset);
}

/*--------------- Dot Matrix Font ----------------*/

//constructor
CogeDotFont::CogeDotFont()
//m_pVideo(pVideo)
{
	//asc12 = NULL;
	asc16 = NULL;
	//hzk12 = NULL;
	hzk16 = NULL;

}

//destructor
CogeDotFont::~CogeDotFont()
{
	//if(asc12) { delete asc12; asc12=NULL; }
	//if(asc16) { delete asc16; asc16=NULL; }
	if(asc16) { free(asc16); asc16=NULL; }

	//if(hzk12) { delete hzk12; hzk12=NULL; }
	//if(hzk16) { delete hzk16; hzk16=NULL; }
	if(hzk16) { free(hzk16); hzk16=NULL; }

}


bool CogeDotFont::InstallFont(const char* sFontFileName, int iType)
{
	bool bResult = false;
	std::ifstream fontfile;
	fontfile.open(sFontFileName, std::ios::binary);
	if (!fontfile.is_open()) return false;

	switch(iType)
	{
    case 0:
        asc16 = (char*)malloc(160*16);
		fontfile.read(asc16, 160*16);
		bResult = true;
		break;
    case 1:
        hzk16 = (char*)malloc(94*87*16 * 2);
		fontfile.read(hzk16, 94*87*16 * 2);
		bResult = true;
		break;
    }

    /*
	if (sType == "_asc12_")
	{
		asc12 = (char*)malloc(160*12);
		fontfile.read(asc12, 160*12);
		bResult = true;
	}
	else if (sType == "_asc16_")
	{
		asc16 = (char*)malloc(160*16);
		fontfile.read(asc16, 160*16);
		bResult = true;
	}
	else if (sType == "_hzk12_")
	{
		hzk12 = (char*)malloc(94*87*12 * 2);
		fontfile.read(hzk12, 94*87*12 * 2);
		bResult = true;
	}
	else if (sType == "_hzk16_")
	{
		hzk16 = (char*)malloc(94*87*16 * 2);
		fontfile.read(hzk16, 94*87*16 * 2);
		bResult = true;
	}
	*/

	fontfile.close();
	return bResult;
}

void CogeDotFont::UninstallFont(int iType)
{
	// do something ...
}

int CogeDotFont::GetTextWidth(const char* pText, int iFontSpace)
{
	int iResult = 0;
	while((unsigned char)(*pText))
	{
		if ((unsigned char)(*pText) <= 0x00a0) // if ascii
		{
			iResult += (8+iFontSpace);
			pText ++;
		}
		else if ( (pText+1) && (unsigned char)(*pText) > 0x00a0 && (unsigned char)(*(pText+1)) > 0x00a0 ) // if hzk
		{
			iResult += (_OGE_DOT_FONT_SIZE_+iFontSpace);
			pText += 2;
		}
		else pText ++;
	}
	return iResult;

}

int CogeDotFont::GetTextHeight(const char* pText, int iMaxWidth,
                               int iFontSpace, int iLineSpace)
{
	int iResult = 0;
	int x = 0;
	if((unsigned char)(*pText) != 0) iResult += (_OGE_DOT_FONT_SIZE_+iLineSpace);
	bool bNextLine = (unsigned char)(*pText) == 10 || (unsigned char)(*pText) == 13;
	while((unsigned char)(*pText))
	{
		if (bNextLine)
		{
			iResult += (_OGE_DOT_FONT_SIZE_+iLineSpace);
			x = 0;
			bNextLine = false;
			continue;
		}

		if ((unsigned char)(*pText) <= 0x00a0) // if ascii
		{
			if ((unsigned char)(*pText) == 10 || (unsigned char)(*pText) == 13)
			{
				pText ++;
				bNextLine = true;
                continue;
			}

			if (x+8+iFontSpace > iMaxWidth)
			{
				bNextLine = true;
                continue;
			}

			x += (8+iFontSpace);
			pText ++;
		}
		else if ( (pText+1) && (unsigned char)(*pText) > 0x00a0 && (unsigned char)(*(pText+1)) > 0x00a0 ) // if hzk
		{
			if (x+_OGE_DOT_FONT_SIZE_+iFontSpace > iMaxWidth)
			{
				bNextLine = true;
                continue;
			}

			x += (_OGE_DOT_FONT_SIZE_+iFontSpace);
			pText += 2;
		}
		else pText ++;
	}

	//if(x > 0 && iResult==0) iResult += (_OGE_DOT_FONT_SIZE_+iLineSpace);

	return iResult;
}

void CogeDotFont::OneLine(const char* pText, char* pDstBuffer, int iLineSize, int iBPP, int iPenColor,
			                             int x, int y, int iWidth, int iFontSpace)
{
	char* pFontData = NULL;
	char* pDst = NULL;
	//int iPenColor = m_pVideo->FormatColor(iPenColorRGB);
	int iMask = 0;
	int iBeginX = x;
	int i, j;

	while((unsigned char)(*pText))
	{
		if ((unsigned char)(*pText) <= 0x00a0) // if ascii
		{
			if (x+8+iFontSpace - iBeginX > iWidth) break;

            /*
			switch (iFontSize)
			{
			case 12:
				pFontData = asc12;
				break;
			case 16:
				pFontData = asc16;
				break;
			default:
				pFontData = asc12;
			}
			*/

            pFontData = asc16;

			if (pFontData) pFontData += ((unsigned char)(*pText)) * _OGE_DOT_FONT_SIZE_;
			else
			{
				pText++;
				continue;
			}

			switch (iBPP)
			{
            case 8:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;
			case 15:
			case 16:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;

			case 32:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;

			} // end of switch

			pText++;

		}  // end if ascii
		else if ( (pText+1) && (unsigned char)(*pText) > 0x00a0 && (unsigned char)(*(pText+1)) > 0x00a0 ) // if hzk
		{
			if (x+_OGE_DOT_FONT_SIZE_+iFontSpace - iBeginX > iWidth) break;

            /*
			switch (iFontSize)
			{
			case 12:
				pFontData = hzk12;
				break;
			case 16:
				pFontData = hzk16;
				break;
			default:
				pFontData = hzk12;
			}
			*/

            pFontData = hzk16;

			if (pFontData) pFontData += ( ((unsigned char)(*pText)-0x00a1)*94 + ((unsigned char)(*(pText+1))-0x00a1) ) * _OGE_DOT_FONT_SIZE_ * 2;
			else
			{
				pText += 2;
				continue;
			}

			switch (iBPP)
			{
            case 8:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;
			case 15:
			case 16:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;

			case 32:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;

			} // end of switch

			pText += 2;

		}  // end of if hzk ...
		else
		{
			pText++;
		}

	} // end while ...


}

void CogeDotFont::Lines(const char* pText, char* pDstBuffer, int iLineSize, int iBPP, int iPenColor,
			                        int x, int y, int iWidth, int iHeight,
									int iFontSpace, int iLineSpace)
{
	char* pFontData = NULL;
	char* pDst = NULL;
	//int iPenColor = m_pVideo->FormatColor(iPenColorRGB);
	int iMask = 0;
	int iBeginX = x;
	int iBeginY = y;
	int i, j;
	bool bNextLine = (unsigned char)(*pText) == 10 || (unsigned char)(*pText) == 13;

	while((unsigned char)(*pText))
	{
		if (bNextLine)
		{
			y += (_OGE_DOT_FONT_SIZE_+iLineSpace);
			x = iBeginX;
			bNextLine = false;
			continue;
		}

		if (y+_OGE_DOT_FONT_SIZE_+iLineSpace - iBeginY > iHeight) break;

		if ((unsigned char)(*pText) <= 0x00a0) // if ascii
		{

			if ((unsigned char)(*pText) == 10 || (unsigned char)(*pText) == 13)
			{
				pText ++;
				bNextLine = true;
                continue;
			}

			if (x+8+iFontSpace - iBeginX > iWidth)
			{
				bNextLine = true;
                continue;
			}

            /*
			switch (iFontSize)
			{
			case 12:
				pFontData = asc12;
				break;
			case 16:
				pFontData = asc16;
				break;
			default:
				pFontData = asc12;
			}
			*/

            pFontData = asc16;

			if (pFontData) pFontData += ((unsigned char)(*pText)) * _OGE_DOT_FONT_SIZE_;
			else
			{
				pText++;
				continue;
			}

			switch (iBPP)
			{
            case 8:
                for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;
			case 15:
			case 16:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;

			case 32:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((unsigned char)(*pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData++;
				}
				x += (iFontSpace+8);
				break;

			} // end of switch

			pText++;

		}  // end if ascii
		else if ( (pText+1) && (unsigned char)(*pText) > 0x00a0 && (unsigned char)(*(pText+1)) > 0x00a0 ) // if hzk
		{
			if (x+8+iFontSpace - iBeginX > iWidth)
			{
				bNextLine = true;
                continue;
			}

            /*
			switch (iFontSize)
			{
			case 12:
				pFontData = hzk12;
				break;
			case 16:
				pFontData = hzk16;
				break;
			default:
				pFontData = hzk12;
			}
			*/

            pFontData = hzk16;

			if (pFontData) pFontData += ( ((unsigned char)(*pText)-0x00a1)*94 + ((unsigned char)(*(pText+1))-0x00a1) ) * _OGE_DOT_FONT_SIZE_ * 2;
			else
			{
				pText += 2;
				continue;
			}

			switch (iBPP)
			{
            case 8:
                for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + (x+j);
							(*((char*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;
			case 15:
			case 16:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 2*(x+j);
							(*((short*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;

			case 32:
				for(i=0; i<_OGE_DOT_FONT_SIZE_; i++)
				{
					//iMask = 0x8000;
					pFontData += 1;
					iMask = 0x80;
					for(j=8;j<_OGE_DOT_FONT_SIZE_;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}
					pFontData -= 1;
					iMask = 0x80;
					for(j=0;j<8;j++)
					{
						if (((*(short*)pFontData) & iMask) > 0)  // if there is a dot ...
						{
							pDst = pDstBuffer + iLineSize * (y+i) + 4*(x+j);
							(*((int*)pDst)) = iPenColor;
						}
						iMask >>= 1;
					}

					pFontData += 2;
				}
				x += (iFontSpace+_OGE_DOT_FONT_SIZE_);
				break;

			} // end of switch

			pText += 2;

		}  // end of if hzk ...
		else
		{
			pText++;
		}

	} // end while ...
}



/*---------------- Image -----------------*/


CogeImage::CogeImage(const std::string& sName):
m_pSurface(NULL),
m_pLocalClipboardA(NULL),
m_pLocalClipboardB(NULL),
m_pCurrentClipboard(NULL),
m_pVideo(NULL),
m_pDotFont(NULL),
m_pDefaultFont(NULL),
m_iWidth(0),
m_iHeight(0),
m_iBPP(0),
m_iColorKeyRGB(-1),
m_iColorKey(-1),
m_iAlpha(-1),
m_iPenColorRGB(0),
m_iLockTimes(0),
m_iTotalUsers(0),
m_sName(sName)
{
    m_iPenColor = 0;
    memset(&m_iPenColorSDL, 0, sizeof(m_iPenColorSDL));

    m_bHasAlphaChannel = false;

    m_iEffectCount = 0;
}

CogeImage::~CogeImage()
{
    if (m_pSurface)
    {
        SDL_FreeSurface(m_pSurface);
        m_pSurface = NULL;
    }

    if (m_pLocalClipboardA)
    {
        SDL_FreeSurface(m_pLocalClipboardA);
        m_pLocalClipboardA = NULL;
    }

    if (m_pLocalClipboardB)
    {
        SDL_FreeSurface(m_pLocalClipboardB);
        m_pLocalClipboardB = NULL;
    }

    m_pCurrentClipboard = NULL;

	m_iTotalUsers  =  0;

	m_pDotFont = NULL;
	m_pDefaultFont = NULL;

}

const std::string& CogeImage::GetName()
{
	return m_sName;
}


// Width getter
int CogeImage::GetWidth()
{
	return m_iWidth;
}

// Height getter
int CogeImage::GetHeight()
{
	return m_iHeight;
}

// BPP getter
int CogeImage::GetBPP()
{
	return m_iBPP;
}

bool CogeImage::HasAlphaChannel()
{
    return m_bHasAlphaChannel;
}

bool CogeImage::HasLocalClipboard()
{
    return m_bHasLocalClipboard;
}

int CogeImage::GetPenColor()
{
	return m_iPenColorRGB;
}
void CogeImage::SetPenColor(int iRGBColor)
{

    if(m_iPenColorRGB == iRGBColor) return;

	m_iPenColorRGB = iRGBColor;

	m_iPenColor = m_pVideo->FormatColor(iRGBColor);

	m_iPenColorSDL.r = (iRGBColor&0x00ff0000)>>16;
	m_iPenColorSDL.g = (iRGBColor&0x0000ff00)>>8;
	m_iPenColorSDL.b =  iRGBColor&0x000000ff;

}

void CogeImage::SetColorKey(int iColorKeyRGB)
{
    if (iColorKeyRGB == -1)
    {
        SDL_SetColorKey(m_pSurface, 0, 0);
        m_iColorKeyRGB = -1;
        m_iColorKey = -1;
    }
	else
	{
	    if(m_iColorKeyRGB != iColorKeyRGB)
	    {
	        m_iColorKey = m_pVideo->FormatColor(iColorKeyRGB);
            SDL_SetColorKey(m_pSurface,
                            OGE_SRCCOLORKEY,
                            m_iColorKey);
            m_iColorKeyRGB = iColorKeyRGB;
	    }
	}
}

int CogeImage::GetColorKey()
{
    return m_iColorKeyRGB;
}

int CogeImage::GetRawColorKey()
{
    return m_iColorKey;
}

void CogeImage::SetAlpha(int iAlpha)
{
    if(iAlpha >= 0)
	{
	    if(m_iAlpha != iAlpha)
	    {
            OGE_SetAlpha(m_pSurface, OGE_SRCALPHA, iAlpha);
            m_iAlpha = iAlpha;
	    }
    }
    else
    {
        OGE_SetAlpha(m_pSurface, 0, 0);
        m_iAlpha = -1;
    }
}

void CogeImage::SetDefaultFont(CogeFont* pFont)
{
    if(m_pDefaultFont == pFont) return;

    if(m_pDefaultFont) m_pDefaultFont->Fire();
    m_pDefaultFont = pFont;
    if(m_pDefaultFont) m_pDefaultFont->Hire();

}
CogeFont* CogeImage::GetDefaultFont()
{
    return m_pDefaultFont;
}

//int CogeImage::GetTotalUsers()
//{
//    return m_iTotalUsers;
//}

CogeVideo* CogeImage::GetVideoEngine()
{
    return m_pVideo;
}

int CogeImage::SaveAsBMP(const std::string& sFileName)
{
    if(m_pSurface) return SDL_SaveBMP(m_pSurface, sFileName.c_str());
    else return -1;
}

SDL_Surface * CogeImage::GetSurface()
{
    return m_pSurface;
}

SDL_Surface* CogeImage::GetFreeClipboard()
{
    if(m_pCurrentClipboard && m_pCurrentClipboard == m_pLocalClipboardA) return m_pLocalClipboardB;
    else return m_pLocalClipboardA;
}
SDL_Surface* CogeImage::GetCurrentClipboard()
{
    return m_pCurrentClipboard;
}
void CogeImage::SetCurrentClipboard(SDL_Surface* pClipboard)
{
    if(pClipboard == NULL) m_pCurrentClipboard = NULL;
    else if(pClipboard == m_pLocalClipboardA) m_pCurrentClipboard = m_pLocalClipboardA;
    else if(pClipboard == m_pLocalClipboardB) m_pCurrentClipboard = m_pLocalClipboardB;
}
void CogeImage::PrepareLocalClipboards(int iEffectCount)
{
    SetCurrentClipboard(NULL);
    m_iEffectCount = iEffectCount;
}


void CogeImage::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}

void CogeImage::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}


/*
void CogeImage::GetValidRect(SDL_Rect& rc)
{
//  if(rc.x < 0) rc.x = 0;
//	if(rc.y < 0) rc.y = 0;
//	if(rc.w < 0) rc.w = 0;
//	if(rc.h < 0) rc.h = 0;

	if(rc.x > m_iWidth) rc.x  = m_iWidth;
	if(rc.y > m_iHeight) rc.y = m_iHeight;
	if(rc.x+rc.w > m_iWidth) rc.w = m_iWidth - rc.x;
	if(rc.y+rc.h > m_iHeight) rc.h = m_iHeight - rc.y;
}
*/

bool CogeImage::GetValidRect(int x, int y, int w, int h, SDL_Rect* rslrc)
{
    //SDL_Rect rc = {0};

    //volatile SDL_Rect* rslrc = rc;

    if(x > m_iWidth || y > m_iHeight) return false;

    if(x < 0) {w=w+x; x=0;}
	if(y < 0) {h=h+y; y=0;}

	//if(w < 0) w = 0;
	//if(h < 0) h = 0;

	if(w <= 0 || h <= 0) return false;

	//if(x > m_iWidth) x = m_iWidth;
	//if(y > m_iHeight) y = m_iHeight;

	if(x+w > m_iWidth) w = m_iWidth - x;
	if(y+h > m_iHeight) h = m_iHeight - y;

	rslrc->x = x;
	rslrc->y = y;
	rslrc->w = w;
	rslrc->h = h;

	return true;

}

bool CogeImage::GetValidRect(int x, int y, SDL_Rect* rcRslSrc, SDL_Rect* rcRslDst)
{
    //SDL_Rect rc = {0};

    //volatile SDL_Rect* rcRslSrc = rcSrc;
    //volatile SDL_Rect* rcRslDst = rcDst;

    if(x > m_iWidth || y > m_iHeight) return false;

    int w = rcRslSrc->w;
	int h = rcRslSrc->h;

    if(x < 0) {rcRslSrc->x=rcRslSrc->x-x; w=w+x; x=0;}
	if(y < 0) {rcRslSrc->y=rcRslSrc->y-y; h=h+y; y=0;}

	if(w <= 0 || h <= 0) return false;

	//if(x > m_iWidth) x = m_iWidth;
	//if(y > m_iHeight) y = m_iHeight;

	if(x+w > m_iWidth)  w = m_iWidth - x;
	if(y+h > m_iHeight) h = m_iHeight - y;

	rcRslSrc->w = w;
	rcRslSrc->h = h;

	rcRslDst->x = x;
	rcRslDst->y = y;
	rcRslDst->w = w;
	rcRslDst->h = h;

	return true;
}


void CogeImage::GetValidRect(CogeRect* rslrc)
{
    //volatile CogeRect* rslrc = rc;

    if(rslrc->left < 0) rslrc->left = 0;
	if(rslrc->top < 0) rslrc->top = 0;
	if(rslrc->right < 0) rslrc->right = 0;
	if(rslrc->bottom < 0) rslrc->bottom = 0;

	if(rslrc->left > m_iWidth) rslrc->left = m_iWidth;
	if(rslrc->top > m_iHeight) rslrc->top = m_iHeight;
	if(rslrc->right > m_iWidth) rslrc->right = m_iWidth;
	if(rslrc->bottom > m_iHeight) rslrc->bottom = m_iHeight;
}

void CogeImage::BeginUpdate()
{
#ifdef __OGE_WITH_SDL2__

#ifdef __IMG_WITH_LOCK__
    if(m_iLockTimes < 0) m_iLockTimes = 0;
    if(m_pSurface && SDL_MUSTLOCK(m_pSurface))
    {
        if(SDL_LockSurface(m_pSurface) == 0) m_iLockTimes++;
    }
#else
    return;
#endif

#else

    return;

#endif

}
void CogeImage::EndUpdate()
{

#ifdef __OGE_WITH_SDL2__

#ifdef __IMG_WITH_LOCK__
    if(m_pSurface && m_iLockTimes > 0)
    {
        SDL_UnlockSurface(m_pSurface);
        m_iLockTimes--;
    }
    if(m_iLockTimes < 0) m_iLockTimes = 0;
#else
    return;
#endif

#else

    return;

#endif

}

CogeRect CogeImage::GetDotTextRect(const char* pText, int x, int y,
                                    int iMaxWidth, int iFontSpace, int iLineSpace)
{
    CogeRect rcDst = {0};

	int iWidth = 0;
	int iHeight = 0;

	if (!m_pDotFont) return rcDst;

	if (iMaxWidth == 0)
	{
		iWidth = m_pDotFont->GetTextWidth(pText, iFontSpace) + iFontSpace;
		iHeight = _OGE_DOT_FONT_SIZE_ + iLineSpace;
	}
	else
	{
		iWidth = iMaxWidth + iFontSpace;
		iHeight = m_pDotFont->GetTextHeight(pText, iMaxWidth, iFontSpace, iLineSpace) + iLineSpace;
	}

    rcDst.left   = x;
	rcDst.top    = y;
	rcDst.right  = x + iWidth;
	rcDst.bottom = y  + iHeight;

	return rcDst;

}

void CogeImage::DotTextOut(const char* pText, int x, int y,
                            int iMaxWidth, int iFontSpace, int iLineSpace)
{
    if(m_pDotFont == NULL) return;

    SDL_Rect rcDst = {0};

	void* pDst = NULL;

	int iLineSize = 0;

	int iWidth = 0;
	int iHeight = 0;

	//if (!m_VideoEngine) return;
	//if (!m_pSurface) return;
	//if (!m_pDotFont) return;

	//if ( m_pVideo->m_iState < 0 ) return;

	if (iMaxWidth == 0)
	{
		iWidth = m_pDotFont->GetTextWidth(pText, iFontSpace) + iFontSpace;
		iHeight = _OGE_DOT_FONT_SIZE_ + iLineSpace;
	}
	else
	{
		iWidth = iMaxWidth + iFontSpace;
		iHeight = m_pDotFont->GetTextHeight(pText, iMaxWidth, iFontSpace, iLineSpace) + iLineSpace;
	}

	rcDst.x = x;
	rcDst.y = y;
	rcDst.w = iWidth;
	rcDst.h = iHeight;

	//if (SDL_LockSurface(m_pSurface) < 0) return;

	//try
	//{

	//if (m_pddSurface->Lock(&rcDst, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	if (iMaxWidth == 0)
	{
		m_pDotFont->OneLine(pText, (char*)pDst, iLineSize, m_iBPP, m_iPenColor, rcDst.x, rcDst.y, iWidth, iFontSpace);
	}
	else
	{
		m_pDotFont->Lines(pText, (char*)pDst, iLineSize, m_iBPP, m_iPenColor, rcDst.x, rcDst.y, iWidth, iHeight,
		                  iFontSpace, iLineSpace);
	}

	//}
	//catch ( ... )
	//{
	//    SDL_UnlockSurface( m_pSurface );
    //}

	//m_pddSurface->Unlock(&rcDst);

	//SDL_UnlockSurface( m_pSurface );

	EndUpdate();

}

void CogeImage::PrintBufferText(const char* pBufferText, int iBufferSize, int x, int y, const char* pCharsetName, CogeFont* pFont, int iAlpha)
{
    //if(m_pFontStock == NULL) return;

    if(pBufferText == NULL || iBufferSize <= 0) return;

    SDL_Surface* pTextImg = NULL;

    if(pFont == NULL)
    {
        if(m_pDefaultFont) pFont = m_pDefaultFont;
        else pFont = m_pVideo->GetDefaultFont();
    }

    if(pFont)
    {
        pTextImg = pFont->PrintBufferText(pBufferText, iBufferSize, pCharsetName, m_iPenColorSDL);
    }

    if(pTextImg)
    {
        if(iAlpha >= 0)
        OGE_SetAlpha(pTextImg, OGE_SRCALPHA, iAlpha);

        SDL_Rect rcSrc = {0};
        SDL_Rect rcDst = {0};

        rcSrc.w = pTextImg->w;
        rcSrc.h = pTextImg->h;

        if (GetValidRect(x, y, &rcSrc, &rcDst))
        SDL_BlitSurface(pTextImg, &rcSrc, m_pSurface, &rcDst);

        //SDL_FreeSurface( pTextImg );

    }
    else
    {
        if(pCharsetName == NULL || strlen(pCharsetName) == 0) DotTextOut(pBufferText, x, y);
        else
        {
            std::string sDefaultCharset = m_pVideo->GetDefaultCharset();
            if(sDefaultCharset.length() == 0) DotTextOut(pBufferText, x, y);
            else DotTextOut("Error", x, y);
        }

        //printf("Text: %s (%d) (%s) \n", pBufferText, iBufferSize, pCharsetName);
    }
}

void CogeImage::PrintBufferTextWrap(const char* pBufferText, int iBufferSize, int x, int y, const char* pCharsetName, CogeFont* pFont, int iAlpha)
{
    if(pBufferText == NULL || iBufferSize <= 0) return;

    SDL_Surface* pTextImg = NULL;

    int iFontSpace = 2;
    int iLineSpace = 2;
    bool bPrintOK = false;

    if(pFont == NULL)
    {
        if(m_pDefaultFont) pFont = m_pDefaultFont;
        else pFont = m_pVideo->GetDefaultFont();
    }

    if(pFont)
    {
        bPrintOK = pFont->PrintBufferTextWrap(pBufferText, iBufferSize, pCharsetName, m_iWidth, m_iPenColorSDL);
    }

    if(bPrintOK)
    {
        std::vector<SDL_Surface*> txtimgs = pFont->GetTextImages();

        for(size_t i = 0; i < txtimgs.size(); i++)
        {
            pTextImg = txtimgs[i];
            if(pTextImg)
            {
                if(iAlpha >= 0)
                OGE_SetAlpha(pTextImg, OGE_SRCALPHA, iAlpha);

                SDL_Rect rcSrc = {0};
                SDL_Rect rcDst = {0};

                rcSrc.w = pTextImg->w;
                rcSrc.h = pTextImg->h;

                if (GetValidRect(x, y, &rcSrc, &rcDst))
                SDL_BlitSurface(pTextImg, &rcSrc, m_pSurface, &rcDst);

                y = y + rcDst.h + iLineSpace;
            }
        }
    }
    else
    {
        if(pCharsetName == NULL || strlen(pCharsetName) == 0) DotTextOut(pBufferText, x, y, m_iWidth-iFontSpace, iFontSpace, iLineSpace);
    }
}

void CogeImage::PrintBufferTextAlign(const char* pBufferText, int iBufferSize, int iAlignX, int iAlignY, const char* pCharsetName, CogeFont* pFont, int iAlpha)
{
    //iAlignX: -1=>Left, 0=>Center, 1=>Right;
    //iAlignY: -1=>Top,  0=>Middle, 1=>Bottom;

    if(pBufferText == NULL || iBufferSize <= 0) return;

    SDL_Surface* pTextImg = NULL;

    if(pFont == NULL)
    {
        if(m_pDefaultFont) pFont = m_pDefaultFont;
        else pFont = m_pVideo->GetDefaultFont();
    }

    if(pFont)
    {
        pTextImg = pFont->PrintBufferText(pBufferText, iBufferSize, pCharsetName, m_iPenColorSDL);
    }

    if(pTextImg)
    {
        int x = 0; int y = 0;

        if(iAlignX > 0)  x = m_iWidth - pTextImg->w;
        if(iAlignX == 0) x = (m_iWidth - pTextImg->w)/2;

        if(iAlignY > 0)  y = m_iHeight - pTextImg->h;
        if(iAlignY == 0) y = (m_iHeight - pTextImg->h)/2;

        if(iAlpha >= 0)
        OGE_SetAlpha(pTextImg, OGE_SRCALPHA, iAlpha);

        SDL_Rect rcSrc = {0};
        SDL_Rect rcDst = {0};

        rcSrc.w = pTextImg->w;
        rcSrc.h = pTextImg->h;

        if (GetValidRect(x, y, &rcSrc, &rcDst))
        SDL_BlitSurface(pTextImg, &rcSrc, m_pSurface, &rcDst);

        //SDL_FreeSurface( pTextImg );

    }
    else if(m_pDotFont && (pCharsetName == NULL || strlen(pCharsetName) == 0))
    {
        int x = 0; int y = 0;

        int iWidth  = m_pDotFont->GetTextWidth(pBufferText, 0) + 0;
		int iHeight = _OGE_DOT_FONT_SIZE_ + 0;

        if(iAlignX > 0)  x = m_iWidth - iWidth;
        if(iAlignX == 0) x = (m_iWidth - iWidth)/2;

        if(iAlignY > 0)  y = m_iHeight - iHeight;
        if(iAlignY == 0) y = (m_iHeight - iHeight)/2;

        DotTextOut(pBufferText, x, y);
    }

}

bool CogeImage::GetBufferTextSize(const char* pBufferText, int iBufferSize, int* iWidth, int* iHeight, const char* pCharsetName, CogeFont* pFont)
{
    if(pBufferText == NULL || iBufferSize <= 0) return false;

    if(pFont == NULL)
    {
        if(m_pDefaultFont) pFont = m_pDefaultFont;
        else pFont = m_pVideo->GetDefaultFont();
    }

    if(pFont)
    {
        return pFont->GetBufferTextSize(pBufferText, iBufferSize, pCharsetName, iWidth, iHeight);
    }
    else if(pCharsetName == NULL)
    {
        *iWidth  = m_pDotFont->GetTextWidth(pBufferText, 0) + 0;
		*iHeight = _OGE_DOT_FONT_SIZE_ + 0;

		return true;
    }
    else return false;
}


void CogeImage::PrintText(const std::string& sText, int x, int y, CogeFont* pFont, int iAlpha)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    std::string sDefaultCharset = m_pVideo->GetDefaultCharset();
    if(sDefaultCharset.length() > 0)
        PrintBufferText(pBufferText, iBufferSize, x, y, sDefaultCharset.c_str(), pFont, iAlpha);
    else
        PrintBufferText(pBufferText, iBufferSize, x, y, _OGE_FONT_DF_CHARSET_, pFont, iAlpha);
}

void CogeImage::PrintTextWrap(const std::string& sText, int x, int y, CogeFont* pFont, int iAlpha)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    std::string sDefaultCharset = m_pVideo->GetDefaultCharset();
    if(sDefaultCharset.length() > 0)
        PrintBufferTextWrap(pBufferText, iBufferSize, x, y, sDefaultCharset.c_str(), pFont, iAlpha);
    else
        PrintBufferTextWrap(pBufferText, iBufferSize, x, y, _OGE_FONT_DF_CHARSET_, pFont, iAlpha);
}

void CogeImage::PrintTextAlign(const std::string& sText, int iAlignX, int iAlignY, CogeFont* pFont, int iAlpha)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    std::string sDefaultCharset = m_pVideo->GetDefaultCharset();
    if(sDefaultCharset.length() > 0)
        PrintBufferTextAlign(pBufferText, iBufferSize, iAlignX, iAlignY, sDefaultCharset.c_str(), pFont, iAlpha);
    else
        PrintBufferTextAlign(pBufferText, iBufferSize, iAlignX, iAlignY, _OGE_FONT_DF_CHARSET_, pFont, iAlpha);
}

bool CogeImage::GetTextSize(const std::string& sText, int* iWidth, int* iHeight, CogeFont* pFont)
{
    const char* pBufferText = sText.c_str();
    int iBufferSize = sText.length() + 1; // included end char '\0' ...

    if(pFont == NULL)
    {
        if(m_pDefaultFont) pFont = m_pDefaultFont;
        else pFont = m_pVideo->GetDefaultFont();
    }

    std::string sDefaultCharset = m_pVideo->GetDefaultCharset();
    if(sDefaultCharset.length() > 0)
    {
        return GetBufferTextSize(pBufferText, iBufferSize, iWidth, iHeight, sDefaultCharset.c_str(), pFont);
    }
    else
    {
        if(pFont)
            return GetBufferTextSize(pBufferText, iBufferSize, iWidth, iHeight, _OGE_FONT_DF_CHARSET_, pFont);
        else
            return GetBufferTextSize(pBufferText, iBufferSize, iWidth, iHeight, NULL, NULL);
    }

}

void CogeImage::Draw(CogeImage* pSrcImage,
                      int iDstLeft, int iDstTop,
                      int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    if(pSrcImage==NULL) return;

	SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	if(pSrcImage->m_iAlpha >= 0)
	{
	    OGE_SetAlpha(pSrcImage->m_pSurface, 0, 0);
	    pSrcImage->m_iAlpha = -1;
    }

    if (!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;


/*
    if(pSrcImage->m_bHasAlphaChannel)
    {
#if (defined __OGE_WITH_SDL2__)
        //bool bNeedLockSrc = SDL_MUSTLOCK(pSrcImage->m_pSurface) != 0;
        //bool bNeedLockDst = SDL_MUSTLOCK(m_pSurface) != 0;
        // just force the images to be locked ...
        bool bNeedLockSrc = true;
        bool bNeedLockDst = true;
#else
        bool bNeedLockSrc = false;
        bool bNeedLockDst = false;
#endif

        if (bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(pSrcImage->m_pSurface) == 0;
        if (bNeedLockDst) bNeedLockDst = SDL_LockSurface(m_pSurface) == 0;

        SDL_BlitSurface(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);

        if (bNeedLockSrc) SDL_UnlockSurface(pSrcImage->m_pSurface);
        if (bNeedLockDst) SDL_UnlockSurface(m_pSurface);
    }
    else
    {
        SDL_BlitSurface(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);
    }
*/

    SDL_BlitSurface(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);

}

void CogeImage::BltAlphaBlend( CogeImage* pSrcImage, int iAlpha,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if(iAlpha <= 0) return;

    bool bAutoAlpha = false;

    if(iAlpha < 255)
	{
	    if(pSrcImage->m_bHasAlphaChannel && pSrcImage->m_bHasLocalClipboard)
	    {
	        SDL_Rect rcSrc = {0};
            SDL_Rect rcDst = {0};

            if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
            if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

            if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;
            if (!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

            SDL_Surface* pSurface = pSrcImage->GetCurrentClipboard();
            if(pSurface == NULL) pSurface = pSrcImage->GetSurface();

            SDL_Surface* pBackup = pSrcImage->GetFreeClipboard();

            if (!pBackup)
            {
                SDL_BlitSurface(pSurface, &rcSrc, m_pSurface, &rcDst);
                return;
            }

#if (defined __OGE_WITH_SDL2__) && (defined __IMG_WITH_LOCK__)
//#if (defined __OGE_WITH_SDL2__)
            bool bNeedLockSrc = SDL_MUSTLOCK(pSurface) != 0;
            bool bNeedLockDst = SDL_MUSTLOCK(pBackup) != 0;
            // just force the images to be locked ...
            //bool bNeedLockSrc = true;
            //bool bNeedLockDst = true;
#else
            bool bNeedLockSrc = false;
            bool bNeedLockDst = false;
#endif

            if (bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(pSurface) == 0;
            if (bNeedLockDst) bNeedLockDst = SDL_LockSurface(pBackup) == 0;

            uint8_t* pSrc = (Uint8 *)pSurface->pixels;
            uint8_t* pDst = (Uint8 *)pBackup->pixels;

            int iSrcLineSize = pSurface->pitch;
            int iDstLineSize = pBackup->pitch;

            //OGE_FX_UpdateAlphaBlt(pDst, iDstLineSize, 0, 0,
            //                      pSrc, iSrcLineSize, 0, 0,
            //                      iSrcWidth, iSrcHeight,
            //                      pSurface->format->BitsPerPixel,
            //                      pSurface->format->Amask,
            //                      iAlpha - 255);

            //OGE_Log("\n BPP: %d, AMask: %d \n", pSurface->format->BitsPerPixel, pSurface->format->Amask);

            OGE_FX_UpdateAlphaBlt(pDst, iDstLineSize, 0, 0,
                                  pSrc, iSrcLineSize, 0, 0,
                                  iSrcWidth, iSrcHeight,
                                  pSurface->format->BitsPerPixel,
                                  pSurface->format->Amask,
                                  iAlpha);

            if (bNeedLockSrc) SDL_UnlockSurface(pSurface);
            if (bNeedLockDst) SDL_UnlockSurface(pBackup);

            pSrcImage->m_iEffectCount = pSrcImage->m_iEffectCount - 1;

            if(pSrcImage->m_iEffectCount < 0) pSrcImage->m_iEffectCount = 0;

            if(pSrcImage->m_iEffectCount == 0)
            {
                pSrcImage->SetCurrentClipboard(NULL);
                SDL_BlitSurface(pBackup, &rcSrc, m_pSurface, &rcDst);
            }
            else pSrcImage->SetCurrentClipboard(pBackup);

            return ;

	    }
	    else
	    {

//#if (!defined __ANDROID__)

	        if(pSrcImage->m_iAlpha != iAlpha)
            {
                OGE_SetAlpha(pSrcImage->m_pSurface, OGE_SRCALPHA, iAlpha);
                pSrcImage->m_iAlpha = iAlpha;
            }

            bAutoAlpha = true;

//#endif

	    }
    }
    else
    {
        Draw(pSrcImage, iDstLeft, iDstTop, iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight);
        return;
    }

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	//rcSrc.x = iSrcLeft;
	//rcSrc.y = iSrcTop;
	//rcSrc.w = iSrcWidth;
	//rcSrc.h = iSrcHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;
    if (!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;


    if(bAutoAlpha)
    {
        SDL_BlitSurface(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);
    }
    else
    {
        uint8_t* pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
        uint8_t* pDst = (Uint8 *)m_pSurface->pixels;

        int iSrcLineSize = pSrcImage->m_pSurface->pitch;
        int iDstLineSize = m_pSurface->pitch;

        OGE_FX_AlphaBlend(pDst, iDstLineSize,
                            rcDst.x, rcDst.y,
                            pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                            rcSrc.x, rcSrc.y,
                            rcSrc.w, rcSrc.h, m_iBPP, iAlpha);
    }

}

void CogeImage::DrawLine(int iStartX, int iStartY, int iEndX, int iEndY)
{
	uint8_t* pDst;
	int iLineSize = 0;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_Line(pDst, iLineSize, m_iBPP, m_iPenColor, m_iWidth-1, m_iHeight-1, iStartX, iStartY, iEndX, iEndY);

	EndUpdate();
}
void CogeImage::DrawCircle(int iCenterX, int iCenterY, int iRadius)
{
    uint8_t* pDst;

	int iLineSize = 0;

	//if (SDL_LockSurface(m_pSurface) < 0) return;

	//try
	//{

	//if (m_pddSurface->Lock(&rcDst, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_Circle(pDst, iLineSize, m_iBPP, m_iPenColor, m_iWidth-1, m_iHeight-1, iCenterX, iCenterY, iRadius);

	//OGE_FX_LightnessRGB(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP, -iAmount, 0, 0 );

	//}
	//catch ( ... )
	//{
	//    SDL_UnlockSurface( m_pSurface );
    //}

	//m_pddSurface->Unlock(&rcDst);

	//SDL_UnlockSurface( m_pSurface );

	EndUpdate();
}

void CogeImage::CopyRect(CogeImage* pSrcImage,
                  int iDstLeft, int iDstTop,
                  int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_CopyRect(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
	this->EndUpdate();

}


void CogeImage::Blt( CogeImage* pSrcImage,
              int iDstLeft, int iDstTop, int iSrcColorKey,
              int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_Blt(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize, iSrcColorKey,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltChangedRGB( CogeImage* pSrcImage, int iAmount,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{

	if ( m_pVideo->m_iState < 0 ) return;

	int iBlueAmount  = iAmount & 0x000000ff;
	int iGreenAmount = (iAmount & 0x0000ff00) >> 8;
	int iRedAmount   = (iAmount & 0x00ff0000) >> 16;
	int iAlphaAmount = (iAmount & 0xff000000) >> 24;

	if((iAlphaAmount & 1) > 0) iBlueAmount = 0 - iBlueAmount;
	if((iAlphaAmount & 2) > 0) iGreenAmount = 0 - iGreenAmount;
	if((iAlphaAmount & 4) > 0) iRedAmount = 0 - iRedAmount;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltChangedRGB(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP,
                        iRedAmount, iGreenAmount, iBlueAmount);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltWithColor( CogeImage* pSrcImage, int iColor, int iAlpha,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	iColor = m_pVideo->FormatColor(iColor);

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltWithColor(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP, iColor, iAlpha);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltWithEdge( CogeImage* pSrcImage, int iEdgeColor,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	iEdgeColor = m_pVideo->FormatColor(iEdgeColor);

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltWithEdge(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP, iEdgeColor);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltWithLightness( CogeImage* pSrcImage, int iAmount,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltLightness(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP, iAmount);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::LightMaskBlend( CogeImage* pSrcImage, int iDstLeft, int iDstTop,
                        int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_LightMaskBlend(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize,
                        rcSrc.x, rcSrc.y,
                        rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
	this->EndUpdate();

}

void CogeImage::BltMask( CogeImage* pSrcImage, CogeImage* pMaskImage, int iDstLeft, int iDstTop,
                  int iSrcLeft, int iSrcTop, int iMaskLeft, int iMaskTop,
                  int iSrcWidth, int iSrcHeight )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;

	if(iMaskLeft + rcSrc.w > pMaskImage->m_iWidth ||
	   iMaskTop + rcSrc.h > pMaskImage->m_iHeight) return;

	uint8_t* pSrc;
	uint8_t* pDst;
	uint8_t* pMask;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;
	int iMaskLineSize = 0;

	if(!GetValidRect(iDstLeft, iDstTop, &rcSrc, &rcDst)) return;

	pSrcImage->BeginUpdate();
	pMaskImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	pMask = (Uint8 *)pMaskImage->m_pSurface->pixels;
	iMaskLineSize =  pMaskImage->m_pSurface->pitch;

	OGE_FX_BltMask(pDst, iDstLineSize,
                        rcDst.x, rcDst.y,
                        pSrc, iSrcLineSize,
                        rcSrc.x, rcSrc.y,
                        pMask, iMaskLineSize,
                        iMaskLeft, iMaskTop,
                        rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
    pMaskImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltStretchFast(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom)
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcRight-iSrcLeft, iSrcBottom-iSrcTop, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iDstRight-iDstLeft, iDstBottom-iDstTop, &rcDst)) return;

	SDL_SoftStretch(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);
}

void CogeImage::BltStretch(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom)
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcRight-iSrcLeft, iSrcBottom-iSrcTop, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iDstRight-iDstLeft, iDstBottom-iDstTop, &rcDst)) return;

	//SDL_SoftStretch(pSrcImage->m_pSurface, &rcSrc, m_pSurface, &rcDst);

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(pSrcImage->m_bHasAlphaChannel && pSrcImage->m_bHasLocalClipboard)
    {
        //SDL_Surface* pSurface = pSrcImage->GetSurface();
        //SDL_Surface* pBackup = pSrcImage->GetBackupSurface();

        SDL_Surface* pSurface = pSrcImage->GetCurrentClipboard();
        if(pSurface == NULL) pSurface = pSrcImage->GetSurface();

        SDL_Surface* pBackup = pSrcImage->GetFreeClipboard();

        if (pSurface && pBackup)
        {

#if (defined __OGE_WITH_SDL2__) && (defined __IMG_WITH_LOCK__)
//#if (defined __OGE_WITH_SDL2__)
            bool bNeedLockSrc = SDL_MUSTLOCK(pSurface) != 0;
            bool bNeedLockDst = SDL_MUSTLOCK(pBackup) != 0;
            // just force the images to be locked ...
            //bool bNeedLockSrc = true;
            //bool bNeedLockDst = true;
#else
            bool bNeedLockSrc = false;
            bool bNeedLockDst = false;
#endif

            bool bStretched = false;

            if (bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(pSurface) == 0;
            if (bNeedLockDst) bNeedLockDst = SDL_LockSurface(pBackup) == 0;

            pSrc = (Uint8 *)pSurface->pixels;
            pDst = (Uint8 *)pBackup->pixels;

            iSrcLineSize = pSurface->pitch;
            iDstLineSize = pBackup->pitch;

            if(rcDst.w <= pSrcImage->m_iWidth && rcDst.h <= pSrcImage->m_iHeight)
            {
                bStretched = true;
                OGE_FX_BltStretch(pDst, iDstLineSize,
                        0, 0, rcDst.w, rcDst.h,
                        pSrc, iSrcLineSize, -1,
                        rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h, pSurface->format->BitsPerPixel);
            }
            else
            {
                SDL_BlitSurface(pSurface, &rcSrc, pBackup, &rcSrc);
            }

            if (bNeedLockSrc) SDL_UnlockSurface(pSurface);
            if (bNeedLockDst) SDL_UnlockSurface(pBackup);

            pSrcImage->m_iEffectCount = pSrcImage->m_iEffectCount - 1;

            if(pSrcImage->m_iEffectCount < 0) pSrcImage->m_iEffectCount = 0;

            if(pSrcImage->m_iEffectCount == 0)
            {
                pSrcImage->SetCurrentClipboard(NULL);
                if(bStretched)
                {
                    rcSrc.x = 0; rcSrc.y = 0;
                    rcSrc.w = rcDst.w; rcSrc.h = rcDst.h;
                }
                SDL_BlitSurface(pBackup, &rcSrc, m_pSurface, &rcDst);
            }
            else pSrcImage->SetCurrentClipboard(pBackup);

            return ;

        }

    }

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltStretch(pDst, iDstLineSize,
                        rcDst.x, rcDst.y, rcDst.w, rcDst.h,
                        pSrc, iSrcLineSize, pSrcImage->m_iColorKey,
                        rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltStretchSmoothly(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom)
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcRight-iSrcLeft, iSrcBottom-iSrcTop, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iDstRight-iDstLeft, iDstBottom-iDstTop, &rcDst)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_StretchSmoothly(pDst, iDstLineSize,
                        rcDst.x, rcDst.y, rcDst.w, rcDst.h,
                        pSrc, iSrcLineSize,
                        rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h, m_iBPP);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltRotate(CogeImage* pSrcImage, double fAngle,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iSrcWidth, iSrcHeight, &rcDst)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	if(pSrcImage->m_bHasAlphaChannel && pSrcImage->m_bHasLocalClipboard)
    {
        //SDL_Surface* pSurface = pSrcImage->GetSurface();
        //SDL_Surface* pBackup = pSrcImage->GetBackupSurface();

        SDL_Surface* pSurface = pSrcImage->GetCurrentClipboard();
        if(pSurface == NULL) pSurface = pSrcImage->GetSurface();

        SDL_Surface* pBackup = pSrcImage->GetFreeClipboard();

        if (pSurface && pBackup)
        {

#if (defined __OGE_WITH_SDL2__) && (defined __IMG_WITH_LOCK__)
//#if (defined __OGE_WITH_SDL2__)
            bool bNeedLockSrc = SDL_MUSTLOCK(pSurface) != 0;
            bool bNeedLockDst = SDL_MUSTLOCK(pBackup) != 0;
            // just force the images to be locked ...
            //bool bNeedLockSrc = true;
            //bool bNeedLockDst = true;
#else
            bool bNeedLockSrc = false;
            bool bNeedLockDst = false;
#endif

            if (bNeedLockSrc) bNeedLockSrc = SDL_LockSurface(pSurface) == 0;
            if (bNeedLockDst) bNeedLockDst = SDL_LockSurface(pBackup) == 0;

            pSrc = (Uint8 *)pSurface->pixels;
            pDst = (Uint8 *)pBackup->pixels;

            iSrcLineSize = pSurface->pitch;
            iDstLineSize = pBackup->pitch;

            OGE_FX_BltRotate(pDst, iDstLineSize, 0, 0, pSrcImage->m_iWidth, pSrcImage->m_iHeight,
                  pSrc, iSrcLineSize, 0, 0, pSrcImage->m_iWidth, pSrcImage->m_iHeight,
                  pSurface->format->BitsPerPixel, -1, fAngle);

            if (bNeedLockSrc) SDL_UnlockSurface(pSurface);
            if (bNeedLockDst) SDL_UnlockSurface(pBackup);

            pSrcImage->m_iEffectCount = pSrcImage->m_iEffectCount - 1;

            if(pSrcImage->m_iEffectCount < 0) pSrcImage->m_iEffectCount = 0;

            if(pSrcImage->m_iEffectCount == 0)
            {
                pSrcImage->SetCurrentClipboard(NULL);
                SDL_BlitSurface(pBackup, &rcSrc, m_pSurface, &rcDst);
            }
            else pSrcImage->SetCurrentClipboard(pBackup);

            return ;

        }

    }

    pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltRotate(pDst, iDstLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h,
                  pSrc, iSrcLineSize, rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h,
                  m_iBPP, pSrcImage->m_iColorKey, fAngle);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltRotozoom(CogeImage* pSrcImage, double fAngle, int iZoom,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight )
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	//rcSrc.x = iSrcLeft;
	//rcSrc.y = iSrcTop;
	//rcSrc.w = iSrcWidth;
	//rcSrc.h = iSrcHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iSrcWidth, iSrcHeight, &rcDst)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	//if(!GetValidRect(iDstLeft, iDstTop, rcSrc, rcDst)) return;

	//int iColorKey = pSrcImage->m_iColorKeyRGB

	//if (SDL_LockSurface(m_pSurface) < 0) return;
	//if (SDL_LockSurface(pSrcImage->m_pSurface) < 0)
	//{
	//    SDL_UnlockSurface( m_pSurface );
    //    return;
	//}

	//try
	//{

	//if (m_pddSurface->Lock(&rcDst, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltRotate(pDst, iDstLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h,
                  pSrc, iSrcLineSize, rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h,
                  m_iBPP, pSrcImage->m_iColorKey, fAngle, iZoom);


	//}
	//catch ( ... )
	//{
	//    SDL_UnlockSurface( pSrcImage->m_pSurface );
	//    SDL_UnlockSurface( m_pSurface );
    //}

	//m_pddSurface->Unlock(&rcDst);

    //SDL_UnlockSurface( pSrcImage->m_pSurface );
	//SDL_UnlockSurface( m_pSurface );

	pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltWave(CogeImage* pSrcImage, double iXAmount, double iYAmount, double iZAmount,
                int iXSteps, int iYSteps,
                int iDstLeft, int iDstTop,
                int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    SDL_Rect rcSrc = {0};
	SDL_Rect rcDst = {0};

	if (iSrcWidth == -1) iSrcWidth = pSrcImage->m_iWidth;
	if (iSrcHeight == -1) iSrcHeight = pSrcImage->m_iHeight;

	if(!pSrcImage->GetValidRect(iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight, &rcSrc)) return;
	if(!GetValidRect(iDstLeft, iDstTop, iSrcWidth, iSrcHeight, &rcDst)) return;

	uint8_t* pSrc;
	uint8_t* pDst;

	int iSrcLineSize = 0;
	int iDstLineSize = 0;

	pSrcImage->BeginUpdate();
	this->BeginUpdate();

	pSrc = (Uint8 *)pSrcImage->m_pSurface->pixels;
	iSrcLineSize =  pSrcImage->m_pSurface->pitch;

	pDst = (Uint8 *)m_pSurface->pixels;
	iDstLineSize = m_pSurface->pitch;

	OGE_FX_BltRoundWave(pDst, iDstLineSize, rcDst.x, rcDst.y,
                  pSrc, iSrcLineSize, rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h,
                  m_iBPP, pSrcImage->m_iColorKey, iXAmount, iYAmount, iZAmount, iXSteps, iYSteps);

    pSrcImage->EndUpdate();
	this->EndUpdate();
}

void CogeImage::BltWave(CogeImage* pSrcImage, int iAmount,
                int iDstLeft, int iDstTop,
                int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    BltWave(pSrcImage, 10, 70, 5, 0, iAmount, iDstLeft, iDstTop,
            iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight);
}


void CogeImage::Grayscale(int iDstX, int iDstY, int iWidth, int iHeight)
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_Grayscale(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP);

	EndUpdate();
}

void CogeImage::SubLight(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount)
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_SubLight(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP, iAmount );

	EndUpdate();

}

void CogeImage::Lightness(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount)
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_Lightness(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP, iAmount );

	EndUpdate();
}

void CogeImage::Blur(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount)
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_GaussianBlur(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP, iAmount );

	EndUpdate();
}

void CogeImage::Grayscale(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount)
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_ChangeGrayLevel(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP, iAmount );

	EndUpdate();
}

void CogeImage::ChangeColorRGB(int iDstX, int iDstY, int iWidth, int iHeight,
                     int iRedAmount, int iGreenAmount, int iBlueAmount )
{
    if ( m_pVideo->m_iState < 0 ) return;

    SDL_Rect rcDst = {0};

	uint8_t* pDst;

	int iLineSize = 0;

	if(!GetValidRect(iDstX, iDstY, iWidth, iHeight, &rcDst)) return;

	BeginUpdate();

	pDst = (Uint8 *)m_pSurface->pixels;
	iLineSize = m_pSurface->pitch;

	OGE_FX_ChangeColorRGB(pDst, iLineSize, rcDst.x, rcDst.y, rcDst.w, rcDst.h, m_iBPP,
	                    iRedAmount, iGreenAmount, iBlueAmount );

    EndUpdate();
}

/*
void CogeImage::UpdateRect(int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight)
{
    if(m_pSurface)
    {
        if(iSrcWidth < 0) iSrcWidth = m_iWidth;
        if(iSrcHeight < 0) iSrcHeight = m_iHeight;

#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_Rect rect;
        rect.x = iSrcLeft;
        rect.y = iSrcTop;
        rect.w = iSrcWidth;
        rect.h = iSrcHeight;
        SDL_UpdateRects(m_pSurface, 1, &rect);

#else
        SDL_UpdateRect(m_pSurface, iSrcLeft, iSrcTop, iSrcWidth, iSrcHeight);
#endif
    }
}
*/


void CogeImage::FillRect(int iRGBColor, int iLeft, int iTop, int iWidth, int iHeight)
{
	SDL_Rect rcDst = {0};

	int iColor = m_pVideo->FormatColor(iRGBColor);

	//int iR = (iRGBColor&0x00ff0000)>>16;
    //int iG = (iRGBColor&0x0000ff00)>>8;
    //int iB = iRGBColor&0x000000ff;
    //int iColor = SDL_MapRGB(m_pSurface->format, iR, iG, iB);

	if (iWidth <= 0 && iHeight <= 0)
        SDL_FillRect(m_pSurface, 0, iColor);
    else
    {
        if (GetValidRect(iLeft, iTop, iWidth, iHeight, &rcDst))
        SDL_FillRect(m_pSurface, &rcDst, iColor);
    }
}

bool CogeImage::LoadBMP(const std::string& sFileName, bool bLoadAlphaChannel, bool bCreateLocalClipboard)
{
    //return false;
    //SDL_Surface* bmp = m_pVideo->OGE_LoadBMP(sFileName.c_str());
    SDL_Surface* bmp = SDL_LoadBMP(sFileName.c_str());

    if (!bmp) return false;

    if (m_pSurface)
    {
        SDL_FreeSurface(m_pSurface);
        m_pSurface = NULL;
    }

    if(bLoadAlphaChannel) m_pSurface = OGE_DisplayFormatAlpha( bmp );
    else m_pSurface = OGE_DisplayFormat( bmp );

    m_bHasAlphaChannel = bLoadAlphaChannel;
    m_bHasLocalClipboard = m_bHasAlphaChannel && bCreateLocalClipboard;

    if(m_pSurface && m_bHasAlphaChannel && m_bHasLocalClipboard)
    {
        if (m_pLocalClipboardA)
        {
            SDL_FreeSurface(m_pLocalClipboardA);
            m_pLocalClipboardA = NULL;
        }

        m_pLocalClipboardA = OGE_DisplayFormatAlpha( bmp );

        if (m_pLocalClipboardB)
        {
            SDL_FreeSurface(m_pLocalClipboardB);
            m_pLocalClipboardB = NULL;
        }

        m_pLocalClipboardB = OGE_DisplayFormatAlpha( bmp );

    }

    SDL_FreeSurface( bmp );

    if (!m_pSurface) return false;
    else return true;

}

bool CogeImage::LoadImg(const std::string& sFileName, bool bLoadAlphaChannel, bool bCreateLocalClipboard)
{
    //return false;

    //SDL_RWops *src = SDL_RWFromFile(sFileName.c_str(), "rb");
    //bool bIsPng = IMG_isPNG(src);
    //SDL_Surface* img = IMG_Load_RW(src, 1);

    //int iPos = sFileName.find_last_of('.');
    //if(iPos < 0) return false;
    //std::string sExt = sFileName.substr(iPos);
    //bool bIsPng = sExt == ".png";

//#ifdef __OGE_WITH_SDL2__
//    if(sFileName.find_last_of(".bmp") > 0)
//    {
//        return LoadBMP(sFileName, bLoadAlphaChannel);
//    }
//#endif

    //SDL_Surface* img = m_pVideo->OGE_LoadIMG(sFileName.c_str());
    SDL_Surface* img = IMG_Load(sFileName.c_str());

    if(img)
    {
        if (m_pSurface)
        {
            SDL_FreeSurface(m_pSurface);
            m_pSurface = NULL;
        }

        if(bLoadAlphaChannel) m_pSurface = OGE_DisplayFormatAlpha( img );
        else m_pSurface = OGE_DisplayFormat( img );

        m_bHasAlphaChannel = bLoadAlphaChannel;
        m_bHasLocalClipboard = m_bHasAlphaChannel && bCreateLocalClipboard;

        if(m_pSurface && m_bHasAlphaChannel && m_bHasLocalClipboard)
        {
            //uint8_t* pDst = (Uint8 *)m_pSurface->pixels;
            //int iDstLineSize = m_pSurface->pitch;
            //OGE_FX_UpdateAlpha(pDst, iDstLineSize, 0, 0, m_pSurface->w, m_pSurface->h, 32, -192);

            if (m_pLocalClipboardA)
            {
                SDL_FreeSurface(m_pLocalClipboardA);
                m_pLocalClipboardA = NULL;
            }

            m_pLocalClipboardA = OGE_DisplayFormatAlpha( img );

            if (m_pLocalClipboardB)
            {
                SDL_FreeSurface(m_pLocalClipboardB);
                m_pLocalClipboardB = NULL;
            }

            m_pLocalClipboardB = OGE_DisplayFormatAlpha( img );

        }

        SDL_FreeSurface(img);

        if (m_pSurface) return true;
        else
        {
            OGE_Log("Could not convert image bbp format %s: %s\n", sFileName.c_str(), SDL_GetError());
            return false;
        }
    }
    else
    {
        OGE_Log("Can not load image %s from file: %s, reason: %s\n", m_sName.c_str(), sFileName.c_str(), SDL_GetError());
        return false;
    }

}

bool CogeImage::LoadImgFromBuffer(char* pBuffer, int iBufferSize, bool bLoadAlphaChannel, bool bCreateLocalClipboard)
{
    SDL_Surface* img = IMG_Load_RW(SDL_RWFromMem((void*)pBuffer, iBufferSize), 0);

    if(img)
    {
        if (m_pSurface)
        {
            SDL_FreeSurface(m_pSurface);
            m_pSurface = NULL;
        }

        if(bLoadAlphaChannel) m_pSurface = OGE_DisplayFormatAlpha( img );
        else m_pSurface = OGE_DisplayFormat( img );

        m_bHasAlphaChannel = bLoadAlphaChannel;
        m_bHasLocalClipboard = m_bHasAlphaChannel && bCreateLocalClipboard;

        if(m_pSurface && m_bHasAlphaChannel && m_bHasLocalClipboard)
        {
            if (m_pLocalClipboardA)
            {
                SDL_FreeSurface(m_pLocalClipboardA);
                m_pLocalClipboardA = NULL;
            }

            m_pLocalClipboardA = OGE_DisplayFormatAlpha( img );

            if (m_pLocalClipboardB)
            {
                SDL_FreeSurface(m_pLocalClipboardB);
                m_pLocalClipboardB = NULL;
            }

            m_pLocalClipboardB = OGE_DisplayFormatAlpha( img );
        }

        SDL_FreeSurface(img);

        if (m_pSurface) return true;
        else
        {
            OGE_Log("Could not convert image bbp format: %s\n", SDL_GetError());
            return false;
        }
    }
    else
    {
        OGE_Log("Couldn't load image from buffer: %s\n", SDL_GetError());
        return false;
    }

}

bool CogeImage::LoadData(const std::string& sFileName, bool bLoadAlphaChannel, bool bCreateLocalClipboard)
{
    return LoadImg(sFileName, bLoadAlphaChannel, bCreateLocalClipboard);
}

bool CogeImage::LoadDataFromBuffer(char* pBuffer, int iBufferSize, bool bLoadAlphaChannel, bool bCreateLocalClipboard)
{
    return LoadImgFromBuffer(pBuffer, iBufferSize, bLoadAlphaChannel, bCreateLocalClipboard);
}
















