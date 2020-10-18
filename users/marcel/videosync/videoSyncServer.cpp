#include "Benchmark.h"
#include "jpegCompression.h"
#include "Log.h"
#include "videoSyncServer.h"
#include <SDL2/SDL.h>
#include <string.h>

#if defined(WINDOWS)
	#include <WS2tcpip.h>
#else
	#include <netinet/tcp.h>
	#include <unistd.h>
#endif

#if defined(WINDOWS)
	#define I_HATE_WINDOWS (char*)
#else
	#define I_HATE_WINDOWS
	#define closesocket close
#endif

namespace Videosync
{
	bool Slave::init(const int tcpPort)
	{
		bool result = true;

		memset(&m_socketAddress, 0, sizeof(m_socketAddress));
		m_socketAddress.sin_family = AF_INET;
		m_socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		m_socketAddress.sin_port = htons(tcpPort);
		
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		
		int set_false = 0;
		int set_true = 1;
		
		// disable SIGPIPE (and handle broken connections ourselves)
		signal(SIGPIPE, SIG_IGN);
		
	#if defined(WINDOWS)
		// disable nagle & linger
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, I_HATE_WINDOWS &set_false, sizeof(set_false));
		setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, I_HATE_WINDOWS &set_true, sizeof(set_true));
	#else
		// disable nagle & linger
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, &set_false, sizeof(set_false));
		setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &set_true, sizeof(set_true));
		// enable address/port reuse
		setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &set_true, sizeof(set_true));
	#endif
		
		if ((bind(m_socket, (struct sockaddr *)&m_socketAddress, sizeof(m_socketAddress))) < 0)
		{
			LOG_ERR("server bind failed");
			result = false;
		}
		else
		{
			if (listen(m_socket, 1) < 0)
			{
				LOG_ERR("server listen failed");
				result = false;
			}
			else
			{
				wantsDisconnect = false;
				
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

	void Slave::shut()
	{
		if (m_socket >= 0)
		{
			closesocket(m_socket);
			m_socket = -1;
		}
	}

	bool Slave::accept(int & clientSocket)
	{
		clientSocket = -1;
		
		sockaddr_in clientAddress;
		memset(&clientAddress, 0, sizeof(clientAddress));
		socklen_t clientAddressSize = sizeof(clientAddress);

		clientSocket = ::accept(m_socket, (struct sockaddr*)&clientAddress, &clientAddressSize);
		
		if (clientSocket < 0)
			return false;
		
		int set_false = 0;
		int set_true = 1;
		
	#if defined(WINDOWS)
		setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, I_HATE_WINDOWS &set_false, sizeof(set_false));
		setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, I_HATE_WINDOWS &set_true, sizeof(set_true));
	#else
	#if defined(MACOS)
		setsockopt(clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set_true, sizeof(set_true));
	#endif
		setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, (void*)&set_false, sizeof(set_false));
		setsockopt(clientSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&set_true, sizeof(set_true));
		setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &set_true, sizeof(set_true));
	#endif
		
		return true;
	}

	JpegLoadData * Slave::consumeFrame()
	{
		JpegLoadData * data;
		
		SDL_LockMutex(m_mutex);
		{
			data = m_jpegData;
			m_jpegData = nullptr;
		}
		SDL_UnlockMutex(m_mutex);
		
		return data;
	}
	
	static void setByte(void * dst, const int index, const uint8_t value)
	{
		((uint8_t*)dst)[index] = value;
	}

	int Slave::listenProc(void * obj)
	{
		Slave * self = (Slave*)obj;
		
		for (;;)
		{
			int clientSocket = 0;
			
			if (self->accept(clientSocket) == false)
			{
				LOG_ERR("server accept: failed to accept connection");
			}
			else
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
				
				while (self->wantsDisconnect == false)
				{
					//LOG_DBG("server: received %d bytes", numBytes);
					
					if (receiveState.state == ReceiveState::kState_ReceiveHeader)
					{
						const int remaining = sizeof(header) - receiveState.stateBytes;
						
						uint8_t bytes[sizeof(header)];
						const int numBytes = recv(clientSocket, I_HATE_WINDOWS bytes, remaining, 0);
						
						if (numBytes <= 0)
						{
							LOG_DBG("server: client socket disconnected");
							break;
						}
						
						for (int i = 0; i < numBytes; ++i)
						{
							setByte(header, receiveState.stateBytes++, bytes[i]);
						}
						
						if (receiveState.stateBytes == sizeof(header))
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
						const int remaining = compressedSize - receiveState.stateBytes;
						
						uint8_t * bytes = compressed + receiveState.stateBytes;
						const int numBytes = recv(clientSocket, I_HATE_WINDOWS bytes, remaining, 0);
						
						if (numBytes <= 0)
						{
							LOG_DBG("server: client socket disconnected");
							break;
						}
						
						receiveState.stateBytes += numBytes;
						
						if (receiveState.stateBytes == compressedSize)
						{
							//Benchmark bm("decompress");
							
							JpegLoadData * data = new JpegLoadData();
							
							if (loadImage_turbojpeg(compressed, compressedSize, *data))
							{
								//LOG_DBG("decompressed image: sx=%d, sy=%d", data->sx, data->sy);
								
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
				
				self->wantsDisconnect = false;
				
				free(compressed);
				compressed = nullptr;

				if (clientSocket >= 0)
				{
				#if 0
					LOG_DBG("slave: disconnecting..");
					if (shutdown(clientSocket, 1) >= 0)
					{
					#if 1
						uint8_t temp;
						while (read(clientSocket, &temp, 1) == 1)
						{
							// not done yet
						}
					#endif
						LOG_DBG("slave: disconnecting.. done");
					}
					else
					{
						LOG_DBG("slave: disconnecting.. failure");
					}
				#endif
					
					closesocket(clientSocket);
					clientSocket = -1;
				}
			}
		}
		
		return 0;
	}
}
