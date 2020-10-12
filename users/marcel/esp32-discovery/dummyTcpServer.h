#pragma once

#include "ip/UdpSocket.h"

#include <thread>
#include <vector>

// a TCP data receiver that will just receive data, discard it, and nothing else
struct Test_DummyTcpReceiver
{
	struct Client
	{
		std::thread thread;
		
		int sock = -1;
		
		void run(const int in_sock);

		void beginShutdown();
		void waitForShutdown();
	};
	
	int sock = -1;
	
	std::thread thread;
	
	std::vector<Client*> clients;
	
	bool init(const IpEndpointName & endpointName);
	
	void beginShutdown();
	void waitForShutdown();
};
