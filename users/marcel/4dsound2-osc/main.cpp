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

// todo : changed files in framework
// todo : + use corrrect osc sheet file
// todo : + detect osc sheet changes

const int VIEW_SX = 1000;
const int VIEW_SY = 740;

static std::set<std::string> s_changedFiles;

struct VfxNodeMemf : VfxNodeBase
{
	enum Input
	{
		kInput_Name,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_COUNT
	};
	
	Vec4 valueOutput;
	
	VfxNodeMemf()
		: VfxNodeBase()
		, valueOutput()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Name, kVfxPlugType_String);
		addOutput(kOutput_Value1, kVfxPlugType_Float, &valueOutput[0]);
		addOutput(kOutput_Value2, kVfxPlugType_Float, &valueOutput[1]);
		addOutput(kOutput_Value3, kVfxPlugType_Float, &valueOutput[2]);
		addOutput(kOutput_Value4, kVfxPlugType_Float, &valueOutput[3]);
	}
	
	virtual void tick(const float dt) override
	{
		const char * name = getInputString(kInput_Name, nullptr);
		
		if (name == nullptr)
		{
			valueOutput.SetZero();
		}
		else
		{
			if (g_currentVfxGraph->getMemf(name, valueOutput) == false)
				valueOutput.SetZero();
		}
	}
};

VFX_NODE_TYPE(VfxNodeMemf)
{
	typeName = "in.value";
	
	in("name", "string");
	out("value1", "float");
	out("value2", "float");
	out("value3", "float");
	out("value4", "float");
}

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
	std::string currentOscSheet;
	
	std::vector<InputInfo> inputInfos;
	
	VfxNode4DSoundObject()
		: VfxNodeBase()
		, currentOscPrefix()
		, currentOscSheet()
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
		
		const char * oscSheet = getInputString(kInput_OscSheet, "");
		
		if (s_changedFiles.count(oscSheet) != 0)
		{
			currentOscSheet.clear();
		}
		
		if (oscPrefix == currentOscPrefix && oscSheet == currentOscSheet)
		{
			return;
		}
		
		currentOscPrefix = oscPrefix;
		currentOscSheet = oscSheet;
		
		inputInfos.clear();
		
		if (oscPrefix[0] == 0 || oscSheet[0] == 0)
		{
			setDynamicInputs(nullptr, 0);
		}
		else
		{
			// parse OSC sheet. detect which parameters belong to this sound object. create dynamic inputs
			
			std::vector<VfxNodeBase::DynamicInput> inputs;
			
			CsvDocument csvDocument;
			
			if (csvDocument.load(oscSheet, true, '\t'))
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
					/*
					else if (strstr(type, "enum") != nullptr)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Int;
						input.name = name;
						input.defaultValue = strcmp(defaultValue, "true") == 0 ? "1" : "0";
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultBool = Parse::Bool(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					*/
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

struct VfxGraphInstance
{
	VfxGraph * vfxGraph = nullptr;
	int sx = 0;
	int sy = 0;
	
	GLuint texture = 0;
	
	RealTimeConnection * realTimeConnection = nullptr;
};

struct VfxGraphManager
{
	virtual ~VfxGraphManager()
	{
	}
	
	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) = 0;
	virtual void free(VfxGraphInstance *& instance) = 0;
	
	virtual void tick(const float dt) = 0;
	virtual void tickVisualizers() = 0;
	
	virtual void traverseDraw() const = 0;
};

struct VfxGraphManager_Basic : VfxGraphManager
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	std::vector<VfxGraphInstance*> instances;
	
	VfxGraphManager_Basic(GraphEdit_TypeDefinitionLibrary * in_typeDefinitionLibrary)
		: typeDefinitionLibrary(in_typeDefinitionLibrary)
	{
	}
	
	virtual ~VfxGraphManager_Basic() override
	{
		Assert(instances.empty());
		
		while (!instances.empty())
		{
			auto instance = instances.front();
			
			free(instance);
		}
	}
	
	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) override
	{
		Graph graph;
		graph.load(filename, typeDefinitionLibrary);
		
		VfxGraphInstance * instance = new VfxGraphInstance();
		
		instance->vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
		instance->sx = sx;
		instance->sy = sy;
		
		instances.push_back(instance);
		
		return instance;
	}
	
	virtual void free(VfxGraphInstance *& instance) override
	{
		auto i = std::find(instances.begin(), instances.end(), instance);
		
		if (i != instances.end())
			instances.erase(i);
		
		delete instance;
		instance = nullptr;
	}
	
	virtual void tick(const float dt) override
	{
		for (auto instance : instances)
			instance->vfxGraph->tick(instance->sx, instance->sy, dt);
	}
	
	virtual void tickVisualizers() override
	{
	
	}
	
	virtual void traverseDraw() const override
	{
		for (auto instance : instances)
			instance->texture = instance->vfxGraph->traverseDraw(instance->sx, instance->sy);
	}
};

struct VfxGraphFile
{
	std::string filename;
	
	std::vector<VfxGraphInstance*> instances;
	
	VfxGraphInstance * activeInstance = nullptr;
	
	//RealTimeConnection * realTimeConnection = nullptr;
	
	GraphEdit * graphEdit = nullptr;
};

struct VfxGraphManager_RTE : VfxGraphManager
{
	int displaySx = 0;
	int displaySy = 0;
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	std::map<std::string, VfxGraphFile*> files;
	
	VfxGraphManager_RTE(const int in_displaySx, const int in_displaySy, GraphEdit_TypeDefinitionLibrary * in_typeDefinitionLibrary)
		: displaySx(in_displaySx)
		, displaySy(in_displaySy)
		, typeDefinitionLibrary(in_typeDefinitionLibrary)
	{
	}
	
	virtual ~VfxGraphManager_RTE()
	{
		for (auto & fileItr : files)
		{
			auto & file = fileItr.second;
			Assert(file->instances.empty());
			
			while (!file->instances.empty())
			{
				auto instance = file->instances.front();
				
				free(instance);
			}
			
			delete file;
			file = nullptr;
		}
		
		files.clear();
	}
	
	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) override
	{
		VfxGraphFile * file = nullptr;
		
		auto fileItr = files.find(filename);
		
		if (fileItr != files.end())
		{
			file = fileItr->second;
		}
		else
		{
			file = new VfxGraphFile();
			file->filename = filename;
			
			file->graphEdit = new GraphEdit(displaySx, displaySy, typeDefinitionLibrary);
			//file->graphEdit->realTimeConnection = file->realTimeConnection;
			
			file->graphEdit->load(filename);
			
			files[filename] = file;
		}
	
		//
	
		auto vfxGraph = constructVfxGraph(*file->graphEdit->graph, typeDefinitionLibrary);
		auto realTimeConnection = new RealTimeConnection(vfxGraph);
		
		//
		
		VfxGraphInstance * instance = new VfxGraphInstance();
		instance->vfxGraph = vfxGraph;
		instance->sx = sx;
		instance->sy = sy;
		instance->realTimeConnection = realTimeConnection;
	
		file->instances.push_back(instance);
	
		//
		
		if (file->activeInstance == nullptr)
		{
			file->activeInstance = instance;
		}
	
		return instance;
	}
	
	virtual void free(VfxGraphInstance *& instance) override
	{
		if (instance == nullptr)
		{
			return;
		}
		
		bool found = false;
		
		for (auto fileItr = files.begin(); fileItr != files.end(); )
		{
			auto & file = fileItr->second;
			
			for (auto instanceItr = file->instances.begin(); instanceItr != file->instances.end(); )
			{
				if (*instanceItr == instance)
				{
					found = true;
					
					instanceItr = file->instances.erase(instanceItr);
					
					if (instance == file->activeInstance)
					{
						if (file->instances.empty())
						{
							file->activeInstance = nullptr;
						}
						else
						{
							file->activeInstance = file->instances.front();
						}
					}
				}
				else
				{
					instanceItr++;
				}
			}
			
			fileItr++;
		}
		
		Assert(found);
		
		delete instance;
		instance = nullptr;
	}
	
	virtual void tick(const float dt) override
	{
		for (auto & file_itr : files)
		{
			for (auto & instance : file_itr.second->instances)
			{
				instance->vfxGraph->tick(instance->sx, instance->sy, dt);
			}
		}
	}
	
	virtual void tickVisualizers() override
	{
	
	}
	
	virtual void traverseDraw() const override
	{
		for (auto & file_itr : files)
		{
			for (auto instance : file_itr.second->instances)
			{
				instance->texture = instance->vfxGraph->traverseDraw(instance->sx, instance->sy);
			}
		}
	}
};

static VfxGraphManager * s_vfxGraphMgr = nullptr;

struct Creature
{
	VfxGraphInstance * vfxInstance = nullptr;
	
	Vec2 currentPos;
	Vec2 desiredPos;
	
	void init(const int id)
	{
		desiredPos.Set(random(-10.f, +10.f), random(-10.f, +10.f));
		
		vfxInstance = s_vfxGraphMgr->createInstance("test1.xml", 64, 64);
		
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
			setColor(colorWhite);
			drawRect(
				currentPos[0] - 1.f, currentPos[1] - 1.f,
				currentPos[0] + 1.f, currentPos[1] + 1.f);
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
			creatures[i].init(i + 1);
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
			gxScalef(20, 20, 1);
			
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
#endif
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	framework.realTimeEditCallback = [](const std::string & filename)
	{
		s_changedFiles.insert(filename);
	};
	
// todo : create vfx graph
	VfxGraph * vfxGraph = nullptr;
	
	RealTimeConnection rtc(vfxGraph);
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	
	GraphEdit graphEdit(VIEW_SX, VIEW_SY, &typeDefinitionLibrary, &rtc);
	
	graphEdit.load("test1.xml");
	
	vfxGraph->setMems("id", "1");
	
	//
	
	//VfxGraphManager_Basic vfxGraphMgr(&typeDefinitionLibrary);
	VfxGraphManager_RTE vfxGraphMgr(VIEW_SX, VIEW_SY, &typeDefinitionLibrary);
	s_vfxGraphMgr = &vfxGraphMgr;
	
	//
	
	World world;
	world.init();
	
	while (!framework.quitRequested)
	{
		s_changedFiles.clear();
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		const float dt = framework.timeStep;
		
		// update the graph editor
		
		graphEdit.tick(dt, false);
		
		// update OSC messages
		
		g_oscEndpointMgr.tick();
		
		// update vfx graph
		
		vfxGraph->tick(VIEW_SX, VIEW_SY, dt);
		
		// update the visualizers after the vfx graph has been updated
		
		graphEdit.tickVisualizers(dt);
		
		// update vfx graphs
		
		vfxGraphMgr.tick(dt);
		
		// update world
		
		world.tick(dt);

		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the vfx graph
			
			vfxGraph->draw(VIEW_SX, VIEW_SY);
			
			// draw vfx graphs
			
			vfxGraphMgr.traverseDraw();
			
			// draw world
			
			world.draw();
		
			// draw the graph editor
			
			graphEdit.draw();
		}
		framework.endDraw();
	}
	
	world.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
