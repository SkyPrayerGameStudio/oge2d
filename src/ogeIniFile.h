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

#ifndef __OGE_INIFILE_H_INCLUDED__
#define __OGE_INIFILE_H_INCLUDED__

#include <string>
#include <vector>
#include <map>

class CogeIniFile
{
private:

    std::string    m_sFileName;

    std::map<std::string, std::map<std::string, std::string>*> m_SectionMap;

    std::string    m_sCurrentPath;

    int            m_iState;

protected:

    void Clear();

public:

    std::string ReadString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
    std::string ReadPath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
    std::string ReadFilePath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault);
    int ReadInteger(const std::string& sSectionName, const std::string& sFieldName, int iDefault);
    double ReadFloat(const std::string& sSectionName, const std::string& sFieldName, double fDefault);

    bool WriteString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue);
    bool WriteInteger(const std::string& sSectionName, const std::string& sFieldName, int iValue);
    bool WriteFloat(const std::string& sSectionName, const std::string& sFieldName, double fValue);

    bool Load(const std::string& sFileName);
    bool Save(const std::string& sFileName = "");

    bool Rewrite(const std::string& sFileName = "");

    bool LoadFromBuffer(char* pBuffer, int iBufferSize);

    void SetCurrentPath(const std::string& sPath);

    int GetSectionCount();
    std::string GetSectionNameByIndex(int iIndex);
    std::map<std::string, std::string>* GetSectionByIndex(int iIndex);
    std::map<std::string, std::string>* GetSectionByName(const std::string& sSectionName);


    int GetState();

    void Show();

    //constructor
    CogeIniFile();
    //destructor
    ~CogeIniFile();


};

#endif // __OGE_INIFILE_H_INCLUDED__
