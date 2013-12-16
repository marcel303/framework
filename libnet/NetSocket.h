#pragma once

#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include "NetAddress.h"

#if defined(WINDOWS)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#define SOCKET int
#endif

class NetSocket
{
public:
	NetSocket();
	~NetSocket();

	bool Bind(uint16_t port);

	bool Send(const void * data, uint32_t size, NetAddress * address);
	bool Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address);

private:
	bool CreateSocket();
	bool DestroySocket();

#if defined(WINDOWS)
	bool WinSockInitialize();
	bool WinSockShutdown();
#endif

	static uint32_t m_socketCount;

	SOCKET m_socket;
	uint16_t m_serverPort;
	NetAddress m_peerAddress;
};

typedef boost::shared_ptr<NetSocket> SharedNetSocket;
