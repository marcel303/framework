#include "artnet.h"
#include "framework.h"
#include "ip/PacketListener.h"
#include "ip/UdpSocket.h"
#include "StringEx.h"
#include <atomic>
#include <vector>

/*

esp32 discovery process relies on the Arduino sketch 'esp32-wifi-configure'.
This is a sketch which lets the user select a Wifi access point and connect to it. The sketch will then proceed sending discovery messages at a regular interval. The discovery message containts the id of the device, and the IP address can be inferred from the received UDP packet.

*/

#define DISCOVERY_RECEIVE_PORT 2400
#define DISCOVERY_ID_STRING_SIZE 16

#define ARTNET_TO_DMX_PORT 6454
#define ARTNET_TO_LED_PORT 6456

#define I2S_FRAME_COUNT   128
#define I2S_CHANNEL_COUNT 2
#define I2S_BUFFER_COUNT  2
#define I2S_PORT 6458

#define I2S_QUAD_FRAME_COUNT   128
#define I2S_QUAD_CHANNEL_COUNT 2
#define I2S_QUAD_BUFFER_COUNT  2
#define I2S_QUAD_PORT 6459

enum Capabilities
{
	kCapability_ArtnetToDmx       = 1 << 0,
	kCapability_ArtnetToLedstrip  = 1 << 1,
	kCapability_ArtnetToAnalogPin = 1 << 2,
	kCapability_TcpToI2S          = 1 << 3,
	kCapability_Webpage           = 1 << 4,
	kCapability_TcpToI2SQuad      = 1 << 5
};

struct DiscoveryPacket
{
	uint64_t id;
	char version[4];
	uint32_t capabilities;
	char description[32];
};

struct DiscoveryRecord
{
	uint64_t id;
	uint32_t capabilities;
	char description[32];
	IpEndpointName endpointName;
};

std::vector<DiscoveryRecord> s_discoveryRecords;

class DiscoveryProcess : public PacketListener
{
	UdpListeningReceiveSocket * receiveSocket = nullptr;
	
	SDL_mutex * mutex = nullptr;
	
	SDL_Thread * thread = nullptr;
	
public:
	~DiscoveryProcess()
	{
		shut();
	}
	
	void init()
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
	
	void shut()
	{
		endThread();
		
		delete receiveSocket;
		receiveSocket = nullptr;
	}
	
	const int getRecordCount() const
	{
		int result;
		
		lock();
		{
			result = s_discoveryRecords.size();
		}
		unlock();
		
		return result;
	}
	
	DiscoveryRecord getDiscoveryRecord(const int index) const
	{
		DiscoveryRecord result;
		
		lock();
		{
			result = s_discoveryRecords[index];
		}
		unlock();
		
		return result;
	}

private:
	void beginThread()
	{
		Assert(mutex == nullptr);
		Assert(thread == nullptr);
		
		mutex = SDL_CreateMutex();
		
		thread = SDL_CreateThread(threadMain, "ESP32 Discovery Process", this);
	}
	
	void endThread()
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
	
	void lock() const
	{
		Verify(SDL_LockMutex(mutex) == 0);
	}
	
	void unlock() const
	{
		Verify(SDL_UnlockMutex(mutex) == 0);
	}
	
	static int threadMain(void * obj)
	{
		DiscoveryProcess * self = (DiscoveryProcess*)obj;
		
		self->receiveSocket->Run();
		
		return 0;
	}
	
	// PacketListener implementation
	
	virtual void ProcessPacket(const char * data, int size, const IpEndpointName & remoteEndpoint) override
	{
		logDebug("received UDP packet!");
		
		// decode the discovery message
		
		if (size < sizeof(DiscoveryPacket))
		{
			logWarning("received invalid discovery message");
			return;
		}
		
		DiscoveryPacket * discoveryPacket = (DiscoveryPacket*)data;
		
		if (memcmp(discoveryPacket->version, "v100", 4) != 0)
		{
			logWarning("received discovery message with unknown version string");
			return;
		}
		
		DiscoveryRecord * existingRecord = nullptr;
		
		for (auto & record : s_discoveryRecords)
		{
			if (record.id == discoveryPacket->id)
				existingRecord = &record;
		}
		
		DiscoveryRecord record;
		memset(&record, 0, sizeof(record));
		record.id = discoveryPacket->id;
		record.capabilities = discoveryPacket->capabilities;
		strcpy_s(record.description, sizeof(record.description), discoveryPacket->description);
		record.endpointName = remoteEndpoint;
		
		if (existingRecord == nullptr)
		{
			logDebug("found a new node! id=%llx", discoveryPacket->id);
			
			lock();
			{
				s_discoveryRecords.push_back(record);
			}
			unlock();
		}
		else
		{
			if (memcmp(existingRecord, &record, sizeof(record)) != 0)
			{
				logDebug("updating existing node! id=%llx", discoveryPacket->id);
				
				lock();
				{
					*existingRecord = record;
				}
				unlock();
			}
		}
	}
};

//

#include "audiostream/AudioIO.h"
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct Test_TcpToI2S
{
	bool isActive = false;
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	Test_TcpToI2S()
		: wantsToStop(false)
	{
	}
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort)
	{
		Assert(isActive == false);
		
		if (isActive)
		{
			shut();
		}
		
		isActive = true;
		
		thread = std::thread([=]()
		{
			struct sockaddr_in addr;
			int sock = 0;
			int sock_value = 0;
			SoundData * soundData = nullptr;
			int samplePosition = 0;
			
			//
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			
			if (sock == -1)
			{
				logError("failed opening socket");
				goto error;
			}
			
			// disable nagle's algorithm and send out packets immediately on write
			
		#if 1
			sock_value = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_value, sizeof(sock_value));
		#endif
		
		#if 1
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			sock_value =
				I2S_BUFFER_COUNT  * /* N times buffered */
				I2S_FRAME_COUNT   * /* frame count */
				I2S_CHANNEL_COUNT * /* stereo */
				sizeof(int16_t) /* sample size */;
 			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sock_value, sizeof(sock_value));
		#endif
		
			// todo : TCP_NODELAY (osx). disables nagle's algorithm and sends out packets immediately on write
			// todo : TCP_NOPUSH (osx)
			// todo : TCP_CORK (linux). will manually batch messages and send them when the cork is removed

		// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(ipAddress);
			addr.sin_port = htons(tcpPort);

			logDebug("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				logError("failed to connect socket");
				goto error;
			}
			
			logError("connected to remote endpoint!");
			
			soundData = loadSound("loop01.ogg");
			
			while (wantsToStop.load() == false)
			{
				// todo : detect is disconnected, and attempt to reconnect
				// todo : avoid high CPU on disconnect
				// todo : perform disconnection test
				
				while (keyboard.isDown(SDLK_SPACE))
				{
					SDL_Delay(10);
				}
				
				// todo : generate some audio data
				
				int16_t data[I2S_FRAME_COUNT][2];
				
				// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
				if (soundData->sampleCount == 0 ||
					soundData->channelCount != 2 ||
					soundData->channelSize != 2)
				{
					memset(data, 0, sizeof(data));
				}
				else
				{
					const int16_t * samples = (const int16_t*)soundData->sampleData;
					
					const int volume = mouse.x * 256 / 800;
					
					for (int i = 0; i < I2S_FRAME_COUNT; ++i)
					{
						data[i][0] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][1] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						
						samplePosition++;
						
						if (samplePosition == soundData->sampleCount)
							samplePosition = 0;
					}
				}
				
				send(sock, data, sizeof(data), 0);
			}
			
		error:
			delete soundData;
			soundData = nullptr;
			
			if (sock != -1)
			{
				close(sock);
				sock = -1;
			}
		});
		
		return true;
	}
	
	void shut()
	{
		if (isActive)
		{
			wantsToStop = true;
			
			thread.join();
			
			wantsToStop = false;
			
			isActive = false;
		}
	}
};

//

struct Test_TcpToI2SQuad
{
	bool isActive = false;
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	Test_TcpToI2SQuad()
		: wantsToStop(false)
	{
	}
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort)
	{
		Assert(isActive == false);
		
		if (isActive)
		{
			shut();
		}
		
		isActive = true;
		
		thread = std::thread([=]()
		{
			struct sockaddr_in addr;
			int sock = 0;
			int sock_value = 0;
			SoundData * soundData = nullptr;
			int samplePosition = 0;
			
			//
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			
			if (sock == -1)
			{
				logError("failed opening socket");
				goto error;
			}
			
			// disable nagle's algorithm and send out packets immediately on write
			
		#if 1
			sock_value = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_value, sizeof(sock_value));
		#endif
		
		#if 1
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			sock_value =
				I2S_QUAD_BUFFER_COUNT  * /* N times buffered */
				I2S_QUAD_FRAME_COUNT   * /* frame count */
				I2S_QUAD_CHANNEL_COUNT * /* stereo */
				sizeof(int16_t) /* sample size */;
 			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sock_value, sizeof(sock_value));
		#endif
		
			// todo : TCP_NODELAY (osx). disables nagle's algorithm and sends out packets immediately on write
			// todo : TCP_NOPUSH (osx)
			// todo : TCP_CORK (linux). will manually batch messages and send them when the cork is removed

		// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(ipAddress);
			addr.sin_port = htons(tcpPort);

			logDebug("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				logError("failed to connect socket");
				goto error;
			}
			
			logError("connected to remote endpoint!");
			
			soundData = loadSound("loop01.ogg");
			
			logDebug("frame size: %d", I2S_QUAD_FRAME_COUNT * 4 * sizeof(int16_t));
			
			while (wantsToStop.load() == false)
			{
				// todo : detect is disconnected, and attempt to reconnect
				// todo : avoid high CPU on disconnect
				// todo : perform disconnection test
				
				while (keyboard.isDown(SDLK_SPACE))
				{
					SDL_Delay(10);
				}
				
				// todo : generate some audio data
				
				int16_t data[I2S_QUAD_FRAME_COUNT][4];
				
				// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
				if (soundData->sampleCount == 0 ||
					soundData->channelCount != 2 ||
					soundData->channelSize != 2)
				{
					memset(data, 0, sizeof(data));
				}
				else
				{
					const int16_t * samples = (const int16_t*)soundData->sampleData;
					
					const int volume = mouse.x * 256 / 800;
					
					for (int i = 0; i < I2S_QUAD_FRAME_COUNT; ++i)
					{
						data[i][0] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][1] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						data[i][2] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][3] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						
						samplePosition++;
						
						if (samplePosition == soundData->sampleCount)
							samplePosition = 0;
					}
				}
				
				send(sock, data, sizeof(data), 0);
			}
			
		error:
			delete soundData;
			soundData = nullptr;
			
			if (sock != -1)
			{
				close(sock);
				sock = -1;
			}
		});
		
		return true;
	}
	
	void shut()
	{
		if (isActive)
		{
			wantsToStop = true;
			{
				thread.join();
			}
			wantsToStop = false;
			
			isActive = false;
		}
	}
};

//

#include <list>

struct NodeState
{
	uint64_t nodeId = -1;
	
	Test_TcpToI2S test_tcpToI2S;
	Test_TcpToI2SQuad test_tcpToI2SQuad;
	
	struct
	{
		uint8_t sequenceNumber = 0;
	} artnetToDmx;
	
	struct
	{
		uint8_t sequenceNumber = 0;
	} artnetToLedstrip;
};

static std::list<NodeState> s_nodeStates;

static NodeState & findOrCreateNodeState(const uint64_t id)
{
	for (auto & nodeState : s_nodeStates)
	{
		if (nodeState.nodeId == id)
			return nodeState;
	}
	
	s_nodeStates.emplace_back();
	auto & nodeState = s_nodeStates.back();
	nodeState.nodeId = id;
	return nodeState;
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	if (!framework.init(800, 600))
		return -1;
	
	DiscoveryProcess discoveryProcess;
	
	discoveryProcess.init();
	
	UdpSocket artnetSocket;
	
	for (;;)
	{
		framework.process();

		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		if (framework.quitRequested)
			break;

		const int numRecords = discoveryProcess.getRecordCount();
		
		for (int i = 0; i < numRecords; ++i)
		{
			auto record = discoveryProcess.getDiscoveryRecord(i);
			
			auto & nodeState = findOrCreateNodeState(record.id);
			
			if (record.capabilities & kCapability_TcpToI2SQuad)
			{
				if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2SQuad.shut();
					}
					else
					{
						nodeState.test_tcpToI2SQuad.init(record.endpointName.address, I2S_QUAD_PORT);
					}
				}
			}
			else if (record.capabilities & kCapability_TcpToI2S)
			{
				if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2S.shut();
					}
					else
					{
						nodeState.test_tcpToI2S.init(record.endpointName.address, I2S_PORT);
					}
				}
			}
		
			// send some artnet data to discovered nodes
			
			if (record.capabilities & kCapability_ArtnetToDmx)
			{
				ArtnetPacket packet;
				
				nodeState.artnetToDmx.sequenceNumber = (nodeState.artnetToDmx.sequenceNumber + 30) % 256;
				if (nodeState.artnetToDmx.sequenceNumber == 0)
					nodeState.artnetToDmx.sequenceNumber++;
				
				auto * values = packet.makeDMX512(4, nodeState.artnetToDmx.sequenceNumber);
				
				for (int i = 0; i < 4; ++i)
				{
					const float brightness = mouse.x / 800.f;
					const float value = (sinf(framework.time * (i + 1) / 10.f) + 1.f) / 2.f * brightness;
					values[i] = uint8_t(value * 255.f);
				}
			
				const IpEndpointName remoteEndpoint(record.endpointName.address, ARTNET_TO_DMX_PORT);
				
				artnetSocket.SendTo(remoteEndpoint, (const char*)packet.data, packet.dataSize);
			}
			
			if (record.capabilities & kCapability_ArtnetToLedstrip)
			{
				ArtnetPacket packet;
			
				auto packFloatToDmx16 = [](const float value, const float gamma, uint8_t & hi, uint8_t & lo)
				{
					const uint16_t value16 = uint16_t(powf(value, gamma) * (1 << 16));
					hi = value16 >> 8;
					lo = uint8_t(value16);
				};
			
				nodeState.artnetToLedstrip.sequenceNumber = (nodeState.artnetToLedstrip.sequenceNumber + 30) % 256;
				if (nodeState.artnetToLedstrip.sequenceNumber == 0)
					nodeState.artnetToLedstrip.sequenceNumber++;
				
				auto * values = packet.makeDMX512(6, nodeState.artnetToLedstrip.sequenceNumber);
			
				for (int i = 0; i < 3; ++i)
				{
					const float brightness = mouse.x / 800.f;
					const float value = (sinf(framework.time * (i + 1) / 10.f) + 1.f) / 2.f * brightness;
					packFloatToDmx16(value, 1.f, values[i * 2 + 0], values[i * 2 + 1]);
				}
				
				const IpEndpointName remoteEndpoint(record.endpointName.address, ARTNET_TO_LED_PORT);
				
				artnetSocket.SendTo(remoteEndpoint, (const char*)packet.data, packet.dataSize);
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			const int numRecords = discoveryProcess.getRecordCount();
			
			for (int i = 0; i < numRecords; ++i)
			{
				auto record = discoveryProcess.getDiscoveryRecord(i);
				
				auto & nodeState = findOrCreateNodeState(record.id);
				
				char endpointName[IpEndpointName::ADDRESS_STRING_LENGTH];
				record.endpointName.AddressAsString(endpointName);
				
				const int sy = 22;
				
				const int x1 = 0;
				const int y1 = (i + 0) * sy;
				const int x2 = 300;
				const int y2 = (i + 1) * sy;
				
				const bool isInside = mouse.x >= x1 && mouse.x < x2 && mouse.y >= y1 && mouse.y < y2;
				const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
				const bool isClicked = isInside && mouse.wentUp(BUTTON_LEFT);
				
				if (isInside)
				{
					setColor(isDown ? colorRed : colorBlue);
					drawRect(x1, y1, x2, y2);
				}
				
				if (isClicked && (record.capabilities & kCapability_Webpage) != 0)
				{
					char command[128];
					sprintf_s(command, sizeof(command), "open http://%s", endpointName);
					system(command);
				}
				
				int x = x1 + 10;
				
				setColor(colorGreen);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", endpointName);
				x += 100;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%llx", record.id);
				x += 100;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", record.description);
				x += 140;
				
				if (record.capabilities & kCapability_ArtnetToDmx)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2DMX");
					x += 100;
				}
				
				if (record.capabilities & kCapability_ArtnetToLedstrip)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2LED");
					x += 100;
				}
				
				if (record.capabilities & kCapability_ArtnetToAnalogPin)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2Pin");
					x += 100;
				}
				
				if (record.capabilities & kCapability_TcpToI2S)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Tcp2I2S(2ch)");
					x += 100;
				}
				
				if (record.capabilities & kCapability_TcpToI2SQuad)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Tcp2I2S(4ch)");
					x += 100;
				}
				
				if (nodeState.test_tcpToI2SQuad.isActive)
				{
					setColor(colorGreen);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "(Playing)");
					x += 100;
				}
			}
		}
		framework.endDraw();
	}
	
	for (auto & nodeState : s_nodeStates)
	{
		nodeState.test_tcpToI2S.shut();
		nodeState.test_tcpToI2SQuad.shut();
	}
	
	discoveryProcess.shut();
	
	framework.shutdown();
	
	return 0;
}
