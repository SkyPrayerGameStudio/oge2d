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

#ifndef __OGE_SCRIPT_H_INCLUDED__
#define __OGE_SCRIPT_H_INCLUDED__

#include "angelscript.h"

#include "scriptbuilder.h"
#include "debugger.h"

#include <string>
#include <list>
#include <map>

#define _OGE_MAX_BP_COUNT_  32

//#define NEW_SCRIPT_STR(s) new std::string(s)
//typedef std::string* ogeScriptString;

//#define NEW_SCRIPT_STR(s) s
//#define ogeScriptString   const std::string&


enum ogeDebugMode
{
	Debug_None     = 0,
	Debug_Local    = 1,
	Debug_Remote   = 2
};

struct CogeBreakpoint
{
    bool used;
    std::string filename;
    int line;
};

class CogeScript;

class CogeScriptBinaryStream : public asIBinaryStream
{
private:

    //std::string m_sReadFile;
    //std::string m_sWriteFile;

    char* m_pReadBuffer;
    char* m_pWriteBuffer;

    int m_iReadBufSize;
    int m_iWriteBufSize;

    int m_iReadPos;
    int m_iWritePos;

public:
	virtual void Read(void *ptr, asUINT size);
	virtual void Write(const void *ptr, asUINT size);

public:

    CogeScriptBinaryStream();
	virtual ~CogeScriptBinaryStream();

	void SetReadBuffer(char* pReadBuffer, int iReadBufSize);
	void SetWriteBuffer(char* pWriteBuffer, int iWriteBufSize);

	void Clear();

	int GetReadSize();
	int GetWriteSize();

};

class CogeScriptDebugger: public CDebugger
{

public:

    virtual void ListLocalVariables(asIScriptContext *ctx);
	virtual void ListGlobalVariables(asIScriptContext *ctx);

    virtual void Output(const std::string &str);

    virtual std::string ToString(void *value, asUINT typeId, bool expandMembers, bool isString, asIScriptEngine *engine);

};

class CogeScripter
{
private:

    typedef std::map<std::string, CogeScript*> ogeScriptMap;
    typedef std::list<CogeScript*> ogeScriptList;

    asIScriptEngine*  m_pScriptEngine;
    CScriptBuilder*   m_pScriptBuilder;
    CDebugger*        m_pScriptDebugger;

    ogeScriptMap      m_ScriptMap;
    ogeScriptList     m_ScriptList;

    int m_iState;

    int m_iNextId;

    int m_iDebugMode;    // 0: no debug; 1: local debug; 2: remote debug (not ready)
    int m_iBinaryMode;   // 0: the input is text code; 1: the input is binary code

    void* m_pOnCheckDebugRequest;  // for remote debug

    void DelAllScripts();


protected:

public:

    CogeScripter();
    ~CogeScripter();

    int GetState();

    int GetNextScriptId();

    int SetDebugMode(int value);
    int GetDebugMode();

    int SetBinaryMode(int value);
    int GetBinaryMode();

    bool IsSuspended();

    CogeScript* NewSingleScript(const std::string& sScriptName, const std::string& sFileName);
    CogeScript* NewSingleScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize);

    CogeScript* NewFixedScript(const std::string& sFileName);
    CogeScript* NewFixedScriptFromBuffer(char* pInputBuffer, int iBufferSize);

    CogeScript* NewScript(const std::string& sScriptName, const std::string& sFileName);
    CogeScript* NewScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize);
    CogeScript* FindScript(const std::string& sScriptName);
    CogeScript* GetScript(const std::string& sScriptName, const std::string& sFileName);
    CogeScript* GetScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize);
    int DelScript(const std::string& sScriptName);
    int DelScript(CogeScript* pScript);

    int SaveBinaryCode(const std::string& sScriptName, char* pOutputBuffer, int iBufferSize);

    CogeScript* GetCurrentScript();
    CScriptBuilder* GetScriptBuilder();
    CDebugger* GetScriptDebugger();

    void AddBreakPoint(const std::string& sFileName, int iLineNumber);

    int HandleDebugRequest(int iSize, char* pInBuf, char* pOutBuf);

    void RegisterCheckDebugRequestFunction(void* pOnCheckDebugRequest);

    int RegisterFunction(void* f, const char* fname);

    int RegisterTypedef(const std::string& newtype, const std::string& newdef);

    int RegisterEnum(const std::string& enumtype);
    int RegisterEnumValue(const std::string& enumtype, const std::string& name, int value);

    int BeginConfigGroup(const std::string& groupname);
    int EndConfigGroup();
    int RemoveConfigGroup(const std::string& groupname);

    int GC(int iFlags = 0);

    friend class CogeScript;

};


class CogeScript
{
private:

    CogeScripter*     m_pScripter;

    asIScriptModule*  m_pScriptModule; // code container

    asIScriptContext* m_pContext;      // code executer

    CogeScriptBinaryStream* m_pBinStream;

    std::string m_sName;

    int m_iState;

    int m_iResult;

    int m_iDebugMode;    // 0: no debug, 1: remote debug

    int m_iTotalUsers;


protected:

public:

    CogeScript(const std::string& sName, CogeScripter* pScripter);
    ~CogeScript();

    int GetState();
    const std::string& GetName();

    void Hire();
    void Fire();

    int SetDebugMode(int value);
    int GetDebugMode();

    int Pause();
    int Resume();
    int Execute();
    int Stop();
    int Reset();

    int GetRuntimeState();
    bool IsSuspended();
    bool IsFinished();

    int PrepareFunction(const std::string& sFunctionName);
    int PrepareFunction(int iFunctionID);
    bool IsFunctionReady(int iFunctionID);
    int CallFunction(int iFunctionID);

    int CallFunction(const std::string& sFunctionName);

    int LoadBinaryCode(char* pInputBuf, int iBufferSize);
    int SaveBinaryCode(char* pOutputBuf, int iBufferSize);

    int GetFunctionCount();
    std::string GetFunctionNameByIndex(int idx);
    int GetVarCount();
    std::string GetVarNameByIndex(int idx);

    int CallGC(int iFlags = 0);

    friend class CogeScripter;

};


#endif // __OGE_SCRIPT_H_INCLUDED__
