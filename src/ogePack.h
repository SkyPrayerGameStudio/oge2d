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

#ifndef __OGE_PACK_H_INCLUDED__
#define __OGE_PACK_H_INCLUDED__

#include "ogeBuffer.h"

#include <string>
#include <map>

struct CogePackBlock
{
    int offset;
    int orgsize;
    int pkgsize;
    int encsize;
};

class CogePacker;

class CogePackFile
{
private:

    CogePacker* m_pPacker;

    std::string m_sFileName;

    std::map<std::string, CogePackBlock*> m_Blocks;

    int m_iState;

    void FreeAll();

protected:


public:

    int GetState();

    int Load(const std::string& sFileName);

    int GetBlockOriginalSize(const std::string& sBlockName);
    int ReadBlock(const std::string& sBlockName, char* pOutputBuf, bool bHasCipher = false);

    std::map<std::string, CogePackBlock*>* GetBlocks();

    explicit CogePackFile(CogePacker* pPacker);
    ~CogePackFile();

    friend class CogePacker;

};

class CogePacker
{
private:

    CogeBufferManager* m_pBufferManager;

    std::map<std::string, CogePackFile*> m_Files;

    std::string m_sCurrentPath;

    int m_iState;

    void FreeAll();

protected:

public:

    int GetState();

    CogePackFile* GetFile(std::string sFileName);

    int PackFiles(std::map<std::string, std::string> lstFiles, const std::string& sOutputFile, bool bHasCipher = false);
    int UnpackFile(const std::string& sPackedFile, const std::string& sOutputFolder);

    int GetFileSize(const std::string& sFileName, const std::string& sBlockName);
    int ReadFile(const std::string& sFileName, const std::string& sBlockName, char* pOutputBuf, bool bHasCipher = false);

    explicit CogePacker(CogeBufferManager* pBufferManager);
    ~CogePacker();

    friend class CogePackFile;

};

#endif // __OGE_PACK_H_INCLUDED__
