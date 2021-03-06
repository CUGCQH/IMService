// Client.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <string>
#include "json.hpp"
#include "Log.h"
using namespace std;
using json = nlohmann::json;
#pragma comment(lib, "ws2_32.lib")
//#include
#include <Ws2tcpip.h>
#define MaxSize  1024
#include <locale>
#include <codecvt>
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
//std::string narrow = converter.to_bytes(wide_utf16_source_string);
//std::wstring wide = converter.from_bytes(narrow_utf8_source_string);

HANDLE hMutex;
Log OutLog;
int MySend(int iSock,char* pchBuf,size_t tLen)
{
	int iThisSend;
	unsigned int iSended = 0;
	if (tLen == 0)
		return 0;
	while (iSended < tLen)
	{
		do {
			iThisSend = send(iSock, pchBuf, tLen - iSended, 0);
		} while ((iThisSend < 0) && errno == EINTR);
		if (iThisSend < 0)
		{
			return iSended;
		}
		iSended += iThisSend;
		pchBuf += iThisSend;
	}
	return tLen;
}

int MyRecv(int iSock, char* pchBuf, size_t tCount)
{
	int iThisRead;
	size_t tByteRead = 0;
	while (tByteRead < tCount)
	{
		do {
			iThisRead = _read(iSock, pchBuf, tCount - tByteRead);
		} while ((iThisRead < 0) && errno == EINTR);
		if (iThisRead < 0)
		{
			return iThisRead;
		}
		else if (iThisRead == 0)
			return tByteRead;
		tByteRead += iThisRead;
		pchBuf += iThisRead;
	}
}
//包头信息
typedef struct tag_packageHead
{
	int nPackageLen;//4Bytes,int,整个包长度
}PACKAGEHEAD, *PPACKAGEHEAD;
void Send(SOCKET sockClient)
{

	int byte = 0;
	json login;
	login["type"] = "login";
	login["userid"] = 1;
	login["pwd"] = "123456";
	string str = login.dump();
	const char * szlogin = "0123456789";
	int nMsgLen = strlen(szlogin);
	PACKAGEHEAD pkHead;
	pkHead.nPackageLen = sizeof(PACKAGEHEAD) + nMsgLen;
	char *pBuff = new char[pkHead.nPackageLen];
	memcpy(pBuff, &pkHead, sizeof(PACKAGEHEAD));
	memcpy(pBuff + sizeof(PACKAGEHEAD), szlogin, nMsgLen);

	
	PACKAGEHEAD head;
	memcpy(&head, pBuff, sizeof(PACKAGEHEAD));
	char *tmp = new char[head.nPackageLen - sizeof(PACKAGEHEAD) + 1];
	memcpy(&tmp, pBuff+ sizeof(PACKAGEHEAD), head.nPackageLen- sizeof(PACKAGEHEAD));
	memcpy(tmp, '\0', head.nPackageLen - sizeof(PACKAGEHEAD));
	//byte = send(sockClient, strlogin.c_str(), strlogin.length() + 1, 0);
	byte = MySend(sockClient, pBuff, pkHead.nPackageLen);
	/*if (pBuff)
	{
		delete[]pBuff;
		pBuff = NULL;
	}*/
	if (byte <= 0)
	{
		LOG_ERROR(" send fail");
	}
	else
		LOG_INFO(" send success");
	while (1)
	{
		WaitForSingleObject(hMutex, INFINITE);
		wchar_t sendBuf[MaxSize];
		_getws_s(sendBuf);
		json jr;
		jr["msg"] = converter.to_bytes(sendBuf);
		jr["type"] = "chat";
		string strmsg = jr.dump();
		//byte = send(sockClient, str.c_str(), str.length()+1, 0);
		char *msgbuf = new char[strmsg.length() + 4];
		*(int*)msgbuf = htonl(strmsg.length());
		memcpy(msgbuf + 4, strmsg.c_str(), strmsg.length());
		//byte = send(sockClient, strlogin.c_str(), strlogin.length() + 1, 0);
		byte = MySend(sockClient, msgbuf, strmsg.length() + 4);
		if (msgbuf)
		{
			delete[]msgbuf;
			msgbuf = NULL;
		}
		if (byte <= 0)
		{
			LOG_ERROR(" send fail");
			break;
		}
		else
			LOG_INFO(" send success");

		ReleaseMutex(hMutex);

	}
	closesocket(sockClient);//关闭socket,一次通信完毕  
}

void Recv(SOCKET sockClient)
{
	char revBuf[MaxSize];
	memset(revBuf, 0, sizeof(revBuf));
	int byte = 0;

	while (1)
	{
		WaitForSingleObject(hMutex, INFINITE);
		//byte = recv(sockClient, revBuf, MaxSize, 0);//服务器从客户端接受数据  
		char *bufMsg=new char[4];
		size_t readLen = MyRecv(sockClient, bufMsg, sizeof(int));
		int nLen = (int)ntohl(*(int*)bufMsg);
		if (bufMsg)
		{
			delete[]bufMsg;
			bufMsg = NULL;
		}
		bufMsg = new char[nLen];
		byte = MyRecv(sockClient, bufMsg, nLen);
		if (byte <= 0)
		{
			LOG_ERROR(" recv fail");
			break;
		}
		else
			LOG_INFO(" recv success {}", revBuf);
		memset(revBuf, 0, sizeof(revBuf));
		ReleaseMutex(hMutex);

	}
	closesocket(sockClient);//关闭socket,一次通信完毕  
}

void main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		LOG_ERROR(" WSAStartup error");
		return;
	}


	if (LOBYTE(wsaData.wVersion) != 1 ||
		HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return;
	}
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	//addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, "127.0.0.1", &addrSrv.sin_addr.s_addr);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);
	connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	HANDLE hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Recv, (LPVOID)sockClient, 0, 0);
	if (hThread1 == NULL)
	{
		LOG_ERROR(" hThread1 error");
	}
	else
		LOG_INFO(" hThread1 success");
	HANDLE hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Send, (LPVOID)sockClient, 0, 0);
	if (hThread2 == NULL)
	{
		LOG_ERROR(" hThread2 error");
	}
	else 
		LOG_INFO(" hThread1 success");
	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);
	CloseHandle(hThread1);
	CloseHandle(hThread2);
	WSACleanup();
}
