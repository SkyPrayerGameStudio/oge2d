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

#include "clay.h"

#include <ctime>

#include <fstream>
#include <iostream>

#define _CLAY_LOG_FILE_  "clay.log"

using namespace clay::lib;
using namespace clay::core;

/* get system time as a simple string */
std::string clay::lib::GetTimeStr()
{
	std::string rsl;

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[20];

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

    rsl = buffer;

    return rsl;
}

/* write log */
void clay::lib::Log(const char* sMsg)
{
    std::ofstream logfile(_CLAY_LOG_FILE_, std::ios::out | std::ios::app);
	if (!logfile.fail())
	{
		logfile << "[" << clay::lib::GetTimeStr() <<  "]: " << sMsg << "\n";
		logfile.close();
	}
}

int clay::lib::GetSpCount()
{
    return CClayServiceAgent::GetAgent()->GetSpCount();
}

clay::core::CClayServiceAgent* clay::lib::GetAgent()
{
    return CClayServiceAgent::GetAgent();
}

void clay::lib::SetAgent(void* pAgent)
{
    return CClayServiceAgent::GetAgent()->SetExternalAgent((clay::core::CClayServiceAgent*)pAgent);
}

int clay::lib::RegisterSp(clay::core::CClayServiceProvider* pServiceProvider)
{
    return CClayServiceAgent::GetAgent()->RegSp(pServiceProvider);
}

int clay::lib::CallService(const std::string& sSpName, const std::string& sSvcName, void* pParam)
{
    return CClayServiceAgent::GetAgent()->CallService(sSpName, sSvcName, pParam);
}

void clay::lib::Clear()
{
    CClayServiceAgent::GetAgent()->Clear();
}

bool clay::lib::ProviderExists(const std::string& sSpName)
{
    return CClayServiceAgent::GetAgent()->SpExists(sSpName);
}

bool clay::lib::ServiceExists(const std::string& sSpName, const std::string& sSvcName)
{
    return CClayServiceAgent::GetAgent()->SvcExists(sSpName, sSvcName);
}

/*------------------ ClayServiceProvider ---------------------*/

static const char* _CLAY_CUSTOM_SVC_PROVIDER_VERSION_ = "Custom Base ServiceProvider v1.0.0";
static const char* _CLAY_CUSTOM_SVC_GET_VERSION_ = "GetVersion";

CClayServiceProvider::CClayServiceProvider(const std::string& sName)
{
    m_sName = sName;
}

CClayServiceProvider::~CClayServiceProvider()
{
    ClearServices();
}

std::string CClayServiceProvider::GetName()
{
    return m_sName;
}

void CClayServiceProvider::ClearServices()
{
    m_SvcFuncMap.clear();
}

int CClayServiceProvider::GetSvcCount()
{
    return m_SvcFuncMap.size();
}

int CClayServiceProvider::RegisterService(const std::string& sSvcName, CLAY_SVC_CLASS_FUNC pSvcFunc)
{
    if (pSvcFunc==NULL) return -1;
    ClayServiceMap::iterator it;
	it = m_SvcFuncMap.find(sSvcName);
	if (it != m_SvcFuncMap.end())
	{
        return 0;
	}
	else
	{
		m_SvcFuncMap.insert(ClayServiceMap::value_type(sSvcName, pSvcFunc));
		return 1;
	}
}

int CClayServiceProvider::UnregisterService(const std::string& sSvcName)
{
    ClayServiceMap::iterator it;
	it = m_SvcFuncMap.find(sSvcName);
	if (it != m_SvcFuncMap.end())
	{
        m_SvcFuncMap.erase(it);
        return 1;
	}
	else
	{
		return 0;
	}
}

int CClayServiceProvider::DoService(const std::string& sSvcName, void* pParam)
{
    ClayServiceMap::iterator it = m_SvcFuncMap.find(sSvcName);

    if (it != m_SvcFuncMap.end())
	{
        CLAY_SVC_CLASS_FUNC svc = (CLAY_SVC_CLASS_FUNC)(it->second);
        return (this->*svc)(pParam);
	}
	else
	{
		std::cout << "ServiceProvider::DoService: Can not find Service " <<  m_sName << "::" << sSvcName << std::endl;
		//throw new std::exception();
		return 0;
	}

}

void CClayServiceProvider::SetupServices()
{
    RegisterService(_CLAY_CUSTOM_SVC_GET_VERSION_, &CClayServiceProvider::GetVersion);
}

int CClayServiceProvider::GetVersion(void* pParam)
{
    return (int)_CLAY_CUSTOM_SVC_PROVIDER_VERSION_;
}

bool CClayServiceProvider::ServiceExists(const std::string& sSvcName)
{
    return m_SvcFuncMap.find(sSvcName) != m_SvcFuncMap.end();
}


/*------------------ ClayServiceAgent ---------------------*/

int CClayServiceAgent::RegSp(CClayServiceProvider* pServiceProvider)
{
    if (pServiceProvider==NULL) return -1;
    ClayServiceProviderMap::iterator it;
    std::string sSpName = pServiceProvider->GetName();
	it = m_SpMap.find(sSpName);
	if (it != m_SpMap.end())
	{
        return 1;
	}
	else
	{
		m_SpMap.insert(ClayServiceProviderMap::value_type(sSpName, pServiceProvider));
		return 0;
	}

}

int CClayServiceAgent::UnregSp(const std::string& sSpName)
{
    ClayServiceProviderMap::iterator it;
	it = m_SpMap.find(sSpName);
	if (it != m_SpMap.end())
	{
	    CClayServiceProvider* sp = it->second;
        m_SpMap.erase(it);
        if (sp) delete sp;
        return 1;
	}
	else
	{
		std::cout << "ServiceAgent::UnRegSp: Can not find ServiceProvider " <<  sSpName << std::endl;
		return 0;
	}

}

int CClayServiceAgent::GetSpCount()
{
    return m_SpMap.size();
}

int CClayServiceAgent::CallService(const std::string& sSpName, const std::string& sSvcName, void* pParam)
{
    ClayServiceProviderMap::iterator it = m_SpMap.find(sSpName);

    if (it != m_SpMap.end())
	{
         return it->second->DoService(sSvcName, pParam);
	}
	else
	{
		std::cout << "ServiceAgent::CallService: Can not find ServiceProvider " <<  sSpName << std::endl;
		//throw new std::exception();
		return 0;
	}


}

bool CClayServiceAgent::SpExists(const std::string& sSpName)
{
    return m_SpMap.find(sSpName) != m_SpMap.end();
}

bool CClayServiceAgent::SvcExists(const std::string& sSpName, const std::string& sSvcName)
{
    ClayServiceProviderMap::iterator it = m_SpMap.find(sSpName);
    if (it != m_SpMap.end()) return it->second->ServiceExists(sSvcName);
	else return false;
}

void CClayServiceAgent::Clear()
{
    // free all
	ClayServiceProviderMap::iterator it;
	CClayServiceProvider* sp = NULL;

	it = m_SpMap.begin();

	while (it != m_SpMap.end())
	{
        sp = it->second;
		m_SpMap.erase(it);
		if (sp) delete sp;
		it = m_SpMap.begin();
	}

    if (m_SpMap.size() > 0) m_SpMap.clear();
}

void CClayServiceAgent::SetExternalAgent(CClayServiceAgent* pAgent)
{
    m_pExternalAgent = pAgent;
}

CClayServiceAgent::CClayServiceAgent()
{
    m_pExternalAgent = NULL;
}

CClayServiceAgent::~CClayServiceAgent()
{
    Clear();
}


