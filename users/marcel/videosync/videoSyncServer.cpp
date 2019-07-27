#include "Benchmark.h"
#include "jpegCompression.h"
#include "Log.h"
#include "videoSyncServer.h"
#include <SDL2/SDL.h>
#include <string.h>

bool TcpServer::init(const int tcpPort)
{
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
		LOG_DBG("server bind failed", 0);
		result = false;
	}
	else
	{
		if (listen(m_socket, 5) < 0)
		{
			LOG_DBG("server listen failed", 0);
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

void TcpServer::shut()
{
	if (m_socket != 0)
		close(m_socket);
}

bool TcpServer::accept(int & clientSocket)
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

int TcpServer::listenProc(void * obj)
{
	TcpServer * self = (TcpServer*)obj;
	
	int clientSocket = 0;
	
	if (self->accept(clientSocket))
	{
		LOG_DBG("server accept: clientSocket=%d", clientSocket);
		
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
				LOG_DBG("server: client socket disconnected", 0);
				break;
			}
			else
			{
				//LOG_DBG("server: received %d bytes", numBytes);
				
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
							LOG_DBG("header: sx=%d, sy=%d, compressedSize=%d",
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
								LOG_DBG("decompressed image: sx=%d, sy=%d", data->sx, data->sy);
								
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
