#include "Log.h"
#include "ps3eye.h"

// for outbound OSC messages
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define OSC_BUFFER_SIZE 2048

#define DO_CONTROLLER 1

#if DO_CONTROLLER
	#include "framework.h"
#endif

using namespace ps3eye;

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

//

void sendCameraData(const uint8_t * frameData, OscSender & sender)
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
		
		p << osc::BeginMessage("/env/light/raw");
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
		
		p << osc::BeginMessage("/env/light/average");
		p << average;
		p << osc::EndMessage;
		
		//sender.send(p.Data(), p.Size());
	}
}

#if DO_CONTROLLER

struct Controller
{
	void init()
	{
		framework.init(0, nullptr, 320, 240);
	}
	
	void shut()
	{
		framework.shutdown();
	}
	
	void tick()
	{
		framework.process();
		
		const float dt = framework.timeStep;
	}
	
	void draw(const uint8_t * frameData)
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			{
				GLuint texture = createTextureFromR8(frameData, 320, 240, false, true);
				
				glBindTexture(GL_TEXTURE_2D, texture);
				GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
				checkErrorGL();
				glBindTexture(GL_TEXTURE_2D, 0);
				checkErrorGL();
	
				gxSetTexture(texture);
				drawRect(0, 0, 320, 240);
				gxSetTexture(0);
				
				glDeleteTextures(1, &texture);
				texture = 0;
			}
			popFontMode();
		}
		framework.endDraw();
	}
};

#endif

int main(int argc, char * argv[])
{
	Controller controller;
	controller.init();
	
	OscSender sender;
	sender.init("255.255.255.255", 8000);
	
	PS3EYECam::PS3EYERef eye;
	
	auto devices = PS3EYECam::getDevices();
	
	LOG_INF("found %d devices", devices.size());
	
	if (devices.empty())
	{
		LOG_ERR("no PS3 eye camera connected", 0);
		return -1;
	}
	
	eye = devices[0];
	devices.clear();
	
	const bool result = eye->init(320, 240, 2, PS3EYECam::EOutputFormat::Gray);

	if (!result)
	{
		LOG_ERR("failed to initialize PS3 eye camera", 0);
		return -1;
	}

	eye->start();

	const int sx = eye->getWidth();
	const int sy = eye->getHeight();

	uint8_t * frameData = new uint8_t[sx * sy];

	for (;;)
	{
		eye->setAutogain(false);
		eye->setAutoWhiteBalance(false);
		//eye->setExposure(0);
		
		eye->getFrame(frameData);
		
		LOG_DBG("got frame data!", 0);
		
		sendCameraData(frameData, sender);
		
		controller.tick();
		
		controller.draw(frameData);
	}

	delete [] frameData;
	frameData = nullptr;

	eye->stop();
	eye = nullptr;
	
	sender.shut();
	
	return 0;
}
