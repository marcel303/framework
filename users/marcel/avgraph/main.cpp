/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

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
#include "vfxNodes/vfxNodeChannelSelect.h"
#include "vfxNodes/vfxNodeChannelSlice.h"
#include "vfxNodes/vfxNodeChannelToGpu.h"
#include "vfxNodes/vfxNodeComposite.h"
#include "vfxNodes/vfxNodeDeepbelief.h"
#include "vfxNodes/vfxNodeDelayLine.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "vfxNodes/vfxNodeDotDetector.h"
#include "vfxNodes/vfxNodeDotTracker.h"
#include "vfxNodes/vfxNodeFsfx.h"
#include "vfxNodes/vfxNodeImageCpuDelayLine.h"
#include "vfxNodes/vfxNodeImageCpuDownsample.h"
#include "vfxNodes/vfxNodeImageCpuEqualize.h"
#include "vfxNodes/vfxNodeImageCpuToGpu.h"
#include "vfxNodes/vfxNodeImageDelayLine.h"
#include "vfxNodes/vfxNodeImageDownsample.h"
#include "vfxNodes/vfxNodeImageScale.h"
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
#include "vfxNodes/vfxNodePointcloud.h"
#include "vfxNodes/vfxNodeSampleAndHold.h"
#include "vfxNodes/vfxNodeSequence.h"
#include "vfxNodes/vfxNodeSound.h"
#include "vfxNodes/vfxNodeSpectrum1D.h"
#include "vfxNodes/vfxNodeSpectrum2D.h"
#include "vfxNodes/vfxNodeSurface.h"
#include "vfxNodes/vfxNodeTime.h"
#include "vfxNodes/vfxNodeTimeline.h"
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
#include "vfxNodeTest.h"

using namespace tinyxml2;

//#define FILENAME "kinect.xml"
//#define FILENAME "yuvtest.xml"
#define FILENAME "timeline.xml"

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
extern void testDotTracker();
extern void testAudiochannels();
extern void testThreading();
extern void testStbTruetype();
extern void testMsdfgen();
extern void testDeepbelief();
extern void testImageCpuDelayLine();

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
	
	DefineNodeImpl("test", VfxNodeTest)
	
	DefineNodeImpl("intBool", VfxNodeBoolLiteral)
	DefineNodeImpl("intLiteral", VfxNodeIntLiteral)
	DefineNodeImpl("floatLiteral", VfxNodeFloatLiteral)
	DefineNodeImpl("transformLiteral", VfxNodeTransformLiteral)
	DefineNodeImpl("stringLiteral", VfxNodeStringLiteral)
	DefineNodeImpl("colorLiteral", VfxNodeColorLiteral)
	DefineNodeImpl("trigger.asFloat", VfxNodeTriggerAsFloat)
	DefineNodeImpl("time", VfxNodeTime)
	DefineNodeImpl("timeline", VfxNodeTimeline)
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
	DefineNodeImpl("surface", VfxNodeSurface)
	DefineNodeImpl("sequence", VfxNodeSequence)
	DefineNodeImpl("mouse", VfxNodeMouse)
	DefineNodeImpl("xinput", VfxNodeXinput)
	DefineNodeImpl("touches", VfxNodeTouches)
	DefineNodeImpl("leap", VfxNodeLeapMotion)
	DefineNodeImpl("kinect1", VfxNodeKinect1)
	DefineNodeImpl("kinect2", VfxNodeKinect2)
	DefineNodeImpl("pointcloud", VfxNodePointcloud)
	DefineNodeImpl("deepbelief", VfxNodeDeepbelief)
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
	DefineNodeImpl("image.dotTracker", VfxNodeDotTracker)
	DefineNodeImpl("image.toGpu", VfxNodeImageCpuToGpu)
	DefineNodeImpl("image.delay", VfxNodeImageDelayLine)
	DefineNodeImpl("image_cpu.equalize", VfxNodeImageCpuEqualize)
	DefineNodeImpl("image_cpu.delay", VfxNodeImageCpuDelayLine)
	DefineNodeImpl("image_cpu.downsample", VfxNodeImageCpuDownsample)
	DefineNodeImpl("image.downsample", VfxNodeImageDownsample)
	DefineNodeImpl("image.scale", VfxNodeImageScale)
	DefineNodeImpl("yuvToRgb", VfxNodeYuvToRgb)
	DefineNodeImpl("channel.select", VfxNodeChannelSelect)
	DefineNodeImpl("channel.slice", VfxNodeChannelSlice)
	DefineNodeImpl("channel.toGpu", VfxNodeChannelToGpu)
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

#include "Timer.h"
#include <immintrin.h>

static void sumChannels_fast(float ** channels, const int numChannels, const int channelSize, float * __restrict output)
{
#if 0
	const int channelSize4 = channelSize / 4;
	
	__m128 * __restrict output128 = (__m128*)output;
	
	memset(output128, 0, sizeof(float) * channelSize);
	
	for (int c = 0; c < numChannels; ++c)
	{
		__m128 * __restrict channel128 = (__m128*)channels[c];
		
		for (int i = 0; i < channelSize4; ++i)
		{
			output128[i] += channel128[i];
		}
	}
#else
	const int channelSize8 = channelSize / 8;
	
	__m256 * __restrict outputVec = (__m256*)output;
	
	memset(outputVec, 0, sizeof(float) * channelSize);
	
	for (int c = 0; c < numChannels; ++c)
	{
		__m256 * __restrict channelVec = (__m256*)channels[c];
		
		for (int i = 0; i < channelSize8; ++i)
		{
			outputVec[i] += channelVec[i];
		}
	}
#endif
}

static void sumChannels_slow(const float * const * channels, const int numChannels, const int channelSize, float * output)
{
	memset(output, 0, sizeof(float) * channelSize);
	
	for (int c = 0; c < numChannels; ++c)
	{
		const float * channel = channels[c];
		
		for (int i = 0; i < channelSize; ++i)
		{
			output[i] += channel[i];
		}
	}
}

static void testAudioMixing()
{
	const int kNumChannels = 24;
	const int kChannelSize = 64;
	
	float * channels[kNumChannels];
	
	for (int c = 0; c < kNumChannels; ++c)
	{
		channels[c] = (float*)_mm_malloc(sizeof(float) * kChannelSize, 16);
	}
	
	for (int c = 0; c < kNumChannels; ++c)
	{
		float * channel = channels[c];
		
		for (int i = 0; i < kChannelSize; ++i)
			channel[i] = c + i;
	}
	
	float output[kChannelSize];
	
	for (int i = 0; i < 100; ++i)
	{
		uint64_t f1 = g_TimerRT.TimeUS_get();
		
		float q = 0.f;
		
		for (int n = 0; n < 200; ++n)
		{
			sumChannels_fast(channels, kNumChannels, kChannelSize, output);
			
			q += output[kChannelSize - 1];
		}
		
		uint64_t f2 = g_TimerRT.TimeUS_get();
		
		//
		
		uint64_t s1 = g_TimerRT.TimeUS_get();
		
		for (int i = 0; i < 200; ++i)
		{
			sumChannels_slow(channels, kNumChannels, kChannelSize, output);
			
			q += output[kChannelSize - 1];
		}
		
		uint64_t s2 = g_TimerRT.TimeUS_get();
		
		//
		
		printf("fast: %gms, slow: %gms (%g)\n", (f2 - f1) / 1000.0, (s2 - s1) / 1000.0, q);
	}
}

//

int main(int argc, char * argv[])
{
	//framework.waitForEvents = true;
	
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = true;
	framework.enableDrawTiming = false;
	//framework.enableProfiling = true;
	
	//framework.fullscreen = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();
		
		vfxSetThreadName("Main Thread");
		
		//testDatGui();
		
		//testNanovg();
		
		//testChaosGame();
		
		//testFourier1d();
		
		//testFourier2d();
		
		//testImpulseResponseMeasurement();
		
		//testTextureAtlas();
		
		//testDynamicTextureAtlas();
		
		//testDotDetector();
		
		//testDotTracker();
		
		//testAudioMixing();

		//testAudiochannels();
		
		//testThreading();
		
		//testStbTruetype();
		
		//testMsdfgen();
		
		//testDeepbelief();
		
		//testImageCpuDelayLine();
		
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
		
		createVfxNodeTypeDefinitions(*typeDefinitionLibrary, g_vfxNodeTypeRegistrationList);
		
		//
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection();
		
		//
		
		GraphEdit * graphEdit = new GraphEdit(typeDefinitionLibrary);
		
		graphEdit->realTimeConnection = realTimeConnection;

		VfxGraph * vfxGraph = new VfxGraph();
		
		realTimeConnection->vfxGraph = vfxGraph;
		realTimeConnection->vfxGraphPtr = &vfxGraph;
		
		//
		
		g_currentVfxGraph = realTimeConnection->vfxGraph;
		
		graphEdit->load(FILENAME);
		
		g_currentVfxGraph = nullptr;
		
		//
		
		Surface * graphEditSurface = new Surface(GFX_SX, GFX_SY, false);
		
		//
		
		bool isPaused = false; // todo : move to editor
		
		double vflip = 1.0;
		
		while (!framework.quitRequested)
		{
			vfxCpuTimingBlock(tick);
			vfxGpuTimingBlock(tick);
			
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
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			//
			
			const float dt = framework.timeStep;
			
			//
			
			if (vfxGraph != nullptr)
			{
				vfxGraph->tick(isPaused ? 0.f : framework.timeStep);
			}
			
			//
			
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
			else if (keyboard.wentDown(SDLK_p) && keyboard.isDown(SDLK_LGUI))
			{
				isPaused = !isPaused;
			}
			
			g_currentVfxGraph = nullptr;
			
			if (graphEdit->state == GraphEdit::kState_Hidden)
				SDL_ShowCursor(0);
			else
				SDL_ShowCursor(1);
			
			// fixme : this should be handled by graph edit
			
			if (!graphEdit->selectedNodes.empty())
			{
				const GraphNodeId nodeId = *graphEdit->selectedNodes.begin();
				
				graphEdit->propertyEditor->setNode(nodeId);
			}
			
			// update vflip effect
			
			const double targetVflip = graphEdit->dragAndZoom.zoom < 0.f ? -1.0 : +1.0;
			const double vflipMix = std::pow(.001, dt);
			vflip = vflip * vflipMix + targetVflip * (1.0 - vflipMix);
			
			framework.beginDraw(31, 31, 31, 255);
			{
				vfxGpuTimingBlock(draw);
				
				if (vfxGraph != nullptr)
				{
					gxPushMatrix();
					{
						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0);
						gxScalef(1.f, vflip, 1.f);
						gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0);
						
						vfxGraph->draw();
					}
					gxPopMatrix();
				}
				
				pushSurface(graphEditSurface);
				{
					graphEditSurface->clear(0, 0, 0, 255);
					pushBlend(BLEND_ADD);
					
					glBlendEquation(GL_FUNC_ADD);
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
					
					graphEdit->draw();
					
					popBlend();
				}
				popSurface();
				
				if (graphEdit->hideTime == 1.f)
					setColor(colorWhite);
				else if (graphEdit->hideTime == 0.f)
					setColor(colorBlack);
				else if (graphEdit->state == GraphEdit::kState_HiddenIdle)
				{
					pushBlend(BLEND_OPAQUE);
					{
						const float radius = std::pow(1.f - graphEdit->hideTime, 2.f) * 200.f;
						
						setShader_GaussianBlurH(graphEditSurface->getTexture(), 32, radius);
						graphEditSurface->postprocess();
						clearShader();
					}
					popBlend();
					
					setColorf(1.f, 1.f, 1.f, graphEdit->hideTime);
				}
				else
				{
					setColorf(1.f, 1.f, 1.f, graphEdit->hideTime);
				}
				
				gxSetTexture(graphEditSurface->getTexture());
				{
					pushBlend(BLEND_ADD);
					
					glBlendEquation(GL_FUNC_ADD);
					glBlendFuncSeparate(GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);
					
					drawRect(0, 0, GFX_SX, GFX_SY);
					
					popBlend();
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}
		
		delete graphEditSurface;
		graphEditSurface = nullptr;
	
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
