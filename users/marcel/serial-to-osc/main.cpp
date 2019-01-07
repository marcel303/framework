#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#include "Log.h"

#define OSC_BUFFER_SIZE 1024
#define OSC_PORT 6448

struct TTY
{
	int port;
	
	TTY()
		: port(-1)
	{
	}
	
	~TTY()
	{
		shut();
	}
	
	bool init()
	{
		const char * ttyPath = "/dev/cu.usbmodem14111";
		
		port = open(ttyPath, O_RDWR);
		
		if (port < 0)
		{
			shut();
			
			return false;
		}
		else
		{
		#if 1
			termios settings;
			
			if (tcgetattr(port, &settings) != 0)
			{
				LOG_ERR("failed to get terminal io settings", 0);
			}
			else
			{
				const int rate = 19200;
				
				if (cfsetispeed(&settings, rate) != 0)
					LOG_ERR("failed to set tty ispeed to %d", rate);
				if (cfsetospeed(&settings, rate) != 0)
					LOG_ERR("failed to set tty ospeed to %d", rate);
				
				//settings.c_cflag = (settings.c_cflag & (~CSIZE)) | CS8;
				//CDTR_IFLOW; // enable
				//CRTS_IFLOW; // disable
				
				if (tcsetattr(port, TCSANOW, &settings) != 0)
					LOG_ERR("failed to apply terminal io settings", 0);
				else
					LOG_INF("succesfully applied terminal io settings", 0);
				
				tcflush(port, TCIOFLUSH);
			}
		#endif
			
			return true;
		}
	}
	
	void shut()
	{
		if (port >= 0)
		{
			close(port);
		}
		
		port = -1;
	}
};

int main(int argc, char * argv[])
{
	TTY tty;
	
	if (!tty.init())
		return -1;

	UdpTransmitSocket * transmitSocket = nullptr;

	const char * ipAddress = "127.0.0.1";
	const int udpPort = { OSC_PORT };
	
	LOG_DBG("setting up UDP transmit sockets for OSC messaging", 0);

	try
	{
		transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to create UDP transmit socket: %s", e.what());
	}

	for (;;)
	{
		char line[64];
		
		int lineSize = 0;
		while (lineSize < sizeof(line) - 1)
		{
			read(tty.port, &line[lineSize], 1);
			if (line[lineSize] == '\n')
				break;
			lineSize++;
		}
		
		line[lineSize] = 0;

		float value = 0.f;
		
		if (sscanf(line, "%f", &value) != 1)
			continue;
		
		printf("value: %.2f\n", value);
		
		char buffer[OSC_BUFFER_SIZE];
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);
		
		//p << osc::BeginBundleImmediate;
		p << osc::BeginMessage("/wek/test");
		{
			p << value;
		}
		p << osc::EndMessage;
		//p << osc::EndBundle;

		transmitSocket->Send(p.Data(), p.Size());
	}

	tty.shut();
	
	return 0;
}
