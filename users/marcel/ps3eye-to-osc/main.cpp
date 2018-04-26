#include "Log.h"
#include "ps3eye.h"
#include "StringEx.h"
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

#define CAM_SX 320
#define CAM_SY 240
#define OSC_IMAGE_SX 8
#define OSC_IMAGE_SY 6

#define FPS 5

#include "framework.h"
#include "../../../libparticle/ui.h" // todo : rename

#include <atomic>
#include <string>

/*

todo :

- apply weighting/masking post process
- show masked/post processed image
- send weighed average pixel X/Y and average value over OSC
- run dot detector over image

*/

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

struct DownsampledImage
{
	static const int sx = OSC_IMAGE_SX;
	static const int sy = OSC_IMAGE_SY;
	
	float values[sy][sx];
	
	float average;
	
	void downsample(const uint8_t * frameData)
	{
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
						
						const int index = yi * CAM_SX + xi;
						
						sum += frameData[index];
					}
				}
				
				values[y][x] = sum / (20 * 20 * 255.f);
			}
		}
		
		// calculate average brightness
		
		average = 0.f;
		
		for (int y = 0; y < sy; ++y)
			for (int x = 0; x < sx; ++x)
				average += values[y][x];
		
		average /= (OSC_IMAGE_SX * OSC_IMAGE_SY);
	}
};

void sendCameraData(const uint8_t * frameData, OscSender & sender, const char * addressPrefix)
{
	// output : OSC_IMAGE_SX x OSC_IMAGE_SY values (float)
	
	DownsampledImage downsampledImage;
	
	downsampledImage.downsample(frameData);
	
	// send raw brightness data
	
	{
		char buffer[OSC_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
		
		char address[64];
		sprintf(address, "%s/raw", addressPrefix);
		
		p << osc::BeginMessage(address);
		{
			for (int y = 0; y < downsampledImage.sy; ++y)
				for (int x = 0; x < downsampledImage.sx; ++x)
					p << downsampledImage.values[y][x];
		}
		p << osc::EndMessage;
		
		sender.send(p.Data(), p.Size());
	}
	
	// send average brightness
	
	{
		char buffer[OSC_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
		
		char address[64];
		sprintf(address, "%s/average", addressPrefix);
		
		p << osc::BeginMessage(address);
		p << downsampledImage.average;
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
	
	int desiredFps;
	int currentFps;
	std::atomic<float> exposure;
	std::atomic<float> gain;
	
	Recorder()
		: quitRequested(false)
		, desiredFps(0)
		, currentFps(0)
		, exposure(1.f)
		, gain(0.f)
	{
	}
	
	~Recorder()
	{
		shut();
	}
	
	bool init(const int deviceIndex, const char * _oscAddressPrefix, const int fps)
	{
		oscAddressPrefix = _oscAddressPrefix;
		desiredFps = fps;
		currentFps = fps;
		
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
		
		const bool result = eye->init(CAM_SX, CAM_SY, fps, PS3EYECam::EOutputFormat::Gray);

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
			std::string oscAddressPrefix;
			
			SDL_LockMutex(s_controllerMutex);
			{
				oscAddressPrefix = self->oscAddressPrefix;
				
				if (self->desiredFps != self->currentFps)
				{
					self->currentFps = self->desiredFps;
					
					//
					
					logDebug("restarting camera stream");
					
					self->eye->stop();
					
					self->eye->setFrameRate(self->desiredFps);
					
					self->eye->start();
				}
			}
			SDL_UnlockMutex(s_controllerMutex);
			
			self->eye->setAutogain(false);
			self->eye->setAutoWhiteBalance(false);
			self->eye->setExposure(self->exposure * 255.f);
			self->eye->setGain(self->gain * 63.f);
			//self->eye->setAutogain(true);
			
			self->eye->getFrame(self->frameData);
			
			//LOG_DBG("got frame data!", 0);
			
			if ((n % 5) == 0)
			{
				sendCameraData(self->frameData, *s_oscSender, oscAddressPrefix.c_str());
			}

			n++;
		}
		
		return 0;
	}
};

struct Controller
{
	UiState uiState;
	
	Recorder * recorder1 = nullptr;
	Recorder * recorder2 = nullptr;
	
	int fps = 0;
	std::string oscEndpointIpAddress = "255.255.255.255";
	int oscEndpointUdpPort = 8000;
	
	std::string currentOscEndpointIpAddress;
	int currentOscEndpointUdpPort = 0;
	
	void init(Recorder * _recorder1, Recorder * _recorder2)
	{
		uiState.x = 10;
		uiState.y = 10;
		uiState.sx = 200;
		uiState.textBoxTextOffset = 100;
		
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
	
	void updateSettings(const int index, const int fps)
	{
		Recorder * recorder = getRecorder(index);
		
		recorder->desiredFps = fps;
	}
	
	void tickSharedMenu(const bool doTick, const bool doDraw, const float dt)
	{
		pushMenu("shared");
		{
			doTextBox(fps, "fps", dt);
			
			doTextBox(oscEndpointIpAddress, "OSC address", dt);
			doTextBox(oscEndpointUdpPort, "OSC port", dt);
			
			if (doTick)
			{
				updateSettings(0, fps);
				updateSettings(1, fps);
				
				// todo : check if OSC endpoint changed
				
				if (oscEndpointIpAddress != currentOscEndpointIpAddress || oscEndpointUdpPort != currentOscEndpointUdpPort)
				{
					currentOscEndpointIpAddress = oscEndpointIpAddress;
					currentOscEndpointUdpPort = oscEndpointUdpPort;
					
					//
					
					logDebug("reinitializing OSC sender");
					
					s_oscSender->shut();
					
					s_oscSender->init(oscEndpointIpAddress.c_str(), oscEndpointUdpPort);
				}
			}
		}
		popMenu();
	}
	
	void tickRecorderMenu(const int index, const bool doTick, const bool doDraw, const float dt)
	{
		char menuName[32];
		sprintf_s(menuName, sizeof(menuName), "cam%d", index);
		
		g_drawX = 10 + index * CAMVIEW_SX;
		g_drawY = GFX_SY - 10 - 30;
		
		pushMenu(menuName);
		{
			Recorder * recorder = getRecorder(index);
			
			if (doTick) SDL_LockMutex(s_controllerMutex);
			{
				doTextBox(recorder->oscAddressPrefix, "OSC prefix", dt);
			}
			if (doTick) SDL_UnlockMutex(s_controllerMutex);
		}
		popMenu();
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
	
	void tick(const float dt)
	{
		makeActive(&uiState, true, false);
		tickSharedMenu(true, false, dt);
		tickRecorderMenu(0, true, false, dt);
		tickRecorderMenu(1, true, false, dt);
		
		if (uiState.activeElem == nullptr)
		{
			tickCameraUi(0);
			tickCameraUi(1);
		}
	}
	
	void drawCameraUi(const int index)
	{
		Recorder * recorder = getRecorder(index);
		
		if (recorder != nullptr && recorder->frameData != nullptr)
		{
			GLuint texture = createTextureFromR8(recorder->frameData, CAM_SX, CAM_SY, false, true);
			
			if (texture != 0)
			{
				glBindTexture(GL_TEXTURE_2D, texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
				checkErrorGL();
				glBindTexture(GL_TEXTURE_2D, 0);
				checkErrorGL();

				gxSetTexture(texture);
				setColor(colorWhite);
				drawRect(0, 0, CAMVIEW_SX, CAMVIEW_SY);
				gxSetTexture(0);
				
				glDeleteTextures(1, &texture);
				texture = 0;
				checkErrorGL();
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
		gxPushMatrix();
		{
			drawCameraUi(0);
			gxTranslatef(CAMVIEW_SX, 0, 0);
			
			drawCameraUi(1);
			gxTranslatef(CAMVIEW_SX, 0, 0);
		}
		gxPopMatrix();
	
		makeActive(&uiState, false, true);
		tickSharedMenu(false, true, 0.f);
		tickRecorderMenu(0, false, true, 0.f);
		tickRecorderMenu(1, false, true, 0.f);
	}
};

void changeDirectory(const char * path);

int main(int argc, char * argv[])
{
	const char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
	
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

	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	Recorder recorder1;
	if (!recorder1.init(0, "/env/light", FPS) && false)
		return -1;
	
	Recorder recorder2;
	if (!recorder2.init(1, "/room/light", FPS) && false)
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
	
	OscSender sender;
	s_oscSender = &sender;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		controller.tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			controller.draw();
			
			popFontMode();
		}
		framework.endDraw();
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
