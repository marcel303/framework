#include "audio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "FileStream.h"
#include "framework.h"
#include "Path.h"
#include "picedit.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "types.h"
#include <vector>

#define DO_COCREATE 0
#define DO_GAUSSIAN_BLUR 0
#define DO_COMPUTE_PARTICLES 0
#define DO_LIGHT_PROPAGATION 0
#define DO_FLOCKING 0
#define DO_HQ_PRIMITIVES 1
#define DO_BUILTIN_SHADER 1

/*

cocompose v1 todo:
- one image per horizontal line? or add them to a grid
- drag and drop images. fixed width/height. stretch so image is fit fully either horizontally or vertically
- audio file at the top, like soundcloud. play/pause button. waveform. allow clicking on edit to open the audio editor
	- enable/disable voices
	- remove voices
	- drag and drop audio files to add voices
		- guard against adding the same file twice
	- record audio using microphone
- drag and drop visuals (shader files). same as images. add to tiles
- option to enlarge a single image? or always the first image. to act as a sort of album cover
- add title (single line, x characters), header, footer text (may be longer, like adding poetry)
- add like button?

cocompose v2 todo:
- evolve button
	- save current creation
	- duplicate current creation
- button to show evolution tree. let it grow like it grew origionally

*/

#if 1
	#if defined(DEBUG)
		#define GFX_SX 1500
		#define GFX_SY 900
	#else
		#define GFX_SX 1920
		#define GFX_SY 1080
	#endif
#else
	#define GFX_SX (1920*2)
	#define GFX_SY (1080*2)
#endif

#if DO_COCREATE

#define kAnimTransitionTime 1.f

struct Rect
{
	Rect()
		: x1(0.f)
		, y1(0.f)
		, x2(0.f)
		, y2(0.f)
	{
	}

	float x1;
	float y1;
	float x2;
	float y2;
};

struct Editable
{
	Editable * mouseHover;
	bool hasMouseHover;

	Editable * mouseFocus;
	bool hasMouseFocus;

	bool wantsRemove;

	void tickBase(const float dt)
	{
		auto children = getChildren();

		for (auto i = children.begin(); i != children.end(); ++i)
		{
			auto child = *i;

			child->tickBase(dt);

			if (child->wantsRemove && removeChild(child))
			{
				if (child == mouseHover)
					mouseHover = nullptr;
				if (child == mouseFocus)
					mouseFocus = nullptr;
			}
		}

		tick(dt);
	}

	bool mouseHoverEnterBase(const int x, const int y)
	{
		if (mouseHoverEnter(x, y))
		{
			hasMouseHover = true;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool mouseEnterBase(const int x, const int y)
	{
		if (mouseEnter(x, y))
		{
			hasMouseFocus = true;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool mouseHoverLeaveBase(const int x, const int y)
	{
		if (mouseHover != nullptr)
		{
			if (!mouseHover->mouseHoverLeaveBase(x, y))
			{
				return false;
			}

			mouseHover = nullptr;
		}

		if (mouseHoverLeave(x, y))
		{
			hasMouseHover = false;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool mouseLeaveBase(const int x, const int y)
	{
		if (mouseFocus != nullptr)
		{
			if (!mouseFocus->mouseLeaveBase(x, y))
			{
				return false;
			}

			mouseFocus = nullptr;
		}

		if (mouseLeave(x, y))
		{
			hasMouseFocus = false;

			return true;
		}
		else
		{
			return false;
		}
	}

	void mouseMoveBase(const int x, const int y)
	{
		if (mouseFocus != nullptr)
		{
			mouseFocus->mouseMoveBase(x, y);
		}
		else if (hasMouseFocus)
		{
			mouseMove(x, y);
		}
	}

	void evaluateMouseHover(const int x, const int y)
	{
		auto children = getChildren();

		for (auto child : children)
		{
			if (child->isInside(x, y))
			{
				if (child == mouseHover)
				{
					break;
				}
				else
				{
					if (mouseHover != nullptr)
					{
						if (!mouseHover->mouseHoverLeaveBase(x, y))
						{
							break;
						}
						else
						{
							mouseHover = nullptr;
						}
					}

					if (child->mouseHoverEnterBase(x, y))
					{
						mouseHover = child;

						break;
					}
				}
			}
			else if (child == mouseHover)
			{
				if (child->mouseHoverLeaveBase(x, y))
				{
					mouseHover = nullptr;
				}
			}
		}

		if (mouseHover != nullptr)
		{
			mouseHover->evaluateMouseHover(x, y);
		}
	}

	void evaluateMouseFocus(const int x, const int y)
	{
		auto children = getChildren();

		for (auto child : children)
		{
			if (child->isInside(x, y))
			{
				if (child == mouseFocus)
				{
					break;
				}
				else
				{
					if (mouseFocus != nullptr)
					{
						if (!mouseFocus->mouseLeaveBase(x, y))
						{
							break;
						}
						else
						{
							mouseFocus = nullptr;
						}
					}

					if (child->mouseEnterBase(x, y))
					{
						mouseFocus = child;

						break;
					}
				}
			}
			else if (child == mouseFocus)
			{
				if (child->mouseLeaveBase(x, y))
				{
					mouseFocus = nullptr;
				}
			}
		}

		if (mouseFocus != nullptr)
		{
			mouseFocus->evaluateMouseFocus(x, y);
		}
	}

	virtual std::vector<Editable*> getChildren() = 0;
	virtual bool removeChild(Editable * child) { return false; }

	virtual void tick(const float dt) = 0;

	virtual bool isInside(const int x, const int y) = 0;
	virtual bool mouseHoverEnter(const int x, const int y) { return true; }
	virtual bool mouseHoverLeave(const int x, const int y) { return true; }
	virtual bool mouseEnter(const int x, const int y) = 0;
	virtual bool mouseLeave(const int x, const int y) = 0;
	virtual void mouseMove(const int x, const int y) = 0;

	virtual bool insertBegin(const std::string & filename, const int x, const int y) = 0;
	virtual void insertEnd(const int x, const int y) = 0;
	virtual void insertMove(const int x, const int y) = 0;
	virtual void insertCancel() = 0;

	Editable()
		: mouseHover(nullptr)
		, hasMouseHover(false)
		, mouseFocus(nullptr)
		, hasMouseFocus(false)
		, wantsRemove(false)
	{
	}

	virtual ~Editable()
	{
	}
};

struct Background : Editable
{
	std::string filename;

	Background()
		: Editable()
		, filename("background1.ps")
	{
	}

	void draw() const
	{
		Shader shader(filename.c_str(), "effect.vs", filename.c_str());

		setShader(shader);
		{
			shader.setImmediate("colormapSize", GFX_SX, GFX_SY);
			shader.setImmediate("time", framework.time); // fixme

			setBlend(BLEND_OPAQUE);
			{
				drawRect(0, 0, GFX_SX, GFX_SY);
			}
			setBlend(BLEND_ALPHA);
		}
		clearShader();
	}

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		return result;
	}

	virtual void tick(const float dt) override
	{
	}

	virtual bool isInside(const int x, const int y) override
	{
		return false;
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		return false;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		return true;
	}

	virtual void mouseMove(const int x, const int y) override
	{
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y) override
	{
		if (String::StartsWith(filename, "background") && Path::GetExtension(filename) == "ps")
			return true;
		else
			return false;
	}

	virtual void insertEnd(const int x, const int y) override
	{
	}

	virtual void insertMove(const int x, const int y) override
	{
	}

	virtual void insertCancel() override
	{
	}
};

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

	bool load(const char * filename)
	{
		reset();

		try
		{
			std::vector<AudioSample> pcmData;
			int sampleRate = 0;

			const std::string cacheFilename = Path::ReplaceExtension(filename, "aud");

			bool gotCached = false;

			if (FileStream::Exists(cacheFilename.c_str()))
			{
				try
				{
					FileStream stream(cacheFilename.c_str(), OpenMode_Read);
					StreamReader reader(&stream, false);
					
					const int numSamples = reader.ReadInt32();
					sampleRate = reader.ReadInt32();

					pcmData.resize(numSamples);
					stream.Read(&pcmData[0], numSamples * sizeof(AudioSample));

					gotCached = true;
				}
				catch (std::exception & e)
				{
					logError(e.what());

					gotCached = false;

					pcmData.clear();
					sampleRate = 0;
				}
			}
			
			if (!gotCached)
			{
				AudioStream_Vorbis audioStream;

				audioStream.Open(filename, false);

				sampleRate = audioStream.mSampleRate;

				const int sampleBufferSize = 1 << 16;
				AudioSample sampleBuffer[sampleBufferSize];

				for (;;)
				{
					const int numSamples = audioStream.Provide(sampleBufferSize, sampleBuffer);

					const int offset = pcmData.size();

					pcmData.resize(pcmData.size() + numSamples);

					if (numSamples > 0)
						memcpy(&pcmData[offset], sampleBuffer, sizeof(AudioSample) * numSamples);

					if (numSamples != sampleBufferSize)
						break;
				}
			}

			SDL_LockMutex(m_mutex);
			{
				m_filename = filename;
				m_pcmData = pcmData;
				m_sampleRate = sampleRate;
				m_duration = pcmData.size() / double(sampleRate);
			}
			SDL_UnlockMutex(m_mutex);

			return true;
		}
		catch (std::exception & e)
		{
			logError("error: %s", e.what());

			reset();

			return false;
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

struct AudioVoice : Editable
{
	enum EditingState
	{
		kEditingState_Idle,
		kEditingState_Insert,
		kEditingState_Drag
	};

	std::string filename;
	bool enabled;
	EditingState editingState;
	float volume;

	AudioFile audioFile;
	Surface * pcmDataSurface;

	AudioVoice()
		: Editable()
		, filename()
		, enabled(true)
		, editingState(kEditingState_Idle)
		, volume(1.f)
		, audioFile()
		, pcmDataSurface(nullptr)
	{
	}

	virtual ~AudioVoice() override
	{
		delete pcmDataSurface;
		pcmDataSurface = nullptr;

		audioFile.reset();
	}

	void drawPcmData(const int sx, const int sy)
	{
		const int64_t numLines = std::ceil(sx);
		const int64_t numSamples = audioFile.m_pcmData.size();

		gxBegin(GL_LINES);
		{
			for (int x = 0; x < sx; ++x)
			{
				const size_t i1 = (x + 0ull) * audioFile.m_pcmData.size() / sx;
				const size_t i2 = (x + 1ull) * audioFile.m_pcmData.size() / sx;

				Assert(i1 >= 0 && i1 <  audioFile.m_pcmData.size());
				Assert(i2 >= 0 && i2 <= audioFile.m_pcmData.size());

				short min = audioFile.m_pcmData[i1].channel[0];
				short max = audioFile.m_pcmData[i1].channel[0];

				for (size_t i = i1; i < i2; ++i)
				{
					min = Calc::Min(min, audioFile.m_pcmData[i].channel[0]);
					min = Calc::Min(min, audioFile.m_pcmData[i].channel[1]);

					max = Calc::Max(max, audioFile.m_pcmData[i].channel[0]);
					max = Calc::Max(max, audioFile.m_pcmData[i].channel[1]);
				}

				gxVertex2f(x, Calc::Scale((min / double(1 << 15) + 1.f) / 2.f, 0, sy));
				gxVertex2f(x, Calc::Scale((max / double(1 << 15) + 1.f) / 2.f, 0, sy));
			}
		}
		gxEnd();
	}

	void updatePcmDataTexture(const int sx, const int sy)
	{
		delete pcmDataSurface;
		pcmDataSurface = nullptr;

		pcmDataSurface = new Surface(sx, sy, false);

		pushSurface(pcmDataSurface);
		{
			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);

			setColor(colorWhite);

			drawPcmData(sx, sy);
		}
		popSurface();
	}

	void draw(const float sx, const float sy)
	{
		if (pcmDataSurface != nullptr)
		{
			gxSetTexture(pcmDataSurface->getTexture());

			setColor(colorBlue);
			drawRect(0, 0, pcmDataSurface->getWidth(), pcmDataSurface->getHeight());
			gxSetTexture(0);
		}
	}

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		return result;
	}

	virtual void tick(const float dt) override
	{
	}

	virtual bool isInside(const int x, const int y) override
	{
		return false;
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		return false;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		return true;
	}

	virtual void mouseMove(const int x, const int y) override
	{
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y)
	{
		return false;
	}

	virtual void insertEnd(const int x, const int y)
	{
	}

	virtual void insertMove(const int x, const int y)
	{
	}

	virtual void insertCancel()
	{
	}
};

const static int kAudioCreationSx = 700;
const static int kAudioCreationDockSy = 250;
const static int kAudioCreateVoiceSy = 150;
const static int kAudioCreateMaxVoices = 8;

struct AudioCreation : Editable, public AudioStreamEx
{
	enum EditingState
	{
		kEditingState_Idle,
		kEditingState_Drag
	};

	enum DisplayState
	{
		kDisplayState_Closed,
		kDisplayState_Open
	};

	std::list<AudioVoice*> voices;
	float x;
	float y;
	EditingState editingstate;
	float editingBeginX;
	float editingBeginY;
	DisplayState displayState;

	AudioCreation()
		: Editable()
		, voices()
		, x(0.f)
		, y(0.f)
		, editingstate(kEditingState_Idle)
		, editingBeginX(0.f)
		, editingBeginY(0.f)
		//, displayState(kDisplayState_Closed)
		, displayState(kDisplayState_Open)
	{
	}

	virtual ~AudioCreation() override
	{
		for (auto voice : voices)
		{
			delete voice;
			voice = nullptr;
		}

		voices.clear();
	}

	void draw()
	{
		float position = 0.f;
		float duration = 0.f;

		for (auto voice : voices)
		{
			const float voicePosition = voice->audioFile.getTime();
			const float voiceDuration = voice->audioFile.m_duration;

			position = Calc::Max(position, voicePosition);
			duration = Calc::Max(duration, voiceDuration);
		}

		gxPushMatrix();
		{
			gxTranslatef(x, y, 0.f);

			setColor(colorBlack);
			drawRect(0, 0, kAudioCreationSx, kAudioCreationDockSy);
			setColor(colorWhite);
			drawRectLine(0, 0, kAudioCreationSx, kAudioCreationDockSy);

			// todo : draw combined waveform

			gxPushMatrix();
			{
				gxTranslatef(0, 100, 0);

				const float amount = position / duration;

				for (auto voice : voices)
				{
					gxSetTexture(voice->pcmDataSurface->getTexture());
					{
						setColor(colorWhite);
						gxBegin(GL_QUADS);
						{
							gxTexCoord2f(0.f,    0.f); gxVertex2f(0.f,                       0.f  );
							gxTexCoord2f(amount, 0.f); gxVertex2f(kAudioCreationSx * amount, 0.f  );
							gxTexCoord2f(amount, 1.f); gxVertex2f(kAudioCreationSx * amount, 100.f);
							gxTexCoord2f(0.f,    1.f); gxVertex2f(0.f,                       100.f);
						}
						gxEnd();
					}
					gxSetTexture(0);

					gxSetTexture(voice->pcmDataSurface->getTexture());
					{
						setColor(255, 127, 63);
						gxBegin(GL_QUADS);
						{
							gxTexCoord2f(amount, 0.f); gxVertex2f(kAudioCreationSx * amount, 0.f  );
							gxTexCoord2f(1.f,    0.f); gxVertex2f(kAudioCreationSx,          0.f  );
							gxTexCoord2f(1.f,    1.f); gxVertex2f(kAudioCreationSx,          100.f);
							gxTexCoord2f(amount, 1.f); gxVertex2f(kAudioCreationSx * amount, 100.f);
						}
						gxEnd();
					}
					gxSetTexture(0);
				}
			}
			gxPopMatrix();

			// todo : draw playback marker

			// todo : draw play/pause button

			// todo : draw track name

			gxPushMatrix();
			{
				gxPushMatrix();
				{
					gxTranslatef(40, 40, 0);

					setColor(colorWhite);
					drawCircle(10, 10, 20, 100);

					gxPushMatrix();
					{
						gxTranslatef(50, 0, 0);
						setFont("calibri.ttf");
						drawText(0, 0, 48, +1.f, 0.f, "ACASHA - Healer");
					}
					gxPopMatrix();
				}
				gxPopMatrix();

				gxPushMatrix();
				{
					gxTranslatef(kAudioCreationSx, kAudioCreationDockSy, 0);
					gxTranslatef(-25, -25, 0);
					setFont("calibri.ttf");
					drawText(0, 0, 32, -1.f, -1.f, "%02d:%02d - %02d:%02d",
						int(position/60.f),
						int(std::fmodf(position, 60.f)),
						int(duration/60.f),
						int(std::fmodf(duration, 60.f)));
				};
				gxPopMatrix();
			}
			gxPopMatrix();

			gxTranslatef(0.f, kAudioCreationDockSy, 0.f);

			if (displayState == kDisplayState_Open)
			{
				// todo : draw individual voices

				for (auto voice : voices)
				{
					voice->draw(kAudioCreationSx, kAudioCreateVoiceSy);

					gxTranslatef(0, kAudioCreateVoiceSy, 0.f);
				}
			}
		}
		gxPopMatrix();
	}

	// Editable

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		for (AudioVoice * voice : voices)
			result.push_back(voice);

		return result;
	}

	virtual void tick(const float dt) override
	{
	}

	virtual bool isInside(const int x, const int y) override
	{
		float sx = kAudioCreationSx;
		float sy = kAudioCreationDockSy;

		if (displayState == kDisplayState_Open)
			sy += kAudioCreationDockSy * voices.size();

		return
			x >= this->x &&
			y >= this->y &&
			x < this->x + sx &&
			y < this->y + sy;
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		return true;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		if (editingstate == kEditingState_Idle)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual void mouseMove(const int x, const int y) override
	{
		if (editingstate == kEditingState_Idle)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				editingstate = kEditingState_Drag;
				editingBeginX = x;
				editingBeginY = y;
			}
		}
		else if (editingstate == kEditingState_Drag)
		{
			if (mouse.wentUp(BUTTON_LEFT))
				editingstate = kEditingState_Idle;
			else
			{
				const float dx = x - editingBeginX;
				const float dy = y - editingBeginY;

				this->x += dx;
				this->y += dy;

				editingBeginX = x;
				editingBeginY = y;
			}
		}
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y)
	{
		const std::string extension = Path::GetExtension(filename);

		if (extension != "ogg")
		{
			return false;
		}
		else
		{
			AudioVoice * voice = new AudioVoice();

			if (voice->audioFile.load(filename.c_str()))
			{
				voice->updatePcmDataTexture(kAudioCreationSx, kAudioCreateVoiceSy);

				voices.push_back(voice);

				return true;
			}
			else
			{
				delete voice;
				voice = nullptr;

				return false;
			}
		}
	}

	virtual void insertEnd(const int x, const int y)
	{
	}

	virtual void insertMove(const int x, const int y)
	{
	}

	virtual void insertCancel()
	{
	}

	// AudioStreamEx

	static void Add(AudioSample & result, const AudioSample & sample)
	{
		for (int i = 0; i < 2; ++i)
		{
			int value = result.channel[i] + sample.channel[i];

			if (value < std::numeric_limits<short>().min())
				value = std::numeric_limits<short>().min();
			if (value > std::numeric_limits<short>().max())
				value = std::numeric_limits<short>().max();

			result.channel[i] = value;
		}
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		int result = 0;

		memset(buffer, 0, numSamples * sizeof(AudioSample));

		AudioSample * tempBuffer = (AudioSample*)alloca(numSamples * sizeof(AudioSample));

		for (auto voice : voices)
		{
			const int numTempSamples = voice->audioFile.Provide(numSamples, tempBuffer);

			for (int i = 0; i < numTempSamples; ++i)
			{
				Add(buffer[i], tempBuffer[i]);
			}

			result = Calc::Max(result, numTempSamples);
		}

		return result;
	}

	virtual int GetSampleRate()
	{
		return 44100;
	}
};

static const int gridSx = 4;
static const int gridSy = 4;
static const int gridCellSx = 256;
static const int gridCellSy = 256;

static const float kPictureTransformRadius = 80.f;
static const float kPictureTransformThickness = 30.f;

struct Picture : Editable
{
	enum EditingState
	{
		kEditingState_Idle,
		kEditingState_Insert,
		kEditingState_DragFocus,
		kEditingState_DragEdit,
		kEditingState_AngleFocus,
		kEditingState_AngleEdit,
		kEditingState_DrawBegin,
		kEditingState_DrawEnd,
		kEditingState_DrawEdit
	};

	std::string filename;
	Surface * image;

	float x;
	float y;
	float size;
	float angle;
	TweenFloat scale;

	EditingState editingState;
	float editingBeginX;
	float editingBeginY;
	float editingBeginSize;
	float editingBeginAngle;

	TweenFloat drawTransition;
	PicEdit * picEdit;

	Picture()
		: Editable()
		, filename()
		, image(nullptr)
		, x(0.f)
		, y(0.f)
		, size(0.f)
		, angle(0.f)
		, scale(1.f)
		, editingState(kEditingState_Idle)
		, editingBeginX(0.f)
		, editingBeginY(0.f)
		, editingBeginSize(0.f)
		, editingBeginAngle(0.f)
		, drawTransition(0.f)
		, picEdit(nullptr)
	{
	}

	~Picture()
	{
		delete picEdit;
		picEdit = nullptr;

		delete image;
		image = nullptr;
	}

	Mat4x4 getTransform() const
	{
		const std::string extension = Path::GetExtension(filename);

		float scale;
		float sx;
		float sy;

		if (extension == "ps")
		{
			scale = 1.f;
			sx = size;
			sy = size;
		}
		else
		{
			Sprite sprite(filename.c_str());

			const float scaleX = size / sprite.getWidth();
			const float scaleY = size / sprite.getHeight();

			scale = Calc::Min(scaleX, scaleY);
			sx = sprite.getWidth();
			sy = sprite.getHeight();
		}

		Mat4x4 mat(true);

		mat = mat.Translate(this->x, this->y, 0.f).RotateZ(Calc::DegToRad(-angle)).Scale(sx / 2.f * scale, sy / 2.f * scale, 1.f);

		if (editingState == kEditingState_DrawBegin || editingState == kEditingState_DrawEnd || editingState == kEditingState_DrawEdit)
		{
			Mat4x4 matDraw(true);

			matDraw = matDraw.Scale(GFX_SX, GFX_SY, 0.f).Scale(.5f, .5f, 1.f).Translate(+1.f, +1.f, 0.f);

			mat = mat * (1.f - drawTransition) + matDraw * drawTransition;
		}


		return mat;
	}

	void draw() const
	{
		const Mat4x4 mat = getTransform();

		gxPushMatrix();
		{
			gxMultMatrixf(mat.m_v);

			const std::string extension = Path::GetExtension(filename);

			if (extension == "ps")
			{
				Shader shader(filename.c_str(), "effect.vs", filename.c_str());
				shader.setImmediate("colormapSize", GFX_SX, GFX_SY);
				//shader.setImmediate("time", g_creation->time); // fixme
				shader.setImmediate("time", framework.time);

				setShader(shader);
				{
					drawRect(-1.f, -1.f, +1.f, +1.f);
				}
				clearShader();
			}
			else if (image != nullptr)
			{
				setColor(colorWhite);
				gxSetTexture(image->getTexture());
				{
					setBlend(BLEND_PREMULTIPLIED_ALPHA);
					drawRect(-1.f, -1.f, +1.f, +1.f);
					setBlend(BLEND_ALPHA);
				}
				gxSetTexture(0);
			}

			if (hasMouseHover)
			{
				setColor(colorWhite);
				drawRectLine(-1.f, -1.f, +1.f, +1.f);
			}
		}
		gxPopMatrix();

		if (hasMouseFocus)
		{
			if (editingState == kEditingState_AngleFocus)
				setColor(255, 255, 0, 255);
			else if (editingState == kEditingState_AngleEdit)
				setColor(255, 255, 255, 255);
			else
				setColor(0, 255, 0, 255);

			gxPushMatrix();
			{
				gxTranslatef(x, y, 0.f);
				gxRotatef(angle, 0.f, 0.f, 1.f);

				gxBegin(GL_TRIANGLES);
				{
					const int numSteps = 100;
					const float angleStep = Calc::m2PI * 3/4 / numSteps;
					const float radius1 = kPictureTransformRadius - kPictureTransformThickness/2.f;
					const float radius2 = kPictureTransformRadius + kPictureTransformThickness/2.f;

					for (int i = 0; i < numSteps; ++i)
					{
						const float angle1 = angleStep * (i+0);
						const float angle2 = angleStep * (i+1);
						const float x1 = std::cosf(angle1);
						const float y1 = std::sinf(angle1);
						const float x2 = std::cosf(angle2);
						const float y2 = std::sinf(angle2);

						gxVertex2f(x1 * radius1, y1 * radius1);
						gxVertex2f(x2 * radius1, y2 * radius1);
						gxVertex2f(x2 * radius2, y2 * radius2);

						gxVertex2f(x1 * radius1, y1 * radius1);
						gxVertex2f(x2 * radius2, y2 * radius2);
						gxVertex2f(x1 * radius2, y1 * radius2);
					}
				}
				gxEnd();
			}
			gxPopMatrix();
		}

		if (picEdit != nullptr)
		{
			picEdit->draw(drawTransition);
		}
	}

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		return result;
	}

	virtual void tick(const float dt) override
	{
		scale.tick(dt);

		drawTransition.tick(dt);

		const std::string extension = Path::GetExtension(filename);

		if (extension != "ps")
		{
			if (image == nullptr)
			{
				Sprite sprite(filename.c_str());

				image = new Surface(sprite.getWidth(), sprite.getHeight(), false);

				pushSurface(image);
				{
					sprite.draw();
				}
				popSurface();

				ComputeShader cs("randomize.cs");
				setShader(cs);
				{
					cs.setImmediate("value", .2f);
					cs.setTextureRw("image", 0, image->getTexture(), GL_RGBA8, false, false);
					cs.dispatch(image->getWidth(), image->getHeight(), 1);
				}
				clearShader();
			}
		}

		if (editingState == kEditingState_Idle)
		{
			if (hasMouseFocus && keyboard.wentDown(SDLK_DELETE))
				wantsRemove = true;
			else if (hasMouseFocus && keyboard.wentDown(SDLK_d))
			{
				if (image != nullptr)
				{
					editingState = kEditingState_DrawBegin;

					Assert(picEdit == nullptr);
					picEdit = new PicEdit(image->getWidth(), image->getHeight(), image);

					drawTransition.to(1.f, kAnimTransitionTime, kEaseType_SineInOut, 0.f);
				}
			}
		}
		else if (editingState == kEditingState_DrawBegin)
		{
			if (drawTransition == 1.f)
			{
				editingState = kEditingState_DrawEdit;
			}
		}
		else if (editingState == kEditingState_DrawEnd)
		{
			if (drawTransition == 0.f)
			{
				editingState = kEditingState_Idle;

				delete picEdit;
				picEdit = nullptr;
			}
		}
		else if (editingState == kEditingState_DrawEdit)
		{
			Assert(hasMouseFocus);

			if (hasMouseFocus && keyboard.wentDown(SDLK_d))
			{
				editingState = kEditingState_DrawEnd;

				drawTransition.to(0.f, kAnimTransitionTime, kEaseType_SineInOut, 0.f);

				Assert(picEdit != nullptr);

				if (picEdit != nullptr)
				{
					Assert(image != nullptr);
					if (image != nullptr)
					{
						picEdit->blitTo(image);
					}
				}
			}
		}

		if (picEdit != nullptr)
		{
			picEdit->tick(dt);
		}
	}

	virtual bool isInside(const int x, const int y) override
	{
		if (editingState == kEditingState_DrawBegin || editingState == kEditingState_DrawEdit)
		{
			return true;
		}
		else
		{
			const Mat4x4 mat = getTransform().Invert();
			const Vec3 v = mat * Vec3(x, y, 0.f);

			return
				v[0] >= -1.f &&
				v[1] >= -1.f &&
				v[0] <= +1.f &&
				v[1] <= +1.f;
		}
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		scale.clear();

		scale.to(1.1f, .5f, kEaseType_SineInOut, 0.f);

		return true;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		if (editingState == kEditingState_DragEdit || editingState == kEditingState_AngleEdit)
		{
			return false;
		}
		else
		{
			editingState = kEditingState_Idle;

			delete picEdit;
			picEdit = nullptr;

			//

			scale.clear();

			scale.to(1.0f, .5f, kEaseType_SineInOut, 0.f);

			return true;
		}
	}

	virtual void mouseMove(const int x, const int y) override
	{
		if (editingState == kEditingState_Idle || editingState == kEditingState_DragFocus  || editingState == kEditingState_AngleFocus)
		{
			const float dx = x - this->x;
			const float dy = y - this->y;
			const float d = Calc::Sqrt(dx * dx + dy * dy);

			const float radius1 = kPictureTransformRadius - kPictureTransformThickness/2.f;
			const float radius2 = kPictureTransformRadius + kPictureTransformThickness/2.f;

			if (d >= radius1 && d <= radius2)
			{
				editingState = kEditingState_AngleFocus;
			}
			else
			{
				editingState = kEditingState_Idle;
			}
		}
		
		if (editingState == kEditingState_DragFocus || editingState == kEditingState_Idle)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				editingState = kEditingState_DragEdit;
				editingBeginX = x;
				editingBeginY = y;
			}
		}
		else if (editingState == kEditingState_DragEdit)
		{
			if (mouse.wentUp(BUTTON_LEFT))
			{
				editingState = kEditingState_DragFocus;
			}
			else
			{
				const float dx = x - editingBeginX;
				const float dy = y - editingBeginY;

				this->x += dx;
				this->y += dy;

				editingBeginX = x;
				editingBeginY = y;
			}
		}
		else if (editingState == kEditingState_AngleFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				editingState = kEditingState_AngleEdit;
				editingBeginX = x;
				editingBeginY = y;
				editingBeginSize = size;
				editingBeginAngle = angle;
			}
		}
		else if (editingState == kEditingState_AngleEdit)
		{
			if (mouse.wentUp(BUTTON_LEFT))
			{
				editingState = kEditingState_AngleFocus;
			}
			else
			{
				const float beginDx = this->x - editingBeginX;
				const float beginDy = this->y - editingBeginY;
				const float beginD = Calc::Sqrt(beginDx * beginDx + beginDy * beginDy);

				const float endDx = this->x - x;
				const float endDy = this->y - y;
				const float endD = Calc::Sqrt(endDx * endDx + endDy * endDy);

				if (beginD != 0.f && endD != 0.f)
				{
					const float scale = endD / beginD;

					this->size = editingBeginSize * scale;
				}

				//

				Vec2 beginVec(editingBeginX, editingBeginY);
				Vec2 endVec(x, y);

				Vec2 originVec(this->x, this->y);
				beginVec -= originVec;
				endVec -= originVec;

				const float beginAngle = std::atan2f(beginVec[1], beginVec[0]);
				const float endAngle = std::atan2f(endVec[1], endVec[0]);

				const float dot = beginVec * endVec;
				const float angle = endAngle - beginAngle;

				this->angle = editingBeginAngle + angle * 360.f / Calc::m2PI;
			}
		}
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y)
	{
		return false;
	}

	virtual void insertEnd(const int x, const int y)
	{
	}

	virtual void insertMove(const int x, const int y)
	{
	}

	virtual void insertCancel()
	{
	}
};

struct PictureSet : Editable
{
	std::vector<Picture*> pictures;

	PictureSet()
		: Editable()
		, pictures()
	{
	}
	
	virtual ~PictureSet() override
	{
		for (auto picture : pictures)
		{
			delete picture;
			picture = nullptr;
		}

		pictures.clear();
	}

	void draw() const
	{
		for (auto i = pictures.rbegin(); i != pictures.rend(); ++i)
		{
			Picture * picture = *i;

			picture->draw();
		}
	}

	Picture * getEditingPicture()
	{
		Picture * result = nullptr;

		for (Picture * picturePtr : pictures)
		{
			Picture & picture = *picturePtr;

			if (picture.editingState != Picture::kEditingState_Idle)
			{
				Assert(result == nullptr);

				result = &picture;
			}
		}

		return result;
	}

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		for (Picture * picture : pictures)
		{
			result.push_back(picture);
		}

		return result;
	}

	virtual bool removeChild(Editable * child) override
	{
		for (auto i = pictures.begin(); i != pictures.end(); )
		{
			Picture * picture = *i;

			if (picture == child)
			{
				delete child;
				child = nullptr;

				i = pictures.erase(i);

				return true;
			}
			else
			{
				++i;
			}
		}

		return false;
	}

	virtual void tick(const float dt) override
	{
		for (size_t i = 0; i < pictures.size(); ++i)
		{
			if (pictures[i]->hasMouseFocus)
			{
				std::swap(pictures[i], pictures[0]);
				return;
			}
		}
	}

	virtual bool isInside(const int x, const int y) override
	{
		return true;
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		return true;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		return true;
	}

	virtual void mouseMove(const int x, const int y) override
	{
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y)
	{
		Assert(getEditingPicture() == nullptr);

		const std::string extension = Path::GetExtension(filename);

		if (extension != "png" && extension != "jpg" && extension != "jpeg" && extension != "ps")
		{
			return false;
		}
		else
		{
			Picture * picture = new Picture();

			picture->editingState = Picture::kEditingState_Insert;
			picture->filename = filename;
			picture->x = x;
			picture->y = y;
			picture->size = 256.f;
			picture->angle = 0.f;

			pictures.push_back(picture);

			return true;
		}
	}

	virtual void insertEnd(const int x, const int y)
	{
		Picture * editingPicture = getEditingPicture();

		Assert(editingPicture != nullptr);
		if (editingPicture != nullptr)
		{
			editingPicture->editingState = Picture::kEditingState_Idle;
		}
	}

	virtual void insertMove(const int x, const int y)
	{
		Picture * editingPicture = getEditingPicture();
		
		Assert(editingPicture != nullptr);
		if (editingPicture != nullptr)
		{
			editingPicture->x = x;
			editingPicture->y = y;
		}
	}

	virtual void insertCancel()
	{
		Picture * editingPicture = getEditingPicture();

		Assert(editingPicture != nullptr);
		if (editingPicture != nullptr)
		{
			// todo : remove picture from set
		}
	}
};

struct Creation : Editable
{
	Background background;
	AudioCreation audioCreation;
	PictureSet pictureSet;
	Editable * activeEditable;
	float time;

	Creation()
		: Editable()
		, background()
		, audioCreation()
		, pictureSet()
		, activeEditable(nullptr)
		, time(0.f)
	{
		openAudio(&audioCreation);

		g_wantsAudioPlayback = true;
	}

	virtual ~Creation() override
	{
		closeAudio();
	}

	void draw()
	{
		background.draw();

		audioCreation.draw();

		pictureSet.draw();
	}

	virtual std::vector<Editable*> getChildren() override
	{
		std::vector<Editable*> result;

		result.push_back(&background);

		result.push_back(&audioCreation);

		result.push_back(&pictureSet);

		return result;
	}

	virtual void tick(const float dt) override
	{
		time += dt;
	}

	virtual bool isInside(const int x, const int y) override
	{
		return true;
	}

	virtual bool mouseEnter(const int x, const int y) override
	{
		return true;
	}

	virtual bool mouseLeave(const int x, const int y) override
	{
		return true;
	}

	virtual void mouseMove(const int x, const int y)
	{
	}

	virtual bool insertBegin(const std::string & filename, const int x, const int y)
	{
		Assert(activeEditable == nullptr);

		if (background.insertBegin(filename, x, y))
		{
			activeEditable = &background;
			return true;
		}
		else if (audioCreation.insertBegin(filename, x, y))
		{
			activeEditable = &audioCreation;
			return true;
		}
		else if (pictureSet.insertBegin(filename, x, y))
		{
			activeEditable = &pictureSet;
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual void insertEnd(const int x, const int y)
	{
		Assert(activeEditable != nullptr);

		if (activeEditable != nullptr)
		{
			activeEditable->insertEnd(x, y);

			activeEditable = nullptr;
		}
	}

	virtual void insertMove(const int x, const int y)
	{
		Assert(activeEditable != nullptr);

		if (activeEditable != nullptr)
		{
			activeEditable->insertMove(x, y);
		}
	}

	virtual void insertCancel()
	{
		Assert(activeEditable != nullptr);

		if (activeEditable != nullptr)
		{
			activeEditable->insertCancel();

			// todo : remove active editable
			// todo : let editable mark itself as dead, cleanup on tick. allows for animation

			activeEditable = nullptr;
		}
	}
};

static Creation * g_creation = nullptr;

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const std::string filename = args.getString("file", "");
		const int x = mouse.x;
		const int y = mouse.y;

		g_creation->insertBegin(filename, x, y);
		g_creation->insertEnd(x, y);
	}
}

static void createAudioCache()
{
	const auto files = listFiles(".", false);

	for (auto file : files)
	{
		if (Path::GetExtension(file) == "ogg")
		{
			const std::string cacheFile = Path::StripExtension(file) + ".aud";

			if (!FileStream::Exists(cacheFile.c_str()))
			{
				AudioFile audioFile;

				if (audioFile.load(file.c_str()))
				{
					try
					{
						const AudioSample * samples = &audioFile.m_pcmData[0];
						const int numSamples = audioFile.m_pcmData.size();

						FileStream stream(cacheFile.c_str(), OpenMode_Write);
						StreamWriter writer(&stream, false);

						writer.WriteInt32(numSamples);
						writer.WriteInt32(audioFile.m_sampleRate);
						stream.Write(samples, numSamples * sizeof(AudioSample));
					}
					catch (std::exception & e)
					{
						logError(e.what());
					}
				}
			}
		}
	}
}

#endif

#if DO_COMPUTE_PARTICLES

struct Particle
{
	float px;
	float py;
	float pz;

	float vx;
	float vy;
	float vz;
};

static void randomizeParticles(ShaderBufferRw & particleBuffer, const int particleCount)
{
	Particle * particleArray = new Particle[particleCount];
	memset(particleArray, 0, sizeof(Particle) * particleCount);

	for (int i = 0; i < particleCount; ++i)
	{
		const float baseRadius = random(0.f, Calc::m2PI);

		{
			const float radius = random(.1f, .5f);
			//const float angle = random(0.f, Calc::m2PI);
			const float angle = baseRadius;

			particleArray[i].px = std::cos(angle) * radius;
			particleArray[i].py = std::sin(angle) * radius;
			particleArray[i].pz = random(-1.f, +1.f);
		}

		{
			const float speed = std::pow(random(0.f, 1.f), 4.f) * 1.f;
			//const float angle = random(0.f, Calc::m2PI);
			const float angle = baseRadius + Calc::mPI2;

			particleArray[i].vx = std::cos(angle) * speed;
			particleArray[i].vy = std::sin(angle) * speed;
			particleArray[i].vz = random(-1.f, +1.f);
		}
	}

	particleBuffer.setDataRaw(particleArray, sizeof(Particle) * particleCount);

	delete[] particleArray;
}

#endif

#if DO_LIGHT_PROPAGATION

struct LightVertex
{
	LightVertex()
	{
		memset(this, 0, sizeof(*this));
	}

	float px;
	float py;

	Vec3 light;
	Vec3 lightIn;
	Vec3 lightOut;

	Vec3 lightVel;
	Vec3 lightForce;
	int numLightForce;
};

struct LightEdge
{
	int vertex1;
	int vertex2;

	int distance;
};

struct LightMesh
{
	std::vector<LightVertex> vertices;
	std::vector<LightEdge> edges;

	void addLight(const int vertex, const Vec3 amount)
	{
		//vertices[vertex].light += amount * 10.f;
		vertices[vertex].light += amount * 2.f;
	}

	void tick(const float dt)
	{
#if 0
		const float amount = 25.f;

		for (int i = edges.size() - 1; i >= 0; --i)
		{
			const LightEdge & e = edges[i];

			LightVertex & v1 = vertices[e.vertex1];
			LightVertex & v2 = vertices[e.vertex2];

			const Vec3 delta = v2.light - v1.light;

			v1.lightForce += delta * amount;
			v2.lightForce -= delta * amount;
			v1.numLightForce++;
			v2.numLightForce++;
		}

		const float falloff = std::pow(.95f, dt);

		for (LightVertex & v : vertices)
		{
			if (true)
			{
				v.lightForce -= v.light * amount;
				v.numLightForce++;
			}

			if (v.numLightForce != 0)
			{
				v.lightVel += (v.lightForce / v.numLightForce) * dt;
			
				v.lightForce.SetZero();
				v.numLightForce = 0;
			}

			v.light += v.lightVel * dt;

			v.lightVel *= falloff;
		}
#else
		for (int i = edges.size() - 1; i >= 0; --i)
		{
			const LightEdge & e = edges[i];

			LightVertex & v1 = vertices[e.vertex1];
			LightVertex & v2 = vertices[e.vertex2];

			const float x = Calc::Min(1.f, dt * 15.f);
			//const float x = 1.f;

			//const Vec3 amount = v1.light * x;
			const Vec3 amount = Vec3(
				v1.light[0] * x / 1.0f,
				v1.light[1] * x / 1.1f,
				v1.light[2] * x / 1.2f);
			
			v2.lightIn += amount;
			v1.lightOut = amount;
		}

		for (LightVertex & v : vertices)
		{
			v.light += v.lightIn;
			v.light -= v.lightOut;
			v.lightIn.SetZero();
			//v.lightIn -= v.
		}
#endif
	}
};

static void subdivideLightMesh(const LightMesh & in, LightMesh & out, const float maxDistance)
{
	out = LightMesh();
	out.vertices = in.vertices;

	for (const LightEdge & e : in.edges)
	{
		const LightVertex & v1 = in.vertices[e.vertex1];
		const LightVertex & v2 = in.vertices[e.vertex2];

		const float dx = v2.px - v1.px;
		const float dy = v2.py - v1.py;
		const float ds = std::hypot(dx, dy);

		const int numSteps = int(std::ceil(ds / maxDistance)) + 1;

		const float sx = dx / (numSteps - 1);
		const float sy = dy / (numSteps - 1);

		int vi1 = e.vertex1;

		for (int i = 0; i < numSteps; ++i)
		{
			int vi2;

			if (i == numSteps - 1)
			{
				vi2 = e.vertex2;
			}
			else
			{
				const float px = v1.px + sx * i;
				const float py = v1.py + sy * i;

				LightVertex v;
				v.px = px;
				v.py = py;

				out.vertices.push_back(v);

				vi2 = out.vertices.size() - 1;
			}

			LightEdge se;
			se.vertex1 = vi1;
			se.vertex2 = vi2;
			out.edges.push_back(se);

			vi1 = vi2;
		}
	}
}

static void createLightMesh(LightMesh & out)
{
	LightMesh mesh;

	LightVertex vertex;
	vertex.px = GFX_SX/2.f;
	vertex.py = GFX_SY;
	mesh.vertices.push_back(vertex);

	for (int i = 0; i < 20; ++i)
	{
		if (false)
		{
			LightVertex vertex;
			vertex.px = random(0.f, float(GFX_SX));
			vertex.py = random(0.f, float(GFX_SY));
			mesh.vertices.push_back(vertex);
		}

		//if (mesh.vertices.size() >= 2)
		{
			int vertex1;
			
			if (rand() % 2)
				vertex1 = rand() % mesh.vertices.size();
			else
				vertex1 = (mesh.vertices.size() - 1);

			//const int numBranches = random(1, 4);
			//const int numBranches = 1;
			const int numBranches = 4;

			for (int i = 0; i < numBranches; ++i)
			{
				int vertex2;

				{
					const float s = 300.f;
					const float sy = 200.f;

					LightVertex vertex = mesh.vertices[vertex1];
					vertex.px += random(-s*3.f/4.f, +s*3.f/4.f);
					vertex.py += random(-sy, -sy*2.f/4.f);

					mesh.vertices.push_back(vertex);
					vertex2 = (mesh.vertices.size() - 1);
				}

				LightEdge edge;
				edge.vertex1 = vertex1;
				edge.vertex2 = vertex2;

				mesh.edges.push_back(edge);
			}
		}
	}

	subdivideLightMesh(mesh, out, 10.f);
}

#endif

#if DO_FLOCKING

struct FlockElem
{
	float px;
	float py;
	float pxOld;
	float pyOld;
	float vx;
	float vy;
};

struct Flock
{
	const static int kNumFlockElems = 200;

	FlockElem elems[kNumFlockElems];

	void randomize()
	{
		for (FlockElem & e : elems)
		{
			e.px = GFX_SX/2 + random(-GFX_SX/4, +GFX_SX/4);
			e.py = random(0, GFX_SY);

			e.vx = 0.f;
			e.vy = 0.f;
		}
	}

	void tick(const float dt)
	{
		float mx = 0.f;
		float my = 0.f;

		for (const FlockElem & e : elems)
		{
			mx += e.px;
			my += e.py;
		}

		mx /= kNumFlockElems;
		my /= kNumFlockElems;

		mx = GFX_SX/2;
		my = GFX_SY/2;

		mx += std::cos(framework.time * 2.f / 3.45f) * 200.f;
		my += std::sin(framework.time * 2.f / 3.21f) * 200.f;

		//

		const float falloff = std::pow(.2f, dt);

		for (FlockElem & e : elems)
		{
			float md = 0.f;
			const FlockElem * me = nullptr;

			for (const FlockElem & other : elems)
			{
				if (&other == &e)
					continue;

				const float dx = other.px - e.px;
				const float dy = other.py - e.py;
				const float ds = std::hypot(dx, dy);

				if (false)
				{
					if (ds > 0.f)
					{
						const float dc = ds - 50.f;
						const float dcs = std::abs(dc);

						const float strength = 1.f / (dcs + .1f) * 4000.f;

						//const float strength = Calc::Saturate(Calc::Lerp(1.f, 0.f, dcs/50.f));

						e.vx += dx / ds * strength * dt;
						e.vy += dy / ds * strength * dt;
					}
				}
				else
				{
					if (ds > 0.f && (ds < md || md == 0.f))
					{
						md = ds;
						me = &other;
					}
				}
			}

#if 1
			if (me != nullptr)
			{
				const float dx = me->px - e.px;
				const float dy = me->py - e.py;
				const float ds = std::hypot(dx, dy);
				const float dc = ds - 50.f;

				//const float strength = (std::abs(dc) < 100.f ? Calc::Sign(dc) : 0.f) * 100.f;

				const float strength = Calc::Sign(dc) * 100.f;
				//const float strength = Calc::Sign(dc) / (std::abs(dc) / 1000.f + .001f);

				e.vx += dx / ds * strength * dt;
				e.vy += dy / ds * strength * dt;
			}
#endif

			if (true)
			{
				const float dx = mx - e.px;
				const float dy = my - e.py;
				const float ds = std::hypot(dx, dy);

				const float strength = 100.f;

				e.vx += dx / ds * strength * dt;
				e.vy += dy / ds * strength * dt;
			}
		}

		for (FlockElem & e : elems)
		{
			e.pxOld = e.px;
			e.pyOld = e.py;

			e.px += e.vx * dt;
			e.py += e.vy * dt;

			if (false)
			{
				const float v = std::hypot(e.vx, e.vy);
				const float kMaxSpeed = 100.f;

				if (v > kMaxSpeed)
				{
					e.vx = e.vx / v * kMaxSpeed;
					e.vy = e.vy / v * kMaxSpeed;
				}
			}
			
			e.vx *= falloff;
			e.vy *= falloff;
		}
	}

	void draw() const
	{
		//gxBegin(GL_LINES);
		gxBegin(GL_POINTS);
		{
			for (const FlockElem & e : elems)
			{
				setColor(colorWhite);
				gxVertex2f(e.pxOld, e.pyOld);
				gxVertex2f(e.px, e.py);
			}
		}
		gxEnd();
	}
};

#endif

int main(int argc, char * argv[])
{
	changeDirectory("data");

#if DO_COCREATE
	createAudioCache();
#endif

	framework.enableRealTimeEditing = true;

#if DO_COCREATE
	framework.filedrop = true;
	framework.actionHandler = handleAction;
#endif

	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;

#if !defined(DEBUG)
	framework.fullscreen = true;
#endif

	//framework.minification = 2;

#if defined(DEBUG)
	framework.windowX = 0;
#endif

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		Surface ** downresSurfaces = nullptr;
		Surface ** blurredSurfaces = nullptr;

		int numSurfaces = 0;

		while ((1 << numSurfaces) < GFX_SX || (1 << numSurfaces) < GFX_SY)
		{
			numSurfaces++;
		}

		downresSurfaces = new Surface*[numSurfaces];
		blurredSurfaces = new Surface*[numSurfaces];

		int sx = GFX_SX;
		int sy = GFX_SY;

		for (int i = 0; i < numSurfaces; ++i, sx = Calc::Max(sx/2, 1), sy = Calc::Max(sy/2, 1))
		{
			downresSurfaces[i] = new Surface(sx, sy, true);
			blurredSurfaces[i] = new Surface(sx, sy, true);
		}

#if DO_COCREATE
		g_creation = new Creation();
#endif

		framework.process();

#if DO_COCREATE
		g_creation->insertBegin("camels.jpg", 100, 100);
		g_creation->insertEnd(100, 100);

#if 1
		for (int i = 0; i < 1; ++i)
		{
			g_creation->insertBegin("camels.jpg", rand() % GFX_SX, rand() % GFX_SY);
			g_creation->insertEnd(100, 100);

			g_creation->insertBegin("circles.ps", rand() % GFX_SX, rand() % GFX_SY);
			g_creation->insertEnd(100, 400);
		}
#endif

		g_creation->insertBegin("background1.ps", rand() % GFX_SX, rand() % GFX_SY);
		g_creation->insertEnd(100, 400);

#if 0
		g_creation->insertBegin("voice.ogg", 100, 400);
		g_creation->insertEnd(100, 400);
		g_creation->insertBegin("voice.ogg", 100, 400);
		g_creation->insertEnd(100, 400);
#endif

#if 0
		PicEdit edit(GFX_SX, GFX_SY);

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				framework.quitRequested = true;
			}

			edit.tick(framework.timeStep);

			framework.beginDraw(0, 0, 0, 0);
			{
				edit.draw();
			}
			framework.endDraw();
		}
#endif

#endif

#if DO_COMPUTE_PARTICLES
		ShaderBufferRw particleBuffer;

		const int particleCount = 100 * 10000;

		randomizeParticles(particleBuffer, particleCount);
#endif

#if DO_LIGHT_PROPAGATION
		LightMesh mesh;

		createLightMesh(mesh);
#endif

#if DO_FLOCKING
		Flock flock;

		flock.randomize();
#endif

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
			{
				framework.quitRequested = true;
			}

			const float dt = framework.timeStep;

#if DO_COCREATE
			g_creation->evaluateMouseHover(mouse.x, mouse.y);

			if (mouse.wentDown(BUTTON_LEFT))
			{
				g_creation->evaluateMouseFocus(mouse.x, mouse.y);
			}

			g_creation->mouseMoveBase(mouse.x, mouse.y);
			
			g_creation->tickBase(dt);
#endif

			// compute

#if DO_COMPUTE_PARTICLES
			if (keyboard.wentDown(SDLK_p))
				randomizeParticles(particleBuffer, particleCount);

			ComputeShader particleCS("particleSim.cs", 64, 1, 1);
			setShader(particleCS);
			{
				particleCS.setBufferRw("particleBuffer", particleBuffer);
				particleCS.setImmediate("dt", dt);
				particleCS.setImmediate("time", framework.time);
				particleCS.setImmediate("gravityStrength", 2.f * mouse.x / float(GFX_SX));
				particleCS.dispatch(particleCount, 1, 1);
			}
			clearShader();
#endif

#if DO_LIGHT_PROPAGATION
			if (keyboard.wentDown(SDLK_m))
			{
				createLightMesh(mesh);
			}

			if (keyboard.wentDown(SDLK_l))
			{
				const float hue = random(-.1f, .4f);
				const Color color = Color::fromHSL(hue, .4f, .5f);

				mesh.addLight(0, Vec3(color.r, color.g, color.b) * 20.f);
			}

			if (keyboard.wentDown(SDLK_k))
			{
				const float hue = random(-.1f, .4f);
				const Color color = Color::fromHSL(hue, .4f, .5f);

				for (int i = 0; i < 4; ++i)
				{
					mesh.addLight(rand() % mesh.vertices.size(), Vec3(color.r, color.g, color.b) * 20.f);
				}
			}
			
			for (int i = 0; i < 8; ++i)
				mesh.tick(dt);
#endif

#if DO_FLOCKING
			if (keyboard.wentDown(SDLK_f))
			{
				flock.randomize();
			}
			
			flock.tick(dt);
#endif

			framework.beginDraw(0, 0, 0, 0);
			{
				Surface * surface = downresSurfaces[0];

#if DO_COCREATE
				pushSurface(surface);
				{
					surface->clear(127, 127, 127, 255);

					g_creation->draw();
				}
				popSurface();
#else
				pushSurface(surface);
				{
					surface->clear(0, 0, 0, 255);
				}
				popSurface();
#endif

#if DO_COMPUTE_PARTICLES
				pushSurface(surface);
				{
					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

						Shader particle("particle");
						setShader(particle);
						{
							particle.setBufferRw("particleBuffer", particleBuffer);
							particle.setImmediate("colorStrength", 8.f * mouse.y / float(GFX_SY));

							setBlend(BLEND_ADD);
							gxEmitVertices(GL_POINTS, particleCount);
							setBlend(BLEND_ALPHA);
						}
						clearShader();
					}
					gxPopMatrix();
				}
				popSurface();
#endif

#if DO_LIGHT_PROPAGATION
				pushSurface(surface);
				{
					glPointSize(4.f);

					gxBegin(GL_POINTS);
					{
						for (const LightVertex & v : mesh.vertices)
						{
							const float a = .4f;
							const float b = 1.f + a;

							const Vec3 c = (v.light + Vec3(a, a, a)) / b;

							setColorf(c[0], c[1], c[2], 1.f);

							gxVertex2f(v.px, v.py);
						}
					}
					gxEnd();

					glPointSize(1.f);
				}
				popSurface();
#endif

#if DO_FLOCKING
				pushSurface(surface);
				{
					glPointSize(5.f);
					{
						flock.draw();
					}
					glPointSize(1.f);
				}
				popSurface();
#endif

			#if DO_GAUSSIAN_BLUR
				setBlend(BLEND_OPAQUE);
				{
					// create downsample chain

					Shader downresShader("downres.ps", "effect.vs", "downres.ps");

					setShader(downresShader);
					{
						for (int i = 1; i < numSurfaces; ++i)
						{
							pushSurface(downresSurfaces[i]);
							{
								downresShader.setTexture("colormap", 0, downresSurfaces[i - 1]->getTexture(), false, true);

								drawRect(0, 0, downresSurfaces[i]->getWidth(), downresSurfaces[i]->getHeight());
							}
							popSurface();
						}
					}
					clearShader();

				#if 1
					for (int i = 0; i < numSurfaces; ++i)
					{
						Surface * srcSurface = downresSurfaces[i];
						Surface * dstSurface = blurredSurfaces[i];

						for (int j = 0; j < 2; ++j)
						{
							const char * shaderName = (j % 2) == 0 ? "guassian-h.ps" : "guassian-v.ps";

							Shader shader(shaderName, "effect.vs", shaderName);
							shader.setTexture("colormap", 0, (j == 0) ? srcSurface->getTexture() : dstSurface->getTexture(), true, true);
							shader.setImmediate("amount", mouse.x / float(GFX_SX) * 2.f);

							dstSurface->postprocess(shader);
						}
					}
				#endif

					for (int i = 0; i < numSurfaces; ++i)
					//for (int i = 0; i < 1; ++i)
					{
						Surface * surface = downresSurfaces[i];

						gxSetTexture(surface->getTexture());
						{
							setColor(colorWhite);
							drawRect(0, 0, surface->getWidth(), surface->getHeight());
						}
						gxSetTexture(0);
					}
				}
				setBlend(BLEND_ALPHA);

				//setBlend(BLEND_ADD);
				setBlend(BLEND_OPAQUE);
				//setBlend(BLEND_ALPHA);
				{
					for (int i = 3; i < 4; ++i)
					{
						Surface * surface = blurredSurfaces[i];
						//Surface * surface = downresSurfaces[i];

						gxSetTexture(surface->getTexture());
						{
							//const float c = .5f;
							//setColorf(c, c, c);
							//setColorf(1.f, 1.f, 1.f, .5f);
							setColor(colorWhite);
							//drawRect(0, 0, GFX_SX, GFX_SY);
						}
						gxSetTexture(0);
					}
				}
				setBlend(BLEND_ALPHA);
			#else
				static bool doPath = true;

				if (keyboard.wentDown(SDLK_p))
					doPath = !doPath;

				pushSurface(surface);
				{
#if 0
					gxSetTexture(getTexture("lights1.jpg"));
					{
						drawRect(0, 0, surface->getWidth(), surface->getHeight());
					}
					gxSetTexture(0);
#endif

					if (doPath)
					{
						gxPushMatrix();
						{
							gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
							//gxScalef(1.f, 1.f, 0.f);
							//gxRotatef(framework.time * 40.f, 0.f, 1.f, 1.f);
							const float scale = mouse.x / float(GFX_SX) * 10.f;
							gxScalef(scale, scale, 1.f);

							setColor(colorWhite);
							Path2d path;
							path.moveTo(-100.f, -100.f);
							path.line(+100.f,    0.f);
							path.line(   0.f, +100.f);
							path.line(-100.f,    0.f);
							path.curve(-200.f, +100.f, -200.f, 0.f, 0.f, -100.f -200.f * std::cos(framework.time * 0.f));
							path.curveTo(0.f, 0.f, 0.f, +100.f, 0.f, +100.f +500.f * std::cos(framework.time * .1f));

							setBlend(BLEND_ALPHA);

							if (keyboard.isDown(SDLK_h))
								hqDrawPath(path);
							else
								drawPath(path);
						}
						gxPopMatrix();
					}
				}
				popSurface();

			#if DO_HQ_PRIMITIVES
				pushSurface(surface);
				{
					static bool doTransform = false;

					static float txTime = 0.f;
					static float txSpeed = 0.f;

					static bool fixedStrokeSize = false;

					//

					if (keyboard.wentDown(SDLK_t))
						doTransform = !doTransform;
					if (keyboard.wentDown(SDLK_s))
						fixedStrokeSize = !fixedStrokeSize;

					//

					if (doTransform)
						txSpeed = std::min(1.f, txSpeed + dt/2.f);
					else
						txSpeed = std::max(0.f, txSpeed - dt/2.f);
					txTime += txSpeed * dt;

					//

					{
						setBlend(BLEND_ALPHA);

						setFont("calibri.ttf");
						setColor(colorWhite);
						drawText(10.f, 10.f, 24, +1, +1, "mode: %s", false ? "software transform / single draw" : "hardware transform / multi draw");

						if (keyboard.isDown(SDLK_a))
							setBlend(BLEND_PREMULTIPLIED_ALPHA_DRAW);

						struct Ve
						{
							Vec2 p;
							Vec2 v;
							float strokeSize;
						};

						const int kMaxVe = 256;
						static int numVe = 0;
						static Ve ve[kMaxVe];
						static bool veInit = true;

						if (veInit || keyboard.isDown(SDLK_i))
						{
							veInit = false;

							numVe = random(0, kMaxVe);

							for (int i = 0; i < numVe; ++i)
							{
								ve[i].p[0] = random(0, GFX_SX);
								ve[i].p[1] = random(0, GFX_SY);
								ve[i].v[0] = random(-30.f, +10.f) * .1f;
								ve[i].v[1] = random(-80.f, +10.f) * .1f;
								ve[i].strokeSize = random(.1f, 1.f);
							}
						}
						else
						{
							for (int i = 0; i < numVe; ++i)
							{
								ve[i].p += ve[i].v * dt;
							}
						}

						//const float strokeSize = 2.f - std::cos(framework.time * .5f) * 1.f;
						const float strokeSize = 0.f + mouse.y / float(GFX_SY) * 10.f;

						{
							gxPushMatrix();
							{
								gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
								gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
								gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
								gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);

								hqBegin(HQ_LINES);
								{
									for (int i = 0; i < numVe/2; ++i)
									{
										const Ve & ve1 = ve[i * 2 + 0];
										const Ve & ve2 = ve[i * 2 + 1];

										const float strokeSize1 = fixedStrokeSize ?  0.f : strokeSize * ve1.strokeSize;
										const float strokeSize2 = fixedStrokeSize ? 12.f : strokeSize * ve2.strokeSize;

										setColor(colorWhite);
										hqLine(ve1.p[0], ve1.p[1], strokeSize1, ve2.p[0], ve2.p[1], strokeSize2);
									}
								}
								hqEnd();
							}
							gxPopMatrix();
						}
					}

#if 1
					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

						hqBegin(HQ_FILLED_TRIANGLES);
						{
							for (int i = 1; i <= 10; ++i)
							{
								setColorf(1.f / i, .5f / i, .25f / i);
								hqFillTriangle(-200.f / i, 0.f, +200.f / i, 0.f, 0.f, +400.f / i);
							}
						}
						hqEnd();

						hqBegin(HQ_STROKED_TRIANGLES);
						{
							for (int i = 1; i <= 10; ++i)
							{
								setColorf(1.f, 1.f, 1.f, 1.f);
								hqStrokeTriangle(-200.f / i, 0.f, +200.f / i, 0.f, 0.f, +400.f / i, 4.f);
							}
						}
						hqEnd();

						hqBegin(HQ_FILLED_CIRCLES);
						{
							for (int i = 1; i <= 10; ++i)
							{
								setColorf(.25f / i, .5f / i, 1.f / i);
								hqFillCircle(0.f, 0.f, 100.f / i);
							}
						}
						hqEnd();

						hqBegin(HQ_STROKED_CIRCLES);
						{
							for (int i = 1; i <= 10; ++i)
							{
								setColor(colorWhite);
								hqStrokeCircle(0.f, 0.f, 100.f / i, 10.f / i + .5f);
							}
						}
						hqEnd();

						hqBegin(HQ_LINES);
						{
							setColor(colorWhite);

							for (int i = 1; i <= 10; ++i)
							{
								hqLine(-100.f, 400.f / i, (i % 2) ? 10-i : 0.f, +100.f, 400.f / i, (i % 2) ? 0.f : 10-i);
							}
						}
						hqEnd();

						gxPushMatrix();
						{
							gxRotatef(framework.time, 0.f, 0.f, 1.f);

							gxPushMatrix();
							{
								gxTranslatef(200.f, 0.f, 0.f);

								hqBegin(HQ_FILLED_RECTS);
								{
									setColor(colorWhite);
									hqFillRect(-50.f, -50.f, +50.f, +50.f);
								}
								hqEnd();
							}
							gxPopMatrix();

							hqBegin(HQ_STROKED_RECTS);
							{
								setColor(colorBlack);
								for (int i = 0; i < 10; ++i)
									hqStrokeRect(-50.f - i * 10, -50.f - i * 10, +50.f + i * 10, +50.f + i * 10, i + .5f);
							}
							hqEnd();
						}
						gxPopMatrix();
					}
					gxPopMatrix();

					gxPushMatrix();
					{
						const float scale = mouse.y / float(GFX_SY) * 10.f;

						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
						gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
						//gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
						gxScalef(scale, scale, 1.f);

						setColorf(1.f / 10.f, .5f / 10.f, .25f / 10.f);

						hqBegin(HQ_FILLED_TRIANGLES);
						{
							const Vec2 po1(-10.f, 0.f);
							const Vec2 po2(+10.f, 0.f);
							const Vec2 po3(0.f, +20.f);

							for (int t = 0; t < 11 * 11; ++t)
							{
								const int ox = (t % 11) - 5;
								const int oy = (t / 11) - 5;

								if (std::abs(ox) + std::abs(oy) <= 3)
									continue;

								const Vec2 o(ox * 20.f, oy * 20.f);

								const Vec2 p1 = po1 + o;
								const Vec2 p2 = po2 + o;
								const Vec2 p3 = po3 + o;

								hqFillTriangle(p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
							}
						}
						hqEnd();
					}
					gxPopMatrix();
#endif

#if 1
					gxPushMatrix();
					{
						const float scale = mouse.y / float(GFX_SY) * 10.f;

						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
						gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
						//gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
						gxScalef(scale, scale, 1.f);

						hqBegin(HQ_FILLED_CIRCLES, true);
						{
							const Vec2 po(0.f, 0.f);
							const float radius = 15.f;

							for (int t = 0; t < 11 * 11; ++t)
							{
								const int ox = (t % 11) - 5;
								const int oy = (t / 11) - 5;

								if (ox == 0 && oy == 0)
									continue;

								const Vec2 o(ox * 20.f, oy * 20.f);
								const Vec2 p = po + o;
									
								hqFillCircle(p[0], p[1], radius);
							}
						}
						hqEnd();
					}
					gxPopMatrix();
#endif

#if 0 // GL vs HQ rect
					surface->clear();

					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

#if 0
						setColor(colorWhite);
						drawRect(-100.f, -100.f, +100.f, +100.f);
#else
						hqBegin(HQ_FILLED_RECTS);
						{
							setColor(colorWhite);
							hqFillRect(-100.f, -100.f, +100.f, +100.f);
						}
						hqEnd();
#endif

						hqBegin(HQ_STROKED_RECTS);
						{
							setColor(255, 0, 0, 127);
							hqStrokeRect(-100.f, -100.f, +100.f, +100.f, 3.f);
						}
						hqEnd();
					}
					gxPopMatrix();
#endif

#if 0 // GL vs HQ circle
					surface->clear();

					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

#if 0
						setColor(colorWhite);
						fillCircle(0.f, 0.f, 100.f, 100);
#else
						hqBegin(HQ_FILLED_CIRCLES);
						{
							setColor(colorWhite);
							hqFillCircle(0.f, 0.f, 100.f);
						}
						hqEnd();
#endif

						hqBegin(HQ_STROKED_CIRCLES);
						{
							setColor(255, 0, 0, 127);
							hqStrokeCircle(0.f, 0.f, 100.f, 3.f);
						}
						hqEnd();
					}
					gxPopMatrix();
#endif

#if 0 // GL vs HQ triangle
					surface->clear();

					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

#if 0
						setColor(colorWhite);
						gxBegin(GL_TRIANGLES);
						{
							gxVertex2f(-100.f,    0.f);
							gxVertex2f(+100.f,    0.f);
							gxVertex2f(   0.f, +100.f);
						}
						gxEnd();
#else
						hqBegin(HQ_FILLED_TRIANGLES);
						{
							setColor(colorWhite);
							hqFillTriangle(-100.f, 0.f, +100.f, 0.f, 0.f, +100.f);
						}
						hqEnd();
#endif

						hqBegin(HQ_STROKED_TRIANGLES);
						{
							setColor(255, 0, 0, 127);
							hqStrokeTriangle(-100.f, 0.f, +100.f, 0.f, 0.f, +100.f, 3.f);
						}
						hqEnd();
					}
					gxPopMatrix();
#endif
				}
				popSurface();
			#endif

			#if DO_BUILTIN_SHADER
				const float treshold = (1.f - std::cos(framework.time)) / 2.f;
				setShader_Invert(surface->getTexture());
				//setShader_TresholdLumi(surface->getTexture(), treshold, colorBlack, colorYellow);
				//setShader_TresholdValue(surface->getTexture(), Color(treshold, treshold*2.f, treshold*3.f, 0.f), colorBlack, colorWhite);
				//setShader_TresholdLumiFail(surface->getTexture(), treshold, colorBlack);
				//setShader_TresholdLumiPass(surface->getTexture(), treshold, colorWhite);
				//setShader_TresholdValuePass(surface->getTexture(), Color(treshold, treshold*2.f, treshold*3.f, 0.f), colorBlack);
				//setShader_GrayscaleLumi(surface->getTexture());
				const float weight1 = (1.f - std::cos(framework.time / 1.1f))/2.f;
				const float weight2 = (1.f - std::cos(framework.time / 2.3f))/2.f;
				const float weight3 = (1.f - std::cos(framework.time / 3.4f))/2.f;
				const Vec3 weights = Vec3(weight1, weight2, weight3).CalcNormalized();
				//setShader_GrayscaleWeights(surface->getTexture(), weights);
				//setShader_Colorize(surface->getTexture(), framework.time * .1f);
				//setShader_HueShift(surface->getTexture(), framework.time * .1f);
				{
					setBlend(BLEND_OPAQUE);
					surface->postprocess();
				}
				clearShader();
			#endif

				setBlend(BLEND_OPAQUE);
				gxSetTexture(surface->getTexture());
				{
					setColor(colorWhite);
					if (keyboard.isDown(SDLK_z))
					{
						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
						gxScalef(4.f, 4.f, 1.f);
						gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);
						drawRect(0, 0, surface->getWidth(), surface->getHeight());
					}
					else
					{
						drawRect(0, 0, surface->getWidth(), surface->getHeight());
					}
				}
				gxSetTexture(0);
				setBlend(BLEND_ALPHA);
			#endif
			}
			framework.endDraw();
		}

#if DO_COCREATE
		delete g_creation;
		g_creation = nullptr;
#endif
	}
	framework.shutdown();

	return 0;
}
