#include "framework.h"
#include "Log.h"
#include <atomic>

// for inbound OSC messages
#include "osc/OscPacketListener.h"

// for outbound OSC messages
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

const int GFX_SX = 1000;
const int GFX_SY = 800;

std::atomic<bool> s_quitRequested(false);

static SDL_mutex * s_mutex = nullptr;

// OSC receiver

struct OscPacketListener : osc::OscPacketListener
{
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		SDL_LockMutex(s_mutex);
		{
			// todo : check address
			
			// todo : record value
			
			// todo : update slow-changing version
			
			// todo : send slow-changing version
			
			// todo : update gradient version
			
			// todo : send gradient version
		}
		SDL_UnlockMutex(s_mutex);
	}
};

// IpEndpointName::ANY_ADDRESS

struct OscReceiver
{
	OscPacketListener * packetListener = nullptr;
	UdpListeningReceiveSocket * receiveSocket = nullptr;
	
	SDL_Thread * receiveThread = nullptr;
	
	~OscReceiver()
	{
		shut();
	}
	
	void init(const char * ipAddress, const int udpPort)
	{
		packetListener = new OscPacketListener();
	
		receiveSocket = new UdpListeningReceiveSocket(IpEndpointName(ipAddress, udpPort), packetListener);
		
		receiveThread = SDL_CreateThread(receiveThreadProc, "OSC Receive", this);
	}
	
	void shut()
	{
		LOG_DBG("terminating OSC receive thread", 0);
		
		if (receiveSocket != nullptr)
		{
			receiveSocket->AsynchronousBreak();
		}
		
		if (receiveThread != nullptr)
		{
			SDL_WaitThread(receiveThread, nullptr);
			receiveThread = nullptr;
		}
		
		LOG_DBG("terminating OSC receive thread [done]", 0);
		
		LOG_DBG("terminating OSC UDP receive socket", 0);
		
		delete receiveSocket;
		receiveSocket = nullptr;
		
		LOG_DBG("terminating OSC UDP receive socket [done]", 0);
		
		delete packetListener;
		packetListener = nullptr;
	}
	
	static int receiveThreadProc(void * obj)
	{
		OscReceiver * self = (OscReceiver*)obj;
		
		self->receiveSocket->Run();
		
		return 0;
	}
};

// timer event

static int s_timerEvent = -1;

static SDL_Thread * s_timerThread = nullptr;

static int timerThreadProc(void * obj)
{
	while (!s_quitRequested)
	{
		SDL_Event e;
		e.type = s_timerEvent;
		SDL_PushEvent(&e);
		
		SDL_Delay(1000);
	}
	
	return 0;
}

//

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	s_mutex = SDL_CreateMutex();
	
	OscReceiver * receiver = new OscReceiver();
	receiver->init("127.0.0.1", 8000);
	
	s_timerEvent = SDL_RegisterEvents(1);
	
	s_timerThread = SDL_CreateThread(timerThreadProc, "Timer", nullptr);
	
	framework.waitForEvents = true;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		bool repaint = false;
		
		for (auto & event : framework.events)
			if (event.type == s_timerEvent)
				repaint = true;
		
		if (repaint)
		{
			framework.beginDraw(rand() % 255, 0, 0, 0);
			{
			
			}
			framework.endDraw();
		}
	}
	
	s_quitRequested = true;
	
	SDL_WaitThread(s_timerThread, nullptr);
	
	delete receiver;
	receiver = nullptr;
	
	SDL_DestroyMutex(s_mutex);
	s_mutex = nullptr;
	
	framework.shutdown();
	
	return 0;
}
