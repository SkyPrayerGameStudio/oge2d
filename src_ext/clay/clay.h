/*
-----------------------------------------------------------------------------
This source file is part of "clay" library.
It is licensed under the terms of the MIT license.
For the latest info, see http://libclay.sourceforge.net

Copyright (c) 2010 Lin Jia Jun (Joe Lam)

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

#ifndef __CLAY_H_INCLUDED__
#define __CLAY_H_INCLUDED__

#define CLAY_SVC_FUNC(func) (clay::core::CLAY_SVC_CLASS_FUNC)(&func)

#include <map>
#include <string>

namespace clay
{

namespace core
{

class CClayServiceProvider;

typedef int (*CLAY_SVC_STATIC_FUNC)(void* pParam);

typedef int (CClayServiceProvider::*CLAY_SVC_CLASS_FUNC)(void* pParam);

typedef std::map<std::string, CLAY_SVC_CLASS_FUNC> ClayServiceMap;

typedef std::map<std::string, CClayServiceProvider*> ClayServiceProviderMap;

class CClayServiceProvider
{
protected:

    std::string m_sName;

    ClayServiceMap m_SvcFuncMap;

    void ClearServices();

    int RegisterService(const std::string& sSvcName, CLAY_SVC_CLASS_FUNC pSvcFunc);
    int UnregisterService(const std::string& sSvcName);

    int GetSvcCount();

    virtual void SetupServices();

    int GetVersion(void* pParam);

public:

    std::string GetName();

    bool ServiceExists(const std::string& sSvcName);

    virtual int DoService(const std::string& sSvcName, void* pParam);

    explicit CClayServiceProvider(const std::string& sName);
    virtual ~CClayServiceProvider();

};


class CClayServiceAgent
{

private:

    ClayServiceProviderMap  m_SpMap;

    CClayServiceAgent* m_pExternalAgent;

    CClayServiceAgent* GetExternalAgent()
    {
        return m_pExternalAgent;
    }


public:

    CClayServiceAgent();
    ~CClayServiceAgent();

    int RegSp(CClayServiceProvider* pServiceProvider);

    int UnregSp(const std::string& sSpName);

    int GetSpCount();

    int CallService(const std::string& sSpName, const std::string& sSvcName, void* pParam);

    bool SpExists(const std::string& sSpName);

    bool SvcExists(const std::string& sSpName, const std::string& sSvcName);

    void Clear();

    void SetExternalAgent(CClayServiceAgent* pAgent);

    // Meyer's Singleton ...
    static CClayServiceAgent* GetAgent()
    {
        static CClayServiceAgent instance;

        CClayServiceAgent* pAgent = instance.GetExternalAgent();

        if(pAgent != NULL) return pAgent;
        else return &instance;
    }


};


} // end of clay::core


namespace lib
{

/* get system time as a simple string */
std::string GetTimeStr();

/* write log */
void Log(const char* sMsg);

clay::core::CClayServiceAgent* GetAgent();

void SetAgent(void* pAgent);

int RegisterSp(clay::core::CClayServiceProvider* pServiceProvider);

int GetSpCount();

bool ProviderExists(const std::string& sSpName);

bool ServiceExists(const std::string& sSpName, const std::string& sSvcName);

int CallService(const std::string& sSpName, const std::string& sSvcName, void* pParam = NULL);

void Clear();


} // end of clay::lib

} // end of clay


#endif // __CLAY_H_INCLUDED__
