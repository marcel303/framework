#pragma once

#include <arpa/inet.h>
#include <unistd.h>

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
