#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WINDOWS)
	#include <unistd.h>
#endif
#include "Log.h"
#include "NetSocket.h"

#if !defined(WINDOWS)
	#define SOCKET_ERROR   (-1)
	#define INVALID_SOCKET (~0)
#endif

uint32_t NetSocket::m_socketCount = 0;

NetSocket::NetSocket()
{
	m_socketCount++;

#if defined(WINDOWS)
	if (m_socketCount == 1)
		WinSockInitialize();
#endif

	CreateSocket();
}

NetSocket::~NetSocket()
{
	DestroySocket();

	m_socketCount--;

#if defined(WINDOWS)
	if (m_socketCount == 0)
		WinSockShutdown();
#endif
}

bool NetSocket::Bind(uint16_t port, bool broadcast)
{
	m_serverPort = port;

	sockaddr_in socketAddress;
	memset(&socketAddress, 0, sizeof(socketAddress));
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	socketAddress.sin_port = htons(m_serverPort);

	// Bind.
	int error = bind(m_socket, reinterpret_cast<sockaddr*>(&socketAddress), sizeof(socketAddress));
		
	if (error == SOCKET_ERROR)
	{
		LOG_ERR("socket bind failed", 0);
		return false;
	}

	// Set 'reuse address' option.
	char value = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(char));

	if (broadcast)
		setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &value, sizeof(char));

	return true;
}

bool NetSocket::Send(const void * data, uint32_t size, NetAddress * address)
{
	NetAddress * dst = address ? address : &m_peerAddress;

	int size2 = sendto(m_socket, reinterpret_cast<const char *>(data), size, 0, reinterpret_cast<sockaddr *>(dst->GetSockAddr()), sizeof(sockaddr_in));

	if (size2 == SOCKET_ERROR)
	{
		LOG_ERR("send failed", 0);
		return false;
	}

	if (size2 < (int)size)
	{
		LOG_ERR("not all data was sent", 0);
		return false;
	}

	return true;
}

bool NetSocket::Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address)
{
	sockaddr_in address;
#if defined(WINDOWS)
	int addressSize = sizeof(sockaddr_in);
#else
	socklen_t addressSize = sizeof(sockaddr_in);
#endif

	int size = recvfrom(
		m_socket,
		(char*)out_data,
		maxSize,
		0,
		(sockaddr*)&address, &addressSize);

	if (size == SOCKET_ERROR)
	{
		// Nothing was received.
		return false;
	}

	*out_size = size;

	if (out_address)
		out_address->Set(htonl(address.sin_addr.s_addr), htons(address.sin_port));

	return true;
}

bool NetSocket::CreateSocket()
{
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (m_socket == INVALID_SOCKET)
	{
		LOG_ERR("failed to create socket", 0);
		return false;
	}

#if defined(WINDOWS)
	// Set socket to non-blocking mode.
	unsigned long nonblock = 1;
	ioctlsocket(m_socket, FIONBIO, &nonblock);
#else
	int flags = fcntl(m_socket, F_GETFL, 0);
	fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

	return true;
}

bool NetSocket::DestroySocket()
{
	#if defined(WINDOWS)
	closesocket(m_socket);
	#else
	close(m_socket);
	#endif

	return true;
}

#if defined(WINDOWS)
bool NetSocket::WinSockInitialize()
{
	WORD version = MAKEWORD(2, 0);
	WSADATA wsaData;

	int error = WSAStartup(version, &wsaData);

	if (error != 0)
	{
		LOG_ERR("unable to startup WinSock", 0);
		return false;
	}
	if (wsaData.wVersion != version)
	{	
		LOG_ERR("WinSock version mismatch", 0);
		return false;
	}

	return true;
}

bool NetSocket::WinSockShutdown()
{
	WSACleanup();

	return true;
}
#endif

