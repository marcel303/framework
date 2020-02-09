#pragma once

#include <stdint.h>
#include <string>

#if defined(WINDOWS)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/types.h>
#endif

class NetAddress
{
public:
	inline NetAddress()
	{
		Set(0, 0);
	}

	inline NetAddress(uint32_t address, uint16_t port)
	{
		Set(address, port);
	}

	inline NetAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
	{
		Set(a, b, c, d, port);
	}

	inline NetAddress(const char * address, uint16_t port)
	{
		SetFromString(address, port);
	}

	inline const sockaddr_in * GetSockAddr() const
	{
		return &m_socketAddress;
	}

	void Set(uint32_t address, uint16_t port);
	void Set(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);
	void SetFromString(const char * address, uint16_t port);

	inline bool IsValid() const
	{
		return m_port != 0;
	}

	std::string ToString(bool includePortNumber) const;

	uint64_t m_userData;

private:
	uint32_t m_address;
	uint16_t m_port;

	sockaddr_in m_socketAddress;
};
