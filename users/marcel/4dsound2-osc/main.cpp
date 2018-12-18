#include "Csv.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "Parse.h"
#include "StringEx.h"
#include "ui.h"
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxUi.h"

// todo : let inputs set if visible in node editor or not
// if not, node editor behavior changes:
// - node gets highlighted when dragging a link endpoint over it
// - a list pops up asking to connect to one of the inputs when released

// todo : changed files in framework
// todo : + use corrrect osc sheet file
// todo : + detect osc sheet changes

const int VIEW_SX = 700;
const int VIEW_SY = 800;

static std::set<std::string> s_changedFiles;

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

struct VfxNode4DSoundObject : VfxNodeBase
{
	enum Input
	{
		kInput_OscEndpoint,
		kInput_OscPrefix,
		kInput_GroupPrefix,
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
	std::string currentOscSheet;
	bool currentGroupPrefix;
	
	std::vector<InputInfo> inputInfos;
	
	VfxNode4DSoundObject()
		: VfxNodeBase()
		, currentOscPrefix()
		, currentOscSheet()
		, currentGroupPrefix(false)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_OscEndpoint, kVfxPlugType_String);
		addInput(kInput_OscPrefix, kVfxPlugType_String);
		addInput(kInput_GroupPrefix, kVfxPlugType_Bool);
		addInput(kInput_OscSheet, kVfxPlugType_String);
	}
	
	void updateOscSheet()
	{
		const char * oscPrefix =
			isPassthrough
			? ""
			: getInputString(kInput_OscPrefix, "");
		
		const bool groupPrefix = getInputBool(kInput_GroupPrefix, true);
		
		const char * oscSheet = getInputString(kInput_OscSheet, "");
		
		if (s_changedFiles.count(oscSheet) != 0)
		{
			currentOscSheet.clear();
		}
		
		if (oscPrefix == currentOscPrefix && oscSheet == currentOscSheet && groupPrefix == currentGroupPrefix)
		{
			return;
		}
		
		currentOscPrefix = oscPrefix;
		currentOscSheet = oscSheet;
		currentGroupPrefix = groupPrefix;
		
		inputInfos.clear();
		
		if (oscPrefix[0] == 0 || oscSheet[0] == 0)
		{
			setDynamicInputs(nullptr, 0);
		}
		else
		{
			// parse OSC sheet. detect which parameters belong to this sound object. create dynamic inputs
			
			std::vector<VfxNodeBase::DynamicInput> inputs;
			
			ReadOnlyCsvDocument csvDocument;
			
			if (csvDocument.load(oscSheet, true, '\t'))
			{
				const int oscAddress_index = csvDocument.getColumnIndex("OSC Address");
				const int type_index = csvDocument.getColumnIndex("Type");
				const int defaultValue_index = csvDocument.getColumnIndex("Default value");
				const int enumValues_index = csvDocument.getColumnIndex("Enumeration Values");
				
				const size_t currentOscPrefixSize = currentOscPrefix.size();
				
				if (oscAddress_index >= 0 && type_index >= 0 && defaultValue_index >= 0)
				{
					for (auto i = csvDocument.firstRow(); i != csvDocument.lastRow(); i = csvDocument.nextRow(i))
					{
						const char * oscAddress = i[oscAddress_index];
						const char * type = i[type_index];
						const char * defaultValue = i[defaultValue_index];
						
						if (strstr(oscAddress, oscPrefix) != oscAddress)
							continue;
						
						const char * name = oscAddress + currentOscPrefixSize;
						
						if (groupPrefix && *name != '/')
							continue;
						
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
						else if (strstr(type, "enum") != nullptr)
						{
							VfxNodeBase::DynamicInput input;
							input.type = kVfxPlugType_Int;
							input.name = name;
							input.defaultValue = strcmp(defaultValue, "true") == 0 ? "1" : "0";
							
							if (enumValues_index >= 0)
							{
								std::vector<std::string> enumNames;
								splitString(i[enumValues_index], enumNames, ',');
								
								input.enumElems.resize(enumNames.size());
								
								for (size_t i = 0; i < enumNames.size(); ++i)
								{
									input.enumElems[i].name = enumNames[i];
									input.enumElems[i].valueText = String::FormatC("%d", i);
								}
							}
							
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
	in("groupPrefix", "bool", "1");
	in("oscSheet", "string");
}

//

static VfxGraphManager * s_vfxGraphMgr = nullptr;

struct Creature
{
	VfxGraphInstance * vfxInstance = nullptr;
	
	Vec2 currentPos;
	Vec2 desiredPos;
	
	float moveTimer = 0.f;
	
	void init(const int id)
	{
		const char * filenames[2] =
		{
			"test1.xml",
			"test2.xml"
		};
		
		const char * filename = filenames[id % 2];
		
		vfxInstance = s_vfxGraphMgr->createInstance(filename, 64, 64);
		
		vfxInstance->vfxGraph->setMems("id", String::FormatC("%d", id + 1).c_str());
		
		vfxInstance->vfxGraph->setMemf("pos", currentPos[0], currentPos[1]);
	}
	
	void shut()
	{
		s_vfxGraphMgr->free(vfxInstance);
		Assert(vfxInstance == nullptr);
	}
	
	void tick(const float dt)
	{
		moveTimer -= dt;
		
		if (moveTimer <= 0.f)
		{
			moveTimer = random(0.f, 10.f);
			
			desiredPos.Set(random(-10.f, +10.f), random(-10.f, +10.f));
		}
		
		const float retainPerSecond = .7f;
		const float retain = powf(retainPerSecond, dt);
		const float attain = 1.f - retain;
		
		currentPos = currentPos * retain + desiredPos * attain;
		
		vfxInstance->vfxGraph->setMemf("pos", currentPos[0], currentPos[1]);
	}
	
	void draw() const
	{
		Assert(vfxInstance->texture != 0);
		gxSetTexture(vfxInstance->texture);
		{
			pushBlend(BLEND_OPAQUE);
			
			setColor(colorWhite);
			drawRect(
				currentPos[0] - 1.f, currentPos[1] - 1.f,
				currentPos[0] + 1.f, currentPos[1] + 1.f);
			
			popBlend();
		}
		gxSetTexture(0);
	}
};

struct World
{
	static const int kNumCreatures = 4;
	
	Creature creatures[kNumCreatures];
	
	void init()
	{
		for (int i = 0; i < kNumCreatures; ++i)
			creatures[i].init(i);
	}
	
	void shut()
	{
		for (int i = 0; i < kNumCreatures; ++i)
			creatures[i].shut();
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < kNumCreatures; ++i)
			creatures[i].tick(dt);
	}
	
	void draw() const
	{
		gxPushMatrix();
		{
			gxTranslatef(VIEW_SX/2.f, VIEW_SY/2.f, 0.f);
			gxScalef(40, 40, 1);
			
			for (int i = 0; i < kNumCreatures; ++i)
				creatures[i].draw();
		}
		gxPopMatrix();
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	framework.realTimeEditCallback = [](const std::string & filename)
	{
		s_changedFiles.insert(filename);
	};
	
	// create vfx type definition library
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	
	// create vfx graph manager
	
	//VfxGraphManager_Basic vfxGraphMgr(true);
	VfxGraphManager_RTE vfxGraphMgr(VIEW_SX, VIEW_SY);
	s_vfxGraphMgr = &vfxGraphMgr;
	
	// setup world

	World world;
	world.init();
	
	// select instance for editing
	
	vfxGraphMgr.selectInstance(world.creatures[0].vfxInstance);
	
	UiState uiState;
	uiState.x = 20;
	uiState.y = VIEW_SY - 140;
	uiState.sx = 200;
	std::string activeInstanceName;
	
	auto doMenus = [&](const bool doActions, const bool doDraw)
	{
		makeActive(&uiState, doActions, doDraw);
		pushMenu("vfx select");
		doVfxGraphInstanceSelect(vfxGraphMgr, activeInstanceName);
		popMenu();
	};
	
	while (!framework.quitRequested)
	{
		s_changedFiles.clear();
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		// update menus
		
		doMenus(true, false);
		
		inputIsCaptured |= uiState.activeElem != nullptr;
		
		// update the graph editor
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(dt, inputIsCaptured);
		
		// update OSC messages
		
		g_oscEndpointMgr.tick();
		
		// update world
		
		world.tick(dt);
		
		// update vfx graphs
		
		vfxGraphMgr.tick(dt);
		
		// update the visualizers after the vfx graphs have been updated
		
		vfxGraphMgr.tickVisualizers(dt);

		framework.beginDraw(0, 0, 0, 0);
		{
			// draw vfx graphs
			
			vfxGraphMgr.traverseDraw();
			
			// draw the world
			
			world.draw();
		
			// draw the graph editor
			
			vfxGraphMgr.drawEditor();
			
			// draw the menus
			
			doMenus(false, true);
		}
		framework.endDraw();
	}
	
	world.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
