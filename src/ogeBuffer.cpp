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

#include "ogeBuffer.h"

#ifdef __WINCE__
#include "winbase.h"
#endif

#include <cstdlib>
#include <cstring>

/************* CogeBuffer *****************/

CogeBuffer::CogeBuffer(ogeBufferSize iSize)
{
    m_iState = -1;
    m_iSize = 0;
    m_iUserCount = 0;
    m_pBuf = NULL;

    if(iSize < _OGE_MIN_BUFFER_SIZE_M_ || iSize > _OGE_MAX_BUFFER_SIZE_M_) return;

    m_iSize = iSize * _OGE_BUFFER_SIZE_1_M_;

#ifdef __WINCE__

    //m_pBuf = (char*)VirtualAlloc(NULL, m_iSize, MEM_COMMIT, PAGE_READWRITE);

    m_pBuf = (char*)VirtualAlloc(NULL, m_iSize, MEM_RESERVE, PAGE_NOACCESS);
	if(m_pBuf != NULL) m_pBuf = (char*)VirtualAlloc((void*)m_pBuf, m_iSize, MEM_COMMIT, PAGE_READWRITE);

#else
    m_pBuf = (char*)malloc(m_iSize);
#endif

    if(m_pBuf == NULL) return;

    m_iState = 0;

}

CogeBuffer::~CogeBuffer()
{

#ifdef __WINCE__
    if(m_pBuf) VirtualFree((void*)m_pBuf, m_iSize, MEM_RELEASE);
#else
    if(m_pBuf) free(m_pBuf);
#endif

    m_pBuf = NULL;

    m_iUserCount = 0;
    m_iSize = 0;
    m_iState = -1;
}

int CogeBuffer::GetState()
{
    return m_iState;
}

int CogeBuffer::GetSize()
{
    return m_iSize;
}

char* CogeBuffer::GetBuffer()
{
    return m_pBuf;
}

bool CogeBuffer::IsBusy()
{
    return m_iState >= 0 && m_iUserCount > 0;
}
int CogeBuffer::Hire()
{
    if(m_iUserCount < 0) m_iUserCount = 0;
    m_iUserCount++;
    return m_iUserCount;
}
int CogeBuffer::Fire()
{
    m_iUserCount--;
    if(m_iUserCount < 0) m_iUserCount = 0;
    return m_iUserCount;
}

/************* CogeBufferManager *****************/

CogeBufferManager::CogeBufferManager()
{
    m_iState = -1;

    std::vector<CogeBuffer*>* lstBuffer_1M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_1M, lstBuffer_1M));

    std::vector<CogeBuffer*>* lstBuffer_2M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_2M, lstBuffer_2M));

    std::vector<CogeBuffer*>* lstBuffer_3M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_3M, lstBuffer_3M));

    std::vector<CogeBuffer*>* lstBuffer_4M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_4M, lstBuffer_4M));

    std::vector<CogeBuffer*>* lstBuffer_5M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_5M, lstBuffer_5M));

    std::vector<CogeBuffer*>* lstBuffer_10M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_10M, lstBuffer_10M));

    std::vector<CogeBuffer*>* lstBuffer_20M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_20M, lstBuffer_20M));

    std::vector<CogeBuffer*>* lstBuffer_50M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_50M, lstBuffer_50M));

    std::vector<CogeBuffer*>* lstBuffer_100M = new std::vector<CogeBuffer*>();
    m_BufferLists.insert(std::map<int, std::vector<CogeBuffer*>*>::value_type(BufferSize_100M, lstBuffer_100M));

    m_iState = 0;

}

CogeBufferManager::~CogeBufferManager()
{
    m_iState = -1;
    FreeAll();
}

void CogeBufferManager::FreeAll()
{
    std::map<int, std::vector<CogeBuffer*>*>::iterator it;
	std::vector<CogeBuffer*>* pMatchedList = NULL;

	it = m_BufferLists.begin();

	while (it != m_BufferLists.end())
	{
		pMatchedList = it->second;
		m_BufferLists.erase(it);
		if(pMatchedList)
		{
            FreeBufferList(pMatchedList);
		    delete pMatchedList;
		    pMatchedList = NULL;
        }
		it = m_BufferLists.begin();
	}

}
void CogeBufferManager::FreeBufferList(std::vector<CogeBuffer*>* pBufferList)
{
    if(pBufferList == NULL) return;

    std::vector<CogeBuffer*>::iterator it;
	CogeBuffer* pMatchedBuffer = NULL;

	it = pBufferList->begin();

	while (it != pBufferList->end())
	{
		pMatchedBuffer = *it;
		pBufferList->erase(it);
		if(pMatchedBuffer) { delete pMatchedBuffer; pMatchedBuffer = NULL; }
		it = pBufferList->begin();
	}

}


int CogeBufferManager::GetState()
{
    return m_iState;
}

CogeBuffer* CogeBufferManager::GetFreeBuffer(int iRequiredSize)
{
    if(iRequiredSize <= 0) return NULL;

#ifdef __WINCE__
	if(iRequiredSize <= _OGE_BUFFER_SIZE_1_M_) iRequiredSize = iRequiredSize + _OGE_BUFFER_SIZE_1_M_; // ...
#endif

    ogeBufferSize iBufSizeM = BufferSize_1M;

    if(iRequiredSize > _OGE_BUFFER_SIZE_1_M_)
    {
        int iRequiredSizeM = iRequiredSize / _OGE_BUFFER_SIZE_1_M_;
        if(iRequiredSize % _OGE_BUFFER_SIZE_1_M_ > 0) iRequiredSizeM++;
        if(iRequiredSizeM > 1 && iRequiredSizeM <= 2) iBufSizeM = BufferSize_2M;
        else if(iRequiredSizeM > 2 && iRequiredSizeM <= 3) iBufSizeM = BufferSize_3M;
        else if(iRequiredSizeM > 3 && iRequiredSizeM <= 4) iBufSizeM = BufferSize_4M;
        else if(iRequiredSizeM > 4 && iRequiredSizeM <= 5) iBufSizeM = BufferSize_5M;
        else if(iRequiredSizeM > 5 && iRequiredSizeM <= 10) iBufSizeM = BufferSize_10M;
        else if(iRequiredSizeM > 10 && iRequiredSizeM <= 20) iBufSizeM = BufferSize_20M;
        else if(iRequiredSizeM > 20 && iRequiredSizeM <= 50) iBufSizeM = BufferSize_50M;
        else if(iRequiredSizeM > 50 && iRequiredSizeM <= 100) iBufSizeM = BufferSize_100M;
        else return NULL;
    }

    std::map<int, std::vector<CogeBuffer*>*>::iterator it = m_BufferLists.find(iBufSizeM);

    if(it == m_BufferLists.end()) return NULL;

    std::vector<CogeBuffer*>* pBufferList = it->second;

    std::vector<CogeBuffer*>::iterator its;
	CogeBuffer* pMatchedBuffer = NULL;
	CogeBuffer* pFreeBuffer = NULL;

	int count = pBufferList->size();

	its = pBufferList->begin();

	while (count > 0)
	{
		pMatchedBuffer = *its;

		if(!pMatchedBuffer->IsBusy())
		{
		    pFreeBuffer = pMatchedBuffer;
		    break;
		}

		its++;

		count--;
	}

	if(pFreeBuffer == NULL)
	{
	    pFreeBuffer = new CogeBuffer(iBufSizeM);
	    if(pFreeBuffer && pFreeBuffer->GetState() >= 0) pBufferList->push_back(pFreeBuffer);
	    else if(pFreeBuffer)
	    {
	        delete pFreeBuffer;
            pFreeBuffer = NULL;
	    }
	}

	return pFreeBuffer;

}

/************* CogeStreamBuffer *****************/

CogeStreamBuffer::CogeStreamBuffer(char* pBuf, int iBufSize)
{
    m_pBuf = pBuf;
    m_iSize = iBufSize;
    m_iReadPos = 0;
    m_iWritePos = 0;
}
CogeStreamBuffer::~CogeStreamBuffer()
{
    m_pBuf = NULL;
    m_iSize = 0;
    m_iReadPos = 0;
    m_iWritePos = 0;
}

int CogeStreamBuffer::GetSize()
{
    return m_iSize;
}
char* CogeStreamBuffer::GetBuffer()
{
    return m_pBuf;
}

int CogeStreamBuffer::GetReadPos()
{
    return  m_iReadPos;
}
int CogeStreamBuffer::GetWritePos()
{
    return m_iWritePos;
}

int CogeStreamBuffer::SetReadPos(int iPos)
{
    m_iReadPos = iPos;
    if(m_iReadPos < 0) m_iReadPos = 0;

    return m_iReadPos;
}
int CogeStreamBuffer::SetWritePos(int iPos)
{
    m_iWritePos = iPos;
    if(m_iWritePos < 0) m_iWritePos = 0;

    return m_iWritePos;
}

char CogeStreamBuffer::GetByte()
{
    char* src = (char*)m_pBuf;
    if(src == NULL) return 0;
    char rsl = *(src + m_iReadPos);
    m_iReadPos = m_iReadPos + 1;
    return rsl;
}
int CogeStreamBuffer::GetInt()
{
    char* src = (char*)m_pBuf;
    if(src == NULL) return 0;
    int dst = 0; int len = sizeof(int);
    memcpy((void*)(&dst), (void*)(src + m_iReadPos), len);
    m_iReadPos = m_iReadPos + len;
    return dst;
}
double CogeStreamBuffer::GetFloat()
{
    char* src = (char*)m_pBuf;
    if(src == NULL) return 0;
    double dst = 0; int len = sizeof(double);
    memcpy((void*)(&dst), (void*)(src + m_iReadPos), len);
    m_iReadPos = m_iReadPos + len;
    return dst;
}
std::string CogeStreamBuffer::GetStr()
{
    char* src = (char*)m_pBuf;
    if(src == NULL) return "";

    int iStrLen = 0;
    for(int i = m_iReadPos; i < m_iSize; i++)
    {
        if((*(src + i)) == '\0') break;
        else iStrLen++;
    }

    if(iStrLen <= 0) return "";

    char tmpbuf[256];

    memcpy((void*)(&tmpbuf[0]), (void*)(src + m_iReadPos), iStrLen);
    tmpbuf[iStrLen] = '\0';
    m_iReadPos = m_iReadPos + iStrLen;

    if(m_iReadPos < m_iSize) m_iReadPos = m_iReadPos + 1;

    std::string rsl = tmpbuf;

    return rsl;
}

int CogeStreamBuffer::SetByte(char value)
{
    char* dst = (char*)m_pBuf;
    dst[m_iWritePos] = value;
    m_iWritePos = m_iWritePos + 1;
    return m_iWritePos;
}
int CogeStreamBuffer::SetInt(int value)
{
    char* dst = (char*)m_pBuf;
    int len = sizeof(int);
    memcpy((void*)(dst+m_iWritePos), (void*)(&value), len);
    m_iWritePos = m_iWritePos + len;
    return m_iWritePos;
}
int CogeStreamBuffer::SetFloat(double value)
{
    char* dst = (char*)m_pBuf;
    int len = sizeof(double);
    memcpy((void*)(dst+m_iWritePos), (void*)(&value), len);
    m_iWritePos = m_iWritePos + len;
    return m_iWritePos;
}
int CogeStreamBuffer::SetStr(const std::string& value)
{
    char* dst = (char*)m_pBuf;
    int len = value.length();
    memcpy((void*)(dst+m_iWritePos), (void*)(value.c_str()), len);
    m_iWritePos = m_iWritePos + len;
    return SetByte('\0');
}

