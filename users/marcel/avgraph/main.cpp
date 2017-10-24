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

#include "mediaplayer_new/MPUtil.h"
#include "../libparticle/ui.h"
#include "Timer.h"

using namespace tinyxml2;

//#define FILENAME "kinect.xml"
//#define FILENAME "yuvtest.xml"
//#define FILENAME "timeline.xml"
//#define FILENAME "channels.xml"
//#define FILENAME "drawtest.xml"
//#define FILENAME "resourceTest.xml"
#define FILENAME "drawImageTest.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testAudiochannels();
extern void testCatmullRom();
extern void testHrtf();
extern void testMacWebcam();
extern void testReactionDiffusion();

extern void testMain();

//

#include "vfxNodes/oscReceiver.h"

struct VfxNodeMidiOsc : VfxNodeBase, OscReceiveHandler
{
	enum Input
	{
		kInput_IpAddress,
		kInput_UdpPort,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Controller,
		kOutput_Value,
		kOutput_ValueNorm,
		kOutput_ControllerTrigger,
		kOutput_COUNT
	};
	
	OscReceiver oscReceiver;
	
	int controllerOutput;
	int controllerValueOutput;
	float controllerValueNormOutput;
	
	VfxNodeMidiOsc()
		: VfxNodeBase()
		, oscReceiver()
		, controllerOutput(0)
		, controllerValueOutput(0)
		, controllerValueNormOutput(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_IpAddress, kVfxPlugType_String);
		addInput(kInput_UdpPort, kVfxPlugType_Int);
		addOutput(kOutput_Controller, kVfxPlugType_Int, &controllerOutput);
		addOutput(kOutput_Value, kVfxPlugType_Int, &controllerValueOutput);
		addOutput(kOutput_ValueNorm, kVfxPlugType_Float, &controllerValueNormOutput);
		addOutput(kOutput_ControllerTrigger, kVfxPlugType_Trigger, nullptr);
	}
	
	virtual void tick(const float dt) override
	{
		const char * ipAddress = getInputString(kInput_IpAddress, "");
		const int udpPort = getInputInt(kInput_UdpPort, 0);
		
		if (oscReceiver.isAddressChange(ipAddress, udpPort))
		{
			logDebug("(re)initialising OSC receiver");
			
			oscReceiver.shut();
			
			oscReceiver.init(ipAddress, udpPort);
		}
		
		oscReceiver.tick(this);
	}
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		logDebug("received OSC message!");
		
		try
		{
			auto a = m.ArgumentStream();
			
			//osc::ReceivedMessageArgument message;
			//a >> message;
			const char * message;
			a >> message;
			
			if (message != nullptr && strcmp(message, "controller_change") == 0)
			{
				osc::int32 controller;
				osc::int32 controllerValue;
				a >> controller;
				a >> controllerValue;
				
				controllerOutput = controller;
				controllerValueOutput = controllerValue;
				controllerValueNormOutput = controllerValue / 127.f;
				
				trigger(kOutput_ControllerTrigger);
			}
		}
		catch (std::exception & e)
		{
			logError("failed to decode midiosc message: %s", e.what());
		}
	}
};

VFX_NODE_TYPE(midi_osc, VfxNodeMidiOsc)
{
	typeName = "midi.osc";
	
	in("ip", "string");
	in("port", "int");
	out("knob", "int");
	out("value", "int");
	out("value_norm", "float");
	out("trigger", "trigger");
}

//

struct VfxNodeResourceTest : VfxNodeBase
{
	enum Input
	{
		kInput_Randomize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float value;
	
	VfxNodeResourceTest()
		: VfxNodeBase()
		, value(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Randomize, kVfxPlugType_Trigger);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		const char * resourceData = node.getResource("float", "value", nullptr);
		
		if (resourceData != nullptr)
		{
			XMLDocument d;
			
			if (d.Parse(resourceData) == XML_SUCCESS)
			{
				auto e = d.FirstChildElement("float");
				
				if (e != nullptr)
				{
					value = e->FloatAttribute("value");
				}
			}
		}
	}
	
	virtual void handleTrigger(const int socketIndex) override
	{
		value = random(0.f, 1.f);
	}
	
	virtual void beforeSave(GraphNode & node) const override
	{
		if (value == 0.f)
		{
			node.clearResource("float", "value");
		}
		else
		{
			XMLPrinter p;
			
			p.OpenElement("float");
			{
				p.PushAttribute("value", value);
			}
			p.CloseElement();
			
			const char * resourceData = p.CStr();
			
			node.setResource("float", "value", resourceData);
		}
	}
};

VFX_NODE_TYPE(resource_test, VfxNodeResourceTest)
{
	typeName = "test.resource";
	
	resourceTypeName = "timeline";
	
	in("randomize!", "trigger");
	out("value", "float");
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
		
		//testAudiochannels();
		
		//testMacWebcam();
		
		//testHrtf();

		//testCatmullRom();
		
		//testReactionDiffusion();
		
		//testMain();
		
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
		
		createVfxEnumTypeDefinitions(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList);
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
		
		float realtimePreviewAnim = 1.f;
		
		double vflip = 1.0;
		
		while (!framework.quitRequested)
		{
			vfxCpuTimingBlock(tick);
			vfxGpuTimingBlock(tick);
			
			// when real-time preview is disabled and all animations are done, wait for events to arrive. otherwise just keep processing and drawing frames
			
			if (graphEdit->editorOptions.realTimePreview == false && realtimePreviewAnim == 0.f && graphEdit->animationIsDone)
				framework.waitForEvents = true;
			else
				framework.waitForEvents = false;
			
			framework.process();
			
			// the time step may be large when waiting for events. to avoid animation hitches we set it to zero here. when the processing below activates an animation we'll use the actual time step we get
			
			if (framework.waitForEvents)
				framework.timeStep = 0.f;
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			// avoid excessively large time steps by clamping it to 1/15th of a second, simulating a minimum of 15 fps
			
			framework.timeStep = std::min(framework.timeStep, 1.f / 15.f);
			
			const float dt = framework.timeStep;
			
			//
			
			if (graphEdit->editorOptions.realTimePreview)
			{
				realtimePreviewAnim = std::min(1.f, realtimePreviewAnim + dt / .3f);
			}
			else
			{
				realtimePreviewAnim = std::max(0.f, realtimePreviewAnim - dt / .5f);
			}
			
			if (vfxGraph != nullptr)
			{
				const double timeStep = dt * realtimePreviewAnim;
				
				vfxGraph->tick(timeStep);
			}
			
			//
			
			bool inputIsCaptured = false;
			
			//
			
			g_currentVfxGraph = realTimeConnection->vfxGraph;
			
			inputIsCaptured |= graphEdit->tick(dt, inputIsCaptured);
			
			g_currentVfxGraph = nullptr;
			
			if (inputIsCaptured == false)
			{
				if (keyboard.wentDown(SDLK_p) && keyboard.isDown(SDLK_LGUI))
				{
					inputIsCaptured = true;
					
					graphEdit->editorOptions.realTimePreview = !graphEdit->editorOptions.realTimePreview;
				}
			}
			
			if (graphEdit->state == GraphEdit::kState_Hidden)
				SDL_ShowCursor(0);
			else
				SDL_ShowCursor(1);
			
			// update vflip effect
			
			const double targetVflip = graphEdit->dragAndZoom.zoom < 0.f ? -1.0 : +1.0;
			const double vflipMix = std::pow(.001, dt);
			vflip = vflip * vflipMix + targetVflip * (1.0 - vflipMix);
			
			framework.beginDraw(0, 0, 0, 255);
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
				
				const float hideTime = graphEdit->hideTime;
				
				if (hideTime > 0.f)
				{
					//else if (graphEdit->state == GraphEdit::kState_HiddenIdle)
					if (hideTime < 1.f)
					{
						pushBlend(BLEND_OPAQUE);
						{
							const float radius = std::pow(1.f - hideTime, 2.f) * 200.f;
							
							setShader_GaussianBlurH(graphEditSurface->getTexture(), 32, radius);
							graphEditSurface->postprocess();
							clearShader();
						}
						popBlend();
					}
					
					Shader composite("composite");
					setShader(composite);
					pushBlend(BLEND_ADD);
					{
						glBlendEquation(GL_FUNC_ADD);
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						
						composite.setTexture("source", 0, graphEditSurface->getTexture(), true);
						composite.setImmediate("opacity", hideTime);
						
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					popBlend();
					clearShader();
				}
			}
			framework.endDraw();
		}
		
		Font("calibri.ttf").saveCache();
		
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
