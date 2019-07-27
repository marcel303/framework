#pragma once

#include <arpa/inet.h>
#include <unistd.h>

struct SDL_mutex;
struct SDL_Thread;

struct JpegLoadData;

struct TcpServer
{
// todo : make members private

	int m_socket = 0;
	sockaddr_in m_socketAddress;
	
	SDL_mutex * m_mutex = nullptr;
	SDL_Thread * m_listenThread = nullptr;
	
	JpegLoadData * m_jpegData = nullptr;
	
	bool init(const int tcpPort);
	void shut();
	
	bool accept(int & clientSocket);

	static int listenProc(void * obj);
};
