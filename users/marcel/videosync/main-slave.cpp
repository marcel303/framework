#include "framework.h"
#include "oscReceiver.h"
#include "videoloop.h"
#include "reflection.h"
#include "reflection-bindtofile.h"

#include "mediaplayer/MPVideoBuffer.h"

#define DEBUG_MASTER_TIME 0

struct Settings
{
	std::string videoFilename;
	int oscReceivePort = 2000;
	int windowWidth = 1920;
	int windowHeight = 1080;
	bool fullscreen = false;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<Settings>("Settings")
			.add("videoFilename", &Settings::videoFilename)
			.add("oscReceivePort", &Settings::oscReceivePort)
			.add("windowWidth", &Settings::windowWidth)
			.add("windowHeight", &Settings::windowHeight)
			.add("fullscreen", &Settings::fullscreen);
	}
};

struct MasterTimer : OscReceiveHandler
{
	bool isStarted = true;
	
	float time = 0.f;
	float extrapolatedReceivedTime = 0.f;
	float extrapolatedReceivedTime_exact = 0.f;
	
	void tick(const float dt)
	{
		if (isStarted)
		{
			extrapolatedReceivedTime += dt;
			extrapolatedReceivedTime_exact += dt;
		}
		
		const float delta1 = fabsf(extrapolatedReceivedTime - extrapolatedReceivedTime_exact);
		extrapolatedReceivedTime = lerp<float>(extrapolatedReceivedTime, extrapolatedReceivedTime_exact, .2f);\
		const float delta2 = fabsf(extrapolatedReceivedTime - extrapolatedReceivedTime_exact);
		//logDebug("extrapolated time diff: %dms -> %dms", int(delta1 * 1000.f), int(delta2 * 1000.f));
	}
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		if (strcmp(m.AddressPattern(), "/start") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsBool())
					isStarted = arg.AsBoolUnchecked();
				if (arg.IsInt32())
					isStarted = arg.AsInt32Unchecked();
				if (arg.IsFloat())
					isStarted = arg.AsFloatUnchecked();
			}
		}
		else if (strcmp(m.AddressPattern(), "/stop") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsBool())
					isStarted = 0 == arg.AsBoolUnchecked();
				if (arg.IsInt32())
					isStarted = 0 == arg.AsInt32Unchecked();
				if (arg.IsFloat())
					isStarted = 0 == arg.AsFloatUnchecked();
			}
		}
		else if (strcmp(m.AddressPattern(), "/time") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsInt32())
				{
					time = arg.AsInt32Unchecked();
				}
				else if (arg.IsFloat())
				{
					time = arg.AsFloatUnchecked();
				}
				else if (arg.IsDouble())
				{
					time = arg.AsDoubleUnchecked();
				}
			}
			
			if (fabsf(extrapolatedReceivedTime - time) > 1.f / 20.f)
				extrapolatedReceivedTime = time;
			extrapolatedReceivedTime_exact = time;
			
			const float delta = fabsf(extrapolatedReceivedTime - extrapolatedReceivedTime_exact);
			//logDebug("extrapolated time diff with exact time: %dms", int(delta * 1000.f));
		}
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	TypeDB typeDB;
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<std::string>("string", kDataType_String);
	Settings::reflect(typeDB);
	
	Settings settings;
	bindObjectToFile(&typeDB, &settings, "settings.json");
	
	//
	
	framework.fullscreen = settings.fullscreen;
	
	if (settings.fullscreen == false)
	{
		settings.windowWidth /= 2;
		settings.windowHeight /= 2;
	}
	
	framework.windowIsResizable = true;
	
	if (!framework.init(settings.windowWidth, settings.windowHeight))
		return -1;
	
	//
	
	OscReceiver oscReceiver;
	if (!oscReceiver.init("255.255.255.255", settings.oscReceivePort))
		logError("failed to initialzie OSC receiver");
	
	MasterTimer masterTimer;
	
#if DEBUG_MASTER_TIME
	float time = 0.f;
	float restartTimer = 0.f;
#endif
	
	float targetTime = 0.f;
	
	VideoLoop videoLoop(settings.videoFilename.c_str());
	
	double actualTime = 0.0;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
	#if DEBUG_MASTER_TIME
		time += dt;
		
		restartTimer += dt;
		
		if (restartTimer >= 3.f)
		{
			logDebug("faking seek to start");
			restartTimer = 0.f;
			time = 0.f;
		}
	#endif
		
		masterTimer.tick(dt);
		
		oscReceiver.flushMessages(&masterTimer);
		
	#if DEBUG_MASTER_TIME
		if ((rand() % 10) == 0)
		{
			receivedTime = time;
			
			masterTimer.extrapolatedReceivedTime = receivedTime;
		}
	#endif
		
		targetTime = masterTimer.extrapolatedReceivedTime;
		
	#if 1
		// forward/backward skip detection and compensation for running out of sync too much
		
		{
			const float kSkipTreshold = 4.f;
			
			if (videoLoop.mediaPlayer1->videoFrame != nullptr)
			{
				actualTime = videoLoop.mediaPlayer1->videoFrame->m_time;
			}
			
			if (actualTime - kSkipTreshold > targetTime)
			{
				logDebug("seek back detected. switching videos");
				
				videoLoop.switchVideos();
				
				if (targetTime > kSkipTreshold)
				{
					logDebug("seeking video");
					videoLoop.seekTo(targetTime, false);
					
					actualTime = videoLoop.mediaPlayer1->presentTime;
				}
			}
			else if (actualTime + kSkipTreshold < targetTime)
			{
				logDebug("seek forward detected. seeking video");
				
				videoLoop.seekTo(targetTime, false);
				
				actualTime = videoLoop.mediaPlayer1->presentTime;
			}
		}
	#endif
		
		videoLoop.tick(targetTime, framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			int videoSxPx;
			int videoSyPx;
			double duration;
			double sampleAspectRatio;
			if (videoLoop.mediaPlayer1->getVideoProperties(videoSxPx, videoSyPx, duration, sampleAspectRatio))
			{
				int sx, sy;
				framework.getCurrentViewportSize(sx, sy);
				
				const float videoSx = videoSxPx;
				const float videoSy = videoSyPx * sampleAspectRatio;
				
				const float scaleX = sx / videoSx;
				const float scaleY = sy / videoSy;
				
				const float scale = fminf(scaleX, scaleY);
				const float rectSx = videoSx * scale;
				const float rectSy = videoSy * scale;
				const float offsetX = (sx - rectSx) / 2.f;
				const float offsetY = (sy - rectSy) / 2.f;
				
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(videoLoop.getTexture(), GX_SAMPLE_LINEAR, true);
				drawRect(offsetX, offsetY, offsetX + rectSx, offsetY + rectSy);
				gxClearTexture();
				popBlend();
			}
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
