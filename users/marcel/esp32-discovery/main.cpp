#include "artnet.h"
#include "framework.h"
#include "imgui-framework.h"
#include "ip/UdpSocket.h"
#include "parameter.h"
#include "parameterUi.h"
#include "StringEx.h"
#include "webrequest.h"
#include <atomic>
#include <inttypes.h> // PRIx64
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

/*

esp32 discovery process relies on the Arduino sketch 'esp32-wifi-configure'.
This is a sketch which lets the user select a Wifi access point and connect to it. The sketch will then proceed sending discovery messages at a regular interval. The discovery message containts the id of the device, and the IP address can be inferred from the received UDP packet.

*/

#include "nodeDiscovery.h"

#include "audioStreamToTcp-2ch-16bit.h"
#include "audioStreamToTcp-4ch-16bit.h"
#include "audioStreamToTcp-1ch-8bit.h"

// -- NodeState

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
	
	struct
	{
		std::string text;
		WebRequest * fetchRequest = nullptr;
	} parameterJson;
	
	struct ParameterUi
	{
		ParameterMgr paramMgr;
		std::list<ParameterMgr> paramMgrs;
	} parameterUi;
	
	void beginFetchParameterJson(const char * endpointName)
	{
		if (parameterJson.fetchRequest != nullptr)
		{
			delete parameterJson.fetchRequest;
			parameterJson.fetchRequest = nullptr;
		}
		
		parameterJson.text.clear();
		
		parameterUi = ParameterUi();
		
		char url[128];
		sprintf_s(url, sizeof(url), "http://%s/getParameters", endpointName);
		
		parameterJson.fetchRequest = createWebRequest(url);
	}
	
	void buildParameterUiFromJson_recursive(rapidjson::Document::ValueType & document, ParameterMgr & parent)
	{
		auto paramMgr_json = document.GetObject();
		
		auto & name_json = paramMgr_json["name"];
		const char * name = name_json.GetString();
		
		parameterUi.paramMgrs.push_back(ParameterMgr());
		ParameterMgr & paramMgr = parameterUi.paramMgrs.back();
		paramMgr.init(name);
		parent.addChild(&paramMgr);
		
		auto decodeFloatArray = [](const rapidjson::Document::Array & array, float * dst, const int numFloats) -> bool
		{
			if (array.Size() != numFloats)
				return false;
			
			for (int i = 0; i < numFloats; ++i)
			{
				if (array[i].IsInt())
					dst[i] = array[i].GetInt();
				else if (array[i].IsNumber())
					dst[i] = array[i].GetFloat();
				else
					return false;
			}
			
			return true;
		};
		
		for (auto member_json = paramMgr_json.begin(); member_json != paramMgr_json.end(); ++member_json)
		{
			if (member_json->name == "parameters")
			{
				auto parameters_json = member_json->value.GetArray();
				
				for (auto parameter_json_element = parameters_json.begin(); parameter_json_element != parameters_json.end(); ++parameter_json_element)
				{
					auto & parameter_json = *parameter_json_element;
					
					auto & name_json = parameter_json["name"];
					auto & type_json = parameter_json["type"];
					
					if (name_json.IsString() == false || type_json.IsString() == false)
					{
						logWarning("parameter name or type is not a string");
						continue;
					}
					
					const char * name = name_json.GetString();
					const char * type = type_json.GetString();
					
					if (!strcmp(type, "bool"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						
						if (defaultValue_json.IsBool() == false)
							logDebug("parameter default value not set");
						if (value_json.IsBool() == false)
							logDebug("parameter value not set");
						
						const bool defaultValue =
							defaultValue_json.IsBool()
							? defaultValue_json.GetBool()
							: false;
						
						const bool value =
							value_json.IsBool()
							? value_json.GetBool()
							: defaultValue;
						
						auto * param = paramMgr.addBool(name, defaultValue);
						param->set(value);
					}
					else if (!strcmp(type, "int"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						
						if (defaultValue_json.IsInt() == false)
							logDebug("parameter default value not set");
						if (value_json.IsInt() == false)
							logDebug("parameter value not set");
						
						const int defaultValue =
							defaultValue_json.IsInt()
							? defaultValue_json.GetInt()
							: false;
						
						const int value =
							value_json.IsInt()
							? value_json.GetInt()
							: defaultValue;
						
						auto * param = paramMgr.addInt(name, defaultValue);
						param->set(value);
						
						//
						
						auto & hasRange_json = parameter_json["hasRange"];
						
						if (hasRange_json.IsBool() && hasRange_json.GetBool())
						{
							auto & min_json = parameter_json["min"];
							auto & max_json = parameter_json["max"];
						
							if (min_json.IsInt() == false || max_json.IsInt() == false)
								logDebug("parameter range not set");
							else
								param->setLimits(min_json.GetInt(), max_json.GetInt());
						}
					}
					else if (!strcmp(type, "float"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						
						if (defaultValue_json.IsNumber() == false)
							logDebug("parameter default value not set");
						if (value_json.IsNumber() == false)
							logDebug("parameter value not set");
						
						const float defaultValue =
							defaultValue_json.IsNumber()
							? defaultValue_json.GetFloat()
							: false;
						
						const float value =
							value_json.IsNumber()
							? value_json.GetFloat()
							: defaultValue;
						
						auto * param = paramMgr.addFloat(name, defaultValue);
						param->set(value);
						
						//
						
						auto & hasRange_json = parameter_json["hasRange"];
						
						if (hasRange_json.IsBool() && hasRange_json.GetBool())
						{
							auto & min_json = parameter_json["min"];
							auto & max_json = parameter_json["max"];
						
							if (min_json.IsNumber() == false || max_json.IsNumber() == false)
								logDebug("parameter range not set");
							else
								param->setLimits(min_json.GetFloat(), max_json.GetFloat());
						}
					}
					else if (!strcmp(type, "vec2"))
					{
						paramMgr.addVec2(name, Vec2());
					}
					else if (!strcmp(type, "vec3"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						
						Vec3 defaultValue;
						const bool hasDefaultValue =
							defaultValue_json.IsArray() &&
							decodeFloatArray(defaultValue_json.GetArray(), &defaultValue[0], 3);
						
						Vec3 value;
						const bool hasValue =
							value_json.IsArray() &&
							decodeFloatArray(value_json.GetArray(), &value[0], 3);
						
						if (hasDefaultValue == false)
							logDebug("parameter default value not set");
						if (hasValue == false)
							logDebug("parameter value not set");
						
						auto * param = paramMgr.addVec3(name, hasDefaultValue ? defaultValue : Vec3());
						
						if (hasValue)
							param->set(value);
					}
					else if (!strcmp(type, "vec4"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						
						Vec4 defaultValue;
						const bool hasDefaultValue =
							defaultValue_json.IsArray() &&
							decodeFloatArray(defaultValue_json.GetArray(), &defaultValue[0], 4);
						
						Vec4 value;
						const bool hasValue =
							value_json.IsArray() &&
							decodeFloatArray(value_json.GetArray(), &value[0], 4);
						
						if (hasDefaultValue == false)
							logDebug("parameter default value not set");
						if (hasValue == false)
							logDebug("parameter value not set");
						
						auto * param = paramMgr.addVec4(name, hasDefaultValue ? defaultValue : Vec4());
						
						if (hasValue)
							param->set(value);
					}
					else if (!strcmp(type, "enum"))
					{
						auto & defaultValue_json = parameter_json["defaultValue"];
						auto & value_json = parameter_json["value"];
						auto & elements_json_element = parameter_json["elements"];
						
						if (defaultValue_json.IsInt() == false)
							logDebug("parameter default value not set");
						if (value_json.IsInt() == false)
							logDebug("parameter value not set");
						if (elements_json_element.IsArray() == false)
							logDebug("parameter enumeration elements not set");
						
						std::vector<ParameterEnum::Elem> elems;
						
						if (elements_json_element.IsArray())
						{
							auto elements_json = elements_json_element.GetArray();
							
							for (auto element_json_element = elements_json.begin(); element_json_element != elements_json.end(); ++element_json_element)
							{
								auto & element_json = *element_json_element;
								
								auto key_json_member = element_json.FindMember("key");
								auto value_json_member = element_json.FindMember("value");
								
								if (key_json_member == element_json.MemberEnd() ||
									value_json_member == element_json.MemberEnd())
								{
									logDebug("parameter enumeration element is missing key and/or value member");
								}
								else
								{
									auto & key_json = key_json_member->value;
									auto & value_json = value_json_member->value;
									
									if (!key_json.IsString() || !value_json.IsInt())
										logDebug("parameter enumeration element is not of a valid key/value type");
									else
									{
										elems.emplace_back(
											ParameterEnum::Elem
												{
													key_json.GetString(),
													value_json.GetInt()
												});
									}
								}
							}
						}
						
						const int defaultValue =
							defaultValue_json.IsInt()
							? defaultValue_json.GetInt()
							: 0;
						
						auto * param = paramMgr.addEnum(name, defaultValue, elems);
						
						if (value_json.IsInt())
							param->set(value_json.GetInt());
					}
					else
					{
						Assert(false);
					}
				}
			}
			else if (member_json->name == "children")
			{
				auto children_json = member_json->value.GetArray();
				
				for (auto child_json_element = children_json.begin(); child_json_element != children_json.end(); ++child_json_element)
				{
					auto & child_json = *child_json_element;
					
					buildParameterUiFromJson_recursive(child_json, paramMgr);
				}
			}
		}
	}
	
	void buildParameterUiFromJson(const char * json)
	{
		rapidjson::Document document;
		document.Parse(json);
		
		buildParameterUiFromJson_recursive(document, parameterUi.paramMgr);
	}
	
	void tick()
	{
		if (parameterJson.fetchRequest != nullptr)
		{
			Assert(parameterJson.text.empty());
			
			if (parameterJson.fetchRequest->isDone())
			{
				char * text = nullptr;
				if (parameterJson.fetchRequest->getResultAsCString(text))
				{
					parameterJson.text = text;
					
					delete [] text;
					text = nullptr;
				}
				
				delete parameterJson.fetchRequest;
				parameterJson.fetchRequest = nullptr;
				
				//
				
				if (!parameterJson.text.empty())
				{
					buildParameterUiFromJson(parameterJson.text.c_str());
				}
			}
		}
	}
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

// -- testDummyTcpServer

#include "dummyTcpServer.h"
#include "threadedTcpClient.h"

static void testDummyTcpServer()
{
	Test_DummyTcpServer tcpServer;
	
	IpEndpointName endpointName(IpEndpointName::ANY_ADDRESS, 4000);
	tcpServer.init(endpointName);
	
#if 1
	Test_TcpToI2S tcpToI2S;
	
	tcpToI2S.init(IpEndpointName("127.0.0.1", -1).address, 4000, "loop01-short.ogg");
	
	SDL_Delay(3000);
	
	logInfo("shutting down dummy TCP server");
	
	tcpServer.beginShutdown();
	
	logInfo("shutting down I2S streamer");
	
	tcpToI2S.shut();
	//tcpConnection.beginShutdown();
	
	logInfo("waiting for I2S streamer shutdown to complete");
	
	// todo : add shutdown methods to I2S streamer : tcpConnection.waitForShutdown();
	
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
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	//testDummyTcpServer(); // todo : remove once done testing TCP connection refactor
	//return 0;
	
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

		discoveryProcess.purgeStaleRecords(30);
		
		const int numRecords = discoveryProcess.getRecordCount();
		
		for (int i = 0; i < numRecords; ++i)
		{
			auto record = discoveryProcess.getDiscoveryRecord(i);
			
			auto & nodeState = findOrCreateNodeState(record.id);
			
			nodeState.tick();
			
			const bool enableTests = nodeState.showTests;
			
			if (record.capabilities & kNodeCapability_TcpToI2SQuad)
			{
				if (enableTests == false)
				{
					nodeState.test_tcpToI2SQuad.shut();
				}
				else if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2SQuad.shut();
					}
					else
					{
						nodeState.test_tcpToI2SQuad.init(record.endpointName.address, I2S_4CH_PORT, "loop01-short.ogg");
					}
				}
				
				nodeState.test_tcpToI2SQuad.tick();
			}
			else if (record.capabilities & kNodeCapability_TcpToI2S)
			{
				if (enableTests == false)
				{
					nodeState.test_tcpToI2S.shut();
				}
				else if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2S.shut();
					}
					else
					{
						nodeState.test_tcpToI2S.init(record.endpointName.address, I2S_2CH_PORT, "loop01-short.ogg");
					}
				}
				
				nodeState.test_tcpToI2S.tick();
			}
			else if (record.capabilities & kNodeCapability_TcpToI2SMono8)
			{
				if (enableTests == false)
				{
					nodeState.test_tcpToI2SMono8.shut();
				}
				else if (keyboard.wentDown(SDLK_s))
				{
					if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
					{
						nodeState.test_tcpToI2SMono8.shut();
					}
					else
					{
						nodeState.test_tcpToI2SMono8.init(record.endpointName.address, I2S_1CH_8_PORT, "loop01-short.ogg");
					}
				}
				
				const float volume = mouse.x / 800.f;
				nodeState.test_tcpToI2SMono8.volume.store(volume);
			}
		
			// send some artnet data to discovered nodes
			
			if (enableTests && (record.capabilities & kNodeCapability_ArtnetToDmx) && nodeState.artnetToDmx.enabled)
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
			
			if (enableTests && (record.capabilities & kNodeCapability_ArtnetToLedstrip) && nodeState.artnetToLedstrip.enabled)
			{
				ArtnetPacket packet;
			
				auto packFloatToDmx16 = [](const double value, const double gamma, uint8_t & hi, uint8_t & lo)
				{
					const uint16_t value16 = uint16_t(pow(value, gamma) * ((1 << 16) - 1));
					hi = value16 >> 8;
					lo = uint8_t(value16);
				};
			
				nodeState.artnetToLedstrip.sequenceNumber = (nodeState.artnetToLedstrip.sequenceNumber + 10) % 256;
				if (nodeState.artnetToLedstrip.sequenceNumber == 0)
					nodeState.artnetToLedstrip.sequenceNumber++;
				
				auto * values = packet.makeDMX512(6, nodeState.artnetToLedstrip.sequenceNumber);
			
				for (int i = 0; i < 3; ++i)
				{
					const double brightness = mouse.x / 800.0;
					const double value = (sin(framework.time * (i * 8 + 1) / 4.0) + 1.0) / 2.0 * brightness;
					packFloatToDmx16(value, 1.0, values[i * 2 + 0], values[i * 2 + 1]);
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
					ImGui::TextColored(ImVec4(255, 255, 255, 255), "%016" PRIx64, record.id);
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(255, 255, 255, 255), "%s", record.description);
					ImGui::SameLine();
					
					if (record.capabilities & kNodeCapability_Webpage)
					{
						if (ImGui::Button("Open webpage"))
						{
							char command[128];
							sprintf_s(command, sizeof(command), "open http://%s", endpointName);
							if (system(command) != 0)
								logDebug("failed to open webpage using shell command");
						}
						ImGui::SameLine();
					
						if (ImGui::Button("Fetch json"))
						{
							nodeState.beginFetchParameterJson(endpointName);
						}
						ImGui::SameLine();
					}
					
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
					
					ImGui::Checkbox("Tests", &nodeState.showTests);
					
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
					
					if (nodeState.parameterJson.fetchRequest != nullptr)
					{
						ImGui::Text("Fetching JSON..");
					}
					
					if (nodeState.parameterJson.text.empty() == false)
					{
						//ImGui::Text("%s", nodeState.parameterJson.text.c_str());
					}
					
					if (nodeState.parameterUi.paramMgrs.empty() == false)
					{
						parameterUi::doParameterUi_recursive(nodeState.parameterUi.paramMgr, nullptr);
						
						// todo : send dirty parameters using http requests
					}
					
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
				const int x2 = 800;
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
					if (system(command) != 0)
						logDebug("failed to open webpage using shell command");
				}
				
				int x = x1 + 10;
				
				setColor(colorBlue);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%d", (SDL_GetTicks() - record.receiveTime) / 1000);
				x += 60;
				
				setColor(colorGreen);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", endpointName);
				x += 100;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%016" PRIx64, record.id);
				x += 130;
				
				setColor(colorWhite);
				drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "%s", record.description);
				x += 140;
				
				if (record.capabilities & kNodeCapability_ArtnetToDmx)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2DMX");
					x += 80;
				}
				
				if (record.capabilities & kNodeCapability_ArtnetToLedstrip)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2LED");
					x += 80;
				}
				
				if (record.capabilities & kNodeCapability_ArtnetToAnalogPin)
				{
					setColor(colorYellow);
					drawText(x, (y1 + y2) / 2.f, 14, +1, 0, "Art2Pin");
					x += 80;
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
				
				if (nodeState.test_tcpToI2SMono8.tcpConnection.isActive ||
					nodeState.test_tcpToI2S.audioStreamToTcp.tcpConnection.isActive ||
					nodeState.test_tcpToI2SQuad.audioStreamToTcp.tcpConnection.isActive)
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
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
