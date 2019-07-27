#include "Benchmark.h"
#include "framework.h"
#include "mediaplayer/MPVideoBuffer.h"
#include "oscReceiver.h"
#include "oscSender.h"
#include "reflection.h"
#include "reflection-bindtofile.h"
#include "video.h"
#include "videoloop.h"

#if defined(LINUX)
	#include <turbojpeg.h>
#else
	#include <turbojpeg/turbojpeg.h>
#endif

const int VIEW_SX = 1200;
const int VIEW_SY = 600;

struct SlaveInfo
{
	std::string oscSendAddress = "127.0.0.1";
	int oscSendPort = 2002;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<SlaveInfo>("SlaveInfo")
			.add("oscSendAddress", &SlaveInfo::oscSendAddress)
			.add("oscReceivePort", &SlaveInfo::oscSendPort);
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
		
		if (tjCompress2(h, (unsigned char *)srcBuffer, srcSx, xPitch, srcSy, TJPF_RGBX, (unsigned char**)&dstBuffer, &dstBufferSize2, subsamp, quality, 0) < 0)
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

#include <arpa/inet.h>
#include <unistd.h>

struct TcpServer
{
	int m_socket = 0;
	sockaddr_in m_socketAddress;
	
	SDL_mutex * m_mutex = nullptr;
	SDL_Thread * m_listenThread = nullptr;
	
	JpegLoadData * m_jpegData = nullptr;
	
	bool init(const int tcpPort)
	{
	// inet_aton
	
		bool result = true;
	
		memset(&m_socketAddress, 0, sizeof(m_socketAddress));
		m_socketAddress.sin_family = AF_INET;
		m_socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		m_socketAddress.sin_port = htons(tcpPort);
		
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		
	// todo : disable nagle
	// todo : disable linger
		
		if ((bind(m_socket, (struct sockaddr *)&m_socketAddress, sizeof(m_socketAddress))) < 0)
		{
			logDebug("server bind failed");
			result = false;
		}
		else
		{
			if (listen(m_socket, 5) < 0)
			{
				logDebug("server listen failed");
				result = false;
			}
			else
			{
				m_mutex = SDL_CreateMutex();
				
				m_listenThread = SDL_CreateThread(listenProc, "TCP Listen", this);
			}
		}
		
		if (result == false)
		{
			shut();
		}
		
		return result;
	}
	
	void shut()
	{
		if (m_socket != 0)
			close(m_socket);
	}
	
	bool accept(int & clientSocket)
	{
		clientSocket = 0;
		
		sockaddr_in clientAddress;
		socklen_t clientAddressSize = sizeof(clientAddress);

		clientSocket = ::accept(m_socket, (struct sockaddr*)&clientAddress, &clientAddressSize);
		
		if (clientSocket == 0)
			return false;
		
		return true;
	}
	
	static void setByte(void * dst, const int index, const uint8_t value)
	{
		((uint8_t*)dst)[index] = value;
	}
	
	static int listenProc(void * obj)
	{
		TcpServer * self = (TcpServer*)obj;
		
		int clientSocket = 0;
		
		if (self->accept(clientSocket))
		{
			logDebug("server accept: clientSocket=%d", clientSocket);
			
			struct ReceiveState
			{
				enum State
				{
					kState_ReceiveHeader,
					kState_ReceiveCompressedImage
				};
				
				State state = kState_ReceiveHeader;
				int stateBytes = 0;
			};
			
			ReceiveState receiveState;
			
			int header[3];
			
			int sx = 0;
			int sy = 0;
			int compressedSize = 0;
			
			uint8_t * compressed = (uint8_t*)malloc(1024 * 1024);
			
			for (;;)
			{
				uint8_t bytes[1024];
				const int numBytes = recv(clientSocket, bytes, 1024, 0);
				
				if (numBytes < 0)
				{
					logDebug("server: client socket disconnected");
					break;
				}
				else
				{
					//logDebug("server: received %d bytes", numBytes);
					
					for (int i = 0; i < numBytes; ++i)
					{
						if (receiveState.state == ReceiveState::kState_ReceiveHeader)
						{
							setByte(header, receiveState.stateBytes++, bytes[i]);
							
							if (receiveState.stateBytes == sizeof(int) * 3)
							{
								sx = header[0];
								sy = header[1];
								compressedSize = header[2];
								
							/*
								logDebug("header: sx=%d, sy=%d, compressedSize=%d",
									sx,
									sy,
									compressedSize);
							*/
							
								receiveState.state = ReceiveState::kState_ReceiveCompressedImage;
								receiveState.stateBytes = 0;
							}
						}
						else if (receiveState.state == ReceiveState::kState_ReceiveCompressedImage)
						{
							compressed[receiveState.stateBytes++] = bytes[i];
							
							if (receiveState.stateBytes == compressedSize)
							{
								Benchmark bm("decompress");
								
								JpegLoadData * data = new JpegLoadData();
								
								if (loadImage_turbojpeg(compressed, compressedSize, *data))
								{
									logDebug("decompressed image: sx=%d, sy=%d", data->sx, data->sy);
									
									JpegLoadData * oldData = nullptr;
									
									SDL_LockMutex(self->m_mutex);
									{
										oldData = self->m_jpegData;
										self->m_jpegData = data;
									}
									SDL_UnlockMutex(self->m_mutex);
									
									delete oldData;
									oldData = nullptr;
								}
								
								sx = 0;
								sy = 0;
								compressedSize = 0;
								
								receiveState.state = ReceiveState::kState_ReceiveHeader;
								receiveState.stateBytes = 0;
							}
						}
					}
				}
			}
			
			free(compressed);
			compressed = nullptr;
		}
		
		return 0;
	}
};

struct TcpClient
{
	int m_clientSocket = 0;
	
	sockaddr_in m_serverSocketAddress;
	
	bool connect(const char * ipAddress, const int tcpPort)
	{
		bool result = true;
		
		m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		
		memset(&m_serverSocketAddress, 0, sizeof(m_serverSocketAddress));
		m_serverSocketAddress.sin_family = AF_INET;
		m_serverSocketAddress.sin_addr.s_addr = inet_addr(ipAddress);
		m_serverSocketAddress.sin_port = htons(tcpPort);
		
		socklen_t serverAddressSize = sizeof(m_serverSocketAddress);
		
		if (::connect(m_clientSocket, (struct sockaddr *)&m_serverSocketAddress, serverAddressSize) < 0)
		{
			logDebug("client: connect failed");
			result = false;
		}
		
		if (result == false)
		{
			shut();
		}
		
		return result;
	}
	
	void shut()
	{
		if (m_clientSocket != 0)
		{
			close(m_clientSocket);
			m_clientSocket = 0;
		}
	}
};

int main(int argc, char * argv[])
{
	TcpServer server;
	server.init(1800);
	SDL_Delay(500);
	
	TcpClient client;
	client.connect("127.0.0.1", 1800);
	
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
	
	std::vector<OscReceiver> oscSenders;
	oscSenders.resize(settings.slaves.size());
	for (size_t i = 0; i < settings.slaves.size(); ++i)
		if (!oscSenders[i].init(settings.slaves[i].oscSendAddress.c_str(), settings.slaves[i].oscSendPort))
			logError("failed to initialzie OSC sender");
	
	MyOscReceiveHandler oscReceiveHandler;
	
	VideoLoop videoLoop("lasers.mp4");
	
	GxTexture texture;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		oscReceiver.flushMessages(&oscReceiveHandler);
		
		if (videoLoop.mediaPlayer1->presentedLastFrame(videoLoop.mediaPlayer1->context))
			videoLoop.switchVideos();
		
		videoLoop.tick(-1.f, framework.timeStep);
		
		if (videoLoop.mediaPlayer1->videoFrame != nullptr)
		{
			Benchmark bm("compress");
			
			auto * videoFrame = videoLoop.mediaPlayer1->videoFrame;
			
			const int sx = videoFrame->m_width;
			const int sy = videoFrame->m_height;
			uint8_t * data = videoFrame->m_frameBuffer;
			
			void * compressed = nullptr;
			int compressedSize = 0;
			
			if (saveImage_turbojpeg(data, sx * sy * 4, sx, sy, compressed, compressedSize))
			{
				if (client.m_clientSocket != 0)
				{
					const int header[3] =
					{
						sx,
						sy,
						compressedSize
					};
					
					send(client.m_clientSocket, header, 3 * sizeof(int), 0);
					send(client.m_clientSocket, compressed, compressedSize, 0);
				}
				
				free(compressed);
				compressed = nullptr;
				
				//logDebug("compressed size: %dKb (down from %dKb)", compressedSize / 1024, sx * sy * 4 / 1024);
			}
		}
		
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
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(videoLoop.getTexture());
			drawRect(0, 0, VIEW_SX/2, VIEW_SY);
			gxSetTexture(0);
			popBlend();
			
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(texture.id);
			drawRect(VIEW_SX/2, 0, VIEW_SX, VIEW_SY);
			gxSetTexture(0);
			popBlend();
		}
		framework.endDraw();
	}
	
	client.shut();
	server.shut();

	framework.shutdown();

	return 0;
}
