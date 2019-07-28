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

struct ServerState
{
	TcpServer tcpServer;
	
	GxTexture texture;
};

struct ClientState
{
	TcpClient tcpClient;
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
	// create a fake server to connect to
	// todo : move this to the slave app
	std::list<ServerState> serverStates;
	serverStates.emplace_back();
	serverStates.back().tcpServer.init(1800);
	serverStates.emplace_back();
	serverStates.back().tcpServer.init(1802);
	SDL_Delay(500);
	
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif
	
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
	
	std::list<ClientState> clientStates;
	for (auto & slave : settings.slaves)
	{
		clientStates.emplace_back();
		
		auto & clientState = clientStates.back();
		
		clientState.tcpClient.connect(slave.ipAddress.c_str(), slave.tcpPort);
	}
	
	MyOscReceiveHandler oscReceiveHandler;
	
	VideoLoop videoLoop("lasers2.mp4");
	
	double lastFrameTime = 0.0;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		// process input
		
		oscReceiver.flushMessages(&oscReceiveHandler);
		
		if (keyboard.wentDown(SDLK_d))
		{
			for (auto & serverState : serverStates)
				serverState.tcpServer.wantsDisconnect = true;
		}
		
		// update video
		
		if (videoLoop.mediaPlayer1->presentedLastFrame(videoLoop.mediaPlayer1->context))
			videoLoop.switchVideos();
		
		videoLoop.tick(-1.f, framework.timeStep);
		
		// send video frame over tcp to client(s)
		
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
				for (auto & clientState : clientStates)
				{
					auto & client = clientState.tcpClient;
					
					if (client.isConnected())
					{
						const int header[3] =
						{
							sx,
							sy,
							compressedSize
						};
						
						if (send(client.m_clientSocket, header, 3 * sizeof(int), 0) < 0 ||
							send(client.m_clientSocket, compressed, compressedSize, 0) < 0)
						{
							LOG_ERR("server: failed to send data to client", 0);
							
							client.disconnect();
							
							client.reconnectTimer = kReconnectTime;
						}
					}
					else
					{
						if (client.reconnectTimer > 0.f)
						{
							client.reconnectTimer = fmaxf(0.f, client.reconnectTimer - framework.timeStep);
							
							if (client.reconnectTimer == 0.f)
							{
								if (client.reconnect() == false)
								{
									client.reconnectTimer = kReconnectTime;
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
		
		for (auto & serverState : serverStates)
		{
			auto & server = serverState.tcpServer;
			auto & texture = serverState.texture;
			
			// update received image texture
			
			JpegLoadData * data = nullptr;
			
		// todo : use a condition variable to wait for frames or disconnects
		
			SDL_LockMutex(server.m_mutex);
			{
				data = server.m_jpegData;
				server.m_jpegData = nullptr;
			}
			SDL_UnlockMutex(server.m_mutex);
			
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
			
			// draw the frames received on the server side
			
			const int numServers = serverStates.size();
			
			if (numServers > 0)
			{
				int index = 0;
				
				for (auto & serverState : serverStates)
				{
					auto & texture = serverState.texture;
					
					pushBlend(BLEND_OPAQUE);
					gxSetTexture(texture.id);
					drawRect(
						sx/2,
						(index + 0) * sy / numServers,
						sx,
						(index + 1) * sy / numServers);
					gxSetTexture(0);
					popBlend();
					
					index++;
				}
			}
			
			setFont("unispace.ttf");
			setColor(colorWhite);
			drawText(10, 10, 12, +1, +1, "Press 'd' to disconnect client(s)");
		}
		framework.endDraw();
	}
	
	for (auto & clientState : clientStates)
		clientState.tcpClient.disconnect();
	clientStates.clear();
	
	for (auto & serverState : serverStates)
		serverState.tcpServer.shut();
	serverStates.clear();

	framework.shutdown();

	return 0;
}
