#pragma once

#include <stdint.h>
#include "NetAddress.h"
#include "SharedPtr.h"

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
	virtual ~NetSocket() { }

	virtual bool Send(const void * data, uint32_t size, NetAddress * address) = 0;
	virtual bool Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address) = 0;
	virtual bool IsReliable() = 0;
};

class NetSocketUDP : public NetSocket
{
public:
	NetSocketUDP();
	~NetSocketUDP();

	bool Bind(uint16_t port, bool broadcast = false);

	virtual bool Send(const void * data, uint32_t size, NetAddress * address);
	virtual bool Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address);
	virtual bool IsReliable() { return false; }

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

typedef SharedPtr<NetSocket> SharedNetSocket;
