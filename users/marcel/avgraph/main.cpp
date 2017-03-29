#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "tinyxml2.h"

using namespace tinyxml2;

/*

todo :
+ replace surface type inputs and outputs to image type
+ add VfxImageBase type. let VfxPlug use this type for image type inputs and outputs. has virtual getTexture method
+ add VfxImage_Surface type. let VfxNodeFsfx use this type
+ add VfxPicture type. type name = 'picture'
+ add VfxImage_Texture type. let VfxPicture use this type
- add VfxVideo type. type name = 'video'
- add default value to socket definitions
- add editorValue to node inputs and outputs. let get*** methods use this value when plug is not connected
- let graph editor set editorValue for nodes. only when editor is set on type definition
+ add socket connection selection. remove connection on BACKSPACE
+ add multiple node selection
- on typing 0..9 let node value editor erase editorValue and begin typing. requires state transition? end editing on ENTER or when selecting another entity
- add ability to increment and decrement editorValue. use mouse Y movement or scroll wheel (?)
- remember number of digits entered after '.' when editing editorValue. use this information when incrementing/decrementing values

*/

#define GFX_SX 1024
#define GFX_SY 768

struct VfxImageBase
{
	VfxImageBase()
	{
	}
	
	virtual ~VfxImageBase()
	{
	}
	
	virtual GLuint getTexture() const = 0;
};

struct VfxImage_Texture : VfxImageBase
{
	GLuint texture;
	
	VfxImage_Texture()
		: VfxImageBase()
		, texture(0)
	{
	}
	
	virtual GLuint getTexture() const override
	{
		return texture;
	}
};

struct VfxImage_Surface : VfxImageBase
{
	Surface * surface;
	
	VfxImage_Surface(Surface * _surface)
		: VfxImageBase()
		, surface(nullptr)
	{
		surface = _surface;
	}
	
	virtual GLuint getTexture() const override
	{
		if (surface)
			return surface->getTexture();
		else
			return 0;
	}
};

enum VfxPlugType
{
	kVfxPlugType_None,
	kVfxPlugType_Int,
	kVfxPlugType_Float,
	kVfxPlugType_String,
	kVfxPlugType_Image,
	kVfxPlugType_Surface
};

struct VfxPlug
{
	VfxPlugType type;
	void * mem;
	
	VfxPlug()
		: type(kVfxPlugType_None)
		, mem(nullptr)
	{
	}
	
	void connectTo(VfxPlug & dst)
	{
		if (dst.type != type)
		{
			logError("node connection failed. type mismatch");
		}
		else
		{
			mem = dst.mem;
		}
	}
	
	bool isConnected() const
	{
		return mem != nullptr;
	}
	
	int getInt() const
	{
		Assert(type == kVfxPlugType_Int);
		return *((int*)mem);
	}
	
	float getFloat() const
	{
		Assert(type == kVfxPlugType_Float);
		return *((float*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(type == kVfxPlugType_String);
		return *((std::string*)mem);
	}
	
	VfxImageBase * getImage() const
	{
		Assert(type == kVfxPlugType_Image);
		return (VfxImageBase*)mem;
	}
	
	Surface * getSurface() const
	{
		Assert(type == kVfxPlugType_Surface);
		return *((Surface**)mem);
	}
};

struct VfxNodeBase
{
	std::vector<VfxPlug> inputs;
	std::vector<VfxPlug> outputs;
	
	std::vector<VfxNodeBase*> predeps;
	
	int lastTraversalId;
	
	VfxNodeBase()
		: inputs()
		, outputs()
		, predeps()
		, lastTraversalId(-1)
	{
	}
	
	virtual ~VfxNodeBase()
	{
	}
	
	void traverse(const int traversalId)
	{
		Assert(lastTraversalId != traversalId);
		lastTraversalId = traversalId;
		
		for (auto predep : predeps)
		{
			if (predep->lastTraversalId != traversalId)
				predep->traverse(traversalId);
		}
		
		draw();
	}
	
	void resizeSockets(const int numInputs, const int numOutputs)
	{
		inputs.resize(numInputs);
		outputs.resize(numOutputs);
	}
	
	void addInput(const int index, VfxPlugType type)
	{
		Assert(index >= 0 && index < inputs.size());
		if (index >= 0 && index < inputs.size())
		{
			inputs[index].type = type;
		}
	}
	
	void addOutput(const int index, VfxPlugType type, void * mem)
	{
		Assert(index >= 0 && index < outputs.size());
		if (index >= 0 && index < outputs.size())
		{
			outputs[index].type = type;
			outputs[index].mem = mem;
		}
	}
	
	VfxPlug * tryGetInput(const int index)
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	VfxPlug * tryGetOutput(const int index)
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	const VfxPlug * tryGetInput(const int index) const
	{
		Assert(index >= 0 && index <= inputs.size());
		if (index < 0 || index >= inputs.size())
			return nullptr;
		else
			return &inputs[index];
	}
	
	const VfxPlug * tryGetOutput(const int index) const
	{
		Assert(index >= 0 && index <= outputs.size());
		if (index < 0 || index >= outputs.size())
			return nullptr;
		else
			return &outputs[index];
	}
	
	int getInputInt(const int index, const int defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getInt();
	}
	
	float getInputFloat(const int index, const float defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getFloat();
	}
	
	const char * getInputString(const int index, const char * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getString().c_str();
	}
	
	const VfxImageBase * getInputImage(const int index, const VfxImageBase * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getImage();
	}
	
	const Surface * getInputSurface(const int index, const Surface * defaultValue) const
	{
		const VfxPlug * plug = tryGetInput(index);
		
		if (plug == nullptr || !plug->isConnected())
			return defaultValue;
		else
			return plug->getSurface();
	}
	
	virtual void initSelf(const GraphNode & node) { }
	virtual void init(const GraphNode & node) { }
	virtual void tick(const float dt) { }
	virtual void draw() const { }
};

struct VfxNodeDisplay : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_COUNT
	};
	
	VfxNodeDisplay()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, 0);
		addInput(kInput_Image, kVfxPlugType_Image);
	}
	
	const VfxImageBase * getImage() const
	{
		return getInputImage(kInput_Image, nullptr);
	}
	
	virtual void draw() const override
	{
	#if 0
		const VfxImageBase * image = getImage();
		
		if (image != nullptr)
		{
			gxSetTexture(image->getTexture());
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			drawRect(0, 0, GFX_SX, GFX_SY);
			popBlend();
			gxSetTexture(0);
		}
	#endif
	}
};

struct VfxNodePicture : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture * image;
	
	VfxNodePicture()
		: VfxNodeBase()
		, image(nullptr)
	{
		image = new VfxImage_Texture();
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Source, kVfxPlugType_String);
		addOutput(kOutput_Image, kVfxPlugType_Image, image);
	}
	
	virtual ~VfxNodePicture()
	{
		delete image;
		image = nullptr;
	}
	
	virtual void init(const GraphNode & node) override
	{
		const char * filename = getInputString(kInput_Source, nullptr);
		
		if (filename == nullptr)
		{
			image->texture = 0;
		}
		else
		{
			image->texture = getTexture(filename);
		}
	}
};

struct VfxNodeFsfx : VfxNodeBase
{
	enum Input
	{
		kInput_Image,
		kInput_Shader,
		kInput_Param1,
		kInput_Param2,
		kInput_Param3,
		kInput_Param4,
		kInput_Opacity,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	Surface * surface;
	
	VfxImage_Texture * image;
	
	bool isPassthrough;
	
	VfxNodeFsfx()
		: VfxNodeBase()
		, surface(nullptr)
		, image(nullptr)
		, isPassthrough(false)
	{
		surface = new Surface(GFX_SX, GFX_SY, true);
		
		surface->clear();
		surface->swapBuffers();
		surface->clear();
		surface->swapBuffers();
		
		image = new VfxImage_Texture();
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Image, kVfxPlugType_Image);
		addInput(kInput_Shader, kVfxPlugType_String);
		addInput(kInput_Param1, kVfxPlugType_Float);
		addInput(kInput_Param2, kVfxPlugType_Float);
		addInput(kInput_Param3, kVfxPlugType_Float);
		addInput(kInput_Param4, kVfxPlugType_Float);
		addInput(kInput_Opacity, kVfxPlugType_Float);
		addOutput(kOutput_Image, kVfxPlugType_Image, image);
	}
	
	virtual ~VfxNodeFsfx() override
	{
		delete image;
		image = nullptr;
		
		delete surface;
		surface = nullptr;
	}
	
	virtual void init(const GraphNode & node) override
	{
		isPassthrough = node.editorIsPassthrough;
	}
	
	virtual void draw() const override
	{
		if (isPassthrough)
		{
			const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
			const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
			image->texture = inputTexture;
			return;
		}
		
		const std::string & shaderName = getInputString(kInput_Shader, "");
		
		if (shaderName.empty())
		{
			// todo : warn ?
		}
		else
		{
			const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
			const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
			Shader shader(shaderName.c_str());
			
			if (shader.isValid())
			{
				pushBlend(BLEND_OPAQUE);
				setShader(shader);
				{
					shader.setTexture("colormap", 0, inputTexture);
					shader.setImmediate("param1", getInputFloat(kInput_Param1, 0.f));
					shader.setImmediate("param2", getInputFloat(kInput_Param2, 0.f));
					shader.setImmediate("param3", getInputFloat(kInput_Param3, 0.f));
					shader.setImmediate("param4", getInputFloat(kInput_Param4, 0.f));
					shader.setImmediate("opacity", getInputFloat(kInput_Opacity, 1.f));
					shader.setImmediate("time", framework.time);
					surface->postprocess();
				}
				clearShader();
				popBlend();
			}
			else
			{
				pushSurface(surface);
				{
					if (inputImage == nullptr)
					{
						pushBlend(BLEND_OPAQUE);
						setColor(0, 127, 0);
						drawRect(0, 0, GFX_SX, GFX_SY);
						popBlend();
					}
					else
					{
						pushBlend(BLEND_OPAQUE);
						gxSetTexture(inputTexture);
						setColorMode(COLOR_ADD);
						setColor(31, 0, 31);
						drawRect(0, 0, GFX_SX, GFX_SY);
						setColorMode(COLOR_MUL);
						gxSetTexture(0);
						popBlend();
					}
				}
				popSurface();
			}
		}
		
		image->texture = surface->getTexture();
	}
};

struct VfxNodeIntLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	int value;
	
	VfxNodeIntLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Int, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Int32(node.editorValue);
	}
};

struct VfxNodeFloatLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float value;
	
	VfxNodeFloatLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Float(node.editorValue);
	}
};

struct VfxNodeStringLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	std::string value;
	
	VfxNodeStringLiteral()
		: VfxNodeBase()
		, value()
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_String, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = node.editorValue;
	}
};

struct VfxNodeMouse : VfxNodeBase
{
	enum Output
	{
		kOutput_X,
		kOutput_Y,
		kOutput_ButtonLeft,
		kOutput_ButtonRight,
		kOutput_COUNT
	};
	
	float x;
	float y;
	float buttonLeft;
	float buttonRight;
	
	VfxNodeMouse()
		: x(0.f)
		, y(0.f)
		, buttonLeft(0.f)
		, buttonRight(0.f)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_X, kVfxPlugType_Float, &x);
		addOutput(kOutput_Y, kVfxPlugType_Float, &y);
		addOutput(kOutput_ButtonLeft, kVfxPlugType_Float, &buttonLeft);
		addOutput(kOutput_ButtonRight, kVfxPlugType_Float, &buttonRight);
	}
	
	virtual void tick(const float dt)
	{
		x = mouse.x / float(GFX_SX);
		y = mouse.y / float(GFX_SY);
		buttonLeft = mouse.isDown(BUTTON_LEFT) ? 1.f : 0.f;
		buttonRight = mouse.isDown(BUTTON_RIGHT) ? 1.f : 0.f;
	}
};

struct VfxGraph
{
	std::map<GraphNodeId, VfxNodeBase*> nodes;
	
	GraphNodeId displayNodeId;
	
	Graph * graph; // todo : remove ?
	
	VfxGraph()
		: nodes()
		, displayNodeId(kGraphNodeIdInvalid)
		, graph(nullptr)
	{
	}
	
	~VfxGraph()
	{
		destroy();
	}
	
	void destroy()
	{
		graph = nullptr;
		
		displayNodeId = kGraphNodeIdInvalid;
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			delete node;
			node = nullptr;
		}
		
		nodes.clear();
	}
	
	void tick(const float dt)
	{
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			node->tick(dt);
		}
	}
	
	void draw() const
	{
		static int traversalId = 0; // fixme
		traversalId++;
		
		// todo : start at output to screen and traverse to leafs and back up again to draw
		
		if (displayNodeId != kGraphNodeIdInvalid)
		{
			auto nodeItr = nodes.find(displayNodeId);
			Assert(nodeItr != nodes.end());
			if (nodeItr != nodes.end())
			{
				auto node = nodeItr->second;
				
				VfxNodeDisplay * displayNode = static_cast<VfxNodeDisplay*>(node);
				
				displayNode->traverse(traversalId);
				
				const VfxImageBase * image = displayNode->getImage();
				
				if (image != nullptr)
				{
					gxSetTexture(image->getTexture());
					pushBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					popBlend();
					gxSetTexture(0);
				}
			}
		}
		
		for (auto i : nodes)
		{
			VfxNodeBase * node = i.second;
			
			node->draw();
		}
	}
};

static VfxGraph * constructVfxGraph(const Graph & graph)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		VfxNodeBase * vfxNode = nullptr;
		
		if (node.typeName == "intLiteral")
		{
			vfxNode= new VfxNodeIntLiteral();
		}
		else if (node.typeName == "floatLiteral")
		{
			vfxNode = new VfxNodeFloatLiteral();
		}
		else if (node.typeName == "stringLiteral")
		{
			vfxNode = new VfxNodeStringLiteral();
		}
		else if (node.typeName == "display")
		{
			auto vfxDisplayNode = new VfxNodeDisplay();
			
			Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
			vfxGraph->displayNodeId = node.id;
			
			vfxNode = vfxDisplayNode;
		}
		else if (node.typeName == "mouse")
		{
			vfxNode = new VfxNodeMouse();
		}
		else if (node.typeName == "picture")
		{
			vfxNode = new VfxNodePicture();
		}
		else if (node.typeName == "fsfx")
		{
			vfxNode = new VfxNodeFsfx();
		}
		else
		{
			logError("unknown node type: %s", node.typeName.c_str());
		}
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		auto srcNodeItr = vfxGraph->nodes.find(link.srcNodeId);
		auto dstNodeItr = vfxGraph->nodes.find(link.dstNodeId);
		
		Assert(srcNodeItr != vfxGraph->nodes.end() && dstNodeItr != vfxGraph->nodes.end());
		if (srcNodeItr == vfxGraph->nodes.end() || dstNodeItr == vfxGraph->nodes.end())
		{
			if (srcNodeItr == vfxGraph->nodes.end())
				logError("source node doesn't exist");
			if (dstNodeItr == vfxGraph->nodes.end())
				logError("destination node doesn't exist");
		}
		else
		{
			auto srcNode = srcNodeItr->second;
			auto dstNode = dstNodeItr->second;
			
			auto input = srcNode->tryGetInput(link.srcNodeSocketIndex);
			auto output = dstNode->tryGetOutput(link.dstNodeSocketIndex);
			
			Assert(input != nullptr && output != nullptr);
			if (input == nullptr || output == nullptr)
			{
				if (input == nullptr)
					logError("input node socket doesn't exist");
				if (output == nullptr)
					logError("output node socket doesn't exist");
			}
			else
			{
				input->connectTo(*output);
				
				srcNode->predeps.push_back(dstNode);
			}
		}
	}
	
	for (auto vfxNodeItr : vfxGraph->nodes)
	{
		auto nodeId = vfxNodeItr.first;
		auto nodeItr = graph.nodes.find(nodeId);
		auto & node = nodeItr->second;
		auto vfxNode = vfxNodeItr.second;
		
		vfxNode->init(node);
	}
	
	return vfxGraph;
}

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		{
			XMLDocument * document = new XMLDocument();
			
			if (document->LoadFile("types.xml") == XML_SUCCESS)
			{
				const XMLElement * xmlLibrary = document->FirstChildElement("library");
				
				if (xmlLibrary != nullptr)
				{
					typeDefinitionLibrary->loadXml(xmlLibrary);
				}
			}
			
			delete document;
			document = nullptr;
		}
		
		//
		
		GraphEdit * graphEdit = new GraphEdit();
		
		graphEdit->typeDefinitionLibrary = typeDefinitionLibrary;
		
		VfxGraph * vfxGraph = nullptr;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			graphEdit->tick(dt);
			
			if (keyboard.wentDown(SDLK_s))
			{
				FILE * file = fopen("graph.xml", "wt");
				
				XMLPrinter xmlGraph(file);;
				
				xmlGraph.OpenElement("graph");
				{
					graphEdit->graph->saveXml(xmlGraph);
				}
				xmlGraph.CloseElement();
				
				fclose(file);
				file = nullptr;
			}
			
			if (keyboard.wentDown(SDLK_l))
			{
				delete graphEdit->graph;
				graphEdit->graph = nullptr;
				
				//
				
				graphEdit->graph = new Graph();
				
				//
				
				XMLDocument document;
				document.LoadFile("graph.xml");
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				if (xmlGraph != nullptr)
					graphEdit->graph->loadXml(xmlGraph);
				
				//
				
				delete vfxGraph;
				vfxGraph = nullptr;
				
				vfxGraph = constructVfxGraph(*graphEdit->graph);
				
			#if 0
				delete vfxGraph;
				vfxGraph = nullptr;
			#endif
			}
			
			framework.beginDraw(31, 31, 31, 255);
			{
				if (vfxGraph != nullptr)
				{
					vfxGraph->tick(framework.timeStep);
					vfxGraph->draw();
				}
				
				graphEdit->draw();
			}
			framework.endDraw();
		}
		
		delete graphEdit;
		graphEdit = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
