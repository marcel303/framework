#include "nodeDiscovery.h"

// libgg includes
#include "Debugging.h"
#include "Log.h"
#include "Multicore/ThreadName.h"
#include "StringEx.h"

// system includes
#include <chrono>
#include <inttypes.h> // PRIx64
#include <mutex>
#include <string>
#include <string.h>
#include <thread>

#if defined(WINDOWS)
	#include <winsock2.h>
#elif defined(LINUX)
	#include <endian.h> // be64toh/ntohll
	#include <unistd.h>
	#define ntohll(x) be64toh(x)
#else
	#include <arpa/inet.h> // ntohll
	#include <unistd.h>
#endif

#define DISCOVERY_RECEIVE_PORT 2400

static int64_t getTicks()
{
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

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

void NodeDiscoveryProcess::purgeStaleRecords(const int timeoutInSeconds)
{
	const int64_t ticks = getTicks();
	
	lock();
	{
		for (auto i = discoveryRecords.begin(); i != discoveryRecords.end(); )
		{
			auto & record = *i;
			
			const int ageInSeconds = (ticks - record.receiveTime) / 1000;
			
			if (ageInSeconds >= timeoutInSeconds)
			{
				i = discoveryRecords.erase(i);
			}
			else
			{
				++i;
			}
		}
	}
	unlock();
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
	
	mutex = new std::mutex();
	
	thread = new std::thread(threadMain, this);
}

void NodeDiscoveryProcess::endThread()
{
	if (receiveSocket != nullptr)
	{
		receiveSocket->AsynchronousBreak();
	}
	
	if (thread != nullptr)
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}
	
	if (mutex != nullptr)
	{
		delete mutex;
		mutex = nullptr;
	}
}

void NodeDiscoveryProcess::lock() const
{
	mutex->lock();
}

void NodeDiscoveryProcess::unlock() const
{
	mutex->unlock();
}

int NodeDiscoveryProcess::threadMain(void * obj)
{
	NodeDiscoveryProcess * self = (NodeDiscoveryProcess*)obj;
	
	SetCurrentThreadName("Node Discovery Process");
	
	self->receiveSocket->Run();
	
	return 0;
}

// PacketListener implementation

void NodeDiscoveryProcess::ProcessPacket(const char * data, int size, const IpEndpointName & remoteEndpoint)
{
	LOG_DBG("received UDP packet!");
	
	// decode the discovery message
	
	if (size < sizeof(NodeDiscoveryPacket))
	{
		LOG_WRN("received invalid discovery message");
		return;
	}
	
	NodeDiscoveryPacket * discoveryPacket = (NodeDiscoveryPacket*)data;
	
	if (memcmp(discoveryPacket->version, "v100", 4) != 0)
	{
		LOG_WRN("received discovery message with unknown version string");
		return;
	}
	
	NodeDiscoveryRecord * existingRecord = nullptr;
	
	for (auto & record : discoveryRecords)
	{
		if (record.id == ntohll(discoveryPacket->id))
			existingRecord = &record;
	}
	
	NodeDiscoveryRecord record;
	memset(&record, 0, sizeof(record));
	record.id = ntohll(discoveryPacket->id);
	record.capabilities = discoveryPacket->capabilities;
	strcpy_s(record.description, sizeof(record.description), discoveryPacket->description);
	record.endpointName = remoteEndpoint;
	record.receiveTime = getTicks();
	
	if (existingRecord == nullptr)
	{
		LOG_DBG("found a new node! id=%016" PRIx64, discoveryPacket->id);
		
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
			LOG_DBG("updating existing node! id=%" PRIx64, discoveryPacket->id);
			
			lock();
			{
				*existingRecord = record;
			}
			unlock();
		}
	}
}
