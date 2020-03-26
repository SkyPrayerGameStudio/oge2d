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

#ifndef __OGE_NET_H_INCLUDED__
#define __OGE_NET_H_INCLUDED__

#include "SDL.h"
#include "SDL_net.h"

#include <string>
#include <vector>
#include <map>

#define _OGE_SOCK_MAX_BUF_SIZE_ 4096

#define _OGE_SOCK_HB_INTERVAL_  5000

#define _OGE_SOCK_MSG_HB_    0
#define _OGE_SOCK_MSG_BIN_   1
#define _OGE_SOCK_MSG_TEXT_  2

struct CogeNetMsgHeader
{
    uint32_t flag;
    uint8_t  type;
    uint8_t  value;
    uint16_t size;
};

class CogeNetwork;

class CogeSocket
{
private:

    uint32_t gGlobalFlag;

    TCPsocket    m_iSocket;
    std::string  m_sName;
    std::string  m_sIp;
    std::string  m_sAddr;
    int          m_iPort;
    int          m_iState;
    bool         m_bServer;

    int          m_iRecvState;

    int          m_iLeft;
    int          m_iCurrentPos;
    int          m_iCurrentSize;
    int          m_iMessageType;
    int          m_iMessageValue;
    char         m_InBuf[_OGE_SOCK_MAX_BUF_SIZE_];
    char         m_OutBuf[_OGE_SOCK_MAX_BUF_SIZE_];

    bool         m_bAutoFeedbackHB;
    int          m_iLastCheckTick;

    CogeNetMsgHeader m_CurrentHeader;

    void FreeSocket();


protected:

public:

    CogeSocket();
    ~CogeSocket();

    int GetState();
    const std::string& GetIp();
    const std::string& GetName();
    int GetPort();
    const std::string& GetAddr();

    int GetMessageType();
    int GetMessageValue();

    int GetSocket();

    char* GetInBuf();
    char* GetOutBuf();

    int GetRecvDataSize();

    bool IsServer();

    int PutByte(int idx, char value);
    int PutInt(int idx, int value);
    int PutFloat(int idx, double value);
    int PutBuf(int idx, char* value, int len);
    int PutStr(int idx, const std::string& value);

    int SendData(const char* buf, int len, int type = _OGE_SOCK_MSG_BIN_, int value = 0);
    int SendData(int len, int type = _OGE_SOCK_MSG_BIN_, int value = 0);

    int SendHeartbeat();
    int FeedbackHeartbeat();

    int RecvData(char** buf);
    bool RecvData(int* iDataSize);



    friend class CogeNetwork;

};

class CogeNetwork
{
private:

    typedef std::vector<CogeSocket*> ogeSocketList;
    typedef std::map<std::string, CogeSocket*> ogeSocketMap;

    uint32_t gGlobalFlag;

    CogeSocket*  m_DebugSocket;

    CogeSocket*  m_LocalServer;
    ogeSocketMap m_RemoteClients;
    ogeSocketMap m_RemoteServers;

    ogeSocketList m_RemoteClientList;
    ogeSocketList m_RemoteServerList;

    ogeSocketMap m_ComeinClientQueue;
    ogeSocketMap m_GetoutClientQueue;
    ogeSocketMap m_ComeinServerQueue;
    ogeSocketMap m_GetoutServerQueue;

    std::string m_sLocalName;
    std::string m_sLocalIp;
    //Uint32 m_iLocalIp;

    //std::string m_sMsg;

    int m_iHeartbeatTime;
    int m_iCheckInterval;
    int m_iTimeoutTime;
    int m_iLastTickHB;
    int m_iLastTickCheck;

    int m_iState;

    void* m_pOnAcceptClient;
    void* m_pOnReceiveFromRemoteClient;
    void* m_pOnRemoteClientDisconnect;
    void* m_pOnRemoteServerDisconnect;
    void* m_pOnReceiveFromRemoteServer;

    void* m_pOnRemoteDebugService;

    // as all network actions will be executed step by step in one single thread,
    // so we could use global buffer to pass the data ...
    char* m_pCurrentData;
    int m_iCurrentDataSize;

    int m_iCurrentMessageType;
    int m_iCurrentMessageValue;

    CogeSocket* m_pCurrentSocket;



protected:


public:

    CogeNetwork();
    ~CogeNetwork();

    int Initialize();
    void Finalize();

    int GetState();

    const std::string& GetLocalIp();

    // functions for server

    int OpenLocalServer(int iPort);

    CogeSocket* FindClient(const std::string& sAddr);

    CogeSocket* AcceptClient(TCPsocket iSocket);

    int AddClient(const std::string& sAddr);

    int KickClient(const std::string& sAddr);

    int RemoveClient(CogeSocket* pClient);
    int RemoveClient(const std::string& sAddr);
    int RemoveAllClients();

    int CloseLocalServer();

    // for debug ...
    int ConnectDebugServer(const std::string& sAddr);
    int DisconnectDebugServer();
    int SendDebugData(const char* buf, int len, int type = _OGE_SOCK_MSG_BIN_);

    CogeSocket* GetDebugSocket();

    bool IsRemoteDebugMode();


    // functions for client

    CogeSocket* ConnectServer(const std::string& sAddr);

    int AddServer(const std::string& sAddr);

    int DisconnectServer(const std::string& sAddr);

    int RemoveServer(const std::string& sAddr);
    int RemoveServer(CogeSocket* pServer);
    int RemoveAllServers();

    CogeSocket* FindServer(const std::string& sAddr);

    CogeSocket* GetLocalServer();

    int RefreshClientList();
    int RefreshServerList();

    CogeSocket* GetClientByIndex(size_t idx);
    CogeSocket* GetServerByIndex(size_t idx);

    int ScanDebugRequest();


    // common functions

    int SendData(const char* buf, int len, int type = _OGE_SOCK_MSG_BIN_);
    int Broadcast(const char* buf, int len, int type = _OGE_SOCK_MSG_BIN_);

    CogeSocket* GetSocket();

    int GetMsgType();
    int GetMsgValue();
    int GetRecvDataSize();
    char* GetRecvData();

    //std::string GetRecvMsg();
    //int SendMsg(const std::string& sMsg);

    void RegisterEvents(void* pOnAcceptClient,
                        void* pOnReceiveFromRemoteClient,
                        void* pOnRemoteClientDisconnect,
                        void* pOnRemoteServerDisconnect,
                        void* pOnReceiveFromRemoteServer,
                        void* pOnRemoteDebugService);


    int Update();  // Update Socket Events



};

#endif // __OGE_NET_H_INCLUDED__
