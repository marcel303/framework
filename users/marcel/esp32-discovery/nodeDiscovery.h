#pragma once

#include "ip/PacketListener.h"
#include "ip/UdpSocket.h"
#include <stdint.h>
#include <vector>

struct SDL_mutex;
struct SDL_Thread;

enum NodeCapabilities
{
	kNodeCapability_ArtnetToDmx       = 1 << 0,
	kNodeCapability_ArtnetToLedstrip  = 1 << 1,
	kNodeCapability_ArtnetToAnalogPin = 1 << 2,
	kNodeCapability_TcpToI2S          = 1 << 3,
	kNodeCapability_Webpage           = 1 << 4,
	kNodeCapability_TcpToI2SQuad      = 1 << 5
};

struct NodeDiscoveryPacket
{
	uint64_t id;
	char version[4];
	uint32_t capabilities;
	char description[32];
};

struct NodeDiscoveryRecord
{
	uint64_t id;
	uint32_t capabilities;
	char description[32];
	IpEndpointName endpointName;
};

class NodeDiscoveryProcess : public PacketListener
{
	UdpListeningReceiveSocket * receiveSocket = nullptr;
	
	SDL_mutex * mutex = nullptr;
	
	SDL_Thread * thread = nullptr;
	
public:
	~NodeDiscoveryProcess();
	
	void init();
	void shut();
	
	const int getRecordCount() const;
	
	NodeDiscoveryRecord getDiscoveryRecord(const int index) const;

private:
	std::vector<NodeDiscoveryRecord> discoveryRecords;

	void beginThread();
	void endThread();
	
	void lock() const;
	void unlock() const;
	
	static int threadMain(void * obj);
	
	// PacketListener implementation
	
	virtual void ProcessPacket(const char * data, int size, const IpEndpointName & remoteEndpoint) override final;
};
