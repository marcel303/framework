#include "Log.h"
#include "NetSocket.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WINDOWS)
	#include <unistd.h>
#endif

#if !defined(WINDOWS)
	#define SOCKET_ERROR   (-1)
	#define INVALID_SOCKET (~0)
#endif

uint32_t NetSocketUDP::m_socketCount = 0;

NetSocketUDP::NetSocketUDP()
{
	m_socketCount++;

#if defined(WINDOWS)
	if (m_socketCount == 1)
		WinSockInitialize();
#endif

	CreateSocket();
}

NetSocketUDP::~NetSocketUDP()
{
	DestroySocket();

	m_socketCount--;

#if defined(WINDOWS)
	if (m_socketCount == 0)
		WinSockShutdown();
#endif
}

bool NetSocketUDP::Bind(uint16_t port, bool broadcast)
{
	LOG_DBG("NetSocket::Bind: port=%d, broadcast=%d", (int)port, (int)broadcast);

	m_serverPort = port;

	sockaddr_in socketAddress;
	memset(&socketAddress, 0, sizeof(socketAddress));
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	socketAddress.sin_port = htons(m_serverPort);

	// Set 'reuse address' option, which allows multiple sockets to bind to the same port.
	if (broadcast)
	{
		char value = 1;
		setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(char));
	}

	// Bind.
	int error = bind(m_socket, reinterpret_cast<sockaddr*>(&socketAddress), sizeof(socketAddress));
		
	if (error == SOCKET_ERROR)
	{
		LOG_ERR("socket bind failed");
		return false;
	}

	if (broadcast)
	{
		char value = 1;
		setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &value, sizeof(char));
	}

	if (false)
	{
		int receiveBufferSize = 1024*1024;
		setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&receiveBufferSize, sizeof(int));
		setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&receiveBufferSize, sizeof(int));
	}

	LOG_DBG("NetSocket::Bind [done]");

	return true;
}

bool NetSocketUDP::Send(const void * data, uint32_t size, NetAddress * address)
{
	NetAddress * dst = address ? address : &m_peerAddress;

	int size2 = sendto(
		m_socket,
		reinterpret_cast<const char *>(data),
		size, 0,
		reinterpret_cast<const sockaddr *>(dst->GetSockAddr()),
		sizeof(sockaddr_in));

	if (size2 == SOCKET_ERROR)
	{
		LOG_ERR("send failed");
		return false;
	}

	if (size2 < (int)size)
	{
		LOG_ERR("not all data was sent");
		return false;
	}

	return true;
}

bool NetSocketUDP::Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address)
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

bool NetSocketUDP::CreateSocket()
{
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (m_socket == INVALID_SOCKET)
	{
		LOG_ERR("failed to create socket");
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

bool NetSocketUDP::DestroySocket()
{
	#if defined(WINDOWS)
	closesocket(m_socket);
	#else
	close(m_socket);
	#endif

	return true;
}

#if defined(WINDOWS)
bool NetSocketUDP::WinSockInitialize()
{
	WORD version = MAKEWORD(2, 0);
	WSADATA wsaData;

	int error = WSAStartup(version, &wsaData);

	if (error != 0)
	{
		LOG_ERR("unable to startup WinSock");
		return false;
	}
	if (wsaData.wVersion != version)
	{	
		LOG_ERR("WinSock version mismatch");
		return false;
	}

	return true;
}

bool NetSocketUDP::WinSockShutdown()
{
	WSACleanup();

	return true;
}
#endif

