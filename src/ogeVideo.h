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

#ifndef __OGE_VIDEO_H_INCLUDED__
#define __OGE_VIDEO_H_INCLUDED__

#include <string>
#include <vector>
#include <list>
#include <map>

#include "SDL.h"
#include "ogeCommon.h"
#include "ogeFont.h"

#define _OGE_DOT_FONT_SIZE_          16

#define _OGE_EN_FONT_FILE_           "asc16.font"
#define _OGE_CN_FONT_FILE_           "hzk16.font"

#define _OGE_FONT_DF_CHARSET_        "UTF-8"

// pure 2d rendering will always use software surfaces ...

#ifndef __OGE_WITH_SDL2__
#define _OGE_VIDEO_DF_MODE_          SDL_SWSURFACE
#define _OGE_VIDEO_MAIN_MODE_        SDL_HWSURFACE|SDL_DOUBLEBUF
#else
#define _OGE_VIDEO_DF_MODE_          0
#define _OGE_VIDEO_MAIN_MODE_        0
#endif

#ifdef __OGE_WITH_GLWIN__
#ifndef __OGE_WITH_SDL2__
#undef __OGE_WITH_GLWIN__
#endif
#endif


/*================== Classes ===============================*/

class CogeImage;
class CogeDotFont;

typedef std::map<std::string, CogeImage*> ogeImageMap;


/*---------------- Video -----------------*/

class CogeVideo
{
private:

    SDL_Surface*       m_pFrontBuffer;

#ifdef __OGE_WITH_GLWIN__
    SDL_Window*        m_pMainWindow;
    SDL_Texture*       m_pMainTexture;
    SDL_Renderer*      m_pMainRenderer;
#else
    void*              m_pMainWindow;
    void*              m_pMainTexture;
    void*              m_pMainRenderer;
#endif


#ifdef __OGE_WITH_SDL2__
    SDL_DisplayMode    m_DisplayMode;
#endif

    //SDL_Surface*       m_pBackBuffer;

    SDL_Surface*       m_pWindowIcon;

    CogeImage*         m_pDefaultScreen;

    CogeImage*         m_pDefaultBg;

    CogeImage*         m_pMainScreen;

    CogeImage*         m_pLastScreen;

    CogeImage*         m_pTextArea;

    CogeImage*         m_pClipboardA;
    CogeImage*         m_pClipboardB;
    CogeImage*         m_pClipboardC;

    CogeDotFont*       m_pDotFont;    // default font for video

    CogeFontStock*     m_pFontStock;

    CogeFont*          m_pDefaultFont;

    ogeImageMap        m_ImageMap;
    //ogeAnimaMap        m_AnimaMap;

    SDL_Rect           m_ViewRect;

    std::string        m_sDefaultResPath;

    // basic data
    int m_iWidth;
    int m_iHeight;
    int m_iBPP;      // bytes per pixel

    int m_iMode;

    int m_iState;

    bool m_bSupportMMX;

    int m_iRealWidth;
    int m_iRealHeight;
    int m_iRealBPP;
    bool m_bNeedStretch;

    bool m_bIsBGRA;

    //int m_iViewX;
    //int m_iViewY;

    int m_iLastFrameTime;
    int m_iFrameRate;
    int m_iOldFrameRate;
    int m_iFrameRateWanted;
    int m_iFrameCount;
    int m_iGlobalTime;

    bool m_bShowFPS;
    //bool m_bLockFPS;

    int m_iLockedFPS;

    int  m_iFrameInterval;


    void DelAllImages();

    void SetView(int x, int y);

    int CheckSystemScreenSize(int iRequiredWidth, int iRequiredHeight, int iRequiredBPP);


protected:

public:

    // sdl compat - begin -

    //SDL_Surface* OGE_DisplayFormat(SDL_Surface* pSurface);
    //SDL_Surface* OGE_DisplayFormatAlpha(SDL_Surface* pSurface);
    void OGE_WarpMouse(unsigned short x, unsigned short y);
    //SDL_Surface* OGE_LoadBMP(const char* filepath);
    //SDL_Surface* OGE_LoadIMG(const char* filepath);

    // sdl compat - end -


    void* GetRawSurface();

    void* GetMainWindow();

//#ifdef __OGE_WITH_SDL2__
    int Initialize(int iWidth, int iHeight, int iBPP, bool bFullscreen, int iFlags = _OGE_VIDEO_DF_MODE_);
//#else
//    int Initialize(int iWidth, int iHeight, int iBPP, bool bFullscreen, int iFlags = _OGE_VIDEO_MAIN_MODE_);
//#endif

    void Finalize();

    int Resume();
    int Pause();

    int Update(int x, int y);
    void UpdateRenderer(void* pSurface, int x = 0, int y = 0, unsigned int w = 0, unsigned int h = 0);

    int GetWidth();
    int GetHeight();
    int GetBPP();
    int GetWindowID();
    int GetMode();

    int GetState();

    int GetScreenUpdateInterval();

    bool GetRealPos(double in_x, double in_y, int* out_x, int* out_y);

    void SetDefaultResPath(const std::string& sPath);

    int FormatColor(int iRGBColor);

    int StringToColor(const std::string& sValue);
    int StringToColor(const std::string& sValue, int* iRed, int* iGreen, int* iBlue);

    int StringToUnicode(const std::string& sText, const std::string& sCharsetName, char* pUnicodeBuffer);
    std::string UnicodeToString(char* pUnicodeBuffer, int iBufferSize, const std::string& sCharsetName);

    void SetWindowCaption(const std::string& sCaption);
    void SetWindowIcon(const std::string& sIconFilePath, const std::string& sIconMask);

    CogeImage* GetScreen();
    void SetScreen(CogeImage* pTheScreen);
    void DefaultScreen();
    CogeImage* GetDefaultScreen();

    CogeImage* GetLastScreen();

    CogeImage* GetClipboardA();
    CogeImage* GetClipboardB();
    CogeImage* GetClipboardC();

    CogeImage* GetDefaultBg();
    void ClearDefaultBg(int iRGBColor = -1);

    int GetFPS();

    void DrawFPS(int x=0, int y=0);

    int GetLockedFPS();
    void LockFPS(int iFPS);
    void UnlockFPS();
    bool IsFPSChanged();

    bool IsBGRAMode();

    bool GetFullScreen();
    void SetFullScreen(bool bValue);


    // main screen func

    void FillRect(int iRGBColor, int iLeft=0, int iTop=0, int iWidth=0, int iHeight=0);

    void DrawImage(const std::string& sSrcImageName,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1);

    void DrawImage(CogeImage* pSrcImage,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1);

    void DotTextOut(char* pText, int x, int y, int iMaxWidth = 0,
                    int iFontSpace = 0, int iLineSpace = 0);

    void PrintText(const std::string& sText, int x, int y, CogeFont* pFont = NULL, int iAlpha = -1);



    // Image management

    CogeImage* NewImage(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB = -1,
                        bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false,
                        const std::string& sFileName  = "");

    CogeImage* NewImageFromBuffer(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB = -1,
                        bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false,
                        char* pBuffer = NULL, int iBufferSize = 0);

    CogeImage* FindImage(const std::string& sName);

    CogeImage* GetImage(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB = -1,
                        bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false,
                        const std::string& sFileName  = "");

    CogeImage* GetImageFromBuffer(const std::string& sName,
                        int iWidth, int iHeight, int iColorKeyRGB = -1,
                        bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false,
                        char* pBuffer = NULL, int iBufferSize = 0);

    bool DelImage(const std::string& sName);
    bool DelImage(CogeImage* pImage);


    // Font ...
    //bool InstallFont(const std::string& sFontName, int iFontSize, const std::string& sFontFileName);
    //void UninstallFont(const std::string& sFontName);
    //bool UseFont(const std::string& sFontName);
    CogeFont* NewFont(const std::string& sFontName, int iFontSize, const std::string& sFileName);
    CogeFont* NewFontFromBuffer(const std::string& sFontName, int iFontSize, char* pBuffer, int iBufferSize);
    CogeFont* FindFont(const std::string& sFontName, int iFontSize);
    CogeFont* GetFont(const std::string& sFontName, int iFontSize, const std::string& sFileName);
    int DelFont(const std::string& sFontName, int iFontSize);

    CogeFont* GetDefaultFont();
    void SetDefaultFont(CogeFont* pFont);

    const std::string& GetDefaultCharset();
    void SetDefaultCharset(const std::string& sDefaultCharset);

    const std::string& GetSystemCharset();
    void SetSystemCharset(const std::string& sSystemCharset);


    CogeVideo();
    ~CogeVideo();

    //friend class CogeDotFont;
    friend class CogeImage;

};

/*---------------- DotFont -----------------*/

class CogeDotFont
{
private:

    //CogeVideo* m_pVideo;

    //char* asc12;   // 8  * 12, one byte one char
    char* asc16;     // 8  * 16, one byte one char
    //char* hzk12;   // 12 * 12, two byte one char
    char* hzk16;     // 16 * 16, one byte one char

protected:

public:

    bool InstallFont(const char* sFontFileName, int iType = 0);
    void UninstallFont(int iType = 0);

    void OneLine(const char* pText, char* pDstBuffer, int iLineSize, int iBPP, int iPenColor,
                    int x, int y, int iWidth, int iFontSpace = 0);

    void Lines(const char* pText, char* pDstBuffer, int iLineSize, int iBPP, int iPenColor,
               int x, int y, int iWidth, int iHeight,
               int iFontSpace = 0, int iLineSpace = 0);

    int GetTextWidth(const char* pText, int iFontSpace);
    int GetTextHeight(const char* pText, int iMaxWidth, int iFontSpace, int iLineSpace);

    //constructor
    CogeDotFont();
    //destructor
    ~CogeDotFont();

};


/*---------------- Image -----------------*/

// Image Interface
class CogeImage
{
private:

    SDL_Surface*       m_pSurface;
    SDL_Surface*       m_pLocalClipboardA;
    SDL_Surface*       m_pLocalClipboardB;
    SDL_Surface*       m_pCurrentClipboard;

    CogeVideo*         m_pVideo;

    CogeDotFont*       m_pDotFont;

    CogeFont*          m_pDefaultFont;

    //CogeFontStock*     m_pFontStock;

    // basic data
    int m_iWidth;
    int m_iHeight;

    int m_iBPP;

    int m_iColorKeyRGB;
    int m_iColorKey;

    int m_iAlpha;

    bool m_bHasAlphaChannel;
    bool m_bHasLocalClipboard;

    int m_iEffectCount;

    int m_iPenColor;

    int m_iPenColorRGB;

    int m_iLockTimes;

    SDL_Color m_iPenColorSDL;

    int m_iTotalUsers;
    //int  m_iBgColor;

    std::string m_sName;

    //std::string m_sDataName;

    SDL_Surface* GetSurface();
    SDL_Surface* GetFreeClipboard();
    SDL_Surface* GetCurrentClipboard();
    void SetCurrentClipboard(SDL_Surface* pClipboard);

    //void GetValidRect(SDL_Rect& rc);
    //void GetValidRect(RECT& SrcRc, RECT& DstRc);

    bool GetValidRect(int x, int y, int w, int h, SDL_Rect* rslrc);
    bool GetValidRect(int x, int y, SDL_Rect* rcRslSrc, SDL_Rect* rcRslDst);
    void GetValidRect(CogeRect* rslrc);

    bool LoadBMP(const std::string& sFileName, bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false);

    bool LoadImg(const std::string& sFileName, bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false);

    bool LoadImgFromBuffer(char* pBuffer, int iBufferSize, bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false);



protected:



public:

    // basic getter & setter

    const std::string& GetName();

    CogeVideo* GetVideoEngine();

    int SaveAsBMP(const std::string& sFileName);


    int GetWidth();
    int GetHeight();
    int GetBPP();

    bool HasAlphaChannel();
    bool HasLocalClipboard();

    //int GetTotalUsers();

    int GetPenColor();
    void SetPenColor(int iRGBColor);

    void SetColorKey(int iColorKeyRGB);
    void SetAlpha(int iAlpha);

    int GetColorKey();
    int GetRawColorKey();

    void SetDefaultFont(CogeFont* pFont);
    CogeFont* GetDefaultFont();

    void BeginUpdate();
    void EndUpdate();

    void Hire();
    void Fire();

    bool LoadData(const std::string& sFileName, bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false);

    bool LoadDataFromBuffer(char* pBuffer, int iBufferSize, bool bLoadAlphaChannel = false, bool bCreateLocalClipboard = false);

    //bool Refresh(bool bRebuildAll = false);

    void PrepareLocalClipboards(int iEffectCount);

    void FillRect(int iRGBColor, int iLeft=0, int iTop=0, int iWidth=0, int iHeight=0);


    void Draw(CogeImage* pSrcImage,
              int iDstLeft, int iDstTop,
              int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1);

    //void UpdateRect(int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1);

    CogeRect GetDotTextRect(const char* pText, int x, int y, int iMaxWidth = 0,
			                int iFontSpace = 0, int iLineSpace = 0);

    void DotTextOut(const char* pText, int x, int y, int iMaxWidth = 0,
                    int iFontSpace = 0, int iLineSpace = 0);

    void PrintBufferText(const char* pBufferText, int iBufferSize, int x, int y, const char* pCharsetName, CogeFont* pFont = NULL, int iAlpha = -1);
    void PrintBufferTextWrap(const char* pBufferText, int iBufferSize, int x, int y, const char* pCharsetName, CogeFont* pFont = NULL, int iAlpha = -1);
    void PrintBufferTextAlign(const char* pBufferText, int iBufferSize, int iAlignX, int iAlignY, const char* pCharsetName, CogeFont* pFont = NULL, int iAlpha = -1);
    bool GetBufferTextSize(const char* pBufferText, int iBufferSize, int* iWidth, int* iHeight, const char* pCharsetName, CogeFont* pFont = NULL);

    void PrintText(const std::string& sText, int x, int y, CogeFont* pFont = NULL, int iAlpha = -1);
    void PrintTextWrap(const std::string& sText, int x, int y, CogeFont* pFont = NULL, int iAlpha = -1);
    void PrintTextAlign(const std::string& sText, int iAlignX, int iAlignY, CogeFont* pFont = NULL, int iAlpha = -1);
    bool GetTextSize(const std::string& sText, int* iWidth, int* iHeight, CogeFont* pFont = NULL);


    void CopyRect(CogeImage* pSrcImage,
                  int iDstLeft, int iDstTop,
                  int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );


    void Blt( CogeImage* pSrcImage,
              int iDstLeft, int iDstTop, int iSrcColorKey,
              int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );


    void DrawLine(int iStartX, int iStartY, int iEndX, int iEndY);
    void DrawCircle(int iCenterX, int iCenterY, int iRadius);


    void Grayscale(int iDstX, int iDstY, int iWidth, int iHeight);

    void Grayscale(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount);

    void SubLight(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount);

    void Lightness(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount);

    void Blur(int iDstX, int iDstY, int iWidth, int iHeight, int iAmount);

    void ChangeColorRGB(int iDstX, int iDstY, int iWidth, int iHeight,
                      int iRedAmount, int iGreenAmount, int iBlueAmount);


    void BltAlphaBlend( CogeImage* pSrcImage, int iAlpha,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltChangedRGB( CogeImage* pSrcImage, int iAmount,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltWithEdge( CogeImage* pSrcImage, int iEdgeColor,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltWithColor( CogeImage* pSrcImage, int iColor, int iAlpha,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltWithLightness( CogeImage* pSrcImage, int iAmount,
                        int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void LightMaskBlend( CogeImage* pSrcImage, int iDstLeft, int iDstTop,
                        int iSrcLeft=0, int iSrcTop=0, int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltMask( CogeImage* pSrcImage, CogeImage* pMaskImage, int iDstLeft, int iDstTop,
                  int iSrcLeft=0, int iSrcTop=0, int iMaskLeft=0, int iMaskTop=0,
                  int iSrcWidth=-1, int iSrcHeight=-1 );

    void BltStretchFast(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom);

    void BltStretch(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom);

    void BltStretchSmoothly(CogeImage* pSrcImage, int iSrcLeft, int iSrcTop, int iSrcRight, int iSrcBottom,
                    int iDstLeft, int iDstTop, int iDstRight, int iDstBottom);

    void BltRotate(CogeImage* pSrcImage, double fAngle,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight  );

    void BltRotozoom(CogeImage* pSrcImage, double fAngle, int iZoom,
                    int iDstLeft, int iDstTop,
                    int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight  );

    void BltWave(CogeImage* pSrcImage, double iXAmount, double iYAmount, double iZAmount,
                int iXSteps, int iYSteps,
                int iDstLeft, int iDstTop,
                int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight);

    void BltWave(CogeImage* pSrcImage, int iAmount,
                int iDstLeft, int iDstTop,
                int iSrcLeft, int iSrcTop, int iSrcWidth, int iSrcHeight);


    //constructor
    explicit CogeImage(const std::string& sName);

    //destructor
    ~CogeImage();


    friend class CogeVideo;
    friend class CogeAnima;

};



#endif // __OGE_VIDEO_H_INCLUDED__
