#include "Benchmark.h"
#include "framework.h"
#include "mediaplayer/MPVideoBuffer.h"
#include "oscReceiver.h"
#include "oscSender.h"
#include "reflection.h"
#include "reflection-bindtofile.h"
#include "video.h"
#include "videoloop.h"

#include "jpegCompression.h"
#include "videoSyncClient.h"
#include "videoSyncServer.h"

const int VIEW_SX = 1200;
const int VIEW_SY = 600;

const float kReconnectTime = 2.f;

struct SlaveInfo
{
	std::string ipAddress = "127.0.0.1";
	int tcpPort = 1800;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<SlaveInfo>("SlaveInfo")
			.add("ipAddress", &SlaveInfo::ipAddress)
			.add("tcpPort", &SlaveInfo::tcpPort);
	}
};

struct Settings
{
	int oscReceivePort = 2000;
	
	std::vector<SlaveInfo> slaves;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<Settings>("Settings")
			.add("oscReceivePort", &Settings::oscReceivePort)
			.add("slaves", &Settings::slaves);
	}
};

struct SlaveState
{
	Videosync::Slave slave;
	
	GxTexture texture;
};

struct MasterState
{
	Videosync::Master master;
};

struct MyOscReceiveHandler : OscReceiveHandler
{
	bool isStarted = false;
	float time = 0.f;
	
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
		}
	}
};

int main(int argc, char * argv[])
{
	// create a fake slave to connect to
	// todo : move this to the slave app
	std::list<SlaveState> slaveStates;
	slaveStates.emplace_back();
	slaveStates.back().slave.init(1800);
	slaveStates.emplace_back();
	slaveStates.back().slave.init(1802);
	SDL_Delay(500);
	
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	TypeDB typeDB;
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<std::string>("string", kDataType_String);
	SlaveInfo::reflect(typeDB);
	Settings::reflect(typeDB);
	
	Settings settings;
	bindObjectToFile(&typeDB, &settings, "master-settings.json");
	
	//
	
	framework.windowIsResizable = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	//
	
	OscReceiver oscReceiver;
	if (!oscReceiver.init("255.255.255.255", settings.oscReceivePort))
		logError("failed to initialzie OSC receiver");
	
	std::list<MasterState> masterStates;
	for (auto & slave : settings.slaves)
	{
		masterStates.emplace_back();
		
		auto & masterState = masterStates.back();
		
		masterState.master.connect(slave.ipAddress.c_str(), slave.tcpPort);
	}
	
	MyOscReceiveHandler oscReceiveHandler;
	
	VideoLoop videoLoop("lasers2.mp4");
	
	double lastFrameTime = 0.0;
	
	bool isPaused = false;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		// process input
		
		oscReceiver.flushMessages(&oscReceiveHandler);
		
		if (keyboard.wentDown(SDLK_d))
		{
			for (auto & slaveState : slaveStates)
				slaveState.slave.wantsDisconnect = true;
		}
		
		if (keyboard.wentDown(SDLK_c))
		{
			if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			{
				for (auto & masterState : masterStates)
					masterState.master.reconnect();
			}
			else
			{
				for (auto & masterState : masterStates)
					masterState.master.disconnect();
			}
		}
		
		if (keyboard.wentDown(SDLK_SPACE))
		{
			isPaused = !isPaused;
		}
		
		// update video
		
		if (videoLoop.mediaPlayer1->presentedLastFrame(videoLoop.mediaPlayer1->context))
			videoLoop.switchVideos();
		
		if (isPaused == false)
		{
			videoLoop.tick(-1.f, framework.timeStep);
		}
		
		// send video frame over tcp to slave(s)
		
		if (videoLoop.mediaPlayer1->videoFrame != nullptr && videoLoop.mediaPlayer1->videoFrame->m_time != lastFrameTime)
		{
			lastFrameTime = videoLoop.mediaPlayer1->videoFrame->m_time;
			
			//Benchmark bm("compress");
			
			auto * videoFrame = videoLoop.mediaPlayer1->videoFrame;
			
			const int sx = videoFrame->m_width;
			const int sy = videoFrame->m_height;
			uint8_t * data = videoFrame->m_frameBuffer;
			
			void * compressed = nullptr;
			int compressedSize = 0;
			
			if (saveImage_turbojpeg(data, sx * sy * 4, sx, sy, compressed, compressedSize))
			{
				for (auto & masterState : masterStates)
				{
					auto & master = masterState.master;
					
					if (master.isConnected())
					{
						const int header[3] =
						{
							sx,
							sy,
							compressedSize
						};
						
					#if defined(MACOS) || defined(WINDOWS)
						const float flags = 0;
					#else
						const float flags = MSG_NOSIGNAL;
					#endif
					
						if (send(master.m_clientSocket, (const char*)header, 3 * sizeof(int), flags) < 0 ||
							send(master.m_clientSocket, (const char*)compressed, compressedSize, flags) < 0)
						{
							LOG_ERR("master: failed to send data to slave", 0);
							
							master.disconnect();
							
							master.reconnectTimer = kReconnectTime;
						}
					}
					else
					{
						if (master.reconnectTimer > 0.f)
						{
							master.reconnectTimer = fmaxf(0.f, master.reconnectTimer - framework.timeStep);
							
							if (master.reconnectTimer == 0.f)
							{
								if (master.reconnect() == false)
								{
									master.reconnectTimer = kReconnectTime;
								}
							}
						}
					}
				}
				
				free(compressed);
				compressed = nullptr;
				
				//logDebug("compressed size: %dKb (down from %dKb)", compressedSize / 1024, sx * sy * 4 / 1024);
			}
		}
		
		for (auto & slaveState : slaveStates)
		{
			auto & slave = slaveState.slave;
			auto & texture = slaveState.texture;
			
			// update received image texture
			
		// todo : use a condition variable to wait for frames or disconnects
			JpegLoadData * data = slave.consumeFrame();
			
			if (data != nullptr)
			{
				if (texture.isChanged(data->sx, data->sy, GX_RGBA8_UNORM))
				{
					texture.allocate(data->sx, data->sy, GX_RGBA8_UNORM, true, true);
				}
				
				texture.upload(data->buffer, 1, 0);
				
				delete data;
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			int sx, sy;
			framework.getCurrentViewportSize(sx, sy);
			
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(videoLoop.getTexture());
			drawRect(0, 0, sx/2, sy);
			gxSetTexture(0);
			popBlend();
			
			// draw the frames received on the slave side
			
			const int numSlaves = slaveStates.size();
			
			if (numSlaves > 0)
			{
				int index = 0;
				
				for (auto & slaveState : slaveStates)
				{
					auto & texture = slaveState.texture;
					
					pushBlend(BLEND_OPAQUE);
					gxSetTexture(texture.id);
					drawRect(
						sx/2,
						(index + 0) * sy / numSlaves,
						sx,
						(index + 1) * sy / numSlaves);
					gxSetTexture(0);
					popBlend();
					
					index++;
				}
			}
			
			setFont("unispace.ttf");
			setColor(colorWhite);
			drawText(10, 10, 12, +1, +1, "Press 'd' to disconnect slave(s)");
			drawText(10, 30, 12, +1, +1, "Press 'c' to disconnect masters(s)");
			drawText(10, 50, 12, +1, +1, "Press 'c' + SHIFT to reconnect masters(s)");
			drawText(10, 70, 12, +1, +1, "Press SPACE to pause/resume");
			
			int y = 200;
			
			for (auto & masterState : masterStates)
			{
				drawText(10, y, 10, +1, -1, "master: isConnected=%s, reconnectTimer=%.2f", masterState.master.isConnected() ? "yes" : "no", masterState.master.reconnectTimer);
				y += 18;
			}
		}
		framework.endDraw();
	}
	
	for (auto & masterState : masterStates)
		masterState.master.disconnect();
	masterStates.clear();
	
	for (auto & slaveState : slaveStates)
		slaveState.slave.shut();
	slaveStates.clear();

	framework.shutdown();

	return 0;
}
