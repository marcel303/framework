#include "artnet.h"
#include "framework.h"
#include "ip/UdpSocket.h"

#define HOST_IP "192.168.1.220"
#define HOST_PORT 6454

static void doSpring(const float mouseX, const bool mouseDown, float & value, float & velocity, float & center, const float stiffness, const float retain, const float dt)
{
	if (mouseDown)
	{
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			value = mouseX;
		else
			center = mouseX;
	}
	
	const float delta = value - center;
	const float force = - stiffness * delta;
	velocity += force * dt;
	value += velocity * dt;
	velocity *= powf(retain, dt);
}

int main(int argc, char * argv[])
{
	if (!framework.init(640, 480))
		return -1;

	IpEndpointName endpointName(HOST_IP, HOST_PORT);
	UdpTransmitSocket transmitSocket(endpointName);

	//uint8_t sequence = 0x01;
	uint8_t sequence = 0x00;

	uint8_t values[512];
	memset(values, 0, sizeof(values));

	float springValues[512];
	float springVelocities[512];
	float springCenters[512];
	memset(springValues, 0, sizeof(springValues));
	memset(springVelocities, 0, sizeof(springVelocities));
	memset(springCenters, 0, sizeof(springCenters));
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		if (mouse.wentDown(BUTTON_RIGHT))
		{
			const int offset = (framework.time * 10.f);

			for (int i = 0; i < 512; ++i)
				values[i] = i + offset;
		}

		if (mouse.isDown(BUTTON_LEFT))
		{
			const int channel = mouse.y / 30;
			if (channel >= 0 && channel < 512)
				values[channel] = mouse.x * 255 / 640;
		}
		if (keyboard.isDown(SDLK_1))
		{
			for (int i = 0; i < 512; ++i)
				values[i] = (128 + 127 * sinf(framework.time / (i + 1.f))) / (i + 1.f);
		}
		
		for (int i = 0; i < 512; ++i)
		{
			const int y = i * 30;
			
			const bool mouseDown = mouse.isDown(BUTTON_LEFT) && (mouse.y >= y && mouse.y < y + 30);
			
			for (int n = 0; n < 10; ++n)
			{
				doSpring(
					mouse.x / float(640), mouseDown,
					springValues[i], springVelocities[i], springCenters[i],
					40.f, .5f,
					framework.timeStep / 10.f);
			}
			
			values[i] = clamp(int(springValues[i] * 255.f), 0, 255);
		}

		{
			SDL_Delay(25);

			ArtnetPacket packet;
			packet.makeDMX512(sequence, 0, 0, values, 3);

			if (sequence != 0x00)
			{
				if (sequence == 0xff)
					sequence = 0x01;
				else
					sequence++;
			}

			transmitSocket.Send((const char*)packet.data, packet.dataSize);
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			for (int i = 0; i < 512; ++i)
			{
				const int y = i * 30;

				if (y >= 480)
					break;

				setColor(colorWhite);
				setLumi(values[i]);
				drawRect(0, y, 640, y + 30);
				setLumi(100);
				drawRectLine(0, y, 640, y + 30);
				
			#if 1
				setLumi(255);
				drawRect(springCenters[i] * 640 - 10, y, springCenters[i] * 640 + 10, y + 30);
				setLumi(40);
				drawRect(springValues[i] * 640 - 7, y, springValues[i] * 640 + 7, y + 30);
			#endif
			}
		}
		framework.endDraw();
	}

	return 0;
}
