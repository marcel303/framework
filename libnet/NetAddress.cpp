#include <string.h>
#include "NetAddress.h"

static int GetOctet(int address, int i)
{
	return (address >> (24 - i * 8)) & 0xff;
}

//

void NetAddress::Set(uint32_t address, uint16_t port)
{
	// Assign address / port number.
	m_address = address;
	m_port = port;

	// Setup sockaddr_in structure.
	memset(&m_socketAddress, 0, sizeof(m_socketAddress));
	m_socketAddress.sin_family = AF_INET;
	m_socketAddress.sin_addr.s_addr = htonl(address != 0 ? m_address : INADDR_BROADCAST);
	m_socketAddress.sin_port = htons(m_port);
}

void NetAddress::Set(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
{
	uint32_t address = (a << 24) | (b << 16) | (c << 8) | (d << 0);

	Set(address, port);
}

void NetAddress::SetFromString(const char * address, uint16_t port)
{
	const uint32_t abcd = ntohl(inet_addr(address));
	const uint8_t a = (abcd >> 24) & 0xff;
	const uint8_t b = (abcd >> 16) & 0xff;
	const uint8_t c = (abcd >>  8) & 0xff;
	const uint8_t d = (abcd >>  0) & 0xff;

	Set(a, b, c, d, port);
}

std::string NetAddress::ToString(bool includePortNumber) const
{
	char temp[64];
	if (includePortNumber)
	{
		sprintf(temp, "%d.%d.%d.%d:%d",
			GetOctet(m_address, 0),
			GetOctet(m_address, 1),
			GetOctet(m_address, 2),
			GetOctet(m_address, 3),
			m_port);
	}
	else
	{
		sprintf(temp, "%d.%d.%d.%d",
			GetOctet(m_address, 0),
			GetOctet(m_address, 1),
			GetOctet(m_address, 2),
			GetOctet(m_address, 3));
	}
	return temp;
}
