#include "Discover.h"
#include "Packet.h"
#include "Timer.h"

Discover::Discover(bool isServer, bool isClient)
{
	m_isServer = isServer;
	m_isClient = isClient;

	if (m_isServer)
	{
		m_svSocket = new NetSocket();
		m_svSocket->Bind(8888, true);
		m_svTimer.Initialize(&g_TimerRT);
		m_svTimer.SetInterval(1.0);
	}

	if (m_isClient)
	{
		m_clSocket = new NetSocket();
		m_clSocket->Bind(8889, true);
	}
}

Discover::~Discover()
{
}

void Discover::Update()
{
	if (m_isServer)
	{
		if (m_svTimer.ReadTick())
		{
			m_svTimer.ClearTick();

			SV_Broadcast();
		}
	}

	if (m_isClient)
	{
		CL_Receive();

		CL_Purge();
	}
}

std::vector<DiscoverServer> Discover::GetServers()
{
	return m_servers;
}

static bool Receive(NetSocket* socket, Packet& out_packet)
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

void Discover::CL_Receive()
{
	Packet packet;

	while (Receive(m_clSocket.get(), packet))
	{
		//printf("receive\n");

		CL_Refresh(packet.m_rcvAddress);
	}
}

void Discover::CL_Purge()
{
}

void Discover::CL_Refresh(NetAddress& address)
{
	// Find existing entry.

	for (size_t i = 0; i < m_servers.size(); ++i)
	{
		sockaddr_in* addr1 = address.GetSockAddr();
		sockaddr_in* addr2 = m_servers[i].m_address.GetSockAddr();

		if (addr1->sin_addr.S_un.S_addr == addr2->sin_addr.S_un.S_addr && addr1->sin_port == addr2->sin_port)
		{
			// Update time.

			//printf("update\n");

			return;
		}
	}

	DiscoverServer server;

	server.m_address = address;
	server.m_time = 0; // fixme.

	m_servers.push_back(server);

	printf("Discovered new server @ %s\n", address.ToString(true).c_str());
}

void Discover::SV_Broadcast()
{
	char* data = "ping";

	NetAddress broadcast(0, 0, 0, 0, 8889);

	if (!m_svSocket->Send(data, 4, &broadcast))
		printf("broadcast err\n");

	//printf("broadcast\n");
}
