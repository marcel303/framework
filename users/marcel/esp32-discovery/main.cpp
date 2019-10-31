#include "artnet.h"
#include "framework.h"
#include "ip/PacketListener.h"
#include "ip/UdpSocket.h"
#include "StringEx.h"
#include <vector>

/*

esp32 discovery process relies on the Arduino sketch 'esp32-wifi-configure'.
This is a sketch which lets the user select a Wifi access point and connect to it. The sketch will then proceed sending discovery messages at a regular interval. The discovery message containts the id of the device, and the IP address can be inferred from the received UDP packet.

*/

#define DISCOVERY_RECEIVE_PORT 2400
#define DISCOVERY_ID_STRING_SIZE 16

#define ARTNET_PORT 6454

struct DiscoveryRecord
{
	uint64_t id;
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
		
		if (size != sizeof(uint64_t))
		{
			logWarning("received invalid discovery message");
			return;
		}
		
		const uint64_t id = *(uint64_t*)data;
		
		bool exists = false;
		
		for (auto & record : s_discoveryRecords)
		{
			if (record.id == id)
				exists = true;
		}
		
		if (exists == false)
		{
			logDebug("found a new node! id=%llx", id);
			
			lock();
			{
				DiscoveryRecord record;
				record.id = id;
				record.endpointName = remoteEndpoint;
				
				s_discoveryRecords.push_back(record);
			}
			unlock();
		}
	}
};

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
	//artnetSocket.
	
	for (;;)
	{
		framework.process();

		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		if (framework.quitRequested)
			break;

		// send some artnet data to discovered nodes
		
		{
			ArtnetPacket packet;
			
			packet.makeDMX512(4);
			
			const int numRecords = discoveryProcess.getRecordCount();
			
			for (int i = 0; i < numRecords; ++i)
			{
				auto record = discoveryProcess.getDiscoveryRecord(i);
				
				IpEndpointName remoteEndpoint(record.endpointName.address, ARTNET_PORT);
				
				for (int i = 0; i < 2; ++i)
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
				
				char endpointName[IpEndpointName::ADDRESS_STRING_LENGTH];
				record.endpointName.AddressAsString(endpointName);
				
				const int sy = 18;
				
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
				
				if (isClicked)
				{
					char command[128];
					sprintf_s(command, sizeof(command), "open http://%s", endpointName);
					system(command);
				}
				
				setColor(colorGreen);
				drawText(x1 + 10, (y1 + y2) / 2.f, 14, +1, 0, "%s", endpointName);
				setColor(colorWhite);
				drawText(x1 + 100, (y1 + y2) / 2.f, 14, +1, 0, "%llx", record.id);
			}
		}
		framework.endDraw();
	}
	
	discoveryProcess.shut();
	
	framework.shutdown();
	
	return 0;
}
