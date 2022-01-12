#include "threadedTcpClient.h"

#include "Debugging.h"
#include "Log.h"

#include <signal.h>

#if defined(WINDOWS)
	#include <winsock2.h>
	#define SHUT_RDWR (SD_SEND | SD_RECEIVE)
	static void sleep(int ms) { Sleep(ms); }
#else
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h> // close

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
	Assert(sock == -1);
	
	bool success = true;
	
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1)
	{
		LOG_ERR("failed to open socket");
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
		#if !defined(WINDOWS)
			// avoid SIGPIPE signals from being generated. they will crash the program if left unhandled. we check error codes from send/recv and assume the user does so too, so we don't need signals to kill our app
			
			signal(SIGPIPE, SIG_IGN);
		#endif

			LOG_DBG("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				LOG_ERR("failed to connect socket");
			}
			else
			{
				LOG_DBG("connected to remote endpoint!");
				
				LOG_DBG("invoking thread function");
				
				threadFunction();
			}
		});
	}
	
	return success;
}

void ThreadedTcpConnection::beginShutdown()
{
	if (sock != -1)
	{
		LOG_DBG("begin graceful shutdown TCP layer");
		
		if (shutdown(sock, SHUT_RDWR) == -1)
		{
			LOG_ERR("failed to shutdown TCP layer");
		}
	}
	
	wantsToStop = true;
}

void ThreadedTcpConnection::waitForShutdown()
{
	if (thread.joinable())
	{
		thread.join();
	}
	
	wantsToStop = false;

	if (sock != -1)
	{
		LOG_DBG("waiting for graceful shutdown TCP layer");
		
		// perform recv on the socket. a recv of zero bytes means the underlying TCP
		// layer has completed the shutdown sequence. a recv of -1 means an error
		// has occurred. we want to keep recv'ing until either condition occurred
		char c;
		while (recv(sock, &c, 1, 0) == 1)
			sleep(0);
		
		LOG_DBG("waiting for graceful shutdown TCP layer [done]");
		
		LOG_DBG("closing socket");
		
		// close the socket
		closesocket(sock);
		sock = -1;
	}
}
