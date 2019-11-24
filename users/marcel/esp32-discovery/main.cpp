#include "artnet.h"
#include "framework.h"
#include "imgui-framework.h"
#include "ip/UdpSocket.h"
#include "StringEx.h"
#include <atomic>
#include <vector>

/*

esp32 discovery process relies on the Arduino sketch 'esp32-wifi-configure'.
This is a sketch which lets the user select a Wifi access point and connect to it. The sketch will then proceed sending discovery messages at a regular interval. The discovery message containts the id of the device, and the IP address can be inferred from the received UDP packet.

*/

#include "nodeDiscovery.h"

//

#include "audiostream/AudioIO.h"
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct Test_TcpToI2S
{
	bool isActive = false;
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	Test_TcpToI2S()
		: wantsToStop(false)
	{
	}
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort)
	{
		Assert(isActive == false);
		
		if (isActive)
		{
			shut();
		}
		
		isActive = true;
		
		thread = std::thread([=]()
		{
			struct sockaddr_in addr;
			int sock = 0;
			int sock_value = 0;
			SoundData * soundData = nullptr;
			int samplePosition = 0;
			
			//
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			
			if (sock == -1)
			{
				logError("failed opening socket");
				goto error;
			}
			
			// disable nagle's algorithm and send out packets immediately on write
			
		#if 1
			sock_value = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_value, sizeof(sock_value));
		#endif
		
		#if 1
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			sock_value =
				I2S_2CH_BUFFER_COUNT  * /* N times buffered */
				I2S_2CH_FRAME_COUNT   * /* frame count */
				I2S_2CH_CHANNEL_COUNT * /* stereo */
				sizeof(int16_t) /* sample size */;
 			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sock_value, sizeof(sock_value));
		#endif
		
			// todo : TCP_NODELAY (osx). disables nagle's algorithm and sends out packets immediately on write
			// todo : TCP_NOPUSH (osx)
			// todo : TCP_CORK (linux). will manually batch messages and send them when the cork is removed

		// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(ipAddress);
			addr.sin_port = htons(tcpPort);

			logDebug("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				logError("failed to connect socket");
				goto error;
			}
			
			logError("connected to remote endpoint!");
			
			soundData = loadSound("loop01.ogg");
			
			while (wantsToStop.load() == false)
			{
				// todo : detect is disconnected, and attempt to reconnect
				// todo : avoid high CPU on disconnect
				// todo : perform disconnection test
				
				while (keyboard.isDown(SDLK_SPACE))
				{
					SDL_Delay(10);
				}
				
				// todo : generate some audio data
				
				int16_t data[I2S_2CH_FRAME_COUNT][2];
				
				// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
				if (soundData->sampleCount == 0 ||
					soundData->channelCount != 2 ||
					soundData->channelSize != 2)
				{
					memset(data, 0, sizeof(data));
				}
				else
				{
					const int16_t * samples = (const int16_t*)soundData->sampleData;
					
					const int volume = mouse.x * 256 / 800;
					
					for (int i = 0; i < I2S_2CH_FRAME_COUNT; ++i)
					{
						data[i][0] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][1] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						
						samplePosition++;
						
						if (samplePosition == soundData->sampleCount)
							samplePosition = 0;
					}
				}
				
				send(sock, data, sizeof(data), 0);
			}
			
		error:
			delete soundData;
			soundData = nullptr;
			
			if (sock != -1)
			{
				close(sock);
				sock = -1;
			}
		});
		
		return true;
	}
	
	void shut()
	{
		if (isActive)
		{
			wantsToStop = true;
			
			thread.join();
			
			wantsToStop = false;
			
			isActive = false;
		}
	}
};

//

struct Test_TcpToI2SQuad
{
	bool isActive = false;
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	Test_TcpToI2SQuad()
		: wantsToStop(false)
	{
	}
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort)
	{
		Assert(isActive == false);
		
		if (isActive)
		{
			shut();
		}
		
		isActive = true;
		
		thread = std::thread([=]()
		{
			struct sockaddr_in addr;
			int sock = 0;
			int sock_value = 0;
			SoundData * soundData = nullptr;
			int samplePosition = 0;
			
			//
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			
			if (sock == -1)
			{
				logError("failed opening socket");
				goto error;
			}
			
			// disable nagle's algorithm and send out packets immediately on write
			
		#if 1
			sock_value = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_value, sizeof(sock_value));
		#endif
		
		#if 1
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			sock_value =
				I2S_4CH_BUFFER_COUNT  * /* N times buffered */
				I2S_4CH_FRAME_COUNT   * /* frame count */
				I2S_4CH_CHANNEL_COUNT * /* stereo */
				sizeof(int16_t) /* sample size */;
 			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sock_value, sizeof(sock_value));
		#endif
		
			// todo : TCP_NODELAY (osx). disables nagle's algorithm and sends out packets immediately on write
			// todo : TCP_NOPUSH (osx)
			// todo : TCP_CORK (linux). will manually batch messages and send them when the cork is removed

		// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(ipAddress);
			addr.sin_port = htons(tcpPort);

			logDebug("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				logError("failed to connect socket");
				goto error;
			}
			
			logError("connected to remote endpoint!");
			
			soundData = loadSound("loop01.ogg");
			
			logDebug("frame size: %d", I2S_4CH_FRAME_COUNT * 4 * sizeof(int16_t));
			
			while (wantsToStop.load() == false)
			{
				// todo : detect is disconnected, and attempt to reconnect
				// todo : avoid high CPU on disconnect
				// todo : perform disconnection test
				
				while (keyboard.isDown(SDLK_SPACE))
				{
					SDL_Delay(10);
				}
				
				// todo : generate some audio data
				
				int16_t data[I2S_4CH_FRAME_COUNT][4];
				
				// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
				if (soundData->sampleCount == 0 ||
					soundData->channelCount != 2 ||
					soundData->channelSize != 2)
				{
					memset(data, 0, sizeof(data));
				}
				else
				{
					const int16_t * samples = (const int16_t*)soundData->sampleData;
					
					const int volume = mouse.x * 256 / 800;
					
					for (int i = 0; i < I2S_4CH_FRAME_COUNT; ++i)
					{
						data[i][0] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][1] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						data[i][2] = (samples[samplePosition * 2 + 0] * volume) >> 8;
						data[i][3] = (samples[samplePosition * 2 + 1] * volume) >> 8;
						
						samplePosition++;
						
						if (samplePosition == soundData->sampleCount)
							samplePosition = 0;
					}
				}
				
				send(sock, data, sizeof(data), 0);
			}
			
		error:
			delete soundData;
			soundData = nullptr;
			
			if (sock != -1)
			{
				close(sock);
				sock = -1;
			}
		});
		
		return true;
	}
	
	void shut()
	{
		if (isActive)
		{
			wantsToStop = true;
			{
				thread.join();
			}
			wantsToStop = false;
			
			isActive = false;
		}
	}
};

//

struct Test_TcpToI2SMono8
{
	bool isActive = false;
	
	std::atomic<bool> wantsToStop;
	
	std::thread thread;
	
	Test_TcpToI2SMono8()
		: wantsToStop(false)
	{
	}
	
	bool init(const uint32_t ipAddress, const uint16_t tcpPort)
	{
		Assert(isActive == false);
		
		if (isActive)
		{
			shut();
		}
		
		isActive = true;
		
		thread = std::thread([=]()
		{
			struct sockaddr_in addr;
			int sock = 0;
			int sock_value = 0;
			SoundData * soundData = nullptr;
			int samplePosition = 0;
			
			//
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			
			if (sock == -1)
			{
				logError("failed opening socket");
				goto error;
			}
			
			// disable nagle's algorithm and send out packets immediately on write
			
		#if 1
			sock_value = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_value, sizeof(sock_value));
		#endif
		
		#if 1
			// tell the TCP stack to use a specific buffer size. usually the TCP stack is
			// configured to use a rather large buffer size to increase bandwidth. we want
			// to keep latency down however, so we reduce the buffer size here
			
			sock_value =
				I2S_1CH_8_BUFFER_COUNT  * /* N times buffered */
				I2S_1CH_8_FRAME_COUNT   * /* frame count */
				I2S_1CH_8_CHANNEL_COUNT * /* stereo */
				sizeof(int8_t) /* sample size */;
 			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sock_value, sizeof(sock_value));
		#endif
		
			// todo : TCP_NODELAY (osx). disables nagle's algorithm and sends out packets immediately on write
			// todo : TCP_NOPUSH (osx)
			// todo : TCP_CORK (linux). will manually batch messages and send them when the cork is removed

		// todo : strp-laserapp : use writev or similar to send multiple packets to the same Artnet controller
		
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(ipAddress);
			addr.sin_port = htons(tcpPort);

			logDebug("connecting socket to remote endpoint");
			
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				logError("failed to connect socket");
				goto error;
			}
			
			logError("connected to remote endpoint!");
			
			soundData = loadSound("loop01.ogg");
			
			logDebug("frame size: %d", I2S_1CH_8_FRAME_COUNT * sizeof(int8_t));
			
			while (wantsToStop.load() == false)
			{
				// todo : detect is disconnected, and attempt to reconnect
				// todo : avoid high CPU on disconnect
				// todo : perform disconnection test
				
				while (keyboard.isDown(SDLK_SPACE))
				{
					SDL_Delay(10);
				}
				
				// todo : generate some audio data
				
				int8_t data[I2S_1CH_8_FRAME_COUNT];
				
				// we're kind of strict with regard to the sound format we're going to allow .. to simplify the streaming a bit
				if (soundData->sampleCount == 0 ||
					soundData->channelCount != 2 ||
					soundData->channelSize != 2)
				{
					memset(data, 0, sizeof(data));
				}
				else
				{
					const int16_t * samples = (const int16_t*)soundData->sampleData;
					
					const int volume = mouse.x * 256 / 800;
					
					for (int i = 0; i < I2S_1CH_8_FRAME_COUNT; ++i)
					{
						const int value =
							(
								(
									samples[samplePosition * 2 + 0] +
									samples[samplePosition * 2 + 1]
								) * volume
							) >> (16 + 1);
						
						//Assert(value >= -128 && value <= +127);
						
						data[i] = value;
						
						//Assert(data[i] == value);
						
						samplePosition++;
						
						if (samplePosition == soundData->sampleCount)
							samplePosition = 0;
					}
				}
				
				send(sock, data, sizeof(data), 0);
			}
			
		error:
			delete soundData;
			soundData = nullptr;
			
			if (sock != -1)
			{
				close(sock);
				sock = -1;
			}
		});
		
		return true;
	}
	
	void shut()
	{
		if (isActive)
		{
			wantsToStop = true;
			{
				thread.join();
			}
			wantsToStop = false;
			
			isActive = false;
		}
	}
};

//

#include <list>

struct NodeState
{
	uint64_t nodeId = -1;
	
	bool showTests = false;
	
	Test_TcpToI2S test_tcpToI2S;
	Test_TcpToI2SQuad test_tcpToI2SQuad;
	Test_TcpToI2SMono8 test_tcpToI2SMono8;
	
	struct
	{
		bool enabled = true;
		uint8_t sequenceNumber = 0;
	} artnetToDmx;
	
	struct
	{
		bool enabled = true;
		uint8_t sequenceNumber = 0;
	} artnetToLedstrip;
};

static std::list<NodeState> s_nodeStates;

static NodeState & findOrCreateNodeState(const uint64_t id)
{
	for (auto & nodeState : s_nodeStates)
	{
		if (nodeState.nodeId == id)
			return nodeState;
	}
	
	s_nodeStates.emplace_back();
	auto & nodeState = s_nodeStates.back();
	nodeState.nodeId = id;
	return nodeState;
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	if (!framework.init(800, 600))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	NodeDiscoveryProcess discoveryProcess;
	
	discoveryProcess.init();
	
	UdpSocket artnetSocket;
	
	for (;;)
	{
		framework.process();

		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		if (framework.quitRequested)
			break;

		const int numRecords = discoveryProcess.getRecordCount();
		
		for (int i = 0; i < numRecords; ++i)
		{
			auto record = discoveryProcess.getDiscoveryRecord(i);
			
			auto & nodeState = findOrCreateNodeState(record.id);
			
			if (record.capabilities & kNodeCapability_TcpToI2SQuad)
			{
				if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2SQuad.shut();
					}
					else
					{
						nodeState.test_tcpToI2SQuad.init(record.endpointName.address, I2S_4CH_PORT);
					}
				}
			}
			else if (record.capabilities & kNodeCapability_TcpToI2S)
			{
				if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2S.shut();
					}
					else
					{
						nodeState.test_tcpToI2S.init(record.endpointName.address, I2S_2CH_PORT);
					}
				}
			}
			else if (record.capabilities & kNodeCapability_TcpToI2SMono8)
			{
				if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2SMono8.shut();
					}
					else
					{
						nodeState.test_tcpToI2SMono8.init(record.endpointName.address, I2S_1CH_8_PORT);
					}
				}
			}
		
			// send some artnet data to discovered nodes
			
			if ((record.capabilities & kNodeCapability_ArtnetToDmx) && nodeState.artnetToDmx.enabled)
			{
				ArtnetPacket packet;
				
				nodeState.artnetToDmx.sequenceNumber = (nodeState.artnetToDmx.sequenceNumber + 10) % 256;
				if (nodeState.artnetToDmx.sequenceNumber == 0)
					nodeState.artnetToDmx.sequenceNumber++;
				
				auto * values = packet.makeDMX512(4, nodeState.artnetToDmx.sequenceNumber);
				
				for (int i = 0; i < 4; ++i)
				{
					const float brightness = mouse.x / 800.f;
					const float value = (sinf(framework.time * (i + 1) / 10.f) + 1.f) / 2.f * brightness;
					values[i] = uint8_t(value * 255.f);
				}
			
				const IpEndpointName remoteEndpoint(record.endpointName.address, ARTNET_TO_DMX_PORT);
				
				artnetSocket.SendTo(remoteEndpoint, (const char*)packet.data, packet.dataSize);
			}
			
			if ((record.capabilities & kNodeCapability_ArtnetToLedstrip) && nodeState.artnetToLedstrip.enabled)
			{
				ArtnetPacket packet;
			
				auto packFloatToDmx16 = [](const float value, const float gamma, uint8_t & hi, uint8_t & lo)
				{
					const uint16_t value16 = uint16_t(powf(value, gamma) * (1 << 16));
					hi = value16 >> 8;
					lo = uint8_t(value16);
				};
			
				nodeState.artnetToLedstrip.sequenceNumber = (nodeState.artnetToLedstrip.sequenceNumber + 10) % 256;
				if (nodeState.artnetToLedstrip.sequenceNumber == 0)
					nodeState.artnetToLedstrip.sequenceNumber++;
				
				auto * values = packet.makeDMX512(6, nodeState.artnetToLedstrip.sequenceNumber);
			
				for (int i = 0; i < 3; ++i)
				{
					const float brightness = mouse.x / 800.f;
					const float value = (sinf(framework.time * (i + 1) / 10.f) + 1.f) / 2.f * brightness;
					packFloatToDmx16(value, 1.f, values[i * 2 + 0], values[i * 2 + 1]);
				}
				
				const IpEndpointName remoteEndpoint(record.endpointName.address, ARTNET_TO_LED_PORT);
				
				artnetSocket.SendTo(remoteEndpoint, (const char*)packet.data, packet.dataSize);
			}
		}
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 800, 600, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(40, 100), ImGuiCond_Once);
			if (ImGui::Begin("Nodes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				const int numRecords = discoveryProcess.getRecordCount();
			
				for (int i = 0; i < numRecords; ++i)
				{
					auto record = discoveryProcess.getDiscoveryRecord(i);
					
					auto & nodeState = findOrCreateNodeState(record.id);
					
					char endpointName[IpEndpointName::ADDRESS_STRING_LENGTH];
					record.endpointName.AddressAsString(endpointName);
					
					ImGui::PushID(endpointName);
					
					ImGui::TextColored(ImVec4(0, 255, 0, 255), "%s", endpointName);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(255, 255, 255, 255), "%llx", record.id);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(255, 255, 255, 255), "%s", record.description);
					ImGui::SameLine();
					
					if (ImGui::Button("Open webpage"))
					{
						char command[128];
						sprintf_s(command, sizeof(command), "open http://%s", endpointName);
						system(command);
					}
					ImGui::SameLine();
					
					ImGui::PushStyleColor(ImGuiCol_Text, (uint32_t)ImColor(255, 255, 0));
					{
						if (record.capabilities & kNodeCapability_ArtnetToDmx)
						{
							ImGui::Text("Art2DMX");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_ArtnetToLedstrip)
						{
							ImGui::Text("Art2Led");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_ArtnetToAnalogPin)
						{
							ImGui::Text("Art2Pin");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2SMono8)
						{
							ImGui::Text("Tcp2I2S(1ch-8)");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2S)
						{
							ImGui::Text("Tcp2I2S(2ch)");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2SQuad)
						{
							ImGui::Text("Tcp2I2S(4ch)");
							ImGui::SameLine();
						}
					}
					ImGui::PopStyleColor();
					
					ImGui::Checkbox("Show tests", &nodeState.showTests);
					
					if (nodeState.showTests)
					{
						if (record.capabilities & kNodeCapability_ArtnetToDmx)
						{
							ImGui::Checkbox("Artnet to DMX", &nodeState.artnetToDmx.enabled);
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_ArtnetToLedstrip)
						{
							ImGui::Checkbox("Artnet to ledstrip", &nodeState.artnetToLedstrip.enabled);
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_ArtnetToAnalogPin)
						{
							ImGui::Text("Art2Pin");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2SMono8)
						{
							ImGui::Text("Tcp2I2S(1ch-8)");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2S)
						{
							ImGui::Text("Tcp2I2S(2ch)");
							ImGui::SameLine();
						}
						
						if (record.capabilities & kNodeCapability_TcpToI2SQuad)
						{
							ImGui::Text("Tcp2I2S(4ch)");
							ImGui::SameLine();
						}
					}
					
					ImGui::NewLine();
					
					ImGui::PopID();
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			const int numRecords = discoveryProcess.getRecordCount();
			
			for (int i = 0; i < numRecords; ++i)
			{
				auto record = discoveryProcess.getDiscoveryRecord(i);
				
				auto & nodeState = findOrCreateNodeState(record.id);
				
				char endpointName[IpEndpointName::ADDRESS_STRING_LENGTH];
				record.endpointName.AddressAsString(endpointName);
				
				const int sy = 22;
				
				const int x1 = 0;
				const int y1 = (i + 0) * sy;
				const int x2 = 300;
				const int y2 = (i + 1) * sy;
				
				const bool isInside = mouse.x >= x1 && mouse.x < x2 && mouse.y >= y1 && mouse.y < y2;
				const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
				const bool isClicked = isInside && mouse.wentUp(BUTTON_LEFT);
				
				if (isInside)
				{
					setColor(isDown ? colorRed : colorBlue);
					drawRect(x1, y1, x2, y2);
				}
				
				if (isClicked && (record.capabilities & kNodeCapability_Webpage) != 0)
				{
					char command[128];
					sprintf_s(command, sizeof(command), "open http://%s", endpointName);
					system(command);
				}
				
				int x = x1 + 10;
				
				setColor(colorGreen);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", endpointName);
				x += 100;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%llx", record.id);
				x += 100;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", record.description);
				x += 140;
				
				if (record.capabilities & kNodeCapability_ArtnetToDmx)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2DMX");
					x += 100;
				}
				
				if (record.capabilities & kNodeCapability_ArtnetToLedstrip)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2LED");
					x += 100;
				}
				
				if (record.capabilities & kNodeCapability_ArtnetToAnalogPin)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2Pin");
					x += 100;
				}
				
				if (record.capabilities & kNodeCapability_TcpToI2SMono8)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Tcp2I2S(1ch-8)");
					x += 100;
				}
				
				if (record.capabilities & kNodeCapability_TcpToI2S)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Tcp2I2S(2ch)");
					x += 100;
				}
				
				if (record.capabilities & kNodeCapability_TcpToI2SQuad)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Tcp2I2S(4ch)");
					x += 100;
				}
				
				if (nodeState.test_tcpToI2SMono8.isActive ||
					nodeState.test_tcpToI2S.isActive ||
					nodeState.test_tcpToI2SQuad.isActive)
				{
					setColor(colorGreen);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "(Playing)");
					x += 100;
				}
			}
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	for (auto & nodeState : s_nodeStates)
	{
		nodeState.test_tcpToI2S.shut();
		nodeState.test_tcpToI2SQuad.shut();
		nodeState.test_tcpToI2SMono8.shut();
	}
	
	discoveryProcess.shut();
	
	framework.shutdown();
	
	return 0;
}
