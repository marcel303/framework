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
			
			ReadOnlyCsvDocument csvDocument;
			
			if (csvDocument.load(oscSheet, true, '\t'))
			{
				const int oscAddress_index = csvDocument.getColumnIndex("OSC Address");
				const int type_index = csvDocument.getColumnIndex("Type");
				const int defaultValue_index = csvDocument.getColumnIndex("Default value");
				
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
	
	~VfxGraphInstance()
	{
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete vfxGraph;
		vfxGraph = nullptr;
	}
};

struct VfxGraphManager
{
	virtual ~VfxGraphManager()
	{
	}
	
	virtual void selectFile(const char * filename) = 0;
	virtual void selectInstance(const VfxGraphInstance * instance) = 0;

	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) = 0;
	virtual void free(VfxGraphInstance *& instance) = 0;
	
	virtual void tick(const float dt) = 0;
	virtual void tickVisualizers(const float dt) = 0;
	
	virtual void traverseDraw() const = 0;
	
	virtual bool tickEditor(const float dt, const bool isInputCaptured) = 0;
	virtual void drawEditor() = 0;
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
	
	virtual void selectFile(const char * filename) override
	{
	}
	
	virtual void selectInstance(const VfxGraphInstance * instance) override
	{
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
	
	virtual void tickVisualizers(const float dt) override
	{
	}
	
	virtual void traverseDraw() const override
	{
		for (auto instance : instances)
			instance->texture = instance->vfxGraph->traverseDraw(instance->sx, instance->sy);
	}
	
	virtual bool tickEditor(const float dt, const bool isInputCaptured) override
	{
		return false;
	}
	
	virtual void drawEditor() override
	{
	}
};

struct VfxGraphFileRTC;

struct VfxGraphFile
{
	std::string filename;
	
	std::vector<VfxGraphInstance*> instances;
	
	const VfxGraphInstance * activeInstance = nullptr;
	
	VfxGraphFileRTC * realTimeConnection = nullptr;
	
	GraphEdit * graphEdit = nullptr;
	
	VfxGraphFile();
};

struct VfxGraphFileRTC : GraphEdit_RealTimeConnection
{
	VfxGraphFile * file = nullptr;
	
	virtual void loadBegin() override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->loadBegin();
	}

	virtual void loadEnd(GraphEdit & graphEdit) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->loadEnd(graphEdit);
	}
	
	virtual void nodeAdd(const GraphNodeId nodeId, const std::string & typeName) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->nodeAdd(nodeId, typeName);
	}

	virtual void nodeRemove(const GraphNodeId nodeId) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->nodeRemove(nodeId);
	}

	virtual void linkAdd(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->linkAdd(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual void linkRemove(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->linkRemove(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}
	
	virtual void setLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setLinkParameter(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex, name, value);
	}
	
	virtual void clearLinkParameter(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex, const std::string & name) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->clearLinkParameter(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex, name);
	}
	
	virtual void setNodeIsPassthrough(const GraphNodeId nodeId, const bool isPassthrough) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setNodeIsPassthrough(nodeId, isPassthrough);
	}
	
	virtual void setSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual bool getSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketValue(nodeId, srcSocketIndex, srcSocketName, value);
	}

	virtual void setDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, const std::string & value) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->setDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}

	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketValue(nodeId, dstSocketIndex, dstSocketName, value);
	}
	
	virtual void clearSrcSocketValue(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		for (auto & instance : file->instances)
			instance->realTimeConnection->clearSrcSocketValue(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getSrcSocketChannelData(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getSrcSocketChannelData(nodeId, srcSocketIndex, srcSocketName, channels);
	}

	virtual bool getDstSocketChannelData(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, GraphEdit_ChannelData & channels) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getDstSocketChannelData(nodeId, dstSocketIndex, dstSocketName, channels);
	}
	
	virtual void handleSrcSocketPressed(const GraphNodeId nodeId, const int srcSocketIndex, const std::string & srcSocketName) override
	{
		if (file->activeInstance != nullptr)
			file->activeInstance->realTimeConnection->handleSrcSocketPressed(nodeId, srcSocketIndex, srcSocketName);
	}
	
	virtual bool getNodeDescription(const GraphNodeId nodeId, std::vector<std::string> & lines) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeDescription(nodeId, lines);
	}
	
	virtual bool getNodeIssues(const GraphNodeId nodeId, std::vector<std::string> & issues) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeIssues(nodeId, issues);
	}
	
	virtual int getNodeActivity(const GraphNodeId nodeId) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeActivity(nodeId);
	}

	virtual int getLinkActivity(const GraphLinkId linkId, const GraphNodeId srcNodeId, const int srcSocketIndex, const GraphNodeId dstNodeId, const int dstSocketIndex) override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getLinkActivity(linkId, srcNodeId, srcSocketIndex, dstNodeId, dstSocketIndex);
	}

	virtual bool getNodeDynamicSockets(const GraphNodeId nodeId, std::vector<DynamicInput> & inputs, std::vector<DynamicOutput> & outputs) const override
	{
		if (file->activeInstance == nullptr)
			return false;
		else
			return file->activeInstance->realTimeConnection->getNodeDynamicSockets(nodeId, inputs, outputs);
	}
	
	virtual int getNodeCpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 1000 * 1000;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuHeatMax();
	}

	virtual int getNodeCpuTimeUs(const GraphNodeId nodeId) const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeCpuTimeUs(nodeId);
	}
	
	virtual int getNodeGpuHeatMax() const override
	{
		if (file->activeInstance == nullptr)
			return 1000 * 1000;
		else
			return file->activeInstance->realTimeConnection->getNodeGpuHeatMax();
	}
	
	virtual int getNodeGpuTimeUs(const GraphNodeId nodeId) const override
	{
		if (file->activeInstance == nullptr)
			return 0;
		else
			return file->activeInstance->realTimeConnection->getNodeGpuTimeUs(nodeId);
	}
};

VfxGraphFile::VfxGraphFile()
{
	realTimeConnection = new VfxGraphFileRTC();
	realTimeConnection->file = this;
}

struct VfxGraphManager_RTE : VfxGraphManager
{
	int displaySx = 0;
	int displaySy = 0;
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	std::map<std::string, VfxGraphFile*> files;
	
	VfxGraphFile * selectedFile = nullptr;
	
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
	
	virtual void selectFile(const char * filename) override
	{
		VfxGraphFile * newSelectedFile = nullptr;
		
		if (filename != nullptr)
		{
			auto fileItr = files.find(filename);
			
			if (fileItr != files.end())
			{
				newSelectedFile = fileItr->second;
			}
		}
		
		if (newSelectedFile != selectedFile)
		{
			if (selectedFile != nullptr)
			{
				selectedFile->graphEdit->cancelEditing();
				
				selectedFile = nullptr;
			}
			
			//
			
			selectedFile = newSelectedFile;
			
			selectedFile->graphEdit->beginEditing();
		}
	}

	virtual void selectInstance(const VfxGraphInstance * instance) override
	{
		if (instance == nullptr)
		{
			selectFile(nullptr);
		}
		else
		{
			for (auto & fileItr : files)
			{
				auto file = fileItr.second;
				
				for (auto & instanceInFile : file->instances)
				{
					if (instance == instanceInFile)
					{
						selectFile(fileItr.first.c_str());
						
						file->activeInstance = instance;
					}
				}
			}
		}
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
			file->graphEdit->realTimeConnection = file->realTimeConnection;
			
			file->graphEdit->load(filename);
			
			files[filename] = file;
		}
	
		//
	
		auto vfxGraph = constructVfxGraph(*file->graphEdit->graph, typeDefinitionLibrary);
		
		VfxGraphInstance * instance = new VfxGraphInstance();
		instance->vfxGraph = vfxGraph;
		instance->sx = sx;
		instance->sy = sy;
		
		auto realTimeConnection = new RealTimeConnection(instance->vfxGraph);
		instance->realTimeConnection = realTimeConnection;
		
		//
	
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
	
	virtual void tickVisualizers(const float dt) override
	{
		if (selectedFile != nullptr)
		{
			selectedFile->graphEdit->tickVisualizers(dt);
		}
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
	
	virtual bool tickEditor(const float dt, const bool isInputCaptured) override
	{
		bool result = false;
		
		if (selectedFile != nullptr)
		{
			result |= selectedFile->graphEdit->tick(dt, isInputCaptured);
		}
		
		return result;
	}

	virtual void drawEditor() override
	{
		if (selectedFile != nullptr)
		{
			selectedFile->graphEdit->draw();
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
	
	// create vfx type definition library
	
	GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	
	// create vfx graph manager
	
	//VfxGraphManager_Basic vfxGraphMgr(&typeDefinitionLibrary);
	VfxGraphManager_RTE vfxGraphMgr(VIEW_SX, VIEW_SY, &typeDefinitionLibrary);
	s_vfxGraphMgr = &vfxGraphMgr;
	
	// setup world
	
	World world;
	world.init();
	
	// select instance for editing
	
	vfxGraphMgr.selectInstance(world.creatures[0].vfxInstance);
	
	while (!framework.quitRequested)
	{
		s_changedFiles.clear();
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		const float dt = framework.timeStep;
		
		// update the graph editor
		
		vfxGraphMgr.tickEditor(dt, false);
		
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
		}
		framework.endDraw();
	}
	
	world.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
