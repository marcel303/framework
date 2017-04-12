#include "discover.h"
#include "Packet.h"
#include "Timer.h"

#if ENABLE_NETWORKING

namespace NetSessionDiscovery
{
	Service::Service()
		: m_purgeTime(0)
	{
	}

	void Service::init(int broadcastInterval, int purgeTime)
	{
		NetSocketUDP * socketUDP = new NetSocketUDP();
		socketUDP->Bind(8888, true);

		m_socket = socketUDP;
		m_broadcastInterval = broadcastInterval * 1000;
		m_nextBroadcastTime = 0;
		m_purgeTime = purgeTime * 1000;
	}

	Service::~Service()
	{
	}

	void Service::update(bool broadcast)
	{
		const uint64_t time = g_TimerRT.TimeMS_get();

		if (broadcast)
		{
			if (time >= m_nextBroadcastTime || m_nextBroadcastTime == 0)
			{
				m_nextBroadcastTime = time + m_broadcastInterval;

				sendAdvertiseMessage();
			}
		}
		else
		{
			m_nextBroadcastTime = 0;
		}

		receive();

		purgeServerList();
	}

	std::vector<ServerInfo> Service::getServerList() const
	{
		return m_serverList;
	}

	static bool receivePacket(NetSocket * socket, Packet & out_packet)
	{
		const int MAX_SIZE = 4096;
		static char data[MAX_SIZE];
		uint32_t size = MAX_SIZE;
		NetAddress address;

		if (socket->Receive(data, size, &size, &address))
		{
			out_packet = Packet(data, size, address);

			return true;
		}

		return false;
	}

	void Service::receive()
	{
		Packet packet;

		while (receivePacket(m_socket.get(), packet))
		{
			refreshServerInfo(packet.m_rcvAddress);
		}
	}

	void Service::purgeServerList()
	{
		const uint64_t time = g_TimerRT.TimeMS_get();

		for (std::vector<ServerInfo>::iterator i = m_serverList.begin(); i != m_serverList.end(); )
		{
			const ServerInfo & serverInfo = *i;

			if (time >= serverInfo.m_time + m_purgeTime)
			{
				i = m_serverList.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	void Service::refreshServerInfo(const NetAddress & address)
	{
    #if defined(WIN32)
		const uint64_t time = g_TimerRT.TimeMS_get();

		// find existing entry

		for (size_t i = 0; i < m_serverList.size(); ++i)
		{
			ServerInfo & serverInfo = m_serverList[i];

			const sockaddr_in * addr1 = address.GetSockAddr();
			const sockaddr_in * addr2 = serverInfo.m_address.GetSockAddr();

			if (addr1->sin_addr.S_un.S_addr == addr2->sin_addr.S_un.S_addr && addr1->sin_port == addr2->sin_port)
			{
				// update time stamp

				serverInfo.m_time = time;

				return;
			}
		}

		ServerInfo serverInfo;

		serverInfo.m_address = address;
		serverInfo.m_time = time;

		m_serverList.push_back(serverInfo);

		LOG_DBG("discovered new server. address=%s", address.ToString(true).c_str());
    #endif
	}

	void Service::sendAdvertiseMessage() const
	{
		const char * data = "ping";

		NetAddress broadcastAddress(0, 0, 0, 0, 8888);

		m_socket->Send(data, 4, &broadcastAddress);
	}
}

#endif
