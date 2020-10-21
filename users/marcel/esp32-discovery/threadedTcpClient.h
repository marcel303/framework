#pragma once

#include <atomic>
#include <functional>
#include <stdint.h>
#include <thread>

struct ThreadedTcpConnection
{
	struct Options
	{
		bool noDelay = false;
	};
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	int sock = -1;
	
	ThreadedTcpConnection();
	
	bool init(
		const uint32_t ipAddress,
		const uint16_t tcpPort,
		const Options & options,
		const std::function<void()> threadFunction);

	void beginShutdown();
	void waitForShutdown();
};
