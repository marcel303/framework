#pragma once

#include <vector>
#include "NetAddress.h"
#include "NetSocket.h"
#include "PolledTimer.h"
#include "SharedPtr.h"

class DiscoverServer
{
public:
	NetAddress m_address;
	int m_time;
};

class Discover
{
public:
	Discover(bool isServer, bool isClient);
	~Discover();

	void Update();

	std::vector<DiscoverServer> GetServers();

private:
	void CL_Receive();
	void CL_Purge();
	void CL_Refresh(NetAddress& address);
	void SV_Broadcast();

	std::vector<DiscoverServer> m_servers;
	bool m_isServer;
	bool m_isClient;
	SharedPtr<NetSocket> m_svSocket;
	SharedPtr<NetSocket> m_clSocket;
	PolledTimer m_svTimer;
};
