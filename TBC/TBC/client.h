#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

#pragma comment (lib, "ws2_32.lib")

WSADATA WsaDat;
SOCKET soc;
SOCKADDR_IN socAddr;

void setup()
{
	// intitialize winsock

	if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
	{
		WSACleanup();
	}

	// socket creation
	soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (soc == INVALID_SOCKET) {
		WSACleanup();
	}
}

void nclose()
{
	closesocket(soc);
}

int connect(const char* ip)
{
	// setup
	memset((char *)&socAddr, 0, sizeof(socAddr));
	socAddr.sin_port = htons(8888);
	socAddr.sin_family = AF_INET;
	socAddr.sin_addr.s_addr = inet_addr(ip);

	// try to connect to server

	if (connect(soc, (SOCKADDR*)(&socAddr), sizeof(socAddr)) != 0) {
		WSACleanup();
		return 0;
	}

	u_long iMode = 1;
	ioctlsocket(soc, FIONBIO, &iMode);
	char v = 1;
	setsockopt(soc, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));

	return 1;
}

int nrecv(char* data, int len)
{
	return recv(soc, data, len, 0);
}
int nrecv(char* data, int len, int flags)
{
	return recv(soc, data, len, flags);
}

void nsend(char* data, int len)
{
	send(soc, data, len, 0);
}