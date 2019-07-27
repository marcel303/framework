#include "Log.h"
#include "videoSyncClient.h"
#include <string.h>

bool TcpClient::connect(const char * ipAddress, const int tcpPort)
{
	bool result = true;
	
	m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	int set = 1;
	setsockopt(m_clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	
	memset(&m_serverSocketAddress, 0, sizeof(m_serverSocketAddress));
	m_serverSocketAddress.sin_family = AF_INET;
	m_serverSocketAddress.sin_addr.s_addr = inet_addr(ipAddress);
	m_serverSocketAddress.sin_port = htons(tcpPort);
	
	socklen_t serverAddressSize = sizeof(m_serverSocketAddress);
	
	if (::connect(m_clientSocket, (struct sockaddr *)&m_serverSocketAddress, serverAddressSize) < 0)
	{
		LOG_ERR("client: connect failed", 0);
		result = false;
	}
	
	if (result == false)
	{
		disconnect();
	}
	
	return result;
}

bool TcpClient::reconnect()
{
	bool result = true;
	
	m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	int set = 1;
	setsockopt(m_clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	
	socklen_t serverAddressSize = sizeof(m_serverSocketAddress);
	
	if (::connect(m_clientSocket, (struct sockaddr *)&m_serverSocketAddress, serverAddressSize) < 0)
	{
		LOG_ERR("client: connect failed", 0);
		result = false;
	}
	
	if (result == false)
	{
		disconnect();
	}
	
	return result;
}

void TcpClient::disconnect()
{
	if (m_clientSocket >= 0)
	{
		close(m_clientSocket);
		m_clientSocket = -1;
	}
}

bool TcpClient::isConnected() const
{
	return m_clientSocket >= 0;
}
