#include "avtypes.h"
#include "Calc.h"
#include "framework.h"
#include "leapstate.h"
#include "StringEx.h"
#include "Traveller.h"
#include "video.h"

#if ENABLE_LEAPMOTION
	#include "leap/Leap.h"
#endif

#include <turbojpeg/turbojpeg.h>

#define GFX_SX 1024
#define GFX_SY 768

#define LOOP_SX_TOTAL 1440
#define LOOP_SX 822
#define LOOP_SY 874

#define DO_VIDEOLOOPS 0
#define DO_GAME 1

#define NUM_LAYERS 3

#define STREAM_ID "b"

//#define kMixVideoSurfaceOpacity (mouse.x / float(GFX_SX))
//#define kMixPostSurfaceRetain (mouse.y / float(GFX_SY))

#define kMixVideoSurfaceOpacity mixingPanel.sliders[0].value
#define kMixPostSurfaceRetain mixingPanel.sliders[1].value
#define kMixLogoOpacity mixingPanel.sliders[2].value
#define kMixUserContentOpacity mixingPanel.sliders[3].value
#define kMixVideoSpeed1 mixingPanel.sliders[4].value
#define kMixVideoSpeed2 mixingPanel.sliders[5].value
#define kMixGodraysStrength mixingPanel.sliders[6].value

// todo : mask using rotating and scaling objects as mask alpha
// todo : grooop logo animation
// todo : think of ways to mix/vj
// todo : use touchpad for map-like moving and scaling videos?
// todo : multitouch touch pad? can SDL handle this? else look for api

struct UIMixingSlider
{
	int px;
	int py;
	int sx;
	int sy;
	
	float value;
	
	bool active;
	
	UIMixingSlider()
	{
		memset(this, 0, sizeof(*this));
	}
	
	void tick(const float dt, const int mx, const int my)
	{
		const bool isInside =
			mx >= px &&
			my >= py &&
			mx < px + sx &&
			my < py + sy;
		
		const float yRelative = (my - py) / float(sy - 1.f);
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (isInside)
				active = true;
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
			active = false;
		
		if (active)
			value = clamp(1.f - yRelative, 0.f, 1.f);
	}
	
	void draw() const
	{
		setColor(255, 0, 0);
		drawRect(px, py + sy * (1.f - value), px + sx, py + sy);
		
		setColor(255, 255, 255);
		drawRectLine(px, py, px + sx, py + sy);
	}
};

struct UIMixingPanel
{
	static const int kNumSliders = 10;
	
	int px;
	int py;
	
	UIMixingSlider sliders[kNumSliders];
	
	UIMixingPanel()
		: px(0)
		, py(0)
	{
		int sx = 20;
		int sy = 200;
		int px = 4;
		
		for (int i = 0; i < kNumSliders; ++i)
		{
			UIMixingSlider & slider = sliders[i];
			
			slider.px = i * (sx + px/2);
			slider.py = 0;
			slider.sx = sx;
			slider.sy = sy;
		}
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < kNumSliders; ++i)
			sliders[i].tick(dt, mouse.x - px, mouse.y - py);
	}
	
	void draw() const
	{
		gxPushMatrix();
		{
			gxTranslatef(px, py, 0);
			
			for (int i = 0; i < kNumSliders; ++i)
				sliders[i].draw();
		}
		gxPopMatrix();
	}
};

static UIMixingPanel mixingPanel;

struct Grain
{
	float life;
	float lifeRcp;
	
	float px;
	float py;
	float vx;
	float vy;
	float vfalloff;
};

struct GrainsEffect
{
	const static int kMaxGrains = 1000;
	
	Grain grains[kMaxGrains];
	
	GrainsEffect()
	{
		memset(grains, 0, sizeof(grains));
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < kMaxGrains; ++i)
		{
			grains[i].life -= dt;
			grains[i].vx *= std::powf(1.f - grains[i].vfalloff, dt);
			grains[i].vy *= std::powf(1.f - grains[i].vfalloff, dt);
			grains[i].px += grains[i].vx * dt;
			grains[i].py += grains[i].vy * dt;
		}
	}
	
	void draw() const
	{
		Shader shader("gradient-circle");
		setShader(shader);
		{
			for (int i = 0; i < kMaxGrains; ++i)
			{
				if (grains[i].life > 0.f)
				{
					const float a = grains[i].life * grains[i].lifeRcp;
					const float radius = 40.f * a;
					
					setColorf(1.f, 1.f, 1.f, a * .2f);
					drawRect(
						grains[i].px - radius,
						grains[i].py - radius,
						grains[i].px + radius,
						grains[i].py + radius);
				}
			}
		}
		clearShader();
	}
};

static void applyMask(GLuint a, GLuint b, GLuint mask)
{
	Shader shader("mask");
	setShader(shader);
	{
		shader.setTexture("colormapA", 0, a);
		shader.setTexture("colormapB", 1, b);
		shader.setTexture("mask", 2, mask);
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f * GFX_SX, 0.f * GFX_SY);
			gxTexCoord2f(1.f, 1.f); gxVertex2f(1.f * GFX_SX, 0.f * GFX_SY);
			gxTexCoord2f(1.f, 0.f); gxVertex2f(1.f * GFX_SX, 1.f * GFX_SY);
			gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f * GFX_SX, 1.f * GFX_SY);
		}
		gxEnd();
	}
	clearShader();
}

static void applyFsfx(Surface & surface, const char * name, const float strength = 1.f, const float param1 = 0.f, const float param2 = 0.f, const float param3 = 0.f, const float param4 = 0.f, GLuint texture1 = 0)
{
	Shader shader(name, "fsfx/fsfx.vs", name);
	setShader(shader);
	{
		shader.setImmediate("params1", strength, 0.f, 0.f, 0.f);
		shader.setImmediate("params2", param1, param2, param3, param4);
		shader.setTexture("colormap", 0, surface.getTexture());
		shader.setTexture("texture1", 1, texture1);
		surface.postprocess();
	}
	clearShader();
}

struct VideoLoop
{
	std::string filename;
	MediaPlayer * mediaPlayer;
	MediaPlayer * mediaPlayer2;
	Surface * firstFrame;
	
	VideoLoop(const char * _filename)
		: filename()
		, mediaPlayer(nullptr)
		, mediaPlayer2(nullptr)
		, firstFrame(nullptr)
	{
		filename = _filename;
	}
	
	~VideoLoop()
	{
		delete firstFrame;
		firstFrame = nullptr;
		
		delete mediaPlayer;
		mediaPlayer = nullptr;
		
		delete mediaPlayer2;
		mediaPlayer2 = nullptr;
	}
	
	void tick(const float dt)
	{
		if (mediaPlayer2 == nullptr)
		{
			if (mediaPlayer == nullptr || mediaPlayer->presentTime >= 1.0)
			{
				mediaPlayer2 = new MediaPlayer();
				mediaPlayer2->openAsync(filename.c_str(), false);
			}
		}
		
		if (mediaPlayer && mediaPlayer->context->hasPresentedLastFrame)
		{
			delete firstFrame;
			firstFrame = nullptr;
			
			delete mediaPlayer;
			mediaPlayer = nullptr;
		}
		
		if (mediaPlayer == nullptr)
		{
			mediaPlayer = mediaPlayer2;
			mediaPlayer2 = nullptr;
		}
		
		if (mediaPlayer)
		{
			mediaPlayer->tick(mediaPlayer->context);
			
			if (firstFrame == nullptr)
			{
				int sx;
				int sy;
				double duration;
				
				if (mediaPlayer->getVideoProperties(sx, sy, duration) && mediaPlayer->getTexture())
				{
					firstFrame = new Surface(sx, sy, false);
					
					pushSurface(firstFrame);
					{
						gxSetTexture(mediaPlayer->getTexture());
						setColor(colorWhite);
						gxBegin(GL_QUADS);
						{
							gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f, 0.f);
							gxTexCoord2f(1.f, 0.f); gxVertex2f(sx,  0.f);
							gxTexCoord2f(1.f, 1.f); gxVertex2f(sx,  sy );
							gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f, sy );
						}
						gxEnd();
						gxSetTexture(0);
					}
					popSurface();
				}
			}
			
			if (mediaPlayer->context->hasBegun)
				mediaPlayer->presentTime += dt;
			//else
			//	logDebug("??");
		}
		
		if (mediaPlayer2)
		{
			mediaPlayer2->tick(mediaPlayer2->context);
		}
	}
	
	GLuint getTexture() const
	{
		return mediaPlayer->getTexture();
	}
	
	GLuint getFirstFrameTexture() const
	{
		return firstFrame ? firstFrame->getTexture() : 0;
	}
};

struct VideoEffect
{
	VideoLoop * currVideoLoop;
	VideoLoop * nextVideoLoop;
	Surface * surface;
	
	VideoEffect()
		: currVideoLoop(nullptr)
		, nextVideoLoop(nullptr)
		, surface(nullptr)
	{
	}
	
	VideoEffect(const char * currFilename, const char * nextFilename)
		: currVideoLoop(nullptr)
		, nextVideoLoop(nullptr)
		, surface(nullptr)
	{
		currVideoLoop = new VideoLoop(currFilename);
		nextVideoLoop = new VideoLoop(nextFilename);
	}
	
	~VideoEffect()
	{
		delete currVideoLoop;
		currVideoLoop = nullptr;
		
		delete nextVideoLoop;
		nextVideoLoop = nullptr;
		
		delete surface;
		surface = nullptr;
	}
	
	void switchVideoLoop(const char * nextFilename)
	{
		delete currVideoLoop;
		currVideoLoop = nullptr;
		
		delete surface;
		surface = nullptr;
		
		currVideoLoop = nextVideoLoop;
		nextVideoLoop = nullptr;
		
		nextVideoLoop = new VideoLoop(nextFilename);
	}
	
	void tick(const float dt)
	{
		if (currVideoLoop != nullptr)
			currVideoLoop->tick(dt);
		if (nextVideoLoop != nullptr)
			nextVideoLoop->tick(0.f);
		
		int sx;
		int sy;
		double duration;
		
		if (currVideoLoop != nullptr && currVideoLoop->mediaPlayer->getVideoProperties(sx, sy, duration))
		{
			if (surface == nullptr)
			{
				surface = new Surface(sx, sy, true);
				surface->clear();
			}
			
			if (surface)
			{
				Shader shader("video-fade");
				setShader(shader);
				{
					//const float opacity = 1.f - std::powf(.1f, dt);
					const float opacity = 1.f;
					shader.setImmediate("opacity", opacity);
					shader.setTexture("source", 0, currVideoLoop->getTexture());
					shader.setTexture("firstFrame", 1, currVideoLoop->getFirstFrameTexture());
					shader.setTexture("colormap", 2, surface->getTexture());
					pushBlend(BLEND_OPAQUE);
					surface->postprocess();
					popBlend();
				}
				clearShader();
				
				pushBlend(BLEND_OPAQUE);
				//applyFsfx(*surface, "fsfx/godrays.ps");
				//const float luminanceStrength = (-std::cosf(framework.time / 4.567f) + 1.f) / 2.f;
				//applyFsfx(*surface, "fsfx/luminance.ps", luminanceStrength);
				popBlend();
			}
		}
	}
	
	GLuint getTexture() const
	{
		return surface ? surface->getTexture() : 0;
	}
	
	GLuint getFirstFrameTexture() const
	{
		return currVideoLoop->getFirstFrameTexture();
	}
};

#if DO_GAME

struct VideoGame
{
	struct VideoLoopInfo
	{
		std::string videoFilenameL;
		std::string videoFilenameR;
		
		VideoLoopInfo()
			: videoFilenameL()
			, videoFilenameR()
		{
		}
	};
	
	std::vector<VideoLoopInfo> videoLoopInfos;
	
	VideoEffect * videoEffectL;
	VideoEffect * videoEffectR;
	
	Surface * videoSurface;
	Surface * postSurface;
	
	VideoGame()
		: videoEffectL(nullptr)
		, videoEffectR(nullptr)
		, videoSurface(nullptr)
	{
		// find the list of video loops we can use
		
		findVideos();
		
		// create video effects
		
		videoEffectL = new VideoEffect();
		videoEffectR = new VideoEffect();
		
		//
		
		videoSurface = new Surface(LOOP_SX_TOTAL, LOOP_SY, true);
		videoSurface->clear();
		
		//
		
		postSurface = new Surface(GFX_SX, GFX_SY, true);
		postSurface->clear();
		
		// queue the next two loops
		
		nextVideoLoop();
		nextVideoLoop();
	}
	
	~VideoGame()
	{
		delete postSurface;
		postSurface = nullptr;
		
		delete videoSurface;
		videoSurface = nullptr;
		
		delete videoEffectL;
		videoEffectL = nullptr;
		delete videoEffectR;
		videoEffectR = nullptr;
	}
	
	void findVideos()
	{
		// retrieve a list of all video loops in the videos folder
		
		auto videos = listFiles("videos", true);
		
		for (auto i = videos.begin(); i != videos.end(); ++i)
		{
			const std::string & videoFilenameL = *i;
			
			auto e = videoFilenameL.find("-0.mp4");
			
			if (e != videoFilenameL.npos)
			{
				const std::string videoFilenameR = videoFilenameL.substr(0, e) + "-1.mp4";
				
				VideoLoopInfo videoLoopInfo;
				videoLoopInfo.videoFilenameL = videoFilenameL;
				videoLoopInfo.videoFilenameR = videoFilenameR;
				videoLoopInfos.push_back(videoLoopInfo);
			}
		}
	}
	
	bool nextVideoLoopInfo(VideoLoopInfo & videoLoopInfo)
	{
		if (videoLoopInfos.empty())
			return false;
		else
		{
			videoLoopInfo = videoLoopInfos[rand() % videoLoopInfos.size()];
			
			return true;
		}
	}
	
	void nextVideoLoop(const VideoLoopInfo & videoLoopInfo)
	{
		videoEffectL->switchVideoLoop(videoLoopInfo.videoFilenameL.c_str());
		videoEffectR->switchVideoLoop(videoLoopInfo.videoFilenameR.c_str());
	}
	
	void nextVideoLoop()
	{
		VideoLoopInfo videoLoopInfo;
		
		if (nextVideoLoopInfo(videoLoopInfo))
		{
			nextVideoLoop(videoLoopInfo);
		}
	}
	
	void tick(const float dt)
	{
		// handle input
		
		if (keyboard.wentDown(SDLK_n))
		{
			nextVideoLoop();
		}
		
		// process videos
	
		videoEffectL->tick(dt * Calc::Lerp(0.f, 4.f, kMixVideoSpeed1));
		videoEffectR->tick(dt * Calc::Lerp(0.f, 4.f, kMixVideoSpeed2));
	}
	
	void draw() const
	{
		// composite both left and right videos onto video surface
		
		pushSurface(videoSurface);
		{
		#if 1
			Shader composeShader("game-compose-videos");
			setShader(composeShader);
			{
				composeShader.setTexture("videoLCurrentFrame", 0, videoEffectL->getTexture());
				composeShader.setTexture("videoRCurrentFrame", 1, videoEffectR->getTexture());
				composeShader.setTexture("videoLFirstFrame", 2, videoEffectL->getFirstFrameTexture());
				composeShader.setTexture("videoRFirstFrame", 3, videoEffectR->getFirstFrameTexture());
				
				const int lx1 = 0;
				const int ly1 = 0;
				const int lx2 = LOOP_SX;
				const int ly2 = LOOP_SY;
				
				const int rx1 = LOOP_SX_TOTAL - LOOP_SX;
				const int ry1 = 0;
				const int rx2 = LOOP_SX_TOTAL;
				const int ry2 = LOOP_SY;
				
				const float sx = float(videoSurface->getWidth());
				const float sy = float(videoSurface->getHeight());
				
				const float lu1 = lx1 / sx;
				const float lv1 = ly1 / sy;
				const float lu2 = lx2 / sx;
				const float lv2 = ly2 / sy;
				const float ru1 = rx1 / sx;
				const float rv1 = ry1 / sy;
				const float ru2 = rx2 / sx;
				const float rv2 = ry2 / sy;
				
				composeShader.setImmediate("videoRectL", lu1, lv1, lu2 - lu1, lv2 - lv1);
				composeShader.setImmediate("videoRectR", ru1, rv1, ru2 - ru1, rv2 - rv1);
				
				pushBlend(BLEND_OPAQUE);
				{
					drawRect(0, 0, videoSurface->getWidth(), videoSurface->getHeight());
				}
				popBlend();
			}
			clearShader();
		#else
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 0);
				
				pushBlend(BLEND_ADD);
				{
					gxSetTexture(videoEffectL->getTexture());
					{
						setColorf(1.f, 1.f, 1.f, opacity);
						drawRect(0, 0, LOOP_SX, LOOP_SY);
					}
					gxSetTexture(0);
				}
				popBlend();
			}
			gxPopMatrix();
			
			gxPushMatrix();
			{
				gxTranslatef(LOOP_SX_TOTAL - LOOP_SX, 0, 0);
				
				pushBlend(BLEND_ADD);
				{
					gxSetTexture(videoEffectR->getTexture());
					{
						setColorf(1.f, 1.f, 1.f, opacity);
						drawRect(0, 0, LOOP_SX, LOOP_SY);
					}
					gxSetTexture(0);
				}
				popBlend();
			}
			gxPopMatrix();
		#endif
		}
		popSurface();
		
		float godraysStrength = 0.f;
		
		bool hasDurationL;
		bool hasDurationR;
		int sx;
		int sy;
		double durationL;
		double durationR;
		
		hasDurationL = videoEffectL->currVideoLoop->mediaPlayer->getVideoProperties(sx, sy, durationL);
		hasDurationR = videoEffectR->currVideoLoop->mediaPlayer->getVideoProperties(sx, sy, durationR);
		
		if (hasDurationL && hasDurationR)
		{
			const double positionL = videoEffectL->currVideoLoop->mediaPlayer->presentTime;
			const double positionR = videoEffectR->currVideoLoop->mediaPlayer->presentTime;
			
			const double deltaL = std::abs(durationL / 2.0 - positionL);
			const double deltaR = std::abs(durationR / 2.0 - positionR);
			
			double nearnessL = std::max(0.0, 1.0 - deltaL / 2.0);
			double nearnessR = std::max(0.0, 1.0 - deltaR / 2.0);
			
			godraysStrength = nearnessL + nearnessR;
		}
		
		godraysStrength *= kMixGodraysStrength;
		
	#if 0
		pushBlend(BLEND_OPAQUE);
		applyFsfx(*videoSurface, "fsfx/godrays.ps", godraysStrength);
		popBlend();
	#endif
		
		pushSurface(postSurface);
		{
			gxPushMatrix();
			{
				const float opacity = Calc::Lerp(0.f, 1.f, kMixVideoSurfaceOpacity);
				
				const float sx = postSurface->getWidth();
				const float sy = postSurface->getHeight();
				
				const float scaleX = sx / float(LOOP_SX_TOTAL);
				const float scaleY = sy / float(LOOP_SY);
				const float scale = Calc::Min(scaleX, scaleY);
				const float paddingX = sx - scale * videoSurface->getWidth();
				const float paddingY = sy - scale * videoSurface->getHeight();
				const float offsetX = paddingX * .5f;
				const float offsetY = paddingY * .5f;
				
				gxTranslatef(offsetX, offsetY, 0.f);
				gxScalef(scale, scale, 1.f);
				
				pushBlend(BLEND_ADD);
				{
					setColorf(1.f, 1.f, 1.f, opacity);
					gxSetTexture(videoSurface->getTexture());
					drawRect(0, 0, videoSurface->getWidth(), videoSurface->getHeight());
					gxSetTexture(0);
				}
				popBlend();
			}
			gxPopMatrix();
			
		#if 1
			pushBlend(BLEND_OPAQUE);
			{
				const float retain = Calc::Lerp(0.f, 1.f, kMixPostSurfaceRetain);
				
				setShader_ColorMultiply(postSurface->getTexture(), Color(retain, retain, retain, 1.f), 1.f);
				postSurface->postprocess();
				clearShader();
				
			#if 0
				setShader_GaussianBlurH(postSurface->getTexture(), 31, 31.f);
				postSurface->postprocess();
				clearShader();
			#endif
			#if 0
				setShader_GaussianBlurV(postSurface->getTexture(), 31, 31.f);
				postSurface->postprocess();
				clearShader();
			#endif
			#if 0
				Surface gradientSurface(GFX_SX, GFX_SY, true);
				const float r1 = Calc::Lerp(30.f, 600.f, (1.f - std::cos(framework.time / 0.345f)) / 2.f);
				const float r2 = r1 + 100.f;
				const float rs = Calc::Lerp(0.f, 40.f, (1.f - std::cos(framework.time / 2.345f)) / 2.f);
				applyFsfx(gradientSurface, "fsfx/ripple.ps", 1.f, r1, r2, 1, framework.time);
				applyFsfx(gradientSurface, "fsfx/luminance-gradient.ps", 1.f, rs);
				applyFsfx(*postSurface, "fsfx/distort-gradient.ps", 1.f, 10.f, .2f, 0.f, 0.f, gradientSurface.getTexture());
			#elif 0
				applyFsfx(*postSurface, "fsfx/distort-gradient.ps", 1.f, 10.f, 0.f, -.5f, 0.f, glitchLoop->getTexture());
			#endif
			#if 1
				applyFsfx(*postSurface, "fsfx/godrays.ps", godraysStrength);
			#endif
			}
			popBlend();
		#endif
		}
		popSurface();
		
	#if 0
		Surface mask(postSurface->getWidth(), postSurface->getHeight(), true);
		mask.clear();
		pushSurface(&mask);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(colorWhite);
				hqFillCircle(mask.getWidth()/2, mask.getHeight()/2, 300.f);
			}
			hqEnd();
		}
		popSurface();
		
		{
			const GLuint a = videoSurface->getTexture();
			const GLuint b = postSurface->getTexture();
			postSurface->swapBuffers();
			
			pushSurface(postSurface);
			applyMask(a, b, mask.getTexture());
			popSurface();
		}
	#endif
	}
};

#endif

struct GrooopLogo : TweenFloatCollection
{
	TweenFloat unfold;
	TweenFloat scale;
	TweenFloat opacity;
	
	GrooopLogo()
		: unfold(0.f)
		, scale(1.f)
		, opacity(1.f)
	{
		addVar("unfold", unfold);
		addVar("scale", scale);
		addVar("opacity", opacity);
		
		unfold.wait(1.f);
		unfold.to(1.f, .5f, kEaseType_SineIn, 0.f);
		
		scale.wait(3.f);
		scale.to(0.f, .5f, kEaseType_SineOut, 0.f);
		
		opacity.wait(3.f);
		opacity.to(0.f, .75f, kEaseType_SineOut, 0.f);
	}
	
	virtual ~GrooopLogo()
	{
	}
	
	void tick(const float dt)
	{
		TweenFloatCollection::tick(dt);
	}
	
	void draw() const
	{
		const float circleFade = (float)unfold;
		
		gxPushMatrix();
		{
			gxTranslatef(GFX_SX/2, GFX_SY/2, 1.f);
			
			setColorf(1.f, 1.f, 1.f, kMixLogoOpacity * (float)opacity);
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				hqStrokeCircle(0.f, 0.f, 100.f * (float)scale, 10.f);
			}
			hqEnd();
			
			for (int i = -1; i <= +1; ++i)
			{
				hqBegin(HQ_STROKED_CIRCLES);
				{
					hqStrokeCircle(i * 60 * circleFade, 0.f, 25.f * (float)scale, 10.f);
				}
				hqEnd();
			}
		}
		gxPopMatrix();
	}
};

#include <portaudio/portaudio.h>

static float mousePx = 0.f;
static float mousePy = 0.f;

struct BaseOsc
{
	virtual ~BaseOsc()
	{
	}
	
	virtual void init(const float frequency) = 0;
	virtual void generate(float * __restrict samples, const int numSamples) = 0;
};

struct SineOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SineOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / 44100.f * M_PI * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePy;
		
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::sinf(phase);
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, M_PI * 2.f);
	}
};

struct SawOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SawOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / 44100.f * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePx;
		
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::fmodf(phase, 2.f) - 1.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

struct TriangleOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~TriangleOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / 44100.f * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		//const float phaseStep = this->phaseStep * mousePx;
		
		const float targetPhaseStep = this->phaseStep * (mousePx + 1.f);
		
		static float phaseStep = this->phaseStep; // fixme : non-static
		
		const float retain1 = .999f;
		const float retain2 = 1.f - retain1;
		
		for (int i = 0; i < numSamples; ++i)
		{
			phaseStep = phaseStep * retain1 + targetPhaseStep * retain2;
			
			samples[i] = (std::abs(std::fmodf(phase, 2.f) - 1.f) - .5f) * 2.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

struct SquareOsc : BaseOsc
{
	float phase;
	float phaseStep;
	
	virtual ~SquareOsc() override
	{
	}
	
	virtual void init(const float frequency) override
	{
		phase = 0.f;
		phaseStep = frequency / 44100.f * 2.f;
	}
	
	virtual void generate(float * __restrict samples, const int numSamples) override
	{
		const float phaseStep = this->phaseStep * mousePx;

		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = std::fmodf(phase, 2.f) < 1.f ? -1.f : +1.f;
			
			phase += phaseStep;
		}
		
		phase = std::fmodf(phase, 2.f);
	}
};

static const int kMaxOscs = 1;

static BaseOsc * oscs[kMaxOscs] = { };

static bool oscIsInit = false;

static void initOsc()
{
	if (oscIsInit)
		return;
	
	oscIsInit = true;
	
	float frequency = 800.f;
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		//const int o = s % 4;
		const int o = 2;
		
		if (o == 0)
			osc = new SineOsc();
		else if (o == 1)
			osc = new SawOsc();
		else if (o == 2)
			osc = new TriangleOsc();
		else
			osc = new SquareOsc();
		
		osc->init(frequency);
		
		frequency *= 1.0234f;
	}
}

static void shutOsc()
{
	if (!oscIsInit)
		return;
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc *& osc = oscs[s];
		
		delete osc;
		osc = nullptr;
	}
	
	oscIsInit = false;
}

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	float * __restrict buffer = (float*)outputBuffer;
	
	float * __restrict oscBuffers[kMaxOscs];
	
	for (int s = 0; s < kMaxOscs; ++s)
	{
		BaseOsc * osc = oscs[s];
		
		oscBuffers[s] = (float*)alloca(sizeof(float) * framesPerBuffer);
		
		osc->generate(oscBuffers[s], framesPerBuffer);
	}
	
	for (int i = 0; i < framesPerBuffer; ++i)
	{
		float v = 0.f;
		
		for (int s = 0; s < kMaxOscs; ++s)
			v += oscBuffers[s][i] * .2f;
		
		*buffer++ = v;
	}
	
    return paContinue;
}

static PaStream * stream = nullptr;

static bool initAudioOutput()
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
#if 0
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
	}
#endif
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice)
	{
		logError("portaudio: failed to find output device");
		return false;
	}
	
	outputParameters.channelCount = 1;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	//if (Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, 44100, 1024, portaudioCallback, nullptr) != paNoError)
	//if (Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 44100, 1024, portaudioCallback, nullptr) != paNoError)
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, 44100, 1024, paDitherOff, portaudioCallback, nullptr)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

static bool shutAudioOutput()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

static void testPortaudio()
{
	initOsc();
	
	if (initAudioOutput())
	{
		for (;;)
		{
			framework.process();
			
			mousePx = mouse.x / float(GFX_SX);
			mousePy = mouse.y / float(GFX_SY);
		}
	}
	
	shutAudioOutput();
	
	shutOsc();
}

static bool loadFileContents(const char * filename, void * bytes, int & numBytes)
{
	bool result = true;
	
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		// load source from file
		
		fseek(file, 0, SEEK_END);
		const int fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		if (numBytes < fileSize)
		{
			result = false;
		}
		else
		{
			numBytes = fileSize;
			
			if (fread(bytes, numBytes, 1, file) != (size_t)1)
			{
				result = false;
			}
		}
		
		fclose(file);
	}
	
	if (!result)
	{
		numBytes = 0;
	}
	
	return result;
}

struct JpegLoadData
{
	unsigned char * buffer;
	int bufferSize;
	bool flipY;
	
	int sx;
	int sy;
	
	JpegLoadData()
		: buffer(nullptr)
		, bufferSize(0)
		, flipY(false)
		, sx(0)
		, sy(0)
	{
	}
	
	~JpegLoadData()
	{
		free();
	}
	
	void disown()
	{
		buffer = 0;
		bufferSize = 0;
		sx = 0;
		sy = 0;
	}
	
	void free()
	{
		delete [] buffer;
		buffer = nullptr;
		bufferSize = 0;
	}
};

static bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data)
{
	bool result = true;
	
	tjhandle h = tjInitDecompress();
	
	if (h == nullptr)
	{
		logError("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		int sx = 0;
		int sy = 0;
		
		int jpegSubsamp = 0;
		int jpegColorspace = 0;
		
		if (tjDecompressHeader3(h, (unsigned char*)buffer, (unsigned long)bufferSize, &sx, &sy, &jpegSubsamp, &jpegColorspace) != 0)
		{
			logError("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			data.sx = sx;
			data.sy = sy;
			
			const TJPF pixelFormat = TJPF_RGBX;
			
			const int pitch = sx * tjPixelSize[pixelFormat];
			
			const int flags = TJFLAG_BOTTOMUP * (data.flipY ? 1 : 0);
			
			const int requiredBufferSize = pitch * sy;
			
			if (data.buffer == nullptr || data.bufferSize != requiredBufferSize)
			{
				delete [] data.buffer;
				data.buffer = nullptr;
				data.bufferSize = 0;
				
				//
				
				data.buffer = new unsigned char[requiredBufferSize];
				data.bufferSize = requiredBufferSize;
			}
			
			if (tjDecompress2(h, (unsigned char*)buffer, (unsigned long)bufferSize, (unsigned char*)data.buffer, sx, pitch, data.sy, pixelFormat, flags) != 0)
			{
				logError("turbojpeg: %s", tjGetErrorStr());
				
				result = false;
			}
			else
			{
				//logDebug("decoded jpeg!");
			}
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

static bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, void * fileBuffer, int fileBufferSize)
{
	bool result = true;
	
	if (loadFileContents(filename, fileBuffer, fileBufferSize) == false)
	{
		logDebug("turbojpeg: %s", "failed to load file contents");
		
		result = false;
	}
	else
	{
		result = loadImage_turbojpeg(fileBuffer, fileBufferSize, data);
	}
	
	return result;
}

static bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void *& dstBuffer, int & dstBufferSize)
{
	bool result = true;
	
	tjhandle h = tjInitCompress();
	
	if (h == nullptr)
	{
		logError("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		const TJPF pixelFormat = TJPF_RGBX;
		const TJSAMP subsamp = TJSAMP_422;
		const int quality = 85;
		
		const int xPitch = srcSx * tjPixelSize[pixelFormat];
		
		unsigned long dstBufferSize2 = dstBufferSize;
		
		if (tjCompress2(h, (const unsigned char *)srcBuffer, srcSx, xPitch, srcSy, TJPF_RGBX, (unsigned char**)&dstBuffer, &dstBufferSize2, subsamp, quality, 0) < 0)
		{
			logError("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			dstBufferSize = dstBufferSize2;
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

static bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void * _saveBuffer, int _saveBufferSize)
{
	bool result = true;
	
	void * saveBuffer = _saveBuffer;
	int saveBufferSize = _saveBufferSize;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, saveBuffer, saveBufferSize) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	Assert(saveBuffer == _saveBuffer);
	Assert(saveBufferSize <= _saveBufferSize);
	
	return result;
}

static bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy)
{
	bool result = true;
	
	void * saveBuffer = nullptr;
	int saveBufferSize = 0;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, saveBuffer, saveBufferSize) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	tjFree((unsigned char*)saveBuffer);
	saveBuffer = nullptr;
	saveBufferSize = 0;
	
	return result;
}

struct JpegStreamer
{
	static const int kFileBufferSize = 1024 * 1024;
	
	struct FileContents
	{
		unsigned char * buffer;
		int bufferSize;
		
		int index;
		
		FileContents()
			: buffer(nullptr)
			, bufferSize(0)
			, index(0)
		{
			 buffer = new unsigned char[kFileBufferSize];
			 bufferSize = kFileBufferSize;
		}
		
		~FileContents()
		{
			delete [] buffer;
			buffer = nullptr;
			bufferSize = 0;
			
			index = 0;
		}
	};
	
	struct ImageContents
	{
		unsigned char * buffer;
		int bufferSize;
		
		int sx;
		int sy;
		
		ImageContents()
			: buffer(nullptr)
			, bufferSize(0)
			, sx(0)
			, sy(0)
		{
		}
		
		~ImageContents()
		{
			delete [] buffer;
			buffer = nullptr;
			bufferSize = 0;
			
			sx = 0;
			sy = 0;
		}
	};
	
	static const int kNumBuffers = 4;
	
	std::string fileBaseName;
	int fileNextIndex;
	
	std::list<FileContents*> fileProduceList;
	std::list<FileContents*> fileConsumeList;
	
	SDL_Thread * fileThread;
	SDL_mutex * fileMutex;
	SDL_sem * fileProduceSema;
	SDL_sem * fileConsumeSema;
	
	//
	
	std::list<ImageContents*> imageProduceList;
	std::list<ImageContents*> imageConsumeList;
	
	SDL_Thread * imageThread;
	SDL_mutex * imageMutex;
	SDL_sem * imageProduceSema;
	SDL_sem * imageConsumeSema;
	
	bool stop;
	
	JpegStreamer(const char * _baseName)
		: fileBaseName(_baseName)
		, fileNextIndex(0)
		, fileProduceList()
		, fileConsumeList()
		, fileThread(nullptr)
		, fileMutex(nullptr)
		, fileProduceSema(nullptr)
		, fileConsumeSema(nullptr)
		, imageProduceList()
		, imageConsumeList()
		, imageThread(nullptr)
		, imageMutex(nullptr)
		, imageProduceSema(nullptr)
		, imageConsumeSema(nullptr)
		, stop(false)
	{
		for (int i = 0; i < kNumBuffers; ++i)
		{
			FileContents * fileContents = new FileContents();
			
			fileProduceList.push_back(fileContents);
		}
		
		fileMutex = SDL_CreateMutex();
		
		fileProduceSema = SDL_CreateSemaphore(kNumBuffers);
		fileConsumeSema = SDL_CreateSemaphore(0);
		
		fileThread = SDL_CreateThread(fileProcess, "JpegStreamer File Process", this);
		
		//
		
		for (int i = 0; i < kNumBuffers; ++i)
		{
			ImageContents * imageContents = new ImageContents();
			
			imageProduceList.push_back(imageContents);
		}
		
		imageMutex = SDL_CreateMutex();
		
		imageProduceSema = SDL_CreateSemaphore(kNumBuffers);
		imageConsumeSema = SDL_CreateSemaphore(0);
		
		imageThread = SDL_CreateThread(imageProcess, "JpegStreamer Image Process", this);
	}
	
	~JpegStreamer()
	{
		SDL_LockMutex(imageMutex);
		{
			stop = true;
		}
		SDL_UnlockMutex(imageMutex);
		
		SDL_SemPost(imageProduceSema);
		
		SDL_WaitThread(imageThread, nullptr);
		imageThread = nullptr;
		
		SDL_DestroySemaphore(imageConsumeSema);
		imageConsumeSema = nullptr;
		
		SDL_DestroySemaphore(imageProduceSema);
		imageProduceSema = nullptr;
		
		SDL_DestroyMutex(imageMutex);
		imageMutex = nullptr;
		
		while (imageProduceList.size() > 0)
		{
			delete imageProduceList.front();
			
			imageProduceList.pop_front();
		}
		
		while (imageConsumeList.size() > 0)
		{
			delete imageConsumeList.front();
			
			imageConsumeList.pop_front();
		}
		
		//
		
		SDL_LockMutex(fileMutex);
		{
			stop = true;
		}
		SDL_UnlockMutex(fileMutex);
		
		SDL_SemPost(fileProduceSema);
		
		SDL_WaitThread(fileThread, nullptr);
		fileThread = nullptr;
		
		SDL_DestroySemaphore(fileConsumeSema);
		fileConsumeSema = nullptr;
		
		SDL_DestroySemaphore(fileProduceSema);
		fileProduceSema = nullptr;
		
		SDL_DestroyMutex(fileMutex);
		fileMutex = nullptr;
		
		while (fileProduceList.size() > 0)
		{
			delete fileProduceList.front();
			
			fileProduceList.pop_front();
		}
		
		while (fileConsumeList.size() > 0)
		{
			delete fileConsumeList.front();
			
			fileConsumeList.pop_front();
		}
	}
	
	static int fileProcess(void * obj)
	{
		JpegStreamer * self = (JpegStreamer*)obj;
		
		self->fileProcess();
		
		return 0;
	}
	
	void fileProcess()
	{
		while (stop == false)
		{
			SDL_SemWait(fileProduceSema);
			
			//
			
			FileContents * fileContents = nullptr;
			
			SDL_LockMutex(fileMutex);
			{
				if (stop == false)
				{
					fileContents = fileProduceList.front();
					
					fileProduceList.pop_front();
				}
			}
			SDL_UnlockMutex(fileMutex);
			
			//
			
			if (fileContents == nullptr)
				break;
			
			//
			
			char filename[256];
			sprintf_s(filename, sizeof(filename), fileBaseName.c_str(), fileNextIndex + 1);
			
			fileContents->bufferSize = kFileBufferSize;
			fileContents->index = fileNextIndex;
	
			const bool hasFile = loadFileContents(filename, fileContents->buffer, fileContents->bufferSize);
			
			fileNextIndex++;
			
			//
			
			if (hasFile)
			{
				SDL_LockMutex(fileMutex);
				{
					fileConsumeList.push_back(fileContents);
				}
				SDL_UnlockMutex(fileMutex);
				
				SDL_SemPost(fileConsumeSema);
			}
			else
			{
				delete fileContents;
				fileContents = nullptr;
			}
		}
	}
	
	static int imageProcess(void * obj)
	{
		JpegStreamer * self = (JpegStreamer*)obj;
		
		self->imageProcess();
		
		return 0;
	}
	
	void imageProcess()
	{
		while (stop == false)
		{
			SDL_SemWait(imageProduceSema);
			
			ImageContents * imageContents = nullptr;
			
			SDL_LockMutex(imageMutex);
			{
				if (stop == false)
				{
					imageContents = imageProduceList.front();
					
					imageProduceList.pop_front();
				}
			}
			SDL_UnlockMutex(imageMutex);
			
			//
			
			if (imageContents == nullptr)
				break;
			
			//
			
			SDL_SemWait(fileConsumeSema);
			
			FileContents * fileContents = nullptr;
			
			SDL_LockMutex(fileMutex);
			{
				if (stop == false)
				{
					fileContents = fileConsumeList.front();
					
					fileConsumeList.pop_front();
				}
			}
			SDL_UnlockMutex(fileMutex);
			
			//
			
			// fixme : stop here is not guaranteed to work (I think)
			
			if (fileContents == nullptr)
				break;
			
			//
			
			JpegLoadData data;
			
			const bool hasImage = loadImage_turbojpeg(
				fileContents->buffer,
				fileContents->bufferSize,
				data);
			
			imageContents->buffer = data.buffer;
			imageContents->bufferSize = data.bufferSize;
			imageContents->sx = data.sx;
			imageContents->sy = data.sy;
			
			if (hasImage)
			{
				SDL_LockMutex(fileMutex);
				{
					fileProduceList.push_back(fileContents);
				}
				SDL_UnlockMutex(fileMutex);
				
				SDL_SemPost(fileProduceSema);
				
				//
				
				SDL_LockMutex(imageMutex);
				{
					imageConsumeList.push_back(imageContents);
				}
				SDL_UnlockMutex(imageMutex);
				
				SDL_SemPost(imageConsumeSema);
			}
			else
			{
				delete imageContents;
				imageContents = nullptr;
				
				delete fileContents;
				fileContents = nullptr;
			}
		}
	}
	
	void consume(ImageContents *& contents)
	{
		SDL_SemWait(imageConsumeSema);
		
		SDL_LockMutex(imageMutex);
		{
			contents = imageConsumeList.front();
			
			imageConsumeList.pop_front();
		}
		SDL_UnlockMutex(imageMutex);
	}
	
	void release(ImageContents *& contents)
	{
		SDL_LockMutex(imageMutex);
		{
			imageProduceList.push_back(contents);
			
			contents = nullptr;
		}
		SDL_UnlockMutex(imageMutex);
		
		SDL_SemPost(imageProduceSema);
	}
};

struct JpegLoop
{
	std::string baseFilename;
	double fps;
	
	unsigned char * fileBuffer;
	int fileBufferSize;
	
	unsigned char ** dstBuffer;
	int * dstBufferSize;
	
	GLuint texture;
	
	JpegLoop(const char * _baseFilename, const double _fps, unsigned char * _fileBuffer, int _fileBufferSize, unsigned char ** _dstBuffer, int * _dstBufferSize)
		: baseFilename(_baseFilename)
		, fps(_fps)
		, fileBuffer(_fileBuffer)
		, fileBufferSize(_fileBufferSize)
		, dstBuffer(_dstBuffer)
		, dstBufferSize(_dstBufferSize)
		, texture(0)
	{
		glGenTextures(1, &texture);
		
		if (texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			checkErrorGL();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();
		}
	}
	
	~JpegLoop()
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}
	
	void seek(const double time)
	{
		const int frameIndex = std::floor(time * fps);
		
		char filename[256];
		sprintf_s(filename, sizeof(filename), baseFilename.c_str(), frameIndex + 1);
		
		JpegLoadData data;
		data.buffer = *dstBuffer;
		data.bufferSize = *dstBufferSize;
		data.flipY = true;
		
		const bool hasImage = loadImage_turbojpeg(filename, data, fileBuffer, fileBufferSize);
		
		*dstBuffer = data.buffer;
		*dstBufferSize = data.bufferSize;
		
		if (hasImage)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			checkErrorGL();
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, data.sx, data.sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.buffer);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();
		}
		
		data.disown();
	}
};

static void speedup(const char * srcBasename, const char * dstBasename, const int numFrames)
{
	const int numDstFrames = numFrames / 2;
	
	int loadBufferSize = 1024 * 1024;
	char * loadBuffer = new char[loadBufferSize];
	
	int saveBufferSize = 1024 * 1024;
	char * saveBuffer = new char[saveBufferSize];
	
	JpegLoadData loadData[2];
	
	for (int i = 0; i < numDstFrames; ++i)
	{
		char srcFilename1[256];
		char srcFilename2[256];
		char dstFilename[256];
		
		sprintf_s(srcFilename1, sizeof(srcFilename1), srcBasename, i*2+0 + 1);
		sprintf_s(srcFilename2, sizeof(srcFilename2), srcBasename, i*2+1 + 1);
		sprintf_s(dstFilename, sizeof(dstFilename), dstBasename, i + 1);
		
		if (loadImage_turbojpeg(srcFilename1, loadData[0], loadBuffer, loadBufferSize) &&
			loadImage_turbojpeg(srcFilename2, loadData[1], loadBuffer, loadBufferSize) &&
			(loadData[0].sx) == (loadData[1].sx) &&
			(loadData[0].sy) == (loadData[1].sy))
		{
			for (int i = 0; i < loadData[0].bufferSize; ++i)
			{
				loadData[0].buffer[i] = (int(loadData[0].buffer[i]) + int(loadData[1].buffer[i])) / 2;
			}
			
			saveImage_turbojpeg(dstFilename, loadData[0].buffer, loadData[0].bufferSize, loadData[0].sx, loadData[0].sy, saveBuffer, saveBufferSize);
		}
		
		if ((i % 100) == 0)
		{
			printf("processing frame %d/%d\n", i, numDstFrames);
		}
	}
	
	for (int i = 0; i < 2; ++i)
		loadData[i].free();
	
	delete [] saveBuffer;
	saveBuffer = nullptr;
	saveBufferSize = 0;
	
	delete [] loadBuffer;
	loadBuffer = nullptr;
	loadBufferSize = 0;
}

#include <sys/stat.h>

int main(int argc, char * argv[])
{
	changeDirectory("/Users/thecat/Google Drive/The Grooop - Welcome");
	
	//
	
#if 0
	int numFrames = 43200;
	int streamIndex = 0;
	
#if 0
	while (numFrames >= 2 && streamIndex < 12)
	{
		numFrames /= 2;
		++streamIndex;
	}
#endif
	
	while (numFrames >= 2)
	{
		char srcPath[256];
		char dstPath[256];
		
		sprintf_s(srcPath, sizeof(srcPath), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d", streamIndex + 0);
		sprintf_s(dstPath, sizeof(dstPath), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d", streamIndex + 1);
		
		mkdir(dstPath, S_IRWXU | S_IRGRP | S_IROTH);
		
		char srcBasename[256];
		char dstBasename[256];
		
		sprintf_s(srcBasename, sizeof(srcBasename), "%s/%%06d.jpg", srcPath);
		sprintf_s(dstBasename, sizeof(dstBasename), "%s/%%06d.jpg", dstPath);
		
		speedup(srcBasename, dstBasename, numFrames);
		
		numFrames /= 2;
		++streamIndex;
	}
#endif

#if 0
	{
		const char * srcFilename1 = "/Users/thecat/videosplitter/slides-" STREAM_ID "-00/000001.jpg";
		const char * srcFilename2 = "/Users/thecat/videosplitter/slides-" STREAM_ID "-00/000100.jpg";
		const char * dstFilename = "/Users/thecat/videosplitter/savetest.jpg";
		
		int fileBufferSize = 1024 * 1024;
		char * fileBuffer = new char[fileBufferSize];
		
		JpegLoadData loadData[2];
		
		if (loadImage_turbojpeg(srcFilename1, loadData[0], fileBuffer, fileBufferSize) &&
			loadImage_turbojpeg(srcFilename2, loadData[1], fileBuffer, fileBufferSize) &&
			(loadData[0].bufferSize == loadData[1].bufferSize))
		{
			for (int i = 0; i < loadData[0].bufferSize; ++i)
			{
				loadData[0].buffer[i] = (int(loadData[0].buffer[i]) + int(loadData[1].buffer[i])) / 2;
			}
			
			saveImage_turbojpeg(dstFilename, loadData[0].buffer, loadData[0].bufferSize, loadData[0].sx, loadData[0].sy);
		}
		
		for (int i = 0; i < 2; ++i)
			loadData[i].free();
		
		delete [] fileBuffer;
		fileBuffer = nullptr;
		fileBufferSize = 0;
	}
#endif

#if 0
	{
		JpegStreamer * jpegStreamer = new JpegStreamer("/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg");
		
		logDebug("testJpegStreamer: start");
		
		for (int i = 0; i < 1000; ++i)
		{
			JpegStreamer::ImageContents * contents = nullptr;
			
			jpegStreamer->consume(contents);
			
			jpegStreamer->release(contents);
		}
		
		logDebug("testJpegStreamer: stop");
		
		delete jpegStreamer;
		jpegStreamer = nullptr;
	}
#endif

#if 0
	unsigned char * dstBuffer = nullptr;
	int dstBufferSize = 0;
	
	int fileBufferSize = 1024 * 1024;
	char * fileBuffer = new char[fileBufferSize];
	
	logDebug("testTurbojpeg: start");
	
	for (int i = 0; i < 1000; ++i)
	{
		char filename[256];
		sprintf_s(filename, sizeof(filename), "/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg", i + 1);
		
		int dstSx;
		int dstSy;
		
		loadImage_turbojpeg(filename, dstBuffer, dstBufferSize, dstSx, dstSy, fileBuffer, fileBufferSize);
	}
	
	logDebug("testTurbojpeg: stop");
	
	delete [] fileBuffer;
	fileBuffer = nullptr;
	fileBufferSize = 0;
	
	delete [] dstBuffer;
	dstBuffer = nullptr;
	dstBufferSize = 0;
#endif

#if 1
	framework.init(0, nullptr, GFX_SX, GFX_SY);
	{
		const int fps = 24;
		const double duration = 1800.0;
		
		unsigned char * dstBuffer = nullptr;
		int dstBufferSize = 0;
		
		int fileBufferSize = 1024 * 1024;
		unsigned char * fileBuffer = new unsigned char[fileBufferSize];
		
		const int kNumJpegLoops = 3;
		
		JpegLoop * jpegLoop[kNumJpegLoops] = { };
		
		for (int i = 0; i < kNumJpegLoops; ++i)
		{
			jpegLoop[i] = new JpegLoop("/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg", fps, fileBuffer, fileBufferSize, &dstBuffer, &dstBufferSize);
		}
		
		Surface * surface = new Surface(GFX_SX, GFX_SY, true);
		surface->clear();
		
		double time = 0.0;
		bool isPaused = false;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_SPACE))
				isPaused = !isPaused;
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				time = duration * mouse.x / double(GFX_SX);
			}
			
			const float speedInterp = (mouse.x / float(GFX_SX) - .5f) * 2.f;
			const float speedFactor = Calc::Lerp(0.f, 1000.f, std::powf(std::abs(speedInterp), 2.5f)) * Calc::Sign(speedInterp);
			
			int streamIndex = 0;
			
			for (float i = 1.f; i * 2.f < std::abs(speedFactor) && streamIndex < 14; i *= 2.f)
				streamIndex++;
			
			//logDebug("stream index: %d", streamIndex);
			
			//
			
			float fpsMultiplier = 1.f;
			
			for (int i = 0; i < streamIndex; ++i)
				fpsMultiplier /= 2.f;
			
			for (int i = 0; i < kNumJpegLoops; ++i)
			{
				float seek = 0.f;
				
				if (i == 0)
					seek = time;
				else
					seek = (duration / i) - time;
				
				jpegLoop[i]->fps = fps * fpsMultiplier;
				
				char baseFilename[256];
				sprintf_s(baseFilename, sizeof(baseFilename), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d/%%06d.jpg", streamIndex);
				
				jpegLoop[i]->baseFilename = baseFilename;
				
				jpegLoop[i]->seek(seek);
			}
			
			if (isPaused == false)
			{
				time += framework.timeStep * speedFactor;
			}
			
			if (time > duration)
				time = 0.0;
			if (time < 0.0)
				time = duration;
			
			framework.beginDraw(0, 0, 0, 0);
			{
				pushSurface(surface);
				{
				#if 0
					pushBlend(BLEND_ALPHA);
				#else
					pushBlend(BLEND_OPAQUE);
				#endif
					setColor(255, 255, 255, 15);
					gxSetTexture(jpegLoop[0]->texture);
					drawRect(0, 0, GFX_SX, GFX_SY);
					gxSetTexture(0);
					popBlend();
				}
				popSurface();
				
			#if 1
				const float kernelSize = 127.f * std::pow(std::abs(mouse.y / float(GFX_SY) - .5f) * 2.f, 4.f);
				pushBlend(BLEND_OPAQUE);
				setShader_GaussianBlurH(surface->getTexture(), kernelSize + 1, kernelSize);
				surface->postprocess();
				setShader_GaussianBlurV(surface->getTexture(), kernelSize + 1, kernelSize);
				surface->postprocess();
				clearShader();
				popBlend();
			#endif
			
				pushSurface(surface);
				{
				#if 1
					for (int i = 0; i < kNumJpegLoops; ++i)
					{
						const int nx = 3;
						const int sx = 128 * 2;
						const int sy = 72 * 2;
						const int px = 10;
						const int py = 10;
						
						const int ix = i % nx;
						const int iy = i / nx;
						
						const int x = (sx + px) * ix + sx/2;
						const int y = (sy + py) * iy + sy/2;
						
						gxPushMatrix();
						{
							gxTranslatef(x, y, 0);
							pushBlend(BLEND_ALPHA);
							setColor(255, 255, 255, 227);
							gxSetTexture(jpegLoop[i]->texture);
							drawRect(0, 0, sx, sy);
							gxSetTexture(0);
							popBlend();
						}
						gxPopMatrix();
					}
				#endif
				
				#if 1
					const int padding = 10;
					setColor(255, 0, 0, 191);
					const int sxf = GFX_SX - padding * 2;
					const double sxt = sxf * time / duration;
					drawRect(padding, GFX_SY - padding - 40, padding + sxt, GFX_SY - padding);
					setColor(colorBlack);
					drawRectLine(padding, GFX_SY - padding - 40, padding + sxf, GFX_SY - padding);
					
					setColor(colorWhite);
					setFont("calibri.ttf");
					int timeInSeconds = int(std::floor(time));
					const int hh = timeInSeconds / 3600;
					timeInSeconds -= hh * 3600;
					const int mm = timeInSeconds / 60;
					timeInSeconds -= mm * 60;
					const int ss = timeInSeconds;
					drawText(GFX_SX/2, GFX_SY - padding - 22, 22, 0.f, -.5f, "%02d:%02d:%02d - %gx", hh, mm, ss, speedFactor);
				#endif
				}
				popSurface();
				
				pushBlend(BLEND_OPAQUE);
				setColor(colorWhite);
				gxSetTexture(surface->getTexture());
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
			}
			framework.endDraw();
		}
		
		for (int i = 0; i < kNumJpegLoops; ++i)
		{
			delete jpegLoop[i];
			jpegLoop[i] = nullptr;
		}
		
		delete surface;
		surface = nullptr;
		
		delete [] fileBuffer;
		fileBuffer = nullptr;
		fileBufferSize = 0;
		
		delete [] dstBuffer;
		dstBuffer = nullptr;
		dstBufferSize = 0;
	}
	framework.shutdown();
	
	return 0;
#endif
	
	//
	
#if defined(DEBUG)
	framework.enableRealTimeEditing = true;
#endif

	//framework.fullscreen = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		//testPortaudio();
		
		framework.fillCachesWithPath(".", false);
		
		//initOsc();
		
		//initAudioOutput();
	
	#if ENABLE_LEAPMOTION
		// initialise LeapMotion controller

		Leap::Controller leapController;
		leapController.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

		LeapListener * leapListener = new LeapListener();
		leapController.addListener(*leapListener);
	#endif

		Surface surface(GFX_SX, GFX_SY, false);
		surface.clear();
		
	#if DO_VIDEOLOOPS
		Surface * layerColors[NUM_LAYERS] = { };
		Surface * layerAlphas[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			layerColors[i] = new Surface(GFX_SX, GFX_SY, true);
			layerColors[i]->clear();
			
			layerAlphas[i] = new Surface(GFX_SX, GFX_SY, false);
			layerAlphas[i]->clear();
		}
		
	#if 0
		const char * videoFilenames[NUM_LAYERS] =
		{
			"testvideos/video5.mpg",
			"testvideos/video5.mpg",
			"testvideos/video5.mpg",
		};
	#elif 0
		const char * videoFilenames[NUM_LAYERS] =
		{
			"testvideos/video4.mp4",
			"testvideos/video4.mp4",
			"testvideos/video4.mp4",
		};
	#else
		const char * videoFilenames[NUM_LAYERS] =
		{
			//"testvideos/video1.mpg",
			"testvideos/video6.mpg",
			"testvideos/video2.mpg",
			"testvideos/video4.mp4",
		};
	#endif
		
		VideoLoop * videoLoops[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			videoLoops[i] = new VideoLoop(videoFilenames[i]);
			
			videoLoops[i]->tick(0.f);
		}
	#endif
		
		VideoLoop * glitchLoop = new VideoLoop("testvideos/glitch3.mp4");
		
	#if DO_GAME
		VideoGame * videoGame = new VideoGame();
	#endif
		
		GrooopLogo * grooopLogo = new GrooopLogo();
		
		mixingPanel = UIMixingPanel();
		mixingPanel.px = 100;
		mixingPanel.py = 50;
		
		Dictionary mixingSettings;
		if (mixingSettings.load("mixing.txt"))
		{
			for (int i = 0; i < UIMixingPanel::kNumSliders; ++i)
			{
				char name[32];
				sprintf_s(name, sizeof(name), "slider%03d", i);
				mixingPanel.sliders[i].value = mixingSettings.getFloat(name, 0.f);
			}
		}
		
		Surface mask(GFX_SX, GFX_SY, false);
		
		float mouseDownTime = 0.f;
		int activeLayer = 0;
		float blurStrength = 0.f;
		float desiredBlurStrength = 0.f;
		bool showBackgroundVideo = true;
		bool showVideoEffects = true;
		bool showGrooopCircles = true;
		bool drawIsolines = false;
		FollowValue barAngle(0.f, .9f);
		FollowValue invertValue(0.f, .9f);
		
		GrainsEffect grainsEffect;
		int nextGrainIndex = 0;
		
		auto uploadedImages = listFiles("/Users/thecat/Downloads/messenger-platform-samples/node/uploads/images", false);
		for (auto i = uploadedImages.begin(); i != uploadedImages.end(); )
			if (!String::EndsWith((*i), ".jpg") && !String::EndsWith((*i), ".png"))
				i = uploadedImages.erase(i);
			else
				++i;
		int uploadedImageIndex = 0;
		float uploadedImageFade = 0.f;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
		#if ENABLE_LEAPMOTION
			// process LeapMotion input

			leapListener->tick();
		#endif
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_1))
				activeLayer = 0;
			if (keyboard.wentDown(SDLK_2))
				activeLayer = 1;
			if (keyboard.wentDown(SDLK_3))
				activeLayer = 2;
			if (keyboard.wentDown(SDLK_3))
				activeLayer = 3;
			
			if (activeLayer > NUM_LAYERS - 1)
				activeLayer = NUM_LAYERS - 1;
			
			if (keyboard.wentDown(SDLK_b))
				desiredBlurStrength = 1.f - desiredBlurStrength;
			
			if (keyboard.wentDown(SDLK_v))
				showBackgroundVideo = !showBackgroundVideo;
			
			if (keyboard.wentDown(SDLK_e))
				showVideoEffects = !showVideoEffects;
			if (keyboard.wentDown(SDLK_c))
				showGrooopCircles = !showGrooopCircles;
			
			if (keyboard.wentDown(SDLK_i))
				drawIsolines = !drawIsolines;
			
			const float dt = framework.timeStep;
			
			blurStrength = Calc::Lerp(desiredBlurStrength, blurStrength, std::powf(.5f, dt));
			
			grainsEffect.tick(dt);
			
		#if DO_VIDEOLOOPS
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				if (i == 0)
				{
					pushSurface(layerAlphas[i]);
					{
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						drawRect(0, 0, GFX_SX, GFX_SY);
						popBlend();
					}
					popSurface();
				}
				
				{
					if (i != 0)
					{
						pushSurface(layerAlphas[i]);
						{
							pushBlend(BLEND_SUBTRACT);
							setColor(1, 1, 1, 255);
							drawRect(0, 0, GFX_SX, GFX_SY);
							popBlend();
						}
						popSurface();
					}
					
					if (1 && i == 1)
					{
						pushSurface(layerAlphas[i]);
						{
							static Traveller traveller;
							
							pushBlend(BLEND_ADD);
							setColor(255, 255, 255, 255, 31);
							
							if (!traveller.m_Active)
							{
								static float x = mouse.x;
								static float y = mouse.y;
								
								auto callback = [](void * obj, const TravelEvent & e)
								{
									x = Calc::Lerp(x, e.x, .05f);
									y = Calc::Lerp(y, e.y, .05f);
									
									Sprite brush("brushes/brush1006.png");
									brush.pivotX = brush.getWidth() / 2.f;
									brush.pivotY = brush.getHeight() / 2.f;
									//brush.drawEx(e.x, e.y, 0.f, .2f);
									brush.drawEx(x, y, 0.f, .2f);
								};
								
								traveller.Setup(2.f, callback, nullptr);
								traveller.Begin(mouse.x, mouse.y);
							}
							
							traveller.Update(mouse.x, mouse.y);
							
							popBlend();
						}
						popSurface();
					}
					
					if (0 && i == 0)
					{
						static Traveller traveller;
						static Surface * surface = nullptr;
						
						surface = layerColors[i];
						//surface = layerAlphas[i];
						
						pushBlend(BLEND_ALPHA);
						{
							if (!traveller.m_Active)
							{
								static float x = mouse.x;
								static float y = mouse.y;
								
								auto callback = [](void * obj, const TravelEvent & e)
								{
									x = Calc::Lerp(x, e.x, .05f);
									y = Calc::Lerp(y, e.y, .05f);
									
									// todo : use (x, y) and calculate deltas
									
									Shader shader("paint-smudge");
									setShader(shader);
									{
										const GLuint brush = getTexture("brushes/brush1006.png");
										shader.setTexture("brush", 0, brush);
										shader.setTexture("colormap", 1, surface->getTexture());
										
										surface->swapBuffers();
										
										pushSurface(surface);
										{
											setColor(colorWhite);
											gxBegin(GL_QUADS);
											{
												const float s = 30.f;
												gxTexCoord2f(0.f, 0.f); gxVertex4f(e.x - s, e.y - s, e.dx, e.dy);
												gxTexCoord2f(1.f, 0.f); gxVertex4f(e.x + s, e.y - s, e.dx, e.dy);
												gxTexCoord2f(1.f, 1.f); gxVertex4f(e.x + s, e.y + s, e.dx, e.dy);
												gxTexCoord2f(0.f, 1.f); gxVertex4f(e.x - s, e.y + s, e.dx, e.dy);
												//gxTexCoord2f(0.f, 0.f); gxVertex2f(e.x - s, e.y - s);
												//gxTexCoord2f(1.f, 0.f); gxVertex2f(e.x + s, e.y - s);
												//gxTexCoord2f(1.f, 1.f); gxVertex2f(e.x + s, e.y + s);
												//gxTexCoord2f(0.f, 1.f); gxVertex2f(e.x - s, e.y + s);
											}
											gxEnd();
										}
										popSurface();
									}
									clearShader();
								};
								
								traveller.Setup(2.f, callback, nullptr);
								traveller.Begin(mouse.x, mouse.y);
							}
							
							traveller.Update(mouse.x, mouse.y);
						}
						popBlend();
					}
					
					if (showGrooopCircles && i == 1)
					{
						pushSurface(layerAlphas[i]);
						{
							gxPushMatrix();
							{
								gxTranslatef(GFX_SX/2.f, GFX_SY/2.f, 0.f);
								
								const float scale = std::cos(framework.time * .1f) + 1.2f;
								gxScalef(scale, scale, 1.f);
								
								const float angle = std::cos(framework.time * .1f) * 360.f;
								gxRotatef(angle, 0.f, 0.f, 1.f);
								
								hqBegin(HQ_STROKED_CIRCLES);
								{
									setColor(colorBlack);
									hqStrokeCircle(0, 0, 100, 30.f);
									
									setColor(colorWhite);
									hqStrokeCircle(0, 0, 100, 5.f);
								}
								hqEnd();
								
								for (int x = -3; x <= +3; ++x)
								{
									gxPushMatrix();
									gxTranslatef(x * 150 * scale, 0, 0);
									gxScalef(scale, scale, 1.f);
									
									hqBegin(HQ_STROKED_CIRCLES);
									{
										setColor(colorBlack);
										hqStrokeCircle(0, 0, 50, 5.f);
										
										setColor(colorWhite);
										hqStrokeCircle(0, 0, 50, 3.f);
									}
									hqEnd();
									
									gxPopMatrix();
								}
							}
							gxPopMatrix();
						}
						popSurface();
					}
				}
			}
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				videoLoops[i]->tick(dt);
			}
		#endif
			
			glitchLoop->tick(dt);
			
			{
				static float speed = 1.f;
				
				if (leapController.isConnected() && leapController.hasFocus())
				{
					if (g_leapState.hands[0].active)
					{
						const float maxSpeed = 4.f;
						
						float desiredSpeed = Calc::Max(0.f, (g_leapState.hands[0].fingers[0].position[1] - 50) / 50.f);
						desiredSpeed *= desiredSpeed;
						desiredSpeed *= maxSpeed;
						desiredSpeed = Calc::Mid(desiredSpeed, 0.f, maxSpeed);
						
						speed = Calc::Lerp(desiredSpeed, speed, std::powf(.05f, dt));
					}
				}
			}
			
			{
				static float speed = .5f;
				
				if (leapController.isConnected() && leapController.hasFocus())
				{
					if (g_leapState.hands[1].active)
					{
						const float maxSpeed = 4.f;
						
						float desiredSpeed = Calc::Max(0.f, (g_leapState.hands[1].fingers[0].position[1] - 50) / 50.f);
						desiredSpeed *= desiredSpeed;
						desiredSpeed *= maxSpeed;
						desiredSpeed = Calc::Mid(desiredSpeed, 0.f, maxSpeed);
						
						speed = Calc::Lerp(desiredSpeed, speed, std::powf(.05f, dt));
					}
				}
			}
			
		#if DO_GAME
			videoGame->tick(dt);
		#endif
			
			if (kMixLogoOpacity == 0.f)
			{
				delete grooopLogo;
				grooopLogo = nullptr;
				
				grooopLogo = new GrooopLogo();
			}
			
			grooopLogo->tick(dt);
			
			mixingPanel.tick(dt);
			
			uploadedImageFade += dt / 4.f;
			
			if (!uploadedImages.empty() && uploadedImageFade >= 1.f)
			{
				uploadedImageFade = 0.f;
				uploadedImageIndex = (uploadedImageIndex + 1) % uploadedImages.size();
			}
			
		#if DO_VIDEOLOOPS
			if (mouse.isDown(BUTTON_LEFT))
			{
				mouseDownTime += dt;
				
				Grain & grain = grainsEffect.grains[nextGrainIndex];
				nextGrainIndex = (nextGrainIndex + 1) % GrainsEffect::kMaxGrains;
				grain.life = 4.f;
				grain.lifeRcp = 1.f / grain.life;
				grain.px = mouse.x;
				grain.py = mouse.y;
				const float speed = random(5.f, 15.f);
				grain.vx = mouse.dx * speed;
				grain.vy = mouse.dy * speed;
				grain.vfalloff = .9;
				
			#if 1
				pushSurface(layerAlphas[activeLayer]);
				{
					const float radius = Calc::Min(mouseDownTime / .2f, 1.f) * 50.f;
					
					Shader shader("gradient-circle");
					setShader(shader);
					{
						pushBlend(BLEND_ADD);
						drawRect(mouse.x - radius, mouse.y - radius, mouse.x + radius, mouse.y + radius);
						popBlend();
					}
					clearShader();
				}
				popSurface();
			#endif
			}
			else
			{
				mouseDownTime = 0.f;
			}
			
			if (keyboard.wentDown(SDLK_r))
				barAngle.targetValue += 45.f;
			if (keyboard.wentDown(SDLK_i))
				invertValue.targetValue += .2f;
			
			barAngle.tick(dt);
			
			invertValue.tick(dt);
			
			pushSurface(layerAlphas[activeLayer]);
			{
				pushBlend(BLEND_ADD);
				grainsEffect.draw();
				popBlend();
			}
			popSurface();
		#endif
			
			framework.beginDraw(0, 0, 0, 0);
			{
				if (showBackgroundVideo == false)
				{
					//
				}
				else
				{
				#if DO_VIDEOLOOPS
					GLuint layerVideos[NUM_LAYERS] = { };
					
					for (int i = 0; i < NUM_LAYERS; ++i)
					{
						layerVideos[i] = videoLoops[i]->getTexture();
						
						pushSurface(layerColors[i]);
						{
							//setColorf(1.f, 1.f, 1.f, 1.f);
							setColorf(1.f, 1.f, 1.f, Calc::Lerp(0.f, 1.f, mouse.x / float(GFX_SX)));
							gxSetTexture(layerVideos[i]);
							drawRect(0, 0, GFX_SX, GFX_SY);
							gxSetTexture(0);
						}
						popSurface();
						
						pushBlend(BLEND_OPAQUE);
						applyFsfx(*layerColors[i], "fsfx/godrays.ps", Calc::Lerp(0.f, 2.f, mouse.y / float(GFX_SY)));
						popBlend();
					}
					
					{
						Shader shader("compose-layers");
						setShader(shader);
						{
							shader.setTexture("layerAlpha0", 0, layerAlphas[0]->getTexture());
							shader.setTexture("layerAlpha1", 1, layerAlphas[1]->getTexture());
							shader.setTexture("layerAlpha2", 2, layerAlphas[2]->getTexture());
							shader.setTexture("layerColor0", 3, layerColors[0]->getTexture());
							shader.setTexture("layerColor1", 4, layerColors[1]->getTexture());
							shader.setTexture("layerColor2", 5, layerColors[2]->getTexture());
							
							pushBlend(BLEND_OPAQUE);
							surface.postprocess();
							popBlend();
						}
						clearShader();
					}
					
				#if 1
					mask.clear();
					pushSurface(&mask);
					{
						gxPushMatrix();
						{
							gxTranslatef(GFX_SX/3.f, GFX_SY/3.f, 0.f);
							//gxRotatef(framework.time * 10.f, 0.f, 0.f, 1.f);
							gxRotatef(barAngle.value, 0.f, 0.f, 1.f);
							
							setColor(colorWhite);
							hqBegin(HQ_FILLED_RECTS);
							{
								hqFillRect(-100, -10000, 0, +10000);
								
								hqFillRect(20, -10000, 25, +10000);
								hqFillRect(40, -10000, 45, +10000);
							}
							hqEnd();
							
							hqBegin(HQ_STROKED_CIRCLES);
							{
								for (int y = -30; y <= +30; ++y)
								{
									hqStrokeCircle(60, y * 20, 3.5f, 2.f);
								}
							}
							hqEnd();
						}
						gxPopMatrix();
					}
					popSurface();
					
					if (leapController.isConnected() && leapController.hasFocus())
					{
						if (g_leapState.hands[0].active)
						{
							pushBlend(BLEND_OPAQUE);
							setShader_GaussianBlurH(surface.getTexture(), 60, std::cosf(g_leapState.hands[0].fingers[2].position[1] / 255.f * 5.f) * 100.f);
							surface.postprocess();
							clearShader();
							popBlend();
						}
						
						if (g_leapState.hands[0].active)
						{
							pushBlend(BLEND_OPAQUE);
							setShader_GaussianBlurV(surface.getTexture(), 60, std::cosf(g_leapState.hands[0].fingers[1].position[1] / 255.f * 5.f) * 100.f);
							surface.postprocess();
							clearShader();
							popBlend();
						}
					}
					
				#if 0
					pushBlend(BLEND_OPAQUE);
					setShader_GaussianBlurH(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 2.345f) * 40.f);
					surface.postprocess();
					clearShader();
					popBlend();
				#elif 1
					{
						pushBlend(BLEND_OPAQUE);
						GLuint texture = surface.getTexture();
						surface.swapBuffers();
						pushSurface(&surface);
						{
							for (int i = 0; i < 20; ++i)
							{
								const float t1 = (i + 0) / 20.f;
								const float t2 = (i + 1) / 20.f;
								const float y1 = t1 * GFX_SY;
								const float y2 = t2 * GFX_SY;
								const float blurStrengthModifier = std::cos(i / 10.f + framework.time);
								const float radius = blurStrength * blurStrengthModifier * std::cos(framework.time / 6.789f) * 200.f;
								setShader_GaussianBlurH(texture, 63, radius);
								gxBegin(GL_QUADS);
								{
									gxTexCoord2f(0.f, 1.f - t1); gxVertex2f(0,      y1);
									gxTexCoord2f(1.f, 1.f - t1); gxVertex2f(GFX_SX, y1);
									gxTexCoord2f(1.f, 1.f - t2); gxVertex2f(GFX_SX, y2);
									gxTexCoord2f(0.f, 1.f - t2); gxVertex2f(0,      y2);
								}
								gxEnd();
								clearShader();
							}
						}
						popSurface();
						popBlend();
					}
				#endif
					
				#if 0
					pushBlend(BLEND_OPAQUE);
					setShader_GaussianBlurV(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 1.123f) * 20.f);
					surface.postprocess();
					clearShader();
					popBlend();
				#elif 1
					pushBlend(BLEND_OPAQUE);
					GLuint texture = surface.getTexture();
					surface.swapBuffers();
					pushSurface(&surface);
					{
						for (int i = 0; i < 30; ++i)
						{
							const float t1 = (i + 0) / 30.f;
							const float t2 = (i + 1) / 30.f;
							const float x1 = t1 * GFX_SX;
							const float x2 = t2 * GFX_SX;
							const float blurStrengthModifier = std::cos(i / 10.f + framework.time);
							const float radius = blurStrength * blurStrengthModifier * std::cos(framework.time / 3.456f) * 100.f;
							setShader_GaussianBlurV(texture, 63, radius);
							gxBegin(GL_QUADS);
							{
								gxTexCoord2f(t1, 1.f); gxVertex2f(x1, 0.f);
								gxTexCoord2f(t2, 1.f); gxVertex2f(x2, 0.f);
								gxTexCoord2f(t2, 0.f); gxVertex2f(x2, GFX_SY);
								gxTexCoord2f(t1, 0.f); gxVertex2f(x1, GFX_SY);
							}
							gxEnd();
							clearShader();
						}
					}
					popSurface();
					popBlend();
				#endif
				
					applyFsfx(surface, "fsfx/invert.ps", (-std::cosf(invertValue.value * Calc::m2PI) + 1.f) / 2.f);
					
					pushBlend(BLEND_OPAQUE);
					setShader_ColorTemperature(surface.getTexture(), (std::cosf(framework.time / 2.f) + 1.f) / 2.f, 1.f);
					surface.postprocess();
					clearShader();
					popBlend();
					
					pushBlend(BLEND_OPAQUE);
					applyMask(surface.getTexture(), layerColors[0]->getTexture(), mask.getTexture());
					popBlend();
				#endif
				#endif
				}
			
			#if DO_GAME
				if (showVideoEffects)
				{
					videoGame->draw();
					
					pushSurface(&surface);
					{
						pushBlend(BLEND_OPAQUE);
						{
							setColor(colorWhite);
							gxSetTexture(videoGame->postSurface->getTexture());
							drawRect(0, 0, videoGame->postSurface->getWidth(), videoGame->postSurface->getHeight());
							gxSetTexture(0);
						}
						popBlend();
					}
					popSurface();
				}
			#endif
				
				if (drawIsolines)
				{
					if (false)
					{
						pushBlend(BLEND_OPAQUE);
						setShader_GaussianBlurH(surface.getTexture(), 10, 10.f);
						surface.postprocess();
						clearShader();
						setShader_GaussianBlurV(surface.getTexture(), 10, 10.f);
						surface.postprocess();
						clearShader();
						popBlend();
					}
				
					applyFsfx(surface, "fsfx/isolines.ps", 1.f);
				}
				
				pushSurface(&surface);
				{
					grooopLogo->draw();
				}
				popSurface();
				
				pushSurface(&surface);
				{
					if (!uploadedImages.empty())
					{
						const char * filename = uploadedImages[uploadedImageIndex].c_str();
						
						const GLuint texture = getTexture(filename);
						
						gxPushMatrix();
						{
							gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
							
							float opacity = 0.f;
							
							if (uploadedImageFade < .25f)
								opacity = EvalEase((uploadedImageFade - 0.f) / .25f, kEaseType_SineIn, 0.f);
							else if (uploadedImageFade > .75f)
								opacity = 1.f - EvalEase((uploadedImageFade - .75f) / .25f, kEaseType_SineOut, 0.f);
							else
								opacity = 1.f;
							
							//const float opacity = std::sinf(uploadedImageFade * M_PI);
							
							gxSetTexture(texture);
							{
								setColorf(1.f, 1.f, 1.f, kMixUserContentOpacity * opacity);
								drawRect(-100, -100, +100, +100);
							}
							gxSetTexture(0);
						}
						gxPopMatrix();
					}
					
					if (false)
					for (int i = 0; i < (int)uploadedImages.size(); ++i)
					{
						const int cx = i % 6;
						const int cy = i / 6;
						
						const int sx = 100;
						const int sy = 100;
						const int px = 10;
						const int py = 10;
						
						const int x = cx * (sx + px/2);
						const int y = cy * (sy + py/2);
						
						const GLuint texture = getTexture(uploadedImages[i].c_str());
						
						gxSetTexture(texture);
						{
							setColorf(1.f, 1.f, 1.f, kMixUserContentOpacity);
							drawRect(x, y, x + sx, y + sy);
						}
						gxSetTexture(0);
					}
					
					mixingPanel.draw();
				}
				popSurface();
				
				pushBlend(BLEND_OPAQUE);
				{
					setColor(colorWhite);
					gxSetTexture(surface.getTexture());
					drawRect(0, 0, surface.getWidth(), surface.getHeight());
					gxSetTexture(0);
				}
				popBlend();
				
				//logDebug("presentTime: %g", presentTime);

			#if ENABLE_LEAPMOTION
				int y = 10;
				
				setFont("calibri.ttf");
				
				drawText(5, y, 16, +1, +1, "LeapMotion connected: %d, hasFocus: %d", (int)leapController.isConnected(), (int)leapController.hasFocus());
				y += 30;

				if (leapController.isConnected() && leapController.hasFocus())
				{
					for (int h = 0; h < 2; ++h)
					{
						if (g_leapState.hands[h].active == false)
							continue;
						
						setColor(255, 255, 255, 255);
						drawText(5, y, 16, +1, +1, "LeapMotion palm position: (%03d, %03d, %03d)",
							(int)g_leapState.hands[h].position[0],
							(int)g_leapState.hands[h].position[1],
							(int)g_leapState.hands[h].position[2]);
						y += 24;
						
						for (int f = 0; f < 5; ++f)
						{
							setColor(191, 191, 191, 255);
							drawText(5, y, 16, +1, +1, "    finger position: (%03d, %03d, %03d)",
								(int)g_leapState.hands[h].fingers[f].position[0],
								(int)g_leapState.hands[h].fingers[f].position[1],
								(int)g_leapState.hands[h].fingers[f].position[2]);
							y += 24;
							
							gxPushMatrix();
							{
								gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
								setColor(255, 255, 255, g_leapState.hands[h].fingers[f].position[1]);
								hqBegin(HQ_STROKED_CIRCLES);
								{
									hqStrokeCircle(
										g_leapState.hands[h].fingers[f].position[0],
										g_leapState.hands[h].fingers[f].position[2],
										5.f,
										6.f);
								}
								hqEnd();
							}
							gxPopMatrix();
						}
						
						y += 6;
					}
				}
			#endif
			}
			framework.endDraw();
		}
		
		// save mixing settings
		
		for (int i = 0; i < UIMixingPanel::kNumSliders; ++i)
		{
			char name[32];
			sprintf_s(name, sizeof(name), "slider%03d", i);
			
			mixingSettings.setFloat(name, mixingPanel.sliders[i].value);
		}
		
		mixingSettings.save("mixing.txt");
		
		//
		
		delete grooopLogo;
		grooopLogo = nullptr;
		
	#if DO_GAME
		delete videoGame;
		videoGame = nullptr;
	#endif
		
		delete glitchLoop;
		glitchLoop = nullptr;
		
	#if DO_VIDEOLOOPS
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete videoLoops[i];
			videoLoops[i] = nullptr;
		}
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete layerAlphas[i];
			layerAlphas[i] = nullptr;
			
			delete layerColors[i];
			layerColors[i] = nullptr;
		}
	#endif
		
	#if ENABLE_LEAPMOTION
		leapController.removeListener(*leapListener);

		delete leapListener;
		leapListener = nullptr;
	#endif
	
		shutAudioOutput();
		
		shutOsc();
		
		framework.shutdown();
	}
	
	return 0;
}
