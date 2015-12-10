#include "Calc.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "image.h"

#ifdef WIN32
	#pragma comment(lib, "ws2_32.lib")
#endif

#define IMAGE "image.png"
#define ADDRESS "127.0.0.1"
#define PORT 1121
#define OUTPUT_BUFFER_SIZE 1024

std::string filename;
ImageData * image = 0;

static void handleAction(const std::string & action, const Dictionary & args);

static void reinit(const char * _filename)
{
	delete image;

	//

	filename = _filename;
	image = loadImage(_filename);

	framework.shutdown();
	framework.windowTitle = "OSC test";
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	framework.init(0, 0, image->sx, image->sy);
}

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		reinit(args.getString("file", "").c_str());
	}
}

int main(int argc, char* argv[])
{
	if (!framework.init(0, 0, 640, 460))
		return -1;

	UdpTransmitSocket transmitSocket(IpEndpointName(ADDRESS, PORT));

	reinit(IMAGE);

	float sample[2] = { 0.f, 0.f };
	float speed[2] = { 11.11f, 3.33f };
	speed[0] /= 3.f;
	speed[1] /= 3.f;

	while (!keyboard.wentDown(SDLK_ESCAPE))
	{
		// update input

		framework.process();

		// update logic

		const int size[2] = { image->sx, image->sy };

		for (int i = 0; i < 2; ++i)
		{
			sample[i] += speed[i];

			if (sample[i] < 0.f)
			{
				sample[i] = 0.f;
				speed[i] *= -1.f;
			}
			if (sample[i] > size[i])
			{
				sample[i] = size[i];
				speed[i] *= -1.f;
			}
		}

		if (mouse.isDown(BUTTON_LEFT))
		{
			const int dx = mouse.x - sample[0];
			const int dy = mouse.y - sample[1];

			sample[0] = mouse.x;
			sample[1] = mouse.y;

			if (dx != 0)
				speed[0] = dx / 10.f;
			if (dy != 0)
				speed[1] = dy / 10.f;
		}

		const int pixelX = Calc::Clamp((int)sample[0], 0, size[0] - 1);
		const int pixelY = Calc::Clamp((int)sample[1], 0, size[1] - 1);
		const int pixelIndex = pixelX + (size[1] - 1 - pixelY) * size[0];
		const ImageData::Pixel & pixel = image->imageData[pixelIndex];

		const int freq1 = pixel.r;
		const int freq2 = pixel.g;
		const int freq3 = pixel.b;

		// update network

		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

		p
			<< osc::BeginBundleImmediate
			<< osc::BeginMessage("/chat")
			<< freq1 << freq2 << freq3 << 4
			<< osc::EndMessage
			<< osc::EndBundle;

		transmitSocket.Send(p.Data(), p.Size());

		// draw

		framework.beginDraw(0, 0, 0, 0);
		{
			const int kBoxSize = 3;

			setColor(colorWhite);
			Sprite(filename.c_str()).draw();

			setColor(colorGreen);
			drawRect(pixelX - kBoxSize, pixelY - kBoxSize, pixelX + kBoxSize, pixelY + kBoxSize);
		}
		framework.endDraw();
	}

	return 0;
}
