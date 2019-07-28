#pragma once

#include <arpa/inet.h>
#include <atomic>
#include <unistd.h>

struct SDL_mutex;
struct SDL_Thread;

struct JpegLoadData;

namespace Videosync
{
	struct Slave
	{
	private:
		int m_socket = -1;
		sockaddr_in m_socketAddress;
		
		SDL_mutex * m_mutex = nullptr;
		SDL_Thread * m_listenThread = nullptr;
		
		JpegLoadData * m_jpegData = nullptr;
		
	public:
		std::atomic<bool> wantsDisconnect;
		
		bool init(const int tcpPort);
		void shut();
		
		bool accept(int & clientSocket);
		
		JpegLoadData * consumeFrame();

		static int listenProc(void * obj);
	};
}
