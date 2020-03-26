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

#ifndef __CLAY_PLUGIN_H_INCLUDED__
#define __CLAY_PLUGIN_H_INCLUDED__

#include <map>
#include <string>

namespace clay
{

namespace plugin
{

class CClayModule
{

private:

    std::string m_sName;

	void* module;

public:

	explicit CClayModule(const std::string& sName);
	virtual ~CClayModule();

	std::string GetName();

	void* GetHandle();

    void* Load(const std::string& sLibraryFileName);
	void Unload();
	bool IsLoaded();

	void* GetFunction(const std::string& sFunctionName);

};

typedef std::map<std::string, CClayModule*> ClayModuleMap;

class CClayModuleManager
{

private:

    ClayModuleMap m_ModuleMap;

    void FreeAllModules();

    std::string GetValidPath(const std::string& sFilePath);

public:

	CClayModuleManager();
	~CClayModuleManager();

    CClayModule* LoadModule(const std::string& sLibraryName, const std::string& sLibraryFilePath);

    CClayModule* FindModule(const std::string& sLibraryName);

};

} // end of clay::plugin

namespace lib
{

int Plugin(const std::string& sLibraryName, const std::string& sLibraryFilePath);


} // end of clay::lib

} // end of clay

#endif // __CLAY_PLUGIN_H_INCLUDED__
