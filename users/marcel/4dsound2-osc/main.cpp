#include "Csv.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "Parse.h"
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"
#include "vfxNodes/oscEndpointMgr.h"

// todo : let inputs set if visible in node editor or not
// if not, node editor behavior changes:
// - node gets highlighted when dragging a link endpoint over it
// - a list pops up asking to connect to one of the inputs when released

const int VIEW_SX = 1000;
const int VIEW_SY = 740;

struct VfxNodeMems : VfxNodeBase
{
	enum Input
	{
		kInput_Name,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	std::string valueOutput;
	
	VfxNodeMems()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Name, kVfxPlugType_String);
		addOutput(kOutput_Value, kVfxPlugType_String, &valueOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const char * name = getInputString(kInput_Name, nullptr);
		
		if (name == nullptr)
		{
			valueOutput.clear();
		}
		else
		{
			if (g_currentVfxGraph->getMems(name, valueOutput) == false)
				valueOutput.clear();
		}
	}
};

VFX_NODE_TYPE(VfxNodeMems)
{
	typeName = "in.string";
	
	in("name", "string");
	out("value", "string");
}

struct VfxNodeStringAppend : VfxNodeBase
{
	enum Input
	{
		kInput_A,
		kInput_B,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	std::string resultOutput;
	
	VfxNodeStringAppend()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_A, kVfxPlugType_String);
		addInput(kInput_B, kVfxPlugType_String);
		addOutput(kOutput_Result, kVfxPlugType_String, &resultOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const char * a = getInputString(kInput_A, "");
		const char * b = getInputString(kInput_B, "");
		
		resultOutput = std::string(a) + b;
	}
};

VFX_NODE_TYPE(VfxNodeStringAppend)
{
	typeName = "string.append";
	
	in("a", "string");
	in("b", "string");
	out("result", "string");
}

struct VfxNode4DSoundObject : VfxNodeBase
{
	enum Input
	{
		kInput_OscEndpoint,
		kInput_OscPrefix,
		kInput_OscSheet,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_COUNT
	};
	
	struct InputInfo
	{
		std::string oscAddress;
		
		bool isVec3f = false;
		
		float defaultFloat = 0.f;
		int defaultInt = 0;
		bool defaultBool = false;
	};
	
	std::string currentOscPrefix;
	
	std::vector<InputInfo> inputInfos;
	
	// todo : parse OSC sheet. detect which parameters belong to this sound object. create dynamic inputs
	
	VfxNode4DSoundObject()
		: VfxNodeBase()
		, currentOscPrefix()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_OscEndpoint, kVfxPlugType_String);
		addInput(kInput_OscPrefix, kVfxPlugType_String);
		addInput(kInput_OscSheet, kVfxPlugType_String);
	}
	
	void updateOscSheet()
	{
		const char * oscPrefix =
			isPassthrough
			? ""
			: getInputString(kInput_OscPrefix, "");
		
		if (oscPrefix == currentOscPrefix)
		{
			return;
		}
		
		currentOscPrefix = oscPrefix;
		
		inputInfos.clear();
		
		if (oscPrefix[0] == 0)
		{
			setDynamicInputs(nullptr, 0);
		}
		else
		{
			std::vector<VfxNodeBase::DynamicInput> inputs;
			
			CsvDocument csvDocument;
			
			if (csvDocument.load("osc-sheet.txt", true, '\t'))
			{
				for (auto & row : csvDocument.m_rows)
				{
					const char * oscAddress = row.getString("OSC Address", nullptr);
					const char * type = row.getString("Type", nullptr);
					const char * defaultValue = row.getString("Default value", "");
					
					if (oscAddress == nullptr || type == nullptr)
						continue;
					
					if (strstr(oscAddress, oscPrefix) != oscAddress)
						continue;
					
					const char * name = oscAddress + currentOscPrefix.size();
					
					while (*name == '/')
						name++;
					
					bool skipped = false;
					
					if (strcmp(type, "f") == 0)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Float;
						input.name = name;
						input.defaultValue = defaultValue;
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultFloat = Parse::Float(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else if (strcmp(type, "f f f") == 0) // todo : not yet supported
					{
						for (int i = 0; i < 3; ++i)
						{
							const char elem[3] = { 'x', 'y', 'z' };
							
							VfxNodeBase::DynamicInput input;
							input.type = kVfxPlugType_Float;
							input.name = String::FormatC("%s.%c", name, elem[i]);
							input.defaultValue = defaultValue;
							inputs.push_back(input);
							
							InputInfo inputInfo;
							inputInfo.oscAddress = oscAddress;
							inputInfo.isVec3f = true;
							inputInfo.defaultFloat = Parse::Float(defaultValue);
							inputInfos.push_back(inputInfo);
						}
					}
					else if (strcmp(type, "i") == 0)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Int;
						input.name = name;
						input.defaultValue = defaultValue;
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultInt = Parse::Int32(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else if (strstr(type, "boolean") != nullptr)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Bool;
						input.name = name;
						input.defaultValue = strcmp(defaultValue, "true") == 0 ? "1" : "0";
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultBool = Parse::Bool(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else
					{
						logDebug("unknown OSC data type: %s", type);
						
						skipped = true;
					}
				}
			}
			
			if (inputs.empty())
			{
				setDynamicInputs(nullptr, 0);
			}
			else
			{
				setDynamicInputs(&inputs[0], inputs.size());
			}
			
			Assert(inputInfos.size() == dynamicInputs.size());
		}
	}
	
	virtual void tick(const float dt) override
	{
		const char * endpointName = getInputString(kInput_OscEndpoint, "");
		
		updateOscSheet();
		
		auto endpoint = g_oscEndpointMgr.findSender(endpointName);
		
		if (endpoint != nullptr)
		{
			char buffer[1 << 12];
			osc::OutboundPacketStream stream(buffer, sizeof(buffer));
			
			bool isEmpty = true;
			
			for (size_t i = 0; i < dynamicInputs.size(); )
			{
				auto & input = dynamicInputs[i];
				
				auto & inputInfo = inputInfos[i];
				
				if (isEmpty)
					stream << osc::BeginBundleImmediate;
				
				stream << osc::BeginMessage(inputInfos[i].oscAddress.c_str());
				{
					if (inputInfo.isVec3f)
					{
						stream
							<< getInputFloat(kInput_COUNT + i + 0, inputInfo.defaultFloat)
							<< getInputFloat(kInput_COUNT + i + 1, inputInfo.defaultFloat)
							<< getInputFloat(kInput_COUNT + i + 2, inputInfo.defaultFloat);
						
						i += 3;
					}
					else
					{
						if (input.type == kVfxPlugType_Float)
							stream << getInputFloat(kInput_COUNT + i, inputInfo.defaultFloat);
						else if (input.type == kVfxPlugType_Int)
							stream << getInputInt(kInput_COUNT + i, inputInfo.defaultInt);
						else if (input.type == kVfxPlugType_Bool)
							stream << getInputBool(kInput_COUNT + i, inputInfo.defaultBool);
						else
						{
							Assert(false);
						}
						
						i += 1;
					}
				}
				stream << osc::EndMessage;
				
				isEmpty = false;
				
				// flush if the buffer is getting full
				
				if (stream.Size() >= 1000)
				{
					stream << osc::EndBundle;
					
					endpoint->send(stream.Data(), stream.Size());
					
					stream = osc::OutboundPacketStream(buffer, sizeof(buffer));
					isEmpty = true;
				}
			}
			
			// flush
			
			if (isEmpty == false)
			{
				stream << osc::EndBundle;
				
				endpoint->send(stream.Data(), stream.Size());
			}
		}
	}
	
	virtual void init(const GraphNode & node) override
	{
		updateOscSheet();
	}
};

VFX_NODE_TYPE(VfxNode4DSoundObject)
{
	typeName = "4d.soundObject";
	
	in("oscEndpoint", "string");
	in("oscPrefix", "string");
	in("oscSheet", "string");
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

// todo : create vfx graph
	VfxGraph * vfxGraph = nullptr;
	
	RealTimeConnection rtc(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	
	GraphEdit graphEdit(VIEW_SX, VIEW_SY, &typeDefinitionLibrary, &rtc);
	
	graphEdit.load("test1.xml");
	
	vfxGraph->setMems("id", "1");

	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		const float dt = framework.timeStep;
		
		// update the graph editor
		
		graphEdit.tick(dt, false);
		
		// update OSC message
		
		g_oscEndpointMgr.tick();
		
		// update vfx graph
		
		vfxGraph->tick(VIEW_SX, VIEW_SY, dt);
		
		// update the visualizers after the vfx graph has been updated
		
		graphEdit.tickVisualizers(dt);

		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the vfx graph
			
			vfxGraph->draw(VIEW_SX, VIEW_SY);
		
			// draw the graph editor
			
			graphEdit.draw();
		}
		framework.endDraw();
	}

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
