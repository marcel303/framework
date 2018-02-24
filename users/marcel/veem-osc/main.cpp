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

// OSC message history

struct OscMessageHistory
{
	struct Elem
	{
		std::string address;
		std::vector<float> values;
		std::vector<float> values_slow;
		uint64_t lastReceiveTime;
		
		void record(const std::vector<float> & _values)
		{
			values = _values;
		}
		
		void updateSlow(const float dt)
		{
			// make sure the arrays have the correct size
			
			while (values_slow.size() < values.size())
				values_slow.push_back(0.f);
			while (values_slow.size() > values.size())
				values_slow.pop_back();
			
			// update
			
			const float retain = std::pow(.1f, dt);
			
			for (size_t i = 0; i < values.size(); ++i)
			{
				const float oldValue = values_slow[i];
				const float newValue = values[i];
				
				values_slow[i] = oldValue * retain + newValue * (1.f - retain);
			}
		}
	};
	
	std::map<std::string, Elem> elems;
	
	Elem & getElem(const char * address)
	{
		return elems[address];
	}
};

static OscMessageHistory s_oscMessageHistory;

// OSC receiver

struct OscPacketListener : osc::OscPacketListener
{
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		SDL_LockMutex(s_mutex);
		{
			// todo : check address
			
			const char * address = m.AddressPattern();
			
			if (address != nullptr)
			{
				// todo : record value
				
				OscMessageHistory::Elem & elem = s_oscMessageHistory.getElem(address);
				
				std::vector<float> values;
				
				for (auto aItr = m.ArgumentsBegin(); aItr != m.ArgumentsEnd(); ++aItr)
				{
					auto & a = *aItr;
					
					const float value = a.IsFloat() ? a.AsFloat() : 0.f;
					
					values.push_back(value);
				}
				
				elem.record(values);
			}
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

//

struct OscSender
{
	UdpTransmitSocket * transmitSocket;
	
	void init(const char * ipAddress, const int udpPort)
	{
		transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
	}
	
	void shut()
	{
		if (transmitSocket != nullptr)
		{
			delete transmitSocket;
			transmitSocket = nullptr;
		}
	}
	
	void send(const void * data, const int dataSize)
	{
		if (transmitSocket != nullptr)
		{
			transmitSocket->Send((char*)data, dataSize);
		}
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
	
	OscSender * sender = new OscSender();
	sender->init("127.0.0.1", 9000);
	
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
		bool updateOsc = false;
		
		for (auto & event : framework.events)
		{
			if (event.type == s_timerEvent)
			{
				repaint = true;
				
				updateOsc = true;
			}
		}
		
		if (updateOsc)
		{
			SDL_LockMutex(s_mutex);
			{
				for (auto & elemItr : s_oscMessageHistory.elems)
				{
					auto & elem = elemItr.second;
					
					// todo : update slow-changing version
					
					elem.updateSlow(1.f);
					
					// todo : send slow-changing version
					
					// todo : update gradient version
				
					// todo : send gradient version
				}
			}
			SDL_UnlockMutex(s_mutex);
		}
		
		if (repaint)
		{
			OscMessageHistory history;
			
			SDL_LockMutex(s_mutex);
			{
				history = s_oscMessageHistory;
			}
			SDL_UnlockMutex(s_mutex);
			
			framework.beginDraw(rand() % 255, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				
				setColor(colorWhite);
				drawText(5, 5, 12, +1, +1, "OSC history:");
				
				popFontMode();
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
