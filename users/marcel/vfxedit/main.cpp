#include "audiostream/AudioStreamVorbis.h"
#include "Calc.h"
#include "framework.h"
#include <algorithm>
#include <list>

#define GFX_SX 1000
#define GFX_SY 400

#define INSERT_Y (GFX_SY/2 - 100)
#define SELECT_Y (GFX_SY/2)
#define DELETE_Y (GFX_SY/2 + 100)

#define UIZONE_SY 32

struct AudioFile
{
	std::string m_filename;
	std::vector<float> m_pcmData;
	double m_duration;

	void reset()
	{
		m_filename.clear();
		m_pcmData.clear();
		m_duration = 0.0;
	}

	void load(const char * filename)
	{
		reset();

		try
		{
			m_filename = filename;

			AudioStream_Vorbis audioStream;

			audioStream.Open(filename, false);

			const int sampleBufferSize = 4096;
			AudioSample sampleBuffer[sampleBufferSize];

			for (;;)
			{
				const int numSamples = audioStream.Provide(sampleBufferSize, sampleBuffer);

				const int offset = m_pcmData.size();

				m_pcmData.resize(m_pcmData.size() + numSamples);

				for (int i = 0; i < numSamples; ++i)
				{
					const float value = (sampleBuffer[i].channel[0] + sampleBuffer[i].channel[1]) / 2.f / (1 << 15);

					m_pcmData[offset + i] = value;
				}

				if (numSamples != sampleBufferSize)
					break;
			}

			m_duration = m_pcmData.size() / double(audioStream.mSampleRate);
		}
		catch (std::exception & e)
		{
			logError("error: %s", e.what());

			reset();
		}
	}
};

static AudioFile g_audioFile;

static Surface * g_audioFileSurface = nullptr;

//

struct EventMarker
{
	double time;
	int id;
};

std::list<EventMarker> g_eventMarkers;

EventMarker * g_selectedEventMarker = nullptr;

int g_eventMarkerSelectionX = 0;

const int markerSizeDraw = 3;
const int markerSizeSelect = 30;

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

int main(int argc, char * argv[])
{
	int scale = 2;

	framework.waitForEvents = true;

	if (framework.init(0, nullptr, GFX_SX * scale, GFX_SY * scale))
	{
		bool stop = false;

		while (!stop)
		{
			// process framework

			framework.process();

			const int mouseX = mouse.x / scale;
			const int mouseY = mouse.y / scale;

			// process events

			if (framework.quitRequested)
				stop = true;

			// process input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_l))
			{
				g_audioFile.load("tracks/heroes.ogg");

				delete g_audioFileSurface;
				g_audioFileSurface = nullptr;
			}

			if (mouse.wentDown(BUTTON_LEFT))
			{
				EventMarker * selectedEventMarker = nullptr;

				if (!g_audioFile.m_pcmData.empty()) // todo : this should be the box surrounding all markers
				{
					// hit test event markers

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
						eventMarker.id = 0;
						eventMarker.time = time;

						g_eventMarkers.push_back(eventMarker);
						g_selectedEventMarker = &g_eventMarkers.back();
					}
					else if (mouseY >= SELECT_Y - UIZONE_SY/2 && mouseY <= SELECT_Y + UIZONE_SY/2)
					{
						g_selectedEventMarker = selectedEventMarker;
					}
					else if (mouseY >= DELETE_Y - UIZONE_SY/2 && mouseY <= DELETE_Y + UIZONE_SY/2)
					{
						if (selectedEventMarker != nullptr)
						{
							deleteEventMarker(selectedEventMarker);
						}

						g_selectedEventMarker = nullptr;
					}
					else
					{
						g_selectedEventMarker = nullptr;
					}

					g_eventMarkerSelectionX = mouseX;
				}

				if (g_selectedEventMarker == nullptr)
				{
					// todo : hit test stuff behind event markers
				}
			}

			if (g_selectedEventMarker != nullptr)
			{
				if (mouse.isDown(BUTTON_LEFT))
				{
					//if (mouseY >= SELECT_Y - UIZONE_SY/2 && mouseY <= SELECT_Y + UIZONE_SY/2)
					{
						g_selectedEventMarker->time = screenXToTime(mouseX);
					}
				}

				if (keyboard.wentDown(SDLK_LEFT) || keyboard.keyRepeat(SDLK_LEFT))
				{
					g_selectedEventMarker->time = Calc::Max(0.0, g_selectedEventMarker->time - 0.1);
				}

				if (keyboard.wentDown(SDLK_RIGHT) || keyboard.keyRepeat(SDLK_RIGHT))
				{
					g_selectedEventMarker->time = Calc::Min(g_audioFile.m_duration, g_selectedEventMarker->time + 0.1);
				}

				if (keyboard.wentDown(SDLK_DELETE))
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

								Assert(i1 >= 0 && i1 < g_audioFile.m_pcmData.size());
								Assert(i2 >= 0 && i2 <= g_audioFile.m_pcmData.size());

								float min = g_audioFile.m_pcmData[i1];
								float max = g_audioFile.m_pcmData[i1];

								for (size_t i = i1 + 1; i < i2; ++i)
								{
									min = Calc::Min(min, g_audioFile.m_pcmData[i]);
									max = Calc::Max(max, g_audioFile.m_pcmData[i]);
								}

								gxVertex2f(x, Calc::Scale((min + 1.f) / 2.f, 0, GFX_SY));
								gxVertex2f(x, Calc::Scale((max + 1.f) / 2.f, 0, GFX_SY));
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

				for (auto & eventMarker : g_eventMarkers)
				{
					setColor(&eventMarker == g_selectedEventMarker ? colorYellow : colorWhite);
					drawRect(
						timeToScreenX(eventMarker.time) - markerSizeDraw,
						0,
						timeToScreenX(eventMarker.time) + markerSizeDraw,
						GFX_SY);
				}
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
