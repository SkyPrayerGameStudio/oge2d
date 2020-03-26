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

#include "clay_plugin.h"
#include "clay.h"

#ifdef __CLAY_WX__
#include "wx/dynlib.h"
#elif __CLAY_SDL__
#include "SDL_loadso.h"
#else

#ifdef __WIN32__
#include "windows.h"
#elif __LINUX__
#include "dlfcn.h"
#else
// for more platforms...
#endif

#endif

using namespace clay::lib;
using namespace clay::core;
using namespace clay::plugin;

static CClayModuleManager g_ClayModuleManager;

/************************* CClayModule ****************************/

CClayModule::CClayModule(const std::string& sName)
{
    m_sName = sName;

    #ifdef __CLAY_WX__
    module = new wxDynamicLibrary();
    #elif __CLAY_SDL__
    module = NULL;
    #else

    #ifdef __WIN32__
    module = NULL;
    #elif __LINUX__
    module = NULL;
    #else
    // for more platforms...
    module = NULL;
    #endif

    #endif

}

CClayModule::~CClayModule()
{
    if(IsLoaded()) Unload();

    #ifdef __CLAY_WX__
    if(module)
    {
        wxDynamicLibrary* p = (wxDynamicLibrary*)module;
        delete p;
        p = NULL;
        module = NULL;
    }
    #elif __CLAY_SDL__
    module = NULL;
    #else

    #ifdef __WIN32__
    module = NULL;
    #elif __LINUX__
    module = NULL;
    #else
    // for more platforms...
    module = NULL;
    #endif

    #endif

}

std::string CClayModule::GetName()
{
    return m_sName;
}

void* CClayModule::GetHandle()
{
    return module;
}

void* CClayModule::Load(const std::string& sLibraryFileName)
{

    #ifdef __CLAY_WX__
    if(module)
	{
	    wxString sLibraryFile(sLibraryFileName.c_str(), wxConvLocal);
	    if(((wxDynamicLibrary*)module)->Load(sLibraryFile)) return module;
	    else return NULL;
	}
	else return NULL;
    #elif __CLAY_SDL__
    module = SDL_LoadObject(sLibraryFileName.c_str());
    return module;
    #else

    #ifdef __WIN32__
	module = LoadLibrary(sLibraryFileName.c_str());
	return module;
	#elif __LINUX__
	module = dlopen(sLibraryFileName.c_str(), RTLD_LAZY);
	return module;
	#else
    // for more platforms...
    module = NULL;
    return module;
    #endif

    #endif


}

void CClayModule::Unload()
{
    if(!module) return;

    #ifdef __CLAY_WX__
    ((wxDynamicLibrary*)module)->Unload();
    #elif __CLAY_SDL__
    SDL_UnloadObject(module);
    module = NULL;
    #else

    #ifdef __WIN32__
    FreeLibrary((HINSTANCE)module);
    module = NULL;
    #elif __LINUX__
    dlclose(module);
    module = NULL;
    #else
    // for more platforms...
    module = NULL;
    #endif

    #endif
}

bool CClayModule::IsLoaded()
{
    if(!module) return false;

    #ifdef __CLAY_WX__
    return ((wxDynamicLibrary*)module)->IsLoaded();
    #elif __CLAY_SDL__
    return true;
    #else

    #ifdef __WIN32__
    return true;
    #elif __LINUX__
    return true;
    #else
    // for more platforms...
    return true;
    #endif

    #endif
}

void* CClayModule::GetFunction(const std::string& sFunctionName)
{
    if(!module) return NULL;

    void* func = NULL;

    #ifdef __CLAY_WX__
    wxString sFunction(sFunctionName.c_str(), wxConvLocal);
    func = ((wxDynamicLibrary*)module)->GetSymbol(sFunction);
    #elif __CLAY_SDL__
    func = SDL_LoadFunction(module, sFunctionName.c_str());
    #else

    #ifdef __WIN32__
    func = (void*) GetProcAddress((HINSTANCE)module, sFunctionName.c_str());
    #elif __LINUX__
    func = dlsym(module, sFunctionName.c_str());
    #else
    // for more platforms...
    func = NULL;
    #endif

    #endif

    return func;
}


/************************* CClayModuleManager ****************************/

static const std::string _CLAY_AGENT_INIT_   = "InitAgent";
static const std::string _CLAY_SERVICE_INIT_ = "InitService";

CClayModuleManager::CClayModuleManager()
{
    return;
}

CClayModuleManager::~CClayModuleManager()
{
    FreeAllModules();
}

void CClayModuleManager::FreeAllModules()
{
    clay::lib::Clear();
    while(m_ModuleMap.size() > 0)
	{
		ClayModuleMap::iterator it = m_ModuleMap.begin();
		CClayModule* module = it->second;
		if(module) delete module;
		m_ModuleMap.erase(it);
	}
}

std::string CClayModuleManager::GetValidPath(const std::string& sFilePath)
{
    return sFilePath;
}

CClayModule* CClayModuleManager::LoadModule(const std::string& sLibraryName, const std::string& sLibraryFilePath)
{
    CClayModule* module = FindModule(sLibraryName);
    if(module != NULL) return module;

    std::string sFilePath = GetValidPath(sLibraryFilePath);

    module = new CClayModule(sLibraryName);

    module->Load(sFilePath);

    if(!module->IsLoaded())
    {
        delete module;
        return NULL;
    }

    CLAY_SVC_STATIC_FUNC func = NULL;

    // pass agent ...
    func = (CLAY_SVC_STATIC_FUNC) module->GetFunction(_CLAY_AGENT_INIT_);
    if(func == NULL)
    {
        delete module;
        return NULL;
    }
    clay::core::CClayServiceAgent* pAgent = clay::lib::GetAgent();
    if (pAgent != NULL) func((void*)pAgent);


    // register service ...

    func = (CLAY_SVC_STATIC_FUNC) module->GetFunction(_CLAY_SERVICE_INIT_);

    if(func == NULL)
    {
        delete module;
        return NULL;
    }

    CClayServiceProvider* sp = (CClayServiceProvider*) func((void*)(sLibraryName.c_str()));

    if(sp==NULL)
    {
        delete module;
        return NULL;
    }

    int result = clay::lib::RegisterSp(sp);

    if(result < 0)
    {
        delete sp;
        delete module;
        return NULL;
    }

    m_ModuleMap.insert(ClayModuleMap::value_type(sLibraryName, module));

    return module;

}

CClayModule* CClayModuleManager::FindModule(const std::string& sLibraryName)
{
    CClayModule* pMatchedModule = NULL;
    ClayModuleMap::iterator it = m_ModuleMap.find(sLibraryName);
	if (it != m_ModuleMap.end()) pMatchedModule = it->second;
	return pMatchedModule;
}

int clay::lib::Plugin(const std::string& sLibraryName, const std::string& sLibraryFilePath)
{
    return (int) g_ClayModuleManager.LoadModule(sLibraryName, sLibraryFilePath);
}

