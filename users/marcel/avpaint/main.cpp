#include "Calc.h"
#include "framework.h"
#include "leapstate.h"
#include "StringEx.h"
#include "TweenFloat.h"
#include "videoloop.h"

#if ENABLE_LEAPMOTION
	#include "leap/Leap.h"
#endif

extern void testAvpaint();
extern void testJpegStreamer();
extern void testPortaudio();

// todo : integrate Facebook Messenger Node.js app with https://cloud.google.com/vision/, for inappropriate content detection
// todo : write something to extract dominant colors from (crowd sourced) images

// todo : mask using rotating and scaling objects as mask alpha
// todo : grooop logo animation
// todo : think of ways to mix/vj
// todo : use touchpad for map-like moving and scaling videos?
// todo : multitouch touch pad? can SDL handle this? else look for api

/*

https://github.com/LabSound/LabSound
LabSound is a graph-based audio engine built using modern C++11. As a fork of the WebAudio implementation in Chrome, LabSound implements the WebAudio API specification while extending it with new features, nodes, bugfixes, and performance improvements.

The engine is packaged as a batteries-included static library meant for integration in many types of software: games, visualizers, interactive installations, live coding environments, VST plugins, audio editing/sequencing applications, and more.

*/

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

#define LOOP_SX_TOTAL 1440
#define LOOP_SX 822
#define LOOP_SY 874

//#define kMixVideoSurfaceOpacity (mouse.x / float(GFX_SX))
//#define kMixPostSurfaceRetain (mouse.y / float(GFX_SY))

#define kMixVideoSurfaceOpacity mixingPanel.sliders[0].value
#define kMixPostSurfaceRetain mixingPanel.sliders[1].value
#define kMixLogoOpacity mixingPanel.sliders[2].value
#define kMixUserContentOpacity mixingPanel.sliders[3].value
#define kMixVideoSpeed1 mixingPanel.sliders[4].value
#define kMixVideoSpeed2 mixingPanel.sliders[5].value
#define kMixGodraysStrength mixingPanel.sliders[6].value

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

void applyFsfx(Surface & surface, const char * name, const float strength = 1.f, const float param1 = 0.f, const float param2 = 0.f, const float param3 = 0.f, const float param4 = 0.f, GLuint texture1 = 0)
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
	#if 1
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
	#else
		VideoLoopInfo videoLoopInfo;
		videoLoopInfo.videoFilenameL = "testvideos/video7.mp4";
		videoLoopInfo.videoFilenameR = "testvideos/video7.mp4";
		videoLoopInfos.push_back(videoLoopInfo);
	#endif
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

#include <sys/stat.h>

int main(int argc, char * argv[])
{
#if defined(DEBUG)
	framework.enableRealTimeEditing = true;
#endif

	//
	
	testAvpaint();
	
	testJpegStreamer();
	
	//
	
	//framework.fullscreen = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		testPortaudio();
		
		changeDirectory("/Users/thecat/Google Drive/The Grooop - Welcome");
	
		framework.fillCachesWithPath(".", false);

	#if ENABLE_LEAPMOTION
		// initialise LeapMotion controller

		Leap::Controller leapController;
		leapController.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

		LeapListener * leapListener = new LeapListener();
		leapController.addListener(*leapListener);
	#endif

		Surface surface(GFX_SX, GFX_SY, false);
		surface.clear();
		
		VideoLoop * glitchLoop = new VideoLoop("testvideos/glitch3.mp4");
		
		VideoGame * videoGame = new VideoGame();
		
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
		
		bool showVideoEffects = true;
		bool drawIsolines = false;
		
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
						
			if (keyboard.wentDown(SDLK_e))
				showVideoEffects = !showVideoEffects;
			
			if (keyboard.wentDown(SDLK_l))
				drawIsolines = !drawIsolines;
			
			const float dt = framework.timeStep;
			
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
			
			videoGame->tick(dt);
			
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
			
			framework.beginDraw(0, 0, 0, 0);
			{
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
		
		delete videoGame;
		videoGame = nullptr;
		
		delete glitchLoop;
		glitchLoop = nullptr;
		
	#if ENABLE_LEAPMOTION
		leapController.removeListener(*leapListener);

		delete leapListener;
		leapListener = nullptr;
	#endif
	
		framework.shutdown();
	}
	
	return 0;
}
