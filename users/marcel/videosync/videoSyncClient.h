#pragma once

#if defined(WINDOWS)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <WinSock2.h>
#else
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

namespace Videosync
{
	struct Master
	{
		int m_clientSocket = -1;
		
		sockaddr_in m_serverSocketAddress;
		
		float reconnectTimer = 0.f;
		
		bool connect(const char * ipAddress, const int tcpPort);
		void disconnect();
		
		bool reconnect();
		
		bool isConnected() const;
	};
}
