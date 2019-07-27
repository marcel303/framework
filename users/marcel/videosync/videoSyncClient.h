#pragma once

#include <arpa/inet.h>
#include <unistd.h>

struct TcpClient
{
	int m_clientSocket = 0;
	
	sockaddr_in m_serverSocketAddress;
	
	bool connect(const char * ipAddress, const int tcpPort)
	{
		bool result = true;
		
		m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		
		memset(&m_serverSocketAddress, 0, sizeof(m_serverSocketAddress));
		m_serverSocketAddress.sin_family = AF_INET;
		m_serverSocketAddress.sin_addr.s_addr = inet_addr(ipAddress);
		m_serverSocketAddress.sin_port = htons(tcpPort);
		
		socklen_t serverAddressSize = sizeof(m_serverSocketAddress);
		
		if (::connect(m_clientSocket, (struct sockaddr *)&m_serverSocketAddress, serverAddressSize) < 0)
		{
			logDebug("client: connect failed");
			result = false;
		}
		
		if (result == false)
		{
			shut();
		}
		
		return result;
	}
	
	void shut()
	{
		if (m_clientSocket != 0)
		{
			close(m_clientSocket);
			m_clientSocket = 0;
		}
	}
};