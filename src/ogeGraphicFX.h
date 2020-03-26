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

#ifndef __OGE_GRAPHICFX_H_INCLUDED__
#define __OGE_GRAPHICFX_H_INCLUDED__

#include <stdint.h>

int OGE_FX_CheckMMX();

int OGE_FX_Init();

int OGE_FX_Saturate(int i, int iMax);

void OGE_FX_SetPixel16(uint8_t* pBase, int iDelta, int iX, int iY, uint16_t iColor);
void OGE_FX_SetPixel32(uint8_t* pBase, int iDelta, int iX, int iY, uint32_t iColor);

void OGE_FX_Line(uint8_t* pDstData, int iDstLineSize, int iBPP, int iColor,
                 int iMaxX, int iMaxY,
                 int x1, int y1, int x2, int y2);

void OGE_FX_Circle(uint8_t* pDstData, int iDstLineSize, int iBPP, int iColor,
                   int iMaxX, int iMaxY,
                   int iCenterX, int iCenterY, int iRadius);

void OGE_FX_CopyRect(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP);

void OGE_FX_Blt(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP);

void OGE_FX_UpdateAlphaBlt(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      uint8_t* pSrcData, int iSrcLineSize,
                      int iSrcX, int iSrcY,
                      int iWidth, int iHeight, int iBPP, uint32_t iAlphaByteMask, int iDelta);

void OGE_FX_Grayscale(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP);

void OGE_FX_ChangeGrayLevel(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_SubLight(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_Lightness(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_BltLightness(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_ChangeColorRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount);

void OGE_FX_BltChangedRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                      int iSrcX, int iSrcY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount);

void OGE_FX_SplitBlur(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_GaussianBlur(uint8_t* pDstData, int iDstLineSize,
                          int iDstX, int iDstY,
                          int iWidth, int iHeight, int iBPP, int iAmount);

void OGE_FX_AlphaBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, uint8_t iAlpha);


void OGE_FX_LightMaskBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP);

void OGE_FX_BltMask(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                uint8_t* pMaskData, int iMaskLineSize,
                int iMaskX, int iMaskY,
                int iWidth, int iHeight, int iBPP, int iBaseAmount = 0x000f0f0f);

void OGE_FX_StretchSmoothly(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY, uint32_t iSrcWidth,uint32_t iSrcHeight, int iBPP);


void OGE_FX_BltStretch(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY, uint32_t iSrcWidth, uint32_t iSrcHeight, int iBPP);

void OGE_FX_BltRotate(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY, uint32_t iSrcWidth, uint32_t iSrcHeight, int iBPP,
                int iSrcColorKey, double fAngle, int iZoom = 65536);

void OGE_FX_BltSquareWave(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP,
                double iXAmount, double iYAmount, double iZAmount); // iAmount: 1~50

void OGE_FX_BltRoundWave(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iSrcColorKey,
                double iXAmount, double iYAmount, double iZAmount, int iXSteps = 0, int iYSteps = 0); // iAmount: 1~50


void OGE_FX_BltWithEdge(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iEdgeColor);

void OGE_FX_BltWithColor(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iColor, int iAlpha);




#endif // __OGE_GRAPHICFX_H_INCLUDED__
