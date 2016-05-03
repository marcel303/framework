#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"
#include "Calc.h"
#include "framework.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "tinyxml2.h"
#include "xml.h"
#include <algorithm>
#include <list>

// todo : send OSC messages
// todo : load confirmation
// todo : combined PCM and sequence data load
// todo : playback pause/resume

using namespace tinyxml2;

#define GFX_SX 1000
#define GFX_SY 400

#define INSERT_Y (GFX_SY/2 - 100)
#define SELECT_Y (GFX_SY/2)
#define DELETE_Y (GFX_SY/2 + 100)

#define UIZONE_SY 32

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

			const int sampleBufferSize = 4096;
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

static AudioFile g_audioFile;

static Surface * g_audioFileSurface = nullptr;

static double g_lastAudioTime = 0.0;

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
	return time / g_audioFile.m_duration * GFX_SX;
}

static double screenXToTime(double x)
{
	return x / double(GFX_SX) * g_audioFile.m_duration;
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

enum MouseInteract
{
	kMouseInteract_None,
	kMouseInteract_MoveEventMarker,
	kMouseInteract_MovePlaybackMarker
};

static MouseInteract g_mouseInteract = kMouseInteract_None;

//

void clearSequence()
{
	g_eventMarkers.clear();
	g_selectedEventMarker = nullptr;
	g_selectedEventMarkerTimeOffset = 0.0;
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
		}
	}

	return result;
}

//

int main(int argc, char * argv[])
{
	int scale = 2;

	UdpTransmitSocket transmitSocket(IpEndpointName(OSC_DEST_ADDRESS, OSC_DEST_PORT));

	//framework.waitForEvents = true;

	framework.windowY = 100;

	if (framework.init(0, nullptr, GFX_SX * scale, GFX_SY * scale))
	{
		AudioOutput_OpenAL * audioOutput = nullptr;

		bool stop = false;

		while (!stop)
		{
			// process framework

			framework.process();

			if (audioOutput != nullptr)
			{
				audioOutput->Update(&g_audioFile);
			}

			const int mouseX = mouse.x / scale;
			const int mouseY = mouse.y / scale;

			// process events

			if (framework.quitRequested)
				stop = true;

			double audioTime1 = g_lastAudioTime;
			double audioTime2 = g_audioFile.getTime();

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

			// process input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_w))
				saveSequence("heroes-seq.xml");

			if (keyboard.wentDown(SDLK_r))
				loadSequence("heroes-seq.xml");

			if (keyboard.wentDown(SDLK_l))
			{
				if (audioOutput != nullptr)
				{
					audioOutput->Stop();
					audioOutput->Shutdown();

					delete audioOutput;
					audioOutput = nullptr;

					g_lastAudioTime = 0.0;
				}

				g_audioFile.load("tracks/heroes.ogg");

				audioOutput = new AudioOutput_OpenAL();
				audioOutput->Initialize(2, g_audioFile.m_sampleRate, 1 << 13); // todo : sample rate;
				audioOutput->Play();

				delete g_audioFileSurface;
				g_audioFileSurface = nullptr;

				clearSequence();
			}

			if (mouse.wentDown(BUTTON_LEFT))
			{
				g_mouseInteract = kMouseInteract_None;

				if (!g_audioFile.m_pcmData.empty()) // todo : this should be the box surrounding all markers
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

					if (mouseY >= INSERT_Y - UIZONE_SY/2 && mouseY <= INSERT_Y + UIZONE_SY/2)
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

				if (g_selectedEventMarker == nullptr)
				{
					// todo : hit test stuff behind event markers
				}
			}
			else if (mouse.wentUp(BUTTON_LEFT))
			{
				g_mouseInteract = kMouseInteract_None;
			}

			if (g_mouseInteract == kMouseInteract_MoveEventMarker)
			{
				if (mouse.isDown(BUTTON_LEFT))
				{
					if (g_selectedEventMarker != nullptr)
					{
						g_selectedEventMarker->time = screenXToTime(mouseX) + g_selectedEventMarkerTimeOffset;
					}
				}
				else
				{
					g_mouseInteract = kMouseInteract_None;
				}
			}
			else if (g_mouseInteract == kMouseInteract_MovePlaybackMarker)
			{
				if (mouse.isDown(BUTTON_LEFT))
				{
					const double time = screenXToTime(mouseX);

					if (g_playbackMarker != time)
					{
						g_playbackMarker = time;

						g_audioFile.seek(time);
						g_lastAudioTime = time;
					}
				}
				else
				{
					g_mouseInteract = kMouseInteract_None;
				}
			}
			else
			{
				//
			}

			if (g_selectedEventMarker != nullptr)
			{
				if (keyboard.wentDown(SDLK_LEFT) || keyboard.keyRepeat(SDLK_LEFT))
				{
					g_selectedEventMarker->time = Calc::Max(0.0, g_selectedEventMarker->time - 0.1);
				}
				else if (keyboard.wentDown(SDLK_RIGHT) || keyboard.keyRepeat(SDLK_RIGHT))
				{
					g_selectedEventMarker->time = Calc::Min(g_audioFile.m_duration, g_selectedEventMarker->time + 0.1);
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
				gxScalef(scale, scale, 1);

				// draw PCM data

				if (g_audioFileSurface == nullptr && !g_audioFile.m_pcmData.empty())
				{
					g_audioFileSurface = new Surface(GFX_SX, GFX_SY);

					pushSurface(g_audioFileSurface);
					{
						glClearColor(0.f, 0.f, 0.f, 0.f);
						glClear(GL_COLOR_BUFFER_BIT);

						setColor(colorBlue);

						gxBegin(GL_LINES);
						{
							for (int x = 0; x < GFX_SX; ++x)
							{
								const size_t i1 = (x + 0ull) * g_audioFile.m_pcmData.size() / GFX_SX;
								const size_t i2 = (x + 1ull) * g_audioFile.m_pcmData.size() / GFX_SX;

								Assert(i1 >= 0 && i1 <  g_audioFile.m_pcmData.size());
								Assert(i2 >= 0 && i2 <= g_audioFile.m_pcmData.size());

								short min = g_audioFile.m_pcmData[i1].channel[0];
								short max = g_audioFile.m_pcmData[i1].channel[1];

								for (size_t i = i1 + 1; i < i2; ++i)
								{
									min = Calc::Min(min, g_audioFile.m_pcmData[i].channel[0]);
									min = Calc::Min(min, g_audioFile.m_pcmData[i].channel[1]);

									max = Calc::Max(max, g_audioFile.m_pcmData[i].channel[0]);
									max = Calc::Max(max, g_audioFile.m_pcmData[i].channel[1]);
								}

								gxVertex2f(x, Calc::Scale((min / double(1 << 15) + 1.f) / 2.f, 0, GFX_SY));
								gxVertex2f(x, Calc::Scale((max / double(1 << 15) + 1.f) / 2.f, 0, GFX_SY));
							}
						}
						gxEnd();
					}
					popSurface();
				}

				if (g_audioFileSurface != nullptr)
				{
					gxSetTexture(g_audioFileSurface->getTexture());
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					gxSetTexture(0);
				}

				if (g_mouseInteract != kMouseInteract_None)
				{
					setColor(255, 255, 255, 31);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}

				// draw playback marker

				const double playbackTime = g_audioFile.getTime();
				setColor(255, 255, 255, 127);
				drawLine(timeToScreenX(playbackTime), 0, timeToScreenX(playbackTime), GFX_SY);

				setColor(colorWhite);
				drawLine(timeToScreenX(g_playbackMarker), 0, timeToScreenX(g_playbackMarker), GFX_SY);

				// draw UI zones

				setBlend(BLEND_ADD);
				{
					const int alpha = 127;

					setFont("calibri.ttf");

					setColor(255, 255, 0, alpha);
					drawRect(0, SELECT_Y - UIZONE_SY/2, GFX_SX, SELECT_Y + UIZONE_SY/2);
					//setColor(colorWhite);
					drawText(GFX_SX / 2, SELECT_Y, 16, 0, 0, "<-- select -->");

					setColor(0, 255, 0, alpha);
					drawRect(0, INSERT_Y - UIZONE_SY/2, GFX_SX, INSERT_Y + UIZONE_SY/2);
					//setColor(colorWhite);
					drawText(GFX_SX / 2, INSERT_Y, 16, 0, 0, "<-- insert -->");

					setColor(255, 0, 0, alpha);
					drawRect(0, DELETE_Y - UIZONE_SY/2, GFX_SX, DELETE_Y + UIZONE_SY/2);
					//setColor(colorWhite);
					drawText(GFX_SX / 2, DELETE_Y, 16, 0, 0, "<-- delete -->");
				}
				setBlend(BLEND_ALPHA);

				// draw event markers

				int drawPosition = 0;

				for (auto & eventMarker : g_eventMarkers)
				{
					setColor(&eventMarker == g_selectedEventMarker ? colorYellow : colorWhite);
					drawRect(
						timeToScreenX(eventMarker.time) - markerSizeDraw,
						0,
						timeToScreenX(eventMarker.time) + markerSizeDraw,
						GFX_SY);

					setFont("calibri.ttf");
					setColor(colorWhite);
					drawText(
						timeToScreenX(eventMarker.time) + markerSizeDraw + 5.f,
						GFX_SY/4 + drawPosition * 20,
						16,
						+1, +1,
						"eventId: %d", eventMarker.eventId);

					drawPosition = (drawPosition + 1) % 8;
				}
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
