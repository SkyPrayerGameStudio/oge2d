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

#include "ogeNet.h"
#include "ogeCommon.h"

//#include "SDLnetsys.h"
// Copy from "SDLnetsys.h" --- Begin ---

/* Include system network headers */
#if defined(__WIN32__) || defined(WIN32) || defined(WINCE)
#define __USE_W32_SOCKETS
#ifdef _WIN64
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
/* NOTE: windows socklen_t is signed
 * and is defined only for winsock2. */
typedef int socklen_t;
#endif /* W64 */
#include <iphlpapi.h>
#else /* UNIX */
#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#endif
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#endif
#ifdef linux /* FIXME: what other platforms have this? */
#include <netinet/tcp.h>
#endif
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#endif /* WIN32 */

// Copy from "SDLnetsys.h" --- End ---


#define MAKE_NUM(A, B, C, D)	(((A+B)<<8)|(C+D))
#define TEMP_PORT	MAKE_NUM('T','E','M','P')

#define _OGE_SOCK_RECV_NONE_     0
#define _OGE_SOCK_RECV_HEADER_   1
#define _OGE_SOCK_RECV_BODY_     2

static const uint32_t gGlobalFlagHigh =  0x7AE38AD6;
static const uint32_t gGlobalFlagLow  =  0x27FD4DB3;

static CogeNetwork* g_network = NULL;


/***************** CogeSocket ********************/

CogeSocket::CogeSocket()
{
    //gGlobalFlag = (gGlobalFlagHigh << 32) | gGlobalFlagLow;

    gGlobalFlag = gGlobalFlagHigh;

    m_sAddr = "";
    m_sIp   = "";
    m_sName = "";

    m_iLeft = 0;
    m_iCurrentPos = 0;
    m_iCurrentSize = 0;

    m_iRecvState = 0;

    m_iSocket = 0;
    m_iState  = -1;

    //memset((void*)&m_CurrentHeader, 0, sizeof(m_CurrentHeader));

    //memset((void*)&m_InBuf[0], 0, sizeof(char)* _OGE_MAX_SOCK_BUF_SIZE_ );
    //memset((void*)&m_OutBuf[0], 0, sizeof(char)* _OGE_MAX_SOCK_BUF_SIZE_ );

    m_bServer = false;

    m_bAutoFeedbackHB = true; //...
    m_iLastCheckTick = 0;
}

CogeSocket::~CogeSocket()
{
    FreeSocket();
    m_iState = -1;
}

void CogeSocket::FreeSocket()
{
    if(m_iSocket)
    {
        SDLNet_TCP_Close(m_iSocket);
        m_iSocket = 0;
    }
}

int CogeSocket::GetState()
{
    return m_iState;
}

const std::string& CogeSocket::GetIp()
{
    return m_sIp;
}
const std::string& CogeSocket::GetName()
{
    return m_sName;
}
int CogeSocket::GetPort()
{
    return m_iPort;
}

const std::string& CogeSocket::GetAddr()
{
    return m_sAddr;
}

char* CogeSocket::GetInBuf()
{
    //return &(m_Buf[0]);
    return m_InBuf;
}

char* CogeSocket::GetOutBuf()
{
    //return &(m_Buf[0]);
    return m_OutBuf;
}

int CogeSocket::GetRecvDataSize()
{
    return m_iCurrentSize;
}

int CogeSocket::GetMessageType()
{
    return m_iMessageType;
}

int CogeSocket::GetMessageValue()
{
    return m_iMessageValue;
}

int CogeSocket::GetSocket()
{
    return (int)m_iSocket;
}

bool CogeSocket::IsServer()
{
    return m_bServer;
}

int CogeSocket::PutByte(int idx, char value)
{
    char* dst = m_OutBuf;
    dst[idx] = value;
    return idx+1;
}

int CogeSocket::PutInt(int idx, int value)
{
    char* dst = m_OutBuf;
    int len = sizeof(int);
    memcpy((void*)(dst+idx), (void*)(&value), len);
    return idx+len;
}

int CogeSocket::PutFloat(int idx, double value)
{
    char* dst = m_OutBuf;
    int len = sizeof(double);
    memcpy((void*)(dst+idx), (void*)(&value), len);
    return idx+len;
}

int CogeSocket::PutBuf(int idx, char* value, int len)
{
    char* dst = m_OutBuf;
    memcpy((void*)(dst+idx), (void*)value, len);
    return idx+len;
}

int CogeSocket::PutStr(int idx, const std::string& value)
{
    char* dst = m_OutBuf;
    int len = value.length();
    memcpy((void*)(dst+idx), (void*)(value.c_str()), len);
    return idx+len;
}

int CogeSocket::SendData(const char* buf, int len, int type, int value)
{
    int result = -1;

    if(!m_iSocket) return -1;

    if(len > 0 && buf == NULL) return -1;

    CogeNetMsgHeader header;

    header.flag = gGlobalFlag;
    header.size = len;
    header.type = type;
    header.value = value;

    int iHeaderSize = sizeof(header);

    // change endianness to network order
    //iHeaderSize=SDL_SwapBE32(iHeaderSize);

    // send header
    result = SDLNet_TCP_Send(m_iSocket, &header, iHeaderSize);
    if(result < iHeaderSize)
    {
        if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
            OGE_Log("SDLNet_TCP_Send(header) error: %s\n", SDLNet_GetError());
        return -1;
    }

    //printf("SDLNet_TCP_Send: Send Header OK\n");

    // change endianness to network order
    //len=SDL_SwapBE32(len);

    // send the buffer, with the NULL as well
    if(len > 0)
    {
        result = SDLNet_TCP_Send(m_iSocket, buf, len);
        if(result < len)
        {
            if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
                OGE_Log("SDLNet_TCP_Send(body) error: %s\n", SDLNet_GetError());
            return -1;
        }
    }
    else return 0;

    //printf("SDLNet_TCP_Send: Send Body OK\n");

    // return the body size we had sent
    return result;
}

int CogeSocket::SendData(int len, int type, int value)
{
    return SendData(m_OutBuf, len, type, value);
}

int CogeSocket::SendHeartbeat()
{
    return SendData(NULL, 0, _OGE_SOCK_MSG_HB_, 0);
}

int CogeSocket::FeedbackHeartbeat()
{
    return SendData(NULL, 0, _OGE_SOCK_MSG_HB_, 1);
}

int CogeSocket::RecvData(char** buf)
{
    int len = 0;
    int result = -1;

    if(!m_iSocket) return -1;

    *buf=NULL;

    CogeNetMsgHeader header;

    int iHeaderSize = sizeof(header);

    // receive the length of the message
    result = SDLNet_TCP_Recv(m_iSocket, &header, iHeaderSize);
    if(result < iHeaderSize)
    {
        if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
            OGE_Log("SDLNet_TCP_Recv(header) error: %s\n", SDLNet_GetError());
        return -1;
    }

    if(header.flag != gGlobalFlag)
    {
        OGE_Log("CogeSocket::RecvData(buf): missing message flag\n");
        return -1;
    }

    len = header.size;

    // it could be only a header (without body) ...
    if(len == 0) return 0;

    // allocate the buffer memory
    *buf = (char*)malloc(len);
    if(!(*buf)) return -1;

    // get the string buffer over the socket
    result = SDLNet_TCP_Recv(m_iSocket, *buf, len);
    if(result < len)
    {
        if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
            OGE_Log("SDLNet_TCP_Recv(body): %s\n", SDLNet_GetError());
        free(*buf);
        buf=NULL;
        return -1;
    }

    // return the new buffer
    return len;
}

bool CogeSocket::RecvData(int* iDataSize)
{
    if(!m_iSocket) return false;

    if(m_iLeft > 0)
    {
        if(m_iCurrentPos+m_iLeft > _OGE_SOCK_MAX_BUF_SIZE_)
        {
            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iCurrentSize = 0;
            *iDataSize = m_iCurrentSize;
            m_iRecvState = _OGE_SOCK_RECV_NONE_;
            return false;
        }
        char* buf = &(m_InBuf[m_iCurrentPos]);
        int iActualSize = SDLNet_TCP_Recv(m_iSocket, buf, m_iLeft);
        if(iActualSize < m_iLeft)
        {
            if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
                OGE_Log("SDLNet_TCP_Recv(when get left): %s\n", SDLNet_GetError());
            m_iCurrentPos = m_iCurrentPos + iActualSize;
            m_iLeft = m_iLeft - iActualSize;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;
            return false;
        }
        else
        {
            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;

            if (m_iRecvState == _OGE_SOCK_RECV_BODY_)
            {
                m_iRecvState = _OGE_SOCK_RECV_NONE_;
                return true;
            }
            else if (m_iRecvState == _OGE_SOCK_RECV_HEADER_)
            {
                memcpy(&m_CurrentHeader, (void*)&(m_InBuf[0]), sizeof(m_CurrentHeader));
                m_iRecvState = _OGE_SOCK_RECV_BODY_;
            }
            else
            {
                return false;
            }

        }
    }

    if (m_iRecvState == _OGE_SOCK_RECV_HEADER_)
    {
        m_iLeft = 0;
        m_iCurrentPos = 0;
        m_iCurrentSize = 0;
        *iDataSize = m_iCurrentSize;
        m_iRecvState = _OGE_SOCK_RECV_NONE_;
        return false;
    }

    if (m_iRecvState == _OGE_SOCK_RECV_NONE_)
    {
        m_iLeft = 0;
        m_iCurrentPos = 0;

        int iTotalSize = sizeof(m_CurrentHeader);

        char* buf = &(m_InBuf[m_iCurrentPos]);

        // get header
        int iActualSize = SDLNet_TCP_Recv(m_iSocket, buf, iTotalSize);
        if(iActualSize < iTotalSize)
        {
            if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
                OGE_Log("SDLNet_TCP_Recv(when get header): %s\n", SDLNet_GetError());
            m_iCurrentPos = m_iCurrentPos + iActualSize;
            m_iLeft = iTotalSize - iActualSize;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;
            m_iRecvState = _OGE_SOCK_RECV_HEADER_;
            return false;
        }
        else
        {
            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;

            memcpy(&m_CurrentHeader, (void*)&(m_InBuf[0]), sizeof(m_CurrentHeader));
            m_iRecvState = _OGE_SOCK_RECV_BODY_;
        }
    }

    if(m_iRecvState == _OGE_SOCK_RECV_BODY_)
    {
        if(m_CurrentHeader.flag != gGlobalFlag)
        {
            OGE_Log("CogeSocket::RecvData(size): missing flag in message header\n");
            m_iCurrentSize = 0;
            *iDataSize = m_iCurrentSize;

            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iRecvState = _OGE_SOCK_RECV_NONE_;

            return false;
        }

        int iTotalSize = m_CurrentHeader.size;

        // check if anything is strange...
        if(iTotalSize < 0 || iTotalSize > _OGE_SOCK_MAX_BUF_SIZE_)
        {
            m_iCurrentSize = 0;
            *iDataSize = m_iCurrentSize;

            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iRecvState = _OGE_SOCK_RECV_NONE_;

            return false;
        }

        m_iMessageType = m_CurrentHeader.type;
        m_iMessageValue = m_CurrentHeader.value;

        m_iLeft = 0;
        m_iCurrentPos = 0;

        if(iTotalSize == 0)
        {
            m_iCurrentSize = 0;
            *iDataSize = m_iCurrentSize;

            if(m_iMessageType == _OGE_SOCK_MSG_HB_ && m_iMessageValue == 0
                && m_bAutoFeedbackHB) FeedbackHeartbeat();

            if(m_iMessageType == _OGE_SOCK_MSG_HB_ && m_iMessageValue == 1)
                m_iLastCheckTick = SDL_GetTicks();

            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iRecvState = _OGE_SOCK_RECV_NONE_;

            return true;
        }

        char* buf = &(m_InBuf[m_iCurrentPos]);

        // get the string buffer over the socket
        int iActualSize = SDLNet_TCP_Recv(m_iSocket, buf, iTotalSize);
        if(iActualSize < iTotalSize)
        {
            if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
                OGE_Log("SDLNet_TCP_Recv(when get body): %s\n", SDLNet_GetError());
            m_iCurrentPos = m_iCurrentPos + iActualSize;
            m_iLeft = iTotalSize - iActualSize;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;
            return false;
        }
        else
        {
            m_iLeft = 0;
            m_iCurrentPos = 0;
            m_iCurrentSize = iActualSize;
            *iDataSize = m_iCurrentSize;
            m_iRecvState = _OGE_SOCK_RECV_NONE_;
            return true;
        }
    }

    return false;
}


/******************* CogeNetwork *********************/


CogeNetwork::CogeNetwork()
{
    //gGlobalFlag = (gGlobalFlagHigh << 32) | gGlobalFlagLow;
    gGlobalFlag = gGlobalFlagHigh;

    m_iState = -1;

    m_DebugSocket = NULL;

    m_sLocalName = "localhost";
    m_sLocalIp   = "127.0.0.1";

    //m_sMsg = "";

    m_LocalServer = NULL;

    m_pOnAcceptClient = NULL;
    m_pOnReceiveFromRemoteClient = NULL;
    m_pOnRemoteClientDisconnect = NULL;
    m_pOnRemoteServerDisconnect = NULL;
    m_pOnReceiveFromRemoteServer = NULL;
    m_pOnRemoteDebugService = NULL;

    m_pCurrentData = NULL;

    m_iCurrentDataSize = 0;

    m_pCurrentSocket = NULL;

    m_iHeartbeatTime = _OGE_SOCK_HB_INTERVAL_;
    m_iCheckInterval = m_iHeartbeatTime * 5;
    m_iTimeoutTime = m_iHeartbeatTime * 3 + 2000;

    m_iLastTickHB = SDL_GetTicks();
    m_iLastTickCheck = SDL_GetTicks();

    g_network = this;

    //printf("SDLNet TCP_NODELAY: %d\n", TCP_NODELAY);

}
CogeNetwork::~CogeNetwork()
{
    Finalize();
    if(m_iState >= 0) SDLNet_Quit();
    m_iState = -1;
}

int CogeNetwork::GetState()
{
    return m_iState;
}

const std::string& CogeNetwork::GetLocalIp()
{
    if(m_sLocalIp.length() <= 0 ||  m_sLocalIp == "127.0.0.1")
    {
        IPaddress localip = {0};

        char sHostName[256] = {0};

        Uint32 ipaddr = 0;

        // Get local host name
        if (gethostname(sHostName, sizeof(sHostName)) == 0)
        {
            OGE_Log("Local Host Name: %s\n", sHostName);

            // also SDLNet_GetLocalAddresses() could be used here ...
            if(SDLNet_ResolveHost(&localip,sHostName,TEMP_PORT)==0)
            {
                //ipaddr=SDL_SwapBE32(localip.host);
                m_sLocalName = sHostName;

                ipaddr=SDL_SwapBE32(localip.host);

                char c[32] = {0};

                sprintf(c, "%d.%d.%d.%d",
                ipaddr>>24,
                (ipaddr>>16)&0xff,
                (ipaddr>>8)&0xff,
                ipaddr&0xff);

                OGE_Log("Local Host Ip: %s\n", c);

                m_sLocalIp = c;

            }

        }
    }

    return m_sLocalIp;
}

int CogeNetwork::Initialize()
{
    if(m_iState < 0) {
        if(SDLNet_Init()== -1) OGE_Log("SDLNet_Init Error: %s\n",SDLNet_GetError());
        else m_iState = 0;
    }
    return m_iState;
}

void CogeNetwork::Finalize()
{
    RemoveAllServers();
    RemoveAllClients();
    CloseLocalServer();
    DisconnectDebugServer();
    return;
}

CogeSocket* CogeNetwork::GetLocalServer()
{
    return m_LocalServer;
}

int CogeNetwork::RefreshClientList()
{
    m_RemoteClientList.clear();
    ogeSocketMap::iterator its, itb, ite;
    itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
    for ( its=itb; its!=ite; its++ ) m_RemoteClientList.push_back(its->second);
    return m_RemoteClientList.size();

}
int CogeNetwork::RefreshServerList()
{
    m_RemoteServerList.clear();
    ogeSocketMap::iterator its, itb, ite;
    itb = m_RemoteServers.begin(); ite = m_RemoteServers.end();
    for ( its=itb; its!=ite; its++ ) m_RemoteServerList.push_back(its->second);
    return m_RemoteServerList.size();
}

CogeSocket* CogeNetwork::GetClientByIndex(size_t idx)
{
    if (idx >= 0 && idx < m_RemoteClientList.size()) return m_RemoteClientList[idx];
    else return NULL;
}
CogeSocket* CogeNetwork::GetServerByIndex(size_t idx)
{
    if (idx >= 0 && idx < m_RemoteServerList.size()) return m_RemoteServerList[idx];
    else return NULL;
}

int CogeNetwork::OpenLocalServer(int iPort)
{
    IPaddress ip     = {0};
    TCPsocket server = 0;

    if(m_LocalServer &&
       m_LocalServer->m_iState >= 0 &&
       m_LocalServer->m_iPort == iPort)
    return 0;

    if(m_LocalServer && m_LocalServer->m_iPort == iPort) CloseLocalServer();

    // Resolve the argument into an IPaddress type
    if(SDLNet_ResolveHost(&ip,NULL,iPort)==-1) return -1;
    server = SDLNet_TCP_Open(&ip);
    if(!server) return -2;

    if(m_LocalServer) CloseLocalServer();

    m_LocalServer = new CogeSocket();

    m_LocalServer->m_bServer = true;

    m_LocalServer->m_sIp = GetLocalIp();
    m_LocalServer->m_sName = m_sLocalName;
    m_LocalServer->m_iPort = iPort;

    char c[8] = {0};
    sprintf(c,"%d",iPort);
    std::string sPort(c);

    m_LocalServer->m_sAddr = m_LocalServer->m_sIp + ":" + sPort;
    m_LocalServer->m_iSocket = server;
    m_LocalServer->m_iState = 0;

    OGE_Log("Listening Port: %d\n", iPort);

    return 0;
}

CogeSocket* CogeNetwork::AcceptClient(TCPsocket iSocket)
{
    if(iSocket == NULL) return NULL;

    IPaddress* ip = SDLNet_TCP_GetPeerAddress(iSocket);

    if(ip == NULL) return NULL;

    Uint32 ipaddr;
    Uint16 iPort;

    iPort = ip->port;

    char c[8] = {0};
    sprintf(c,"%d",iPort);

    std::string sPort(c);


    ipaddr = SDL_SwapBE32(ip->host);

    char cc[32] = {0};
    sprintf(cc,"%d.%d.%d.%d",
            ipaddr>>24,
            (ipaddr>>16)&0xff,
            (ipaddr>>8)&0xff,
            ipaddr&0xff);

    std::string sIp(cc);


    std::string sAddr = sIp + ":" + sPort;


    m_GetoutClientQueue.erase(sAddr);


    ogeSocketMap::iterator it;

    it = m_RemoteClients.find(sAddr);
	if (it != m_RemoteClients.end()) return 0;

	it = m_ComeinClientQueue.find(sAddr);
	if (it != m_ComeinClientQueue.end()) return 0;


	CogeSocket* pClient = new CogeSocket();

    pClient->m_bServer = false;

    pClient->m_sIp = sIp;
    pClient->m_sName = "";
    pClient->m_iPort = iPort;

    pClient->m_sAddr = sAddr;
    pClient->m_iSocket = iSocket;
    pClient->m_iState = 0;

    m_ComeinClientQueue.insert(ogeSocketMap::value_type(sAddr, pClient));

    return pClient;

}

int CogeNetwork::AddClient(const std::string& sAddr)
{
    CogeSocket* pClient = NULL;

    ogeSocketMap::iterator it;

	it = m_ComeinClientQueue.find(sAddr);
	if (it == m_ComeinClientQueue.end()) return 0;
	else
	{
        pClient = it->second;
        m_ComeinClientQueue.erase(it);
	}

	if(pClient == NULL) return 0;

	it = m_RemoteClients.find(sAddr);
	if (it != m_RemoteClients.end()) return 0;

	pClient->m_iLastCheckTick = SDL_GetTicks();

	m_RemoteClients.insert(ogeSocketMap::value_type(sAddr, pClient));

	return 1;
}

int CogeNetwork::KickClient(const std::string& sAddr)
{
    CogeSocket* pClient = NULL;

    ogeSocketMap::iterator it;

    it = m_ComeinClientQueue.find(sAddr);
	if (it != m_ComeinClientQueue.end()) pClient = it->second;

	if (pClient)
	{
		m_ComeinClientQueue.erase(it);

		delete pClient;

		//return 0;
	}

	pClient = NULL;

	it = m_RemoteClients.find(sAddr);
	if (it == m_RemoteClients.end()) return 0;
	else pClient = it->second;

	if(pClient == NULL) return 0;

	it = m_GetoutClientQueue.find(sAddr);
	if (it != m_GetoutClientQueue.end()) return 0;

	m_GetoutClientQueue.insert(ogeSocketMap::value_type(sAddr, pClient));

	return 1;

}

int CogeNetwork::RemoveClient(CogeSocket* pClient)
{
    if(pClient == NULL) return -1;

    return RemoveClient(pClient->m_sAddr);

}

int CogeNetwork::RemoveClient(const std::string& sAddr)
{
    m_GetoutClientQueue.erase(sAddr);

    CogeSocket* pMatchedClient = NULL;

    ogeSocketMap::iterator it;

	//if(pClient == NULL) return 0;

    it = m_RemoteClients.find(sAddr);
	if (it != m_RemoteClients.end()) pMatchedClient = it->second;
	else return -1;

	if (pMatchedClient)
	{
		m_RemoteClients.erase(it);

		delete pMatchedClient;

		return 0;
	}
	else return -1;
}

int CogeNetwork::RemoveAllClients()
{
    m_RemoteClientList.clear();

    ogeSocketMap::iterator it;
	CogeSocket* pMatchedClient = NULL;

	it = m_ComeinClientQueue.begin();

	while (it != m_ComeinClientQueue.end())
	{
		pMatchedClient = it->second;
		m_ComeinClientQueue.erase(it);
		delete pMatchedClient;
		it = m_ComeinClientQueue.begin();
	}

	it = m_GetoutClientQueue.begin();

	while (it != m_GetoutClientQueue.end())
	{
		pMatchedClient = it->second;
		m_GetoutClientQueue.erase(it);
		delete pMatchedClient;
		it = m_GetoutClientQueue.begin();
	}

	it = m_RemoteClients.begin();

	while (it != m_RemoteClients.end())
	{
		pMatchedClient = it->second;
		m_RemoteClients.erase(it);
		delete pMatchedClient;
		it = m_RemoteClients.begin();
	}

	return m_ComeinClientQueue.size() + m_GetoutClientQueue.size() + m_RemoteClients.size();
}

CogeSocket* CogeNetwork::FindClient(const std::string& sAddr)
{
    ogeSocketMap::iterator it;

    it = m_RemoteClients.find(sAddr);
	if (it != m_RemoteClients.end()) return it->second;
	else return NULL;
}

int CogeNetwork::CloseLocalServer()
{
    //if(m_LocalServer != NULL && m_LocalServer.m_iState >= 0)
    //return 0;

    if(m_LocalServer == NULL ||
       m_LocalServer->m_iState < 0 ||
       m_LocalServer->m_iSocket == 0)
    return 0;

    RemoveAllClients();

    //SDLNet_TCP_Close(m_LocalServer->m_iSocket);

    delete m_LocalServer;

    m_LocalServer = NULL;

    return 0;

}

// for debug ...
int CogeNetwork::ConnectDebugServer(const std::string& sAddr)
{
    size_t iPos = sAddr.find_first_of(':');

    if(iPos == std::string::npos) return -3;

    std::string sIp   = sAddr.substr(0, iPos);
    std::string sPort = sAddr.substr(iPos+1);

    int iPort = atoi(sPort.c_str());

    IPaddress ip     = {0};
    TCPsocket client = 0;

    if(SDLNet_ResolveHost(&ip,sIp.c_str(),iPort)==-1) return -1;

    if(m_DebugSocket)
    {
        if(m_DebugSocket->GetState() >= 0
            && m_DebugSocket->GetPort() == iPort
            && m_DebugSocket->GetIp().compare(sIp) == 0) {
            return 0;
        }

        delete m_DebugSocket;
        m_DebugSocket = NULL;
    }

    // open the server socket
    client = SDLNet_TCP_Open(&ip);
    if(!client) return -2;

    CogeSocket* pRemote = new CogeSocket();

    pRemote->m_bServer = false;

    pRemote->m_sIp = sIp;
    pRemote->m_sName = "";
    pRemote->m_iPort = iPort;

    pRemote->m_sAddr = sAddr;
    pRemote->m_iSocket = client;
    pRemote->m_iState = 0;

    m_DebugSocket = pRemote;

    OGE_Log("Connected remote debug server %s successfully.\n", sAddr.c_str());

    return 0;

}

int CogeNetwork::DisconnectDebugServer()
{
    if(m_DebugSocket)
    {
        delete m_DebugSocket;
        m_DebugSocket = NULL;
        return 1;
    }
    return 0;
}

CogeSocket* CogeNetwork::GetDebugSocket()
{
    return m_DebugSocket;
}

bool CogeNetwork::IsRemoteDebugMode()
{
    return m_DebugSocket != NULL;
}

int CogeNetwork::SendDebugData(const char* buf, int len, int type)
{
    if(m_DebugSocket && m_DebugSocket->m_iSocket && len > 0)
    return m_DebugSocket->SendData(buf, len, type);
    else return -1;
}

// functions for client

CogeSocket* CogeNetwork::ConnectServer(const std::string& sAddr)
{
    //if(FindServer(sAddr) != NULL) return 0;

    m_GetoutServerQueue.erase(sAddr);

    ogeSocketMap::iterator it;

    it = m_RemoteServers.find(sAddr);
	if (it != m_RemoteServers.end()) return it->second;

	it = m_ComeinServerQueue.find(sAddr);
	if (it != m_ComeinServerQueue.end()) return it->second;

    size_t iPos = sAddr.find_first_of(':');

    if(iPos == std::string::npos) return NULL;

    std::string sIp   = sAddr.substr(0, iPos);
    std::string sPort = sAddr.substr(iPos+1);

    int iPort = atoi(sPort.c_str());

    IPaddress ip     = {0};
    TCPsocket client = 0;

    if(SDLNet_ResolveHost(&ip,sIp.c_str(),iPort)==-1) return NULL;

    // open the server socket
    client = SDLNet_TCP_Open(&ip);
    if(!client) return NULL;

    CogeSocket* pRemote = new CogeSocket();

    pRemote->m_bServer = false;

    pRemote->m_sIp = sIp;
    pRemote->m_sName = "";
    pRemote->m_iPort = iPort;

    pRemote->m_sAddr = sAddr;
    pRemote->m_iSocket = client;
    pRemote->m_iState = 0;

    //m_RemoteServers.insert(ogeSocketMap::value_type(sAddr, pRemote));

    m_ComeinServerQueue.insert(ogeSocketMap::value_type(sAddr, pRemote));

    return pRemote;

}

int CogeNetwork::AddServer(const std::string& sAddr)
{
    CogeSocket* pRemote = NULL;

    ogeSocketMap::iterator it;

	it = m_ComeinServerQueue.find(sAddr);
	if (it == m_ComeinServerQueue.end()) return 0;
	else
	{
        pRemote = it->second;
        m_ComeinServerQueue.erase(it);
	}

	if(pRemote == NULL) return 0;

	it = m_RemoteServers.find(sAddr);
	if (it != m_RemoteServers.end()) return 0;

	pRemote->m_iLastCheckTick = SDL_GetTicks();

	m_RemoteServers.insert(ogeSocketMap::value_type(sAddr, pRemote));

	return 1;
}

int CogeNetwork::DisconnectServer(const std::string& sAddr)
{
    CogeSocket* pRemote = NULL;

    ogeSocketMap::iterator it;

    it = m_ComeinServerQueue.find(sAddr);
	if (it != m_ComeinServerQueue.end()) pRemote = it->second;

	if (pRemote)
	{
		m_ComeinServerQueue.erase(it);

		delete pRemote;
	}

	pRemote = NULL;

	it = m_RemoteServers.find(sAddr);
	if (it == m_RemoteServers.end()) return 0;
	else pRemote = it->second;

	if(pRemote == NULL) return 0;

	it = m_GetoutServerQueue.find(sAddr);
	if (it != m_GetoutServerQueue.end()) return 0;

	m_GetoutServerQueue.insert(ogeSocketMap::value_type(sAddr, pRemote));

	return 1;
}

int CogeNetwork::RemoveServer(const std::string& sAddr)
{
    m_GetoutServerQueue.erase(sAddr);

    CogeSocket* pMatchedServer = NULL;

    ogeSocketMap::iterator it;

    it = m_RemoteServers.find(sAddr);
	if (it != m_RemoteServers.end()) pMatchedServer = it->second;
	else return -1;

	if (pMatchedServer)
	{
		m_RemoteServers.erase(it);

		delete pMatchedServer;

		return 0;
	}
	else return -1;

    //return 0;
}

int CogeNetwork::RemoveServer(CogeSocket* pServer)
{
    if(pServer == NULL) return -1;

    return RemoveServer(pServer->m_sAddr);
}

int CogeNetwork::RemoveAllServers()
{
    m_RemoteServerList.clear();

    ogeSocketMap::iterator it;
	CogeSocket* pMatchedServer = NULL;

	it = m_ComeinServerQueue.begin();

	while (it != m_ComeinServerQueue.end())
	{
		pMatchedServer = it->second;
		m_ComeinServerQueue.erase(it);
		delete pMatchedServer;
		it = m_ComeinServerQueue.begin();
	}

	it = m_GetoutServerQueue.begin();

	while (it != m_GetoutServerQueue.end())
	{
		pMatchedServer = it->second;
		m_GetoutServerQueue.erase(it);
		delete pMatchedServer;
		it = m_GetoutServerQueue.begin();
	}

	it = m_RemoteServers.begin();

	while (it != m_RemoteServers.end())
	{
		pMatchedServer = it->second;
		m_RemoteServers.erase(it);
		delete pMatchedServer;
		it = m_RemoteServers.begin();
	}

	//return m_RemoteServers.size();
	return m_ComeinServerQueue.size() + m_GetoutServerQueue.size() + m_RemoteServers.size();
}

CogeSocket* CogeNetwork::FindServer(const std::string& sAddr)
{
    ogeSocketMap::iterator it;

    it = m_RemoteServers.find(sAddr);
	if (it != m_RemoteServers.end()) return it->second;
	else return NULL;

}


// common functions

int CogeNetwork::SendData(const char* buf, int len, int type)
{
    if(m_pCurrentSocket && m_pCurrentSocket->m_iSocket && len > 0)
    return m_pCurrentSocket->SendData(buf, len, type);
    else return -1;
}
int CogeNetwork::Broadcast(const char* buf, int len, int type)
{
    ogeSocketMap::iterator its, itb, ite;
    itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
    int i = 0;
    for ( its=itb; its!=ite; its++ )
    {
        if(its->second->m_iSocket)
        {
            if(len > 0 && its->second->SendData(buf, len, type) == len) i++;
        }
    }
    return i;
}
char* CogeNetwork::GetRecvData()
{
    return m_pCurrentData;
}

/*
std::string CogeNetwork::GetRecvMsg()
{
    if(m_pCurrentData)
    {
        m_sMsg = m_pCurrentData;
    }
    else m_sMsg = "";

    return m_sMsg;
}

int CogeNetwork::SendMsg(const std::string& sMsg)
{
    return SendData(sMsg.c_str(), sMsg.length(), 1);
}
*/

int CogeNetwork::GetRecvDataSize()
{
    return m_iCurrentDataSize;
}

int CogeNetwork::GetMsgType()
{
    return m_iCurrentMessageType;
}

CogeSocket* CogeNetwork::GetSocket()
{
    return m_pCurrentSocket;
}

void CogeNetwork::RegisterEvents(void* pOnAcceptClient,
                                void* pOnReceiveFromRemoteClient,
                                void* pOnRemoteClientDisconnect,
                                void* pOnRemoteServerDisconnect,
                                void* pOnReceiveFromRemoteServer,
                                void* pOnRemoteDebugService)
{
    m_pOnAcceptClient = pOnAcceptClient;
    m_pOnReceiveFromRemoteClient = pOnReceiveFromRemoteClient;
    m_pOnRemoteClientDisconnect = pOnRemoteClientDisconnect;
    m_pOnRemoteServerDisconnect = pOnRemoteServerDisconnect;
    m_pOnReceiveFromRemoteServer = pOnReceiveFromRemoteServer;
    m_pOnRemoteDebugService = pOnRemoteDebugService;
}

int CogeNetwork::Update()
{
    if(!IsRemoteDebugMode())
    {
        int iCurrentTick = SDL_GetTicks();
        if(iCurrentTick - m_iLastTickHB >= m_iHeartbeatTime)
        {
            // send heartbeat ...

            //printf("\n Send heartbeat: %d \n", iCurrentTick);

            ogeSocketMap::iterator its, itb, ite;

            itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
            for ( its=itb; its!=ite; its++ ) its->second->SendHeartbeat();

            itb = m_RemoteServers.begin(); ite = m_RemoteServers.end();
            for ( its=itb; its!=ite; its++ ) its->second->SendHeartbeat();

            m_iLastTickHB = SDL_GetTicks();
        }

    }

    ScanDebugRequest();

    // handle client's connection ...

    if(m_LocalServer && m_LocalServer->m_iSocket)
    {
        SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
        if(set)
        {
            SDLNet_TCP_AddSocket(set,m_LocalServer->m_iSocket);

            int numready = SDLNet_CheckSockets(set, 0);

            if(numready > 0)
            {
                if(SDLNet_SocketReady(m_LocalServer->m_iSocket))
                {
                    CogeSocket* pRemote = NULL;
                    TCPsocket sock = SDLNet_TCP_Accept(m_LocalServer->m_iSocket);
                    if(sock) pRemote = AcceptClient(sock);

                    if(pRemote) m_pCurrentSocket = pRemote;

                    //if(pScript && m_iEventOnAcceptClient>=0)
                    //pScript->CallFunction(m_iEventOnAcceptClient);

                    if(m_pOnAcceptClient)
                    {
                        ogeCommonFunction callback = (ogeCommonFunction)m_pOnAcceptClient;
                        callback();
                    }

                    m_pCurrentSocket = NULL;
                }
            }

            SDLNet_FreeSocketSet(set);
        }
    }


    // handle client's request ...

    ogeSocketMap::iterator its, itb, ite;

    int iRemoteClientCount = m_RemoteClients.size();

    if(iRemoteClientCount > 0)
    {
        SDLNet_SocketSet set = SDLNet_AllocSocketSet(iRemoteClientCount);

        if(set)
        {
            itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
            for ( its=itb; its!=ite; its++ ) SDLNet_TCP_AddSocket(set, its->second->m_iSocket);

            SDLNet_CheckSockets(set, 0);

            itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
            for(its=itb; its!=ite; its++)
            if(SDLNet_SocketReady(its->second->m_iSocket))
            {
                int len = 0;
                if(its->second->RecvData(&len))
                {
                    // skip heartbeat msg ...
                    if(len == 0 && its->second->GetMessageType() == _OGE_SOCK_MSG_HB_) continue;

                    // as all network actions will be executed step by step in one single thread,
                    // so we could use global buffer to pass the data ...
                    m_pCurrentData = its->second->GetInBuf();
                    m_iCurrentDataSize = len;
                    m_iCurrentMessageType = its->second->GetMessageType();
                    m_iCurrentMessageValue = its->second->GetMessageValue();
                    m_pCurrentSocket = its->second;

                    if(m_pOnReceiveFromRemoteClient)
                    {
                        ogeCommonFunction callback = (ogeCommonFunction)m_pOnReceiveFromRemoteClient;
                        callback();
                    }

                }

                //if(pMsg) free(pMsg);

                m_pCurrentData = NULL;
                m_iCurrentDataSize = 0;

                m_pCurrentSocket = NULL;

            }
            else
            {
                //KickClient(its->first);
            }

            SDLNet_FreeSocketSet(set);
        }
    }


    // handle server's request ...

    int iRemoteServerCount = m_RemoteServers.size();

    if(iRemoteServerCount > 0)
    {
        SDLNet_SocketSet set = SDLNet_AllocSocketSet(iRemoteServerCount);

        if(set)
        {
            itb = m_RemoteServers.begin(); ite = m_RemoteServers.end();
            for ( its=itb; its!=ite; its++ ) SDLNet_TCP_AddSocket(set, its->second->m_iSocket);

            SDLNet_CheckSockets(set, 0);

            itb = m_RemoteServers.begin(); ite = m_RemoteServers.end();
            for(its=itb; its!=ite; its++)
            if(SDLNet_SocketReady(its->second->m_iSocket))
            {
                int len = 0;
                if(its->second->RecvData(&len))
                {
                    // skip heartbeat msg ...
                    if(len == 0 && its->second->GetMessageType() == _OGE_SOCK_MSG_HB_) continue;

                    // as all network actions will be executed step by step in one single thread,
                    // so we could use global buffer to pass the data ...
                    m_pCurrentData = its->second->GetInBuf();
                    m_iCurrentDataSize = len;
                    m_iCurrentMessageType = its->second->GetMessageType();
                    m_iCurrentMessageValue = its->second->GetMessageValue();
                    m_pCurrentSocket = its->second;

                    if(m_pOnReceiveFromRemoteServer)
                    {
                        ogeCommonFunction callback = (ogeCommonFunction)m_pOnReceiveFromRemoteServer;
                        callback();
                    }
                }

                //if(pMsg) free(pMsg);

                m_pCurrentData = NULL;
                m_iCurrentDataSize = 0;

                m_pCurrentSocket = NULL;

            }
            else
            {
                //DisconnectServer(its->first);
            }

            SDLNet_FreeSocketSet(set);
        }
    }

    // check connections ...
    if(!IsRemoteDebugMode())
    {
        int iCurrentTick = SDL_GetTicks();
        if(iCurrentTick - m_iLastTickCheck >= m_iCheckInterval)
        {
            // check connections ...

            //printf("\n Check connections: %d \n", iCurrentTick);

            ogeSocketMap::iterator its, itb, ite;

            itb = m_RemoteClients.begin(); ite = m_RemoteClients.end();
            for ( its=itb; its!=ite; its++ )
            {
                if(iCurrentTick - its->second->m_iLastCheckTick >= m_iTimeoutTime)
                {
                    KickClient(its->first);
                }
            }

            itb = m_RemoteServers.begin(); ite = m_RemoteServers.end();
            for ( its=itb; its!=ite; its++ )
            {
                if(iCurrentTick - its->second->m_iLastCheckTick >= m_iTimeoutTime)
                {
                    DisconnectServer(its->first);
                }
            }

            m_iLastTickCheck = SDL_GetTicks();
        }
    }

    // update queues ...

    while(m_ComeinClientQueue.size() > 0)
    {
        its = m_ComeinClientQueue.begin();
        AddClient(its->second->m_sAddr);
    }

    while(m_GetoutClientQueue.size() > 0)
    {
        its = m_GetoutClientQueue.begin();
        m_pCurrentSocket = its->second;

        //if(pScript && m_iEventOnRemoteClientDisconnect>=0)
        //pScript->CallFunction(m_iEventOnRemoteClientDisconnect);

        if(m_pOnRemoteClientDisconnect)
        {
            ogeCommonFunction callback = (ogeCommonFunction)m_pOnRemoteClientDisconnect;
            callback();
        }

        RemoveClient(its->second->m_sAddr);

        m_pCurrentSocket = NULL;
    }

    while(m_ComeinServerQueue.size() > 0)
    {
        its = m_ComeinServerQueue.begin();
        AddServer(its->second->m_sAddr);
    }

    while(m_GetoutServerQueue.size() > 0)
    {
        its = m_GetoutServerQueue.begin();

        m_pCurrentSocket = its->second;
        if(m_pOnRemoteServerDisconnect)
        {
            ogeCommonFunction callback = (ogeCommonFunction)m_pOnRemoteServerDisconnect;
            callback();
        }

        RemoveServer(its->second->m_sAddr);

        m_pCurrentSocket = NULL;
    }

    return 0;
}

int CogeNetwork::ScanDebugRequest()
{
    if(m_DebugSocket && m_DebugSocket->m_iSocket)
    {
        SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
        if(set)
        {
            SDLNet_TCP_AddSocket(set, m_DebugSocket->m_iSocket);

            int numready = SDLNet_CheckSockets(set, 0);

            if(numready > 0)
            {
                if(SDLNet_SocketReady(m_DebugSocket->m_iSocket))
                {
                    int len = 0;
                    if(m_DebugSocket->RecvData(&len))
                    {
                        if (len > 0 && m_DebugSocket->GetMessageType() != _OGE_SOCK_MSG_HB_ && m_pOnRemoteDebugService)
                        {
                            ogeCommonFunction callback = (ogeCommonFunction)m_pOnRemoteDebugService;
                            callback();
                        }
                    }
                    else
                    {
                        if(SDLNet_GetError() && strlen(SDLNet_GetError())) // sometimes blank!
                        {
                            OGE_Log("SDLNet_TCP_Recv(when remote debug): %s\n", SDLNet_GetError());
                            return -1;
                        }
                    }
                }
            } else if(numready < 0) return -1;

            SDLNet_FreeSocketSet(set);
        }
    }

    return 0;

}
