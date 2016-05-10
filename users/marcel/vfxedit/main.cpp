#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"
#include "Calc.h"
#include "FileStream.h"
#include "framework.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "Path.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "xml.h"
#include <algorithm>
#include <list>

#if !defined(DEBUG)
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

// todo : + send OSC messages
// todo : # load confirmation
// todo : + combined PCM and sequence data load
// todo : + playback pause/resume

using namespace tinyxml2;

#define GFX_SX 1700
#define GFX_SY 300
#define SCALE 2

#define INSERT_Y (GFX_SY/2 - 70)
#define SELECT_Y (GFX_SY/2)
#define DELETE_Y (GFX_SY/2 + 70)

#define UIZONE_SY 32

#define COMMENTREGION_Y 5
#define COMMENTREGION_SY (GFX_SY - COMMENTREGION_Y * 2)
#define COMMENTREGION_ALPHA .2f
#define COMMENTREGION_SELECT_Y COMMENTREGION_Y
#define COMMENTREGION_SELECT_SY 25

#define OSC_BUFFER_SIZE 1000
#define OSC_DEST_ADDRESS "127.0.0.1"
#define OSC_DEST_PORT 1121

struct AudioFile : public AudioStream
{
	std::string m_filename;
	std::vector<AudioSample> m_pcmData;
	int m_sampleRate;
	double m_duration;
	int m_provideOffset;
	SDL_mutex * m_mutex;

	AudioFile()
		: m_duration(0.0)
		, m_provideOffset(0)
		, m_mutex(nullptr)
	{
		m_mutex = SDL_CreateMutex();
	}

	~AudioFile()
	{
		SDL_DestroyMutex(m_mutex);
		m_mutex = nullptr;
	}

	void reset()
	{
		SDL_LockMutex(m_mutex);
		{
			m_filename.clear();
			m_pcmData.clear();
			m_duration = 0.0;
			m_provideOffset = 0;
		}
		SDL_UnlockMutex(m_mutex);
	}

	void load(const char * filename)
	{
		reset();

		try
		{
			std::vector<AudioSample> pcmData;

			AudioStream_Vorbis audioStream;

			audioStream.Open(filename, false);

			const int sampleBufferSize = 1 << 16;
			AudioSample sampleBuffer[sampleBufferSize];

			for (;;)
			{
				const int numSamples = audioStream.Provide(sampleBufferSize, sampleBuffer);

				const int offset = pcmData.size();

				pcmData.resize(pcmData.size() + numSamples);

				memcpy(&pcmData[offset], sampleBuffer, sizeof(AudioSample) * numSamples);

				if (numSamples != sampleBufferSize)
					break;
			}

			SDL_LockMutex(m_mutex);
			{
				m_filename = filename;
				m_pcmData = pcmData;
				m_sampleRate = audioStream.mSampleRate;
				m_duration = pcmData.size() / double(audioStream.mSampleRate);
			}
			SDL_UnlockMutex(m_mutex);
		}
		catch (std::exception & e)
		{
			logError("error: %s", e.what());

			reset();
		}
	}
	
	void seek(double time)
	{
		SDL_LockMutex(m_mutex);
		{
			m_provideOffset = Calc::Clamp(int(time * m_sampleRate), 0, m_pcmData.size());
		}
		SDL_UnlockMutex(m_mutex);
	}

	double getTime() const
	{
		double result;

		SDL_LockMutex(m_mutex);
		{
			result = m_provideOffset / double(m_sampleRate);
		}
		SDL_UnlockMutex(m_mutex);

		return result;
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		SDL_LockMutex(m_mutex);
		{
			const int available = m_pcmData.size() - m_provideOffset;
			numSamples = Calc::Clamp(available, 0, numSamples);

			if (numSamples > 0)
			{
				memcpy(buffer, &m_pcmData[m_provideOffset], sizeof(AudioSample) * numSamples);
				m_provideOffset += numSamples;
			}
		}
		SDL_UnlockMutex(m_mutex);

		return numSamples;
	}
};

static AudioFile * g_audioFile = nullptr;

static Surface * g_audioFileSurface = nullptr;

static double g_lastAudioTime = 0.0;

//

static SDL_Thread * g_audioThread = nullptr;
static volatile bool g_stopAudioThread = false;
static AudioOutput_OpenAL * g_audioOutput = nullptr;
static bool g_wantsAudioPlayback = false;
static uint32_t g_audioUpdateEvent = -1;

static int SDLCALL ExecuteAudioThread(void * arg)
{
	while (!g_stopAudioThread)
	{
		if (g_wantsAudioPlayback && !g_audioOutput->IsPlaying_get())
			g_audioOutput->Play();
		if (!g_wantsAudioPlayback && g_audioOutput->IsPlaying_get())
			g_audioOutput->Stop();

		g_audioOutput->Update(g_audioFile);
		SDL_Delay(10);

		if (g_audioOutput->IsPlaying_get())
		{
			SDL_Event e;
			memset(&e, 0, sizeof (e));
			e.type = g_audioUpdateEvent;
			SDL_PushEvent(&e);
		}
	}

	return 0;
}

//

static double g_playbackMarker = 0.0;

//

struct EventMarker
{
	EventMarker()
		: time(0.0)
		, eventId(0)
	{
	}

	bool operator<(const EventMarker & other) const
	{
		return time < other.time;
	}

	double time;
	int eventId;
};

std::list<EventMarker> g_eventMarkers;

EventMarker * g_selectedEventMarker = nullptr;
double g_selectedEventMarkerTimeOffset = 0.0;

const int markerSizeDraw = 1;
const int markerSizeSelect = 50;

static double timeToScreenX(double time)
{
	if (g_audioFile)
		return time / g_audioFile->m_duration * GFX_SX;
	else
		return 0.0;
}

static double screenXToTime(double x)
{
	if (g_audioFile)
		return x / double(GFX_SX) * g_audioFile->m_duration;
	else
		return 0.0;
}

static void deleteEventMarker(EventMarker * eventMarker)
{
	for (auto i = g_eventMarkers.begin(); i != g_eventMarkers.end(); ++i)
	{
		EventMarker & eventMarkerItr = *i;

		if (&eventMarkerItr == eventMarker)
		{
			g_eventMarkers.erase(i);
			break;
		}
	}

	g_selectedEventMarker = nullptr;
}

//

struct CommentRegion
{
	CommentRegion()
		: time(0.0)
		, duration(0.0)
	{
	}

	double time;
	double duration;
	std::string text;
};

enum CommentRegionEdit
{
	kCommentRegionEdit_None,
	kCommentRegionEdit_Move,
	kCommentRegionEdit_SizeL,
	kCommentRegionEdit_SizeR,
};

static std::list<CommentRegion> g_commentRegions;

static CommentRegion * g_selectedCommentRegion = nullptr;
static CommentRegionEdit g_commentRegionEdit = kCommentRegionEdit_None;
static double g_commentRegionEditOffset = 0.0;

static CommentRegion * hittestCommentRegion(double x, double y)
{
	CommentRegion * result = nullptr;

	if (y >= COMMENTREGION_SELECT_Y && y < (COMMENTREGION_SELECT_Y + COMMENTREGION_SELECT_SY))
	{
		const double time = screenXToTime(x);

		for (CommentRegion & commentRegion : g_commentRegions)
		{
			if (time >= commentRegion.time && time < commentRegion.time + commentRegion.duration)
			{
				result = &commentRegion;
			}
		}
	}

	return result;
}

//

enum MouseInteract
{
	kMouseInteract_None,
	kMouseInteract_MoveEventMarker,
	kMouseInteract_MovePlaybackMarker,
	kMouseInteract_ModifyCommentRegion
};

static MouseInteract g_mouseInteract = kMouseInteract_None;

//

void clearAudio()
{
	if (g_audioThread != nullptr)
	{
		g_stopAudioThread = true;
		SDL_WaitThread(g_audioThread, nullptr);
		g_audioThread = nullptr;
		g_stopAudioThread = false;
	}

	if (g_audioOutput != nullptr)
	{
		g_audioOutput->Stop();
		g_audioOutput->Shutdown();
		delete g_audioOutput;
		g_audioOutput = nullptr;
	}

	g_wantsAudioPlayback = false;

	g_lastAudioTime = 0.0;

	if (g_audioFile != nullptr)
	{
		delete g_audioFile;
		g_audioFile = nullptr;
	}

	if (g_audioFileSurface != nullptr)
	{
		delete g_audioFileSurface;
		g_audioFileSurface = nullptr;
	}
}

void loadAudio(const char * filename)
{
	Assert(g_audioFile == nullptr);
	g_audioFile = new AudioFile();
	g_audioFile->load(filename);

	Assert(g_audioOutput == nullptr);
	Assert(g_lastAudioTime == 0.0);
	g_audioOutput = new AudioOutput_OpenAL();
	g_audioOutput->Initialize(2, g_audioFile->m_sampleRate, 1 << 12); // todo : sample rate;

	Assert(g_audioThread == nullptr);
	Assert(!g_stopAudioThread);
	g_audioThread = SDL_CreateThread(ExecuteAudioThread, "AudioThread", nullptr);
}

//

void clearSequence()
{
	g_eventMarkers.clear();
	g_selectedEventMarker = nullptr;
	g_selectedEventMarkerTimeOffset = 0.0;
	
	g_commentRegions.clear();
	g_selectedCommentRegion = nullptr;
	g_commentRegionEdit = kCommentRegionEdit_None;
	g_commentRegionEditOffset = 0.0;
	
	g_playbackMarker = 0.0;
	g_mouseInteract = kMouseInteract_None;
}

bool saveSequence(const char * filename)
{
	bool result = true;

	XMLPrinter p;

	p.OpenElement("sequence");
	{
		for (EventMarker & eventMarker : g_eventMarkers)
		{
			p.OpenElement("marker");
			{
				p.PushAttribute("time", eventMarker.time);
				p.PushAttribute("eventId", eventMarker.eventId);
			}
			p.CloseElement();
		}

		for (CommentRegion & commentRegion : g_commentRegions)
		{
			p.OpenElement("comment");
			{
				p.PushAttribute("time", commentRegion.time);
				p.PushAttribute("duration", commentRegion.duration);
				p.PushAttribute("text", commentRegion.text.c_str());
			}
			p.CloseElement();
		}
	}
	p.CloseElement();

	FILE * f = fopen(filename, "wt");

	if (f == nullptr)
	{
		result = false;
	}
	else
	{
		fprintf_s(f, "%s", p.CStr());
		fclose(f);
	}

	return result;
}

bool loadSequence(const char * filename)
{
	bool result = true;

	clearSequence();

	XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

		result = false;
	}
	else
	{
		for (XMLElement * sequenceXml = xmlDoc.FirstChildElement("sequence"); sequenceXml; sequenceXml = sequenceXml->NextSiblingElement("sequence"))
		{
			for (XMLElement * eventMarkerXml = sequenceXml->FirstChildElement("marker"); eventMarkerXml; eventMarkerXml = eventMarkerXml->NextSiblingElement("marker"))
			{
				EventMarker eventMarker;
				eventMarker.time = floatAttrib(eventMarkerXml, "time", 0.f);
				eventMarker.eventId = intAttrib(eventMarkerXml, "eventId", -1);

				g_eventMarkers.push_back(eventMarker);
			}

			for (XMLElement * commentXml = sequenceXml->FirstChildElement("comment"); commentXml; commentXml = commentXml->NextSiblingElement("comment"))
			{
				CommentRegion commentRegion;
				commentRegion.time = floatAttrib(commentXml, "time", 0.f);
				commentRegion.duration = floatAttrib(commentXml, "duration", 0.f);
				commentRegion.text = stringAttrib(commentXml, "text", "");

				g_commentRegions.push_back(commentRegion);
			}
		}
	}

	return result;
}

//

static bool doFileDialog(const std::string & extension, std::string & filename)
{
	filename.clear();

	//

	auto files = listFiles("tracks", false);
	for (auto i = files.begin(); i != files.end(); )
	{
		if (Path::GetExtension(*i) != extension)
			i = files.erase(i);
		else
			++i;
	}

	bool done = false;
	int selection = 0;
	bool accept = false;

	if (files.empty())
		done = true;

	while (!done)
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			gxScalef(SCALE, SCALE, 1);

			for (size_t i = 0; i < files.size(); ++i)
			{
				setFont("calibri.ttf");
				if (i == selection)
					setColor(colorYellow);
				else
					setColor(colorWhite);
				drawText(GFX_SX/2, 40 + i * 20, 16, 0, 0, "%s", files[i].c_str());
			}
		}
		framework.endDraw();

		framework.process();

		if (keyboard.wentDown(SDLK_UP))
			selection = (selection - 1 + files.size()) % files.size();
		if (keyboard.wentDown(SDLK_DOWN))
			selection = (selection + 1 + files.size()) % files.size();
		if (keyboard.wentDown(SDLK_RETURN))
		{
			done = true;
			accept = true;
		}
		if (keyboard.wentDown(SDLK_ESCAPE))
			done = true;
	}

	if (accept)
	{
		filename = files[selection];
	}

	return accept;
}

//

static bool isChar(char c)
{
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		(c == ' ' || c == '.' || c == ',' || c == '!' || c == '(' || c == ')');
}

static bool doTextDialog(std::string & text)
{
	bool done = false;
	int selection = 0;
	bool accept = false;

	while (!done)
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			gxScalef(SCALE, SCALE, 1);

			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX/2, GFX_SY/2, 16, 0, 0, "%s", text.c_str());
		}
		framework.endDraw();

		framework.process();

		if (keyboard.wentDown(SDLK_BACKSPACE))
		{
			if (text.size() > 0)
				text.pop_back();
		}

		if (keyboard.wentDown(SDLK_DELETE))
		{
			if (text.size() > 0)
				text.pop_back();
		}

		for (int i = 0; i < 256; ++i)
			if (keyboard.wentDown((SDLKey)i) && isChar((SDLKey)i))
				text.push_back(i);

		if (keyboard.wentDown(SDLK_RETURN))
		{
			done = true;
			accept = true;
		}
		if (keyboard.wentDown(SDLK_ESCAPE))
			done = true;
	}

	if (!accept)
	{
		text.clear();
	}

	return accept;
}

//

int main(int argc, char * argv[])
{
	UdpTransmitSocket transmitSocket(IpEndpointName(OSC_DEST_ADDRESS, OSC_DEST_PORT));

	framework.waitForEvents = true;

	framework.windowY = 100;

	if (framework.init(0, nullptr, GFX_SX * SCALE, GFX_SY * SCALE))
	{
		g_audioUpdateEvent = SDL_RegisterEvents(1);

		//

		if (argc >= 2)
		{
			const std::string filename = argv[1];

			loadAudio(filename.c_str());

			//

			const std::string sequenceFilename = Path::ReplaceExtension(filename, "xml");

			if (FileStream::Exists(sequenceFilename.c_str()))
			{
				loadSequence(sequenceFilename.c_str());
			}
		}

		//

		bool stop = false;

		while (!stop)
		{
			// process framework

			framework.process();

			const int mouseX = mouse.x / SCALE;
			const int mouseY = mouse.y / SCALE;

			// process events

			if (framework.quitRequested)
				stop = true;

			if (g_audioFile != nullptr)
			{
				double audioTime1 = g_lastAudioTime;
				double audioTime2 = g_audioFile->getTime();

				for (EventMarker & eventMarker : g_eventMarkers)
				{
					if (eventMarker.time >= audioTime1 && eventMarker.time < audioTime2)
					{
						logDebug("trigger event %d", eventMarker.eventId);

						char buffer[OSC_BUFFER_SIZE];

						osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

						p
							<< osc::BeginBundleImmediate
							<< osc::BeginMessage("/event")
							<< ""
							<< (osc::int32)eventMarker.eventId
							<< osc::EndMessage
							<< osc::EndBundle;

						transmitSocket.Send(p.Data(), p.Size());
					}
				}

				g_lastAudioTime = audioTime2;
			}

			// process input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_w))
			{
				if (g_audioFile != nullptr)
				{
					const std::string sequenceFilename = Path::ReplaceExtension(g_audioFile->m_filename, "xml");

					saveSequence(sequenceFilename.c_str());
				}
			}

			if (keyboard.wentDown(SDLK_r))
				loadSequence("tracks/heroes.xml");

			if (keyboard.wentDown(SDLK_l))
			{
				std::string filename;

				if (doFileDialog("ogg", filename))
				{
					clearAudio();

					clearSequence();

					//

					loadAudio(filename.c_str());

					//

					const std::string sequenceFilename = Path::ReplaceExtension(filename, "xml");

					if (FileStream::Exists(sequenceFilename.c_str()))
					{
						loadSequence(sequenceFilename.c_str());
					}
				}
			}

			if (keyboard.wentDown(SDLK_t))
			{
				if (g_selectedCommentRegion != nullptr)
				{
					std::string text = g_selectedCommentRegion->text;

					if (doTextDialog(text))
					{
						g_selectedCommentRegion->text = text;
					}
				}
			}

			if (keyboard.wentDown(SDLK_SPACE) && g_audioFile != nullptr)
			{
				if (keyboard.isDown(SDLK_LSHIFT))
				{
					g_audioFile->seek(g_playbackMarker);
					g_wantsAudioPlayback = true;
				}
				else
				{
					g_wantsAudioPlayback = !g_wantsAudioPlayback;
				}
			}

			// determine mouse interaction

			if (g_mouseInteract == kMouseInteract_None && mouse.wentDown(BUTTON_LEFT))
			{
				if (g_audioFile != nullptr)
				{
					// hit test event markers

					EventMarker * selectedEventMarker = nullptr;

					int bestDistance = -1;

					for (auto & eventMarker : g_eventMarkers)
					{
						const int x = timeToScreenX(eventMarker.time);
						const int distance = Calc::Abs(mouseX - x);

						if (distance < markerSizeSelect && (bestDistance == -1 || distance < bestDistance))
						{
							bestDistance = distance;
							selectedEventMarker = &eventMarker;
						}
					}

					if (mouseY >= COMMENTREGION_SELECT_Y && mouseY < (COMMENTREGION_SELECT_Y + COMMENTREGION_SELECT_SY))
					{
						// hit test comment regions

						const double time = screenXToTime(mouseX);

						g_selectedCommentRegion = hittestCommentRegion(mouseX, mouseY);

						if (g_selectedCommentRegion != nullptr)
						{
							const int edgeSize = 10;

							if (timeToScreenX(time) < timeToScreenX(g_selectedCommentRegion->time) + edgeSize)
							{
								g_commentRegionEdit = kCommentRegionEdit_SizeL;
								g_commentRegionEditOffset = g_selectedCommentRegion->time - time;
							}
							else if (timeToScreenX(time) > timeToScreenX(g_selectedCommentRegion->time + g_selectedCommentRegion->duration) - edgeSize)
							{
								g_commentRegionEdit = kCommentRegionEdit_SizeR;
								g_commentRegionEditOffset = (g_selectedCommentRegion->time + g_selectedCommentRegion->duration) - time;
							}
							else
							{
								g_commentRegionEdit = kCommentRegionEdit_Move;
								g_commentRegionEditOffset = g_selectedCommentRegion->time - time;
							}

							g_mouseInteract = kMouseInteract_ModifyCommentRegion;
						}

						if (g_selectedCommentRegion == nullptr)
						{
							CommentRegion region;
							region.time = time;
							region.duration = 0.0;

							g_commentRegions.push_back(region);
							g_selectedCommentRegion = &g_commentRegions.back();

							g_mouseInteract = kMouseInteract_ModifyCommentRegion;
							g_commentRegionEdit = kCommentRegionEdit_SizeR;
							g_commentRegionEditOffset = 0.0;
						}
					}
					else if (mouseY >= INSERT_Y - UIZONE_SY/2 && mouseY <= INSERT_Y + UIZONE_SY/2)
					{
						const double time = screenXToTime(mouseX);

						EventMarker eventMarker;
						eventMarker.eventId = 0;
						eventMarker.time = time;

						g_eventMarkers.push_back(eventMarker);
						g_selectedEventMarker = &g_eventMarkers.back();
						g_selectedEventMarkerTimeOffset = 0.0;

						g_mouseInteract = kMouseInteract_MoveEventMarker;
					}
					else if (mouseY >= SELECT_Y - UIZONE_SY/2 && mouseY <= SELECT_Y + UIZONE_SY/2)
					{
						if (selectedEventMarker == nullptr)
						{
							g_selectedEventMarker = nullptr;
							g_selectedEventMarkerTimeOffset = 0.0;
						}
						else
						{
							const double time = screenXToTime(mouseX);

							g_selectedEventMarker = selectedEventMarker;
							g_selectedEventMarkerTimeOffset = selectedEventMarker->time - time;

							g_mouseInteract = kMouseInteract_MoveEventMarker;
						}
					}
					else if (mouseY >= DELETE_Y - UIZONE_SY/2 && mouseY <= DELETE_Y + UIZONE_SY/2)
					{
						if (selectedEventMarker != nullptr)
						{
							deleteEventMarker(selectedEventMarker);
						}

						g_selectedEventMarker = nullptr;
						g_selectedEventMarkerTimeOffset = 0.0;
					}
					else
					{
						g_selectedEventMarkerTimeOffset = 0.0;

						g_mouseInteract = kMouseInteract_MovePlaybackMarker;
					}
				}
				else
				{
					Assert(g_selectedEventMarker == nullptr);
					Assert(g_selectedEventMarkerTimeOffset == 0.0);
				}
			}

			// process mouse interaction

			if (g_mouseInteract == kMouseInteract_None)
			{
			}
			else if (g_mouseInteract == kMouseInteract_MoveEventMarker)
			{
				if (mouse.wentUp(BUTTON_LEFT))
				{
					g_mouseInteract = kMouseInteract_None;
				}
				else
				{
					if (g_selectedEventMarker != nullptr)
					{
						g_selectedEventMarker->time = screenXToTime(mouseX) + g_selectedEventMarkerTimeOffset;
					}
				}
			}
			else if (g_mouseInteract == kMouseInteract_MovePlaybackMarker)
			{
				Assert(g_audioFile != nullptr);

				if (mouse.wentUp(BUTTON_LEFT))
				{
					g_mouseInteract = kMouseInteract_None;
				}
				else if (g_audioFile != nullptr)
				{
					const double time = screenXToTime(mouseX);

					if (g_playbackMarker != time)
					{
						g_playbackMarker = time;

						g_audioFile->seek(time);
						g_lastAudioTime = time;
					}
				}
			}
			else if (g_mouseInteract == kMouseInteract_ModifyCommentRegion)
			{
				Assert(g_selectedCommentRegion != nullptr);

				if (mouse.wentUp(BUTTON_LEFT))
				{
					g_mouseInteract = kMouseInteract_None;
				}
				else if (g_selectedCommentRegion != nullptr)
				{
					const double time = screenXToTime(mouseX) + g_commentRegionEditOffset;

					if (g_commentRegionEdit == kCommentRegionEdit_Move)
					{
						g_selectedCommentRegion->time = time;
					}
					else if (g_commentRegionEdit == kCommentRegionEdit_SizeL)
					{
						const double delta = g_selectedCommentRegion->time - time;
						g_selectedCommentRegion->time -= delta;
						g_selectedCommentRegion->duration += delta;
					}
					else if (g_commentRegionEdit == kCommentRegionEdit_SizeR)
					{
						g_selectedCommentRegion->duration = time - g_selectedCommentRegion->time;
					}
					else
					{
						Assert(false);
					}

					if (g_selectedCommentRegion->duration < 0.1)
					{
						g_selectedCommentRegion->duration = 0.1;
					}
				}
			}
			else
			{
				Assert(false);
			}

			// process keyboard interaction

			if (g_selectedEventMarker != nullptr)
			{
				if (keyboard.wentDown(SDLK_LEFT) || keyboard.keyRepeat(SDLK_LEFT))
				{
					g_selectedEventMarker->time = Calc::Max(0.0, g_selectedEventMarker->time - 0.1);
				}
				else if (keyboard.wentDown(SDLK_RIGHT) || keyboard.keyRepeat(SDLK_RIGHT))
				{
					if (g_audioFile != nullptr)
					{
						g_selectedEventMarker->time = Calc::Min(g_audioFile->m_duration, g_selectedEventMarker->time + 0.1);
					}
				}
				else if (keyboard.wentDown(SDLK_UP) || keyboard.keyRepeat(SDLK_UP))
				{
					g_selectedEventMarker->eventId = Calc::Min(50, g_selectedEventMarker->eventId + 1);
				}
				else if (keyboard.wentDown(SDLK_DOWN) || keyboard.keyRepeat(SDLK_DOWN))
				{
					g_selectedEventMarker->eventId = Calc::Max(0, g_selectedEventMarker->eventId - 1);
				}
				else if (keyboard.wentDown(SDLK_DELETE))
				{
					deleteEventMarker(g_selectedEventMarker);
				}
			}
			
			// draw

			framework.beginDraw(0, 0, 0, 0);
			{
				gxScalef(SCALE, SCALE, 1);

				// draw PCM data

				if (g_audioFileSurface == nullptr && g_audioFile != nullptr && !g_audioFile->m_pcmData.empty())
				{
					const int sx = GFX_SX * SCALE;
					const int sy = GFX_SY * SCALE;

					g_audioFileSurface = new Surface(sx, sy);

					pushSurface(g_audioFileSurface);
					{
						glClearColor(0.f, 0.f, 0.f, 0.f);
						glClear(GL_COLOR_BUFFER_BIT);

						setColor(colorWhite);

						gxBegin(GL_LINES);
						{
							for (int x = 0; x < sx; ++x)
							{
								const size_t i1 = (x + 0ull) * g_audioFile->m_pcmData.size() / sx;
								const size_t i2 = (x + 1ull) * g_audioFile->m_pcmData.size() / sx;

								Assert(i1 >= 0 && i1 <  g_audioFile->m_pcmData.size());
								Assert(i2 >= 0 && i2 <= g_audioFile->m_pcmData.size());

								short min = g_audioFile->m_pcmData[i1].channel[0];
								short max = g_audioFile->m_pcmData[i1].channel[0];

								for (size_t i = i1; i < i2; ++i)
								{
									min = Calc::Min(min, g_audioFile->m_pcmData[i].channel[0]);
									min = Calc::Min(min, g_audioFile->m_pcmData[i].channel[1]);

									max = Calc::Max(max, g_audioFile->m_pcmData[i].channel[0]);
									max = Calc::Max(max, g_audioFile->m_pcmData[i].channel[1]);
								}

								gxVertex2f(x, Calc::Scale((min / double(1 << 15) + 1.f) / 2.f, 0, sy));
								gxVertex2f(x, Calc::Scale((max / double(1 << 15) + 1.f) / 2.f, 0, sy));
							}
						}
						gxEnd();
					}
					popSurface();
				}

				// draw comment regions

				if (g_audioFile != nullptr)
				{
					setColor(255, 255, 255, 127);
					drawRect(0, COMMENTREGION_SELECT_Y, GFX_SX, COMMENTREGION_SELECT_Y + COMMENTREGION_SELECT_SY);

					setBlend(BLEND_ADD);

					CommentRegion * focusCommentRegion = hittestCommentRegion(mouseX, mouseY);

					for (const auto & commentRegion : g_commentRegions)
					{
						double alphaMultiplier = 1.0;
						double alphaMultiplierOutline = 1.0;

						if (&commentRegion == focusCommentRegion || &commentRegion == g_selectedCommentRegion)
						{
							alphaMultiplier = 1.2;
							alphaMultiplierOutline = 2.0;
						}
						else
						{
							alphaMultiplier = 1.0;
							alphaMultiplierOutline = 1.0;
						}

						const double hue = (commentRegion.time + commentRegion.duration) * 1.5 / g_audioFile->m_duration;
						const Color color = Color::fromHSL(hue, 1.0, 0.5);
						setColorf(color.r, color.g, color.b, COMMENTREGION_ALPHA * alphaMultiplier);

						const double x1 = timeToScreenX(commentRegion.time);
						const double x2 = timeToScreenX(commentRegion.time + commentRegion.duration);
						drawRect(x1, COMMENTREGION_Y, x2, COMMENTREGION_Y + COMMENTREGION_SY);

						const Color borderColor = Color::fromHSL(hue, 0.5, 0.5);
						setColorf(borderColor.r, borderColor.g, borderColor.b, COMMENTREGION_ALPHA * alphaMultiplierOutline);
						drawRectLine(x1, COMMENTREGION_Y, x2, COMMENTREGION_Y + COMMENTREGION_SY);

						setFont("calibri.ttf");
						setColorf(1.f, 1.f, 1.f, COMMENTREGION_ALPHA * alphaMultiplierOutline * 2.f);
						drawText(x1, COMMENTREGION_Y, 16, +1, +1, "%s", commentRegion.text.c_str());
					}
					setBlend(BLEND_ALPHA);
				}

				if (g_audioFileSurface != nullptr)
				{
					gxSetTexture(g_audioFileSurface->getTexture());
					const int c = 127;
					//setColor(c, c, c, 255);
					setColor(colorBlue);
					drawRect(0, 0, GFX_SX, GFX_SY);
					gxSetTexture(0);
				}

				if (g_mouseInteract != kMouseInteract_None)
				{
					setColor(255, 255, 255, 31);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}

				// draw playback marker

				if (g_audioFile != nullptr)
				{
					const double playbackTime = g_audioFile->getTime();
					setColor(255, 255, 255, 127);
					drawLine(timeToScreenX(playbackTime), 0, timeToScreenX(playbackTime), GFX_SY);

					setColor(255, 255, 255, 191);
					drawLine(timeToScreenX(g_playbackMarker), 0, timeToScreenX(g_playbackMarker), GFX_SY);
				}

				// draw UI zones

				setBlend(BLEND_ADD);
				{
					const int alpha = 63;

					setFont("calibri.ttf");

					setColor(0, 255, 0, alpha);
					drawRect(0, INSERT_Y - UIZONE_SY/2, GFX_SX, INSERT_Y + UIZONE_SY/2);
					drawText(GFX_SX / 2, INSERT_Y, 16, 0, 0, "<-- insert -->");

					setColor(255, 255, 0, alpha);
					drawRect(0, SELECT_Y - UIZONE_SY/2, GFX_SX, SELECT_Y + UIZONE_SY/2);
					drawText(GFX_SX / 2, SELECT_Y, 16, 0, 0, "<-- select -->");

					setColor(255, 0, 0, alpha);
					drawRect(0, DELETE_Y - UIZONE_SY/2, GFX_SX, DELETE_Y + UIZONE_SY/2);
					drawText(GFX_SX / 2, DELETE_Y, 16, 0, 0, "<-- delete -->");
				}
				setBlend(BLEND_ALPHA);

				// draw event markers

				int drawPosition = 0;

				g_eventMarkers.sort();

				for (auto & eventMarker : g_eventMarkers)
				{
					if (&eventMarker == g_selectedEventMarker)
						setColor(255, 255, 0, 191);
					else
						setColor(255, 255, 255, 191);

					const int x = timeToScreenX(eventMarker.time);
					drawRect(x - markerSizeDraw, GFX_SY*0/11, x + markerSizeDraw, GFX_SY* 5/11);
					drawRect(x - markerSizeDraw, GFX_SY*6/11, x + markerSizeDraw, GFX_SY*11/11);

					setFont("calibri.ttf");
					drawText(
						timeToScreenX(eventMarker.time) + markerSizeDraw + 5.f,
						GFX_SY/4 + drawPosition * 20,
						16,
						+1, +1,
						"# %d", eventMarker.eventId);

					drawPosition = (drawPosition + 1) % 10;
				}
			}
			framework.endDraw();
		}

		clearAudio();

		clearSequence();

		framework.shutdown();
	}

	return 0;
}
