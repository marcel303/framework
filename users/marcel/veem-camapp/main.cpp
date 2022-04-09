#include "Log.h"
#include "ps3eye.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

// for outbound OSC messages
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define GFX_SX (640*2)
#define GFX_SY (240*2)
#define CAMVIEW_SX (GFX_SX/2)
#define CAMVIEW_SY (GFX_SY)

#define OSC_BUFFER_SIZE 2048

#define FPS 5

#if defined(LINUX)
	#define DO_CONTROLLER 0
#else
	#define DO_CONTROLLER 1
#endif

#if DO_CONTROLLER
	#include "framework.h"
#else
	#include <SDL2/SDL.h>
#endif

#include <atomic>
#include <string>

using namespace ps3eye;

//

static SDL_mutex * s_controllerMutex = nullptr;

//

struct OscSender
{
	UdpSocket * transmitSocket;
	IpEndpointName remoteEndpoint;
	
	bool init(const char * ipAddress, const int udpPort)
	{
		try
		{
			transmitSocket = new UdpSocket();
			transmitSocket->SetEnableBroadcast(true);
			transmitSocket->Bind(IpEndpointName(IpEndpointName::ANY_ADDRESS, IpEndpointName::ANY_PORT));
			
			remoteEndpoint = IpEndpointName(ipAddress, udpPort);
			
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
				transmitSocket->SendTo(remoteEndpoint, (char*)data, dataSize);
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
			average += values[y][x];
	
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
		
		sender.send(p.Data(), p.Size());
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
		oscAddressPrefix = _oscAddressPrefix;
		
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
		
		const bool result = eye->init(320, 240, FPS, PS3EYECam::EOutputFormat::Gray);

		if (!result)
		{
			LOG_ERR("failed to initialize PS3 eye camera");
			shut();
			return false;
		}

		eye->start();
		
		const int sx = eye->getWidth();
		const int sy = eye->getHeight();
		
		frameData = new uint8_t[sx * sy];
		
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
		
		int n = 0;
		
		while (self->quitRequested == false)
		{
			self->eye->setAutogain(false);
			self->eye->setAutoWhiteBalance(false);
			self->eye->setExposure(self->exposure * 255.f);
			self->eye->setGain(self->gain * 63.f);
			//self->eye->setAutogain(true);
			
			self->eye->getFrame(self->frameData);
			
			//LOG_DBG("got frame data!");
			
			// note : we copy the OSC address prefix to ensure it's thread safe to use later
			//        it may change and become invalid due to ui interactions otherwise
			
			std::string oscAddressPrefix;
			
			SDL_LockMutex(s_controllerMutex);
			{
				oscAddressPrefix = self->oscAddressPrefix;
			}
			SDL_UnlockMutex(s_controllerMutex);
			
			if ((n % 5) == 0)
			{
				sendCameraData(self->frameData, *s_oscSender, oscAddressPrefix.c_str());
			}

			n++;
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
		framework.init(GFX_SX, GFX_SY);
		
		recorder1 = _recorder1;
		recorder2 = _recorder2;
		
		s_controllerMutex = SDL_CreateMutex();
	}
	
	void shut()
	{
		SDL_DestroyMutex(s_controllerMutex);
		s_controllerMutex = nullptr;
		
		framework.shutdown();
	}
	
	Recorder * getRecorder(const int index)
	{
		Recorder * recorder = nullptr;
		
		if (index == 0)
			recorder = recorder1;
		if (index == 1)
			recorder = recorder2;
		
		return recorder;
	}
	
	void tickCameraUi(const int index)
	{
		Recorder * recorder = getRecorder(index);
		
		const int x1 = (index + 0) * CAMVIEW_SX;
		const int y1 = 0;
		const int x2 = (index + 1) * CAMVIEW_SX;
		const int y2 = CAMVIEW_SY;
		
		const bool isInside =
			mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x < x2 &&
			mouse.y < y2;
		
		if (isInside && mouse.isDown(BUTTON_LEFT))
		{
			if (mouse.y < CAMVIEW_SY/2)
				recorder->exposure = (mouse.x - x1) / float(CAMVIEW_SX);
			else
				recorder->gain = (mouse.x - x1) / float(CAMVIEW_SX);
		}
	}
	
	bool tick()
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
			
		tickCameraUi(0);
		tickCameraUi(1);
		
		return !framework.quitRequested;
	}
	
	void drawCameraUi(const int index)
	{
		Recorder * recorder = getRecorder(index);
		
		if (recorder != nullptr && recorder->frameData != nullptr)
		{
			GxTexture texture;
			
			texture.allocate(320, 240, GX_R8_UNORM);
			
			if (texture.isValid())
			{
				texture.upload(recorder->frameData, 1, 0);
				texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);

				gxSetTexture(texture.id, GX_SAMPLE_NEAREST, true);
				setColor(colorWhite);
				drawRect(0, 0, CAMVIEW_SX, CAMVIEW_SY);
				gxClearTexture();
			}
		}
		
		if (recorder != nullptr)
		{
			setColor(0, 0, 255, 63);
			drawRect(0, 0, CAMVIEW_SX * recorder->exposure, CAMVIEW_SY/2);
		
			setColor(255, 0, 0, 63);
			drawRect(0, CAMVIEW_SY/2, CAMVIEW_SX * recorder->gain, CAMVIEW_SY);
		}
	}
	
	void draw()
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				drawCameraUi(0);
				gxTranslatef(CAMVIEW_SX, 0, 0);
				
				drawCameraUi(1);
				gxTranslatef(CAMVIEW_SX, 0, 0);
			}
			gxPopMatrix();
		}
		framework.endDraw();
	}
};

#else

struct Controller
{
	Recorder * recorder1 = nullptr;
	Recorder * recorder2 = nullptr;
	
	void init(Recorder * _recorder1, Recorder * _recorder2)
	{
		SDL_Init(0);
		
		s_controllerMutex = SDL_CreateMutex();
		
		recorder1 = _recorder1;
		recorder2 = _recorder2;
		
		printf("press 'q' to quit\n");
		printf("press 's' to swap cameras\n");
		
		printf("press 'i' and 'o' to change exposure of 1st camera\n");
		printf("press 'k' and 'l' to change gain of 1st camera\n");
		
		printf("press 'y' and 'u' to change exposure of 2nd camera\n");
		printf("press 'h' and 'j' to change gain of 2nd camera\n");
		
		printf("\nexposure: %.2f, gain: %.2f", (float)recorder1->exposure, (float)recorder1->gain);
		printf("\nexposure: %.2f, gain: %.2f", (float)recorder2->exposure, (float)recorder2->gain);
	}
	
	void shut()
	{
		SDL_DestroyMutex(s_controllerMutex);
		s_controllerMutex = nullptr;
		
		SDL_Quit();
	}
	
	bool tick()
	{
		system("/bin/stty raw");
		const int c = getchar();
		system("/bin/stty cooked");
		
		if (c == 's')
		{
			SDL_LockMutex(s_controllerMutex);
			{
				std::swap(recorder1->oscAddressPrefix, recorder2->oscAddressPrefix);
			}
			SDL_UnlockMutex(s_controllerMutex);
		}
		else if (c == 'q')
		{
			return false;
		}
		
		if (c == 'i')
		{
			recorder1->exposure = std::max(0.f, recorder1->exposure - .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder1->exposure, (float)recorder1->gain);
		}
		else if (c == 'o')
		{
			recorder1->exposure = std::min(1.f, recorder1->exposure + .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder1->exposure, (float)recorder1->gain);
		}
		else if (c == 'k')
		{
			recorder1->gain = std::max(0.f, recorder1->gain - .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder1->exposure, (float)recorder1->gain);
		}
		else if (c == 'l')
		{
			recorder1->gain = std::min(1.f, recorder1->gain + .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder1->exposure, (float)recorder1->gain);
		}
		
		if (c == 'y')
		{
			recorder2->exposure = std::max(0.f, recorder2->exposure - .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder2->exposure, (float)recorder2->gain);
		}
		else if (c == 'u')
		{
			recorder2->exposure = std::min(1.f, recorder2->exposure + .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder2->exposure, (float)recorder2->gain);
		}
		else if (c == 'h')
		{
			recorder2->gain = std::max(0.f, recorder2->gain - .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder2->exposure, (float)recorder2->gain);
		}
		else if (c == 'j')
		{
			recorder2->gain = std::min(1.f, recorder2->gain + .01f);
			printf("\nexposure: %.2f, gain: %.2f", (float)recorder2->exposure, (float)recorder2->gain);
		}
		
		return true;
	}
	
	void draw()
	{
	}
};

#endif

void changeDirectory(const char * path);

int main(int argc, char * argv[])
{
#if DO_CONTROLLER
	setupPaths(CHIBI_RESOURCE_PATHS);
#else
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

#if 1
	// show connected devices
	
	{
		auto devices = PS3EYECam::getDevices();
		
		for (auto & device : devices)
		{
			char identifier[1024];
			device->getUSBPortPath(identifier, sizeof(identifier));
			
			printf("found connected PS3 camera. identifier: %s", identifier);
		}
	}
#endif

	printf("veem-camapp. this app will output:\n");
	printf("OSC message /env/light for camera at index #0\n");
	printf("OSC message /room/light for camera at index #1\n");
	printf("see the veem OSC sheet for details\n");
	
	OscSender sender;
	if (!sender.init("255.255.255.255", 8000))
		return -1;
	s_oscSender = &sender;
	
	Recorder recorder1;
	if (!recorder1.init(0, "/env/light") && false)
		return -1;
	
	Recorder recorder2;
	if (!recorder2.init(1, "/room/light") && false)
		return -1;
	
	// load settings
	
	{
		tinyxml2::XMLDocument d;
		if (d.LoadFile("settings.xml") == tinyxml2::XML_SUCCESS)
		{
			auto settingsXml = d.FirstChildElement("settings");
			
			if (settingsXml)
			{
				auto recorder1Xml = settingsXml->FirstChildElement("recorder1");
				
				if (recorder1Xml)
				{
					recorder1.exposure = floatAttrib(recorder1Xml, "exposure", recorder1.exposure);
					recorder1.gain = floatAttrib(recorder1Xml, "gain", recorder1.gain);
				}
				
				auto recorder2Xml = settingsXml->FirstChildElement("recorder2");
				
				if (recorder2Xml)
				{
					recorder2.exposure = floatAttrib(recorder2Xml, "exposure", recorder2.exposure);
					recorder2.gain = floatAttrib(recorder2Xml, "gain", recorder2.gain);
				}
			}
		}
	}
	
	Controller controller;
	controller.init(&recorder1, &recorder2);
	
	for (;;)
	{
		if (!controller.tick())
			break;
		
		controller.draw();
	}
	
	// save settings
	
	{
		tinyxml2::XMLPrinter p;
		p.OpenElement("settings");
		{
			p.OpenElement("recorder1");
			{
				p.PushAttribute("exposure", recorder1.exposure);
				p.PushAttribute("gain", recorder1.gain);
			}
			p.CloseElement();
			
			p.OpenElement("recorder2");
			{
				p.PushAttribute("exposure", recorder2.exposure);
				p.PushAttribute("gain", recorder2.gain);
			}
			p.CloseElement();
		}
		p.CloseElement();
		
		tinyxml2::XMLDocument d;
		d.Parse(p.CStr());
		d.SaveFile("settings.xml");
	}
	
	controller.shut();
	
	recorder1.shut();
	recorder2.shut();
	
	sender.shut();
	
	return 0;
}
