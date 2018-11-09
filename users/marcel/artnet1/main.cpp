#include "framework.h"
#include "ip/UdpSocket.h"

#define HOST_IP "192.168.1.220"
//#define HOST_IP "255.255.255.255"
#define HOST_PORT 6454

static const char * kArtnetID = "Art-Net";

enum ArtnetOpcode
{
	kArtnetOpcode_ArtDMX = 0x5000
};

enum ArtnetVersion
{
	kArtnetVersion_14 = 14
};

/*
DMX sequence number is 1..255. 0 means sequence numbers are disabled
*/

struct ArtnetPacket
{
	static const int kMaxSize = 2048;

	uint8_t data[kMaxSize];
	uint16_t dataSize = 0;

	template <typename T>
	static void write(uint8_t *& dst, const T & src)
	{
		memcpy(dst, &src, sizeof(src));
		dst += sizeof(src);
	}

	static void write(uint8_t *& dst, const void * src, const int srcSize)
	{
		memcpy(dst, src, srcSize);
		dst += srcSize;
	}

	void makeDMX512(const uint8_t sequence, const uint8_t physical, const uint16_t universe,
		const uint8_t * values, const int numValues)
	{
		uint8_t * p = data;

		const uint16_t opcode = kArtnetOpcode_ArtDMX;
		const uint16_t version = kArtnetVersion_14;
		const uint16_t length = numValues;

		const uint8_t opcodeLO = opcode & 0xff;
		const uint8_t opcodeHI = (opcode >> 8) & 0xff;
		const uint8_t versionHI = (version >> 8) & 0xff;
		const uint8_t versionLO = version & 0xff;
		const uint8_t universeLO = universe & 0xff;
		const uint8_t universeHI = (universe >> 8) & 0xff;
		const uint8_t lengthHI = (length >> 8) & 0xff;
		const uint8_t lengthLO = length & 0xff;

		write(p, kArtnetID, sizeof(kArtnetID));
		write(p, opcodeLO);
		write(p, opcodeHI);
		write(p, versionHI);
		write(p, versionLO);
		write(p, sequence);
		write(p, physical);
		write(p, universeLO);
		write(p, universeHI);
		write(p, lengthHI);
		write(p, lengthLO);
		write(p, values, numValues * sizeof(uint8_t));

		dataSize = p - data;
	}
};

int main(int argc, char * argv[])
{
	if (!framework.init(640, 480))
		return -1;

	IpEndpointName endpointName(HOST_IP, HOST_PORT);
	UdpTransmitSocket transmitSocket(endpointName);

	uint8_t sequence = 0x01;

	uint8_t values[512];
	memset(values, 0, sizeof(values));

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

		{
			SDL_Delay(20);

			ArtnetPacket packet;
			packet.makeDMX512(sequence * 0, 0, 0, values, 7);

			if (sequence == 0xff)
				sequence = 0x01;
			else
				sequence++;

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
			}
		}
		framework.endDraw();
	}

	return 0;
}