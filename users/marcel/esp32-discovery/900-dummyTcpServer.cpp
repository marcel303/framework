#include "test_audioStreamToTcp.h"

#include "test_dummyTcpServer.h"
#include "threadedTcpClient.h"

#include "framework.h"
#include "Log.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.init(800, 600);
	
	Test_DummyTcpServer tcpServer;
	
	IpEndpointName endpointName(IpEndpointName::ANY_ADDRESS, 4000);
	tcpServer.init(endpointName);
	
#if 1
	Test_AudioStreamToTcp audioStreamToTcp;
	
	audioStreamToTcp.init(
		IpEndpointName("127.0.0.1", -1).address,
		4000,
		2,
		128,
		1,
		true,
		"loop01-short.ogg");
	
	SDL_Delay(3000);
	
	logInfo("begin dummy TCP server shutdown");
	
	tcpServer.beginShutdown();
	
	logInfo("shutting down audio streamer");
	
	audioStreamToTcp.shut();
	
	logInfo("waiting for dummy TCP server shutdown to complete");
	
	tcpServer.waitForShutdown();
#else
	ThreadedTcpConnection tcpConnection;
	
	ThreadedTcpConnection::Options options;
	options.noDelay = true;
	tcpConnection.init(IpEndpointName("127.0.0.1", -1).address, 4000, options, [&]()
		{
			for (int i = 0; i < 26; ++i)
			{
				char c = 'a' + i;
			
				if (send(tcpConnection.sock, &c, 1, 0) < 1)
				{
					logError("failed to send data");
					tcpConnection.wantsToStop = true;
				}
			}
		});
	
	sleep(1);
	
	logInfo("shutting down dummy TCP server");
	
	tcpServer.beginShutdown();
	
	logInfo("shutting down client connection");
	
	tcpConnection.beginShutdown();
	
	logInfo("waiting for connection shutdown to complete");
	
	tcpConnection.waitForShutdown();
	
	logInfo("waiting for dummy TCP server shutdown to complete");
	
	tcpServer.waitForShutdown();
	
	logInfo("dummy TCP server test completed");
#endif

	framework.shutdown();

	return 0;
}
