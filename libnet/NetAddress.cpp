#include <string.h>
#include "NetAddress.h"

void NetAddress::Set(uint32_t address, uint16_t port)
{
	// Assign address / port number.
	m_address = address;
	m_port = port;

	// Setup sockaddr_in structure.
	memset(&m_socketAddress, 0, sizeof(m_socketAddress));
	m_socketAddress.sin_family = AF_INET;
	m_socketAddress.sin_addr.s_addr = htonl(m_address);
	m_socketAddress.sin_port = htons(m_port);
}

void NetAddress::Set(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
{
	uint32_t address = (a << 24) | (b << 16) | (c << 8) | (d << 0);

	Set(address, port);
}
