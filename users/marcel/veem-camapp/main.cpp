#include "Log.h"
#include "ps3eye.h"

// for outbound OSC messages
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define GFX_SX 640
#define GFX_SY 240

#define OSC_BUFFER_SIZE 2048

#define DO_CONTROLLER 1

#if DO_CONTROLLER
	#include "framework.h"
#else
	#include <SDL2/SDL.h>
#endif

#include <atomic>
#include <string>

using namespace ps3eye;

//

struct OscSender
{
	UdpTransmitSocket * transmitSocket;
	
	bool init(const char * ipAddress, const int udpPort)
	{
		try
		{
			transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
			
			return true;
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to create UDP transmit socket: %s", e.what());
			return false;
		}
	}
	
	void shut()
	{
		if (transmitSocket != nullptr)
		{
			try
			{
				delete transmitSocket;
				transmitSocket = nullptr;
			}
			catch (std::exception & e)
			{
				LOG_DBG("%s", e.what());
			}
		}
	}
	
	void send(const void * data, const int dataSize)
	{
		if (transmitSocket != nullptr)
		{
			try
			{
				transmitSocket->Send((char*)data, dataSize);
			}
			catch (std::exception & e)
			{
				LOG_DBG("failed to send UDP packet: %s", e.what());
			}
		}
	}
};

static OscSender * s_oscSender = nullptr;

//

void sendCameraData(const uint8_t * frameData, OscSender & sender, const char * addressPrefix)
{
	// output : 16 x 12 values (float)
	
	const int sx = 16;
	const int sy = 12;
	
	float values[sy][sx];
	
	for (int y = 0; y < sy; ++y)
	{
		for (int x = 0; x < sx; ++x)
		{
			int sum = 0;
			
			for (int ry = 0; ry < 20; ++ry)
			{
				for (int rx = 0; rx < 20; ++rx)
				{
					const int yi = y * 20 + ry;
					const int xi = x * 20 + rx;
					
					const int index = yi * 320 + xi;
					
					sum += frameData[index];
				}
			}
			
			values[y][x] = sum / (20 * 20 * 255.f);
		}
	}
	
	// send raw brightness data
	
	{
		char buffer[OSC_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
		
		char address[64];
		sprintf(address, "%s/raw", addressPrefix);
		
		p << osc::BeginMessage(address);
		{
			for (int y = 0; y < sy; ++y)
				for (int x = 0; x < sx; ++x)
					p << values[y][x];
		}
		p << osc::EndMessage;
		
		sender.send(p.Data(), p.Size());
	}
	
	// calculate average brightness
	
	float average = 0.f;
	
	for (int y = 0; y < sy; ++y)
		for (int x = 0; x < sx; ++x)
			average += values[x][y];
	
	average /= (16 * 12);
	
	// send average brightness
	
	{
		char buffer[OSC_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
		
		char address[64];
		sprintf(address, "%s/average", addressPrefix);
		
		p << osc::BeginMessage(address);
		p << average;
		p << osc::EndMessage;
		
		//sender.send(p.Data(), p.Size());
	}
}

struct Recorder
{
	PS3EYECam::PS3EYERef eye;
	
	uint8_t * frameData = nullptr;
	
	std::string oscAddressPrefix;
	
	SDL_Thread * receiveThread = nullptr;
	
	std::atomic<bool> quitRequested;
	
	std::atomic<float> exposure;
	std::atomic<float> gain;
	
	Recorder()
		: quitRequested(false)
		, exposure(1.f)
		, gain(0.f)
	{
	}
	
	~Recorder()
	{
		shut();
	}
	
	bool init(const int deviceIndex, const char * _oscAddressPrefix)
	{
		auto devices = PS3EYECam::getDevices();
		
		LOG_INF("found %d devices", devices.size());
		
		if (deviceIndex >= devices.size())
		{
			LOG_ERR("no PS3 eye camera connected at index=%d", deviceIndex);
			shut();
			return false;
		}
		
		eye = devices[deviceIndex];
		devices.clear();
		
		const bool result = eye->init(320, 240, 60, PS3EYECam::EOutputFormat::Gray);

		if (!result)
		{
			LOG_ERR("failed to initialize PS3 eye camera", 0);
			shut();
			return false;
		}

		eye->start();
		
		const int sx = eye->getWidth();
		const int sy = eye->getHeight();
		
		frameData = new uint8_t[sx * sy];
		
		oscAddressPrefix = _oscAddressPrefix;
		
		receiveThread = SDL_CreateThread(receiveThreadProc, "PS3 Eye Camera", this);
		
		return true;
	}
	
	void shut()
	{
		if (receiveThread != nullptr)
		{
			quitRequested = true;
			
			SDL_WaitThread(receiveThread, nullptr);
			receiveThread = nullptr;
			
			quitRequested = false;
		}
		
		if (frameData != nullptr)
		{
			delete [] frameData;
			frameData = nullptr;
		}
		
		if (eye != nullptr)
		{
			eye->stop();
			eye = nullptr;
		}
	}
	
	static int receiveThreadProc(void * obj)
	{
		auto self = (Recorder*)obj;
		
		while (self->quitRequested == false)
		{
			self->eye->setAutogain(false);
			self->eye->setAutoWhiteBalance(false);
			self->eye->setExposure(self->exposure * 255.f);
			self->eye->setGain(self->gain * 63.f);
			//self->eye->setAutogain(true);
			
			self->eye->getFrame(self->frameData);
			
			//LOG_DBG("got frame data!", 0);
			
			// todo : make thread safe
			
			sendCameraData(self->frameData, *s_oscSender, self->oscAddressPrefix.c_str());
		}
		
		return 0;
	}
};

#if DO_CONTROLLER

struct Controller
{
	Recorder * recorder1;
	Recorder * recorder2;
	
	void init(Recorder * _recorder1, Recorder * _recorder2)
	{
		framework.init(0, nullptr, GFX_SX, GFX_SY);
		
		recorder1 = _recorder1;
		recorder2 = _recorder2;
	}
	
	void shut()
	{
		framework.shutdown();
	}
	
	bool tick()
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
			
		if (mouse.isDown(BUTTON_LEFT))
		{
			if (mouse.y < 120)
				recorder1->exposure = mouse.x / 320.f;
			else
				recorder1->gain = mouse.x / 320.f;
		}
		
		return !framework.quitRequested;
	}
	
	void drawCamera(const int index)
	{
		Recorder * recorder = nullptr;
		
		if (index == 0)
			recorder = recorder1;
		if (index == 1)
			recorder = recorder2;
		
		if (recorder != nullptr && recorder->frameData != nullptr)
		{
			GLuint texture = createTextureFromR8(recorder->frameData, 320, 240, false, true);
			
			glBindTexture(GL_TEXTURE_2D, texture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			checkErrorGL();
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			gxSetTexture(texture);
			setColor(colorWhite);
			drawRect(0, 0, 320, 240);
			gxSetTexture(0);
			
			glDeleteTextures(1, &texture);
			texture = 0;
		}
	}
	
	void draw()
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			{
				gxPushMatrix();
				{
					drawCamera(0);
					gxTranslatef(320, 0, 0);
					
					drawCamera(1);
					gxTranslatef(320, 0, 0);
				}
				gxPopMatrix();

				setColor(0, 0, 255, 63);
				drawRect(0, 0, 320 * recorder1->exposure, 120);
				
				setColor(255, 0, 0, 63);
				drawRect(0, 120, 320 * recorder1->gain, 240);
			}
			popFontMode();
		}
		framework.endDraw();
	}
};

#else

struct Controller
{
	void init(Recorder * _recorder1, Recorder * _recorder2)
	{
		SDL_Init(0);
	}
	
	void shut()
	{
		SDL_Quit();
	}
	
	bool tick()
	{
		return true;
	}
	
	void draw()
	{
	}
};

#endif

int main(int argc, char * argv[])
{
	OscSender sender;
	if (!sender.init("255.255.255.255", 8000))
		return -1;
	s_oscSender = &sender;
	
	Recorder recorder1;
	if (!recorder1.init(0, "/env/light"))
		return -1;
	
	Recorder recorder2;
	if (!recorder2.init(1, "/room/light") && false)
		return -1;
	
	Controller controller;
	controller.init(&recorder1, &recorder2);
	
	for (;;)
	{
		if (!controller.tick())
			break;
		
		controller.draw();
	}
	
	recorder1.shut();
	recorder2.shut();
	
	sender.shut();
	
	return 0;
}
