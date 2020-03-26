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

#include "ogeIniFile.h"

#include "ogeCommon.h"

#include <cstdlib>
#include <fstream>

#define _OGE_MAX_INISTR_LENGTH_ 128

static std::string trim(std::string const& source, char const* delims = " \t\r\n")
{
    std::string result(source);
    std::string::size_type index = result.find_last_not_of(delims);
    if(index != std::string::npos) result.erase(++index);
    index = result.find_first_not_of(delims);
    if(index != std::string::npos) result.erase(0, index);
    else result.erase();
    return result;
}

CogeIniFile::CogeIniFile()
{
    m_iState = -1;
    m_sFileName = "";
    m_sCurrentPath = "";
}

CogeIniFile::~CogeIniFile()
{
    m_iState = -1;
    Clear();
}

int CogeIniFile::GetState()
{
    return m_iState;
}

void CogeIniFile::Show()
{
    std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.begin();

    while(it != m_SectionMap.end())
	{

	    OGE_Log("\n[%s]\n", it->first.c_str());

        std::map<std::string, std::string>* pFieldMap = it->second;

        if(pFieldMap)
        {
            std::map<std::string, std::string>::iterator its = pFieldMap->begin();

	        while(its != pFieldMap->end())
	        {
                OGE_Log("%s=%s\n", its->first.c_str(), its->second.c_str());
	            its++;
	        }
        }
	    it++;
	}
}

void CogeIniFile::Clear()
{
    std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.begin();

    while(it != m_SectionMap.end())
	{
	    std::map<std::string, std::string>* pFieldMap = it->second;

	    m_SectionMap.erase(it);

	    if(pFieldMap)
	    {
	        pFieldMap->clear();
	        delete pFieldMap;
	    }

	    it = m_SectionMap.begin();
	}

}

bool CogeIniFile::Rewrite(const std::string& sFileName)
{
    Clear();
    m_sFileName = sFileName;
    m_iState = 0;
    return true;
}

bool CogeIniFile::Save(const std::string& sFileName)
{
	FILE *stream;
	if (sFileName.length() > 0)
	{
		if( (stream = fopen( sFileName.c_str(), "w" )) == NULL ) return false;
	}
	else
	{
	    if (m_sFileName.length() <= 0) return false;
		if( (stream = fopen( m_sFileName.c_str(), "w" )) == NULL ) return false;
	}

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.begin();

    while(it != m_SectionMap.end())
	{
	    if (fputs(("\n[" + it->first + "]\n").c_str(), stream)<0)
        {
            fclose(stream);
            return false;
        }

        std::map<std::string, std::string>* pFieldMap = it->second;

        if(pFieldMap)
        {
            std::map<std::string, std::string>::iterator its = pFieldMap->begin();

	        while(its != pFieldMap->end())
	        {
                if (fputs((its->first + "=" + its->second + "\n").c_str(), stream)<0)
                {
                    fclose(stream);
                    return false;
                }
	            its++;
	        }
        }

	    it++;
	}

	if (fflush(stream)==0)
	{
		fclose(stream);
		return true;
	}
	else
	{
        fclose(stream);
        return false;
	}

}

void CogeIniFile::SetCurrentPath(const std::string& sPath)
{
    m_sCurrentPath = trim(sPath);

    for(unsigned i=0; i<m_sCurrentPath.length(); i++)
    if( m_sCurrentPath[i] == '\\' ) m_sCurrentPath[i] = '/';

    if(m_sCurrentPath[m_sCurrentPath.length()-1] != '/')
    m_sCurrentPath.append(1, '/');

}


bool CogeIniFile::Load(const std::string& sFileName)
{
    FILE *stream;

    if( (stream = fopen(sFileName.c_str(), "r" )) == NULL ) return false;

    m_iState = -1;

    m_sFileName = "";

    Clear();

    char line[_OGE_MAX_INISTR_LENGTH_];
	size_t i, j, linecount = 0;
	std::string sLine = "";
	std::string sSection = "";
	std::string sField   = "";
	std::string sValue   = "";
	std::map<std::string, std::string>* pFieldMap = NULL;

    /* Cycle until end of file reached: */
    while( !feof( stream ) )
    {
        /* Attempt to read one line: */
        if( fgets( line, _OGE_MAX_INISTR_LENGTH_, stream ) != NULL )
        {
            sLine = line;
            sLine = trim(sLine);
            if (sLine.length() <= 2) continue;
            i = sLine.find('[', 0);
            j = sLine.find(']', 0);
            if (i!=std::string::npos && j!=std::string::npos && j>i+1)
            {
                sSection = sLine.substr(i+1, j-i-1);
                pFieldMap = new std::map<std::string, std::string>();
                m_SectionMap.insert(
                    std::map<std::string, std::map<std::string, std::string>*>::value_type(sSection, pFieldMap));
            }
            else
            {
                i = sLine.find('=', 0);
                if ( i!=std::string::npos && i!=0 && pFieldMap!=NULL )
                {
                    sField = sLine.substr(0, i);
                    sValue = sLine.substr(i+1);
                    pFieldMap->insert(
                        std::map<std::string, std::string>::value_type(sField, sValue));

                }
            }

            linecount++;

        }
        else if (!feof(stream))
        {
            fclose( stream );
            return false;
        }

    }
    //printf( "Number of lines read = %d\n", linecount );
    fclose( stream );
    m_sFileName = sFileName;
    m_iState = 0;
    return true;
}

bool CogeIniFile::LoadFromBuffer(char* pBuffer, int iBufferSize)
{
    if(iBufferSize <= 0) return false;

    size_t i = 0;
    size_t j = 0;

    std::string sLine = "";
    std::string sSection = "";
	std::string sField   = "";
	std::string sValue   = "";
	std::map<std::string, std::string>* pFieldMap = NULL;

	m_iState = -1;

	m_sFileName = "";

	Clear();

	for(int k=0; k<iBufferSize; k++)
	{
	    char ch = pBuffer[k];

	    //printf("%c", ch);

	    if(ch == '\r' || ch == '\n')
	    {
	        if(sLine.length() > 2)
	        {
	            //sLine = trim(sLine);
                //if (sLine.length() == 0) continue;
                i = sLine.find('[', 0);
                j = sLine.find(']', 0);
                if (i!=std::string::npos && j!=std::string::npos && j>i+1)
                {
                    sSection = sLine.substr(i+1, j-i-1);
                    pFieldMap = new std::map<std::string, std::string>();
                    m_SectionMap.insert(
                        std::map<std::string, std::map<std::string, std::string>*>::value_type(sSection, pFieldMap));
                }
                else
                {
                    i = sLine.find('=', 0);
                    if ( i!=std::string::npos && i!=0 && pFieldMap!=NULL )
                    {
                        sField = sLine.substr(0, i);
                        sValue = sLine.substr(i+1);
                        pFieldMap->insert(
                            std::map<std::string, std::string>::value_type(sField, sValue));

                    }
                }
	        }

	        sLine = "";

	    } else sLine.append(1, ch);

	}

	m_iState = 0;

	return true;

}

bool CogeIniFile::WriteInteger(const std::string& sSectionName, const std::string& sFieldName, int iValue)
{
	if (m_iState < 0) return false;

	char c[32];
	std::string sValue(OGE_itoa(iValue, c, 10));

	return WriteString(sSectionName, sFieldName, sValue);

}

bool CogeIniFile::WriteFloat(const std::string& sSectionName, const std::string& sFieldName, double fValue)
{
    if (m_iState < 0) return false;

    char c[32];
    sprintf(c,"%0.2f",fValue);

    std::string sValue(c);

	return WriteString(sSectionName, sFieldName, sValue);
}

bool CogeIniFile::WriteString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sValue)
{
    if (m_iState < 0) return false;

	std::map<std::string, std::string>* pFieldMap = NULL;

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
	if(it == m_SectionMap.end())
	{
	    pFieldMap = new std::map<std::string, std::string>();
        m_SectionMap.insert(
            std::map<std::string, std::map<std::string, std::string>*>::value_type(sSectionName, pFieldMap));
	}
	else pFieldMap = it->second;

	if(pFieldMap == NULL) return false;

	std::map<std::string, std::string>::iterator its = pFieldMap->find(sFieldName);
	if(its != pFieldMap->end()) its->second = sValue;
	else pFieldMap->insert(std::map<std::string, std::string>::value_type(sFieldName, sValue));

	return true;

}

int CogeIniFile::ReadInteger(const std::string& sSectionName, const std::string& sFieldName, int iDefault)
{
    if (m_iState < 0) return iDefault;

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
	if(it == m_SectionMap.end()) return iDefault;
	else
	{
	    std::map<std::string, std::string>* pFieldMap = it->second;
        if(pFieldMap == NULL) return iDefault;
        std::map<std::string, std::string>::iterator its = pFieldMap->find(sFieldName);
        if(its == pFieldMap->end()) return iDefault;
        else return atoi(its->second.c_str());
	}
}

double CogeIniFile::ReadFloat(const std::string& sSectionName, const std::string& sFieldName, double fDefault)
{
    if (m_iState < 0) return fDefault;

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
	if(it == m_SectionMap.end()) return fDefault;
	else
	{
	    std::map<std::string, std::string>* pFieldMap = it->second;
        if(pFieldMap == NULL) return fDefault;
        std::map<std::string, std::string>::iterator its = pFieldMap->find(sFieldName);
        if(its == pFieldMap->end()) return fDefault;
        else return atof(its->second.c_str());
	}
}

std::string CogeIniFile::ReadString(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    if (m_iState < 0) return sDefault;

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
	if(it == m_SectionMap.end()) return sDefault;
	else
	{
	    std::map<std::string, std::string>* pFieldMap = it->second;
        if(pFieldMap == NULL) return sDefault;
        std::map<std::string, std::string>::iterator its = pFieldMap->find(sFieldName);
        if(its == pFieldMap->end()) return sDefault;
        else return its->second;
	}
}

std::string CogeIniFile::ReadPath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    std::string sPath = sDefault;

    if(m_iState >= 0)
    {
        std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
        if(it != m_SectionMap.end())
        {
            std::map<std::string, std::string>* pFieldMap = it->second;
            if(pFieldMap != NULL)
            {
                std::map<std::string, std::string>::iterator its = pFieldMap->find(sFieldName);
                if(its != pFieldMap->end()) sPath = its->second;
            }
        }
    }

    //sPath = OGE_StrReplace(sPath, "\\", "/");
    for(unsigned i=0; i<sPath.length(); i++) if( sPath[i] == '\\' ) sPath[i] = '/'; // fast?

	if (sPath.length() >= 2)
    {
        if (sPath.at(0) == '/' || sPath.at(1) == ':') return sPath; // absolute dir
        else return m_sCurrentPath + sPath;
    }
    else return m_sCurrentPath + sPath;
}

std::string CogeIniFile::ReadFilePath(const std::string& sSectionName, const std::string& sFieldName, const std::string& sDefault)
{
    if (sDefault.length() > 0) return ReadPath(sSectionName, sFieldName, sDefault);

    std::string sValue = ReadString(sSectionName, sFieldName, sDefault);
    if (sValue.length() > 0) sValue = ReadPath(sSectionName, sFieldName, "");
    return sValue;
}

int CogeIniFile::GetSectionCount()
{
    return m_SectionMap.size();
}
std::string CogeIniFile::GetSectionNameByIndex(int iIndex)
{
    int idx = 0;
    std::string sName = "";
    std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.begin();
    while(it != m_SectionMap.end())
    {
        sName = it->first;
        if(idx == iIndex) break;
        idx++;
        it++;
    }
    return sName;
}
std::map<std::string, std::string>* CogeIniFile::GetSectionByIndex(int iIndex)
{
    int idx = 0;
    std::map<std::string, std::string>* pSection = NULL;
    std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.begin();
    while(it != m_SectionMap.end())
    {
        pSection = it->second;
        if(idx == iIndex) break;
        idx++;
        it++;
    }
    return pSection;
}

std::map<std::string, std::string>* CogeIniFile::GetSectionByName(const std::string& sSectionName)
{
    if (m_iState < 0) return NULL;

	std::map<std::string, std::map<std::string, std::string>*>::iterator it = m_SectionMap.find(sSectionName);
	if(it == m_SectionMap.end()) return NULL;
	else return it->second;
}




