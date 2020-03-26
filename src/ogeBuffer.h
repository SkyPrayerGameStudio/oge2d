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

#ifndef __OGE_BUFFER_H_INCLUDED__
#define __OGE_BUFFER_H_INCLUDED__

#include <string>
#include <vector>
#include <map>

#define _OGE_MIN_BUFFER_SIZE_M_  1
#define _OGE_MAX_BUFFER_SIZE_M_  100

#define _OGE_BUFFER_SIZE_1_M_    1048576

enum ogeBufferSize
{
    BufferSize_1M   = 1,
    BufferSize_2M   = 2,
    BufferSize_3M   = 3,
    BufferSize_4M   = 4,
    BufferSize_5M   = 5,
    BufferSize_10M  = 10,
    BufferSize_20M  = 20,
    BufferSize_50M  = 50,
    BufferSize_100M = 100
};

class CogeBuffer
{
private:

    char* m_pBuf;

    int m_iSize;

    int m_iState;

    int m_iUserCount;

protected:


public:

    int GetState();
    int GetSize();
    char* GetBuffer();

    bool IsBusy();
    int Hire();
    int Fire();

    explicit CogeBuffer(ogeBufferSize iSize);
    ~CogeBuffer();

};


class CogeBufferManager
{
private:

    std::map<int, std::vector<CogeBuffer*>*> m_BufferLists;

    int m_iState;

    void FreeAll();
    void FreeBufferList(std::vector<CogeBuffer*>* pBufferList);

protected:

public:

    int GetState();

    CogeBuffer* GetFreeBuffer(int iRequiredSize = _OGE_BUFFER_SIZE_1_M_);

    CogeBufferManager();
    ~CogeBufferManager();

};


class CogeStreamBuffer
{
private:

    char* m_pBuf;

    int m_iSize;

    int m_iReadPos;

    int m_iWritePos;

protected:


public:

    int GetSize();
    char* GetBuffer();

    int GetReadPos();
    int GetWritePos();

    int SetReadPos(int iPos);
    int SetWritePos(int iPos);

    char GetByte();
    int GetInt();
    double GetFloat();
    std::string GetStr();

    int SetByte(char value);
    int SetInt(int value);
    int SetFloat(double value);
    int SetStr(const std::string& value);


    CogeStreamBuffer(char* pBuf, int iBufSize);
    ~CogeStreamBuffer();

};

#endif // __OGE_BUFFER_H_INCLUDED__
