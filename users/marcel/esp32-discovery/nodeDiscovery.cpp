#include "Debugging.h"
#include "Log.h"
#include "nodeDiscovery.h"
#include "StringEx.h"
#include <SDL2/SDL.h>
#include <string>
#include <string.h>

#define DISCOVERY_RECEIVE_PORT 2400

NodeDiscoveryProcess::~NodeDiscoveryProcess()
{
	shut();
}

void NodeDiscoveryProcess::init()
{
	Assert(receiveSocket == nullptr);
	
	std::string ipAddress;
	const int udpPort = DISCOVERY_RECEIVE_PORT;

	IpEndpointName endpointName;

	if (ipAddress.empty())
		endpointName = IpEndpointName(IpEndpointName::ANY_ADDRESS, udpPort);
	else
		endpointName = IpEndpointName(ipAddress.c_str(), udpPort);
	
	receiveSocket = new UdpListeningReceiveSocket(endpointName, this);
	
	beginThread();
}

void NodeDiscoveryProcess::shut()
{
	endThread();
	
	delete receiveSocket;
	receiveSocket = nullptr;
}

const int NodeDiscoveryProcess::getRecordCount() const
{
	int result;
	
	lock();
	{
		result = discoveryRecords.size();
	}
	unlock();
	
	return result;
}

NodeDiscoveryRecord NodeDiscoveryProcess::getDiscoveryRecord(const int index) const
{
	NodeDiscoveryRecord result;
	
	lock();
	{
		result = discoveryRecords[index];
	}
	unlock();
	
	return result;
}

void NodeDiscoveryProcess::beginThread()
{
	Assert(mutex == nullptr);
	Assert(thread == nullptr);
	
	mutex = SDL_CreateMutex();
	
	thread = SDL_CreateThread(threadMain, "ESP32 Discovery Process", this);
}

void NodeDiscoveryProcess::endThread()
{
	if (receiveSocket != nullptr)
	{
		receiveSocket->Break();
		
		delete receiveSocket;
		receiveSocket = nullptr;
	}
	
	if (thread != nullptr)
	{
		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
	}
	
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
}

void NodeDiscoveryProcess::lock() const
{
	Verify(SDL_LockMutex(mutex) == 0);
}

void NodeDiscoveryProcess::unlock() const
{
	Verify(SDL_UnlockMutex(mutex) == 0);
}

int NodeDiscoveryProcess::threadMain(void * obj)
{
	NodeDiscoveryProcess * self = (NodeDiscoveryProcess*)obj;
	
	self->receiveSocket->Run();
	
	return 0;
}

// PacketListener implementation

void NodeDiscoveryProcess::ProcessPacket(const char * data, int size, const IpEndpointName & remoteEndpoint)
{
	LOG_DBG("received UDP packet!", 0);
	
	// decode the discovery message
	
	if (size < sizeof(NodeDiscoveryPacket))
	{
		LOG_WRN("received invalid discovery message", 0);
		return;
	}
	
	NodeDiscoveryPacket * discoveryPacket = (NodeDiscoveryPacket*)data;
	
	if (memcmp(discoveryPacket->version, "v100", 4) != 0)
	{
		LOG_WRN("received discovery message with unknown version string", 0);
		return;
	}
	
	NodeDiscoveryRecord * existingRecord = nullptr;
	
	for (auto & record : discoveryRecords)
	{
		if (record.id == discoveryPacket->id)
			existingRecord = &record;
	}
	
	NodeDiscoveryRecord record;
	memset(&record, 0, sizeof(record));
	record.id = discoveryPacket->id;
	record.capabilities = discoveryPacket->capabilities;
	strcpy_s(record.description, sizeof(record.description), discoveryPacket->description);
	record.endpointName = remoteEndpoint;
	
	if (existingRecord == nullptr)
	{
		LOG_DBG("found a new node! id=%llx", discoveryPacket->id);
		
		lock();
		{
			discoveryRecords.push_back(record);
		}
		unlock();
	}
	else
	{
		if (memcmp(existingRecord, &record, sizeof(record)) != 0)
		{
			LOG_DBG("updating existing node! id=%llx", discoveryPacket->id);
			
			lock();
			{
				*existingRecord = record;
			}
			unlock();
		}
	}
}
