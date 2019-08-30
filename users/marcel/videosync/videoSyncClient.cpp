#include "Log.h"
#include "videoSyncClient.h"
#include <string.h>

#if defined(WINDOWS)
	#include <WS2tcpip.h>
#else
	#include <netinet/tcp.h>
#endif

#if defined(WINDOWS)
	#define I_HATE_WINDOWS (char*)
#else
	#define I_HATE_WINDOWS
	#define closesocket close
#endif

namespace Videosync
{
	bool Master::connect(const char * ipAddress, const int tcpPort)
	{
		bool result = true;
		
		m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		
		int set_false = 0;
		int set_true = 1;
	
	#if defined(WIN32)
		setsockopt(m_clientSocket, SOL_SOCKET, SO_LINGER, I_HATE_WINDOWS &set_false, sizeof(set_false));
		setsockopt(m_clientSocket, IPPROTO_TCP, TCP_NODELAY, I_HATE_WINDOWS &set_true, sizeof(set_true));
	#else
	#if defined(MACOS)
		setsockopt(m_clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set_true, sizeof(set_true));
	#endif
		setsockopt(m_clientSocket, SOL_SOCKET, SO_LINGER, (void*)&set_false, sizeof(set_false));	
		setsockopt(m_clientSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&set_true, sizeof(set_true));
		setsockopt(m_clientSocket, IPPROTO_TCP, TCP_NODELAY, &set_true, sizeof(set_true));
	#endif
		
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

	bool Master::reconnect()
	{
		bool result = true;
		
		if (isConnected())
		{
			disconnect();
		}
		
		m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		
		int set_false = 0;
		int set_true = 1;
		
	#if defined(WIN32)
		setsockopt(m_clientSocket, SOL_SOCKET, SO_LINGER, I_HATE_WINDOWS &set_false, sizeof(set_false));
		setsockopt(m_clientSocket, IPPROTO_TCP, TCP_NODELAY, I_HATE_WINDOWS &set_true, sizeof(set_true));
	#else
	#if defined(MACOS)
		setsockopt(m_clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set_true, sizeof(set_true));
	#endif
		setsockopt(m_clientSocket, SOL_SOCKET, SO_LINGER, (void*)&set_false, sizeof(set_false));
		setsockopt(m_clientSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&set_true, sizeof(set_true));
		setsockopt(m_clientSocket, IPPROTO_TCP, TCP_NODELAY, &set_true, sizeof(set_true));
	#endif

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

	void Master::disconnect()
	{
		if (m_clientSocket >= 0)
		{
		#if 0
			LOG_DBG("master: disconnecting..", 0);
			if (shutdown(m_clientSocket, 1) >= 0)
			{
			#if 1
				uint8_t temp;
				while (read(m_clientSocket, &temp, 1) == 1)
				{
					// not done yet
				}
			#endif
				LOG_DBG("master: disconnecting.. done", 0);
			}
			else
			{
				LOG_DBG("master: disconnecting.. failed", 0);
			}
		#endif
		
			closesocket(m_clientSocket);
			m_clientSocket = -1;
		}
	}

	bool Master::isConnected() const
	{
		return m_clientSocket >= 0;
	}
}
