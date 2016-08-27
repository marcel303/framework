#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include "BezierTraveller.h"
#include "Bitmap.h"
#include "Calc.h"
#include "DataStream.h"
#include "Debugging.h"
#include "FileArchive.h"
#include "FileStream.h"
#include "Filter.h"
#include "HtmlTemplateEngine.h"
#include "Image.h"
#include "ImageLoader_Photoshop.h"
#include "ImageLoader_Tiff.h"
#include "KlodderSystem.h"
#include "Log.h"
#include "StreamReader.h"
#include "TestCode_iPhone.h"
#include "Timer.h"
#include "Util_Color.h"

#ifdef DEBUG

void DBG_TestFileArchive()
{
	int byteCount = 100;
	uint8_t* bytes = new uint8_t[byteCount];
	
	MemoryStream stream1;
	for (int i = 0; i < byteCount; ++i)
		bytes[i] = i;
	stream1.Write(bytes, byteCount);
	
	MemoryStream stream2;
	for (int i = 0; i < byteCount; ++i)
		bytes[i] = i + 100;
	stream2.Write(bytes, byteCount);
	
	FileArchive archive;
	
	archive.Add("stream1", &stream1);
	archive.Add("stream2", &stream2);
	
	MemoryStream stream;
	
	throw ExceptionNA();
//	archive.Save(&stream);
	
	stream.Seek(0, SeekMode_Begin);
	
	FileArchive archive2;
	
	archive2.Load(&stream);
	
	Stream* stream1b = archive2.GetStream("stream1");
	Stream* stream2b = archive2.GetStream("stream2");
	
	Assert(stream1b);
	Assert(stream2b);
	
	Assert(stream1b->Length_get() == byteCount);
	Assert(stream2b->Length_get() == byteCount);
	
	StreamReader reader1(stream1b, false);
	for (int i = 0; i < byteCount; ++i)
		Assert(reader1.ReadUInt8() == i);
	
	StreamReader reader2(stream2b, false);
	for (int i = 0; i < byteCount; ++i)
		Assert(reader2.ReadUInt8() == i + 100);
}

void DBG_TestTiff()
{
	ImageLoader_Tiff loader;
	
	Image image;
	
	image.SetSize(320, 480);
	
	for (int y = 0; y < image.m_Sy; ++y)
	{
		for (int x = 0; x < image.m_Sx; ++x)
		{
			int a = rand() % 128;
			int r = ((rand() % 256) * a) >> 8;
			int g = ((rand() % 256) * a) >> 8;
			int b = ((rand() % 256) * a) >> 8;
//			r = g = b = a = 128;
			image.GetLine(y)[x].r = r;
			image.GetLine(y)[x].g = g;
			image.GetLine(y)[x].b = b;
			image.GetLine(y)[x].a = a;
		}
	}
	
	std::string fileName = gSystem.GetDocumentPath("test.tiff");
	
	for (int i = 0; i < 3; ++i)
	{
		loader.Save(image, fileName);
	}
}

void DBG_TestPhotoshop()
{
	ImageLoader_Photoshop loader;
	
	Image image;
	
//	image.SetSize(320, 480);
	image.SetSize(64, 32);
	
#if 1
	for (int y = 0; y < image.m_Sy; ++y)
	{
		for (int x = 0; x < image.m_Sx; ++x)
		{
			int a = rand() % 128;
			int r = ((rand() % 256) * a) >> 8;
			int g = ((rand() % 256) * a) >> 8;
			int b = ((rand() % 256) * a) >> 8;
//			r = g = b = a = 128;
			image.GetLine(y)[x].r = r;
			image.GetLine(y)[x].g = g;
			image.GetLine(y)[x].b = b;
			image.GetLine(y)[x].a = a;
		}
	}
#endif
	
	std::string fileName1 = gSystem.GetResourcePath("test.psd");
	
	loader.Load(image, fileName1);
	
	std::string fileName = gSystem.GetDocumentPath("test.psd");
	
	for (int i = 0; i < 1; ++i)
	{
		loader.Save(image, fileName);
		loader.Load(image, fileName);
	}
}

static void RenderHtml(HtmlTemplateEngine& engine, std::string func, std::vector<std::string> args)
{
	if (func == "render")
	{
		if (args.size() != 1)
			throw ExceptionNA();
		
		if (args[0] == "content")
		{
			for (int i = 0; i < 10; ++i)
			{
				engine.SetKey("thumbnail", "test.png");
				engine.SetKey("link", "test.klodder");
				engine.SetKey("name", "Hello World!");
				engine.SetKey("date", "today");
				
				engine.IncludeResource("browser_picture.txt");
			}
		}
		else
			throw ExceptionNA();
	}
	else
		throw ExceptionNA();
}

void DBG_TestHtmlTemplate()
{
#ifndef DEBUG
	throw ExceptionNA();
#endif
	
	HtmlTemplateEngine engine(RenderHtml);
	
	std::string page = "<html><body>[!render:content]</body></html>";
	
	engine.Begin();
	engine.IncludeResource("browser_page.txt");
	
	std::string text = engine.ToString();
	
	LOG_DBG("%s", text.c_str());
}

void DBG_TestHslRgb()
{
#ifndef DEBUG
	throw ExceptionNA();
#endif

#if 0
	float rgb[3];
	float hsl[3];
	
	rgb[0] = 255 / 255.0f;
	rgb[1] = 128 / 255.0f;
	rgb[2] = 64 / 255.0f;
	
	for (int i = 0; i < 10; ++i)
	{
		float r = rgb[0];
		float g = rgb[1];
		float b = rgb[2];
		
		RgbToHsl(rgb, hsl);
		
		float h = hsl[0];
		float s = hsl[1];
		float l = hsl[2];
		
		HslToRgb(hsl, rgb);
		
		LOG_DBG("%1.3f %1.3f %1.3f -> %1.3f %1.3f %1.3f -> %1.3f %1.3f %1.3f\n",
				r * 255.0f, g * 255.0f, b * 255.0f,
				h * 360.0f, s * 100.0f, l * 100.0f,
				rgb[0] * 255.0f, rgb[1] * 255.0f, rgb[2] * 255.0f);
	}
#endif
}

void DBG_TestDataStream()
{
	FileStream stream;
	
	stream.Open(gSystem.GetDocumentPath("test.datastream").c_str(), OpenMode_Write);
	
	DataStreamWriter writer(&stream, false);
	
	for (int i = 0; i < 10; ++i)
	{
		int byteCount = Calc::Random(1000, 10000);
		char* bytes = new char[byteCount];
		for (int j = 0; j < byteCount; ++j)
			bytes[j] = j;
		
		LOG_DBG("writing segment: data_length=%d", byteCount);
		
		writer.WriteSegment("image_jpg", "image", bytes, byteCount, true);
	}
	
	stream.Close();
	
	stream.Open(gSystem.GetDocumentPath("test.datastream").c_str(), OpenMode_Read);
	
	DataStreamReader reader(&stream, false);
	
	while (!stream.EOF_get())
	{
		DataHeader header = reader.ReadHeader();
		
		LOG_DBG("data header: segment_position=%lu, segment_length=%lu, data_position=%lu, data_length=%lu", header.mStreamPosition, header.mSegmentLength, header.mDataStreamPosition, header.mDataLength);
		
		reader.SkipSegment(header);
	}
	
	stream.Close();
}

#endif

class ProfileScope
{
public:
	ProfileScope(const char * name)
		: m_name(name)
		, m_time(0)
	{
		m_time -= m_timer.TimeUS_get();
	}
	
	~ProfileScope()
	{
		m_time += m_timer.TimeUS_get();
		printf("time: %gms\n", m_time / 1000.0f);
	}
	
private:
	const char * m_name;
	Timer m_timer;
	uint64_t m_time;
};

void DBG_ProfileFilter()
{
	Filter filter;
	filter.Size_set(256, 256);
	for (int y = 0; y < filter.Sy_get(); ++y)
	{
		for (int x = 0; x < filter.Sx_get(); ++x)
		{
			filter.Line_get(y)[x] = 0.0f;
		}
	}
	
	{
		float offsetX = 0.2f;
		float offsetY = 0.6f;
		
		float weights[4];
		SampleAA_Prepare(offsetX, offsetY, weights);
		
		float v = 0.0f;
		
		{
			ProfileScope s("filter sample AA");
			
			for (uint32_t i = 0; i < 1000; ++i)
			{
				for (int y = 0; y < filter.Sy_get(); ++y)
				{
					for (int x = 0; x < filter.Sx_get(); ++x)
					{
						v += filter.SampleAA(x, y, weights);
					}
				}
			}
		}
		
		printf("sample result: %g\n", v);
	}
}

void DBG_ProfileBitmap()
{
	Bitmap bmp;
	bmp.Size_set(256, 256, true);
	
	{
		float offsetX = 0.2f;
		float offsetY = 0.6f;
		
		float weights[4];
		SampleAA_Prepare(offsetX, offsetY, weights);
		
		float v = 0.0f;
		
		{
			ProfileScope s("bitmap sample AA");
			
			for (uint32_t i = 0; i < 100; ++i)
			{
				for (int y = 0; y < bmp.Sy_get(); ++y)
				{
					for (int x = 0; x < bmp.Sx_get(); ++x)
					{
						Rgba rgba;
						bmp.SampleAA(x, y, rgba);
						v += rgba.rgb[0];
					}
				}
			}
		}
		
		printf("sample result: %g\n", v);
	}
	
	{
		float offsetX = 0.2f;
		float offsetY = 0.6f;
		
		float weights[4];
		SampleAA_Prepare(offsetX, offsetY, weights);
		
		float v = 0.0f;
		
		{
			ProfileScope s("bitmap sample AA (neon)");
			
			for (uint32_t i = 0; i < 100; ++i)
			{
				for (int y = 0; y < bmp.Sy_get(); ++y)
				{
					for (int x = 0; x < bmp.Sx_get(); ++x)
					{
#ifdef __ARM_NEON__
						float32_t p[2] = { x, y, };
						float32x2_t v_p = vld1_f32(p);
						Rgba rgba;
						bmp.SampleAA(v_p, rgba);
#else
						Rgba rgba;
						bmp.SampleAA(x, y, rgba);
#endif
						v += rgba.rgb[0];
					}
				}
			}
		}
		
		printf("sample result: %g\n", v);
	}
}

void DBG_ProfileMacImage()
{
}
