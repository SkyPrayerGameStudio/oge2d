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

#include "ogeScript.h"

#include "ogeCommon.h"

#include "scriptarray.h"
#include "scriptstdstring.h"
//#include "scriptstring.h"
#include "scriptmath.h"
#include "scriptfile.h"
#include "scriptdictionary.h"

#include "SDL.h"

#include <iostream>
#include <sstream>

#define _OGE_DEBUG_NOP_           0
#define _OGE_DEBUG_ADD_BP_        1
#define _OGE_DEBUG_DEL_BP_        2
#define _OGE_DEBUG_NEXT_INST_     3
#define _OGE_DEBUG_NEXT_LINE_     4
#define _OGE_DEBUG_CONTINUE_      5
#define _OGE_DEBUG_STOP_          6

static int g_debugcmd = 0; // 0: no action
static asIScriptContext* g_suspendedctx = NULL;
static CogeScripter* g_scripter = NULL;
static CogeScript* g_currentscript = NULL;
static CogeBreakpoint g_breakpoints[_OGE_MAX_BP_COUNT_];

void OGE_ScriptMessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";

	OGE_Log("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}


void OGE_ScriptLineCallback(asIScriptContext* ctx, void* obj)
{
    if(ctx == NULL || g_scripter == NULL) return;

    int iDebugMode = g_scripter->GetDebugMode();
    if(iDebugMode <= 0) return;

    if(iDebugMode == Debug_Local)
    {
        g_scripter->GetScriptDebugger()->LineCallback(ctx);
    }
    else if(iDebugMode == Debug_Remote)
    {
        if(g_debugcmd == _OGE_DEBUG_NEXT_INST_ || g_debugcmd == _OGE_DEBUG_NEXT_LINE_)
        {
            g_debugcmd = _OGE_DEBUG_NOP_;
            g_suspendedctx = ctx;
            ctx->Suspend();
            return;
        }

        asIScriptEngine* engine = ctx->GetEngine();
        if(!engine) return;
        //int funcid = ctx->GetCurrentFunction();
        //asIScriptFunction* func = engine->GetFunctionDescriptorById(funcid);
        asIScriptFunction* func = ctx->GetFunction();
        if(!func) return;
        const char* scriptFileName = func->GetScriptSectionName();
        //int lineNumber = ctx->GetCurrentLineNumber();
        int lineNumber = ctx->GetLineNumber();

        for(int i=0; i<_OGE_MAX_BP_COUNT_; i++)
        {
            if(g_breakpoints[i].used == false) continue;
            if(g_breakpoints[i].line == lineNumber)
            {
                if(g_breakpoints[i].filename.compare(scriptFileName) == 0)
                {
                    g_debugcmd = _OGE_DEBUG_NOP_;
                    g_suspendedctx = ctx;
                    ctx->Suspend();
                    return;
                }
            }

        }
    }



}

/*------------------ CogeScriptBinaryStream ------------------*/
CogeScriptBinaryStream::CogeScriptBinaryStream()
{
    Clear();
}

CogeScriptBinaryStream::~CogeScriptBinaryStream()
{
    Clear();
}

void CogeScriptBinaryStream::SetReadBuffer(char* pReadBuffer, int iReadBufSize)
{
    m_pReadBuffer = pReadBuffer;
    m_iReadBufSize = iReadBufSize;
    m_iReadPos = 0;
}

void CogeScriptBinaryStream::SetWriteBuffer(char* pWriteBuffer, int iWriteBufSize)
{
    m_pWriteBuffer = pWriteBuffer;
    m_iWriteBufSize = iWriteBufSize;
    m_iWritePos = 0;
}

void CogeScriptBinaryStream::Clear()
{
    m_pReadBuffer = NULL;
    m_pWriteBuffer = NULL;

    m_iReadBufSize = 0;
    m_iWriteBufSize = 0;

    m_iReadPos = 0;
    m_iWritePos = 0;
}

int CogeScriptBinaryStream::GetReadSize()
{
    return m_iReadPos;
}
int CogeScriptBinaryStream::GetWriteSize()
{
    return m_iWritePos;
}

void CogeScriptBinaryStream::Read(void *ptr, asUINT size)
{
    if(m_pReadBuffer == NULL || m_iReadBufSize <= 0) return;
    char* pBuf = m_pReadBuffer + m_iReadPos;
    memcpy(ptr, pBuf, size);
    m_iReadPos += size;
}

void CogeScriptBinaryStream::Write(const void *ptr, asUINT size)
{
    if(m_pWriteBuffer == NULL || m_iWriteBufSize <= 0) return;
    char* pBuf = m_pWriteBuffer + m_iWritePos;
    memcpy(pBuf, ptr, size);
    m_iWritePos += size;

}

/*------------------ CogeScriptDebugger ------------------*/

void CogeScriptDebugger::Output(const std::string &str)
{
    std::string sText = str;
    if(sText.length() == 0) return;
    if(sText[sText.length() - 1] != '\n') sText = sText + "\n";
    sText = "[OGE_DBG]" + sText;
    CDebugger::Output(sText);
    std::cout.flush();
}

std::string CogeScriptDebugger::ToString(void *value, asUINT typeId, bool expandMembers, bool isString, asIScriptEngine *engine)
{
    if(!isString) return CDebugger::ToString(value, typeId, expandMembers, engine);
    else
    {
        if( typeId & asTYPEID_OBJHANDLE )
        {
            value = *(void**)value;
            return CDebugger::ToString(value, typeId, expandMembers, engine);
        }
        else
        {
            std::string sValue = *(std::string*)value;
            return sValue;
        }
    }
}

void CogeScriptDebugger::ListLocalVariables(asIScriptContext *ctx)
{
    asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	std::stringstream s;
	for( asUINT n = 0; n < func->GetVarCount(); n++ )
	{
		if( ctx->IsVarInScope(n) )
        {
            bool bIsStrType = false;
            std::string sDefine = func->GetVarDecl(n);
            if(sDefine.find("string ") != std::string::npos) bIsStrType = true;
            else if(sDefine.find("string&") != std::string::npos) bIsStrType = true;

            if(bIsStrType)
                s << sDefine << " = " << ToString(ctx->GetAddressOfVar(n), ctx->GetVarTypeId(n), false, bIsStrType, ctx->GetEngine()) << std::endl;
            else
                s << sDefine << " = " << CDebugger::ToString(ctx->GetAddressOfVar(n), ctx->GetVarTypeId(n), false, ctx->GetEngine()) << std::endl;

            //s << func->GetVarDecl(n) << " = " << ToString(ctx->GetAddressOfVar(n), ctx->GetVarTypeId(n), false, ctx->GetEngine()) << endl;
        }

	}
	Output(s.str());
}
void CogeScriptDebugger::ListGlobalVariables(asIScriptContext *ctx)
{
    // Determine the current module from the function
	asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	asIScriptModule *mod = ctx->GetEngine()->GetModule(func->GetModuleName(), asGM_ONLY_IF_EXISTS);
	if( !mod ) return;

	std::stringstream s;
	for( asUINT n = 0; n < mod->GetGlobalVarCount(); n++ )
	{
	    int typeId;
	    const char *varName = 0, *nameSpace = 0;
		mod->GetGlobalVar(n, &varName, &nameSpace, &typeId);

		bool bIsStrType = false;
        std::string sDefine = mod->GetGlobalVarDeclaration(n);
        if(sDefine.find("string ") != std::string::npos) bIsStrType = true;

        if(bIsStrType)
            s << sDefine << " = " << ToString(mod->GetAddressOfGlobalVar(n), typeId, false, bIsStrType, ctx->GetEngine()) << std::endl;
        else
            s << sDefine << " = " << CDebugger::ToString(mod->GetAddressOfGlobalVar(n), typeId, false, ctx->GetEngine()) << std::endl;

		//s << mod->GetGlobalVarDeclaration(n) << " = " << ToString(mod->GetAddressOfGlobalVar(n), typeId, false, ctx->GetEngine()) << endl;
	}
	Output(s.str());
}

/*------------------ CogeScripter ------------------*/

CogeScripter::CogeScripter()
:m_pScriptEngine(NULL)
,m_pScriptBuilder(NULL)
,m_pScriptDebugger(NULL)
{
    m_iState = -1;

    m_iNextId = 0;

    m_iDebugMode = 0; // no debug ...

    g_currentscript = NULL;

    m_iBinaryMode = 0; // default text code input

    m_pOnCheckDebugRequest = NULL;

    for(int i=0; i<_OGE_MAX_BP_COUNT_; i++) g_breakpoints[i].used = false;

    m_pScriptEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if(m_pScriptEngine)
    {
        m_pScriptEngine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0); // the script file is not strict with unicode input ...
        m_pScriptEngine->SetMessageCallback(asFUNCTION(OGE_ScriptMessageCallback), 0, asCALL_CDECL);

        RegisterScriptArray(m_pScriptEngine, true);
        //RegisterScriptString(m_pScriptEngine);
        RegisterStdString(m_pScriptEngine);
        RegisterStdStringUtils(m_pScriptEngine);
        RegisterScriptMath(m_pScriptEngine);
        RegisterScriptFile(m_pScriptEngine);
        RegisterScriptDictionary(m_pScriptEngine);

        //RegisterScriptByteArray(m_pScriptEngine);

        m_pScriptBuilder = new CScriptBuilder();
        //m_pScriptDebugger = new CDebugger();
        m_pScriptDebugger = new CogeScriptDebugger();

        m_iState = 0;
    }

    if(m_iState >= 0) g_scripter = this;
    else g_scripter = NULL;
}

CogeScripter::~CogeScripter()
{
    g_scripter = NULL;

    m_iState = -1;

    m_iDebugMode = 0; // no debug ...

    g_currentscript = NULL;

    m_iBinaryMode = 0;

    DelAllScripts();

    if(m_pScriptBuilder) delete m_pScriptBuilder;
    if(m_pScriptDebugger) delete m_pScriptDebugger;

    if(m_pScriptEngine) m_pScriptEngine->Release();

}

int CogeScripter::SetDebugMode(int value)
{
    if(m_iState < 0) return m_iState;

    if(value < 0) m_iDebugMode = 0;
    else m_iDebugMode = value;

    if(m_iDebugMode > 0)
    {
        ogeScriptMap::iterator it;
        CogeScript* pMatchedScript = NULL;

        it = m_ScriptMap.begin();

        while (it != m_ScriptMap.end())
        {
            pMatchedScript = it->second;
            pMatchedScript->SetDebugMode(value);
            it++;
        }


        ogeScriptList::iterator itx;
        itx = m_ScriptList.begin();

        while (itx != m_ScriptList.end())
        {
            pMatchedScript = *itx;
            pMatchedScript->SetDebugMode(value);
            itx++;
        }


    }

    return m_iDebugMode;

}
int CogeScripter::GetDebugMode()
{
    return m_iDebugMode;
}

int CogeScripter::SetBinaryMode(int value)
{
    if(m_iBinaryMode != value)
    {
        m_iBinaryMode = value;
    }
    return m_iBinaryMode;
}
int CogeScripter::GetBinaryMode()
{
    return m_iBinaryMode;
}

bool CogeScripter::IsSuspended()
{
    return g_suspendedctx != NULL;
}

CogeScript* CogeScripter::NewSingleScript(const std::string& sScriptName, const std::string& sFileName)
{
    if(!m_pScriptEngine) return NULL;

    //CogeScript* pTheNewScript = NULL;

    /*
	ogeScriptMap::iterator it;

    it = m_ScriptMap.find(sScriptName);
	if (it != m_ScriptMap.end())
	{
        OGE_Log("The script name '%s' is in use.\n", sScriptName.c_str());
        return NULL;
	}
	*/

    int r = -1;
    r = m_pScriptBuilder->StartNewModule(m_pScriptEngine, sScriptName.c_str());
    if( r < 0 )
	{
		OGE_Log("Failed to create a new script module: %s\n", sScriptName.c_str());
		return NULL;
	}

	r = m_pScriptBuilder->AddSectionFromFile(sFileName.c_str());
	if( r < 0 )
	{
		OGE_Log("Failed to load script from file: %s\n", sFileName.c_str());
		m_pScriptEngine->DiscardModule(sScriptName.c_str());
		return NULL;
	}

	r = m_pScriptBuilder->BuildModule();
	if( r < 0 )
	{
		OGE_Log("Failed to build script module: %s\n", sScriptName.c_str());
		m_pScriptEngine->DiscardModule(sScriptName.c_str());
		return NULL;
	}

	// if load script successfully then prepare the Script class ...

	CogeScript* pTheNewScript = new CogeScript(sScriptName, this);

	if (pTheNewScript->m_iState < 0)
	{
	    m_pScriptEngine->DiscardModule(sScriptName.c_str());
	    delete pTheNewScript;
	    return NULL;
    }

    //m_ScriptMap.insert(ogeScriptMap::value_type(sScriptName, pTheNewScript));

    return pTheNewScript;
}
CogeScript* CogeScripter::NewSingleScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize)
{
    if(!m_pScriptEngine) return NULL;

    bool bIsBinary = m_iBinaryMode > 0;

    //CogeScript* pTheNewScript = NULL;

    /*
	ogeScriptMap::iterator it;

    it = m_ScriptMap.find(sScriptName);
	if (it != m_ScriptMap.end())
	{
        OGE_Log("The script name '%s' is in use.\n", sScriptName.c_str());
        return NULL;
	}
	*/

    if(bIsBinary)
    {
        asIScriptModule* pModule = m_pScriptEngine->GetModule(sScriptName.c_str(), asGM_ALWAYS_CREATE);
        if(pModule == NULL)
        {
            OGE_Log("Failed to create an empty new script module: %s\n", sScriptName.c_str());
            return NULL;
        }
    }
    else
    {
        int r = m_pScriptBuilder->StartNewModule(m_pScriptEngine, sScriptName.c_str());
        if( r < 0 )
        {
            OGE_Log("Failed to create a new script module: %s\n", sScriptName.c_str());
            return NULL;
        }
    }


	CogeScript* pTheNewScript = new CogeScript(sScriptName, this);

	if (pTheNewScript->m_iState < 0)
	{
	    m_pScriptEngine->DiscardModule(sScriptName.c_str());
	    delete pTheNewScript;
	    return NULL;
    }

    if(bIsBinary)
    {
        int iRealReadSize = pTheNewScript->LoadBinaryCode(pInputBuffer, iBufferSize);
        if(iRealReadSize < 0)
        {
            m_pScriptEngine->DiscardModule(sScriptName.c_str());
            delete pTheNewScript;
            return NULL;
        }
    }
    else
    {
        // ensure the input buffer is like a valid string ...
        pInputBuffer[iBufferSize] = '\0';

        // the char buffer must be full text code without any #include ...
        //std::string sSectionName = sScriptName + "_FullTextSection";
        std::string sSectionName = sScriptName + "_FTS";
        int r = m_pScriptBuilder->AddSectionFromMemory(pInputBuffer, sSectionName.c_str());
        if( r < 0 )
        {
            OGE_Log("Failed to load script from buffer: %s\n", sSectionName.c_str());
            m_pScriptEngine->DiscardModule(sScriptName.c_str());
            delete pTheNewScript;
            return NULL;
        }

        r = m_pScriptBuilder->BuildModule();
        if( r < 0 )
        {
            OGE_Log("Failed to build script module: %s\n", sScriptName.c_str());
            m_pScriptEngine->DiscardModule(sScriptName.c_str());
            delete pTheNewScript;
            return NULL;
        }
    }

    //m_ScriptMap.insert(ogeScriptMap::value_type(sScriptName, pTheNewScript));

    return pTheNewScript;
}

CogeScript* CogeScripter::NewFixedScript(const std::string& sFileName)
{
    //std::string sScriptName = "_" + OGE_itoa(m_ScriptList.size());
    std::string sScriptName = "_" + OGE_itoa(GetNextScriptId());

    CogeScript* pTheNewScript = NewSingleScript(sScriptName, sFileName);
    if(pTheNewScript == NULL) return NULL;

    m_ScriptList.push_back(pTheNewScript);

    return pTheNewScript;
}
CogeScript* CogeScripter::NewFixedScriptFromBuffer(char* pInputBuffer, int iBufferSize)
{
    //std::string sScriptName = "_" + OGE_itoa(m_ScriptList.size());
    std::string sScriptName = "_" + OGE_itoa(GetNextScriptId());

    CogeScript* pTheNewScript = NewSingleScriptFromBuffer(sScriptName, pInputBuffer, iBufferSize);
    if(pTheNewScript == NULL) return NULL;

    m_ScriptList.push_back(pTheNewScript);

    return pTheNewScript;
}

CogeScript* CogeScripter::NewScript(const std::string& sScriptName, const std::string& sFileName)
{
	ogeScriptMap::iterator it;

    it = m_ScriptMap.find(sScriptName);
	if (it != m_ScriptMap.end())
	{
        OGE_Log("The script name '%s' is in use.\n", sScriptName.c_str());
        return NULL;
	}

	CogeScript* pTheNewScript = NewSingleScript(sScriptName, sFileName);
    if(pTheNewScript == NULL) return NULL;

    m_ScriptMap.insert(ogeScriptMap::value_type(sScriptName, pTheNewScript));

    return pTheNewScript;

}

CogeScript* CogeScripter::NewScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize)
{
	ogeScriptMap::iterator it;

    it = m_ScriptMap.find(sScriptName);
	if (it != m_ScriptMap.end())
	{
        OGE_Log("The script name '%s' is in use.\n", sScriptName.c_str());
        return NULL;
	}

	CogeScript* pTheNewScript = NewSingleScriptFromBuffer(sScriptName, pInputBuffer, iBufferSize);
    if(pTheNewScript == NULL) return NULL;

    m_ScriptMap.insert(ogeScriptMap::value_type(sScriptName, pTheNewScript));

    return pTheNewScript;

}

int CogeScripter::SaveBinaryCode(const std::string& sScriptName, char* pOutputBuffer, int iBufferSize)
{
    CogeScript* pScript = FindScript(sScriptName);
    if(pScript && pScript->GetState() >= 0) return pScript->SaveBinaryCode(pOutputBuffer, iBufferSize);
    else return 0;
}

CogeScript* CogeScripter::FindScript(const std::string& sScriptName)
{
    ogeScriptMap::iterator it;

    it = m_ScriptMap.find(sScriptName);
	if (it != m_ScriptMap.end()) return it->second;
	else return NULL;

}

CogeScript* CogeScripter::GetScript(const std::string& sScriptName, const std::string& sFileName)
{
    CogeScript* pMatchedScript = FindScript(sScriptName);
    if (!pMatchedScript) pMatchedScript = NewScript(sScriptName, sFileName);
    //if (pMatchedScript) pMatchedScript->Hire();
    return pMatchedScript;

}

CogeScript* CogeScripter::GetScriptFromBuffer(const std::string& sScriptName, char* pInputBuffer, int iBufferSize)
{
    CogeScript* pMatchedScript = FindScript(sScriptName);
    if (!pMatchedScript) pMatchedScript = NewScriptFromBuffer(sScriptName, pInputBuffer, iBufferSize);
    return pMatchedScript;
}

CogeScript* CogeScripter::GetCurrentScript()
{
    return g_currentscript;
}

CScriptBuilder* CogeScripter::GetScriptBuilder()
{
    return m_pScriptBuilder;
}
CDebugger* CogeScripter::GetScriptDebugger()
{
    return m_pScriptDebugger;
}

void CogeScripter::AddBreakPoint(const std::string& sFileName, int iLineNumber)
{
    if(m_pScriptDebugger) m_pScriptDebugger->AddFileBreakPoint(sFileName, iLineNumber);
}

int CogeScripter::HandleDebugRequest(int iSize, char* pInBuf, char* pOutBuf)
{
    if(m_iDebugMode != Debug_Remote) return 0;

    int iCmd = OGE_BufGetInt(pInBuf, 0);
    switch(iCmd)
    {
    case _OGE_DEBUG_NOP_:
    {
        g_debugcmd = _OGE_DEBUG_NOP_;
        break;
    }
    case _OGE_DEBUG_ADD_BP_:
    {
        g_debugcmd = _OGE_DEBUG_NOP_;
        int iFreeIdx = -1;
        for(int i=0; i<_OGE_MAX_BP_COUNT_; i++)
        {
            if(g_breakpoints[i].used == false)
            {
                iFreeIdx = i;
                break;
            }
        }
        if(iFreeIdx >= 0)
        {
            g_breakpoints[iFreeIdx].line = OGE_BufGetInt(pInBuf, 4);
            int iFileNameSize = OGE_BufGetInt(pInBuf, 8);
            g_breakpoints[iFreeIdx].filename = OGE_BufGetStr(pInBuf, 12, iFileNameSize);
            g_breakpoints[iFreeIdx].used = true;
        }
        break;
    }
    case _OGE_DEBUG_DEL_BP_:
    {
        g_debugcmd = _OGE_DEBUG_NOP_;
        int iDelIdx = OGE_BufGetInt(pInBuf, 4);
        if(iDelIdx >= 0 && iDelIdx < _OGE_MAX_BP_COUNT_)
        {
            g_breakpoints[iDelIdx].used = false;
        }
        break;
    }
    case _OGE_DEBUG_NEXT_INST_:
    {
        g_debugcmd = _OGE_DEBUG_NEXT_INST_;
        break;
    }
    case _OGE_DEBUG_NEXT_LINE_:
    {
        g_debugcmd = _OGE_DEBUG_NEXT_LINE_;
        break;
    }
    case _OGE_DEBUG_CONTINUE_:
    {
        g_debugcmd = _OGE_DEBUG_CONTINUE_;
        break;
    }
    case _OGE_DEBUG_STOP_: break;
    }
    return 0;
}

int CogeScripter::DelScript(const std::string& sScriptName)
{
    CogeScript* pMatchedScript = FindScript(sScriptName);
	if (pMatchedScript)
	{
	    //pMatchedScript->Fire();
	    if (pMatchedScript->m_iTotalUsers > 0) return 0;
		m_ScriptMap.erase(sScriptName);
		delete pMatchedScript;
		return 1;

	}
	return -1;

}

int CogeScripter::DelScript(CogeScript* pScript)
{
    if(!pScript) return 0;

    bool bFound = false;

    if(!bFound)
    {
        ogeScriptMap::iterator it;
        CogeScript* pMatchedScript = NULL;

        it = m_ScriptMap.begin();

        while (it != m_ScriptMap.end())
        {
            pMatchedScript = it->second;
            if(pMatchedScript == pScript)
            {
                bFound = true;
                break;
            }
            it++;
        }

        if(bFound)
        {
            m_ScriptMap.erase(it);
            if(pMatchedScript) delete pMatchedScript;
        }

    }

    if(!bFound)
    {

        ogeScriptList::iterator itx;
        CogeScript* pMatchedScript = NULL;

        itx = m_ScriptList.begin();

        while (itx != m_ScriptList.end())
        {
            pMatchedScript = *itx;
            if(pMatchedScript == pScript)
            {
                bFound = true;
                break;
            }
            itx++;
        }

        if(bFound)
        {
            m_ScriptList.erase(itx);
            if(pMatchedScript) delete pMatchedScript;
        }
    }

    if(bFound) return 1;
    else return 0;
}

void CogeScripter::DelAllScripts()
{
    // free all Scripts
	ogeScriptMap::iterator it;
	CogeScript* pMatchedScript = NULL;

	it = m_ScriptMap.begin();

	while (it != m_ScriptMap.end())
	{
		pMatchedScript = it->second;
		m_ScriptMap.erase(it);
		delete pMatchedScript;
		it = m_ScriptMap.begin();
	}


	ogeScriptList::iterator itx;
	pMatchedScript = NULL;

	itx = m_ScriptList.begin();

	while (itx != m_ScriptList.end())
	{
		pMatchedScript = *itx;
		m_ScriptList.erase(itx);
		delete pMatchedScript;
		itx = m_ScriptList.begin();
	}

}

int CogeScripter::GetState()
{
    return m_iState;
}

int CogeScripter::GetNextScriptId()
{
    m_iNextId++;
    return m_iNextId;
}

void CogeScripter::RegisterCheckDebugRequestFunction(void* pOnCheckDebugRequest)
{
    m_pOnCheckDebugRequest = pOnCheckDebugRequest;
}

int CogeScripter::RegisterFunction(void* f, const char* fname)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->RegisterGlobalFunction(fname, asFUNCTION(f), asCALL_CDECL);

}

int CogeScripter::RegisterTypedef(const std::string& newtype, const std::string& newdef)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->RegisterTypedef(newtype.c_str(), newdef.c_str());
}

int CogeScripter::RegisterEnum(const std::string& enumtype)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->RegisterEnum(enumtype.c_str());
}
int CogeScripter::RegisterEnumValue(const std::string& enumtype, const std::string& name, int value)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->RegisterEnumValue(enumtype.c_str(), name.c_str(), value);
}

int CogeScripter::BeginConfigGroup(const std::string& groupname)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->BeginConfigGroup(groupname.c_str());
}
int CogeScripter::EndConfigGroup()
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->EndConfigGroup();
}
int CogeScripter::RemoveConfigGroup(const std::string& groupname)
{
    if(!m_pScriptEngine) return -1;
    return m_pScriptEngine->RemoveConfigGroup(groupname.c_str());
}

int CogeScripter::GC(int iFlags)
{
    if(!m_pScriptEngine) return -1;
    if(iFlags == 0) iFlags = asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE;
    return m_pScriptEngine->GarbageCollect(iFlags);
}

/*------------------ CogeScript ------------------*/

CogeScript::CogeScript(const std::string& sName, CogeScripter* pScripter)
{
    m_iResult = 0;

    m_iDebugMode = 0; // no debug ...

    m_iTotalUsers = 0;

    m_iState = -1;

    m_pBinStream = new CogeScriptBinaryStream();

    m_pScripter = pScripter;

    if(!m_pScripter) return;

    if(m_pScripter->m_pScriptEngine == NULL) return;

    m_pScriptModule = pScripter->m_pScriptEngine->GetModule(sName.c_str());

    if(!m_pScriptModule) return;

    m_pContext = NULL;

    m_sName = sName;

    m_pContext = m_pScripter->m_pScriptEngine->CreateContext();

    if(!m_pContext) return;

    if(m_pScripter->m_iDebugMode > 0)
    m_pContext->SetLineCallback(asFUNCTION(OGE_ScriptLineCallback), &m_iResult, asCALL_CDECL);

    m_iState = 0;

}

CogeScript::~CogeScript()
{
    m_iState = -1;

    if(m_pBinStream)
    {
        delete m_pBinStream;
        m_pBinStream = NULL;
    }

    if(m_pContext)
    {
        switch(m_pContext->GetState())
        {
        case asEXECUTION_SUSPENDED:
        case asEXECUTION_ACTIVE:
        m_pContext->Abort();
        break;

        case asEXECUTION_PREPARED:
        case asEXECUTION_ERROR:
        m_pContext->Unprepare();
        break;

        case asEXECUTION_FINISHED:
        case asEXECUTION_ABORTED:
        case asEXECUTION_EXCEPTION:
        case asEXECUTION_UNINITIALIZED:
        break;

        }

        m_pContext->Release();
    }

    if(m_pScriptModule)
    {
        if(m_pScripter && m_pScripter->m_pScriptEngine)
        {
            m_pScripter->m_pScriptEngine->DiscardModule(m_sName.c_str());
            m_pScriptModule = NULL;
        }
    }
}

int CogeScript::GetState()
{
    return m_iState;
}

const std::string& CogeScript::GetName()
{
    return m_sName;
}

int CogeScript::GetFunctionCount()
{
    if(m_iState < 0 || m_pScriptModule == NULL) return 0;
    return m_pScriptModule->GetFunctionCount();

}
std::string CogeScript::GetFunctionNameByIndex(int idx)
{
    if(m_iState < 0 || m_pScriptModule == NULL) return "";
    //asIScriptFunction* pDescr = m_pScriptModule->GetFunctionDescriptorByIndex(idx);
    asIScriptFunction* pFunc = m_pScriptModule->GetFunctionByIndex(idx);
    if(pFunc == NULL) return "";
    return pFunc->GetName();

}
int CogeScript::GetVarCount()
{
    if(m_iState < 0 || m_pScriptModule == NULL) return 0;
    return m_pScriptModule->GetGlobalVarCount();
}
std::string CogeScript::GetVarNameByIndex(int idx)
{
    if(m_iState < 0 || m_pScriptModule == NULL) return "";
    //return m_pScriptModule->GetGlobalVarName(idx);
    return m_pScriptModule->GetGlobalVarDeclaration(idx);
}

int CogeScript::LoadBinaryCode(char* pInputBuf, int iBufferSize)
{
    int iLoadedSize = -1;
    if(pInputBuf && iBufferSize > 0 && m_iState >= 0)
    {
        if(m_pScriptModule)
        {
            m_pBinStream->SetReadBuffer(pInputBuf, iBufferSize);
            m_pScriptModule->LoadByteCode(m_pBinStream);
            iLoadedSize = m_pBinStream->GetReadSize();
            m_pBinStream->Clear();
        }
    }
    return iLoadedSize;
}

int CogeScript::SaveBinaryCode(char* pOutputBuf, int iBufferSize)
{
    int iSavedSize = -1;
    if(pOutputBuf && iBufferSize > 0 && m_iState >= 0)
    {
        if(m_pScriptModule)
        {
            m_pBinStream->SetWriteBuffer(pOutputBuf, iBufferSize);
            m_pScriptModule->SaveByteCode(m_pBinStream);
            iSavedSize = m_pBinStream->GetWriteSize();
            m_pBinStream->Clear();
        }
    }
    return iSavedSize;
}

int CogeScript::SetDebugMode(int value)
{
    if(value < 0) m_iDebugMode = 0;
    else m_iDebugMode = value;

    if(m_iDebugMode > 0)
    {
        m_pContext->SetLineCallback(asFUNCTION(OGE_ScriptLineCallback), &m_iResult, asCALL_CDECL);
    }

    return m_iDebugMode;

}
int CogeScript::GetDebugMode()
{
    if(m_pScripter) return m_pScripter->m_iDebugMode;
    else return this->m_iDebugMode;
}

int CogeScript::GetRuntimeState()
{
    //if(!m_pContext) return -1;
    if(m_iState < 0) return -1;
    return m_pContext->GetState();

}

bool CogeScript::IsSuspended()
{
    if(m_iState < 0) return false;
    return m_pContext->GetState() == asEXECUTION_SUSPENDED;
}

bool CogeScript::IsFinished()
{
    if(m_iState < 0) return true;
    return m_pContext->GetState() == asEXECUTION_FINISHED;
}

void CogeScript::Fire()
{
    m_iTotalUsers--;
	if (m_iTotalUsers < 0) m_iTotalUsers = 0;
}

void CogeScript::Hire()
{
    if (m_iTotalUsers < 0) m_iTotalUsers = 0;
	m_iTotalUsers++;
}

int CogeScript::Pause()
{
    if(m_iState < 0) return -1;
    return m_pContext->Suspend();
}

int CogeScript::Resume()
{
    if(m_iState < 0) return -1;
    if(m_pContext->GetState() != asEXECUTION_SUSPENDED) return -1;
    g_currentscript = this;
    return m_pContext->Execute();
}

int CogeScript::Execute()
{
    if(m_iState < 0) return -1;
    g_currentscript = this;
    return m_pContext->Execute();
}

int CogeScript::Stop()
{
    //if(!m_pContext) return -1;
    if(m_iState < 0) return -1;

    int iRsl = -1;

    switch(m_pContext->GetState())
    {
    case asEXECUTION_SUSPENDED:
    case asEXECUTION_ACTIVE:
    m_pContext->Abort();
    iRsl = 2;
    break;

    case asEXECUTION_PREPARED:
    case asEXECUTION_ERROR:
    m_pContext->Unprepare();
    iRsl = 1;
    break;

    case asEXECUTION_FINISHED:
    case asEXECUTION_ABORTED:
    case asEXECUTION_EXCEPTION:
    case asEXECUTION_UNINITIALIZED:
    iRsl = 0;
    break;

    }

    return iRsl;
}

int CogeScript::Reset()
{
    if(m_iState < 0) return -1;

    int iRsl = -1;

    switch(m_pContext->GetState())
    {
    case asEXECUTION_SUSPENDED:
    case asEXECUTION_ACTIVE:
    m_pContext->Abort();
    iRsl = 2;
    break;

    case asEXECUTION_PREPARED:
    case asEXECUTION_ERROR:
    m_pContext->Unprepare();
    iRsl = 1;
    break;

    case asEXECUTION_FINISHED:
    case asEXECUTION_ABORTED:
    case asEXECUTION_EXCEPTION:
    case asEXECUTION_UNINITIALIZED:
    iRsl = 0;
    break;

    }

    return iRsl;
}

int CogeScript::PrepareFunction(const std::string& sFunctionName)
{
    if(m_iState < 0) return -1;
    //return m_pScripter->m_pScriptEngine->GetFunctionIDByName(m_sName.c_str(), sFunctionName.c_str());
    return m_pScriptModule->GetFunctionIdByName(sFunctionName.c_str());
}

int CogeScript::PrepareFunction(int iFunctionID)
{
    if(m_iState < 0) return -1;

    switch(m_pContext->GetState())
    {
    case asEXECUTION_SUSPENDED:
    case asEXECUTION_ACTIVE:
    return -1;
    break;

    case asEXECUTION_PREPARED:
    case asEXECUTION_ERROR:
    m_pContext->Unprepare();
    break;

    case asEXECUTION_FINISHED:
    case asEXECUTION_ABORTED:
    case asEXECUTION_EXCEPTION:
    case asEXECUTION_UNINITIALIZED:
    break;

    }

    return m_pContext->Prepare(iFunctionID);

}
bool CogeScript::IsFunctionReady(int iFunctionID)
{
    if(m_iState < 0) return false;

    //return m_pContext->GetCurrentFunction() == iFunctionID;

    if( m_pContext->GetState() == asEXECUTION_SUSPENDED || m_pContext->GetState() == asEXECUTION_ACTIVE )
    {
        return m_pContext->GetFunction() &&  m_pContext->GetFunction()->GetId() == iFunctionID;
    }

    return false;
}

int CogeScript::CallFunction(int iFunctionID)
{
    if(m_iState < 0) return -1;
    switch(m_pContext->GetState())
    {
    case asEXECUTION_SUSPENDED:
    case asEXECUTION_ACTIVE:
    return -1;
    break;

    case asEXECUTION_PREPARED:
    case asEXECUTION_ERROR:
    m_pContext->Unprepare();
    break;

    case asEXECUTION_FINISHED:
    case asEXECUTION_ABORTED:
    case asEXECUTION_EXCEPTION:
    case asEXECUTION_UNINITIALIZED:
    break;

    }

    int rsl = 0;

    if (m_pContext->Prepare(iFunctionID) >= 0)
    {
        int iCmdResult = 0;
        g_currentscript = this;
        rsl = m_pContext->Execute();

        if(m_iDebugMode == Debug_Remote)
        {
            while(rsl == asEXECUTION_SUSPENDED)
            {
                while(g_debugcmd != _OGE_DEBUG_NEXT_INST_
                        && g_debugcmd != _OGE_DEBUG_NEXT_LINE_
                        && g_debugcmd != _OGE_DEBUG_CONTINUE_)
                {
                    SDL_Delay(10);  // cool down cpu time ...

                    //iScanResult = OGE_ScanDebugRequest(g_currentscript);
                    //if(iScanResult < 0) break;

                    if(m_pScripter && m_pScripter->m_pOnCheckDebugRequest)
                    {
                        ogeCommonFunction callback = (ogeCommonFunction)m_pScripter->m_pOnCheckDebugRequest;
                        iCmdResult = callback();
                        if(iCmdResult < 0) break;
                    }

                }
                if(iCmdResult < 0)
                {
                    g_debugcmd = _OGE_DEBUG_STOP_;
                    g_suspendedctx = NULL;
                    this->Stop();
                    break;
                }
                SDL_Delay(10);  // cool down cpu time ...
                if(g_debugcmd == _OGE_DEBUG_CONTINUE_) g_suspendedctx = NULL;
                g_currentscript = this;
                rsl = m_pContext->Execute();
            }
        }

    }

    return rsl;

}

int CogeScript::CallFunction(const std::string& sFunctionName)
{
    int id = PrepareFunction(sFunctionName);
    if (id >= 0) return CallFunction(id);
    return -1;

}

int CogeScript::CallGC(int iFlags)
{
    if(m_pScripter == NULL) return -1;
    return m_pScripter->GC(iFlags);

    //return 0;
}










