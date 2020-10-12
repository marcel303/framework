#include "dummyTcpServer.h"

// 1st party includes
#include "Debugging.h"
#include "Log.h"

// system includes

#include <stdio.h>

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	//#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h> // close
	#define closesocket close
#endif

void Test_DummyTcpReceiver::Client::run(const int in_sock)
{
	sock = in_sock;
	
	thread = std::thread([=]()
	{
		// call recv in a loop in discard data
		
		int n = 0;
		
		for (;;)
		{
			uint8_t b;
			const int ret = recv(sock, &b, 1, 0);
			if (ret != 1)
				break;
			
			if ((n++ % 1000) == 0)
				LOG_DBG("received byte: %02x", b);
		}
		
		LOG_DBG("begin graceful shutdown TCP layer");
		
		shutdown(sock, SHUT_WR);
	});
}

void Test_DummyTcpReceiver::Client::beginShutdown()
{
	shutdown(sock, SHUT_RDWR);
	
	// todo : signal socket so recv is interrupted
	
	thread.join();
}

void Test_DummyTcpReceiver::Client::waitForShutdown()
{
	if (sock != -1)
	{
		char c;
		while (recv(sock, &c, 1, 0) != 0)
			sleep(0);
		
		closesocket(sock);
		sock = -1;
	}
}

//

bool Test_DummyTcpReceiver::init(const IpEndpointName & endpointName)
{
	bool success = true;
	
	struct sockaddr_in addr;

	if (success)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
	
		if (sock == -1)
		{
			LOG_ERR("failed opening socket");
			success = false;
		}
	}
	
	if (success)
	{
		const int sock_value = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sock_value, sizeof(sock_value)) == -1)
		{
			LOG_ERR("failed to set SO_REUSEADDR socket option");
			success = false;
		}
	}
	
	if (success)
	{
		const int sock_value = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sock_value, sizeof(sock_value)) == -1)
		{
			LOG_ERR("failed to set SO_REUSEPORT socket option");
			success = false;
		}
	}

	if (success)
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr =
			endpointName.address == IpEndpointName::ANY_ADDRESS
			? INADDR_ANY
			: htonl(endpointName.address);
		addr.sin_port = htons(endpointName.port);
	
		const int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
		
		if (ret != 0)
		{
			LOG_ERR("failed to bind socket to address");
			success = false;
		}
	}
	
	if (success)
	{
		const int ret = listen(sock, 8);
		
		if (ret != 0)
		{
			LOG_ERR("failed to transition socket to listening state");
			success = false;
		}
	}
	
	if (success)
	{
		thread = std::thread([=]()
		{
			while (true) // todo : check stop condition
			{
				const int client_sock = accept(sock, nullptr, nullptr);
				
				if (client_sock == -1)
				{
					Assert(errno == ECONNABORTED);
					
					// socket got closed from the main thread. we're done here
					break;
				}
				else
				{
					LOG_DBG("accepting client");
					
					// add client
					
					Client * client = new Client();
					
					client->run(client_sock);
					
					clients.push_back(client);
				}
			}
		});
	}
	
	if (success == false)
	{
		LOG_ERR("an error occurred at init");
		
		if (sock != -1)
		{
			closesocket(sock);
			sock = -1;
		}
	}
	
	return success;
}

void Test_DummyTcpReceiver::beginShutdown()
{
	closesocket(sock);
	sock = -1;
	
	thread.join();
	
	// begin graceful TCP shutdown on all clients
	
	for (auto * client : clients)
	{
		client->beginShutdown();
	}
}

void Test_DummyTcpReceiver::waitForShutdown()
{
	// wait for graceful TCP shutdown to complete for all clients
	
	for (auto * client : clients)
	{
		client->waitForShutdown();
	}
	
	// free clients
	
	for (auto *& client : clients)
	{
		delete client;
		client = nullptr;
	}
	
	clients.clear();
}
