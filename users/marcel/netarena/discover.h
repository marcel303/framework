#pragma once

#include <vector>
#include "NetAddress.h"
#include "NetSocket.h"
#include "SharedPtr.h"

namespace NetSessionDiscovery
{
	class ServerInfo
	{
	public:
		ServerInfo()
			: m_time(0)
		{
		}

		NetAddress m_address;
		uint64_t m_time;
	};

	class Service
	{
	public:
		Service();
		~Service();

		void init(int broadcastInterval, int purgeTime);
		void update(bool broadcast);

		std::vector<ServerInfo> getServerList() const;

	private:
		void receive();
		void purgeServerList();
		void refreshServerInfo(const NetAddress & address);
		void sendAdvertiseMessage() const;

		std::vector<ServerInfo> m_serverList;
		SharedPtr<NetSocket> m_socket;
		uint64_t m_broadcastInterval;
		uint64_t m_nextBroadcastTime;
		uint64_t m_purgeTime;
	};
}
