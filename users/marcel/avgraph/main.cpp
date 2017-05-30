#include "Calc.h"
#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"

#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxTypes.h"

#include "vfxNodes/vfxNodeBase.h"
#include "vfxNodes/vfxNodeBinaryOutput.h"
#include "vfxNodes/vfxNodeComposite.h"
#include "vfxNodes/vfxNodeDelayLine.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxNodes/vfxNodeDotDetector.h"
#include "vfxNodes/vfxNodeFsfx.h"
#include "vfxNodes/vfxNodeImageCpuDownsample.h"
#include "vfxNodes/vfxNodeImageCpuEqualize.h"
#include "vfxNodes/vfxNodeImageCpuToGpu.h"
#include "vfxNodes/vfxNodeImageDownsample.h"
#include "vfxNodes/vfxNodeImpulseResponse.h"
#include "vfxNodes/vfxNodeKinect1.h"
#include "vfxNodes/vfxNodeKinect2.h"
#include "vfxNodes/vfxNodeLeapMotion.h"
#include "vfxNodes/vfxNodeLiteral.h"
#include "vfxNodes/vfxNodeLogicSwitch.h"
#include "vfxNodes/vfxNodeMapEase.h"
#include "vfxNodes/vfxNodeMapRange.h"
#include "vfxNodes/vfxNodeMath.h"
#include "vfxNodes/vfxNodeMouse.h"
#include "vfxNodes/vfxNodeNoiseSimplex2D.h"
#include "vfxNodes/vfxNodeOsc.h"
#include "vfxNodes/vfxNodeOscPrimitives.h"
#include "vfxNodes/vfxNodeOscSend.h"
#include "vfxNodes/vfxNodePhysicalSpring.h"
#include "vfxNodes/vfxNodePicture.h"
#include "vfxNodes/vfxNodePictureCpu.h"
#include "vfxNodes/vfxNodeSampleAndHold.h"
#include "vfxNodes/vfxNodeSound.h"
#include "vfxNodes/vfxNodeSpectrum1D.h"
#include "vfxNodes/vfxNodeSpectrum2D.h"
#include "vfxNodes/vfxNodeTime.h"
#include "vfxNodes/vfxNodeTransform2D.h"
#include "vfxNodes/vfxNodeTriggerOnchange.h"
#include "vfxNodes/vfxNodeTriggerTimer.h"
#include "vfxNodes/vfxNodeTriggerTreshold.h"
#include "vfxNodes/vfxNodeTouches.h"
#include "vfxNodes/vfxNodeVideo.h"
#include "vfxNodes/vfxNodeXinput.h"
#include "vfxNodes/vfxNodeYuvToRgb.h"

#include "mediaplayer_new/MPUtil.h"
#include "../libparticle/ui.h"

using namespace tinyxml2;

//#define FILENAME "kinect.xml"
#define FILENAME "yuvtest.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testDatGui();
extern void testNanovg();
extern void testFourier1d();
extern void testFourier2d();
extern void testChaosGame();
extern void testImpulseResponseMeasurement();
extern void testTextureAtlas();
extern void testDynamicTextureAtlas();
extern void testDotDetector();

struct VfxNodeTriggerAsFloat : VfxNodeBase
{
	enum Input
	{
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float outputValue;
	
	VfxNodeTriggerAsFloat()
		: VfxNodeBase()
		, outputValue(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Trigger, kVfxPlugType_Trigger);
		addOutput(kOutput_Value, kVfxPlugType_Float, &outputValue);
	}
	
	virtual void tick(const float dt) override
	{
		const VfxTriggerData * triggerData = getInputTriggerData(kInput_Trigger);
		
		if (triggerData != nullptr)
			outputValue = triggerData->asFloat();
		else
			outputValue = 0.f;
	}
};

VfxNodeBase * createVfxNode(const GraphNodeId nodeId, const std::string & typeName, VfxGraph * vfxGraph)
{
	VfxNodeBase * vfxNode = nullptr;
	
#define DefineNodeImpl(_typeName, _type) \
	else if (typeName == _typeName) \
		vfxNode = new _type();
	
	if (false)
	{
	}
	DefineNodeImpl("intBool", VfxNodeBoolLiteral)
	DefineNodeImpl("intLiteral", VfxNodeIntLiteral)
	DefineNodeImpl("floatLiteral", VfxNodeFloatLiteral)
	DefineNodeImpl("transformLiteral", VfxNodeTransformLiteral)
	DefineNodeImpl("stringLiteral", VfxNodeStringLiteral)
	DefineNodeImpl("colorLiteral", VfxNodeColorLiteral)
	DefineNodeImpl("trigger.asFloat", VfxNodeTriggerAsFloat)
	DefineNodeImpl("time", VfxNodeTime)
	DefineNodeImpl("sampleAndHold", VfxNodeSampleAndHold)
	DefineNodeImpl("binary.output", VfxNodeBinaryOutput)
	DefineNodeImpl("transform.2d", VfxNodeTransform2D)
	DefineNodeImpl("trigger.onchange", VfxNodeTriggerOnchange)
	DefineNodeImpl("trigger.timer", VfxNodeTriggerTimer)
	DefineNodeImpl("trigger.treshold", VfxNodeTriggerTreshold)
	DefineNodeImpl("logic.switch", VfxNodeLogicSwitch)
	DefineNodeImpl("noise.simplex2d", VfxNodeNoiseSimplex2D)
	DefineNodeImpl("sample.delay", VfxNodeDelayLine)
	DefineNodeImpl("impulse.response", VfxNodeImpulseResponse)
	DefineNodeImpl("physical.spring", VfxNodePhysicalSpring)
	DefineNodeImpl("map.range", VfxNodeMapRange)
	DefineNodeImpl("ease", VfxNodeMapEase)
	DefineNodeImpl("math.add", VfxNodeMathAdd)
	DefineNodeImpl("math.sub", VfxNodeMathSub)
	DefineNodeImpl("math.mul", VfxNodeMathMul)
	DefineNodeImpl("math.sin", VfxNodeMathSin)
	DefineNodeImpl("math.cos", VfxNodeMathCos)
	DefineNodeImpl("math.abs", VfxNodeMathAbs)
	DefineNodeImpl("math.min", VfxNodeMathMin)
	DefineNodeImpl("math.max", VfxNodeMathMax)
	DefineNodeImpl("math.sat", VfxNodeMathSat)
	DefineNodeImpl("math.neg", VfxNodeMathNeg)
	DefineNodeImpl("math.sqrt", VfxNodeMathSqrt)
	DefineNodeImpl("math.pow", VfxNodeMathPow)
	DefineNodeImpl("math.exp", VfxNodeMathExp)
	DefineNodeImpl("math.mod", VfxNodeMathMod)
	DefineNodeImpl("math.fract", VfxNodeMathFract)
	DefineNodeImpl("math.floor", VfxNodeMathFloor)
	DefineNodeImpl("math.ceil", VfxNodeMathCeil)
	DefineNodeImpl("math.round", VfxNodeMathRound)
	DefineNodeImpl("math.sign", VfxNodeMathSign)
	DefineNodeImpl("math.hypot", VfxNodeMathHypot)
	DefineNodeImpl("math.pitch", VfxNodeMathPitch)
	DefineNodeImpl("math.semitone", VfxNodeMathSemitone)
	DefineNodeImpl("osc.sine", VfxNodeOscSine)
	DefineNodeImpl("osc.saw", VfxNodeOscSaw)
	DefineNodeImpl("osc.triangle", VfxNodeOscTriangle)
	DefineNodeImpl("osc.square", VfxNodeOscSquare)
	DefineNodeImpl("osc.random", VfxNodeOscRandom)
	else if (typeName == "display")
	{
		vfxNode = new VfxNodeDisplay();
		
		// fixme : move display node id handling out of here. remove nodeId and vfxGraph passed in to this function
		Assert(vfxGraph->displayNodeId == kGraphNodeIdInvalid);
		vfxGraph->displayNodeId = nodeId;
	}
	DefineNodeImpl("mouse", VfxNodeMouse)
	DefineNodeImpl("xinput", VfxNodeXinput)
	DefineNodeImpl("touches", VfxNodeTouches)
	DefineNodeImpl("leap", VfxNodeLeapMotion)
	DefineNodeImpl("kinect1", VfxNodeKinect1)
	DefineNodeImpl("kinect2", VfxNodeKinect2)
	DefineNodeImpl("osc", VfxNodeOsc)
	DefineNodeImpl("osc.send", VfxNodeOscSend)
	DefineNodeImpl("composite", VfxNodeComposite)
	DefineNodeImpl("picture", VfxNodePicture)
	DefineNodeImpl("picture.cpu", VfxNodePictureCpu)
	DefineNodeImpl("video", VfxNodeVideo)
	DefineNodeImpl("sound", VfxNodeSound)
	DefineNodeImpl("spectrum.1d", VfxNodeSpectrum1D)
	DefineNodeImpl("spectrum.2d", VfxNodeSpectrum2D)
	DefineNodeImpl("fsfx", VfxNodeFsfx)
	DefineNodeImpl("image.dots", VfxNodeDotDetector)
	DefineNodeImpl("image.toGpu", VfxNodeImageCpuToGpu)
	DefineNodeImpl("image_cpu.equalize", VfxNodeImageCpuEqualize)
	DefineNodeImpl("image_cpu.downsample", VfxNodeImageCpuDownsample)
	DefineNodeImpl("image.downsample", VfxNodeImageDownsample)
	DefineNodeImpl("yuvToRgb", VfxNodeYuvToRgb)
	else
	{
		logError("unknown node type: %s", typeName.c_str());
	}
	
#undef DefineNodeImpl

	return vfxNode;
}

VfxGraph * constructVfxGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary)
{
	VfxGraph * vfxGraph = new VfxGraph();
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		if (node.nodeType != kGraphNodeType_Regular)
		{
			continue;
		}
		
		if (node.isEnabled == false)
		{
			continue;
		}
		
		VfxNodeBase * vfxNode = createVfxNode(node.id, node.typeName, vfxGraph);
		
		Assert(vfxNode != nullptr);
		if (vfxNode == nullptr)
		{
			logError("unable to create node");
		}
		else
		{
			vfxNode->isPassthrough = node.isPassthrough;
			
			vfxNode->initSelf(node);
			
			vfxGraph->nodes[node.id] = vfxNode;
		}
	}
	
	for (auto & linkItr : graph.links)
	{
		auto & link = linkItr.second;
		
		if (link.isEnabled == false)
		{
			continue;
		}
		
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
				
				// note : this may add the same node multiple times to the list of predeps. note that this
				//        is ok as nodes will be traversed once through the travel id + it works nicely
				//        with the live connection as we can just remove the predep and still have one or
				//        references to the predep if the predep was referenced more than once
				srcNode->predeps.push_back(dstNode);
				
				// if this is a trigger, add a trigger target to dstNode
				if (output->type == kVfxPlugType_Trigger)
				{
					VfxNodeBase::TriggerTarget triggerTarget;
					triggerTarget.srcNode = srcNode;
					triggerTarget.srcSocketIndex = link.srcNodeSocketIndex;
					triggerTarget.dstSocketIndex = link.dstNodeSocketIndex;
					
					dstNode->triggerTargets.push_back(triggerTarget);
				}
			}
		}
	}
	
	for (auto nodeItr : graph.nodes)
	{
		auto & node = nodeItr.second;
		
		auto typeDefintion = typeDefinitionLibrary->tryGetTypeDefinition(node.typeName);
		
		if (typeDefintion == nullptr)
			continue;
		
		auto vfxNodeItr = vfxGraph->nodes.find(node.id);
		
		if (vfxNodeItr == vfxGraph->nodes.end())
			continue;
		
		VfxNodeBase * vfxNode = vfxNodeItr->second;
		
		auto & vfxNodeInputs = vfxNode->inputs;
		
		for (auto inputValueItr : node.editorInputValues)
		{
			const std::string & inputName = inputValueItr.first;
			const std::string & inputValue = inputValueItr.second;
			
			for (size_t i = 0; i < typeDefintion->inputSockets.size(); ++i)
			{
				if (typeDefintion->inputSockets[i].name == inputName)
				{
					if (i < vfxNodeInputs.size())
					{
						if (vfxNodeInputs[i].isConnected() == false)
						{
							vfxGraph->connectToInputLiteral(vfxNodeInputs[i], inputValue);
						}
					}
				}
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

//

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();
		
		//testDatGui();
		
		//testNanovg();
		
		//testChaosGame();
		
		//testFourier1d();
		
		//testFourier2d();
		
		//testImpulseResponseMeasurement();
		
		//testTextureAtlas();
		
		//testDynamicTextureAtlas();
		
		//testDotDetector();
		
		//
		
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
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection();
		
		//
		
		GraphEdit * graphEdit = new GraphEdit(typeDefinitionLibrary);
		
		graphEdit->realTimeConnection = realTimeConnection;

		VfxGraph * vfxGraph = new VfxGraph();
		
		realTimeConnection->vfxGraph = vfxGraph;
		realTimeConnection->vfxGraphPtr = &vfxGraph;
		
		//
		
		graphEdit->load(FILENAME);
		
		//
		
		while (!framework.quitRequested)
		{
			if (graphEdit->editorOptions.realTimePreview)
				framework.waitForEvents = false;
			else
			{
				framework.waitForEvents = true;
			}
			
			framework.process();
			
			if (!graphEdit->editorOptions.realTimePreview)
			{
				framework.timeStep = std::min(framework.timeStep, 1.f / 15.f);
			}
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			g_currentVfxGraph = realTimeConnection->vfxGraph;
			
			if (graphEdit->tick(dt))
			{
			}
			else if (keyboard.wentDown(SDLK_s))
			{
				graphEdit->save(FILENAME);
			}
			else if (keyboard.wentDown(SDLK_l))
			{
				graphEdit->load(FILENAME);
			}
			
			g_currentVfxGraph = nullptr;
			
			// fixme : this should be handled by graph edit
			
			if (!graphEdit->selectedNodes.empty())
			{
				const GraphNodeId nodeId = *graphEdit->selectedNodes.begin();
				
				graphEdit->propertyEditor->setNode(nodeId);
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
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete typeDefinitionLibrary;
		typeDefinitionLibrary = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
