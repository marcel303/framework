#include "Benchmark.h"
#include "framework.h"
#include "vfxNodes/openglTexture.h"
#include "ps3eye.h"

#include "../libparticle/ui.h"
#include "vfxNodes/dotDetector.h"
#include "vfxNodes/dotTracker.h"

#include <thread>

#define PS3_MAX_SX 640
#define PS3_MAX_SY 480

#define MAX_DOTS 32
#define MAX_DOT_RADIUS 30

extern const int GFX_SX;
extern const int GFX_SY;

using namespace ps3eye;

void testPs3eye()
{
	SDL_mutex * mutex = SDL_CreateMutex();
	Assert(mutex != nullptr);
	
	const bool enableColor = true;
	
    PS3EYECam::PS3EYERef eye;

    // list out the devices
    std::vector<PS3EYECam::PS3EYERef> devices;
    {
    	Benchmark bm("get devices");
		devices = PS3EYECam::getDevices();
	}
	logDebug("found %d devices", devices.size());
	
	uint8_t * frameData = nullptr;
	uint8_t * lumiData = nullptr;
	uint8_t * maskData = nullptr;
	
	OpenglTexture texture;
	OpenglTexture textureLumi;
	OpenglTexture textureMask;
	
    if (!devices.empty())
    {   
        eye = devices.at(0);
		
        const bool result = eye->init(320, 240, 187, enableColor ? PS3EYECam::EOutputFormat::RGB : PS3EYECam::EOutputFormat::Gray);
		
        logDebug("eye init result: %d", result);
		
		if (result)
		{
			{
				Benchmark bm("eye start");
				
				eye->start();
			}
			
			const int sx = eye->getWidth();
			const int sy = eye->getHeight();
			
			frameData = new uint8_t[sx * sy * (enableColor ? 3 : 1)];
			
			lumiData = new uint8_t[sx * sy];
			maskData = new uint8_t[sx * sy];
			
			{
				Benchmark bm("texture create");
				
				texture.allocate(eye->getWidth(), eye->getHeight(), enableColor ? GL_RGB8 : GL_R8, true, true);
				if (!enableColor)
					texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
				
				textureLumi.allocate(sx, sy, GL_R8, true, true);
				textureLumi.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
				
				textureMask.allocate(sx, sy, GL_R8, true, true);
				textureMask.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
			}
		}
	}
	
	std::atomic_bool autoGain(eye->getAutogain());
	std::atomic_int gain(eye->getGain());
	std::atomic_bool autoWhiteBalance(eye->getAutoWhiteBalance());
	std::atomic_int whiteBalance(eye->getRedBalance());
	std::atomic_int exposure(eye->getExposure());
	std::atomic_int sharpness(eye->getSharpness());
	std::atomic_int hue(eye->getHue());
	std::atomic_int brightness(eye->getBrightness());
	std::atomic_int contrast(eye->getContrast());
	std::atomic_int frameIndex(0);
	
	UiState uiState;
	uiState.x = 20;
	uiState.y = 20;
	uiState.sx = 200;
	
	std::atomic_bool stop(false);
	
	int numDots_shared = 0;
	TrackedDot trackedDots_shared[MAX_DOTS];
	
	std::thread thread([&]()
		{
			DotTracker dotTracker;
			
			while (stop == false)
			{
				if (eye)
				{
				#if 1
					if (autoGain != eye->getAutogain())
						eye->setAutogain(autoGain);
					if (gain != eye->getGain())
						eye->setGain(gain);
					
					if (autoWhiteBalance != eye->getAutoWhiteBalance())
						eye->setAutoWhiteBalance(autoWhiteBalance);
					if (autoWhiteBalance == false)
					{
						if (eye->getRedBalance() != whiteBalance)
						{
							eye->setRedBalance(whiteBalance);
							eye->setGreenBalance(whiteBalance);
							eye->setBlueBalance(whiteBalance);
						}
					}
					
					if (exposure != eye->getExposure())
						eye->setExposure(exposure);
					if (sharpness != eye->getSharpness())
						eye->setSharpness(sharpness);
					
					if (hue != eye->getHue())
						eye->setHue(hue);
					
					if (brightness != eye->getBrightness())
						eye->setBrightness(brightness);
					if (contrast != eye->getContrast())
						eye->setContrast(contrast);
				#endif
				
					eye->getFrame(frameData);
					
					frameIndex++;
					
					const int sx = eye->getWidth();
					const int sy = eye->getHeight();
					
					// create a luminance map from the RGB camera image

					for (int y = 0; y < sy; ++y)
					{
						const uint8_t * __restrict src = frameData + y * sx * 3;
							  uint8_t * __restrict dst = lumiData + y * sx;
						
						for (int x = 0; x < sx; ++x)
						{
							const int value = (src[x * 3 + 0] + (src[x * 3 + 1] << 1) + src[x * 3 + 2]) >> 2;
							
							dst[x] = value;
						}
					}

					// perform dot detection by first performing a masking operation and then detecting the dots

					DotDetector::treshold(lumiData, sx, maskData, sx, sx, sy, DotDetector::kTresholdTest_GreaterEqual, 127);

					DotIsland islands[MAX_DOTS];

					const int numDots = DotDetector::detectDots(maskData, sx, sy, MAX_DOT_RADIUS, islands, MAX_DOTS, true);

					TrackedDot trackedDots[MAX_DOTS];
					
					for (int i = 0; i < numDots; ++i)
					{
						auto & island = islands[i];
						auto & trackedDot = trackedDots[i];
						
						trackedDot.x = island.x;
						trackedDot.y = island.y;
					}
					
					dotTracker.identify(trackedDots, numDots, 1.f / eye->getFrameRate(), 10.f);
					
					SDL_LockMutex(mutex);
					{
						numDots_shared = numDots;
						memcpy(trackedDots_shared, trackedDots, sizeof(trackedDots_shared));
					}
					SDL_UnlockMutex(mutex);
				}
			}
		});
	
	int framesPerSecond = 0;
	int lastFrameIndex = 0;
	int lastSecond = 0;
	
	do
	{
		//SDL_Delay(1000);
		
		framework.process();
		
		const int second = int(framework.time);
		if (second != lastSecond)
		{
			lastSecond = second;
			framesPerSecond = frameIndex - lastFrameIndex;
			lastFrameIndex = frameIndex;
		}
		
		if (eye)
		{
			texture.upload(frameData, 4, 0, enableColor ? GL_RGB : GL_RED, GL_UNSIGNED_BYTE);
			
			textureLumi.upload(lumiData, 1, 0, GL_RED, GL_UNSIGNED_BYTE);
			textureMask.upload(maskData, 1, 0, GL_RED, GL_UNSIGNED_BYTE);
			
			int numDots;
			TrackedDot trackedDots[MAX_DOTS];
			
			SDL_LockMutex(mutex);
			{
				numDots = numDots_shared;
				memcpy(trackedDots, trackedDots_shared, sizeof(trackedDots));
			}
			SDL_UnlockMutex(mutex);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				
				const GLuint textureId =
					keyboard.isDown(SDLK_m) ? textureMask.id :
					keyboard.isDown(SDLK_l) ? textureLumi.id :
					texture.id;
				
				gxSetTexture(textureId);
				{
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
				
				gxPushMatrix();
				{
					const float scaleX = GFX_SX / float(eye->getWidth());
					const float scaleY = GFX_SY / float(eye->getHeight());
					
					gxScalef(scaleX, scaleY, 1.f);
					
					hqBegin(HQ_STROKED_CIRCLES);
					{
						setColor(colorGreen);
						
						for (int i = 0; i < numDots; ++i)
						{
							auto & d = trackedDots[i];
							
							hqStrokeCircle(d.x, d.y, MAX_DOT_RADIUS/2.f, 2.f);
						}
					}
					hqEnd();
					
					beginTextBatch();
					{
						for (int i = 0; i < numDots; ++i)
						{
							auto & d = trackedDots[i];
							
							drawText(d.x, d.y, 12.f, 0.f, 0.f, "%d", d.id);
						}
					}
					endTextBatch();
				}
				gxPopMatrix();
				
				makeActive(&uiState, true, true);
				pushMenu("controls");
				{
					bool autoGainb = autoGain.load();
					int gaini = gain;
					
					bool autoWhiteBalanceb = autoWhiteBalance.load();
					int whiteBalancei = whiteBalance.load();
					
					int exposurei = exposure;
					int sharpnessi = sharpness;
					
					int huei = hue;
					
					int brightnessi = brightness;
					int contrasti = contrast;
					
					doCheckBox(autoGainb, "auto gain", false);
					doTextBox(gaini, "gain", framework.timeStep);
					doCheckBox(autoWhiteBalanceb, "auto white balance", false);
					doTextBox(whiteBalancei, "white balance", framework.timeStep);
					doTextBox(exposurei, "exposure", framework.timeStep);
					doTextBox(sharpnessi, "sharpness", framework.timeStep);
					doTextBox(huei, "hue", framework.timeStep);
					doTextBox(brightnessi, "brightness", framework.timeStep);
					doTextBox(contrasti, "contrast", framework.timeStep);
					
					autoGain = autoGainb;
					gain = gaini;
					autoWhiteBalance = autoWhiteBalanceb;
					whiteBalance = whiteBalancei;
					exposure = exposurei;
					sharpness = sharpnessi;
					hue = huei;
					brightness = brightnessi;
					contrast = contrasti;
				}
				popMenu();
				
				drawText(20, 400, 12, +1, +1, "frame: %d, fps: %d", frameIndex.load(), framesPerSecond);
				
				popFontMode();
			}
			framework.endDraw();
		}
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	stop = true;
	
	{
		Benchmark bm("thread join");
		thread.join();
	}
	
	delete [] frameData;
	frameData = nullptr;
	
	if (eye)
	{
		Benchmark bm("eye stop");
		eye->stop();
	}
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}
