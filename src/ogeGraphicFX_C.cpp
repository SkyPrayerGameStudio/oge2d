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

#include "ogeGraphicFX.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifndef __FX_WITH_C__
#ifndef __FX_WITH_MMX__

#define __FX_WITH_C__

#endif
#endif

// macros
#ifndef SAFE_SET_16_PX
#define SAFE_SET_16_PX(theX, theY) \
if (theX >= 0 && theY >= 0 && theX < iMaxX && theY < iMaxY) \
*(uint16_t*)(pDstData + (theY) * iDstLineSize + 2*(theX)) = (uint16_t)iColor;
#endif //SAFE_SET_16_PX

#ifndef SAFE_SET_32_PX
#define SAFE_SET_32_PX(theX, theY) \
if (theX >= 0 && theY >= 0 && theX < iMaxX && theY < iMaxY) \
*(uint32_t*)(pDstData + (theY) * iDstLineSize + 4*(theX)) = (uint32_t)iColor;
#endif //SAFE_SET_32_PX

#ifndef _OGE_PI_
#define _OGE_PI_  3.14159265
#endif //_OGE_PI_

#ifdef __FX_WITH_C__

#if defined(__MACOSX__) || defined(__IPHONE__)

struct ogeColor32
{
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

#else

struct ogeColor32
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

#endif



union CogeColor32
{
    uint32_t color;
    ogeColor32 argb;
};



#if defined(__MACOSX__) || defined(__IPHONE__)

static const uint32_t alphamask = 0x000000ff;
static const uint32_t redmask   = 0x0000ff00;
static const uint32_t greenmask = 0x00ff0000;
static const uint32_t bluemask  = 0xff000000;

static const uint32_t alphaoffset = 0;
static const uint32_t redoffset   = 8;
static const uint32_t greenoffset = 16;
static const uint32_t blueoffset  = 24;

#else

static const uint32_t alphamask = 0xff000000;
static const uint32_t redmask   = 0x00ff0000;
static const uint32_t greenmask = 0x0000ff00;
static const uint32_t bluemask  = 0x000000ff;

static const uint32_t alphaoffset = 24;
static const uint32_t redoffset   = 16;
static const uint32_t greenoffset = 8;
static const uint32_t blueoffset  = 0;

#endif



/*
static uint64_t __attribute__((aligned(8))) i64MaskRed16   = 0xf800f800f800f800LL;
static uint64_t __attribute__((aligned(8))) i64MaskGreen16 = 0x07e007e007e007e0LL;
static uint64_t __attribute__((aligned(8))) i64MaskBlue16  = 0x001f001f001f001fLL;
static uint64_t __attribute__((aligned(8))) i64X16         = 0x003f003f003f003fLL;
*/
//ri, gi, bi: array[Word] of Byte;
//rw, gw, bw: array[Byte] of Word;

static uint8_t  ri[65536], gi[65536], bi[65536], div3[766];
static uint16_t rw[256],   gw[256],   bw[256];
static uint16_t maskr16, maskg16, maskb16;
static int wavex[65536], wavey[65536];
//static int wavesin[65536], wavecos[65536];

static ogeColor32 cm16[65536];

static int grays[768];
static uint16_t alphas[256];

static int mmxflag = 0;

static int Scale8(int c, int bit)
{
    switch(bit)
    {
    case 1:  if (c) return 255; else return 0;
    case 2:  return (c << 6) | (c << 4) | (c << 2) | c;
    case 3:  return (c << 5) | (c << 2) | (c >> 1);
    case 4:  return (c << 4) | c;
    case 5:  return (c << 3) | (c >> 2);
    case 6:  return (c << 2) | (c >> 4);
    case 7:  return (c << 1) | (c >> 6);
    default: return c;
    }
}

int OGE_FX_Init()
{
    maskr16 = 0xf800; // 5
    maskg16 = 0x07e0; // 6
    maskb16 = 0x001f; // 5

    for(uint32_t X=0; X<=65535; X++)
    {
        bi[X] = X << 3;
        gi[X] = X >> 5 << 2;
        ri[X] = X >> 11 << 3;

        //wavesin[X] = lround( sin(2 * _OGE_PI_ * X / 65535) * 32768 ) + 32768;
        //wavecos[X] = lround( cos(2 * _OGE_PI_ * X / 65535) * 32768 ) + 32768;

        cm16[X].b = Scale8(X & maskb16, 5);
        cm16[X].g = Scale8((X & maskg16) >> 5, 6);
        cm16[X].r = Scale8((X & maskr16) >> 11, 5);

    }
    for(uint32_t X=0; X<=255; X++)
    {
        bw[X] = X >> 3;
        gw[X] = X >> 2 << 5;
        rw[X] = X >> 3 << 11;
    }

    int k=0;
    for(uint32_t i=0; i<=255; i++)
    {
        div3[k++]=i;
        div3[k++]=i;
        div3[k++]=i;
    }

    return 0;
}

int OGE_FX_CheckMMX()
{

    /* Returns 1 if MMX instructions are supported,
	   3 if Cyrix MMX and Extended MMX instructions are supported
	   5 if AMD MMX and 3DNow! instructions are supported
	   0 if hardware does not support any of these
	*/
	register int rval = 0;

	/* Return */
	mmxflag = rval;
	return(rval);

}

int OGE_FX_Saturate(int i, int iMax)
{
    if (i < 0) return 0;
    else if(i>iMax) return iMax;
    else return i;
}

void OGE_FX_SetPixel16(uint8_t* pBase, int iDelta, int iX, int iY, uint16_t iColor)
{
	*((uint16_t*)(pBase + iY * iDelta + 2*iX)) = iColor;
}

void OGE_FX_SetPixel32(uint8_t* pBase, int iDelta, int iX, int iY, uint32_t iColor)
{
	*((uint32_t*)(pBase + iY * iDelta + 4*iX)) = iColor;
}

void OGE_FX_Line(uint8_t* pDstData, int iDstLineSize, int iBPP, int iColor, int iMaxX, int iMaxY,
                 int x1, int y1, int x2, int y2)
{
    int i,deltax,deltay,numpixels,d,dinc1,dinc2,x,xinc1,xinc2,y,yinc1,yinc2,x0,y0;

    if (iMaxX < 0 || iMaxY < 0) return;

	if (std::max(x1,x2) < 0 || std::max(y1,y2) < 0) return;

	x0 = std::max(std::min(x1,x2), 0);
	y0 = std::max(std::min(y1,y2), 0);

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	// Initialize all vars based on which is the independent variable
	if (deltax >= deltay)
	{

		// x is independent variable
		numpixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		// y is independent variable
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	// Make sure x and y move in the right directions
	if (x1 > x2)
	{
		xinc1 = 0 - xinc1;
		xinc2 = 0 - xinc2;
	}
	if (y1 > y2)
	{
		yinc1 = 0 - yinc1;
		yinc2 = 0 - yinc2;
	}

	// Start drawing at
	x = x1;
	y = y1;

	// Draw the pixels
	switch (iBPP)
	{
	case 15:
	case 16:
		for (i = 1; i <= numpixels; i++)
		{
			SAFE_SET_16_PX(x, y);
			//*(uint16_t*)(pDstData + (y-y0) * iDstLineSize + 2*(x-x0)) = (uint16_t)iColor;
			if (d < 0)
			{
				d += dinc1;
				x += xinc1;
				y += yinc1;
			}
			else
			{
				d += dinc2;
				x += xinc2;
				y += yinc2;
			}
		}
		break;

	case 32:
		for (i = 1; i <= numpixels; i++)
		{
			SAFE_SET_32_PX(x, y);
			//*(uint32_t*)(pDstData + (y-y0) * iDstLineSize + 4*(x-x0)) = (uint32_t)iColor;
			if (d < 0)
			{
				d += dinc1;
				x += xinc1;
				y += yinc1;
			}
			else
			{
				d += dinc2;
				x += xinc2;
				y += yinc2;
			}
		}
		break;

	}   // end switch


}

void OGE_FX_Circle(uint8_t* pDstData, int iDstLineSize, int iBPP, int iColor, int iMaxX, int iMaxY,
                   int iCenterX, int iCenterY, int iRadius)
{
    int x, y, p, x0, y0;

	if (iRadius <= 0) return;

	if (iMaxX < 0 || iMaxY < 0) return;

	x0 = iCenterX;
	y0 = iCenterY;

	x = 0; y = iRadius; p = 1-iRadius;

    //CircleSetPoint(iCenterX, iCenterY, x, y);
	switch (iBPP)
	{
    case 15:
	case 16:

        SAFE_SET_16_PX(x0 + x, y0 + y);
        SAFE_SET_16_PX(x0 - x, y0 + y);
        SAFE_SET_16_PX(x0 + x, y0 - y);
        SAFE_SET_16_PX(x0 - x, y0 - y);
        SAFE_SET_16_PX(x0 + y, y0 + x);
        SAFE_SET_16_PX(x0 - y, y0 + x);
        SAFE_SET_16_PX(x0 + y, y0 - x);
        SAFE_SET_16_PX(x0 - y, y0 - x);

        break;

	case 32:

        SAFE_SET_32_PX(x0 + x, y0 + y);
        SAFE_SET_32_PX(x0 - x, y0 + y);
        SAFE_SET_32_PX(x0 + x, y0 - y);
        SAFE_SET_32_PX(x0 - x, y0 - y);
        SAFE_SET_32_PX(x0 + y, y0 + x);
        SAFE_SET_32_PX(x0 - y, y0 + x);
        SAFE_SET_32_PX(x0 + y, y0 - x);
        SAFE_SET_32_PX(x0 - y, y0 - x);

        break;

	} // end switch case

    while (x < y)
	{
		x++;
		if (p < 0) p += 2*x + 1;
		else
		{
			y--;
			p += 2*(x-y) + 1;
		}

      //CircleSetPoint(iCenterX, iCenterY, x, y);
		switch (iBPP)
		{
		//case 15:
		case 16:

            SAFE_SET_16_PX(x0 + x, y0 + y);
            SAFE_SET_16_PX(x0 - x, y0 + y);
            SAFE_SET_16_PX(x0 + x, y0 - y);
            SAFE_SET_16_PX(x0 - x, y0 - y);
            SAFE_SET_16_PX(x0 + y, y0 + x);
            SAFE_SET_16_PX(x0 - y, y0 + x);
            SAFE_SET_16_PX(x0 + y, y0 - x);
            SAFE_SET_16_PX(x0 - y, y0 - x);

			break;

		case 32:

            SAFE_SET_32_PX(x0 + x, y0 + y);
            SAFE_SET_32_PX(x0 - x, y0 + y);
            SAFE_SET_32_PX(x0 + x, y0 - y);
            SAFE_SET_32_PX(x0 - x, y0 - y);
            SAFE_SET_32_PX(x0 + y, y0 + x);
            SAFE_SET_32_PX(x0 - y, y0 + x);
            SAFE_SET_32_PX(x0 + y, y0 - x);
            SAFE_SET_32_PX(x0 - y, y0 - x);

			break;

		} // end switch case

	}


}

void OGE_FX_CopyRect(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP)
{
    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes = 0;
	int iLineWidth = 0;
	int iRemain;

	bool bNeedCutSrcWord = false;
	bool bNeedCutDstWord = false;

	int i;

	switch (iBPP)
	{
    case 8:
      iLineBytes = iWidth;
      pDstData += iDstY * iDstLineSize + iDstX;
      pSrcData += iSrcY * iSrcLineSize + iSrcX;
	  break;
	case 15:
	case 16:
      bNeedCutSrcWord = (iSrcX & 1) != 0;
      bNeedCutDstWord = (iDstX & 1) != 0;
      iLineBytes = iWidth << 1;
      pDstData += iDstY * iDstLineSize + (iDstX << 1);
      pSrcData += iSrcY * iSrcLineSize + (iSrcX << 1);
	  break;

    case 24:
      iLineBytes = iWidth * 3;
      pDstData += iDstY * iDstLineSize + iDstX * 3;
      pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;
	  break;

    case 32:
      iLineBytes = iWidth << 2;
	  pDstData += iDstY * iDstLineSize + (iDstX << 2);
      pSrcData += iSrcY * iSrcLineSize + (iSrcX << 2);
	  break;

	}

	iSrcDataPad = iSrcLineSize - iLineBytes;
	iDstDataPad = iDstLineSize - iLineBytes;

	if(bNeedCutSrcWord != bNeedCutDstWord)
    {
        iWidth = iLineBytes >> 1;
        iRemain = iLineBytes & 1;

        while (iHeight > 0)
        {
            iLineWidth = iWidth;
            while (iLineWidth > 0)
            {
                *((uint16_t*)pDstData) = *((uint16_t*)pSrcData);

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            // handle the remain
            for (i=1; i<=iRemain; i++)
            {
                *(pDstData) = *(pSrcData);

                pSrcData++;
                pDstData++;
            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        } // end of while (iHeight > 0)

    }
    else if(!bNeedCutSrcWord)
    {

        iWidth = iLineBytes >> 2;
        iRemain = iLineBytes & 3;

        while (iHeight > 0)
        {
            iLineWidth = iWidth;
            while (iLineWidth > 0)
            {
                *((int*)pDstData) = *((int*)pSrcData);

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            // handle the remain
            for (i=1; i<=iRemain; i++)
            {
                *(pDstData) = *(pSrcData);

                pSrcData++;
                pDstData++;
            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        } // end of while (iHeight > 0)


    }
    else
    {

        iWidth = (iLineBytes - 2) >> 2;
        iRemain = (iLineBytes - 2) & 3;

        while (iHeight > 0)
        {

            *((uint16_t*)pDstData) = *((uint16_t*)pSrcData);

            pSrcData += 2;
            pDstData += 2;


            iLineWidth = iWidth;
            while (iLineWidth > 0)
            {
                *((int*)pDstData) = *((int*)pSrcData);

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            // handle the remain
            for (i=1; i<=iRemain; i++)
            {
                *(pDstData) = *(pSrcData);

                pSrcData++;
                pDstData++;
            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        } // end of while (iHeight > 0)

    }




}

void OGE_FX_Blt(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16Color = *((uint16_t*)pSrcData);
                if(bNoColorKey || i16Color != i16ColorKey) *((uint16_t*)pDstData) = i16Color;

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


    case 24:
    {
        iLineBytes = iWidth * 3;
        pDstData += iDstY * iDstLineSize + iDstX * 3;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i24ColorKey = iSrcColorKey & 0x00ffffff;
        uint32_t i24Color = 0;

        int iColorSize = sizeof(uint8_t) * 3;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i24Color = (*(uint32_t*)pSrcData) & 0x00ffffff;
                if(bNoColorKey || i24Color != i24ColorKey) memcpy(pDstData, pSrcData, iColorSize);

                pSrcData += 3;
                pDstData += 3;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32Color = *((uint32_t*)pSrcData);
                if(bNoColorKey || i32Color != i32ColorKey) *((uint32_t*)pDstData) = i32Color;

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}



}


void OGE_FX_SubLight(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    int iDstDataPad, iPixel;
	uint8_t iB, iG, iR, i8Amount, i6Amount, i5Amount;

	int iLineBytes;
	int iLineWidth;

	i8Amount = abs(iAmount);

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;

        iDstDataPad = iDstLineSize - iLineBytes;

        i5Amount = i8Amount >> 3;
        i6Amount = i8Amount >> 2;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((uint16_t*)pDstData);
                iB = iPixel & 0x001f;
                iG = (iPixel >> 5) &  0x003f;
                iR = (iPixel >> 11) & 0x001f;

                iB = OGE_FX_Saturate(iB - i5Amount, 31);
                iG = OGE_FX_Saturate(iG - i6Amount, 63);
                iR = OGE_FX_Saturate(iR - i5Amount, 31);

                *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

                pDstData += 2;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;

        iDstDataPad = iDstLineSize - iLineBytes;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((int*)pDstData);
                iB = (iPixel >> blueoffset)  & 0xff;
                iG = (iPixel >> greenoffset) & 0xff;
                iR = (iPixel >> redoffset)   & 0xff;

                iB = OGE_FX_Saturate(iB - i8Amount, 255);
                iG = OGE_FX_Saturate(iG - i8Amount, 255);
                iR = OGE_FX_Saturate(iR - i8Amount, 255);

                *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                pDstData += 4;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}

}

void OGE_FX_Lightness(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    int iDstDataPad, iPixel;
	uint8_t iB, iG, iR, i8Amount;

	int iLineBytes;
	int iLineWidth;

	i8Amount = abs(iAmount);

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;

        iDstDataPad = iDstLineSize - iLineBytes;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((uint16_t*)pDstData);
                iB = iPixel & 0x001f;
                iG = (iPixel >> 5) &  0x003f;
                iR = (iPixel >> 11) & 0x001f;

                if (iAmount >= 0)
                {
                    iB = OGE_FX_Saturate(iB + (i8Amount * (iB ^ 31) >> 8), 31);
                    iG = OGE_FX_Saturate(iG + (i8Amount * (iG ^ 63) >> 8), 63);
                    iR = OGE_FX_Saturate(iR + (i8Amount * (iR ^ 31) >> 8), 31);
                }
                else
                {
                    iB = OGE_FX_Saturate(iB - (i8Amount * iB >> 8), 31);
                    iG = OGE_FX_Saturate(iG - (i8Amount * iG >> 8), 63);
                    iR = OGE_FX_Saturate(iR - (i8Amount * iR >> 8), 31);
                }

                *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

                pDstData += 2;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;

        iDstDataPad = iDstLineSize - iLineBytes;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((int*)pDstData);

                iB = (iPixel >> blueoffset)  & 0xff;
                iG = (iPixel >> greenoffset) & 0xff;
                iR = (iPixel >> redoffset)   & 0xff;

                if (iAmount >= 0)
                {
                    iB = OGE_FX_Saturate(iB + (i8Amount * (iB ^ 255) >> 8), 255);
                    iG = OGE_FX_Saturate(iG + (i8Amount * (iG ^ 255) >> 8), 255);
                    iR = OGE_FX_Saturate(iR + (i8Amount * (iR ^ 255) >> 8), 255);
                }
                else
                {
                    iB = OGE_FX_Saturate(iB - (i8Amount * iB >> 8), 255);
                    iG = OGE_FX_Saturate(iG - (i8Amount * iG >> 8), 255);
                    iR = OGE_FX_Saturate(iR - (i8Amount * iR >> 8), 255);
                }

                *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                pDstData += 4;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}

}

void OGE_FX_ChangeColorRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount)
{

    int iDstDataPad, iPixel;
	uint8_t iB, iG, iR, i8RedAmount, i8GreenAmount, i8BlueAmount;

	int iLineBytes;
	int iLineWidth;

#if defined(__MACOSX__) || defined(__IPHONE__)

    if(iBPP == 16)
    {
        iLineBytes = iRedAmount;
        iRedAmount = iBlueAmount;
        iBlueAmount = iLineBytes;
    }

#endif


    i8RedAmount = abs(iRedAmount);
    i8GreenAmount = abs(iGreenAmount);
    i8BlueAmount = abs(iBlueAmount);

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;

        iDstDataPad = iDstLineSize - iLineBytes;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((uint16_t*)pDstData);
                iB = iPixel & 0x001f;
                iG = (iPixel >> 5) &  0x003f;
                iR = (iPixel >> 11) & 0x001f;

                if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + (iBlueAmount >> 3), 31);
                else iB = OGE_FX_Saturate(iB - (i8BlueAmount >> 3), 31);

                if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + (iGreenAmount >> 2), 63);
                else iG = OGE_FX_Saturate(iG - (i8GreenAmount >> 2), 63);

                if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + (iRedAmount >> 3), 31);
                else iR = OGE_FX_Saturate(iR - (i8RedAmount >> 3), 31);

                *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

                pDstData += 2;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;

        iDstDataPad = iDstLineSize - iLineBytes;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                iPixel = *((int*)pDstData);

                iB = (iPixel >> blueoffset)  & 0xff;
                iG = (iPixel >> greenoffset) & 0xff;
                iR = (iPixel >> redoffset)   & 0xff;

                if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + iBlueAmount, 255);
                else iB = OGE_FX_Saturate(iB - i8BlueAmount, 255);

                if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + iGreenAmount, 255);
                else iG = OGE_FX_Saturate(iG - i8GreenAmount, 255);

                if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + iRedAmount, 255);
                else iR = OGE_FX_Saturate(iR - i8RedAmount, 255);

                *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                pDstData += 4;

                iLineWidth--;

            }

            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}

}

void OGE_FX_ChangeGrayLevel(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    int X, Y, iGray;
    int r, g, b;
    //uint32_t c32;
    uint16_t cw;
    char* start;
    uint16_t* pw;
    ogeColor32* pc32;

    for(int i=0; i<256; i++) alphas[i] = (i*iAmount) >> 8;

    X = 0;
    for(int i=0; i<256; i++)
    {
        iGray = i - alphas[i];
        grays[X++] = iGray;
        grays[X++] = iGray;
        grays[X++] = iGray;
    }

    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;

        pw  = (uint16_t*)(pDstData);
        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pw;
            for(X=0;X<iWidth;X++)
            {
                cw = *pw;

                r = (cw&maskr16)>>11<<3;
                g = (cw&maskg16)>>5<<2;
                b = (cw&maskb16)<<3;

                iGray = grays[r+g+b];

                r = OGE_FX_Saturate(iGray+alphas[r], 255);
                g = OGE_FX_Saturate(iGray+alphas[g], 255);
                b = OGE_FX_Saturate(iGray+alphas[b], 255);

                *pw = (r>>3 << 11) | (g>>2 << 5) | (b>>3);

                pw++;

            }
            pw = (uint16_t*)(start + iDstLineSize);

        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;

        pc32  = (ogeColor32*)(pDstData);
        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pc32;
            //pc32  = (ogeColor32*)(pDstData + iDstLineSize*Y);
            for(X=0;X<iWidth;X++)
            {
                iGray = grays[pc32->r + pc32->g + pc32->b];
                pc32->r = OGE_FX_Saturate(iGray+alphas[pc32->r], 255);
                pc32->g = OGE_FX_Saturate(iGray+alphas[pc32->g], 255);
                pc32->b = OGE_FX_Saturate(iGray+alphas[pc32->b], 255);

                pc32++;

            }
            pc32 = (ogeColor32*)(start + iDstLineSize);

        }


    break;
    }
}


void OGE_FX_BltLightness(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iAmount)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	//int iPixel;
	uint8_t iB, iG, iR;

	uint8_t i8Amount = abs(iAmount);

	bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16Color = *((uint16_t*)pSrcData);
                if (bNoColorKey || i16Color != i16ColorKey)
                {
                    iB = i16Color & 0x001f;
                    iG = (i16Color >> 5) &  0x003f;
                    iR = (i16Color >> 11) & 0x001f;
                    if (iAmount >= 0)
                    {
                        iB = OGE_FX_Saturate(iB + (iAmount * (iB ^ 31) >> 8), 31);
                        iG = OGE_FX_Saturate(iG + (iAmount * (iG ^ 63) >> 8), 63);
                        iR = OGE_FX_Saturate(iR + (iAmount * (iR ^ 31) >> 8), 31);
                    }
                    else
                    {
                        iB = OGE_FX_Saturate(iB - (i8Amount * iB >> 8), 31);
                        iG = OGE_FX_Saturate(iG - (i8Amount * iG >> 8), 63);
                        iR = OGE_FX_Saturate(iR - (i8Amount * iR >> 8), 31);
                    }

                    *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
                }

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32Color = *((uint32_t*)pSrcData);
                if(bNoColorKey || i32Color != i32ColorKey)
                {
                    iB = (i32Color >> blueoffset)  & 0xff;
                    iG = (i32Color >> greenoffset) & 0xff;
                    iR = (i32Color >> redoffset)   & 0xff;

                    if (iAmount >= 0)
                    {
                        iB = OGE_FX_Saturate(iB + (iAmount * (iB ^ 255) >> 8), 255);
                        iG = OGE_FX_Saturate(iG + (iAmount * (iG ^ 255) >> 8), 255);
                        iR = OGE_FX_Saturate(iR + (iAmount * (iR ^ 255) >> 8), 255);
                    }
                    else
                    {
                        iB = OGE_FX_Saturate(iB - (i8Amount * iB >> 8), 255);
                        iG = OGE_FX_Saturate(iG - (i8Amount * iG >> 8), 255);
                        iR = OGE_FX_Saturate(iR - (i8Amount * iR >> 8), 255);
                    }

                    *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);
                }

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}

}

void OGE_FX_BltChangedRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                      int iSrcX, int iSrcY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

#if defined(__MACOSX__) || defined(__IPHONE__)

    if(iBPP == 16)
    {
        iLineBytes = iRedAmount;
        iRedAmount = iBlueAmount;
        iBlueAmount = iLineBytes;
    }

#endif

	//int iPixel;
	uint8_t iB, iG, iR;

	uint8_t i8RedAmount = abs(iRedAmount);
	uint8_t i8GreenAmount = abs(iGreenAmount);
	uint8_t i8BlueAmount = abs(iBlueAmount);

	uint8_t i5RedAmount = i8RedAmount >> 3;
	uint8_t i6GreenAmount = i8GreenAmount >> 2;
	uint8_t i5BlueAmount = i8BlueAmount >> 3;

	bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16Color = *((uint16_t*)pSrcData);
                if (bNoColorKey || i16Color != i16ColorKey)
                {
                    iB = i16Color & 0x001f;
                    iG = (i16Color >> 5) &  0x003f;
                    iR = (i16Color >> 11) & 0x001f;

                    if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + i5BlueAmount, 31);
                    else iB = OGE_FX_Saturate(iB - i5BlueAmount, 31);

                    if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + i6GreenAmount, 63);
                    else iG = OGE_FX_Saturate(iG - i6GreenAmount, 63);

                    if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + i5RedAmount, 31);
                    else iR = OGE_FX_Saturate(iR - i5RedAmount, 31);

                    *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
                }

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32Color = 0;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32Color = *((uint32_t*)pSrcData);
                if(bNoColorKey || i32Color != i32ColorKey)
                {
                    iB = (i32Color >> blueoffset)  & 0xff;
                    iG = (i32Color >> greenoffset) & 0xff;
                    iR = (i32Color >> redoffset)   & 0xff;

                    if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + i8BlueAmount, 255);
                    else iB = OGE_FX_Saturate(iB - i8BlueAmount, 255);

                    if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + i8GreenAmount, 255);
                    else iG = OGE_FX_Saturate(iG - i8GreenAmount, 255);

                    if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + i8RedAmount, 255);
                    else iR = OGE_FX_Saturate(iR - i8RedAmount, 255);

                    *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);
                }

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}


}

void OGE_FX_AlphaBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, uint8_t iAlpha)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	//int iPixel;
	uint8_t iB, iG, iR;

	uint8_t iSmallAlpha = iAlpha >> 2;

	bool bIsHalfAlpha = iAlpha == 128;

	bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16ColorSrc, i16ColorDst;

        if(bIsHalfAlpha)
        {
            while(iHeight > 0)
            {
                iLineWidth = iWidth;
                while(iLineWidth > 0)
                {
                    i16ColorSrc = *((uint16_t*)pSrcData);
                    if (bNoColorKey || i16ColorSrc != i16ColorKey)
                    {
                        i16ColorDst = *((uint16_t*)pDstData);
                        *((uint16_t*)pDstData) = ((i16ColorSrc & 0xF7DE) >> 1) + ((i16ColorDst & 0xF7DE) >> 1);
                    }
                    pSrcData += 2;
                    pDstData += 2;
                    iLineWidth--;
                }
                pSrcData += iSrcDataPad;
                pDstData += iDstDataPad;
                iHeight--;
            }
        }
        else
        {

            while(iHeight > 0)
            {
                iLineWidth = iWidth;
                while(iLineWidth > 0)
                {
                    i16ColorSrc = *((uint16_t*)pSrcData);
                    if (bNoColorKey || i16ColorSrc != i16ColorKey)
                    {
                        i16ColorDst = *((uint16_t*)pDstData);

                        iB = ((iAlpha * ((i16ColorSrc & 0x001f) + 64 - (i16ColorDst & 0x001f))) >> 8) +
                            (i16ColorDst & 0x001f) - iSmallAlpha;

                        iG = ((iAlpha * (((i16ColorSrc & 0x07e0) >> 5) + 64 - ((i16ColorDst & 0x07e0) >> 5))) >> 8) +
                            ((i16ColorDst & 0x07e0) >> 5) - iSmallAlpha;

                        iR = ((iAlpha * (((i16ColorSrc & 0xf800) >> 11) + 64 - ((i16ColorDst & 0xf800) >> 11))) >> 8) +
                            ((i16ColorDst & 0xf800) >> 11) - iSmallAlpha;

                        *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
                    }

                    pSrcData += 2;
                    pDstData += 2;

                    iLineWidth--;

                }

                pSrcData += iSrcDataPad;
                pDstData += iDstDataPad;

                iHeight--;

            }

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32ColorSrc, i32ColorDst;


        if(bIsHalfAlpha)
        {
            while(iHeight > 0)
            {
                iLineWidth = iWidth;
                while(iLineWidth > 0)
                {
                    i32ColorSrc = *((uint32_t*)pSrcData);
                    if(bNoColorKey || i32ColorSrc != i32ColorKey)
                    {
                        i32ColorDst = *((int*)pDstData);
                        *((int*)pDstData) = ((i32ColorSrc & 0xFEFEFE) >> 1) + ((i32ColorDst & 0xFEFEFE) >> 1);
                    }
                    pSrcData += 4;
                    pDstData += 4;
                    iLineWidth--;
                }
                pSrcData += iSrcDataPad;
                pDstData += iDstDataPad;
                iHeight--;
            }
        }
        else
        {

            while(iHeight > 0)
            {
                iLineWidth = iWidth;
                while(iLineWidth > 0)
                {
                    i32ColorSrc = *((uint32_t*)pSrcData);
                    if(bNoColorKey || i32ColorSrc != i32ColorKey)
                    {
                        i32ColorDst = *((int*)pDstData);

                        iB = ((iAlpha * (((i32ColorSrc & bluemask)  >>  blueoffset) + 256 - ((i32ColorDst & bluemask)  >> blueoffset))) >> 8) +
                            ((i32ColorDst & bluemask)  >> blueoffset)  - iAlpha;

                        iG = ((iAlpha * (((i32ColorSrc & greenmask) >> greenoffset) + 256 - ((i32ColorDst & greenmask) >> greenoffset))) >> 8) +
                            ((i32ColorDst & greenmask) >> greenoffset) - iAlpha;

                        iR = ((iAlpha * (((i32ColorSrc & redmask)   >> redoffset) + 256 - ((i32ColorDst & redmask)     >> redoffset))) >> 8) +
                            ((i32ColorDst & redmask)   >> redoffset)   - iAlpha;

                        *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                    }

                    pSrcData += 4;
                    pDstData += 4;

                    iLineWidth--;

                }

                pSrcData += iSrcDataPad;
                pDstData += iDstDataPad;

                iHeight--;

            }

        }

        break;
    }


	}

}

void OGE_FX_LightMaskBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP)
{


    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	uint8_t iB, iG, iR;

	//bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        //uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16ColorSrc, i16ColorDst;

        uint8_t iAmount, iGreenAmount;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16ColorSrc = *((uint16_t*)pSrcData);
                i16ColorDst = *((uint16_t*)pDstData);

                iAmount = i16ColorSrc & 0x001f;
                iGreenAmount = (i16ColorSrc & 0x07e0) >> 5;
                iB = i16ColorDst & 0x001f;
                iB = OGE_FX_Saturate(iB - ((iAmount * iB) >> 5), 31);
                iG = (i16ColorDst & 0x07e0) >> 5;
                iG = OGE_FX_Saturate(iG - ((iGreenAmount * iG) >> 6), 63);
                iR = (i16ColorDst & 0xf800) >> 11;
                iR = OGE_FX_Saturate(iR - ((iAmount * iR) >> 5), 31);

                *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        //uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32ColorSrc, i32ColorDst;
        uint8_t iAmount;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32ColorSrc = *((uint32_t*)pSrcData);
                i32ColorDst = *((uint32_t*)pDstData);

                iAmount = (i32ColorSrc & bluemask) >> blueoffset;
                iB = (i32ColorDst  & bluemask) >> blueoffset;
                iB = OGE_FX_Saturate(iB - ((iAmount * iB) >> 8), 255);
                iG = (i32ColorDst & greenmask) >> greenoffset;
                iG = OGE_FX_Saturate(iG - ((iAmount * iG) >> 8), 255);
                iR = (i32ColorDst & redmask)   >> redoffset;
                iR = OGE_FX_Saturate(iR - ((iAmount * iR) >> 8), 255);

                *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}


}

void OGE_FX_BltMask(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                uint8_t* pMaskData, int iMaskLineSize,
                int iMaskX, int iMaskY,
                int iWidth, int iHeight, int iBPP, int iBaseAmount)
{

    int iSrcDataPad;
	int iDstDataPad;
	int iMaskDataPad;

	int iLineBytes;
	int iLineWidth;

	//bool bNoColorKey = iSrcColorKey == -1;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;
        pMaskData += iMaskY * iMaskLineSize + iMaskX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;
        iMaskDataPad = iMaskLineSize - iLineBytes;

        //uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16ColorSrc, i16ColorMask;

        uint32_t iB = iBaseAmount & 0x000000ff;
        uint32_t iG = iBaseAmount & 0x0000ff00 >> 8;
        uint32_t iR = iBaseAmount & 0x00ff0000 >> 16;

        uint16_t iAmount = (iR >> 3 << 11 | iG >> 2 << 5 | iB >> 3) & 0x0000ffff;


        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16ColorSrc = *((uint16_t*)pSrcData);
                i16ColorMask = *((uint16_t*)pMaskData);

                if(i16ColorMask > iAmount) *((uint16_t*)pDstData) = i16ColorSrc;

                pSrcData += 2;
                pDstData += 2;
                pMaskData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;
            pMaskData += iMaskDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
        pMaskData += iMaskY * iMaskLineSize + iMaskX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;
        iMaskDataPad = iMaskLineSize - iLineBytes;

        //uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32ColorSrc, i32ColorMask;
        uint32_t iAmount = iBaseAmount;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32ColorSrc = *((uint32_t*)pSrcData);
                i32ColorMask = *((uint32_t*)pMaskData);

                if(i32ColorMask > iAmount) *((uint32_t*)pDstData) = i32ColorSrc;

                pSrcData += 4;
                pDstData += 4;
                pMaskData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;
            pMaskData += iMaskDataPad;

            iHeight--;

        }

        break;
    }


	}

}

void OGE_FX_StretchSmoothly(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY, uint32_t iSrcWidth, uint32_t iSrcHeight, int iBPP)
{
    //int X, Y, xp;

    uint32_t X, Y, xp, yp, ypp, xpp, t, z, z2, iz2, w1, w2, w3, w4;

    ogeColor32* y1;
    ogeColor32* y2;
    ogeColor32* pc;

    uint16_t* yw1;
    uint16_t* yw2;
    uint16_t* pcw;


    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;


        yp  = 0;
        xpp = ((iSrcWidth - 1) << 16) / iDstWidth;
        ypp = ((iSrcHeight - 1) << 16) / iDstHeight;

        for(Y=0;Y<iDstHeight;Y++)
        {
            xp = yp >> 16;

            yw1 = (uint16_t*)(pSrcData + iSrcLineSize*xp);
            if(xp < iSrcHeight - 1) xp++;
            yw2 = (uint16_t*)(pSrcData + iSrcLineSize*xp);
            xp = 0;
            z2 = (yp & 0xffff) + 1;
            iz2= ((~yp) & 0xffff) + 1;

            pcw  = (uint16_t*)(pDstData + iDstLineSize*Y);

            for(X=0;X<iDstWidth;X++)
            {
                t  = xp >> 16;
                z  = xp & 0xffff;
                w2 = (iz2 * z) >> 16;
                w1 = iz2 - w2;
                w4 = (z2 * z) >> 16;
                w3 = z2 - w4;

                *pcw =
        bw[(bi[*(yw1+t)] * w1 + bi[*(yw1+t+1)] * w2 + bi[*(yw2+t)] * w3 + bi[*(yw2+t+1)] * w4) >> 16] |
        gw[(gi[*(yw1+t)] * w1 + gi[*(yw1+t+1)] * w2 + gi[*(yw2+t)] * w3 + gi[*(yw2+t+1)] * w4) >> 16] |
        rw[(ri[*(yw1+t)] * w1 + ri[*(yw1+t+1)] * w2 + ri[*(yw2+t)] * w3 + ri[*(yw2+t+1)] * w4) >> 16];

                xp += xpp;

                pcw++;


            }

            yp += ypp;
        }

    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        //iDstDataPad = (iDstLineSize - iDstWidth << 2) >> 2;


        yp  = 0;
        xpp = ((iSrcWidth - 1) << 16) / iDstWidth;
        ypp = ((iSrcHeight - 1) << 16) / iDstHeight;

        //pc  = (ogeColor32*)(pDstData);

        for(Y=0;Y<iDstHeight;Y++)
        {
            xp = yp >> 16;
            y1 = (ogeColor32*)(pSrcData + iSrcLineSize*xp);
            if(xp < iSrcHeight - 1) xp++;
            y2 = (ogeColor32*)(pSrcData + iSrcLineSize*xp);
            xp = 0;
            z2 = (yp & 0xffff) + 1;
            iz2= ((~yp) & 0xffff) + 1;

            pc  = (ogeColor32*)(pDstData + iDstLineSize*Y);

            for(X=0;X<iDstWidth;X++)
            {
                t  = xp >> 16;
                z  = xp & 0xffff;
                w2 = (iz2 * z) >> 16;
                w1 = iz2 - w2;
                w4 = (z2 * z) >> 16;
                w3 = z2 - w4;

                pc->b = (  (y1+t)->b * w1 + (y1+t+1)->b * w2 + (y2+t)->b * w3 + (y2+t+1)->b * w4   ) >> 16;
                pc->g = (  (y1+t)->g * w1 + (y1+t+1)->g * w2 + (y2+t)->g * w3 + (y2+t+1)->g * w4   ) >> 16;
                pc->r = (  (y1+t)->r * w1 + (y1+t+1)->r * w2 + (y2+t)->r * w3 + (y2+t+1)->r * w4   ) >> 16;

                //pc->a = 0;

                xp += xpp;

                pc++;


            }
            //pc += iDstDataPad;
            yp += ypp;
        }


    break;
    }
}

void OGE_FX_BltStretch(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY, uint32_t iSrcWidth, uint32_t iSrcHeight, int iBPP)
{
    uint32_t X, Y, xp, yp, sx, sy;

    uint32_t* pc32;
    uint32_t* pline32;
    uint16_t* pc16;
    uint16_t* pline16;

    int iColor;


    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        sx = (iSrcWidth << 16) / iDstWidth;
        sy = (iSrcHeight << 16) / iDstHeight;

        yp = 0;

        if(iSrcColorKey == -1)
        {
            for(Y=0;Y<iDstHeight;Y++)
            {
                xp = yp >> 16;
                pline16 = (uint16_t*)(pSrcData + iSrcLineSize*xp);
                xp = 0;
                pc16  = (uint16_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    *pc16 = *(pline16 + (xp >> 16));
                    xp += sx;
                    pc16++;
                }
                //pc += iDstDataPad;
                yp += sy;
            }
        }
        else
        {
            for(Y=0;Y<iDstHeight;Y++)
            {
                xp = yp >> 16;
                pline16 = (uint16_t*)(pSrcData + iSrcLineSize*xp);
                xp = 0;
                pc16  = (uint16_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    iColor = *(pline16 + (xp >> 16));
                    if(iColor != iSrcColorKey) *pc16 = iColor;
                    xp += sx;
                    pc16++;
                }
                //pc += iDstDataPad;
                yp += sy;
            }
        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        //iDstDataPad = (iDstLineSize - iDstWidth << 2) >> 2;

        sx = (iSrcWidth << 16) / iDstWidth;
        sy = (iSrcHeight << 16) / iDstHeight;

        yp = 0;

        //pc  = (ogeColor32*)(pDstData);

        if(iSrcColorKey == -1)
        {
            for(Y=0;Y<iDstHeight;Y++)
            {
                xp = yp >> 16;
                pline32 = (uint32_t*)(pSrcData + iSrcLineSize*xp);
                xp = 0;
                pc32  = (uint32_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    *pc32 = *(pline32 + (xp >> 16));
                    xp += sx;
                    pc32++;
                }
                //pc += iDstDataPad;
                yp += sy;
            }
        }
        else
        {
            for(Y=0;Y<iDstHeight;Y++)
            {
                xp = yp >> 16;
                pline32 = (uint32_t*)(pSrcData + iSrcLineSize*xp);
                xp = 0;
                pc32  = (uint32_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    iColor = *(pline32 + (xp >> 16));
                    if(iColor != iSrcColorKey) *pc32 = iColor;
                    xp += sx;
                    pc32++;
                }
                //pc += iDstDataPad;
                yp += sy;
            }
        }

    break;
    }
}

void OGE_FX_UpdateAlphaBlt(uint8_t* pDstData, int iDstLineSize,
                          int iDstX, int iDstY,
                          uint8_t* pSrcData, int iSrcLineSize,
                          int iSrcX, int iSrcY,
                          int iWidth, int iHeight, int iBPP, uint32_t iAlphaByteMask, int iDelta)
{
    if(iBPP != 32)
    {
        OGE_FX_CopyRect(pDstData, iDstLineSize, iDstX, iDstY,
                        pSrcData, iSrcLineSize, iSrcX, iSrcY,
                        iWidth, iHeight, iBPP);
        return;
    }

    uint8_t idx = 0;

    if(iAlphaByteMask == 0xff000000) idx = 3;

    uint8_t iAlpha;

    uint32_t* pc;
    uint32_t* pc32;

    uint32_t color;

    pSrcData += iSrcX * iSrcLineSize + iSrcX * 4;
    pDstData += iDstY * iDstLineSize + iDstX * 4;

    for(int Y=0;Y<iHeight;Y++)
    {
        pc   = (uint32_t*)(pSrcData + iSrcLineSize*Y);
        pc32 = (uint32_t*)(pDstData + iDstLineSize*Y);

        for(int X=0;X<iWidth;X++)
        {
            color = *pc;

            if(idx > 0) iAlpha = (color & 0xff000000) >> 24;
            else iAlpha = color & 0x000000ff;

            //int iNewAlpha = iAlpha + iDelta;
            int iNewAlpha = iAlpha * iDelta / 255;
            if(iNewAlpha < 0) iNewAlpha = 0;
            if(iNewAlpha > 255) iNewAlpha = 255;
            iAlpha = iNewAlpha;

            if(idx > 0) color = (color & 0x00ffffff) | (iAlpha << 24);
            else color = (color & 0xffffff00) | iAlpha;

            *pc32 = color;

            pc++;
            pc32++;

        }

    }


}

void OGE_FX_BltRotate(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY, uint32_t iDstWidth, uint32_t iDstHeight,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY, uint32_t iSrcWidth, uint32_t iSrcHeight, int iBPP,
                int iSrcColorKey, double fAngle, int iZoom)
{

    uint32_t X, Y, dx, dy, xd, yd, sdx, sdy, ax, ay, cx, cy, isin, icos;

    int iColor;

    uint32_t* pc32;
    uint16_t* pc16;

    bool bNoColorKey = iSrcColorKey == -1;

    cx = iDstWidth >> 1;
    cy = iDstHeight >> 1;

    //if(iBPP == 16) iZoom = iZoom >> 8;

    isin = lround(sin(fAngle * _OGE_PI_ / 180) * iZoom);
    icos = lround(cos(fAngle * _OGE_PI_ / 180) * iZoom);

    xd = ((iSrcWidth  << 16) - (iDstWidth  << 16)) / 2;
    yd = ((iSrcHeight << 16) - (iDstHeight << 16)) / 2;
    ax = (cx << 16) - (icos * cx);
    ay = (cy << 16) - (isin * cx);

    //pc  = (ogeColor32*)(pDstData);


    if(bNoColorKey)
    {
        switch(iBPP)
        {
        case 16:
            pDstData += iDstY * iDstLineSize + iDstX * 2;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;
            for(Y=0;Y<iDstHeight;Y++)
            {
                dy  = cy - Y;
                sdx = (ax + (isin * dy)) + xd;
                sdy = (ay - (icos * dy)) + yd;
                pc16  = (uint16_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    dx = sdx >> 16;
                    dy = sdy >> 16;
                    if (dx < iSrcWidth && dy < iSrcHeight)
                    {
                        *pc16 = *((uint16_t*)(pSrcData + dy * iSrcLineSize + dx * 2));
                    }
                    sdx += icos;
                    sdy += isin;
                    pc16++;
                }
            }
        break;
        case 32:
            pDstData += iDstY * iDstLineSize + iDstX * 4;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
            //iDstDataPad = (iDstLineSize - iDstWidth << 2) >> 2;
            for(Y=0;Y<iDstHeight;Y++)
            {
                dy  = cy - Y;
                sdx = (ax + (isin * dy)) + xd;
                sdy = (ay - (icos * dy)) + yd;
                pc32  = (uint32_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    dx = sdx >> 16;
                    dy = sdy >> 16;
                    if (dx < iSrcWidth && dy < iSrcHeight)
                    {
                        *pc32 = *((uint32_t*)(pSrcData + dy * iSrcLineSize + dx * 4));
                    }
                    sdx += icos;
                    sdy += isin;
                    pc32++;
                }
            }
        break;
        }

    }
    else
    {
        switch(iBPP)
        {
        case 16:
            pDstData += iDstY * iDstLineSize + iDstX * 2;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;
            for(Y=0;Y<iDstHeight;Y++)
            {
                dy  = cy - Y;
                sdx = (ax + (isin * dy)) + xd;
                sdy = (ay - (icos * dy)) + yd;
                pc16  = (uint16_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    dx = sdx >> 16;
                    dy = sdy >> 16;
                    if (dx < iSrcWidth && dy < iSrcHeight)
                    {
                        iColor = *((uint16_t*)(pSrcData + dy * iSrcLineSize + dx * 2));
                        if(iColor != iSrcColorKey) *pc16 = iColor;
                    }
                    sdx += icos;
                    sdy += isin;
                    pc16++;
                }
            }
        break;
        case 32:
            pDstData += iDstY * iDstLineSize + iDstX * 4;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
            //iDstDataPad = (iDstLineSize - iDstWidth << 2) >> 2;
            for(Y=0;Y<iDstHeight;Y++)
            {
                dy  = cy - Y;
                sdx = (ax + (isin * dy)) + xd;
                sdy = (ay - (icos * dy)) + yd;
                pc32  = (uint32_t*)(pDstData + iDstLineSize*Y);
                for(X=0;X<iDstWidth;X++)
                {
                    dx = sdx >> 16;
                    dy = sdy >> 16;
                    if (dx < iSrcWidth && dy < iSrcHeight)
                    {
                        iColor = *((uint32_t*)(pSrcData + dy * iSrcLineSize + dx * 4));
                        if(iColor != iSrcColorKey) *pc32 = iColor;
                    }
                    sdx += icos;
                    sdy += isin;
                    pc32++;
                }
            }
        break;
        }

    }




}

void OGE_FX_Grayscale(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP)
{
    int X, Y;
    uint8_t c;
    //uint32_t c32;
    uint16_t cw;
    char* start;
    uint16_t* pw;
    ogeColor32* pc32;

    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;

        pw  = (uint16_t*)(pDstData);
        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pw;
            for(X=0;X<iWidth;X++)
            {
                cw = *pw;

                c = div3[((cw&maskr16)>>11<<3) + ((cw&maskg16)>>5<<2) + ((cw&maskb16)<<3)];
                *pw = (c>>3 << 11) | (c>>2 << 5) | (c>>3);

                pw++;

            }
            pw = (uint16_t*)(start + iDstLineSize);

        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;

        pc32  = (ogeColor32*)(pDstData);
        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pc32;
            //pc32  = (ogeColor32*)(pDstData + iDstLineSize*Y);
            for(X=0;X<iWidth;X++)
            {
                c = div3[pc32->r + pc32->g + pc32->b];
                pc32->r = c;
                pc32->g = c;
                pc32->b = c;
                pc32++;

            }
            pc32 = (ogeColor32*)(start + iDstLineSize);

        }


    break;
    }
}


void OGE_FX_BltSquareWave(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP,
                double iXAmount, double iYAmount, double iZAmount)
{
    int X, Y, xx, yy;
    char* start;
    int* pc32;
    int* py;
    uint16_t* pyw;
    uint16_t* pw;


    for(int i=0; i < iWidth; i++)  wavex[i] = lround(sin(i / iXAmount) * iZAmount);
    for(int i=0; i < iHeight; i++) wavey[i] = lround(sin(i / iYAmount) * iZAmount);

    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        pw  = (uint16_t*)(pDstData);

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pw;

            yy = wavey[Y] + Y;

            if (yy >= 0 && yy < iHeight)
            {
                pyw  = (uint16_t*)(pSrcData + iSrcLineSize*yy);

                for(X=0;X<iWidth;X++)
                {
                    xx = wavex[X] + X;
                    if (xx >= 0 && xx < iWidth) *pw = *(pyw+xx);
                    pw++;
                }
            }

            pw = (uint16_t*)(start + iDstLineSize);
        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        pc32  = (int*)(pDstData);

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pc32;

            yy = wavey[Y] + Y;

            if (yy >= 0 && yy < iHeight)
            {
                py    = (int*)(pSrcData + iSrcLineSize*yy);

                for(X=0;X<iWidth;X++)
                {
                    xx = wavex[X] + X;
                    if (xx >= 0 && xx < iWidth) *pc32 = *(py+xx);
                    pc32++;
                }
            }

            pc32 = (int*)(start + iDstLineSize);
        }


    break;
    }
}

void OGE_FX_BltRoundWave(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iSrcColorKey,
                double iXAmount, double iYAmount, double iZAmount,
                int iXSteps, int iYSteps)
{
    int X, Y, xx, yy;
    char* start;
    int* pc32;
    int* py;
    uint16_t* pyw;
    uint16_t* pw;

    int iColor;

    if(iYSteps > 0)
        for(int i=0; i < iHeight; i++) wavex[i] = lround(sin((i+iYSteps) / iXAmount) * iZAmount);
    else
        for(int i=0; i < iHeight; i++) wavex[i] = lround(sin(i / iXAmount) * iZAmount);

    if(iXSteps > 0)
        for(int i=0; i < iWidth; i++)  wavey[i] = lround(sin((i+iXSteps) / iYAmount) * iZAmount);
    else
        for(int i=0; i < iWidth; i++)  wavey[i] = lround(sin(i / iYAmount) * iZAmount);

    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        pw  = (uint16_t*)(pDstData);

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pw;

            xx = wavex[Y];

            for(X=0;X<iWidth;X++)
            {
                yy = wavey[X] + Y;

                if (xx >= 0 && xx < iWidth
                 && yy >= 0 && yy < iHeight)
                {
                    pyw = (uint16_t*)(pSrcData + iSrcLineSize*yy);
                    iColor = *(pyw+xx);
                    if(iSrcColorKey == -1 || iColor != iSrcColorKey) *pw = iColor;
                }
                pw++;
                xx++;
            }

            pw = (uint16_t*)(start + iDstLineSize);
        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        pc32  = (int*)(pDstData);

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pc32;

            xx = wavex[Y];

            for(X=0;X<iWidth;X++)
            {
                yy = wavey[X] + Y;

                if (xx >= 0 && xx < iWidth
                 && yy >= 0 && yy < iHeight)
                {
                    py = (int*)(pSrcData + iSrcLineSize*yy);
                    iColor = *(py+xx);
                    if(iSrcColorKey == -1 || iColor != iSrcColorKey) *pc32 = iColor;

                }
                pc32++;
                xx++;
            }

            pc32 = (int*)(start + iDstLineSize);
        }


    break;
    }
}

void OGE_FX_SplitBlur(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    int n, S, E, w, X, Y;
    char* start;
    ogeColor32 c1, c2, c3, c4, pc;
    uint16_t *pw, *yyw1, *yyw2;
    ogeColor32 *yy1, *yy2;
    uint32_t *pc32;

    switch(iBPP)
    {
    case 16:

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pw  = (uint16_t*)(pDstData);

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pw;

            n = Y + iAmount;
            if (n > iHeight - 1) n = iHeight - 1;
            S = Y - iAmount;
            if (S < 0) S = 0;
            yyw1 = (uint16_t*)(pDstData + S * iDstLineSize);
            yyw2 = (uint16_t*)(pDstData + n * iDstLineSize);

            for(X=0;X<iWidth;X++)
            {
                E = X + iAmount;
                if (E > iWidth - 1) E = iWidth - 1;
                w = X - iAmount;
                if (w < 0) w = 0;

                c1 = cm16[yyw1[w]];
                c2 = cm16[yyw1[E]];
                c3 = cm16[yyw2[w]];
                c4 = cm16[yyw2[E]];

                *pw = bw[(c1.b + c2.b + c3.b + c4.b) >> 2]   |
                        gw[(c1.g + c2.g + c3.g + c4.g) >> 2] |
                        rw[(c1.r + c2.r + c3.r + c4.r) >> 2];

                pw++;
            }

            pw = (uint16_t*)(start + iDstLineSize);
        }


    break;
    case 32:

        pDstData += iDstY * iDstLineSize + iDstX * 4;

        pc32 = (uint32_t*)(pDstData);

        pc.a = 0; // not support alpha channel ....

        for(Y=0;Y<iHeight;Y++)
        {
            start = (char*)pc32;

            n = Y + iAmount;
            if (n > iHeight - 1) n = iHeight - 1;
            S = Y - iAmount;
            if (S < 0) S = 0;
            yy1 = (ogeColor32*)(pDstData + S * iDstLineSize);
            yy2 = (ogeColor32*)(pDstData + n * iDstLineSize);

            for(X=0;X<iWidth;X++)
            {
                E = X + iAmount;
                if (E > iWidth - 1) E = iWidth - 1;
                w = X - iAmount;
                if (w < 0) w = 0;

                pc.b = (yy1[w].b + yy1[E].b + yy2[w].b + yy2[E].b) >> 2;
                pc.g = (yy1[w].g + yy1[E].g + yy2[w].g + yy2[E].g) >> 2;
                pc.r = (yy1[w].r + yy1[E].r + yy2[w].r + yy2[E].r) >> 2;

                *pc32 = *((uint32_t*)(&pc));

                pc32++;
            }

            pc32 = (uint32_t*)(start + iDstLineSize);
        }


    break;
    }
}

void OGE_FX_GaussianBlur(uint8_t* pDstData, int iDstLineSize,
                          int iDstX, int iDstY,
                          int iWidth, int iHeight, int iBPP, int iAmount)
{
    for(int i=1; i<=iAmount; i++)
        OGE_FX_SplitBlur(pDstData, iDstLineSize, iDstX, iDstY, iWidth, iHeight, iBPP, i);
}

void OGE_FX_BltWithEdge(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iEdgeColor)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	int x,y;

	//int iPixel;
	//uint8_t iB, iG, iR;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16ColorSrc;
        uint16_t i16Color = iEdgeColor;

        y = 0;

        while(iHeight > 0)
        {
            x = 0;
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16ColorSrc = *((uint16_t*)pSrcData);
                if (i16ColorSrc != i16ColorKey) *((uint16_t*)pDstData) = i16ColorSrc;
                else
                {
                    if (x > 0 && *((uint16_t*)(pSrcData-2)) != i16ColorKey) *((uint16_t*)pDstData) = i16Color;
                    else if (y > 0 && *((uint16_t*)(pSrcData-iSrcLineSize)) != i16ColorKey) *((uint16_t*)pDstData) = i16Color;
                    else if (iLineWidth > 1 && *((uint16_t*)(pSrcData+2)) != i16ColorKey) *((uint16_t*)pDstData) = i16Color;
                    else if (iHeight > 1 && *((uint16_t*)(pSrcData+iSrcLineSize)) != i16ColorKey) *((uint16_t*)pDstData) = i16Color;
                }

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;
                x++;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;
            y++;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32ColorSrc;
        uint32_t i32Color = iEdgeColor;

        y = 0;

        while(iHeight > 0)
        {
            x = 0;
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {

                i32ColorSrc = *((uint32_t*)pSrcData);
                if (i32ColorSrc != i32ColorKey) *((uint32_t*)pDstData) = i32ColorSrc;
                else
                {
                    if (x > 0 && *((uint32_t*)(pSrcData-4)) != i32ColorKey) *((uint32_t*)pDstData) = i32Color;
                    else if (y > 0 && *((uint32_t*)(pSrcData-iSrcLineSize)) != i32ColorKey) *((uint32_t*)pDstData) = i32Color;
                    else if (iLineWidth > 1 && *((uint32_t*)(pSrcData+4)) != i32ColorKey) *((uint32_t*)pDstData) = i32Color;
                    else if (iHeight > 1 && *((uint32_t*)(pSrcData+iSrcLineSize)) != i32ColorKey) *((uint32_t*)pDstData) = i32Color;
                }

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;
                x++;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;
            y++;

        }

        break;
    }


	}

}

void OGE_FX_BltWithColor(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iColor, int iAlpha)
{

    int iSrcDataPad;
	int iDstDataPad;

	int iLineBytes;
	int iLineWidth;

	//int iPixel;
	uint8_t iB, iG, iR;

	//uint8_t iSmallAlpha = iAlpha >> 2;

	bool bNoColorKey = iSrcColorKey == -1;

	if(iAlpha > 256) iAlpha = 256;
	else if(iAlpha < 0) iAlpha = 0;

	iAlpha = 256 - iAlpha;

	switch (iBPP)
	{
	case 16:
    {
        iLineBytes = iWidth << 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
        uint16_t i16ColorSrc, i16ColorDst;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i16ColorSrc = *((uint16_t*)pSrcData);
                if (bNoColorKey || i16ColorSrc != i16ColorKey)
                {

                    i16ColorDst = iColor;

                    // result = (ALPHA * (src - dst)) / 256 + dst

                    iB = ((iAlpha * ((i16ColorSrc & 0x001f) - (i16ColorDst & 0x001f))) >> 8) +
                        (i16ColorDst & 0x001f);

                    iG = ((iAlpha * (((i16ColorSrc & 0x07e0) >> 5) - ((i16ColorDst & 0x07e0) >> 5))) >> 8) +
                        ((i16ColorDst & 0x07e0) >> 5);

                    iR = ((iAlpha * (((i16ColorSrc & 0xf800) >> 11) - ((i16ColorDst & 0xf800) >> 11))) >> 8) +
                        ((i16ColorDst & 0xf800) >> 11);

                    *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

                }

                pSrcData += 2;
                pDstData += 2;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }

    case 32:
    {
        iLineBytes = iWidth << 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iLineBytes;
        iDstDataPad = iDstLineSize - iLineBytes;

        uint32_t i32ColorKey = iSrcColorKey;
        uint32_t i32ColorSrc, i32ColorDst;

        while(iHeight > 0)
        {
            iLineWidth = iWidth;
            while(iLineWidth > 0)
            {
                i32ColorSrc = *((uint32_t*)pSrcData);
                if(bNoColorKey || i32ColorSrc != i32ColorKey)
                {

                    i32ColorDst = iColor;

                    iB = ((iAlpha * (((i32ColorSrc & bluemask) >> blueoffset) - ((i32ColorDst & bluemask) >> blueoffset))) >> 8) +
                        ((i32ColorDst & bluemask) >> blueoffset);

                    iG = ((iAlpha * (((i32ColorSrc & greenmask) >> greenoffset) - ((i32ColorDst & greenmask) >> greenoffset))) >> 8) +
                        ((i32ColorDst & greenmask) >> greenoffset);

                    iR = ((iAlpha * (((i32ColorSrc & redmask) >> redoffset) - ((i32ColorDst & redmask) >> redoffset))) >> 8) +
                        ((i32ColorDst & redmask) >> redoffset);

                    *((int*)pDstData) = (iR << redoffset) | (iG << greenoffset) | (iB << blueoffset);

                }

                pSrcData += 4;
                pDstData += 4;

                iLineWidth--;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }

        break;
    }


	}

}

#endif //__FX_WITH_MMX__
