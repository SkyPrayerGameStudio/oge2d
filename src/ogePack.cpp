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

#include "ogePack.h"

#include "ogeCommon.h"

//#ifdef __OGE_WITH_LZO__
//#include "minilzo.h"
//#el
#if defined __OGE_WITH_ZLIB__
#include "zlib.h"
#elif defined __OGE_WITH_SNAPPY__
#include "snappy.h"
#endif

#include "blowfish.h"

#include <stdint.h>

#include <cstring>
#include <fstream>

#define _OGE_MAX_FILE_NAME_LEN_         128
//#define _OGE_DF_ENC_KEY_                "THE_KEY"


//#ifdef __OGE_WITH_LZO__
//#define HEAP_ALLOC(var,size) lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]
//static HEAP_ALLOC(_LZO_COMPRESS_WORK_MEM_, LZO1X_1_MEM_COMPRESS);
//#endif


static const uint32_t gPackBeginSign  = 0x4145474F;
static const uint32_t gPackEndSign    = 0x5A45474F;
static const uint32_t gPackVersion    = 0x00000001;


/************* CogePackFile *****************/

CogePackFile::CogePackFile(CogePacker* pPacker)
{
    m_iState = -1;
    m_sFileName = "";
    m_pPacker = pPacker;
}

CogePackFile::~CogePackFile()
{
    FreeAll();
}

void CogePackFile::FreeAll()
{
    m_iState = -1;

    std::map<std::string, CogePackBlock*>::iterator it;
	CogePackBlock* pMatchedBlock = NULL;

	it = m_Blocks.begin();

	while (it != m_Blocks.end())
	{
		pMatchedBlock = it->second;
		m_Blocks.erase(it);
		if(pMatchedBlock)
		{
		    delete pMatchedBlock;
		    pMatchedBlock = NULL;
        }
		it = m_Blocks.begin();
	}
}

int CogePackFile::GetState()
{
    return m_iState;
}

int CogePackFile::Load(const std::string& sPackedFileName)
{
    FreeAll();

    std::ifstream ifile;
    ifile.open(sPackedFileName.c_str(), std::ios::binary);
    if (!ifile.is_open()) return -1;

    int iBeginSign = 0;
    //int iEndSign = 0;
    int iVersion = 0;
    int iBlockCount = 0;

    int iOffset = 0;
    int iOrgSize = 0;
    int iPkgSize = 0;
    int iEncSize = 0;
    int iFileNameLength = 0;

    ifile.read((char*)&iBeginSign, 4);
    ifile.read((char*)&iVersion, 4);
    ifile.read((char*)&iBlockCount, 4);

    char pFileName[_OGE_MAX_FILE_NAME_LEN_];

    for(int i=0; i<iBlockCount; i++)
    {
        ifile.read((char*)&iOffset, 4);
        ifile.read((char*)&iOrgSize, 4);
        ifile.read((char*)&iPkgSize, 4);
        ifile.read((char*)&iEncSize, 4);
        ifile.read((char*)&iFileNameLength, 4);

        memset(pFileName, 0, _OGE_MAX_FILE_NAME_LEN_);
        ifile.read((char*)pFileName, iFileNameLength);

        std::string sFileName(pFileName);
        CogePackBlock* pBlock = new CogePackBlock();
        pBlock->offset = iOffset;
        pBlock->orgsize = iOrgSize;
        pBlock->pkgsize = iPkgSize;
        pBlock->encsize = iEncSize;

        m_Blocks.insert(std::map<std::string, CogePackBlock*>::value_type(sFileName, pBlock));
    }

    ifile.close();

    m_sFileName = sPackedFileName;

    m_iState = 0;

    return m_iState;

}

int CogePackFile::GetBlockOriginalSize(const std::string& sBlockName)
{
    std::map<std::string, CogePackBlock*>::iterator it = m_Blocks.find(sBlockName);
    if(it == m_Blocks.end()) return 0;
    else return it->second->orgsize;
}

int CogePackFile::ReadBlock(const std::string& sBlockName, char* pOutputBuf, bool bHasCipher)
{
    if(m_iState < 0) return -1;
    if(m_pPacker == NULL) return -1;
    if(m_pPacker->m_iState < 0) return -1;

    std::map<std::string, CogePackBlock*>::iterator it = m_Blocks.find(sBlockName);
    if(it == m_Blocks.end()) return -1;

    CogePackBlock* pBlock = it->second;

    std::ifstream ifile;
    ifile.open(m_sFileName.c_str(), std::ios::binary);
    if (!ifile.is_open()) return -1;

    CogeBuffer* pInputBuffer = m_pPacker->m_pBufferManager->GetFreeBuffer(pBlock->encsize);
    if(pInputBuffer) pInputBuffer->Hire();

    if(pInputBuffer == NULL || pInputBuffer->GetState() < 0)
    {
        if(pInputBuffer) pInputBuffer->Fire();
        return 0;
    }


#ifdef _OGE_DF_ENC_KEY_
    BF_SetPassword(_OGE_DF_ENC_KEY_);
#else
    bHasCipher = false;
#endif


    char* pInputBuf = pInputBuffer->GetBuffer();
    char* pNewBuf = pOutputBuf;

    ifile.seekg(pBlock->offset, std::ios::beg);

    ifile.read(pInputBuf, pBlock->encsize);

    ifile.close();


#ifdef _OGE_DF_ENC_KEY_
    // start to decrypt ...
    // ... ...
    if(bHasCipher)
    {
        //printf("[%s] Before Decrypt: %d, %d, %d, %d \n", sBlockName.c_str(), *(pInputBuf), *(pInputBuf+1), *(pInputBuf+2), *(pInputBuf+3));
        BF_Decrypt((void*)pInputBuf, pBlock->encsize);
        //printf("[%s] After Decrypt: %d, %d, %d, %d \n", sBlockName.c_str(), *(pInputBuf), *(pInputBuf+1), *(pInputBuf+2), *(pInputBuf+3));
    }
    // end of decryption
#endif


    // start to decompress ...

/*
#ifdef __OGE_WITH_LZO__

    lzo_uint iNewSize = 0;
    int rsl = lzo1x_decompress((unsigned char*)pInputBuf, pBlock->pkgsize,
                               (unsigned char*)pNewBuf, &iNewSize, NULL);

    if(pInputBuffer) pInputBuffer->Fire();

    if (rsl != LZO_E_OK)
    {
        OGE_Log("internal error - decompression failed: %d\n", rsl);
        return 0;
    }

#el
*/

#if defined __OGE_WITH_ZLIB__

    unsigned long iNewSize = pBlock->orgsize;
    int rsl = uncompress((unsigned char*)pNewBuf, &iNewSize,
                         (unsigned char*)pInputBuf, pBlock->pkgsize);

    if(pInputBuffer) pInputBuffer->Fire();

    if (rsl != Z_OK)
    {
        OGE_Log("internal error - decompression failed: %d\n", rsl);
        return 0;
    }

#elif defined __OGE_WITH_SNAPPY__

    bool rsl = snappy::RawUncompress(pInputBuf, pBlock->pkgsize, pNewBuf);

    if(pInputBuffer) pInputBuffer->Fire();

    if (!rsl)
    {
        OGE_Log("internal error - decompression failed.\n");
        return 0;
    }

#else

    memcpy(pNewBuf, pInputBuf, pBlock->pkgsize);

#endif

    // end of decompression

    return pBlock->orgsize;

}

std::map<std::string, CogePackBlock*>* CogePackFile::GetBlocks()
{
    return &m_Blocks;
}


/************* CogePacker *****************/

CogePacker::CogePacker(CogeBufferManager* pBufferManager)
{
    m_iState = -1;
    m_pBufferManager = NULL;
    m_sCurrentPath = "";
    if(pBufferManager == NULL || pBufferManager->GetState() < 0) return;
    //if(lzo_init() != LZO_E_OK) return;
    m_pBufferManager = pBufferManager;
    m_iState = 0;

//#ifdef _OGE_DF_ENC_KEY_
//    printf("KEY: %s\n", _OGE_DF_ENC_KEY_);
//#endif
}

CogePacker::~CogePacker()
{
    m_iState = -1;
    m_sCurrentPath = "";
    m_pBufferManager = NULL;
    FreeAll();
}

void CogePacker::FreeAll()
{
    m_iState = -1;

    std::map<std::string, CogePackFile*>::iterator it;
	CogePackFile* pMatchedFile = NULL;

	it = m_Files.begin();

	while (it != m_Files.end())
	{
		pMatchedFile = it->second;
		m_Files.erase(it);
		if(pMatchedFile)
		{
		    delete pMatchedFile;
		    pMatchedFile = NULL;
        }
		it = m_Files.begin();
	}
}

CogePackFile* CogePacker::GetFile(std::string sFileName)
{
    CogePackFile* pFile = NULL;
    std::map<std::string, CogePackFile*>::iterator it = m_Files.find(sFileName);
    if(it == m_Files.end())
    {
        pFile = new CogePackFile(this);
        pFile->Load(sFileName);
        if(pFile->GetState() < 0)
        {
            delete pFile;
            return NULL;
        }
        m_Files.insert(std::map<std::string, CogePackFile*>::value_type(sFileName, pFile));

    } else pFile = it->second;

    return pFile;
}

int CogePacker::GetState()
{
    return m_iState;
}

int CogePacker::PackFiles(std::map<std::string, std::string> lstFiles, const std::string& sOutputFile, bool bHasCipher)
{
    if(m_pBufferManager == NULL) return 0;

    uint32_t iFileCount = lstFiles.size();

    if(iFileCount <= 0) return 0;

    std::ofstream ofile(sOutputFile.c_str(), std::ios::out | std::ios::binary);

    if (ofile.fail()) return -1;

    //ofile << gPackBeginSign;
    ofile.write((char*)&gPackBeginSign, sizeof(gPackBeginSign));
    //ofile << gPackVersion;
    ofile.write((char*)&gPackVersion, sizeof(gPackVersion));
    //ofile << lstFiles.size();
    ofile.write((char*)&iFileCount, sizeof(iFileCount));

    int iIndexOffset = ofile.tellp();

    std::map<std::string, std::string>::iterator it;
	std::string sFileName = "";
	std::string sFileSaveName = "";

	uint32_t iOffset = 0;
	uint32_t iOrgSize = 0;
	uint32_t iPkgSize = 0;
	uint32_t iEncSize = 0;

#ifdef _OGE_DF_ENC_KEY_
    BF_SetPassword(_OGE_DF_ENC_KEY_);
#else
    bHasCipher = false;
#endif

	//int rsl = 0;

	for(it=lstFiles.begin(); it!=lstFiles.end(); it++)
	{
	    ofile.write((char*)&iOffset, sizeof(iOffset));
	    ofile.write((char*)&iOrgSize, sizeof(iOrgSize));
	    ofile.write((char*)&iPkgSize, sizeof(iPkgSize));
	    ofile.write((char*)&iEncSize, sizeof(iEncSize));

	    sFileSaveName = it->second;

	    int iFileNameLength = sFileSaveName.length();

	    ofile.write((char*)&iFileNameLength, sizeof(iFileNameLength));

	    ofile.write(sFileSaveName.c_str(), sFileSaveName.length());
	}

	int iBlockOffset = ofile.tellp();

	for(it=lstFiles.begin(); it!=lstFiles.end(); it++)
	{
		sFileName = it->first;
		sFileSaveName = it->second;

		std::ifstream ifile;
		ifile.open(sFileName.c_str(), std::ios::binary);
		if (ifile.is_open())
        {
            ifile.seekg(0, std::ios::end);
            int iFileSize = ifile.tellg();
            if(iFileSize == 0)
            {
                ofile.seekp(iIndexOffset, std::ios::beg);

                iOffset = iBlockOffset;
                iOrgSize = 0;
                iPkgSize = 0;
                iEncSize = 0;

                ofile.write((char*)&iOffset, sizeof(iOffset));
                ofile.write((char*)&iOrgSize, sizeof(iOrgSize));
                ofile.write((char*)&iPkgSize, sizeof(iPkgSize));
                ofile.write((char*)&iEncSize, sizeof(iEncSize));

                iIndexOffset = ofile.tellp();
                iIndexOffset = iIndexOffset + 4 + sFileSaveName.length();
            }
            else
		    {
		        CogeBuffer* pInputBuffer = m_pBufferManager->GetFreeBuffer(iFileSize);
		        if(pInputBuffer) pInputBuffer->Hire();
		        CogeBuffer* pOutputBuffer = m_pBufferManager->GetFreeBuffer(iFileSize + 1024); // ...
		        if(pOutputBuffer) pOutputBuffer->Hire();

		        if(pInputBuffer == NULL || pInputBuffer->GetState() < 0
                    || pOutputBuffer == NULL || pOutputBuffer->GetState() < 0)
		        {
		            if(pInputBuffer) pInputBuffer->Fire();
		            if(pOutputBuffer) pOutputBuffer->Fire();

		            ofile.seekp(iIndexOffset, std::ios::beg);

                    iOffset = iBlockOffset;
                    iOrgSize = 0;
                    iPkgSize = 0;
                    iEncSize = 0;

                    ofile.write((char*)&iOffset, sizeof(iOffset));
                    ofile.write((char*)&iOrgSize, sizeof(iOrgSize));
                    ofile.write((char*)&iPkgSize, sizeof(iPkgSize));
                    ofile.write((char*)&iEncSize, sizeof(iEncSize));

                    iIndexOffset = ofile.tellp();
                    iIndexOffset = iIndexOffset + 4 + sFileSaveName.length();
		        }
		        else
		        {
                    char* pInputBuf = pInputBuffer->GetBuffer();
                    char* pOutputBuf = pOutputBuffer->GetBuffer();

                    ifile.seekg(0, std::ios::beg);
                    ifile.read(pInputBuf, iFileSize);


                    // start to compress ...

                    iOrgSize = iFileSize;

/*
#ifdef __OGE_WITH_LZO__

                    lzo_uint iNewSize = 0;
                    int rsl = lzo1x_1_compress((unsigned char*)pInputBuf, iOrgSize,
                                           (unsigned char*)pOutputBuf, &iNewSize, _LZO_COMPRESS_WORK_MEM_);

                    if (rsl != LZO_E_OK)
                    {
                        printf("Fail to pack file '%s', internal error - compression failed: %d\n", sFileName.c_str(), rsl);
                        memcpy(pOutputBuf, pInputBuf, iFileSize);
                        iPkgSize = iFileSize;
                        iNewSize = iFileSize;
                    }
                    else
                    {
                        iPkgSize = iNewSize;
                    }


#el
*/

#if defined __OGE_WITH_ZLIB__


                    unsigned long iNewSize = iFileSize + 1024; // ...
                    int rsl = compress((unsigned char*)pOutputBuf, &iNewSize,
                                       (const unsigned char*)pInputBuf, iOrgSize);

                    if (rsl != Z_OK)
                    {
                        printf("Fail to pack file '%s', internal error - compression failed: %d\n", sFileName.c_str(), rsl);
                        memcpy(pOutputBuf, pInputBuf, iFileSize);
                        iPkgSize = iFileSize;
                        iNewSize = iFileSize;
                    }
                    else
                    {
                        iPkgSize = iNewSize;
                    }


#elif defined __OGE_WITH_SNAPPY__

                    size_t iNewSize = 0;

                    snappy::RawCompress(pInputBuf, iOrgSize, pOutputBuf, &iNewSize);
                    iPkgSize = iNewSize;

#else

                    size_t iNewSize = iFileSize;
                    memcpy(pOutputBuf, pInputBuf, iFileSize);
                    iPkgSize = iFileSize;

#endif

                    // end of compression


#ifdef _OGE_DF_ENC_KEY_
                    // start to encrypt ...
                    // ... ...
                    if(bHasCipher)
                    {
                        iNewSize = iPkgSize % 8;
                        if(iNewSize != 0) iNewSize = iPkgSize + 8 - iNewSize;
                        else iNewSize = iPkgSize;
                        //printf("[%s] Before Encrypt: %d, %d, %d, %d \n", sFileSaveName.c_str(), *(pOutputBuf), *(pOutputBuf+1), *(pOutputBuf+2), *(pOutputBuf+3));
                        BF_Encrypt((void*)pOutputBuf, iNewSize);
                        //printf("[%s] After Encrypt: %d, %d, %d, %d \n", sFileSaveName.c_str(), *(pOutputBuf), *(pOutputBuf+1), *(pOutputBuf+2), *(pOutputBuf+3));
                        iEncSize = iNewSize;
                    }
                    else
                    {
                        iEncSize = iNewSize;
                    }
                    // end of encryption
#else
                    iEncSize = iNewSize;
#endif


                    iOffset = iBlockOffset;

                    ofile.seekp(iBlockOffset, std::ios::beg);
                    ofile.write(pOutputBuf, iNewSize);

                    iBlockOffset = ofile.tellp();


                    ofile.seekp(iIndexOffset, std::ios::beg);

                    ofile.write((char*)&iOffset, sizeof(iOffset));
                    ofile.write((char*)&iOrgSize, sizeof(iOrgSize));
                    ofile.write((char*)&iPkgSize, sizeof(iPkgSize));
                    ofile.write((char*)&iEncSize, sizeof(iEncSize));

                    iIndexOffset = ofile.tellp();
                    iIndexOffset = iIndexOffset + 4 + sFileSaveName.length();

                    if(pInputBuffer) pInputBuffer->Fire();
		            if(pOutputBuffer) pOutputBuffer->Fire();

		        }
		    }

            ifile.close();

        }
        else
        {
            ofile.seekp(iIndexOffset, std::ios::beg);

            iOffset = iBlockOffset;
            iOrgSize = 0;
            iPkgSize = 0;
            iEncSize = 0;

            ofile.write((char*)&iOffset, sizeof(iOffset));
            ofile.write((char*)&iOrgSize, sizeof(iOrgSize));
            ofile.write((char*)&iPkgSize, sizeof(iPkgSize));
            ofile.write((char*)&iEncSize, sizeof(iEncSize));

            iIndexOffset = ofile.tellp();
            iIndexOffset = iIndexOffset + 4 + sFileSaveName.length();

        }
	}

	ofile.seekp(iBlockOffset, std::ios::beg);

	ofile.write((char*)&gPackEndSign, sizeof(gPackEndSign));

	ofile.close();

    return 0;
}

int CogePacker::UnpackFile(const std::string& sPackedFile, const std::string& sOutputFolder)
{
    return 0;
}

int CogePacker::GetFileSize(const std::string& sFileName, const std::string& sBlockName)
{
    CogePackFile* pFile = GetFile(sFileName);
    if(pFile == NULL) return -1;
    else return pFile->GetBlockOriginalSize(sBlockName);
}

int CogePacker::ReadFile(const std::string& sFileName, const std::string& sBlockName, char* pOutputBuf, bool bHasCipher)
{
    CogePackFile* pFile = GetFile(sFileName);
    if(pFile == NULL) return -1;
    else return pFile->ReadBlock(sBlockName, pOutputBuf, bHasCipher);
}


