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

#ifdef __FX_WITH_MMX__

struct ogeColor32
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

union CogeColor32
{
    uint32_t color;
    ogeColor32 argb;
};


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
    /*
    int iResult = 1;

    asm(
            "movl   $1,  %%eax           \n\t"
            "cpuid                       \n\t"  // when EAX=1, CPUID will send CPU version info to EAX,
                                                // and stores the properties in EDX
            "testl  $0x800000, %%edx     \n\t"
            "jnz   __support_mmx_        \n\t"
            "movl   $0,  %0              \n\t"
        "__support_mmx_:                 \n\t"

            : "=m"(iResult)
            : // no input
            : "eax", "edx", "memory"
    );

    return iResult;
    */


    // the following code is a copy of the classic c function "mm_support(void)" ...

    /* Returns 1 if MMX instructions are supported,
	   3 if Cyrix MMX and Extended MMX instructions are supported
	   5 if AMD MMX and 3DNow! instructions are supported
	   0 if hardware does not support any of these
	*/
	register int rval = 0;

	__asm__ __volatile__ (
		/* See if CPUID instruction is supported ... */
		/* ... Get copies of EFLAGS into eax and ecx */
		"pushf\n\t"
		"popl %%eax\n\t"
		"movl %%eax, %%ecx\n\t"

		/* ... Toggle the ID bit in one copy and store */
		/*     to the EFLAGS reg */
		"xorl $0x200000, %%eax\n\t"
		"push %%eax\n\t"
		"popf\n\t"

		/* ... Get the (hopefully modified) EFLAGS */
		"pushf\n\t"
		"popl %%eax\n\t"

		/* ... Compare and test result */
		"xorl %%eax, %%ecx\n\t"
		"testl $0x200000, %%ecx\n\t"
		"jz NotSupported1\n\t"		/* Nothing supported */


		/* Get standard CPUID information, and
		       go to a specific vendor section */
		"movl $0, %%eax\n\t"
		"cpuid\n\t"

		/* Check for Intel */
		"cmpl $0x756e6547, %%ebx\n\t"
		"jne TryAMD\n\t"
		"cmpl $0x49656e69, %%edx\n\t"
		"jne TryAMD\n\t"
		"cmpl $0x6c65746e, %%ecx\n"
		"jne TryAMD\n\t"
		"jmp Intel\n\t"

		/* Check for AMD */
		"\nTryAMD:\n\t"
		"cmpl $0x68747541, %%ebx\n\t"
		"jne TryCyrix\n\t"
		"cmpl $0x69746e65, %%edx\n\t"
		"jne TryCyrix\n\t"
		"cmpl $0x444d4163, %%ecx\n"
		"jne TryCyrix\n\t"
		"jmp AMD\n\t"

		/* Check for Cyrix */
		"\nTryCyrix:\n\t"
		"cmpl $0x69727943, %%ebx\n\t"
		"jne NotSupported2\n\t"
		"cmpl $0x736e4978, %%edx\n\t"
		"jne NotSupported3\n\t"
		"cmpl $0x64616574, %%ecx\n\t"
		"jne NotSupported4\n\t"
		/* Drop through to Cyrix... */


		/* Cyrix Section */
		/* See if extended CPUID is supported */
		"movl $0x80000000, %%eax\n\t"
		"cpuid\n\t"
		"cmpl $0x80000000, %%eax\n\t"
		"jl MMXtest\n\t"	/* Try standard CPUID instead */

		/* Extended CPUID supported, so get extended features */
		"movl $0x80000001, %%eax\n\t"
		"cpuid\n\t"
		"testl $0x00800000, %%eax\n\t"	/* Test for MMX */
		"jz NotSupported5\n\t"		/* MMX not supported */
		"testl $0x01000000, %%eax\n\t"	/* Test for Ext'd MMX */
		"jnz EMMXSupported\n\t"
		"movl $1, %0 \n\t"		/* MMX Supported */
		"jmp Return\n\n"
		"EMMXSupported:\n\t"
		"movl $3, %0 \n\t"		/* EMMX and MMX Supported */
		"jmp Return\n\t"


		/* AMD Section */
		"AMD:\n\t"

		/* See if extended CPUID is supported */
		"movl $0x80000000, %%eax\n\t"
		"cpuid\n\t"
		"cmpl $0x80000000, %%eax\n\t"
		"jl MMXtest\n\t"	/* Try standard CPUID instead */

		/* Extended CPUID supported, so get extended features */
		"movl $0x80000001, %%eax\n\t"
		"cpuid\n\t"
		"testl $0x00800000, %%edx\n\t"	/* Test for MMX */
		"jz NotSupported6\n\t"		/* MMX not supported */
		"testl $0x80000000, %%edx\n\t"	/* Test for 3DNow! */
		"jnz ThreeDNowSupported\n\t"
		"movl $1, %0 \n\t"		/* MMX Supported */
		"jmp Return\n\n"
		"ThreeDNowSupported:\n\t"
		"movl $5, %0 \n\t"		/* 3DNow! and MMX Supported */
		"jmp Return\n\t"


		/* Intel Section */
		"Intel:\n\t"

		/* Check for MMX */
		"MMXtest:\n\t"
		"movl $1, %%eax\n\t"
		"cpuid\n\t"
		"testl $0x00800000, %%edx\n\t"	/* Test for MMX */
		"jz NotSupported7\n\t"		/* MMX Not supported */
		"movl $1, %0 \n\t"		/* MMX Supported */
		"jmp Return\n\t"

		/* Nothing supported */
		"\nNotSupported1:\n\t"
		"#movl $101, %0:\n\n\t"
		"\nNotSupported2:\n\t"
		"#movl $102, %0:\n\n\t"
		"\nNotSupported3:\n\t"
		"#movl $103, %0:\n\n\t"
		"\nNotSupported4:\n\t"
		"#movl $104, %0:\n\n\t"
		"\nNotSupported5:\n\t"
		"#movl $105, %0:\n\n\t"
		"\nNotSupported6:\n\t"
		"#movl $106, %0:\n\n\t"
		"\nNotSupported7:\n\t"
		"#movl $107, %0:\n\n\t"
		"movl $0, %0 \n\t"

		"Return:\n\t"
		: "=r"(rval)
		: /* no input */
		: "eax", "ebx", "ecx", "edx"
	);

	/* Return */
	mmxflag = rval;
	return(rval);

}

int OGE_FX_Saturate(int i, int iMax)
{
    /*
    asm(".intel_syntax noprefix\n");
    asm("mov eax, i      \n");
    asm("cmp eax, 0      \n");
    asm("js  __min       \n");
    asm("cmp eax, iMax   \n");
    asm("ja  __max       \n");
    asm("jmp __end       \n");
    asm("__max:          \n");
    asm("mov eax, iMax   \n");
    asm("jmp __end       \n");
    asm("__min:          \n");
    asm("mov eax, 0      \n");
    asm("__end:          \n");
    */

    //int iRsl = i;

    //asm(".intel_syntax noprefix\n");

    /*
    asm("                \
        mov %0, %%eax;   \
        cmp $0, %%eax;   \
        js  1f;          \
        cmp %1, %%eax;   \
        ja  2f;          \
        jmp 3f;          \
        2:               \
        mov %1, %%eax;   \
        jmp 3f;          \
        1:               \
        mov $0, %%eax;   \
        3:               "
        //mov %%eax, %0;   "
        ://"=m"(iRsl)
        :"m"(i), "m"(iMax)
        :"%eax"
        );
    */

    //return iRsl;

    //if (i < 0) return 0;
    //else if(i>iMax) return iMax;
    //else return i;

    register int iRsl = 0;

    asm (

        "cmp $0, %1     \n\t"
        "js  1f         \n\t"
        "cmp %2, %1     \n\t"
        "ja  2f         \n\t"
        "jmp 3f         \n\t"
        "2:             \n\t"
        "mov %2, %1     \n\t"
        "jmp 3f         \n\t"
        "1:             \n\t"
        "mov $0, %1     \n\t"
        "3:             \n\t"
        "mov %1, %0     \n\t"
        :"=r"(iRsl)
        :"r"(i), "r"(iMax)
        :"memory"

    );

    return iRsl;

}

void OGE_FX_SetPixel16(uint8_t* pBase, int iDelta, int iX, int iY, uint16_t iColor)
{
    __asm__ __volatile__ (

        "movl    %2, %%eax         \n\t"
		"shll    $1, %%eax         \n\t"  // eax := 2*iX
		"addl    %%eax, %0         \n\t"  // pBase := pBase + 2*iX

		"movl    %3, %%eax         \n\t"  // eax := iY
		"mull    %4                \n\t"  // edx eax := iY * iDelta

		"addl    %%eax, %0         \n\t"  // pBase := pBase + 2*iX + iY * iDelta

		"movl    %0, %%eax         \n\t"
		"movw    %1, (%%eax)       \n\t"  // PWord(pBase + iY * iDelta + 2*iX)^ = iColor

		: // no output
		: "r"(pBase), "r"(iColor), "m"(iX), "m"(iY), "m"(iDelta)
		: "%eax", "%edx", "memory"
	);
}
void OGE_FX_SetPixel32(uint8_t* pBase, int iDelta, int iX, int iY, uint32_t iColor)
{
    __asm__ __volatile__ (

        "movl    %2, %%eax         \n\t"
		"shll    $2, %%eax         \n\t"  // eax := 4*iX
		"addl    %%eax, %0         \n\t"  // pBase := pBase + 4*iX

		"movl    %3, %%eax         \n\t"  // eax := iY
		"mull    %4                \n\t"  // edx eax := iY * iDelta

		"addl    %%eax, %0         \n\t"  // pBase := pBase + 4*iX + iY * iDelta

		"movl    %0, %%eax         \n\t"
		"movl    %1, (%%eax)       \n\t"  // PDWord(pBase + iY * iDelta + 4*iX)^ = iColor

		: // no output
		: "r"(pBase), "r"(iColor), "m"(iX), "m"(iY), "m"(iDelta)
		: "%eax", "%edx", "memory"
	);
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
    if(mmxflag <= 0) return;

    int iSrcDataPad;
	int iDstDataPad;

	switch (iBPP)
	{
    case 8:
      pDstData += iDstY * iDstLineSize + iDstX;
      pSrcData += iSrcY * iSrcLineSize + iSrcX;
	  break;
	case 15:
	case 16:
      iWidth <<= 1;
      pDstData += iDstY * iDstLineSize + iDstX * 2;
      pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;
	  break;

    case 24:
      iWidth = iWidth * 3;
      pDstData += iDstY * iDstLineSize + iDstX * 3;
      pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;
	  break;

    case 32:
      iWidth <<= 2;
	  pDstData += iDstY * iDstLineSize + iDstX * 4;
      pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
	  break;

	}

	iSrcDataPad = iSrcLineSize - iWidth;
	iDstDataPad = iDstLineSize - iWidth;


	// asm code
    __asm__ __volatile__ (

                // %0 == iWidth,       %1 == iHeight,
                // %2 == pSrcData,     %3 == pDstData,
                // %4 == iSrcDataPad   %5 == iDstDataPad

                "push    %%eax                   \n\t"
                "push    %%ebx                   \n\t"
                "push    %%edx                   \n\t"
                "push    %%ecx                   \n\t"
                "push    %%esi                   \n\t"
                "push    %%edi                   \n\t"

                "movl    %0, %%edx               \n\t"
                "movl    %1, %%ebx               \n\t"

                "movl    %2, %%esi               \n\t"
                "movl    %3, %%edi               \n\t"


        "__do_next_line:                         \n\t"

                "movl    %%edx, %%ecx            \n\t"

                "cmpl    $16,   %%ecx            \n\t"
                "jb      __skip_16               \n\t"

        "__do_copy_16:                           \n\t"

                "movq    (%%esi),   %%mm0        \n\t"
                "movq    8(%%esi),  %%mm1        \n\t"
                "movq    %%mm0,     (%%edi)      \n\t"
                "movq    %%mm1,     8(%%edi)     \n\t"
                "addl    $16,       %%edi        \n\t"
                "addl    $16,       %%esi        \n\t"

                "subl    $16,       %%ecx        \n\t"
                "cmpl     $16,       %%ecx        \n\t"
                "jae     __do_copy_16            \n\t"

        "__skip_16:                              \n\t"

                "cmpl     $8,        %%ecx        \n\t"
                "jb      __skip_8                \n\t"

        "__do_copy_8:                            \n\t"

                "movq    (%%esi),   %%mm0        \n\t"
                "movq    %%mm0,     (%%edi)      \n\t"
                "addl    $8,        %%edi        \n\t"
                "addl    $8,        %%esi        \n\t"

                "subl    $8,        %%ecx        \n\t"
                "cmpl     $8,       %%ecx        \n\t"
                "jae     __do_copy_8             \n\t"

        "__skip_8:                               \n\t"

                "cmpl     $4,       %%ecx        \n\t"
                "jb      __skip_4                \n\t"

        "__do_copy_4:                            \n\t"

                "movl    (%%esi),   %%eax        \n\t"
                "movl    %%eax,     (%%edi)      \n\t"
                "addl    $4,        %%edi        \n\t"
                "addl    $4,        %%esi        \n\t"

                "subl    $4,        %%ecx        \n\t"
                "cmpl    $4,        %%ecx        \n\t"
                "jae     __do_copy_4             \n\t"

        "__skip_4:                               \n\t"

                "cmpl     $2,       %%ecx        \n\t"
                "jb      __skip_2                \n\t"

        "__do_copy_2:                            \n\t"

                "movw    (%%esi),   %%ax         \n\t"
                "movw    %%ax,      (%%edi)      \n\t"
                "addl    $2,        %%edi        \n\t"
                "addl    $2,        %%esi        \n\t"

                "subl    $2,        %%ecx        \n\t"
                "cmp     $2,        %%ecx        \n\t"
                "jae     __do_copy_2             \n\t"

        "__skip_2:                               \n\t"

                "cmpl     $1,       %%ecx        \n\t"
                "jb      __skip_1                \n\t"

        "__do_copy_1:                            \n\t"

                "movb    (%%esi),   %%al         \n\t"
                "movb    %%al,      (%%edi)      \n\t"
                "inc     %%edi                   \n\t"
                "inc     %%esi                   \n\t"

                "dec     %%ecx                   \n\t"
                "jnz     __do_copy_1             \n\t"


        "__skip_1:                               \n\t"

                "add     %4, %%esi               \n\t"
                "add     %5, %%edi               \n\t"

                "dec     %%ebx                   \n\t"
                "jnz     __do_next_line          \n\t"



                //__fin_all:

                "pop     %%edi                   \n\t"
                "pop     %%esi                   \n\t"
                "pop     %%ecx                   \n\t"
                "pop     %%edx                   \n\t"
                "pop     %%ebx                   \n\t"
                "pop     %%eax                   \n\t"

                "emms                            \n\t"

                // %0 == iWidth,       %1 == iHeight,
                // %2 == pSrcData,     %3 == pDstData,
                // %4 == iSrcDataPad   %5 == iDstDataPad

                : // no output
                : "m"(iWidth), "m"(iHeight),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iSrcDataPad), "m"(iDstDataPad)
                : "memory"

    );     // end of asm code

}

void OGE_FX_Blt(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP)
{
    if(mmxflag <= 0) return;

    int iSrcDataPad;
	int iDstDataPad;

	uint64_t i64ColorKey = iSrcColorKey;
	uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;
	uint8_t  i8ColorKey  = iSrcColorKey & 0x000000ff;

	int iColor = 0;
    int iSize = sizeof(uint8_t) * 3;

	uint32_t iMask = 0x00ffffff;

	switch (iBPP)
	{
    case 8:
        pDstData += iDstY * iDstLineSize + iDstX;
        pSrcData += iSrcY * iSrcLineSize + iSrcX;
        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        i64ColorKey = (i64ColorKey << 56) | (i64ColorKey << 48) | (i64ColorKey << 40) | (i64ColorKey << 32) |
                      (i64ColorKey << 24) | (i64ColorKey << 16) | (i64ColorKey << 8)  |  i64ColorKey;

        // asm code
    __asm__ __volatile__ (

                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == i8ColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                //"push    %%ecx                   \n\t"

                "cmpl    $0, %0                  \n\t"
				"jbe     __end_blt_8             \n\t"

                "movl    %1, %%esi               \n\t"
                "movl    %2, %%edi               \n\t"


        "__do_next_line_blt_8:                   \n\t"

                "mov     %3, %%ecx               \n\t"

                "cmpl    $8,   %%ecx             \n\t"
                "jb      __skip_mmx_blt_8        \n\t"

        "__do_mmx_blt_8:                         \n\t"

                "movq    (%%esi), %%mm0          \n\t"  // mm0 = ccss
                "movq    (%%edi), %%mm1          \n\t"  // mm1 = dddd

                // handle color key form src
                "movq    %%mm0, %%mm2            \n\t"  // mm2 = ccss
                "pcmpeqb %4,    %%mm2            \n\t"  // mm2 = 1100
                "movq    %%mm2, %%mm3            \n\t"  // mm3 = 1100
                "pandn   %%mm0, %%mm3            \n\t"  // mm3 = (~1100) & ccss = 00ss
                "pand    %%mm2, %%mm1            \n\t"  // mm1 = dddd & 1100    = dd00
                "por     %%mm3, %%mm1            \n\t"  // mm1 = dd00 & 00ss    = ddss
                // fin
                "movq    %%mm1, (%%edi)          \n\t"

                "addl    $8, %%edi               \n\t"
                "addl    $8, %%esi               \n\t"

                "subl    $8, %%ecx               \n\t"
                "cmpl    $8, %%ecx               \n\t"
                "jae     __do_mmx_blt_8          \n\t"

        "__skip_mmx_blt_8:                       \n\t"

                "cmpl    $1, %%ecx               \n\t"
                "jb      __skip_blt_8            \n\t"

        "__do_blt_8:                             \n\t"

                "movb    (%%esi), %%al           \n\t"
                "cmpb     %5, %%al               \n\t"
                "je      __next_blt_8            \n\t"
                "movb     %%al, (%%edi)          \n\t"

        "__next_blt_8:                           \n\t"

                "incl     %%edi                  \n\t"
                "incl     %%esi                  \n\t"

                "decl     %%ecx                  \n\t"
                "jnz     __do_blt_8              \n\t"

       "__skip_blt_8:                            \n\t"

                "addl     %6, %%esi              \n\t"
                "addl     %7, %%edi              \n\t"

                "decl     %0                     \n\t"
                "jnz     __do_next_line_blt_8    \n\t"

                //"pop     %%ecx                   \n\t"

                //"pop     %%edi                   \n\t"
                //"pop     %%esi                   \n\t"

        "__end_blt_8:                            \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == i8ColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad
                : "m"(iHeight), "r"(pSrcData), "r"(pDstData), "m"(iWidth),
                  "m"(i64ColorKey), "m"(i8ColorKey), "m"(iSrcDataPad), "m"(iDstDataPad)
                : "%esi", "%edi", "%eax", "%ecx", "memory"


        );

    break;
	case 15:
	case 16:

        iWidth <<= 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        i64ColorKey = (i64ColorKey << 48) | (i64ColorKey << 32) | (i64ColorKey << 16) | i64ColorKey;

        // asm code
    __asm__ __volatile__ (

                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == i16ColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                "cmpl    $0, %0                  \n\t"
				"jbe     __end_blt_16            \n\t"

                "movl    %1, %%esi               \n\t"
                "movl    %2, %%edi               \n\t"


        "__do_next_line_blt_16:                  \n\t"


                "mov     %3, %%ecx               \n\t"

                "cmpl    $8, %%ecx               \n\t"
                "jb      __skip_mmx_blt_16       \n\t"

        "__do_mmx_blt_16:                        \n\t"

                "movq    (%%esi), %%mm0          \n\t"  // mm0 = ccss
                "movq    (%%edi), %%mm1          \n\t"  // mm1 = dddd

                // handle color key form src
                "movq    %%mm0, %%mm2            \n\t"  // mm2 = ccss
                "pcmpeqw %4,    %%mm2            \n\t"  // mm2 = 1100
                "movq    %%mm2, %%mm3            \n\t"  // mm3 = 1100
                "pandn   %%mm0, %%mm3            \n\t"  // mm3 = (~1100) & ccss = 00ss
                "pand    %%mm2, %%mm1            \n\t"  // mm1 = dddd & 1100    = dd00
                "por     %%mm3, %%mm1            \n\t"  // mm1 = dd00 & 00ss    = ddss
                // fin
                "movq    %%mm1, (%%edi)          \n\t"

                "addl    $8, %%edi               \n\t"
                "addl    $8, %%esi               \n\t"

                "subl    $8, %%ecx               \n\t"
                "cmpl    $8, %%ecx               \n\t"
                "jae     __do_mmx_blt_16         \n\t"

        "__skip_mmx_blt_16:                      \n\t"

                "cmpl    $2, %%ecx               \n\t"
                "jb      __skip_blt_16           \n\t"

        "__do_blt_16:                            \n\t"

                "movw    (%%esi), %%ax           \n\t"
                "cmpw     %5, %%ax               \n\t"
                "je      __next_blt_16           \n\t"
                "movw     %%ax, (%%edi)          \n\t"

        "__next_blt_16:                          \n\t"

                "addl    $2, %%edi               \n\t"
                "addl    $2, %%esi               \n\t"

                "subl    $2, %%ecx               \n\t"
                "jnz     __do_blt_16             \n\t"

       "__skip_blt_16:                           \n\t"

                "addl    %6, %%esi               \n\t"
                "addl    %7, %%edi               \n\t"

                "decl    %0                      \n\t"
                "jnz     __do_next_line_blt_16   \n\t"

                //"pop     %%edi                   \n\t"
                //"pop     %%esi                   \n\t"

        "__end_blt_16:                           \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == i16ColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad
                : "m"(iHeight), "r"(pSrcData), "r"(pDstData), "m"(iWidth),
                  "m"(i64ColorKey), "m"(i16ColorKey), "m"(iSrcDataPad), "m"(iDstDataPad)
                : "%esi", "%edi", "%eax", "%ecx", "memory"


        );

    break;

    case 24:

        iWidth = iWidth * 3;
        pDstData += iDstY * iDstLineSize + iDstX * 3;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        if(iHeight <= 0) break;

        iSrcColorKey = iSrcColorKey & iMask;

        while(iHeight > 0)
        {
            for(int i=iWidth; i>=3; i-=3)
            {
                iColor = (*(int*)pSrcData) & iMask;
                if(iColor != iSrcColorKey) memcpy(pDstData, pSrcData, iSize);

                pSrcData += 3;
                pDstData += 3;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }


    break;

    case 32:

        iWidth <<= 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        i64ColorKey = (i64ColorKey << 32) | i64ColorKey;

        // asm code
    __asm__ __volatile__ (

                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == iSrcColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                "cmpl    $0, %0                  \n\t"
				"jbe     __end_blt_32            \n\t"

                "movl    %1, %%esi               \n\t"
                "movl    %2, %%edi               \n\t"


        "__do_next_line_blt_32:                  \n\t"

                "mov     %3, %%ecx               \n\t"

                "cmpl    $8, %%ecx               \n\t"
                "jb      __skip_mmx_blt_32       \n\t"

        "__do_mmx_blt_32:                        \n\t"

                "movq    (%%esi), %%mm0          \n\t"  // mm0 = ccss
                "movq    (%%edi), %%mm1          \n\t"  // mm1 = dddd

                // handle color key form src
                "movq    %%mm0, %%mm2            \n\t"  // mm2 = ccss
                "pcmpeqd %4,    %%mm2            \n\t"  // mm2 = 1100
                "movq    %%mm2, %%mm3            \n\t"  // mm3 = 1100
                "pandn   %%mm0, %%mm3            \n\t"  // mm3 = (~1100) & ccss = 00ss
                "pand    %%mm2, %%mm1            \n\t"  // mm1 = dddd & 1100    = dd00
                "por     %%mm3, %%mm1            \n\t"  // mm1 = dd00 & 00ss    = ddss
                // fin
                "movq    %%mm1, (%%edi)          \n\t"

                "addl    $8, %%edi               \n\t"
                "addl    $8, %%esi               \n\t"

                "subl    $8, %%ecx               \n\t"
                "cmpl    $8, %%ecx               \n\t"
                "jae     __do_mmx_blt_32         \n\t"

        "__skip_mmx_blt_32:                      \n\t"

                "cmpl    $4, %%ecx               \n\t"
                "jb      __skip_blt_32           \n\t"

        "__do_blt_32:                            \n\t"

                "movl    (%%esi), %%eax          \n\t"
                "cmpl     %5, %%eax              \n\t"
                "je      __next_blt_32           \n\t"
                "movl     %%eax, (%%edi)         \n\t"

        "__next_blt_32:                          \n\t"

                "addl    $4, %%edi               \n\t"
                "addl    $4, %%esi               \n\t"

                "subl    $4, %%ecx               \n\t"
                "jnz     __do_blt_32             \n\t"

       "__skip_blt_32:                           \n\t"

                "addl    %6, %%esi               \n\t"
                "addl    %7, %%edi               \n\t"

                "decl    %0                      \n\t"
                "jnz     __do_next_line_blt_32   \n\t"

                //"pop     %%edi                   \n\t"
                //"pop     %%esi                   \n\t"

        "__end_blt_32:                           \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64ColorKey   %5 == iSrcColorKey
                // %6 == iSrcDataPad   %7 == iDstDataPad
                : "m"(iHeight), "r"(pSrcData), "r"(pDstData), "m"(iWidth),
                  "m"(i64ColorKey), "m"(iSrcColorKey), "m"(iSrcDataPad), "m"(iDstDataPad)
                : "%esi", "%edi", "%eax", "%ecx", "memory"


        );

    break;

	}



}


void OGE_FX_SubLight(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iDstDataPad, iRealAmount, iPixel;
	uint8_t iB, iG, iR, i8Amount;

	uint16_t i16Amount;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X, i64Mask, i64Amount;

	//uint64_t i64Amount;

	iRealAmount = iAmount;
	i8Amount    = abs(iAmount);
	i16Amount   = i8Amount;
	i64Amount   = i8Amount;

	i64Mask = 0xff;

	switch (iBPP)
	{

    case 8:
    case 24:
    case 32:

        if(iBPP == 8)
        {
            pDstData += iDstY * iDstLineSize + iDstX;
        }
        else if(iBPP == 24)
        {
            iWidth = iWidth * 3;
            pDstData += iDstY * iDstLineSize + iDstX * 3;
        }
        else
        {
            iWidth <<= 2;
            pDstData += iDstY * iDstLineSize + iDstX * 4;
        }
        //pDstData += iDstY * iDstLineSize + iDstX;
        iDstDataPad = iDstLineSize - iWidth;

        i64Amount   = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;
        i64Mask     = (i64Mask << 48)   | (i64Mask << 32)   | (i64Mask << 16)   | i64Mask;

        //i64Amount = (i64Amount << 56) | (i64Amount << 48) | (i64Amount << 40) | (i64Amount << 32) |
        //              (i64Amount << 24) | (i64Amount << 16) | (i64Amount << 8)  |  i64Amount;


        __asm__ __volatile__ (

                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                //"push    %%ecx                   \n\t"

                "movq    %0,      %%mm5          \n\t"
                "movq    %1,      %%mm6          \n\t"
                "pxor    %%mm7,   %%mm7          \n\t"

                "xorl    %%eax,   %%eax          \n\t"
                "xorl    %%edx,   %%edx          \n\t"

                "movb    %2,      %%dh           \n\t"

                "cmpl    $0,      %3             \n\t"
				"jbe     __end_sub_light_8            \n\t"

                "movl    %4,      %%edi          \n\t"


        "__do_next_line_sub_light_8:                \n\t"

                "movl    %5,      %%ecx          \n\t"

                "cmpl    $8,      %%ecx          \n\t"
                "jb      __skip_mmx_sub_light_8     \n\t"

        "__do_mmx_sub_light_8:                      \n\t"


                "movq    (%%edi), %%mm1          \n\t" // dst
                //"movq    %%mm1,   %%mm3          \n\t" // make a copy
                //"movq    %%mm1,   %%mm2          \n\t" // make a copy
                //"punpcklbw %%mm7, %%mm1          \n\t" // mm1 = unpacked low half of dst
                //"punpckhbw %%mm7, %%mm2          \n\t" // mm2 = unpacked high half of dst
                //"pmullw  %%mm6,   %%mm1          \n\t" // amount * (dst)
                //"pmullw  %%mm6,   %%mm2          \n\t" // amount * (dst)
                //"psrlw   $8,      %%mm1          \n\t" // amount * (dst) / 256
                //"psrlw   $8,      %%mm2          \n\t" // amount * (dst) / 256
                //"packuswb  %%mm2, %%mm1          \n\t" // combine with unsigned saturation
                //"psubusb %%mm1,   %%mm3          \n\t" // dst - amount * (dst) / 256

                "psubusb %%mm6,   %%mm1          \n\t"

                // fin
                "movq    %%mm1,   (%%edi)        \n\t"

                "addl    $8,      %%edi          \n\t"

                "subl    $8,      %%ecx          \n\t"
                "cmpl    $8,      %%ecx          \n\t"
                "jae     __do_mmx_sub_light_8       \n\t"

        "__skip_mmx_sub_light_8:                    \n\t"

                "cmpl    $1,      %%ecx          \n\t"
                "jb      __skip_sub_light_8         \n\t"

        "__do_sub_light_8:                          \n\t"

                "movb    (%%edi), %%al           \n\t"
                //"movb    %%al,    %%dl           \n\t"

                //"mulb    %%dh                    \n\t"
                //"shrw    $8,      %%ax           \n\t"
                //"subb    %%al,    %%dl           \n\t"
                "subb    %%dh,    %%al           \n\t"
                "jc       __sub_light_min_8         \n\t"

                "movb     %%al, (%%edi)          \n\t"
                "jmp      __next_sub_light_8        \n\t"

        "__sub_light_min_8:                         \n\t"

                "movb     $0, (%%edi)         \n\t"

        "__next_sub_light_8:                        \n\t"

                "incl     %%edi                  \n\t"

                "decl     %%ecx                  \n\t"
                "jnz     __do_sub_light_8           \n\t"

       "__skip_sub_light_8:                         \n\t"

                "addl     %6, %%edi              \n\t"

                "decl     %3                     \n\t"
                "jnz     __do_next_line_sub_light_8 \n\t"


        "__end_sub_light_8:                         \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad
                : "m"(i64Mask), "m"(i64Amount), "m"(i8Amount), "m"(iHeight),
                  "m"(pDstData), "m"(iWidth), "m"(iDstDataPad)
                : "%eax", "%edx", "%edi", "%ecx", "memory"


        );

    break;


	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;
      i64X         = 0x003f003f003f003fLL;    // for green

      //i8Amount = i64Amount  << 5 >> 8;
      //i16Amount = i64Amount << 6 >> 8;

      i64X = i64Amount >> 2;
      //i64X = i64Amount << 1;
      i16Amount = i64X;

      i64Amount = i64Amount  >> 3;
      i8Amount = i64Amount;

      i64Amount    = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;
      i64X    = (i64X << 48) | (i64X << 32) | (i64X << 16) | i64X;

      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iDstDataPad = iDstLineSize - (iWidth << 1);

      iRemain = iWidth & 3;
      iWidth  = (iWidth & (~3)) >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,       %1 == pDstData,
	      // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	      // %5 == iRealAmount,  %6 == i64Amount,      %7 == i64X

    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"
                    "movl     %0,      %%ecx   \n\t"  // %0 == iWidth
                    "cmpl     $0,      %%ecx   \n\t"
                    "jz      __skip_sub_light_16  \n\t"  // if this row is less than 4 then jump to next

                    "movq     %6,      %%mm6   \n\t"
                    "movq     %7,      %%mm7   \n\t"

                    "pushl    %%esi            \n\t"
                    "movl     %1, %%esi        \n\t"  // %1 == pDstData

                    "movl     %5, %%ebx        \n\t"  // %1 == pDstData

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_sub_light_16:                       \n\t"

                    "movq    (%%esi), %%mm0   \n\t"
                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %2,      %%mm1   \n\t"  // %2 == i64MaskBlue,  put blue  to %%mm1
                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"  // %3 == i64MaskGreen, put green to %%mm2
                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"  // %4 == i64MaskRed,   put red   to %%mm3
                    //"cmpl    $0,      %%ebx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    //"js      __sub_light_16        \n\t"



        "__sub_light_16:                           \n\t"

                    // handle blue                   // result := dst - amount * dst / 256
                    //"movq    %%mm1, %%mm7     \n\t"  // backup it
                    //"pmullw  %6,    %%mm7     \n\t"  // amount * dst

                    //"psrlw   $8,    %%mm7     \n\t"  // amount * dst / 256
                    //"psubusw %%mm7, %%mm1     \n\t"  // dst - amount * dst / 256

                    "psubusw %%mm6, %%mm1     \n\t"

                    // handle green
                    "psrlw   $5,    %%mm2     \n\t"
                    //"movq    %%mm2, %%mm7     \n\t"
                    //"pmullw  %6,    %%mm7     \n\t"

                    //"psrlw   $8,    %%mm7     \n\t"
                    //"psubusw %%mm7, %%mm2     \n\t"

                    "psubusw %%mm7, %%mm2     \n\t"

                    "psllw   $5,    %%mm2     \n\t"

                    // handle red
                    "psrlw   $11,   %%mm3     \n\t"
                    //"movq    %%mm3, %%mm7     \n\t"
                    //"pmullw  %6,    %%mm7     \n\t"

                    //"psrlw   $8,    %%mm7     \n\t"
                    //"psubusw %%mm6, %%mm3     \n\t"

                    "psubusw %%mm6, %%mm3     \n\t"

                    "psllw   $11,   %%mm3     \n\t"

        "__next_sub_light_16:                     \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    "movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi     \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx             \n\t"
                    "jnz	__do_sub_light_16     \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %1         \n\t"
                    "popl	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_sub_light_16:                     \n\t"

                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                    // %0 == iWidth,       %1 == pDstData,
	                // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	                // %5 == iRealAmount,  %6 == i64Amount, %7 == i64X
                    : "m"(iWidth), "m"(pDstData), "m"(i64MaskBlue), "m"(i64MaskGreen),
                      "m"(i64MaskRed), "m"(iRealAmount), "m"(i64Amount), "m"(i64X)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((uint16_t*)pDstData);
			iB = iPixel & 0x001f;
			iG = (iPixel >> 5) &  0x003f;
			iR = (iPixel >> 11) & 0x001f;

            iB = OGE_FX_Saturate(iB - i8Amount, 31);
            iG = OGE_FX_Saturate(iG - i16Amount, 63);
            iR = OGE_FX_Saturate(iR - i8Amount, 31);

			*((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

			pDstData += 2;

		}

         pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;

	}

}

void OGE_FX_Lightness(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP, int iAmount)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iDstDataPad, iRealAmount, iPixel;
	uint8_t iB, iG, iR, i8Amount;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X, i64Mask, i64Amount;

	//uint64_t i64Amount;

	iRealAmount = iAmount;
	i8Amount    = abs(iAmount);
	i64Amount   = i8Amount;

	i64Mask = 0xff;

/*
	switch (iBPP)
	{
    case 8:
      pDstData += iDstY * iDstLineSize + iDstX;
      //pSrcData += iSrcY * iSrcLineSize + iSrcX;
	  break;
	case 15:
	case 16:
      //iWidth <<= 1;

      pDstData += iDstY * iDstLineSize + iDstX * 2;
      iRemain = iWidth & 3;
      iWidth  = (iWidth & (~3)) >> 2;

      //pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;
	  break;

    case 24:
      iWidth = iWidth * 3;
      pDstData += iDstY * iDstLineSize + iDstX * 3;
      //pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;
	  break;

    case 32:
      iWidth <<= 2;
	  pDstData += iDstY * iDstLineSize + iDstX * 4;
      //pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
	  break;

	}

	//iSrcDataPad = iSrcLineSize - iWidth;
	iDstDataPad = iDstLineSize - iWidth;
*/


	switch (iBPP)
	{

    case 8:
    case 24:
    case 32:

        if(iBPP == 8)
        {
            pDstData += iDstY * iDstLineSize + iDstX;
        }
        else if(iBPP == 24)
        {
            iWidth = iWidth * 3;
            pDstData += iDstY * iDstLineSize + iDstX * 3;
        }
        else
        {
            iWidth <<= 2;
            pDstData += iDstY * iDstLineSize + iDstX * 4;
        }
        //pDstData += iDstY * iDstLineSize + iDstX;
        iDstDataPad = iDstLineSize - iWidth;

        i64Amount   = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;
        i64Mask     = (i64Mask << 48)   | (i64Mask << 32)   | (i64Mask << 16)   | i64Mask;

        //i64Amount = (i64Amount << 56) | (i64Amount << 48) | (i64Amount << 40) | (i64Amount << 32) |
        //              (i64Amount << 24) | (i64Amount << 16) | (i64Amount << 8)  |  i64Amount;

    if(iRealAmount >= 0)
    {

        // asm code
    __asm__ __volatile__ (

                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                //"push    %%ecx                   \n\t"

                "movq    %0,      %%mm5          \n\t"
                "movq    %1,      %%mm6          \n\t"
                "pxor    %%mm7,   %%mm7          \n\t"

                "xorl    %%eax,   %%eax          \n\t"
                "xorl    %%edx,   %%edx          \n\t"

                "movb    %2,      %%dh           \n\t"

                "cmpl    $0,      %3             \n\t"
				"jbe     __end_bright_8          \n\t"

                "movl    %4,      %%edi          \n\t"


        "__do_next_line_bright_8:                \n\t"

                "movl    %5,      %%ecx          \n\t"

                "cmpl    $8,      %%ecx          \n\t"
                "jb      __skip_mmx_bright_8     \n\t"

        "__do_mmx_bright_8:                      \n\t"


                "movq    (%%edi), %%mm1          \n\t" // dst
                "movq    %%mm1,   %%mm3          \n\t" // make a copy
                "movq    %%mm1,   %%mm2          \n\t" // make a copy
                "punpcklbw %%mm7, %%mm1          \n\t" // mm1 = unpacked low half of dst
                "punpckhbw %%mm7, %%mm2          \n\t" // mm2 = unpacked high half of dst
                "pxor    %%mm5,   %%mm1          \n\t" // dst xor mask
                "pxor    %%mm5,   %%mm2          \n\t" // dst xor mask
                "pmullw  %%mm6,   %%mm1          \n\t" // amount * (dst xor mask)
                "pmullw  %%mm6,   %%mm2          \n\t" // amount * (dst xor mask)
                "psrlw   $8,      %%mm1          \n\t" // amount * (dst xor mask) / 256
                "psrlw   $8,      %%mm2          \n\t" // amount * (dst xor mask) / 256
                "packuswb  %%mm2, %%mm1          \n\t" // combine with unsigned saturation
                "paddusb %%mm3,   %%mm1          \n\t" // dst + amount * (dst xor mask) / 256

                // fin
                "movq    %%mm1,   (%%edi)        \n\t"

                "addl    $8,      %%edi          \n\t"

                "subl    $8,      %%ecx          \n\t"
                "cmpl    $8,      %%ecx          \n\t"
                "jae     __do_mmx_bright_8       \n\t"

        "__skip_mmx_bright_8:                    \n\t"

                "cmpl    $1,      %%ecx          \n\t"
                "jb      __skip_bright_8         \n\t"

        "__do_bright_8:                          \n\t"

                "movb    (%%edi), %%al           \n\t"
                "movb    %%al,    %%dl           \n\t"
                "xorb    $0xff,   %%al           \n\t"
                "mulb    %%dh                    \n\t"
                "shrw    $8,      %%ax           \n\t"
                "addb    %%dl,    %%al           \n\t"
                "jc       __bright_max_8         \n\t"

                "movb     %%al, (%%edi)          \n\t"
                "jmp      __next_bright_8        \n\t"

        "__bright_max_8:                         \n\t"

                "movb     $0xff, (%%edi)         \n\t"

        "__next_bright_8:                        \n\t"

                "incl     %%edi                  \n\t"

                "decl     %%ecx                  \n\t"
                "jnz     __do_bright_8           \n\t"

       "__skip_bright_8:                         \n\t"

                "addl     %6, %%edi              \n\t"

                "decl     %3                     \n\t"
                "jnz     __do_next_line_bright_8 \n\t"


        "__end_bright_8:                         \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad
                : "m"(i64Mask), "m"(i64Amount), "m"(i8Amount), "m"(iHeight),
                  "m"(pDstData), "m"(iWidth), "m"(iDstDataPad)
                : "%eax", "%edx", "%edi", "%ecx", "memory"


        );
    }
    else
    {
        __asm__ __volatile__ (

                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                //"push    %%ecx                   \n\t"

                "movq    %0,      %%mm5          \n\t"
                "movq    %1,      %%mm6          \n\t"
                "pxor    %%mm7,   %%mm7          \n\t"

                "xorl    %%eax,   %%eax          \n\t"
                "xorl    %%edx,   %%edx          \n\t"

                "movb    %2,      %%dh           \n\t"

                "cmpl    $0,      %3             \n\t"
				"jbe     __end_dark_8            \n\t"

                "movl    %4,      %%edi          \n\t"


        "__do_next_line_dark_8:                \n\t"

                "movl    %5,      %%ecx          \n\t"

                "cmpl    $8,      %%ecx          \n\t"
                "jb      __skip_mmx_dark_8     \n\t"

        "__do_mmx_dark_8:                      \n\t"


                "movq    (%%edi), %%mm1          \n\t" // dst
                "movq    %%mm1,   %%mm3          \n\t" // make a copy
                "movq    %%mm1,   %%mm2          \n\t" // make a copy
                "punpcklbw %%mm7, %%mm1          \n\t" // mm1 = unpacked low half of dst
                "punpckhbw %%mm7, %%mm2          \n\t" // mm2 = unpacked high half of dst
                "pmullw  %%mm6,   %%mm1          \n\t" // amount * (dst)
                "pmullw  %%mm6,   %%mm2          \n\t" // amount * (dst)
                "psrlw   $8,      %%mm1          \n\t" // amount * (dst) / 256
                "psrlw   $8,      %%mm2          \n\t" // amount * (dst) / 256
                "packuswb  %%mm2, %%mm1          \n\t" // combine with unsigned saturation
                "psubusb %%mm1,   %%mm3          \n\t" // dst - amount * (dst) / 256

                // fin
                "movq    %%mm3,   (%%edi)        \n\t"

                "addl    $8,      %%edi          \n\t"

                "subl    $8,      %%ecx          \n\t"
                "cmpl    $8,      %%ecx          \n\t"
                "jae     __do_mmx_dark_8       \n\t"

        "__skip_mmx_dark_8:                    \n\t"

                "cmpl    $1,      %%ecx          \n\t"
                "jb      __skip_dark_8         \n\t"

        "__do_dark_8:                          \n\t"

                "movb    (%%edi), %%al           \n\t"
                "movb    %%al,    %%dl           \n\t"

                "mulb    %%dh                    \n\t"
                "shrw    $8,      %%ax           \n\t"
                "subb    %%al,    %%dl           \n\t"
                "jc       __dark_min_8         \n\t"

                "movb     %%dl, (%%edi)          \n\t"
                "jmp      __next_dark_8        \n\t"

        "__dark_min_8:                         \n\t"

                "movb     $0, (%%edi)         \n\t"

        "__next_dark_8:                        \n\t"

                "incl     %%edi                  \n\t"

                "decl     %%ecx                  \n\t"
                "jnz     __do_dark_8           \n\t"

       "__skip_dark_8:                         \n\t"

                "addl     %6, %%edi              \n\t"

                "decl     %3                     \n\t"
                "jnz     __do_next_line_dark_8 \n\t"


        "__end_dark_8:                         \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == i64Mask,
                // %1 == i64Amount, %2 == i8Amount,
                // %3 == iHeight,
                // %4 == pDstData
                // %5 == iWidth
                // %6 == iDstDataPad
                : "m"(i64Mask), "m"(i64Amount), "m"(i8Amount), "m"(iHeight),
                  "m"(pDstData), "m"(iWidth), "m"(iDstDataPad)
                : "%eax", "%edx", "%edi", "%ecx", "memory"


        );
    }

    break;


	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;
      i64X         = 0x003f003f003f003fLL;    // for green

      i64Amount    = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;

      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iDstDataPad = iDstLineSize - (iWidth << 1);

      iRemain = iWidth & 3;
      iWidth  = (iWidth & (~3)) >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,       %1 == pDstData,
	      // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	      // %5 == iRealAmount,  %6 == i64Amount,      %7 == i64X

    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"
                    "movl     %0,      %%ecx   \n\t"  // %0 == iWidth
                    "cmpl     $0,      %%ecx   \n\t"
                    "jz      __skip_light_16  \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi            \n\t"
                    "movl     %1, %%esi        \n\t"  // %1 == pDstData

                    "movl     %5, %%ebx        \n\t"  // %1 == pDstData

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_light_16:                       \n\t"

                    "movq    (%%esi), %%mm0   \n\t"
                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %2,      %%mm1   \n\t"  // %2 == i64MaskBlue,  put blue  to %%mm1
                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"  // %3 == i64MaskGreen, put green to %%mm2
                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"  // %4 == i64MaskRed,   put red   to %%mm3
                    "cmpl    $0,      %%ebx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_16        \n\t"

        "__bright_16:                         \n\t"

                    // handle blue                   // result := dst + amount * (dst xor mask) / 256
                    "movq    %%mm1,   %%mm7   \n\t"  // backup it
                    "pxor    %2,      %%mm1   \n\t"  // dst xor mask
                    "pmullw  %6,      %%mm1   \n\t"  // %6 == i64Amount, amount * (dst xor mask)

                    "psrlw   $8,      %%mm1   \n\t"  // amount * (dst xor mask) / 256
                    "psllw   $11,     %%mm1   \n\t"
                    "psllw   $11,     %%mm7   \n\t"  // move to highest for saturae add
                    "paddusw %%mm7,   %%mm1   \n\t"  // dst + amount * (dst xor mask) / 256
                    "psrlw   $11,     %%mm1   \n\t"  // remove to the original position

                    // handle green
                    "psrlw   $5,      %%mm2   \n\t"
                    "movq    %%mm2,   %%mm7   \n\t"
                    "pxor    %7,      %%mm2   \n\t"  // %7 == i64X,  green is 6 bit ...
                    "pmullw  %6,      %%mm2   \n\t"

                    "psrlw   $8,      %%mm2   \n\t"
                    "psllw   $10,     %%mm2   \n\t"
                    "psllw   $10,     %%mm7   \n\t"
                    "paddusw %%mm7,   %%mm2   \n\t"
                    "psrlw   $5,      %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"

                    // handle red
                    "psrlw   $11,     %%mm3   \n\t"
                    "movq    %%mm3,   %%mm7   \n\t"
                    "pxor    %2,      %%mm3   \n\t"
                    "pmullw  %6,      %%mm3   \n\t"

                    "psrlw   $8,      %%mm3   \n\t"
                    "psllw   $11,     %%mm3   \n\t"
                    "psllw   $11,     %%mm7   \n\t"
                    "paddusw %%mm7,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"

                    "jmp     __next_light_16  \n\t"

        "__dark_16:                           \n\t"

                    // handle blue                   // result := dst - amount * dst / 256
                    "movq    %%mm1, %%mm7     \n\t"  // backup it
                    "pmullw  %6,    %%mm7     \n\t"  // amount * dst

                    "psrlw   $8,    %%mm7     \n\t"  // amount * dst / 256
                    "psubusw %%mm7, %%mm1     \n\t"  // dst - amount * dst / 256

                    // handle green
                    "psrlw   $5,    %%mm2     \n\t"
                    "movq    %%mm2, %%mm7     \n\t"
                    "pmullw  %6,    %%mm7     \n\t"

                    "psrlw   $8,    %%mm7     \n\t"
                    "psubusw %%mm7, %%mm2     \n\t"
                    "psllw   $5,    %%mm2     \n\t"

                    // handle red
                    "psrlw   $11,   %%mm3     \n\t"
                    "movq    %%mm3, %%mm7     \n\t"
                    "pmullw  %6,    %%mm7     \n\t"

                    "psrlw   $8,    %%mm7     \n\t"
                    "psubusw %%mm7, %%mm3     \n\t"
                    "psllw   $11,   %%mm3     \n\t"

        "__next_light_16:                     \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    "movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi     \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx             \n\t"
                    "jnz	__do_light_16     \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %1         \n\t"
                    "popl	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_light_16:                     \n\t"

                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                    // %0 == iWidth,       %1 == pDstData,
	                // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	                // %5 == iRealAmount,  %6 == i64Amount,      %7 == i64X
                    : "m"(iWidth), "m"(pDstData), "m"(i64MaskBlue), "m"(i64MaskGreen),
                      "m"(i64MaskRed), "m"(iRealAmount), "m"(i64Amount), "m"(i64X)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((uint16_t*)pDstData);
			iB = iPixel & 0x001f;
			iG = (iPixel >> 5) &  0x003f;
			iR = (iPixel >> 11) & 0x001f;
			if (iRealAmount >= 0)
			{
				iB = OGE_FX_Saturate(iB + (iRealAmount * (iB ^ 31) >> 8), 31);
				iG = OGE_FX_Saturate(iG + (iRealAmount * (iG ^ 63) >> 8), 63);
				iR = OGE_FX_Saturate(iR + (iRealAmount * (iR ^ 31) >> 8), 31);
			}
			else
			{
				iB = OGE_FX_Saturate(iB - (abs(iRealAmount) * iB >> 8), 31);
				iG = OGE_FX_Saturate(iG - (abs(iRealAmount) * iG >> 8), 63);
				iR = OGE_FX_Saturate(iR - (abs(iRealAmount) * iR >> 8), 31);
			}

			*((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

			pDstData += 2;

		}

         pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;

/*
    case 32:
// RGB 8888 Mode ..

      i64MaskRed   = 0x00ff000000ff0000LL;
      i64MaskGreen = 0x0000ff000000ff00LL;
      i64MaskBlue  = 0x000000ff000000ffLL;
      i64Amount    = (i64Amount << 32) | i64Amount;

      pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

      iDstDataPad     = iDstLineSize - (iWidth << 2);

      iRemain      = iWidth & 1;
      iWidth     >>= 1;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,       %1 == pDstData,
	      // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	      // %5 == iRealAmount,  %6 == i64Amount,      %7 == i64X

    // asm code
		  __asm__ __volatile__ (

                    "push    %%ecx            \n\t"
                    "mov     %0,      %%ecx   \n\t"  // %0 == iWidth
                    "cmp     $0,      %%ecx   \n\t"
                    "jz      __skip8888_32    \n\t"  // if this row is less than 4 then jump to next

                    "push    %%esi            \n\t"
                    "mov     %1, %%esi        \n\t"  // %1 == pDstData

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_light_32:                       \n\t"

                    "movq    (%%esi), %%mm0   \n\t"

                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %2,      %%mm1   \n\t"  // %2 == i64MaskBlue,  put blue  to %%mm1

                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"  // %3 == i64MaskGreen, put green to %%mm2

                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"  // %4 == i64MaskRed,   put red   to %%mm3

                    "cmp     $0,      %5      \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_32        \n\t"

        "__bright_32:                         \n\t"

                    // handle blue                   // result := dst + amount * (dst xor mask) / 256
                    "movq    %%mm1,   %%mm7   \n\t"  // backup it
                    "pxor    %2,      %%mm1   \n\t"  // dst xor mask
                    "pmullw  %6,      %%mm1   \n\t"  // %6 == i64Amount, amount * (dst xor mask)

                    "psrld   $8,      %%mm1   \n\t"  // amount * (dst xor mask) / 256
                    "pslld   $24,     %%mm1   \n\t"
                    "pslld   $24,     %%mm7   \n\t"  // move to highest for saturae add
                    "paddusw %%mm7,   %%mm1   \n\t"  // dst + amount * (dst xor mask) / 256
                    "psrld   $24,     %%mm1   \n\t"  // remove to the original position

                    // handle green
                    "psrld   $8,      %%mm2   \n\t"
                    "movq    %%mm2,   %%mm7   \n\t"
                    "pxor    %2,      %%mm2   \n\t"  //
                    "pmullw  %6,      %%mm2   \n\t"

                    "psrld   $8,      %%mm2   \n\t"
                    "pslld   $24,     %%mm2   \n\t"
                    "pslld   $24,     %%mm7   \n\t"
                    "paddusw %%mm7,   %%mm2   \n\t"
                    "psrld   $24,     %%mm2   \n\t"
                    "pslld   $8,      %%mm2   \n\t"

                    // handle red
                    "psrld   $16,     %%mm3   \n\t"
                    "movq    %%mm3,   %%mm7   \n\t"
                    "pxor    %2,      %%mm3   \n\t"
                    "pmullw  %6,      %%mm3   \n\t"

                    "psrld   $8,      %%mm3   \n\t"
                    "pslld   $24,     %%mm3   \n\t"
                    "pslld   $24,     %%mm7   \n\t"
                    "paddusw %%mm7,   %%mm3   \n\t"
                    "psrld   $24,     %%mm3   \n\t"
                    "pslld   $16,     %%mm3   \n\t"

                    "jmp     __next_32        \n\t"

        "__dark_32:                           \n\t"

                    // handle blue                   // result := dst - amount * dst / 256
                    "movq    %%mm1, %%mm7     \n\t"  // backup it
                    "pmullw  %6,    %%mm7     \n\t"  // amount * dst

                    "psrld   $8,    %%mm7     \n\t"  // amount * dst / 256
                    "psubusw %%mm7, %%mm1     \n\t"  // dst - amount * dst / 256

                    // handle green
                    "psrld   $8,    %%mm2     \n\t"
                    "movq    %%mm2, %%mm7     \n\t"
                    "pmullw  %6,    %%mm7     \n\t"

                    "psrld   $8,    %%mm7     \n\t"
                    "psubusw %%mm7, %%mm2     \n\t"
                    "pslld   $8,    %%mm2     \n\t"

                    // handle red
                    "psrld   $16,   %%mm3     \n\t"
                    "movq    %%mm3, %%mm7     \n\t"
                    "pmullw  %6,    %%mm7     \n\t"

                    "psrld   $8,    %%mm7     \n\t"
                    "psubusw %%mm7, %%mm3     \n\t"
                    "pslld   $16,   %%mm3     \n\t"

        "__next_32:                           \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    "movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "add     $8,    %%esi     \n\t"

                    //
                    // Loop again or break.
                    //
                    "dec	%%ecx             \n\t"
                    "jnz	__do_light_32     \n\t"

                    //
                    // Write back ...
                    //
                    "mov	%%esi, %1         \n\t"
                    "pop	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip8888_32:                       \n\t"

                    "pop	%%ecx             \n\t"

                    : // no output
                    // %0 == iWidth,       %1 == pDstData,
	                // %2 == i64MaskBlue,  %3 == i64MaskGreen,   %4 == i64MaskRed
	                // %5 == iRealAmount,  %6 == i64Amount,      %7 == i64X
                    : "m"(iWidth), "m"(pDstData), "m"(i64MaskBlue), "m"(i64MaskGreen),
                      "m"(i64MaskRed), "m"(iRealAmount), "m"(i64Amount)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((int*)pDstData);
			iB = iPixel & 0xff;
			iG = (iPixel >> 8) &  0xff;
			iR = (iPixel >> 16) & 0xff;
			if (iRealAmount >= 0)
			{
				iB = OGE_FX_Saturate(iB + (iRealAmount * (iB ^ 255) >> 8), 255);
				iG = OGE_FX_Saturate(iG + (iRealAmount * (iG ^ 255) >> 8), 255);
				iR = OGE_FX_Saturate(iR + (iRealAmount * (iR ^ 255) >> 8), 255);
			}
			else
			{
				iB = OGE_FX_Saturate(iB - (abs(iRealAmount) * iB >> 8), 255);
				iG = OGE_FX_Saturate(iG - (abs(iRealAmount) * iG >> 8), 255);
				iR = OGE_FX_Saturate(iR - (abs(iRealAmount) * iR >> 8), 255);
			}

			*((int*)pDstData) = (iR << 16) | (iG << 8) | iB;

			pDstData += 4;

		}

        pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break; // end of 8888 mode

*/


    } // end of switch case


}

void OGE_FX_ChangeColorRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iDstDataPad, iPixel;
	uint8_t iB, iG, iR;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X;
	uint64_t i64RedAmount, i64GreenAmount, i64BlueAmount;

	//uint64_t i64Amount;

	//iRealAmount   = iRedAmount + iGreenAmount + iBlueAmount;
	i64RedAmount  = abs(iRedAmount);
	i64GreenAmount= abs(iGreenAmount);
	i64BlueAmount = abs(iBlueAmount);

	switch (iBPP)
	{

	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;
      i64X         = 0x003f003f003f003fLL;    // for green

      i64RedAmount    = (i64RedAmount << 48)   | (i64RedAmount << 32)   | (i64RedAmount << 16)   | i64RedAmount;
      i64GreenAmount  = (i64GreenAmount << 48) | (i64GreenAmount << 32) | (i64GreenAmount << 16) | i64GreenAmount;
      i64BlueAmount   = (i64BlueAmount << 48)  | (i64BlueAmount << 32)  | (i64BlueAmount << 16)  | i64BlueAmount;

      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iDstDataPad = iDstLineSize - (iWidth << 1);

      iRemain = iWidth & 3;
      iWidth  = iWidth >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,         %1 == pDstData,
	      // %2 == i64MaskBlue,    %3 == i64MaskGreen,   %4  == i64MaskRed,  %5 == i64X,
	      // %6 == i64BlueAmount,  %7 == i64GreenAmount, %8  == i64RedAmount,
	      // %9 == iBlueAmount,    %10 == iGreenAmount,  %11 == iRedAmount
	      //
	      //


    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"  // iBlueAmount
                    "pushl    %%edx            \n\t"  // iGreenAmount
                    "pushl    %%eax            \n\t"  // iRedAmount

                    "movl     %0,      %%ecx   \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx   \n\t"
                    "jz      __skip_light_RGB_16  \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi            \n\t"
                    "movl     %1, %%esi        \n\t"  // %? == pDstData

                    "movl     %9,  %%ebx        \n\t"  // %? == iBlueAmount
                    "movl     %10, %%edx        \n\t"  // %? == iGreenAmount
                    "movl     %11, %%eax        \n\t"  // %? == iRedAmount

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_light_RGB_16:                       \n\t"

                    "movq    (%%esi), %%mm0   \n\t"
                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %2,      %%mm1   \n\t"  // %2 == i64MaskBlue,  put blue  to %%mm1
                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"  // %3 == i64MaskGreen, put green to %%mm2
                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"  // %4 == i64MaskRed,   put red   to %%mm3

                    "cmpl    $0,      %%ebx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_blue_16        \n\t"

        "__bright_RGB_blue_16:                         \n\t"

                    // handle blue
                    "movq    %6,      %%mm7   \n\t"

                    "psllw   $3,      %%mm1   \n\t"
                    "paddusb %%mm7,   %%mm1   \n\t"
                    "psrlw   $3,      %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_green_16        \n\t"

        "__bright_RGB_green_16:                         \n\t"

                    // handle green
                    "movq    %7,      %%mm7   \n\t"

                    "psrlw   $5,      %%mm2   \n\t"
                    "psllw   $2,      %%mm2   \n\t"
                    "paddusb %%mm7,   %%mm2   \n\t"
                    "psrlw   $2,      %%mm2   \n\t"
                    "psllw   $5,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_red_16        \n\t"

        "__bright_RGB_red_16:                         \n\t"

                    // handle red
                    "movq    %8,      %%mm7   \n\t"

                    "psrlw   $11,     %%mm3   \n\t"
                    "psllw   $3,      %%mm3   \n\t"
                    "paddusb %%mm7,   %%mm3   \n\t"
                    "psrlw   $3,      %%mm3   \n\t"
                    "psllw   $11,     %%mm3   \n\t"

                    "jmp     __next_light_RGB_16  \n\t"

        "__dark_RGB_blue_16:                           \n\t"

                    // handle blue

                    "movq    %6,      %%mm7   \n\t"

                    "psllw   $3,      %%mm1   \n\t"
                    "psubusb %%mm7,   %%mm1   \n\t"
                    "psrlw   $3,      %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_RGB_green_16     \n\t"

        "__dark_RGB_green_16:                           \n\t"

                    // handle green
                    "movq    %7,      %%mm7   \n\t"

                    "psrlw   $5,      %%mm2   \n\t"
                    "psllw   $2,      %%mm2   \n\t"
                    "psubusb %%mm7,   %%mm2   \n\t"
                    "psrlw   $2,      %%mm2   \n\t"
                    "psllw   $5,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_RGB_red_16        \n\t"

        "__dark_RGB_red_16:                           \n\t"

                    // handle red

                    "movq    %8,      %%mm7   \n\t"

                    "psrlw   $11,     %%mm3   \n\t"
                    "psllw   $3,      %%mm3   \n\t"
                    "psubusb %%mm7,   %%mm3   \n\t"
                    "psrlw   $3,      %%mm3   \n\t"
                    "psllw   $11,     %%mm3   \n\t"

        "__next_light_RGB_16:                     \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    "movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi     \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx             \n\t"
                    "jnz	__do_light_RGB_16     \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %1         \n\t"
                    "popl	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_light_RGB_16:                     \n\t"

                    "popl	%%eax             \n\t"
                    "popl	%%edx             \n\t"
                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                      // %0 == iWidth,         %1 == pDstData,
                      // %2 == i64MaskBlue,    %3 == i64MaskGreen,   %4  == i64MaskRed,  %5 == i64X,
                      // %6 == i64BlueAmount,  %7 == i64GreenAmount, %8  == i64RedAmount,
                      // %9 == iBlueAmount,    %10 == iGreenAmount,  %11 == iRedAmount
                    : "m"(iWidth),        "m"(pDstData),
                      "m"(i64MaskBlue),   "m"(i64MaskGreen),   "m"(i64MaskRed), "m"(i64X),
                      "m"(i64BlueAmount), "m"(i64GreenAmount), "m"(i64RedAmount),
                      "m"(iBlueAmount),   "m"(iGreenAmount),   "m"(iRedAmount)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((uint16_t*)pDstData);
			iB = iPixel & 0x001f;
			iG = (iPixel >> 5) &  0x003f;
			iR = (iPixel >> 11) & 0x001f;

			if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + (iBlueAmount >> 3), 31);
			else iB = OGE_FX_Saturate(iB - (abs(iBlueAmount) >> 3), 31);

			if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + (iGreenAmount >> 2), 63);
			else iG = OGE_FX_Saturate(iG - (abs(iGreenAmount) >> 2), 63);

			if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + (iRedAmount >> 3), 31);
			else iR = OGE_FX_Saturate(iR - (abs(iRedAmount) >> 3), 31);

			*((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

			pDstData += 2;

		}

         pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;


    case 32:
// RGB 8888 Mode ..

      i64MaskRed   = 0x00ff000000ff0000LL;
      i64MaskGreen = 0x0000ff000000ff00LL;
      i64MaskBlue  = 0x000000ff000000ffLL;

      //i64Amount    = (i64Amount << 32) | i64Amount;

      i64BlueAmount   = (i64BlueAmount << 32)  | i64BlueAmount;
      i64GreenAmount  = (i64GreenAmount << 32) | i64GreenAmount;
      i64RedAmount    = (i64RedAmount << 32)   | i64RedAmount;

      pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

      iDstDataPad  = iDstLineSize - (iWidth << 2);

      iRemain      = iWidth & 1;
      iWidth     >>= 1;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,         %1 == pDstData,
	      // %2 == i64MaskBlue,    %3 == i64MaskGreen,   %4  == i64MaskRed,
	      // %5 == i64BlueAmount,  %6 == i64GreenAmount, %7  == i64RedAmount,
	      // %8 == iBlueAmount,    %9 == iGreenAmount,   %10 == iRedAmount

    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"  // iBlueAmount
                    "pushl    %%edx            \n\t"  // iGreenAmount
                    "pushl    %%eax            \n\t"  // iRedAmount

                    "mov     %0,      %%ecx   \n\t"  // %0 == iWidth
                    "cmp     $0,      %%ecx   \n\t"
                    "jz      __skip_light_RGB_32    \n\t"  // if this row is less than 4 then jump to next

                    "push    %%esi            \n\t"
                    "mov     %1, %%esi        \n\t"  // %1 == pDstData

                    "movl     %8,  %%ebx        \n\t"  // %? == iBlueAmount
                    "movl     %9, %%edx        \n\t"  // %? == iGreenAmount
                    "movl     %10, %%eax        \n\t"  // %? == iRedAmount

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_light_RGB_32:                       \n\t"

                    "movq    (%%esi), %%mm0   \n\t"

                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %2,      %%mm1   \n\t"  // %2 == i64MaskBlue,  put blue  to %%mm1

                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %3,      %%mm2   \n\t"  // %3 == i64MaskGreen, put green to %%mm2

                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %4,      %%mm3   \n\t"  // %4 == i64MaskRed,   put red   to %%mm3

                    "cmp     $0,      %%ebx      \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_blue_32        \n\t"

        "__bright_RGB_blue_32:                         \n\t"

                    // handle blue
                    "movq    %5,      %%mm7   \n\t"

                    "paddusb %%mm7,   %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_green_32        \n\t"

        "__bright_RGB_green_32:                         \n\t"

                    // handle green
                    "movq    %6,      %%mm7   \n\t"

                    "psrld   $8,      %%mm2   \n\t"
                    "paddusb %%mm7,   %%mm2   \n\t"
                    "pslld   $8,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_RGB_red_32        \n\t"

        "__bright_RGB_red_32:                         \n\t"

                    // handle red
                    "movq    %7,      %%mm7   \n\t"

                    "psrld   $16,     %%mm3   \n\t"
                    "paddusb %%mm7,   %%mm3   \n\t"
                    "pslld   $16,     %%mm3   \n\t"

                    "jmp     __next_light_RGB_32       \n\t"

        "__dark_RGB_blue_32:                           \n\t"

                    // handle blue
                    "movq    %5,    %%mm7     \n\t"
                    "psubusb %%mm7, %%mm1     \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_RGB_green_32    \n\t"

        "__dark_RGB_green_32:                          \n\t"

                    // handle green
                    "movq    %6,    %%mm7     \n\t"

                    "psrld   $8,    %%mm2     \n\t"
                    "psubusb %%mm7, %%mm2     \n\t"
                    "pslld   $8,    %%mm2     \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_RGB_red_32      \n\t"

        "__dark_RGB_red_32:                            \n\t"

                    // handle red
                    "movq    %7,    %%mm7     \n\t"

                    "psrld   $16,   %%mm3     \n\t"
                    "psubusb %%mm7, %%mm3     \n\t"
                    "pslld   $16,   %%mm3     \n\t"

        "__next_light_RGB_32:                          \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    "movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "add     $8,    %%esi     \n\t"

                    //
                    // Loop again or break.
                    //
                    "dec	%%ecx             \n\t"
                    "jnz	__do_light_RGB_32 \n\t"

                    //
                    // Write back ...
                    //
                    "mov	%%esi, %1         \n\t"
                    "pop	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_light_RGB_32:                 \n\t"

                    "popl	%%eax             \n\t"
                    "popl	%%edx             \n\t"
                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                      // %0 == iWidth,         %1 == pDstData,
                      // %2 == i64MaskBlue,    %3 == i64MaskGreen,   %4  == i64MaskRed,
                      // %5 == i64BlueAmount,  %6 == i64GreenAmount, %7  == i64RedAmount,
                      // %8 == iBlueAmount,    %9 == iGreenAmount,   %10 == iRedAmount
                    : "m"(iWidth), "m"(pDstData),
                      "m"(i64MaskBlue), "m"(i64MaskGreen), "m"(i64MaskRed),
                      "m"(i64BlueAmount), "m"(i64GreenAmount), "m"(i64RedAmount),
                      "m"(iBlueAmount), "m"(iGreenAmount), "m"(iRedAmount)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((int*)pDstData);
			iB = iPixel & 0xff;
			iG = (iPixel >> 8) &  0xff;
			iR = (iPixel >> 16) & 0xff;


			if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + iBlueAmount, 255);
			else iB = OGE_FX_Saturate(iB - abs(iBlueAmount), 255);

			if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + iGreenAmount, 255);
			else iG = OGE_FX_Saturate(iG - abs(iGreenAmount), 255);

			if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + iRedAmount, 255);
			else iR = OGE_FX_Saturate(iR - abs(iRedAmount), 255);

			*((int*)pDstData) = (iR << 16) | (iG << 8) | iB;

			pDstData += 4;

		}

        pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break; // end of 8888 mode

    } // end of switch case
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
    if(mmxflag <= 0) return;

    int i, iRemain, iRealAmount, iPixel;
	uint8_t iB, iG, iR;

	//uint32_t i32Amount;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X, i64Mask, i64Amount;

	int iSrcDataPad;
	int iDstDataPad;

	uint64_t i64ColorKey = iSrcColorKey;
	//uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;

	//uint64_t i64Amount;

	iRealAmount = iAmount;
	//i32Amount   = abs(iAmount);
	i64Amount   = abs(iAmount);

	i64Mask     = 0xff;


	switch (iBPP)
	{

	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;
      i64X         = 0x003f003f003f003fLL;    // for green

      i64ColorKey  = (i64ColorKey << 48)  | (i64ColorKey << 32)  | (i64ColorKey << 16)  | i64ColorKey;
      i64Amount    = (i64Amount << 48)    | (i64Amount << 32)    | (i64Amount << 16)    | i64Amount;

      pSrcData = pSrcData + iSrcLineSize * iSrcY + 2*iSrcX;
      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iSrcDataPad = iSrcLineSize - (iWidth << 1);
      iDstDataPad = iDstLineSize - (iWidth << 1);

      iRemain = iWidth & 3;
      iWidth  = iWidth >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,
	      // %1 == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
	      // %4 == iRealAmount,  %5 == i64Amount,     %6 == i64X
	      // %7 == pSrcData,     %8 == pDstData,      %9 == i64ColorKey

    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx              \n\t"
                    "pushl    %%ebx              \n\t"

                    "movl     %0,      %%ecx     \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx     \n\t"
                    "jz      __skip_blt_light_16 \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi              \n\t"
                    "pushl    %%edi              \n\t"

                    "movl     %4, %%ebx          \n\t"  // %? == iRealAmount

                    "movl     %7, %%esi          \n\t"  // %? == pSrcData
                    "movl     %8, %%edi          \n\t"  // %? == pDstData

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red

        "__do_blt_light_16:                      \n\t"

                    "movq    (%%esi), %%mm0      \n\t"
                    "movq    %%mm0,   %%mm1      \n\t"
                    "pand    %1,      %%mm1      \n\t"  // %? == i64MaskBlue,  put blue  to %%mm1
                    "movq    %%mm0,   %%mm2      \n\t"
                    "pand    %2,      %%mm2      \n\t"  // %? == i64MaskGreen, put green to %%mm2
                    "movq    %%mm0,   %%mm3      \n\t"
                    "pand    %3,      %%mm3      \n\t"  // %? == i64MaskRed,   put red   to %%mm3
                    "cmpl    $0,      %%ebx      \n\t"  // %? == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __blt_dark_16       \n\t"

        "__blt_bright_16:                        \n\t"

                    // handle blue                      // result := dst + amount * (dst xor mask) / 256
                    "movq    %%mm1,   %%mm7      \n\t"  // backup it
                    "pxor    %1,      %%mm1      \n\t"  // %? = i64MaskBlue, dst xor mask
                    "pmullw  %5,      %%mm1      \n\t"  // %? == i64Amount, amount * (dst xor mask)

                    "psrlw   $8,      %%mm1      \n\t"  // amount * (dst xor mask) / 256
                    "psllw   $11,     %%mm1      \n\t"
                    "psllw   $11,     %%mm7      \n\t"  // move to highest for saturae add
                    "paddusw %%mm7,   %%mm1      \n\t"  // dst + amount * (dst xor mask) / 256
                    "psrlw   $11,     %%mm1      \n\t"  // remove to the original position

                    // handle green
                    "psrlw   $5,      %%mm2      \n\t"
                    "movq    %%mm2,   %%mm7      \n\t"
                    "pxor    %6,      %%mm2      \n\t"  // %? == i64X,  green is 6 bit ...
                    "pmullw  %5,      %%mm2      \n\t"  //  %? == i64Amount

                    "psrlw   $8,      %%mm2      \n\t"
                    "psllw   $10,     %%mm2      \n\t"
                    "psllw   $10,     %%mm7      \n\t"
                    "paddusw %%mm7,   %%mm2      \n\t"
                    "psrlw   $5,      %%mm2      \n\t"
                    "pand    %2,      %%mm2      \n\t"  //  %? == i64MaskGreen

                    // handle red
                    "psrlw   $11,     %%mm3      \n\t"
                    "movq    %%mm3,   %%mm7      \n\t"
                    "pxor    %1,      %%mm3      \n\t"  //  %? == i64MaskBlue
                    "pmullw  %5,      %%mm3      \n\t"  //  %? == i64Amount

                    "psrlw   $8,      %%mm3      \n\t"
                    "psllw   $11,     %%mm3      \n\t"
                    "psllw   $11,     %%mm7      \n\t"
                    "paddusw %%mm7,   %%mm3      \n\t"
                    "pand    %3,      %%mm3      \n\t"  //  %? == i64MaskRed

                    "jmp     __next_blt_light_16 \n\t"

        "__blt_dark_16:                          \n\t"

                    // handle blue                     // result := dst - amount * dst / 256
                    "movq    %%mm1, %%mm7        \n\t" // backup it
                    "pmullw  %5,    %%mm7        \n\t" // %? == i64Amount, amount * dst

                    "psrlw   $8,    %%mm7        \n\t"  // amount * dst / 256
                    "psubusw %%mm7, %%mm1        \n\t"  // dst - amount * dst / 256

                    // handle green
                    "psrlw   $5,    %%mm2        \n\t"
                    "movq    %%mm2, %%mm7        \n\t"
                    "pmullw  %5,    %%mm7        \n\t" //%? == i64Amount

                    "psrlw   $8,    %%mm7        \n\t"
                    "psubusw %%mm7, %%mm2        \n\t"
                    "psllw   $5,    %%mm2        \n\t"

                    // handle red
                    "psrlw   $11,   %%mm3        \n\t"
                    "movq    %%mm3, %%mm7        \n\t"
                    "pmullw  %5,    %%mm7        \n\t" //%? == i64Amount

                    "psrlw   $8,    %%mm7        \n\t"
                    "psubusw %%mm7, %%mm3        \n\t"
                    "psllw   $11,   %%mm3        \n\t"

        "__next_blt_light_16:                    \n\t"

                    "por     %%mm2, %%mm1        \n\t"
                    "por     %%mm3, %%mm1        \n\t"

                    //"movq    %%mm1, (%%esi)   \n\t"

        "__blt_light_color_key_16:               \n\t"

                    "movq    (%%edi), %%mm7      \n\t"
                    "movq    %%mm0, %%mm2        \n\t"

                    "pcmpeqw %9, %%mm2           \n\t"  //%? == i64ColorKey
                    "movq    %%mm2, %%mm3        \n\t"

                    "pandn   %%mm1, %%mm3        \n\t"
                    "pand    %%mm2, %%mm7        \n\t"

                    "por     %%mm7, %%mm3        \n\t"
                    "movq    %%mm3, (%%edi)      \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_blt_light_16    \n\t"

                    //
                    // Write back ...
                    //

                    "movl	%%esi, %7            \n\t"
                    "movl	%%edi, %8            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

        "__skip_blt_light_16:                    \n\t"

                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0 == iWidth,
                      // %1 == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
                      // %4 == iRealAmount,  %5 == i64Amount,     %6 == i64X
                      // %7 == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
                    : "m"(iWidth),
                      "m"(i64MaskBlue), "m"(i64MaskGreen), "m"(i64MaskRed),
                      "m"(iRealAmount), "m"(i64Amount),    "m"(i64X),
                      "m"(pSrcData),    "m"(pDstData),     "m"(i64ColorKey)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((uint16_t*)pSrcData);
			if (iPixel != iSrcColorKey)
			{
				iB = iPixel & 0x001f;
				iG = (iPixel >> 5) &  0x003f;
				iR = (iPixel >> 11) & 0x001f;
				if (iRealAmount >= 0)
				{
					iB = OGE_FX_Saturate(iB + (iRealAmount * (iB ^ 31) >> 8), 31);
					iG = OGE_FX_Saturate(iG + (iRealAmount * (iG ^ 63) >> 8), 63);
					iR = OGE_FX_Saturate(iR + (iRealAmount * (iR ^ 31) >> 8), 31);
				}
				else
				{
					iB = OGE_FX_Saturate(iB - (abs(iRealAmount) * iB >> 8), 31);
					iG = OGE_FX_Saturate(iG - (abs(iRealAmount) * iG >> 8), 63);
					iR = OGE_FX_Saturate(iR - (abs(iRealAmount) * iR >> 8), 31);
				}

				*((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
			}

			pSrcData += 2;
			pDstData += 2;

		}

        pSrcData += iSrcDataPad;
		pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;



    case 32:
// RGB 8888 Mode ..

    i64MaskRed   = 0x00ff000000ff0000LL;
    i64MaskGreen = 0x0000ff000000ff00LL;
    i64MaskBlue  = 0x000000ff000000ffLL;

    //i64Amount    = (i64Amount << 32) | i64Amount;

    i64ColorKey  = (i64ColorKey << 32) | i64ColorKey;

    i64Amount   = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;
    i64Mask     = (i64Mask << 48)   | (i64Mask << 32)   | (i64Mask << 16)   | i64Mask;

    pSrcData     = pSrcData + iSrcLineSize * iSrcY + 4*iSrcX;
    pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

    iSrcDataPad     = iSrcLineSize - (iWidth << 2);
    iDstDataPad     = iDstLineSize - (iWidth << 2);

    iRemain      = iWidth & 1;
    iWidth     >>= 1;

    while (iHeight > 0)
    {
	      // %0 == iWidth,
	      // %1 == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
	      // %4 == iRealAmount,  %5 == i64Amount,     %6 == i64Mask
	      // %7 == pSrcData,     %8 == pDstData,      %9 == i64ColorKey

    // asm code
		  __asm__ __volatile__ (

                    "pushl   %%ecx               \n\t"
                    "pushl   %%ebx               \n\t"

                    "movl    %0,  %%ecx          \n\t"  // %? == iWidth
                    "cmpl    $0,  %%ecx          \n\t"
                    "jz      __skip_blt_light_32 \n\t"  // if this row is less than 4 then jump to next

                    "pushl   %%esi               \n\t"
                    "pushl   %%edi               \n\t"

                    "movl    %4,  %%ebx          \n\t"  // %? == iRealAmount

                    "movq    %9,  %%mm4          \n\t" // %? == i64ColorKey
                    "movq    %6,  %%mm5          \n\t"  // %? == i64Mask
                    "movq    %5,  %%mm6          \n\t" // %? == i64Amount

                    "movl    %7,  %%esi          \n\t"  // %? == pSrcData
                    "movl    %8,  %%edi          \n\t"  // %? == pDstData


        "__do_blt_light_32:                      \n\t"

                    "movq    (%%esi), %%mm0      \n\t"

                    "cmp     $0,      %%ebx      \n\t"  // %? == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __blt_dark_32       \n\t"

        "__blt_bright_32:                        \n\t"

                    "pxor    %%mm7,   %%mm7      \n\t"

                    "movq    %%mm0,   %%mm1      \n\t" // dst

                    "movq    %%mm1,   %%mm3      \n\t" // make a copy
                    "movq    %%mm1,   %%mm2      \n\t" // make a copy
                    "punpcklbw %%mm7, %%mm1      \n\t" // mm1 = unpacked low half of dst
                    "punpckhbw %%mm7, %%mm2      \n\t" // mm2 = unpacked high half of dst
                    "pxor    %%mm5,   %%mm1      \n\t" // dst xor mask
                    "pxor    %%mm5,   %%mm2      \n\t" // dst xor mask
                    "pmullw  %%mm6,   %%mm1      \n\t" // amount * (dst xor mask)
                    "pmullw  %%mm6,   %%mm2      \n\t" // amount * (dst xor mask)
                    "psrlw   $8,      %%mm1      \n\t" // amount * (dst xor mask) / 256
                    "psrlw   $8,      %%mm2      \n\t" // amount * (dst xor mask) / 256
                    "packuswb  %%mm2, %%mm1      \n\t" // combine with unsigned saturation
                    "paddusb %%mm3,   %%mm1      \n\t" // dst + amount * (dst xor mask) / 256


                    "jmp     __blt_light_color_key_32  \n\t"

                    // fin
                    //"movq    %%mm1,   (%%edi)        \n\t"


        "__blt_dark_32:                          \n\t"

                    "pxor    %%mm7,   %%mm7      \n\t"

                    "movq    %%mm0, %%mm1        \n\t" // dst

                    "movq    %%mm1,   %%mm3      \n\t" // make a copy
                    "movq    %%mm1,   %%mm2      \n\t" // make a copy
                    "punpcklbw %%mm7, %%mm1      \n\t" // mm1 = unpacked low half of dst
                    "punpckhbw %%mm7, %%mm2      \n\t" // mm2 = unpacked high half of dst
                    "pmullw  %%mm6,   %%mm1      \n\t" // amount * (dst)
                    "pmullw  %%mm6,   %%mm2      \n\t" // amount * (dst)
                    "psrlw   $8,      %%mm1      \n\t" // amount * (dst) / 256
                    "psrlw   $8,      %%mm2      \n\t" // amount * (dst) / 256
                    "packuswb  %%mm2, %%mm1      \n\t" // combine with unsigned saturation
                    "psubusb %%mm1,   %%mm3      \n\t" // dst - amount * (dst) / 256

                    "movq    %%mm3,   %%mm1      \n\t" // dst

                    // fin
                    //"movq    %%mm3,   (%%edi)        \n\t"

        "__blt_light_color_key_32:               \n\t"

                    "movq    (%%edi), %%mm7      \n\t"
                    "movq    %%mm0, %%mm2        \n\t"

                    "pcmpeqd %%mm4, %%mm2        \n\t"
                    "movq    %%mm2, %%mm3        \n\t"

                    "pandn   %%mm1, %%mm3        \n\t"
                    "pand    %%mm2, %%mm7        \n\t"

                    "por     %%mm7, %%mm3        \n\t"
                    "movq    %%mm3, (%%edi)      \n\t"

                    "jmp     __next_blt_light_32 \n\t"

        "__next_blt_light_32:                    \n\t"

                    //"por     %%mm2, %%mm1     \n\t"
                    //"por     %%mm3, %%mm1     \n\t"

                    //"movq    %%mm1, (%%esi)   \n\t"

                    // Advance to the next four pixels.

                    "addl    $8,    %%esi        \n\t"
                    "addl    $8,    %%edi        \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_blt_light_32    \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %7            \n\t"
                    "movl	%%edi, %8            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

        "__skip_blt_light_32:                    \n\t"

                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0 == iWidth,
                      // %1 == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
                      // %4 == iRealAmount,  %5 == i64Amount,     %6 == i64Mask
                      // %7 == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
                    : "m"(iWidth),
                      "m"(i64MaskBlue), "m"(i64MaskGreen), "m"(i64MaskRed),
                      "m"(iRealAmount), "m"(i64Amount),    "m"(i64Mask),
                      "m"(pSrcData),    "m"(pDstData),     "m"(i64ColorKey)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((int*)pSrcData);
			if (iPixel != iSrcColorKey)
			{
				iB = iPixel & 0xff;
				iG = (iPixel >> 8) &  0xff;
				iR = (iPixel >> 16) & 0xff;
				if (iRealAmount >= 0)
				{
					iB = OGE_FX_Saturate(iB + (iRealAmount * (iB ^ 255) >> 8), 255);
					iG = OGE_FX_Saturate(iG + (iRealAmount * (iG ^ 255) >> 8), 255);
					iR = OGE_FX_Saturate(iR + (iRealAmount * (iR ^ 255) >> 8), 255);
				}
				else
				{
					iB = OGE_FX_Saturate(iB - (abs(iRealAmount) * iB >> 8), 255);
					iG = OGE_FX_Saturate(iG - (abs(iRealAmount) * iG >> 8), 255);
					iR = OGE_FX_Saturate(iR - (abs(iRealAmount) * iR >> 8), 255);
				}

				*((int*)pDstData) = (iR << 16) | (iG << 8) | iB;
			}

			pSrcData += 4;
			pDstData += 4;

		}

        pSrcData += iSrcDataPad;
		pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break; // end of 8888 mode


    } // end of switch case

}

void OGE_FX_BltChangedRGB(uint8_t* pDstData, int iDstLineSize,
                      int iDstX, int iDstY,
                      uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                      int iSrcX, int iSrcY,
                      int iWidth, int iHeight, int iBPP,
                      int iRedAmount, int iGreenAmount, int iBlueAmount)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iPixel;
	uint8_t iB, iG, iR;

	int iSrcDataPad;
	int iDstDataPad;

	uint64_t i64ColorKey = iSrcColorKey;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X;
	uint64_t i64RedAmount, i64GreenAmount, i64BlueAmount;

	i64RedAmount  = abs(iRedAmount);
	i64GreenAmount= abs(iGreenAmount);
	i64BlueAmount = abs(iBlueAmount);

	switch (iBPP)
	{

	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;
      i64X         = 0x003f003f003f003fLL;    // for green

      i64RedAmount    = (i64RedAmount << 48)   | (i64RedAmount << 32)   | (i64RedAmount << 16)   | i64RedAmount;
      i64GreenAmount  = (i64GreenAmount << 48) | (i64GreenAmount << 32) | (i64GreenAmount << 16) | i64GreenAmount;
      i64BlueAmount   = (i64BlueAmount << 48)  | (i64BlueAmount << 32)  | (i64BlueAmount << 16)  | i64BlueAmount;

      i64ColorKey     = (i64ColorKey << 48)    | (i64ColorKey << 32)    | (i64ColorKey << 16)    | i64ColorKey;

      pSrcData = pSrcData + iSrcLineSize * iSrcY + 2*iSrcX;
      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iSrcDataPad = iSrcLineSize - (iWidth << 1);
      iDstDataPad = iDstLineSize - (iWidth << 1);

      iRemain = iWidth & 3;
      iWidth  = iWidth >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,         %1 == pSrcData,       %2  == pDstData,
	      // %3 == i64MaskBlue,    %4 == i64MaskGreen,   %5  == i64MaskRed,  %6 == i64ColorKey,
	      // %7 == i64BlueAmount,  %8 == i64GreenAmount, %9  == i64RedAmount,
	      // %10 == iBlueAmount,   %11 == iGreenAmount,  %12 == iRedAmount,
	      //


    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"  // iBlueAmount
                    "pushl    %%edx            \n\t"  // iGreenAmount
                    "pushl    %%eax            \n\t"  // iRedAmount

                    "movl     %0,      %%ecx   \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx   \n\t"
                    "jz      __skip_blt_light_RGB_16  \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi            \n\t"  // pSrcData
                    "pushl    %%edi            \n\t"  // pDstData

                    "movl     %1, %%esi        \n\t"  // %? == pSrcData
                    "movl     %2, %%edi        \n\t"  // %? == pDstData

                    "movl     %10, %%ebx        \n\t"  // %? == iBlueAmount
                    "movl     %11, %%edx        \n\t"  // %? == iGreenAmount
                    "movl     %12, %%eax        \n\t"  // %? == iRedAmount

                    "movq     %6,  %%mm4        \n\t"  // %? == i64ColorKey

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red
                    // mm4  colorkey

        "__do_blt_light_RGB_16:                   \n\t"

                    "movq    (%%esi), %%mm0   \n\t"
                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %3,      %%mm1   \n\t"  // %3 == i64MaskBlue,  put blue  to %%mm1
                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %4,      %%mm2   \n\t"  // %4 == i64MaskGreen, put green to %%mm2
                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %5,      %%mm3   \n\t"  // %5 == i64MaskRed,   put red   to %%mm3

                    "cmpl    $0,      %%ebx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_blue_16        \n\t"

        "__bright_blt_RGB_blue_16:                         \n\t"

                    // handle blue
                    "movq    %7,      %%mm7   \n\t"

                    "psllw   $3,      %%mm1   \n\t"
                    "paddusb %%mm7,   %%mm1   \n\t"
                    "psrlw   $3,      %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_green_16        \n\t"

        "__bright_blt_RGB_green_16:                         \n\t"

                    // handle green
                    "movq    %8,      %%mm7   \n\t"

                    "psrlw   $5,      %%mm2   \n\t"
                    "psllw   $2,      %%mm2   \n\t"
                    "paddusb %%mm7,   %%mm2   \n\t"
                    "psrlw   $2,      %%mm2   \n\t"
                    "psllw   $5,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_red_16        \n\t"

        "__bright_blt_RGB_red_16:                         \n\t"

                    // handle red
                    "movq    %9,      %%mm7   \n\t"

                    "psrlw   $11,     %%mm3   \n\t"
                    "psllw   $3,      %%mm3   \n\t"
                    "paddusb %%mm7,   %%mm3   \n\t"
                    "psrlw   $3,      %%mm3   \n\t"
                    "psllw   $11,     %%mm3   \n\t"

                    "jmp     __next_blt_light_RGB_16  \n\t"

        "__dark_blt_RGB_blue_16:                      \n\t"

                    // handle blue

                    "movq    %7,      %%mm7   \n\t"

                    "psllw   $3,      %%mm1   \n\t"
                    "psubusb %%mm7,   %%mm1   \n\t"
                    "psrlw   $3,      %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "jns      __bright_blt_RGB_green_16     \n\t"

        "__dark_blt_RGB_green_16:                           \n\t"

                    // handle green
                    "movq    %8,      %%mm7   \n\t"

                    "psrlw   $5,      %%mm2   \n\t"
                    "psllw   $2,      %%mm2   \n\t"
                    "psubusb %%mm7,   %%mm2   \n\t"
                    "psrlw   $2,      %%mm2   \n\t"
                    "psllw   $5,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // iRealAmount > 0 ? bright : dark
                    "jns      __bright_blt_RGB_red_16     \n\t"

        "__dark_blt_RGB_red_16:                           \n\t"

                    // handle red

                    "movq    %9,      %%mm7   \n\t"

                    "psrlw   $11,     %%mm3   \n\t"
                    "psllw   $3,      %%mm3   \n\t"
                    "psubusb %%mm7,   %%mm3   \n\t"
                    "psrlw   $3,      %%mm3   \n\t"
                    "psllw   $11,     %%mm3   \n\t"

        "__next_blt_light_RGB_16:                 \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    //"movq    %%mm1, (%%esi)   \n\t"

        "__blt_light_RGB_color_key_16:           \n\t"

                    "movq    (%%edi), %%mm7      \n\t"
                    "movq    %%mm0, %%mm2        \n\t"

                    "pcmpeqw %%mm4, %%mm2        \n\t"  //i64ColorKey
                    "movq    %%mm2, %%mm3        \n\t"

                    "pandn   %%mm1, %%mm3        \n\t"
                    "pand    %%mm2, %%mm7        \n\t"

                    "por     %%mm7, %%mm3        \n\t"
                    "movq    %%mm3, (%%edi)      \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx             \n\t"
                    "jnz	__do_blt_light_RGB_16     \n\t"

                    //
                    // Write back ...
                    //

                    "movl	%%esi, %1         \n\t"
                    "movl	%%edi, %2         \n\t"

                    "popl	%%edi             \n\t"
                    "popl	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_blt_light_RGB_16:                 \n\t"

                    "popl	%%eax             \n\t"
                    "popl	%%edx             \n\t"
                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                      // %0 == iWidth,         %1 == pSrcData,       %2  == pDstData,
                      // %3 == i64MaskBlue,    %4 == i64MaskGreen,   %5  == i64MaskRed,  %6 == i64ColorKey,
                      // %7 == i64BlueAmount,  %8 == i64GreenAmount, %9  == i64RedAmount,
                      // %10 == iBlueAmount,   %11 == iGreenAmount,  %12 == iRedAmount,
                      //
                    : "m"(iWidth),        "m"(pSrcData),       "m"(pDstData),
                      "m"(i64MaskBlue),   "m"(i64MaskGreen),   "m"(i64MaskRed),   "m"(i64ColorKey),
                      "m"(i64BlueAmount), "m"(i64GreenAmount), "m"(i64RedAmount),
                      "m"(iBlueAmount),   "m"(iGreenAmount),   "m"(iRedAmount)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((uint16_t*)pSrcData);
			if (iPixel != iSrcColorKey)
			{
                iB = iPixel & 0x001f;
                iG = (iPixel >> 5) &  0x003f;
                iR = (iPixel >> 11) & 0x001f;

                if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + (iBlueAmount >> 3), 31);
                else iB = OGE_FX_Saturate(iB - (abs(iBlueAmount) >> 3), 31);

                if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + (iGreenAmount >> 2), 63);
                else iG = OGE_FX_Saturate(iG - (abs(iGreenAmount) >> 2), 63);

                if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + (iRedAmount >> 3), 31);
                else iR = OGE_FX_Saturate(iR - (abs(iRedAmount) >> 3), 31);

                *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

			}

			pSrcData += 2;
            pDstData += 2;

		}

         pSrcData += iSrcDataPad;
         pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;


    case 32:
// RGB 8888 Mode ..

      i64MaskRed   = 0x00ff000000ff0000LL;
      i64MaskGreen = 0x0000ff000000ff00LL;
      i64MaskBlue  = 0x000000ff000000ffLL;

      i64ColorKey     = (i64ColorKey << 32)    | i64ColorKey;

      i64BlueAmount   = (i64BlueAmount << 32)  | i64BlueAmount;
      i64GreenAmount  = (i64GreenAmount << 32) | i64GreenAmount;
      i64RedAmount    = (i64RedAmount << 32)   | i64RedAmount;

      pSrcData     = pSrcData + iSrcLineSize * iSrcY + 4*iSrcX;
      pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

      iSrcDataPad  = iSrcLineSize - (iWidth << 2);
      iDstDataPad  = iDstLineSize - (iWidth << 2);

      iRemain      = iWidth & 1;
      iWidth     >>= 1;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,         %1 == pDstData,
	      // %2 == i64MaskBlue,    %3 == i64MaskGreen,   %4  == i64MaskRed,
	      // %5 == i64BlueAmount,  %6 == i64GreenAmount, %7  == i64RedAmount,
	      // %8 == iBlueAmount,    %9 == iGreenAmount,   %10 == iRedAmount

	      // %0 == iWidth,         %1 == pSrcData,       %2  == pDstData,
	      // %3 == i64MaskBlue,    %4 == i64MaskGreen,   %5  == i64MaskRed,  %6 == i64ColorKey,
	      // %7 == i64BlueAmount,  %8 == i64GreenAmount, %9  == i64RedAmount,
	      // %10 == iBlueAmount,   %11 == iGreenAmount,  %12 == iRedAmount,
	      //

    // asm code
		  __asm__ __volatile__ (

                    "pushl    %%ecx            \n\t"
                    "pushl    %%ebx            \n\t"  // iBlueAmount
                    "pushl    %%edx            \n\t"  // iGreenAmount
                    "pushl    %%eax            \n\t"  // iRedAmount

                    "mov     %0,      %%ecx   \n\t"   // %0 == iWidth
                    "cmp     $0,      %%ecx   \n\t"
                    "jz      __skip_blt_light_RGB_32    \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi            \n\t"  // pSrcData
                    "pushl    %%edi            \n\t"  // pDstData

                    "movl     %1, %%esi        \n\t"  // %? == pSrcData
                    "movl     %2, %%edi        \n\t"  // %? == pDstData

                    "movl     %10, %%ebx        \n\t"  // %? == iBlueAmount
                    "movl     %11, %%edx        \n\t"  // %? == iGreenAmount
                    "movl     %12, %%eax        \n\t"  // %? == iRedAmount

                    "movq     %6,  %%mm4        \n\t"  // %? == i64ColorKey

                    // mm0  data
                    // mm1  blue
                    // mm2  green
                    // mm3  red
                    // mm4  colorkey

        "__do_blt_light_RGB_32:                     \n\t"

                    "movq    (%%esi), %%mm0   \n\t"

                    "movq    %%mm0,   %%mm1   \n\t"
                    "pand    %3,      %%mm1   \n\t"  // %3 == i64MaskBlue,  put blue  to %%mm1

                    "movq    %%mm0,   %%mm2   \n\t"
                    "pand    %4,      %%mm2   \n\t"  // %4 == i64MaskGreen, put green to %%mm2

                    "movq    %%mm0,   %%mm3   \n\t"
                    "pand    %5,      %%mm3   \n\t"  // %5 == i64MaskRed,   put red   to %%mm3

                    "cmp     $0,      %%ebx   \n\t"  // iRealAmount,  iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_blue_32     \n\t"

        "__bright_blt_RGB_blue_32:                      \n\t"

                    // handle blue
                    "movq    %7,      %%mm7   \n\t"

                    "paddusb %%mm7,   %%mm1   \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_green_32        \n\t"

        "__bright_blt_RGB_green_32:                         \n\t"

                    // handle green
                    "movq    %8,      %%mm7   \n\t"

                    "psrld   $8,      %%mm2   \n\t"
                    "paddusb %%mm7,   %%mm2   \n\t"
                    "pslld   $8,      %%mm2   \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // iRealAmount > 0 ? bright : dark
                    "js      __dark_blt_RGB_red_32        \n\t"

        "__bright_blt_RGB_red_32:                         \n\t"

                    // handle red
                    "movq    %9,      %%mm7   \n\t"

                    "psrld   $16,     %%mm3   \n\t"
                    "paddusb %%mm7,   %%mm3   \n\t"
                    "pslld   $16,     %%mm3   \n\t"

                    "jmp     __next_blt_light_RGB_32       \n\t"

        "__dark_blt_RGB_blue_32:                           \n\t"

                    // handle blue
                    "movq    %7,    %%mm7     \n\t"
                    "psubusb %%mm7, %%mm1     \n\t"

                    "cmpl    $0,      %%edx   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_blt_RGB_green_32    \n\t"

        "__dark_blt_RGB_green_32:                          \n\t"

                    // handle green
                    "movq    %8,    %%mm7     \n\t"

                    "psrld   $8,    %%mm2     \n\t"
                    "psubusb %%mm7, %%mm2     \n\t"
                    "pslld   $8,    %%mm2     \n\t"

                    "cmpl    $0,      %%eax   \n\t"  // %5 == iRealAmount,  iRealAmount > 0 ? bright : dark
                    "jns      __bright_blt_RGB_red_32      \n\t"

        "__dark_blt_RGB_red_32:                            \n\t"

                    // handle red
                    "movq    %9,    %%mm7     \n\t"

                    "psrld   $16,   %%mm3     \n\t"
                    "psubusb %%mm7, %%mm3     \n\t"
                    "pslld   $16,   %%mm3     \n\t"

        "__next_blt_light_RGB_32:                          \n\t"

                    "por     %%mm2, %%mm1     \n\t"
                    "por     %%mm3, %%mm1     \n\t"

                    //"movq    %%mm1, (%%esi)   \n\t"

        "__blt_light_RGB_color_key_32:           \n\t"

                    "movq    (%%edi), %%mm7      \n\t"
                    "movq    %%mm0, %%mm2        \n\t"

                    "pcmpeqd %%mm4, %%mm2        \n\t"
                    "movq    %%mm2, %%mm3        \n\t"

                    "pandn   %%mm1, %%mm3        \n\t"
                    "pand    %%mm2, %%mm7        \n\t"

                    "por     %%mm7, %%mm3        \n\t"
                    "movq    %%mm3, (%%edi)      \n\t"

                    // Advance to the next four pixels.

                    "addl    $8,    %%esi        \n\t"
                    "addl    $8,    %%edi        \n\t"

                    //
                    // Loop again or break.
                    //
                    "dec	%%ecx             \n\t"
                    "jnz	__do_blt_light_RGB_32 \n\t"

                    // Write back ...
                    //

                    "movl	%%esi, %1         \n\t"
                    "movl	%%edi, %2         \n\t"

                    "popl	%%edi             \n\t"
                    "popl	%%esi             \n\t"

                    "emms                     \n\t"

        "__skip_blt_light_RGB_32:                 \n\t"

                    "popl	%%eax             \n\t"
                    "popl	%%edx             \n\t"
                    "popl	%%ebx             \n\t"
                    "popl	%%ecx             \n\t"

                    : // no output
                      // %0 == iWidth,         %1 == pSrcData,       %2  == pDstData,
                      // %3 == i64MaskBlue,    %4 == i64MaskGreen,   %5  == i64MaskRed,  %6 == i64ColorKey,
                      // %7 == i64BlueAmount,  %8 == i64GreenAmount, %9  == i64RedAmount,
                      // %10 == iBlueAmount,   %11 == iGreenAmount,  %12 == iRedAmount
                    : "m"(iWidth),        "m"(pSrcData),       "m"(pDstData),
                      "m"(i64MaskBlue),   "m"(i64MaskGreen),   "m"(i64MaskRed),   "m"(i64ColorKey),
                      "m"(i64BlueAmount), "m"(i64GreenAmount), "m"(i64RedAmount),
                      "m"(iBlueAmount),   "m"(iGreenAmount),   "m"(iRedAmount)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixel = *((int*)pSrcData);
			if (iPixel != iSrcColorKey)
			{
                iB = iPixel & 0xff;
                iG = (iPixel >> 8) &  0xff;
                iR = (iPixel >> 16) & 0xff;

                if (iBlueAmount >= 0) iB = OGE_FX_Saturate(iB + iBlueAmount, 255);
                else iB = OGE_FX_Saturate(iB - abs(iBlueAmount), 255);

                if (iGreenAmount >= 0) iG = OGE_FX_Saturate(iG + iGreenAmount, 255);
                else iG = OGE_FX_Saturate(iG - abs(iGreenAmount), 255);

                if (iRedAmount >= 0) iR = OGE_FX_Saturate(iR + iRedAmount, 255);
                else iR = OGE_FX_Saturate(iR - abs(iRedAmount), 255);

                *((int*)pDstData) = (iR << 16) | (iG << 8) | iB;
			}

            pSrcData += 4;
			pDstData += 4;

		}

        pSrcData += iSrcDataPad;
        pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break; // end of 8888 mode

    } // end of switch case

}

void OGE_FX_AlphaBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, uint8_t iAlpha)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iSrcDataPad, iDstDataPad, iPixelSrc, iPixelDst;

	uint8_t iB, iG, iR;

	//uint32_t i32Amount;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64X, i64Alpha, i64AlphaDiv4;

	int iUseColorKey = (iSrcColorKey == -1) ? 0 : 1;

	uint64_t i64ColorKey = iSrcColorKey;
	//uint16_t i16ColorKey = iSrcColorKey & 0x0000ffff;

	i64Alpha = iAlpha;

	switch (iBPP)
	{

	case 16:
    // RGB 565 Mode ..

        i64MaskRed   = 0xf800f800f800f800LL;
        i64MaskGreen = 0x07e007e007e007e0LL;
        i64MaskBlue  = 0x001f001f001f001fLL;
        i64X         = 0x0040004000400040LL;    // ...

        i64AlphaDiv4 = iAlpha >> 2;

        i64ColorKey  = (i64ColorKey << 48)  | (i64ColorKey << 32)  | (i64ColorKey << 16)  | i64ColorKey;
        i64Alpha     = (i64Alpha << 48)     | (i64Alpha << 32)     | (i64Alpha << 16)     | i64Alpha;
        i64AlphaDiv4 = (i64AlphaDiv4 << 48) | (i64AlphaDiv4 << 32) | (i64AlphaDiv4 << 16) | i64AlphaDiv4;

        pSrcData = pSrcData + iSrcLineSize * iSrcY + 2*iSrcX;
        pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

        iSrcDataPad = iSrcLineSize - (iWidth << 1);
        iDstDataPad = iDstLineSize - (iWidth << 1);

        iRemain = iWidth & 3;
        iWidth  = iWidth >> 2;

        while (iHeight > 0)
        {
            // %0  == iWidth,
            // %1  == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
            // %4  == i64AlphaDiv4, %5 == i64Alpha,      %6 == i64X
            // %7  == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
            // %10 == iSrcColorKey, %11 == iUseColorKey

            // asm code
            __asm__ __volatile__ (

                    "pushl    %%ecx              \n\t"
                    "pushl    %%ebx              \n\t"
                    "pushl    %%edx              \n\t"

                    "movl     %0,      %%ecx     \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx     \n\t"
                    "jz      __skip_ablend_16    \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi              \n\t"
                    "pushl    %%edi              \n\t"

                    "movl     %10,%%edx          \n\t"  // %? == iSrcColorKey
                    "movl     %11,%%ebx          \n\t"  // %? == iUseColorKey

                    "movl     %7, %%esi          \n\t"  // %? == pSrcData
                    "movl     %8, %%edi          \n\t"  // %? == pDstData

                    //   mm1~mm3: bgr of src
                    //   mm4~mm6: bgr of dst
                    //   mm0: src
                    //   mm7: dst

            "__do_ablend_16:                     \n\t"

                    "movq     (%%esi), %%mm0     \n\t"

                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __start_ablend_16  \n\t"

                    "movq     %%mm0, %%mm1       \n\t"
                    "movd     %%mm1, %%eax       \n\t"

                    "cmpw     %%dx,  %%ax        \n\t"
                    "jne      __start_ablend_16  \n\t"
                    "shrl     $16,  %%eax        \n\t"
                    "cmpw     %%dx,  %%ax        \n\t"
                    "jne      __start_ablend_16  \n\t"

                    "psrlq    $32,  %%mm1        \n\t"
                    "movd     %%mm1,%%eax        \n\t"

                    "cmpw     %%dx,  %%ax        \n\t"
                    "jne      __start_ablend_16  \n\t"
                    "shrl     $16,  %%eax        \n\t"
                    "cmpw     %%dx,  %%ax        \n\t"
                    "jne      __start_ablend_16  \n\t"

                    "jmp      __final_ablend_16  \n\t"

            "__start_ablend_16:                  \n\t"

                    "movq     (%%edi), %%mm7     \n\t"

                    // handle blue
                    "movq     %%mm0,   %%mm1     \n\t"
                    "pand     %1,      %%mm1     \n\t"  // %? == i64MaskBlue,  put blue to %%mm1
                    "movq     %%mm7,   %%mm4     \n\t"
                    "pand     %1,      %%mm4     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddw    %6,      %%mm1     \n\t"  // %? == i64X, src + 64
                    "psubw    %%mm4,   %%mm1     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm1     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrlw    $8,      %%mm1     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddw    %%mm4,   %%mm1     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubw    %4,      %%mm1     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)

                    // handle green
                    "movq     %%mm0,   %%mm2     \n\t"
                    "pand     %2,      %%mm2     \n\t"  // %? == i64MaskGreen,  put green to %%mm2
                    "psrlw    $5,      %%mm2     \n\t"
                    "movq     %%mm7,   %%mm5     \n\t"
                    "pand     %2,      %%mm5     \n\t"
                    "psrlw    $5,      %%mm5     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddw    %6,      %%mm2     \n\t"  // %? == i64X, src + 64
                    "psubw    %%mm5,   %%mm2     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm2     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrlw    $8,      %%mm2     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddw    %%mm5,   %%mm2     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubw    %4,      %%mm2     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)
                    "psllw    $5,      %%mm2     \n\t"

                    // handle red
                    "movq     %%mm0,   %%mm3     \n\t"
                    "pand     %3,      %%mm3     \n\t"  // %? == i64MaskRed,  put red to %%mm3
                    "psrlw    $11,     %%mm3     \n\t"
                    "movq     %%mm7,   %%mm6     \n\t"
                    "pand     %3,      %%mm6     \n\t"
                    "psrlw    $11,     %%mm6     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddw    %6,      %%mm3     \n\t"  // %? == i64X, src + 64
                    "psubw    %%mm6,   %%mm3     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm3     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrlw    $8,      %%mm3     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddw    %%mm6,   %%mm3     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubw    %4,      %%mm3     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)
                    "psllw    $11,     %%mm3     \n\t"

                    // finish
                    "por      %%mm2,   %%mm1     \n\t"
                    "por      %%mm3,   %%mm1     \n\t"

                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __nokey_ablend_16  \n\t"

            "__srckey_ablend_16:                 \n\t"

                    "movq     %%mm0,   %%mm2     \n\t"

            "__handlekey_ablend_16:              \n\t"

                    "pcmpeqw  %9,      %%mm2     \n\t"  // %? == i64ColorKey
                    "movq     %%mm2,   %%mm3     \n\t"

                    "pandn    %%mm1,   %%mm3     \n\t"
                    "pand     %%mm2,   %%mm7     \n\t"

                    "por      %%mm7,   %%mm3     \n\t"
                    "movq     %%mm3,   (%%edi)   \n\t"

                    "jmp      __final_ablend_16  \n\t"

            "__nokey_ablend_16:                  \n\t"

                    "movq     %%mm1,   (%%edi)   \n\t"

            "__final_ablend_16:                  \n\t"

                    // go to the next four pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_ablend_16       \n\t"

                    //
                    // return ...
                    //

                    "movl	%%esi, %7            \n\t"
                    "movl	%%edi, %8            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

            "__skip_ablend_16:                   \n\t"

                    "popl	%%edx                \n\t"
                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0  == iWidth,
                      // %1  == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
                      // %4  == i64AlphaDiv4, %5 == i64Alpha,      %6 == i64X
                      // %7  == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
                      // %10 == iSrcColorKey, %11 == iUseColorKey
                    : "m"(iWidth),
                      "m"(i64MaskBlue),  "m"(i64MaskGreen), "m"(i64MaskRed),
                      "m"(i64AlphaDiv4), "m"(i64Alpha),     "m"(i64X),
                      "m"(pSrcData),     "m"(pDstData),     "m"(i64ColorKey),
                      "m"(iSrcColorKey), "m"(iUseColorKey)
                    : "memory"
            );


            // handle the remain

            for (i=1; i<=iRemain; i++)
            {
                iPixelSrc = *((uint16_t*)pSrcData);
                if (iPixelSrc != iSrcColorKey || iUseColorKey != 1)
                {

                    iPixelDst = *((uint16_t*)pDstData);

                    // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA / 4)

                    iB = ((iAlpha * ((iPixelSrc & 0x001f) + 64 - (iPixelDst & 0x001f))) >> 8) +
                        (iPixelDst & 0x001f) - (iAlpha >> 2);

                    iG = ((iAlpha * (((iPixelSrc & 0x07e0) >> 5) + 64 - ((iPixelDst & 0x07e0) >> 5))) >> 8) +
                        ((iPixelDst & 0x07e0) >> 5) - (iAlpha >> 2);

                    iR = ((iAlpha * (((iPixelSrc & 0xf800) >> 11) + 64 - ((iPixelDst & 0xf800) >> 11))) >> 8) +
                        ((iPixelDst & 0xf800) >> 11) - (iAlpha >> 2);

                    *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
                }

                pSrcData += 2;
                pDstData += 2;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        } // end of while (iHeight > 0)

    break;



    case 32:

    // RGB 8888 Mode ..

        /*

        //i64MaskRed   = 0x00ff000000ff0000LL;
        //i64MaskGreen = 0x0000ff000000ff00LL;
        //i64MaskBlue  = 0x000000ff000000ffLL;
        //i64X         = 0x0040004000400040LL;

        //i64X         = 0x0040004000400040LL;    // ...

        i64ColorKey  = (i64ColorKey << 32)  | i64ColorKey;
        //i64Alpha     = (i64Alpha << 32)     | i64Alpha;

        i64Alpha     = (i64Alpha << 48)     | (i64Alpha << 32)     | (i64Alpha << 16)     | i64Alpha;
        //i64AlphaDiv4 = (i64AlphaDiv4 << 48) | (i64AlphaDiv4 << 32) | (i64AlphaDiv4 << 16) | i64AlphaDiv4;

        //i64AlphaDiv4 = (i64AlphaDiv4 << 32) | i64AlphaDiv4;

        pSrcData     = pSrcData + iSrcLineSize * iSrcY + 4*iSrcX;
        pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

        iSrcDataPad  = iSrcLineSize - (iWidth << 2);
        iDstDataPad  = iDstLineSize - (iWidth << 2);

        iRemain      = iWidth & 1;
        iWidth     >>= 1;

        while (iHeight > 0)
        {
            // %0  == iWidth,
            // %1  == iUseColorKey,  %2 == iSrcColorKey,  %3 == i64ColorKey
            // %4  == pSrcData,      %5 == pDstData,      %6 == i64Alpha

            // asm code
            __asm__ __volatile__ (

                    "pushl    %%ecx              \n\t"
                    "pushl    %%ebx              \n\t"
                    "pushl    %%edx              \n\t"

                    "movl     %0,      %%ecx     \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx     \n\t"
                    "jz      __skip_ablend_32    \n\t"  // if this row is less than 2 then jump to next

                    "pushl    %%esi              \n\t"
                    "pushl    %%edi              \n\t"

                    "movl     %1, %%ebx          \n\t"  // %? == iUseColorKey
                    "movl     %2, %%edx          \n\t"  // %? == iSrcColorKey

                    "movl     %4, %%esi          \n\t"  // %? == pSrcData
                    "movl     %5, %%edi          \n\t"  // %? == pDstData

                    //   mm1~mm3: bgr of src
                    //   mm4~mm6: bgr of dst
                    //   mm0: src
                    //   mm7: dst

            "__do_ablend_32:                     \n\t"

                    "movq     (%%esi), %%mm1     \n\t"

                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "movq     %%mm1, %%mm2       \n\t"
                    "movd     %%mm2, %%eax       \n\t"

                    "cmpl     %%edx, %%eax       \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "psrlq    $32,  %%mm2        \n\t"
                    "movd     %%mm2,%%eax        \n\t"

                    "cmpl     %%edx, %%eax       \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "jmp      __final_ablend_32  \n\t"

            "__start_ablend_32:                  \n\t"

                    "movq     (%%edi), %%mm0     \n\t"
                    "movq     %%mm0,   %%mm6     \n\t"

                    "pxor     %%mm7,   %%mm7     \n\t"

                    "movq     %%mm0,   %%mm2     \n\t" // second copy of dst
                    "movq     %%mm1,   %%mm3     \n\t" // second copy of src

                    "movq       %%mm0, %%mm4     \n\t" // third copy of dst

                    "punpcklbw  %%mm7, %%mm0     \n\t" // mm0 = unpacked low half of dst
                    "punpcklbw  %%mm7, %%mm1     \n\t" // mm1 = unpacked low half of src
                    "punpckhbw  %%mm7, %%mm2     \n\t" // mm2 = unpacked high half of dst
                    "punpckhbw  %%mm7, %%mm3     \n\t" // mm3 = unpacked high half of src

                    "psubw      %%mm0, %%mm1     \n\t" // mm0 = low half of (src-dst)
                    "psubw      %%mm2, %%mm3     \n\t" // mm2 = high half of (src-dst)

                    "pmullw     %6,    %%mm1     \n\t" // low (src-dst)*alpha
                    "pmullw     %6,    %%mm3     \n\t" // high (src-dst)*alpha
                    "psrlw      $8,    %%mm1     \n\t" // low (src-dst)*alpha / 256
                    "psrlw      $8,    %%mm3     \n\t" // high (src-dst)*alpha / 256

                    //"paddb      %%mm0, %%mm1     \n\t"
                    //"paddb      %%mm2, %%mm3     \n\t"

                    "packuswb   %%mm3, %%mm1     \n\t" // combine with unsigned saturation
                    "paddb      %%mm4, %%mm1     \n\t" // (src-dst)*alpha / 256 + dst
                                                       // (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    // finish
                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __nokey_ablend_32  \n\t"

            "__srckey_ablend_32:                 \n\t"

                    "movq     %%mm1,   %%mm2     \n\t"

            "__handlekey_ablend_32:              \n\t"

                    "pcmpeqd  %3,      %%mm2     \n\t"  // %? == i64ColorKey
                    "movq     %%mm2,   %%mm3     \n\t"

                    "pandn    %%mm1,   %%mm3     \n\t"
                    "pand     %%mm2,   %%mm6     \n\t"

                    "por      %%mm6,   %%mm3     \n\t"
                    "movq     %%mm3,   (%%edi)   \n\t"

                    "jmp      __final_ablend_32  \n\t"

            "__nokey_ablend_32:                  \n\t"

                    "movq     %%mm1,   (%%edi)   \n\t"

            "__final_ablend_32:                  \n\t"

                    // go to the next two pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_ablend_32       \n\t"

                    //
                    // return ...
                    //

                    "movl	%%esi, %4            \n\t"
                    "movl	%%edi, %5            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

            "__skip_ablend_32:                   \n\t"

                    "popl	%%edx                \n\t"
                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0  == iWidth,
                      // %1  == iUseColorKey,  %2 == iSrcColorKey,  %3 == i64ColorKey
                      // %4  == pSrcData,      %5 == pDstData,      %6 == i64Alpha
                    : "m"(iWidth),
                      "m"(iUseColorKey),  "m"(iSrcColorKey), "m"(i64ColorKey),
                      "m"(pSrcData),      "m"(pDstData),     "m"(i64Alpha)
                    : "memory"
            );

            // handle the remain
            for (i=1; i<=iRemain; i++)
            {
                iPixelSrc = *((int*)pSrcData);
                if (iPixelSrc != iSrcColorKey || iUseColorKey != 1)
                {
                    iPixelDst = *((int*)pDstData);

                    // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA / 4)

                    iB = ((iAlpha * ((iPixelSrc & 0x0000ff) - (iPixelDst & 0x0000ff))) >> 8) +
					(iPixelDst & 0x00ff);

                    iG = ((iAlpha * (((iPixelSrc & 0x00ff00) >> 8) - ((iPixelDst & 0x00ff00) >> 8))) >> 8) +
                        ((iPixelDst & 0x00ff00) >> 8);

                    iR = ((iAlpha * (((iPixelSrc & 0xff0000) >> 16) - ((iPixelDst & 0xff0000) >> 16))) >> 8) +
                        ((iPixelDst & 0xff0000) >> 16);

                    *((int*)pDstData) = (iR << 16) | (iG << 8) | iB;
                }

                pSrcData += 4;
                pDstData += 4;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        } // end of while (iHeight > 0)


        */


        // the code above is fast and short but its effect is not good...
        // so let's use back the old way ...

        i64MaskRed   = 0x00ff000000ff0000LL;
        i64MaskGreen = 0x0000ff000000ff00LL;
        i64MaskBlue  = 0x000000ff000000ffLL;
        i64X         = 0x0000010000000100LL;

        i64AlphaDiv4 = i64Alpha;

        i64ColorKey  = (i64ColorKey << 32)  | i64ColorKey;
        i64Alpha     = (i64Alpha << 32)     | i64Alpha;
        i64AlphaDiv4 = (i64AlphaDiv4 << 32) | i64AlphaDiv4;

        pSrcData     = pSrcData + iSrcLineSize * iSrcY + 4*iSrcX;
        pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

        iSrcDataPad  = iSrcLineSize - (iWidth << 2);
        iDstDataPad  = iDstLineSize - (iWidth << 2);

        iRemain      = iWidth & 1;
        iWidth     >>= 1;

        while (iHeight > 0)
        {
            // %0  == iWidth,
            // %1  == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
            // %4  == i64AlphaDiv4, %5 == i64Alpha,      %6 == i64X
            // %7  == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
            // %10 == iSrcColorKey, %11 == iUseColorKey

            // asm code
            __asm__ __volatile__ (

                    "pushl    %%ecx              \n\t"
                    "pushl    %%ebx              \n\t"
                    "pushl    %%edx              \n\t"

                    "movl     %0,      %%ecx     \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx     \n\t"
                    "jz      __skip_ablend_32    \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi              \n\t"
                    "pushl    %%edi              \n\t"

                    "movl     %10,%%edx          \n\t"  // %? == iSrcColorKey
                    "movl     %11,%%ebx          \n\t"  // %? == iUseColorKey

                    "movl     %7, %%esi          \n\t"  // %? == pSrcData
                    "movl     %8, %%edi          \n\t"  // %? == pDstData

                    //   mm1~mm3: bgr of src
                    //   mm4~mm6: bgr of dst
                    //   mm0: src
                    //   mm7: dst

            "__do_ablend_32:                     \n\t"

                    "movq     (%%esi), %%mm0     \n\t"

                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "movq     %%mm0, %%mm2       \n\t"
                    "movd     %%mm2, %%eax       \n\t"

                    "cmpl     %%edx, %%eax       \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "psrlq    $32,  %%mm2        \n\t"
                    "movd     %%mm2,%%eax        \n\t"

                    "cmpl     %%edx, %%eax       \n\t"
                    "jne      __start_ablend_32  \n\t"

                    "jmp      __final_ablend_32  \n\t"

            "__start_ablend_32:                  \n\t"

                    "movq     (%%edi), %%mm7     \n\t"

                    // handle blue
                    "movq     %%mm0,   %%mm1     \n\t"
                    "pand     %1,      %%mm1     \n\t"  // %? == i64MaskBlue,  put blue to %%mm1
                    "movq     %%mm7,   %%mm4     \n\t"
                    "pand     %1,      %%mm4     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddd    %6,      %%mm1     \n\t"  // %? == i64X, src + 64
                    "psubd    %%mm4,   %%mm1     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm1     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrld    $8,      %%mm1     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddd    %%mm4,   %%mm1     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubd    %4,      %%mm1     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)

                    // handle green
                    "movq     %%mm0,   %%mm2     \n\t"
                    "pand     %2,      %%mm2     \n\t"  // %? == i64MaskGreen,  put green to %%mm2
                    "psrld    $8,      %%mm2     \n\t"
                    "movq     %%mm7,   %%mm5     \n\t"
                    "pand     %2,      %%mm5     \n\t"
                    "psrld    $8,      %%mm5     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddd    %6,      %%mm2     \n\t"  // %? == i64X, src + 64
                    "psubd    %%mm5,   %%mm2     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm2     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrld    $8,      %%mm2     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddd    %%mm5,   %%mm2     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubd    %4,      %%mm2     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)
                    "pslld    $8,      %%mm2     \n\t"

                    // handle red
                    "movq     %%mm0,   %%mm3     \n\t"
                    "pand     %3,      %%mm3     \n\t"  // %? == i64MaskRed,  put red to %%mm3
                    "psrld    $16,     %%mm3     \n\t"
                    "movq     %%mm7,   %%mm6     \n\t"
                    "pand     %3,      %%mm6     \n\t"
                    "psrld    $16,     %%mm6     \n\t"
                                                        // result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA/4)
                    "paddd    %6,      %%mm3     \n\t"  // %? == i64X, src + 64
                    "psubd    %%mm6,   %%mm3     \n\t"  // src + 64 - dst
                    "pmullw   %5,      %%mm3     \n\t"  // %? == i64Alpha, (src + 64 - dst) * Alpha
                    "psrld    $8,      %%mm3     \n\t"  // (src + 64 - dst) * Alpha / 256
                    "paddd    %%mm6,   %%mm3     \n\t"  // (src + 64 - dst) * Alpha / 256 + dst
                    "psubd    %4,      %%mm3     \n\t" // %? == i64AlphaDiv4 (src + 64 - dst) * Alpha / 256 + dst - (ALPHA/4)
                    "pslld    $16,     %%mm3     \n\t"

                    // finish
                    "por      %%mm2,   %%mm1     \n\t"
                    "por      %%mm3,   %%mm1     \n\t"

                    "cmpl     $1,  %%ebx         \n\t"
                    "jne      __nokey_ablend_32  \n\t"

            "__srckey_ablend_32:                 \n\t"

                    "movq     %%mm0,   %%mm2     \n\t"

            "__handlekey_ablend_32:              \n\t"

                    "pcmpeqd  %9,      %%mm2     \n\t"  // %? == i64ColorKey
                    "movq     %%mm2,   %%mm3     \n\t"

                    "pandn    %%mm1,   %%mm3     \n\t"
                    "pand     %%mm2,   %%mm7     \n\t"

                    "por      %%mm7,   %%mm3     \n\t"
                    "movq     %%mm3,   (%%edi)   \n\t"

                    "jmp      __final_ablend_32  \n\t"

            "__nokey_ablend_32:                  \n\t"

                    "movq     %%mm1,   (%%edi)   \n\t"

            "__final_ablend_32:                  \n\t"

                    // go to the next four pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_ablend_32       \n\t"

                    //
                    // return ...
                    //

                    "movl	%%esi, %7            \n\t"
                    "movl	%%edi, %8            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

            "__skip_ablend_32:                   \n\t"

                    "popl	%%edx                \n\t"
                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0  == iWidth,
                      // %1  == i64MaskBlue,  %2 == i64MaskGreen,  %3 == i64MaskRed
                      // %4  == i64AlphaDiv4, %5 == i64Alpha,      %6 == i64X
                      // %7  == pSrcData,     %8 == pDstData,      %9 == i64ColorKey
                      // %10 == iSrcColorKey, %11 == iUseColorKey
                    : "m"(iWidth),
                      "m"(i64MaskBlue),  "m"(i64MaskGreen), "m"(i64MaskRed),
                      "m"(i64AlphaDiv4), "m"(i64Alpha),     "m"(i64X),
                      "m"(pSrcData),     "m"(pDstData),     "m"(i64ColorKey),
                      "m"(iSrcColorKey), "m"(iUseColorKey)
                    : "memory"
            );

            // handle the remain
            for (i=1; i<=iRemain; i++)
            {
                iPixelSrc = *((int*)pSrcData);
                if (iPixelSrc != iSrcColorKey || iUseColorKey != 1)
                {
                    iPixelDst = *((int*)pDstData);

                    // result = (ALPHA * ((src+256) - dst)) / 256 + dst - ALPHA

                    iB = ((iAlpha * ((iPixelSrc & 0x0000ff) + 256 - (iPixelDst & 0x0000ff))) >> 8) +
                        (iPixelDst & 0x00ff) - iAlpha;

                    iG = ((iAlpha * (((iPixelSrc & 0x00ff00) >> 8) + 256 - ((iPixelDst & 0x00ff00) >> 8))) >> 8) +
                        ((iPixelDst & 0x00ff00) >> 8) - iAlpha;

                    iR = ((iAlpha * (((iPixelSrc & 0xff0000) >> 16) + 256 - ((iPixelDst & 0xff0000) >> 16))) >> 8) +
                        ((iPixelDst & 0xff0000) >> 16) - iAlpha;

                    *((int*)pDstData) = (iR << 16) | (iG << 8) | iB;
                }

                pSrcData += 4;
                pDstData += 4;

            }

            pSrcData += iSrcDataPad;
            pDstData += iDstDataPad;

            iHeight--;

        }



        break; // end of 8888 mode


    } // end of switch case
}

void OGE_FX_LightMaskBlend(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iSrcDataPad, iDstDataPad, iPixelSrc, iPixelDst, iAmount, iGreenAmount;
	uint8_t iB, iG, iR;

	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64One;

	i64One = 1;


	switch (iBPP)
	{

    case 8:
    case 24:
    case 32:

        if(iBPP == 8)
        {
            pSrcData += iSrcY * iSrcLineSize + iSrcX;
            pDstData += iDstY * iDstLineSize + iDstX;
        }
        else if(iBPP == 24)
        {
            iWidth = iWidth * 3;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 3;
            pDstData += iDstY * iDstLineSize + iDstX * 3;
        }
        else
        {
            iWidth <<= 2;
            pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;
            pDstData += iDstY * iDstLineSize + iDstX * 4;
        }

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        //i64Amount   = (i64Amount << 48) | (i64Amount << 32) | (i64Amount << 16) | i64Amount;
        //i64Mask     = (i64Mask << 48)   | (i64Mask << 32)   | (i64Mask << 16)   | i64Mask;

        //i64Amount = (i64Amount << 56) | (i64Amount << 48) | (i64Amount << 40) | (i64Amount << 32) |
        //              (i64Amount << 24) | (i64Amount << 16) | (i64Amount << 8)  |  i64Amount;

        i64One = (i64One << 48) | (i64One << 32) | (i64One << 16)  |  i64One;


        // asm code

        __asm__ __volatile__ (

                // %0 == iHeight,     %1 == iWidth
                // %2 == pSrcData,    %3 == pDstData,
                // %4 == iSrcDataPad, %5 == iDstDataPad, %6 == i64One

                "pxor    %%mm7,   %%mm7          \n\t"

                "movq    %6,      %%mm6          \n\t"

                "xorl    %%eax,   %%eax          \n\t"
                "xorl    %%edx,   %%edx          \n\t"

                "cmpl    $0,      %0             \n\t"    // %? == iHeight
				"jbe     __end_light_sub_8       \n\t"

                "movl    %2,      %%esi          \n\t"  // %? == pSrcData,
                "movl    %3,      %%edi          \n\t"  // %? == pDstData,


        "__do_next_line_light_sub_8:             \n\t"

                "movl    %1,      %%ecx          \n\t"  // %? == iWidth

                "cmpl    $8,      %%ecx          \n\t"
                "jb      __skip_mmx_light_sub_8  \n\t"

        "__do_mmx_light_sub_8:                   \n\t"

                "movq    (%%esi), %%mm0          \n\t" // src
                "movq    (%%edi), %%mm1          \n\t" // dst
                "movq    %%mm0,   %%mm2          \n\t" // make a copy
                "movq    %%mm1,   %%mm3          \n\t" // make a copy
                "movq    %%mm1,   %%mm4          \n\t" // make a copy

                "punpcklbw %%mm7, %%mm0          \n\t" // mm1 = unpacked low half of src
                "paddusw %%mm6,   %%mm0          \n\t" // 0~255 -> 1~256
                "punpcklbw %%mm7, %%mm1          \n\t" // mm1 = unpacked low half of dst

                "punpckhbw %%mm7, %%mm2          \n\t" // mm2 = unpacked high half of src
                "paddusw %%mm6,   %%mm2          \n\t" // 0~255 -> 1~256
                "punpckhbw %%mm7, %%mm3          \n\t" // mm2 = unpacked high half of dst


                "pmullw  %%mm1,   %%mm0          \n\t" // amount * (dst)
                "pmullw  %%mm3,   %%mm2          \n\t" // amount * (dst)
                "psrlw   $8,      %%mm0          \n\t" // amount * (dst) / 256
                "psrlw   $8,      %%mm2          \n\t" // amount * (dst) / 256
                "packuswb  %%mm2, %%mm0          \n\t" // combine with unsigned saturation
                "psubusb %%mm0,   %%mm4          \n\t" // dst - amount * (dst) / 256

                // fin
                "movq    %%mm4,   (%%edi)        \n\t"

                "addl    $8,      %%edi          \n\t"
                "addl    $8,      %%esi          \n\t"

                "subl    $8,      %%ecx          \n\t"
                "cmpl    $8,      %%ecx          \n\t"
                "jae     __do_mmx_light_sub_8    \n\t"

        "__skip_mmx_light_sub_8:                 \n\t"

                "cmpl    $1,      %%ecx          \n\t"
                "jb      __skip_light_sub_8      \n\t"

        "__do_light_sub_8:                       \n\t"

                "movb    (%%esi), %%al           \n\t"
                "movb    (%%edi), %%dl           \n\t"

                "movb    %%dl,    %%dh           \n\t"

                "mulb    %%dh                    \n\t"
                "shrw    $8,      %%ax           \n\t"
                "subb    %%al,    %%dl           \n\t"
                "jc       __light_sub_min_8      \n\t"

                "movb     %%dl, (%%edi)          \n\t"
                "jmp      __next_light_sub_8     \n\t"

        "__light_sub_min_8:                      \n\t"

                "movb     $0, (%%edi)            \n\t"

        "__next_light_sub_8:                     \n\t"

                "incl     %%esi                  \n\t"
                "incl     %%edi                  \n\t"

                "decl     %%ecx                  \n\t"
                "jnz     __do_light_sub_8        \n\t"

       "__skip_light_sub_8:                      \n\t"

                "addl     %4, %%esi              \n\t"   // %? == iSrcDataPad,
                "addl     %5, %%edi              \n\t"   // %? == iDstDataPad,

                "decl     %0                     \n\t"  // %? == iHeight
                "jnz     __do_next_line_light_sub_8   \n\t"


        "__end_light_sub_8:                      \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,     %1 == iWidth
                // %2 == pSrcData,    %3 == pDstData,
                // %4 == iSrcDataPad, %5 == iDstDataPad, %6 == i64One
                : "m"(iHeight), "m"(iWidth),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iSrcDataPad), "m"(iDstDataPad), "m"(i64One)
                : "%eax", "%edx", "%esi", "%edi", "%ecx", "memory"


        );

    break;


	case 16:
// RGB 565 Mode ..

      i64MaskRed   = 0xf800f800f800f800LL;
      i64MaskGreen = 0x07e007e007e007e0LL;
      i64MaskBlue  = 0x001f001f001f001fLL;

      //i64ColorKey  = (i64ColorKey << 48) | (i64ColorKey << 32) | (i64ColorKey << 16) | i64ColorKey;

      i64One = (i64One << 48) | (i64One << 32) | (i64One << 16)  |  i64One;

      pSrcData = pSrcData + iSrcLineSize * iSrcY + 2*iSrcX;
      pDstData = pDstData + iDstLineSize * iDstY + 2*iDstX;

      iSrcDataPad = iSrcLineSize - (iWidth << 1);
      iDstDataPad = iDstLineSize - (iWidth << 1);

      // ensure the width is a multiple of four.
      iRemain = iWidth & 3;
      iWidth  = iWidth >> 2;

      while (iHeight > 0)
	  {
	      // %0 == iWidth,
	      // %1 == pSrcData,     %2 == pDstData,
	      // %3 == i64MaskBlue,  %4 == i64MaskGreen,   %5 == i64MaskRed , %6 == i64One

    // asm code
		  __asm__ __volatile__ (

                    //"pushl    %%edx            \n\t"
                    "pushl    %%ecx              \n\t"
                    "movl     %0,      %%ecx     \n\t"  // %? == iWidth
                    "cmpl     $0,      %%ecx     \n\t"
                    "jz      __skip_light_sub_16 \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi              \n\t"
                    "pushl    %%edi              \n\t"

                    "movl     %1, %%esi          \n\t"  // %? == pSrcData
                    "movl     %2, %%edi          \n\t"  // %? == pDstData

                    "movq     %6,      %%mm6     \n\t"

                    //   mm1: amount
                    //   mm4~mm6: bgr of dst
                    //   mm0: src
                    //   mm7: dst

        "__do_light_sub_16:                      \n\t"

                    "movq    (%%esi), %%mm0      \n\t"
                    "movq    (%%edi), %%mm7      \n\t"


                    "movq    %%mm0,   %%mm1      \n\t"
                    "movq    %%mm1,   %%mm3      \n\t"

                    "pand    %3,      %%mm1      \n\t"  // %? == i64MaskBlue,  get amount
                    "paddusw %%mm6,   %%mm1      \n\t"  // 0~31 -> 1~32
                    "pand    %4,      %%mm3      \n\t"  // %? == i64MaskGreen,
                    "psrlw   $5,      %%mm3      \n\t"  // get special green amount
                    "paddusw %%mm6,   %%mm3      \n\t"  // 0~63 -> 1~64

                    // handle blue                      // result := dst - amount * dst / 32
                    "movq    %%mm7,   %%mm2      \n\t"
                    "pand    %3,      %%mm2      \n\t"  // %? == i64MaskBlue,  get blue
                    "movq    %%mm2,   %%mm4      \n\t"  // backup it
                    "pmullw  %%mm1,   %%mm2      \n\t"  // amount * dst

                    "psrlw   $5,      %%mm2      \n\t"  // amount * dst / 32
                    "psubusw %%mm2,   %%mm4      \n\t"  // dst - amount * dst / 32

                    // handle green
                    "movq    %%mm7,   %%mm2      \n\t"
                    "pand    %4,      %%mm2      \n\t" // %? == i64MaskGreen
                    "psrlw   $5,      %%mm2      \n\t"
                    "movq    %%mm2,   %%mm5      \n\t"  // backup it
                    "pmullw  %%mm3,   %%mm2      \n\t"  // amount * dst

                    "psrlw   $6,      %%mm2      \n\t"  // amount * dst / 64
                    "psubusw %%mm2,   %%mm5      \n\t"  // dst - amount * dst / 64
                    "psllw   $5,      %%mm5      \n\t"  // remove to middle

                    "por     %%mm5,   %%mm4      \n\t"

                    // handle red
                    "movq    %%mm7,   %%mm2      \n\t"
                    "pand    %5,      %%mm2      \n\t"   // %? == i64MaskRed
                    "psrlw   $11,     %%mm2      \n\t"
                    "movq    %%mm2,   %%mm5      \n\t"
                    "pmullw  %%mm1,   %%mm2      \n\t"

                    "psrlw   $5,      %%mm2      \n\t"  // amount * dst / 32
                    "psubusw %%mm2,   %%mm5      \n\t"
                    "psllw   $11,     %%mm5      \n\t"

                    "por     %%mm5,   %%mm4      \n\t"

                    //"por     %%mm6,   %%mm4      \n\t"

                    "movq    %%mm4,   (%%edi)    \n\t"

            "__next_light_sub_16:                \n\t"

                    // Advance to the next four pixels.

                    "addl     $8,    %%esi       \n\t"
                    "addl     $8,    %%edi       \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_light_sub_16    \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %1            \n\t"   // %? == pSrcData
                    "movl	%%edi, %2            \n\t"   // %? == pDstData

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"


        "__skip_light_sub_16:                    \n\t"

                    "emms                        \n\t"

                    "popl	%%ecx                \n\t"
                    //"popl	%%edx                  \n\t"

                    : // no output
                    // %0 == iWidth,
                    // %1 == pSrcData,     %2 == pDstData,
                    // %3 == i64MaskBlue,  %4 == i64MaskGreen,   %5 == i64MaskRed , %6 == i64One
                    : "m"(iWidth), "m"(pSrcData), "m"(pDstData),
                      "m"(i64MaskBlue), "m"(i64MaskGreen), "m"(i64MaskRed), "m"(i64One)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixelSrc = *((uint16_t*)pSrcData);

            iPixelDst = *((uint16_t*)pDstData);
            iAmount = iPixelSrc & 0x001f;
            iGreenAmount = (iPixelSrc & 0x07e0) >> 5;
            iB = iPixelDst & 0x001f;
            iB = OGE_FX_Saturate(iB - ((iAmount * iB) >> 5), 31);
            iG = (iPixelDst & 0x07e0) >> 5;
            iG = OGE_FX_Saturate(iG - ((iGreenAmount * iG) >> 6), 63);
            iR = (iPixelDst & 0xf800) >> 11;
            iR = OGE_FX_Saturate(iR - ((iAmount * iR) >> 5), 31);

            *((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;

			pSrcData += 2;
			pDstData += 2;

		}

        pSrcData += iSrcDataPad;
		pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break;


    } // end of switch case
}

void OGE_FX_BltMask(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize,
                int iSrcX, int iSrcY,
                uint8_t* pMaskData, int iMaskLineSize,
                int iMaskX, int iMaskY,
                int iWidth, int iHeight, int iBPP, int iBaseAmount)
{
    if(mmxflag <= 0) return;

    int iSrcDataPad;
	int iDstDataPad;
	int iMaskDataPad;

	//uint16_t i16Mask = 0x7f7f;
	//uint32_t i32Mask = 0x00ffffff;

	//uint32_t i32Mask = 0x007f7f7f;

	uint32_t iB = iBaseAmount & 0x000000ff;
	uint32_t iG = iBaseAmount & 0x0000ff00 >> 8;
	uint32_t iR = iBaseAmount & 0x00ff0000 >> 16;

	uint16_t i16Mask = (iR >> 3 << 11 | iG >> 2 << 5 | iB >> 3) & 0x0000ffff;

	uint32_t i32Mask = iBaseAmount;

	uint64_t i64Mask = i32Mask;

	switch (iBPP)
	{

	//case 15:
	case 16:

        iWidth <<= 1;
        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        pMaskData += iMaskY * iMaskLineSize + iMaskX * 2;

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        iMaskDataPad = iMaskLineSize - iWidth;

        i64Mask = i16Mask;
        i64Mask = i64Mask << 48 | i64Mask << 32 | i64Mask << 16 | i64Mask;



        // asm code
    __asm__ __volatile__ (

                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64Mask   %5 == i16Mask
                // %6 == iSrcDataPad   %7 == iDstDataPad
                // %8 == pMaskData,    %9 == iMaskDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                "cmpl    $0, %0                  \n\t"
				"jbe     __end_blt_mask_16            \n\t"

                "movl    %1, %%esi               \n\t"
                "movl    %2, %%edi               \n\t"
                "movl    %8, %%edx               \n\t"

                "movq    %4, %%mm7               \n\t"  // mm7 = Mask


        "__do_next_line_blt_mask_16:                  \n\t"


                "mov     %3, %%ecx               \n\t"

                "cmpl    $8, %%ecx               \n\t"
                "jb      __skip_mmx_blt_mask_16       \n\t"

        "__do_mmx_blt_mask_16:                        \n\t"

                "movq    (%%esi), %%mm0          \n\t"  // mm0 = ssss
                "movq    (%%edi), %%mm1          \n\t"  // mm1 = dddd
                "movq    (%%edx), %%mm2          \n\t"  // mm2 = ffee

                // handle color key form src
                //"movq    %%mm2, %%mm3            \n\t"  // mm3 = ffee
                "pcmpgtw %%mm7, %%mm2            \n\t"  // mm2 = 1100
                "movq    %%mm2, %%mm3            \n\t"  // mm3 = 1100
                "pand    %%mm2, %%mm0            \n\t"  // mm0 = ssss & 1100    = ss00
                "pandn   %%mm1, %%mm3            \n\t"  // mm3 = (~1100) & dddd = 00dd
                "por     %%mm3, %%mm0            \n\t"  // mm0 = ss00 & 00dd    = ssdd
                // fin
                "movq    %%mm0, (%%edi)          \n\t"

                "addl    $8, %%edx               \n\t"
                "addl    $8, %%edi               \n\t"
                "addl    $8, %%esi               \n\t"

                "subl    $8, %%ecx               \n\t"
                "cmpl    $8, %%ecx               \n\t"
                "jae     __do_mmx_blt_mask_16         \n\t"

        "__skip_mmx_blt_mask_16:                      \n\t"

                "cmpl    $2, %%ecx               \n\t"
                "jb      __skip_blt_mask_16           \n\t"

        "__do_blt_mask_16:                            \n\t"

                "movw    (%%edx), %%ax           \n\t"
                "cmpw     %5, %%ax               \n\t"
                "jbe      __next_blt_mask_16          \n\t"
                "movw    (%%esi), %%bx           \n\t"
                "movw     %%bx, (%%edi)          \n\t"

        "__next_blt_mask_16:                          \n\t"

                "addl    $2, %%edx               \n\t"
                "addl    $2, %%edi               \n\t"
                "addl    $2, %%esi               \n\t"

                "subl    $2, %%ecx               \n\t"
                "jnz     __do_blt_mask_16             \n\t"

       "__skip_blt_mask_16:                           \n\t"

                "addl    %9, %%edx               \n\t"
                "addl    %6, %%esi               \n\t"
                "addl    %7, %%edi               \n\t"

                "decl    %0                      \n\t"
                "jnz     __do_next_line_blt_mask_16   \n\t"

                //"pop     %%edi                   \n\t"
                //"pop     %%esi                   \n\t"

        "__end_blt_mask_16:                           \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64Mask   %5 == i16Mask
                // %6 == iSrcDataPad   %7 == iDstDataPad
                // %8 == pMaskData,    %9 == iMaskDataPad
                : "m"(iHeight),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iWidth),
                  "m"(i64Mask), "m"(i16Mask),
                  "m"(iSrcDataPad), "m"(iDstDataPad),
                  "m"(pMaskData),   "m"(iMaskDataPad)
                : "%esi", "%edi", "%eax", "%ecx", "%edx", "%ebx", "memory"


        );

    break;

    case 32:

        iWidth <<= 2;
        pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

        pMaskData += iMaskY * iMaskLineSize + iMaskX * 4;

        iSrcDataPad = iSrcLineSize - iWidth;
        iDstDataPad = iDstLineSize - iWidth;

        iMaskDataPad = iMaskLineSize - iWidth;

        i64Mask = i32Mask;
        i64Mask = (i64Mask << 32) | i64Mask;

        // asm code
    __asm__ __volatile__ (

                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64Mask   %5 == i32Mask
                // %6 == iSrcDataPad   %7 == iDstDataPad
                // %8 == pMaskData,    %9 == iMaskDataPad

                //"push    %%esi                   \n\t"
                //"push    %%edi                   \n\t"

                "cmpl    $0, %0                  \n\t"
				"jbe     __end_blt_mask_32            \n\t"

                "movl    %1, %%esi               \n\t"
                "movl    %2, %%edi               \n\t"
                "movl    %8, %%edx               \n\t"

                "movq    %4, %%mm7               \n\t"  // mm7 = Mask


        "__do_next_line_blt_mask_32:                  \n\t"


                "mov     %3, %%ecx               \n\t"

                "cmpl    $8, %%ecx               \n\t"
                "jb      __skip_mmx_blt_mask_32       \n\t"

        "__do_mmx_blt_mask_32:                        \n\t"

                "movq    (%%esi), %%mm0          \n\t"  // mm0 = ssss
                "movq    (%%edi), %%mm1          \n\t"  // mm1 = dddd
                "movq    (%%edx), %%mm2          \n\t"  // mm2 = ffee

                // handle color key form src
                //"movq    %%mm2, %%mm3            \n\t"  // mm3 = ffee
                "pcmpgtd %%mm7, %%mm2            \n\t"  // mm2 = 1100
                "movq    %%mm2, %%mm3            \n\t"  // mm3 = 1100
                "pand    %%mm2, %%mm0            \n\t"  // mm0 = ssss & 1100    = ss00
                "pandn   %%mm1, %%mm3            \n\t"  // mm3 = (~1100) & dddd = 00dd
                "por     %%mm3, %%mm0            \n\t"  // mm0 = ss00 & 00dd    = ssdd
                // fin
                "movq    %%mm0, (%%edi)          \n\t"

                "addl    $8, %%edx               \n\t"
                "addl    $8, %%edi               \n\t"
                "addl    $8, %%esi               \n\t"

                "subl    $8, %%ecx               \n\t"
                "cmpl    $8, %%ecx               \n\t"
                "jae     __do_mmx_blt_mask_32         \n\t"

        "__skip_mmx_blt_mask_32:                      \n\t"

                "cmpl    $4, %%ecx               \n\t"
                "jb      __skip_blt_mask_32           \n\t"

        "__do_blt_mask_32:                            \n\t"

                "movl    (%%edx), %%eax           \n\t"
                "cmpl     %5, %%eax               \n\t"
                "jbe      __next_blt_mask_32          \n\t"
                "movl    (%%esi), %%ebx           \n\t"
                "movl     %%ebx, (%%edi)          \n\t"

        "__next_blt_mask_32:                          \n\t"

                "addl    $4, %%edx               \n\t"
                "addl    $4, %%edi               \n\t"
                "addl    $4, %%esi               \n\t"

                "subl    $4, %%ecx               \n\t"
                "jnz     __do_blt_mask_32             \n\t"

       "__skip_blt_mask_32:                           \n\t"

                "addl    %9, %%edx               \n\t"
                "addl    %6, %%esi               \n\t"
                "addl    %7, %%edi               \n\t"

                "decl    %0                      \n\t"
                "jnz     __do_next_line_blt_mask_32   \n\t"

                //"pop     %%edi                   \n\t"
                //"pop     %%esi                   \n\t"

        "__end_blt_mask_32:                           \n\t"

                "emms                            \n\t"

                : // no output
                // %0 == iHeight,
                // %1 == pSrcData, %2 == pDstData,
                // %3 == iWidth,
                // %4 == i64Mask   %5 == i32Mask
                // %6 == iSrcDataPad   %7 == iDstDataPad
                // %8 == pMaskData,    %9 == iMaskDataPad
                : "m"(iHeight),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iWidth),
                  "m"(i64Mask), "m"(i32Mask),
                  "m"(iSrcDataPad), "m"(iDstDataPad),
                  "m"(pMaskData),   "m"(iMaskDataPad)
                : "%esi", "%edi", "%eax", "%ecx", "%edx", "%ebx", "memory"


        );

    break;

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
                    if(iSrcColorKey == -1 || iColor != iSrcColorKey) *pc16 = iColor;
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
                    if(iSrcColorKey == -1 || iColor != iSrcColorKey) *pc32 = iColor;
                }

                sdx += icos;
                sdy += isin;

                pc32++;

            }

        }

    break;
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
    int iSrcDataPad, iDstDataPad, iOrgWidth, iFirstRow;

	switch (iBPP)
	{
	//case 15:
	case 16:
		iWidth <<= 1;

		iSrcDataPad = iSrcLineSize - iWidth;
		iDstDataPad = iDstLineSize - iWidth;

		pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

		iOrgWidth = iWidth;
		iFirstRow = 1;

		//iRGBColorKey16 = iRGBColorKey;

  // asm code

        __asm__ __volatile__ (


                // %0 == iWidth,       %1 == iHeight
                // %2 == pSrcData,     %3 == pDstData
                // %4 == iSrcDataPad,  %5 == iDstDataPad
                // %6 == iSrcColorKey, %7 == iEdgeColor
                // %8 == iFirstRow,    %9 == iOrgWidth

                "pushl    %%eax         \n\t"
                "pushl    %%ebx        \n\t"
                "pushl    %%edx        \n\t"
                "pushl    %%ecx        \n\t"
                "pushl    %%esi        \n\t"
                "pushl    %%edi        \n\t"

                "movl     %0, %%ebx         \n\t"
                "addl     %4, %%ebx         \n\t"

                "movl     %2, %%esi        \n\t"
                "movl     %3, %%edi        \n\t"

        "__do_next_line_r1:        \n\t"

                "movl     %0, %%ecx         \n\t"

                "cmpl     $2,    %%ecx         \n\t"
                "jb      __skip_2_r1        \n\t"

        "__do_copy_2_r1:        \n\t"

                "movw     (%%esi), %%ax      \n\t"
                "cmpw     %6, %%ax         \n\t"
                "je      __test_left_r1        \n\t"

        "__set_point_r1:        \n\t"

                "movw     %%ax, (%%edi)       \n\t"
                "jmp      __next_2_r1        \n\t"

		"__set_edge_r1:        \n\t"

				"movw     %7, %%ax        \n\t"
                "movw     %%ax,  (%%edi)       \n\t"
                "jmp     __next_2_r1        \n\t"

        "__test_left_r1:        \n\t"

                "cmpl     %9, %%ecx         \n\t"
                "je      __test_up_r1        \n\t"

                "movw     -2(%%esi), %%dx       \n\t"
                "cmpw     %6, %%dx         \n\t"
                "jne     __set_edge_r1        \n\t"

        "__test_up_r1:        \n\t"

                "cmpl     $1, %8        \n\t"
                "je      __test_right_r1        \n\t"

                "movl     %%esi, %%edx        \n\t"
                "subl     %%ebx, %%edx        \n\t"
                "movw     (%%edx), %%dx       \n\t"
                "cmpw     %6, %%dx         \n\t"
                "jne     __set_edge_r1        \n\t"

        "__test_right_r1:        \n\t"

                "cmpl     $2, %%ecx        \n\t"
                "je      __test_down_r1        \n\t"

                "movw     2(%%esi), %%dx       \n\t"
                "cmpw     %6, %%dx         \n\t"
                "jne     __set_edge_r1        \n\t"

        "__test_down_r1:        \n\t"

                "cmpl     $1, %1        \n\t"
                "je      __next_2_r1        \n\t"

                "movl     %%esi, %%edx        \n\t"
                "addl     %%ebx, %%edx       \n\t"
                "movw     (%%edx), %%dx       \n\t"
                "cmpw     %6, %%dx         \n\t"
                "jne     __set_edge_r1        \n\t"

        "__next_2_r1:        \n\t"
                "addl     $2, %%edi         \n\t"
                "addl     $2, %%esi         \n\t"

                "subl     $2, %%ecx        \n\t"
                "jnz     __do_copy_2_r1        \n\t"

        "__skip_2_r1:        \n\t"

                "addl     %4, %%esi         \n\t"
                "addl     %5, %%edi         \n\t"

                "incl     %8        \n\t"

                "decl     %1        \n\t"
                "jnz     __do_next_line_r1        \n\t"

       //__fin_all:

                "popl     %%edi        \n\t"
                "popl     %%esi        \n\t"
                "popl     %%ecx        \n\t"
                "popl     %%edx        \n\t"
                "popl     %%ebx        \n\t"
                "popl     %%eax        \n\t"

                : // no output

                // %0 == iWidth,       %1 == iHeight
                // %2 == pSrcData,     %3 == pDstData
                // %4 == iSrcDataPad,  %5 == iDstDataPad
                // %6 == iSrcColorKey, %7 == iEdgeColor
                // %8 == iFirstRow,    %9 == iOrgWidth

                : "m"(iWidth), "m"(iHeight),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iSrcDataPad), "m"(iDstDataPad),
                  "m"(iSrcColorKey), "m"(iEdgeColor),
                  "m"(iFirstRow), "m"(iOrgWidth)
                : "memory"

		);     // end of asm code

    break;

	case 32:
		iWidth <<= 2;

		iSrcDataPad = iSrcLineSize - iWidth;
		iDstDataPad = iDstLineSize - iWidth;

		pDstData += iDstY * iDstLineSize + iDstX * 4;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 4;

		iOrgWidth = iWidth;
		iFirstRow = 1;

  // asm code
        __asm__ __volatile__ (


                // %0 == iWidth,       %1 == iHeight
                // %2 == pSrcData,     %3 == pDstData
                // %4 == iSrcDataPad,  %5 == iDstDataPad
                // %6 == iSrcColorKey, %7 == iEdgeColor
                // %8 == iFirstRow,    %9 == iOrgWidth

                "pushl    %%eax         \n\t"
                "pushl    %%ebx        \n\t"
                "pushl    %%edx        \n\t"
                "pushl    %%ecx        \n\t"
                "pushl    %%esi        \n\t"
                "pushl    %%edi        \n\t"

                "movl     %0, %%ebx         \n\t"
                "addl     %4, %%ebx         \n\t"

                "movl     %2, %%esi        \n\t"
                "movl     %3, %%edi        \n\t"

        "__do_next_line_r2:        \n\t"

                "movl     %0, %%ecx         \n\t"

                "cmpl     $4,    %%ecx         \n\t"
                "jb      __skip_4_r2        \n\t"

        "__do_copy_4_r2:        \n\t"

                "movl     (%%esi), %%eax      \n\t"
                "cmpl     %6, %%eax         \n\t"
                "je      __test_left_r2        \n\t"

        "__set_point_r2:        \n\t"

                "movl     %%eax, (%%edi)       \n\t"
                "jmp      __next_4_r2        \n\t"

		"__set_edge_r2:        \n\t"

				"movl     %7, %%eax        \n\t"
                "movl     %%eax,  (%%edi)       \n\t"
                "jmp     __next_4_r2        \n\t"

        "__test_left_r2:        \n\t"

                "cmpl     %9, %%ecx         \n\t"
                "je      __test_up_r2        \n\t"

                "movl     -4(%%esi), %%edx       \n\t"
                "cmpl     %6, %%edx         \n\t"
                "jne     __set_edge_r2        \n\t"

        "__test_up_r2:        \n\t"

                "cmpl     $1, %8        \n\t"
                "je      __test_right_r2        \n\t"

                "movl     %%esi, %%edx        \n\t"
                "subl     %%ebx, %%edx        \n\t"
                "movl     (%%edx), %%edx       \n\t"
                "cmpl     %6, %%edx         \n\t"
                "jne     __set_edge_r2        \n\t"

        "__test_right_r2:        \n\t"

                "cmpl     $4, %%ecx        \n\t"
                "je      __test_down_r2        \n\t"

                "movl     4(%%esi), %%edx       \n\t"
                "cmpl     %6, %%edx         \n\t"
                "jne     __set_edge_r2        \n\t"

        "__test_down_r2:        \n\t"

                "cmpl     $1, %1        \n\t"
                "je      __next_4_r2        \n\t"

                "movl     %%esi, %%edx        \n\t"
                "addl     %%ebx, %%edx       \n\t"
                "movl     (%%edx), %%edx       \n\t"
                "cmpl     %6, %%edx         \n\t"
                "jne     __set_edge_r2        \n\t"

        "__next_4_r2:        \n\t"
                "addl     $4, %%edi         \n\t"
                "addl     $4, %%esi         \n\t"

                "subl     $4, %%ecx        \n\t"
                "jnz     __do_copy_4_r2        \n\t"

        "__skip_4_r2:        \n\t"

                "addl     %4, %%esi         \n\t"
                "addl     %5, %%edi         \n\t"

                "incl     %8        \n\t"

                "decl     %1        \n\t"
                "jnz     __do_next_line_r2        \n\t"

       //__fin_all:

                "popl     %%edi        \n\t"
                "popl     %%esi        \n\t"
                "popl     %%ecx        \n\t"
                "popl     %%edx        \n\t"
                "popl     %%ebx        \n\t"
                "popl     %%eax        \n\t"

                : // no output

                // %0 == iWidth,       %1 == iHeight
                // %2 == pSrcData,     %3 == pDstData
                // %4 == iSrcDataPad,  %5 == iDstDataPad
                // %6 == iSrcColorKey, %7 == iEdgeColor
                // %8 == iFirstRow,    %9 == iOrgWidth

                : "m"(iWidth), "m"(iHeight),
                  "m"(pSrcData), "m"(pDstData),
                  "m"(iSrcDataPad), "m"(iDstDataPad),
                  "m"(iSrcColorKey), "m"(iEdgeColor),
                  "m"(iFirstRow), "m"(iOrgWidth)
                : "memory"

		);     // end of asm code
		break;

	}  // end of switch case

}


void OGE_FX_BltWithColor(uint8_t* pDstData, int iDstLineSize,
                int iDstX, int iDstY,
                uint8_t* pSrcData, int iSrcLineSize, int iSrcColorKey,
                int iSrcX, int iSrcY,
                int iWidth, int iHeight, int iBPP, int iColor, int iAlpha)
{
    if(mmxflag <= 0) return;

    int i, iRemain, iSrcDataPad, iDstDataPad, iPixelSrc, iPixelDst;
	uint8_t iB, iG, iR;
	uint64_t i64MaskRed, i64MaskGreen, i64MaskBlue, i64ColorKey, i64Color, i64Alpha;

	if(iAlpha > 256) iAlpha = 256;
	else if(iAlpha < 0) iAlpha = 0;

	iAlpha = 256 - iAlpha;

    i64ColorKey  = iSrcColorKey;
    i64Color = iColor;

	i64Alpha = iAlpha;

	//i64AlphaDiv4 = iAlpha >> 2;

	switch (iBPP)
	{
	case 16:
// RGB 565 Mode ..

    //
    // Set the bit masks for red, green and blue.
    //
        i64MaskRed   = 0xf800f800f800f800LL;
        i64MaskGreen = 0x07e007e007e007e0LL;
        i64MaskBlue  = 0x001f001f001f001fLL;
        //i6464        = 0x0040004000400040LL;

        i64ColorKey  = (i64ColorKey << 48)  | (i64ColorKey << 32)  | (i64ColorKey << 16)  | i64ColorKey;
        i64Alpha     = (i64Alpha << 48)     | (i64Alpha << 32)     | (i64Alpha << 16)     | i64Alpha;
        //i64AlphaDiv4 = (i64AlphaDiv4 << 48) | (i64AlphaDiv4 << 32) | (i64AlphaDiv4 << 16) | i64AlphaDiv4;

        i64Color     = (i64Color << 48)     | (i64Color << 32)     | (i64Color << 16)     | i64Color;

        pDstData += iDstY * iDstLineSize + iDstX * 2;
        pSrcData += iSrcY * iSrcLineSize + iSrcX * 2;

        iSrcDataPad  = iSrcLineSize - (iWidth << 1);
        iDstDataPad  = iDstLineSize - (iWidth << 1);


        // ensure the width is a multiple of four.
        iRemain = iWidth & 3;
        iWidth  = iWidth >> 2;

        while (iHeight > 0)
        {

    // asm code
        __asm__ __volatile__ (

                // %0 == iWidth,
                // %1 == i64MaskBlue,  %2  == i64MaskGreen, %3  == i64MaskRed,
                // %4 == i64ColorKey,  %5  == i64Alpha,     %6  == i64Color
                // %7 == pSrcData,     %8 == pDstData,      %9  == iSrcColorKey

			      "pushl    %%edx       \n\t"
                  "pushl    %%ecx       \n\t"
                  "movl     %0, %%ecx       \n\t"
                  "cmpl     $0, %%ecx       \n\t"
                  "jz      __skip565_alpha_16        \n\t" // if this row is less than 4 then jump to next

                  "pushl    %%esi       \n\t"
                  "pushl    %%edi       \n\t"
                  "movl     %7, %%esi       \n\t"
                  "movl     %8, %%edi       \n\t"

                  "movq     %6, %%mm7       \n\t"

          "__do_blend_alpha_16:       \n\t"
                  //   mm1~mm3: bgr of src
                  //   mm4~mm6: bgr of dst
                  //   mm0: src
                  //   mm7: dst

                  "movq    (%%esi), %%mm0       \n\t"

				  //"cmpl     $1, iUseColorKey       \n\t"
				  //"jne     __start_light_alpha_16       \n\t"

				  "movl     %9, %%edx       \n\t"
				  "movq    %%mm0, %%mm1        \n\t"

				  "movd    %%mm1, %%eax        \n\t"

				  "cmpw     %%dx, %%ax         \n\t"
				  "jne     __start_light_alpha_16       \n\t"
				  "shrl     $16, %%eax      \n\t"
				  "cmpw     %%dx, %%ax         \n\t"
				  "jne     __start_light_alpha_16       \n\t"

				  "psrlq   $32, %%mm1       \n\t"
				  "movd    %%mm1, %%eax        \n\t"

				  "cmpw    %%dx, %%ax         \n\t"
				  "jne     __start_light_alpha_16       \n\t"
				  "shrl    $16, %%eax       \n\t"
				  "cmpw     %%dx, %%ax       \n\t"
				  "jne     __start_light_alpha_16       \n\t"

				  "jmp     __final_alpha_16       \n\t"


		"__start_light_alpha_16:       \n\t"

                  //"movq    (%%edi), %%mm7       \n\t"

                  // handle blue
                  "movq    %%mm0, %%mm1        \n\t"
                  "pand    %1, %%mm1        \n\t"
                  "movq    %%mm7, %%mm4        \n\t"
                  "pand    %1, %%mm4        \n\t"

                  "psubw   %%mm4, %%mm1         \n\t"   // src - dst
                  "pmullw  %5,    %%mm1         \n\t"   // (src - dst) * Alpha
                  "psrlw   $8,    %%mm1         \n\t"   // (src - dst) * Alpha / 256
                  "paddw   %%mm4, %%mm1         \n\t"   // (src - dst) * Alpha / 256 + dst

                  // handle green
                  "movq    %%mm0, %%mm2        \n\t"
                  "pand    %2, %%mm2        \n\t"
                  "psrlw   $5, %%mm2        \n\t"
                  "movq    %%mm7, %%mm5        \n\t"
                  "pand    %2, %%mm5       \n\t"
                  "psrlw   $5, %%mm5       \n\t"

                  "psubw   %%mm5, %%mm2         \n\t"   // src - dst
                  "pmullw  %5,    %%mm2         \n\t"   // (src - dst) * Alpha
                  "psrlw   $8,    %%mm2         \n\t"   // (src - dst) * Alpha / 256
                  "paddw   %%mm5, %%mm2         \n\t"   // (src - dst) * Alpha / 256 + dst
                  "psllw   $5,    %%mm2        \n\t"

                  // handle red
                  "movq    %%mm0, %%mm3        \n\t"
                  "pand    %3, %%mm3        \n\t"
                  "psrlw   $11, %%mm3        \n\t"
                  "movq    %%mm7, %%mm6       \n\t"
                  "pand    %3, %%mm6       \n\t"
                  "psrlw   $11, %%mm6        \n\t"

                  "psubw   %%mm6, %%mm3         \n\t"   // src - dst
                  "pmullw  %5,    %%mm3         \n\t"   // (src - dst) * Alpha
                  "psrlw   $8,    %%mm3         \n\t"   // (src - dst) * Alpha / 256
                  "paddw   %%mm6, %%mm3         \n\t"   // (src - dst) * Alpha / 256 + dst
                  "psllw   $11, %%mm3           \n\t"


                  // finish
                  "por     %%mm2, %%mm1       \n\t"
                  "por     %%mm3, %%mm1       \n\t"

                  //"cmpl    $1, iUseColorKey        \n\t"
                  //"jne     __no_color_key_alpha_16       \n\t"

          "__src_color_key_alpha_16:       \n\t"

                  "movq    %%mm0, %%mm2        \n\t"

                  "movq    (%%edi), %%mm5       \n\t"

         "__handle_color_key_alpha_16:       \n\t"

                  "pcmpeqw %4, %%mm2        \n\t"
                  "movq    %%mm2, %%mm3        \n\t"

                  "pandn   %%mm1, %%mm3       \n\t"
                  "pand    %%mm2, %%mm5        \n\t"

                  "por     %%mm5, %%mm3        \n\t"
                  "movq    %%mm3, (%%edi)        \n\t"
                  "jmp     __final_alpha_16       \n\t"

         "__no_color_key_alpha_16:       \n\t"

                  "movq    %%mm1, (%%edi)        \n\t"

          "__final_alpha_16:       \n\t"
                  // final
                  "addl     $8, %%edi        \n\t"
                  "addl     $8, %%esi       \n\t"

                  "decl     %%ecx       \n\t"
                  "jnz     __do_blend_alpha_16       \n\t"

                  "movl     %%esi, %7       \n\t"
                  "movl     %%edi, %8       \n\t"
                  "popl     %%edi       \n\t"
                  "popl     %%esi       \n\t"

                  "emms       \n\t"

          "__skip565_alpha_16:       \n\t"
                  "popl     %%ecx       \n\t"
				  "popl     %%edx       \n\t"

				  : // no output
				    // %0 == iWidth,
                    // %1 == i64MaskBlue,  %2  == i64MaskGreen, %3  == i64MaskRed,
                    // %4 == i64ColorKey,  %5  == i64Alpha,     %6  == i64Color
                    // %7 == pSrcData,     %8 == pDstData,      %9  == iSrcColorKey
                  : "m"(iWidth),
                    "m"(i64MaskBlue), "m"(i64MaskGreen), "m"(i64MaskRed),
                    "m"(i64ColorKey), "m"(i64Alpha),     "m"(i64Color),
                    "m"(pSrcData), "m"(pDstData), "m"(iSrcColorKey)
                  : "memory"

		);     // end of asm code

        // handle the remain
		for (i=1; i<=iRemain; i++)
		{
			iPixelSrc = *((uint16_t*)pSrcData);
			if (iPixelSrc != iSrcColorKey )
			{

				//iPixelDst = *((uint16_t*)pDstData);

				iPixelDst = iColor;

				// result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA / 4)
				// result = (ALPHA * (src - dst)) / 256 + dst

				iB = ((iAlpha * ((iPixelSrc & 0x001f) - (iPixelDst & 0x001f))) >> 8) +
					(iPixelDst & 0x001f);

				iG = ((iAlpha * (((iPixelSrc & 0x07e0) >> 5) - ((iPixelDst & 0x07e0) >> 5))) >> 8) +
					((iPixelDst & 0x07e0) >> 5);

				iR = ((iAlpha * (((iPixelSrc & 0xf800) >> 11) - ((iPixelDst & 0xf800) >> 11))) >> 8) +
					((iPixelDst & 0xf800) >> 11);

				*((uint16_t*)pDstData) = (iR << 11) | (iG << 5) | iB;
			}

			pSrcData += 2;
			pDstData += 2;

		}

        pSrcData += iSrcDataPad;
		pDstData += iDstDataPad;

        iHeight--;

	}
    break;  // end 565 mode

    case 32:
    // RGB 8888 Mode ..

    //i64MaskRed   = 0x00ff000000ff0000LL;
    //i64MaskGreen = 0x0000ff000000ff00LL;
    //i64MaskBlue  = 0x000000ff000000ffLL;
    //i6464        = 0x0040004000400040LL;

    //i64Amount    = (i64Amount << 32) | i64Amount;

    i64ColorKey  = (i64ColorKey << 32)  | i64ColorKey;

    i64Alpha     = (i64Alpha << 48)     | (i64Alpha << 32)     | (i64Alpha << 16)     | i64Alpha;

    //i64Color     = (i64Color << 48)     | (i64Color << 32)     | (i64Color << 16)     | i64Color;

    //i64Alpha     = (i64Alpha << 32)     | i64Alpha;
    //i64AlphaDiv4 = (i64AlphaDiv4 << 32) | i64AlphaDiv4;

    i64Color     = (i64Color << 32)     | i64Color;

    pSrcData     = pSrcData + iSrcLineSize * iSrcY + 4*iSrcX;
    pDstData     = pDstData + iDstLineSize * iDstY + 4*iDstX;

    iSrcDataPad     = iSrcLineSize - (iWidth << 2);
    iDstDataPad     = iDstLineSize - (iWidth << 2);

    iRemain      = iWidth & 1;
    iWidth     >>= 1;

    while (iHeight > 0)
    {
                // %0 == iWidth,
                // %1 == i64ColorKey,  %2  == i64Alpha,     %3  == i64Color
                // %4 == pSrcData,     %5 == pDstData,

    // asm code
		  __asm__ __volatile__ (

                    "pushl   %%ecx               \n\t"
                    "pushl   %%ebx               \n\t"

                    "movl    %0,  %%ecx          \n\t"  // %? == iWidth
                    "cmpl    $0,  %%ecx          \n\t"
                    "jz      __skip_blt_alpha_32 \n\t"  // if this row is less than 4 then jump to next

                    "pushl    %%esi       \n\t"
                    "pushl    %%edi       \n\t"
                    "movl     %4, %%esi       \n\t"
                    "movl     %5, %%edi       \n\t"

                    "movq    %3, %%mm5       \n\t"

                    "movq    %2, %%mm6       \n\t"


        "__do_blt_alpha_32:                      \n\t"

                    "pxor    %%mm7, %%mm7       \n\t"

                    "movq    (%%esi), %%mm0      \n\t"     // src1
                    "movq    %%mm0,   %%mm4      \n\t"     // src1

                    "movq    %%mm5,   %%mm1          \n\t" // src2

                    "movq    %%mm0,   %%mm2          \n\t" // make a copy
                    "movq    %%mm1,   %%mm3          \n\t" // make a copy
                    //"movq    %%mm1,   %%mm4          \n\t" // make a copy

                    "punpcklbw %%mm7, %%mm0          \n\t" // mm1 = unpacked low half of src
                    "punpcklbw %%mm7, %%mm1          \n\t" // mm1 = unpacked low half of dst
                    "punpckhbw %%mm7, %%mm2          \n\t" // mm2 = unpacked high half of src
                    "punpckhbw %%mm7, %%mm3          \n\t" // mm2 = unpacked high half of dst

                    "psubw     %%mm1, %%mm0          \n\t" // mm0 = low half of (src1-src2)
                    "psubw     %%mm3, %%mm2         \n\t" // mm2 = high half of (src1-src2)
                    "pmullw    %%mm6, %%mm0        \n\t" // low (src1-src2)*alpha
                    "pmullw    %%mm6, %%mm2          \n\t" // high (src1-src2)*alpha

                    "psrlw     $8,    %%mm0          \n\t" // low (src1-src2)*alpha / 256
                    "psrlw     $8,    %%mm2         \n\t" // high (src1-src2)*alpha / 256
                    "packuswb  %%mm2, %%mm0          \n\t" // combine with unsigned saturation
                    "paddb     %%mm5, %%mm0         \n\t" // (src1-src2)*alpha / 256 + src2

                    // fin
                    //"movq    %%mm0,   (%%edi)        \n\t"


                    "movq    %1, %%mm1      \n\t"

                    "movq    (%%edi), %%mm7      \n\t"
                    "movq    %%mm4, %%mm2        \n\t"

                    "pcmpeqd %%mm1, %%mm2        \n\t"
                    "movq    %%mm2, %%mm3        \n\t"

                    "pandn   %%mm0, %%mm3        \n\t"
                    "pand    %%mm2, %%mm7        \n\t"

                    "por     %%mm7, %%mm3        \n\t"
                    "movq    %%mm3, (%%edi)      \n\t"

                    // Advance to the next four pixels.

                    "addl    $8,    %%esi        \n\t"
                    "addl    $8,    %%edi        \n\t"

                    //
                    // Loop again or break.
                    //
                    "decl	%%ecx                \n\t"
                    "jnz	__do_blt_alpha_32    \n\t"

                    //
                    // Write back ...
                    //
                    "movl	%%esi, %4            \n\t"
                    "movl	%%edi, %5            \n\t"

                    "popl	%%edi                \n\t"
                    "popl	%%esi                \n\t"

                    "emms                        \n\t"

        "__skip_blt_alpha_32:                    \n\t"

                    "popl	%%ebx                \n\t"
                    "popl	%%ecx                \n\t"

                    : // no output
                      // %0 == iWidth,
                      // %1 == i64ColorKey,  %2  == i64Alpha,     %3  == i64Color
                      // %4 == pSrcData,     %5 == pDstData,
                    : "m"(iWidth),
                      "m"(i64ColorKey), "m"(i64Alpha), "m"(i64Color),
                      "m"(pSrcData),    "m"(pDstData)
                    : "memory"
        );


        // handle the remain

		for (i=1; i<=iRemain; i++)
		{
			iPixelSrc = *((int*)pSrcData);
			if (iPixelSrc != iSrcColorKey)
			{
				//iPixelDst = *((int*)pDstData);

				iPixelDst = iColor;

				// result = (ALPHA * ((src+64) - dst)) / 256 + dst - (ALPHA / 4)

				iB = ((iAlpha * ((iPixelSrc & 0x0000ff) - (iPixelDst & 0x0000ff))) >> 8) +
					(iPixelDst & 0x00ff);

				iG = ((iAlpha * (((iPixelSrc & 0x00ff00) >> 8) - ((iPixelDst & 0x00ff00) >> 8))) >> 8) +
					((iPixelDst & 0x00ff00) >> 8);

				iR = ((iAlpha * (((iPixelSrc & 0xff0000) >> 16) - ((iPixelDst & 0xff0000) >> 16))) >> 8) +
					((iPixelDst & 0xff0000) >> 16);

				*((int*)pDstData) = (iR << 16) | (iG << 8) | iB;
			}

			pSrcData += 4;
			pDstData += 4;

		}

        pSrcData += iSrcDataPad;
		pDstData += iDstDataPad;

        iHeight--;

	} // end of while (iHeight > 0)

	break; // end of 8888 mode

  } // end switch case

}


#endif //__FX_WITH_MMX__
