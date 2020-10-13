#include "threadedTcpClient.h"

#include "Debugging.h"
#include "Log.h"

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h>

	#define closesocket close
#endif

ThreadedTcpConnection::ThreadedTcpConnection()
	: wantsToStop(false)
{
}

bool ThreadedTcpConnection::init(
	const uint32_t ipAddress,
	const uint16_t tcpPort,
	const Options & options,
	const std::function<void()> threadFunction)
{
	Assert(isActive == false);
	
	if (isActive)
	{
		beginShutdown();
		
		waitForShutdown();
	}
	
	//
	
	bool success = true;
	
	isActive = true;
	
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1)
	{
		LOG_ERR("failed opening socket");
		success = false;
	}
	
	if (success)
	{
		// optionally disable nagle's algorithm and send out packets immediately on write
		const int sock_value = options.noDelay;
		LOG_INF("setting TCP_NODELAY to %d", sock_value);
		if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&sock_value, sizeof(sock_value)) == -1)
		{
			LOG_ERR("failed to set TCP_NODELAY socket option");
			success = false;
		}
	}
	
	if (success)
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ipAddress);
		addr.sin_port = htons(tcpPort);
		
		thread = std::thread([=]()
		{
			// avoid SIGPIPE signals from being generated. they will crash the program if left unhandled. we check error codes from send/recv and assume the user does so too, so we don't need signals to kill our app
			
			signal(SIGPIPE, SIG_IGN);
			
			LOG_DBG("connecting socket to remote endpoint");
		
		// todo : connect from the main thread ?
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				LOG_ERR("failed to connect socket");
				
				closesocket(sock);
				sock = -1;
			}
			else
			{
				LOG_DBG("connected to remote endpoint!");
				
				LOG_DBG("invoking thread function");
				
				threadFunction();
				
				LOG_DBG("begin graceful shutdown TCP layer");
				
				if (shutdown(sock, SHUT_WR) == -1)
				{
					LOG_ERR("failed to shutdown socket. closing socket NOW");
				
					// close the socket
					closesocket(sock);
					sock = -1;
				}
			}
		});
	}
	
	return success;
}

void ThreadedTcpConnection::beginShutdown()
{
	if (isActive)
	{
		wantsToStop = true;
		
	// todo : signal socket so recv is interrupted
	// fixme : socket is created on another thread. if we want to signal: we need a mutex. or create/close the socket on the main thread
		
		thread.join();
		
		wantsToStop = false;
		
		isActive = false;
	}
}

void ThreadedTcpConnection::waitForShutdown()
{
// todo : set TCP_CONNECTIONTIMEOUT (initial connection timeout)
// todo : set TCP_RXT_CONNDROPTIME (drop/retransmission timeout)
// todo : set TCP_SENDMOREACKS for audio streamers (requires sender to hold less data)

	if (sock != -1)
	{
		// perform recv on the socket. a recv of zero bytes means the underlying TCP
		// layer has completed the shutdown sequence
		char c;
		while (recv(sock, &c, 1, 0) != 0)
			sleep(0);
		
		// close the socket
		closesocket(sock);
		sock = -1;
	}
}
