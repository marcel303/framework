#pragma once

#include "ip/PacketListener.h"
#include "ip/UdpSocket.h"
#include <mutex>
#include <stdint.h>
#include <thread>
#include <vector>

#define ARTNET_TO_DMX_PORT 6454
#define ARTNET_TO_LED_PORT 6456

#define I2S_2CH_FRAME_COUNT   128
#define I2S_2CH_CHANNEL_COUNT 2
#define I2S_2CH_BUFFER_COUNT  2
#define I2S_2CH_PORT          6458

#define I2S_4CH_FRAME_COUNT   128
#define I2S_4CH_CHANNEL_COUNT 4
#define I2S_4CH_BUFFER_COUNT  2
#define I2S_4CH_PORT          6459

#define I2S_1CH_8_FRAME_COUNT   1024
#define I2S_1CH_8_CHANNEL_COUNT 1
#define I2S_1CH_8_BUFFER_COUNT  8
#define I2S_1CH_8_PORT          6460

enum NodeCapabilities
{
	kNodeCapability_ArtnetToDmx       = 1 << 0,
	kNodeCapability_ArtnetToLedstrip  = 1 << 1,
	kNodeCapability_ArtnetToAnalogPin = 1 << 2,
	kNodeCapability_TcpToI2S          = 1 << 3,
	kNodeCapability_Webpage           = 1 << 4,
	kNodeCapability_TcpToI2SQuad      = 1 << 5,
	kNodeCapability_TcpToI2SMono8     = 1 << 6
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
	uint64_t id = 0;
	uint32_t capabilities = 0;
	char description[32] = { };
	IpEndpointName endpointName;
	int64_t receiveTime = 0; // the time (in miliseconds) at which a discovery message for this node was last received
	
	bool isValid() const
	{
		return id != 0;
	}
};

class NodeDiscoveryProcess : public PacketListener
{
	UdpListeningReceiveSocket * receiveSocket = nullptr;
	
	std::mutex * mutex = nullptr;
	
	std::thread * thread = nullptr;
	
public:
	~NodeDiscoveryProcess();
	
	void init();
	void shut();
	
	void purgeStaleRecords(const int timeoutInSeconds);
	
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
