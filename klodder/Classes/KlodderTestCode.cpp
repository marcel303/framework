#include "ImageLoader_Photoshop.h"
#include "KlodderTestCode.h"

// test code
#include "BrushLibrary.h"
#include "BrushSettingsLibrary.h"
#include "FileStream.h"
#include "Image.h"
#include "ImageLoader_Tga.h"
#include "MouseMgr.h"
#if USE_QUICKTIME
	#include "QuickTimeEncoder.h"
#endif
#include "Settings.h"
#include "TravellerCapturer.h"
#include "XmlReader.h"
#include "XmlWriter.h"

void TestQuickTime()
{
#if USE_QUICKTIME
	MacImage image;

	image.Size_set(320, 240, true);

	QuickTimeEncoder encoder;

	encoder.Initialize(
		"test.mov",
		QtVideoCodec_H264,
		//QtVideoCodec_Jpeg2000,
		//QtVideoQuality_High,
		QtVideoQuality_Medium,
		&image);

	int frameMS = 1000 / 25;

	for (int i = 0; i < 1000; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			MacRgba& pixel = image.Line_get(rand() % image.Sy_get())[rand() % image.Sx_get()];

			pixel.rgba[0] = rand() & 255;
			pixel.rgba[1] = rand() & 255;
			pixel.rgba[2] = rand() & 255;
		}

		LOG_DBG("committing frame..", 0);

		encoder.CommitVideoFrame(frameMS);
	}

	encoder.Shutdown();
#endif
}

void TestXml()
{
	XmlWriter writer;

	FileStream stream;
	stream.Open("test.xml", OpenMode_Write);
	writer.Open(&stream, false);

	writer.BeginNode("settings");
	writer.BeginNode("item");
	writer.WriteAttribute("key", "value");
	writer.EndNode();
	writer.EndNode();

	writer.Close();
	stream.Close();

	XmlReader reader;

	stream.Open("test.xml", OpenMode_Read);
	reader.Load(&stream);

	XmlNode* node = reader.RootNode_get();

	for (size_t i = 0; i < node->mChildNodes.size(); ++i)
	{
		if (node->mChildNodes[i]->mName == "item")
		{
			for (std::map<std::string, std::string>::iterator j = node->mChildNodes[i]->mAttributes.begin(); j != node->mChildNodes[i]->mAttributes.end(); ++j)
			{
				LOG_DBG("%s -> %s", j->first.c_str(), j->second.c_str());
			}
		}
	}
}

void TestExportPsd()
{
	ImageLoader_Photoshop loader;

	Image image;

	loader.Load(image, "test.psd");

	image.SetSize(256, 256);

	for (int y = 0; y < image.m_Sy; ++y)
	{
		ImagePixel* line = image.GetLine(y);

		for (int x = 0; x < image.m_Sx; ++x)
		{
			if (y % 2 && 0)
			{
				int a = rand() % 256;
				int r = ((rand() % 256) * a) >> 8;
				int g = ((rand() % 256) * a) >> 8;
				int b = ((rand() % 256) * a) >> 8;

				line[x].rgba[0] = r;
				line[x].rgba[1] = g;
				line[x].rgba[2] = b;
				line[x].rgba[3] = a;
			}
			else
			{
				line[x].rgba[0] = x;
				line[x].rgba[1] = x;
				line[x].rgba[2] = x;
				line[x].rgba[3] = x;
			}
		}
	}

	//loader.SaveMultiLayer(&image, 1, "test_save.psd");
	loader.Save(image, "test_save.psd");

	loader.Load(image, "test_save.psd");
}
