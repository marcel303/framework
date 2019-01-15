#include "audio.h"
#include "framework.h"
#include "objects/paobject.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

const int GFX_SX = 800;
const int GFX_SY = 300;

struct Region
{
	int begin;
	int end;
};

struct FileInfo
{
	std::vector<Region> regions;
	
	void saveXml(tinyxml2::XMLPrinter & p)
	{
		for (auto & r : regions)
		{
			p.OpenElement("region");
			{
				p.PushAttribute("begin", r.begin);
				p.PushAttribute("end", r.end);
			}
			p.CloseElement();
		}
	}
	
	void loadXml(tinyxml2::XMLElement * e)
	{
		for (auto regionXml = e->FirstChildElement("region"); regionXml != nullptr; regionXml = regionXml->NextSiblingElement("region"))
		{
			Region r;
			
			r.begin = intAttrib(regionXml, "begin", 0);
			r.end = intAttrib(regionXml, "end", 0);
			
			regions.push_back(r);
		}
	}
};

static void exportFragments(const SoundData * soundData, const std::vector<Region> & regions, const char * format)
{
	int index = 0;
	
	for (auto & region : regions)
	{
		const std::string filename = String::FormatC(format, index);
		
		// extract the region
		
		const int begin = std::max(0, region.begin);
		const int end = std::min(soundData->sampleCount, region.end);
		const int size = end - begin;
		
		FILE * file = fopen(filename.c_str(), "wb");
		
		if (file != nullptr)
		{
			if (size > 0)
			{
				fwrite(soundData->sampleData, soundData->channelSize * soundData->channelCount, size, file);
			}
			
			fclose(file);
			file = nullptr;
		}
		
		index++;
	}
}

static SoundData * s_soundData = nullptr;;

struct MyPortAudioHandler : PortAudioHandler
{
	std::atomic<int> position;
	
	MyPortAudioHandler()
		: position(0)
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer)
	{
		const int frameSize = s_soundData->channelCount * sizeof(float);
		
		memset(outputBuffer, 0, framesPerBuffer * frameSize);
		
		if (s_soundData->channelSize == 4)
		{
			const void * samples = (uint8_t*)s_soundData->sampleData + position * frameSize;
			
			const int count = std::min(framesPerBuffer, s_soundData->sampleCount - position);
			
			if (count > 0)
			{
				memcpy(outputBuffer, samples, count * frameSize);
			}
		}
		
		position += framesPerBuffer;
	}
};

static MyPortAudioHandler * s_paHandler = nullptr;

static bool commandMod()
{
	return keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
}

enum State
{
	kState_Idle,
	kState_DecideDrawOrClick,
	kState_DrawRegion,
	kState_DragRegion
};

static State s_state = kState_Idle;

static int s_selectedRegionIndex = -1;

static int s_sampleIndex1 = 0;
static int s_sampleIndex2 = 0;

static int decideOrClickLocation = 0;

static bool s_dragL = false;
static bool s_dragR = false;

static void doEditor(const SoundData * soundData, FileInfo & fi)
{
	const int kMoveAmount = 512;
	
	const Mat4x4 sampleToView =
		Mat4x4(true)
		.Scale(GFX_SX, GFX_SY, 1)
		.Scale(1.f / (s_sampleIndex2 - s_sampleIndex1), .5f, 1)
		.Translate(-s_sampleIndex1, 1, 0);
	
	const Mat4x4 viewToSample = sampleToView.Invert();
	
	gxPushMatrix();
	{
		gxMultMatrixf(sampleToView.m_v);
		
		setColor(colorWhite);
		
		if (soundData->channelSize == 4)
		{
			const float * samples = (float*)soundData->sampleData;
			
			gxBegin(GL_POINTS);
			{
				const int skip = std::max(1, (s_sampleIndex2 - s_sampleIndex1) / GFX_SX);
				
				for (int i = s_sampleIndex1; i < s_sampleIndex2; i += skip)
				{
					const float value = samples[i * soundData->channelCount] * 4.f;
					
					drawPoint(i, value);
				}
			}
			gxEnd();
		}
		
		for (size_t i = 0; i < fi.regions.size(); ++i)
		{
			auto & r = fi.regions[i];
			
			setColor(colorBlue);
			if (i == s_selectedRegionIndex)
				setAlpha(200);
			else
				setAlpha(100);
			
			drawRect(r.begin, -1.f, r.end, +1.f);
			
			if (i == s_selectedRegionIndex)
			{
				setColor((s_dragL && s_dragR) ? colorBlue : colorWhite);
				if (s_dragL)
					drawLine(r.begin, -1, r.begin, +1);
				if (s_dragR)
					drawLine(r.end, -1, r.end, +1);
			}
		}
		
		setColor(colorWhite);
		gxBegin(GL_LINES);
		{
			const int position = s_paHandler->position;
			
			gxVertex2f(position, -1.f);
			gxVertex2f(position, +1.f);
		}
		gxEnd();
	}
	gxPopMatrix();
	
	switch (s_state)
	{
	case kState_Idle:
		{
			if (keyboard.wentDown(SDLK_c))
			{
				if (s_sampleIndex1 == 0) // fixme : remember mode
				{
					const int kRegionSize = soundData->sampleRate * 2;
					
					const Vec2 p = viewToSample.Mul4(Vec2(mouse.x, mouse.y));
					
					s_sampleIndex1 = p[0] - kRegionSize/2;
					s_sampleIndex2 = p[0] + kRegionSize/2;
				}
				else
				{
					s_sampleIndex1 = 0;
					s_sampleIndex2 = s_soundData->sampleCount;
				}
			}
			
			if (keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
			{
				if (s_selectedRegionIndex != -1)
				{
					auto i = fi.regions.begin() + s_selectedRegionIndex;
					
					fi.regions.erase(i);
					
					s_selectedRegionIndex = -1;
				}
			}
			
			if (keyboard.wentDown(SDLK_LEFT, true) && s_selectedRegionIndex != -1)
			{
				auto & r = fi.regions[s_selectedRegionIndex];
				
				r.begin -= kMoveAmount;
				r.end -= kMoveAmount;
			}
			
			if (keyboard.wentDown(SDLK_RIGHT, true) && s_selectedRegionIndex != -1)
			{
				auto & r = fi.regions[s_selectedRegionIndex];
				
				r.begin += kMoveAmount;
				r.end += kMoveAmount;
			}
			
			if (keyboard.wentDown(SDLK_RETURN, true) && s_selectedRegionIndex != -1)
			{
				auto & r = fi.regions[s_selectedRegionIndex];
				
				s_paHandler->position = r.begin;
			}
			
			if (mouse.wentDown(BUTTON_LEFT))
			{
				const Vec2 p = viewToSample.Mul4(Vec2(mouse.x, mouse.y));
				const int sampleIndex = p[0];
				
				if (commandMod())
				{
					decideOrClickLocation = mouse.x;
					
					s_state = kState_DecideDrawOrClick;
					break;
				}
				
				// hit test regions
				
				s_selectedRegionIndex = -1;
				
				for (size_t i = 0; i < fi.regions.size(); ++i)
				{
					auto & r = fi.regions[i];
					
					if (sampleIndex >= r.begin && sampleIndex < r.end)
						s_selectedRegionIndex = i;
				}
				
				if (s_selectedRegionIndex == -1)
				{
					decideOrClickLocation = mouse.x;
					
					s_state = kState_DecideDrawOrClick;
					break;
				}
				else
				{
					auto & r = fi.regions[s_selectedRegionIndex];
					
					const Vec2 p1 = sampleToView.Mul4(Vec2(r.begin, 0));
					const Vec2 p2 = sampleToView.Mul4(Vec2(r.end, 0));
					
					s_dragL = std::abs(p1[0] - mouse.x) < 6;
					s_dragR = std::abs(p2[0] - mouse.x) < 6;
					
					if (s_dragL == false && s_dragR == false)
						s_dragL = s_dragR = true;
					
					s_state = kState_DragRegion;
					break;
				}
			}
			
			break;
		}
	
	case kState_DecideDrawOrClick:
		{
			const int dx = decideOrClickLocation - mouse.x;
			
			if (std::abs(dx) >= 10)
			{
				const Vec2 p = viewToSample.Mul4(Vec2(decideOrClickLocation, 0));
				const int sampleIndex = p[0];
			
				Region region;
				region.begin = sampleIndex;
				region.end = sampleIndex;
				fi.regions.push_back(region);
			
				s_selectedRegionIndex = fi.regions.size() - 1;
			
				s_state = kState_DrawRegion;
				break;
			}
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				const Vec2 p = viewToSample.Mul4(Vec2(decideOrClickLocation, 0));
				const int sampleIndex = p[0];
				
				s_paHandler->position = sampleIndex;
				
				s_state = kState_Idle;
				break;
			}
			
			break;
		}
		
	case kState_DrawRegion:
		{
			const Vec2 p = viewToSample.Mul4(Vec2(mouse.x, mouse.y));
			const int sampleIndex = p[0];
			
			auto & r = fi.regions[s_selectedRegionIndex];
			
			r.end = sampleIndex;
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				if (r.end < r.begin)
					std::swap(r.begin, r.end);
				
				const int kMinSize = 4096;
				
				if (r.end < r.begin + kMinSize)
					r.end = r.begin + kMinSize;
				
				s_state = kState_Idle;
			}
		}
		break;
		
	case kState_DragRegion:
		{
			if (mouse.wentUp(BUTTON_LEFT))
			{
				s_dragL = false;
				s_dragR = false;
				
				s_state = kState_Idle;
				break;
			}
			
			auto & r = fi.regions[s_selectedRegionIndex];
			
			const float s = (s_sampleIndex2 - s_sampleIndex1) / float(GFX_SX);
			const int offset = mouse.dx * s;
		
			if (s_dragL)
				r.begin += offset;
			if (s_dragR)
				r.end += offset;
		}
		break;
	}
}

int main(int argc, char * argv[])
{
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	SoundData * soundData = loadSound("test.wav");
	s_soundData = soundData;
	
	if (soundData == nullptr)
		return -1;
	
	s_sampleIndex1 = 0;
	s_sampleIndex2 = soundData->sampleCount;
	
	FileInfo fi;
	
	// load file info
	
	{
		tinyxml2::XMLDocument d;
		
		if (d.LoadFile("test.xml") == tinyxml2::XML_SUCCESS)
		{
			auto fileXml = d.FirstChildElement("file");
			
			if (fileXml != nullptr)
			{
				fi.loadXml(fileXml);
			}
		}
	}
	
	MyPortAudioHandler paHandler;
	s_paHandler = &paHandler;
	
	PortAudioObject pa;
	
	pa.init(soundData->sampleRate, soundData->channelCount, 0, 256, &paHandler);
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			doEditor(soundData, fi);
		}
		framework.endDraw();
	}
	
	pa.shut();
	
	// export PCM data
	
	exportFragments(soundData, fi.regions, "fragment_%04d.pcm");
	
	// save file info
	
	{
		tinyxml2::XMLPrinter p;
		p.OpenElement("file");
		{
			fi.saveXml(p);
		}
		p.CloseElement();
		
		tinyxml2::XMLDocument d;
		if (d.Parse(p.CStr()) == tinyxml2::XML_SUCCESS)
		{
			d.SaveFile("test.xml");
		}
	}
	
	delete soundData;
	soundData = nullptr;
	
	framework.shutdown();
	
	return 0;
}
