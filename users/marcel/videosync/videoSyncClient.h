#pragma once

#include <arpa/inet.h>
#include <unistd.h>

struct TcpClient
{
	int m_clientSocket = -1;
	
	sockaddr_in m_serverSocketAddress;
	
	bool connect(const char * ipAddress, const int tcpPort);
	void disconnect();
	
	bool isConnected() const;
};
