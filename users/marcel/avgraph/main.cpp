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

#include "vfxNodes/oscEndpointMgr.h"
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
//#define FILENAME "drawImageTest.xml"
//#define FILENAME "oscpath.xml"
//#define FILENAME "oscpathlist.xml"
//#define FILENAME "wekinatorTest.xml"
#define FILENAME "testVfxGraph.xml"
//#define FILENAME "sampleTest.xml"
//#define FILENAME "midiTest.xml"
//#define FILENAME "draw3dTest.xml"
//#define FILENAME "nodeDataTest.xml"
//#define FILENAME "fsfxv2Test.xml"
//#define FILENAME "kinectTest2.xml"

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

extern void codevember1();

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

VFX_NODE_TYPE(VfxNodeResourceTest)
{
	typeName = "test.resource";
	
	resourceTypeName = "timeline";
	
	in("randomize!", "trigger");
	out("value", "float");
}

//

#include "Noise.h"

static void testGradientShader()
{
	const int kNumCircles = 32;
	
	Vec3 circles[kNumCircles];
	
	for (int i = 0; i < kNumCircles; ++i)
	{
		circles[i][0] = random(0, GFX_SX);
		circles[i][1] = random(0, GFX_SY);
		circles[i][2] = random(0.f, 5.f); // life
	}
	
	do
	{
		framework.process();
		
		for (int i = 0; i < kNumCircles; ++i)
		{
			circles[i][2] -= framework.timeStep;
			
			if (circles[i][2] <= 0.f)
			{
				circles[i][0] = random(0, GFX_SX);
				circles[i][1] = random(0, GFX_SY);
				circles[i][2] = random(0.f, 5.f); // life
			}
			
			circles[i][0] += scaled_octave_noise_3d(4, .5f, .01f, -500.f, +500.f, framework.time * 20.f + i, circles[i][0], circles[i][1]) * framework.timeStep;
			circles[i][1] += scaled_octave_noise_3d(4, .5f, .01f, -500.f, +500.f, framework.time * 20.f - i, circles[i][0], circles[i][1]) * framework.timeStep;
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const Mat4x4 cmat = Mat4x4(true)
				.Scale(1.f / 100.f, 1.f / 100.f, 1.f)
				.RotateZ(-framework.time)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			const Mat4x4 tmat = Mat4x4(true)
				.Translate(.5f, .5f, 0.f)
				.Scale(1.f / 400.f, 1.f / 400.f, 1.f)
				.RotateZ(+framework.time)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			
			const GRADIENT_TYPE gradientType = mouse.isDown(BUTTON_LEFT) ? GRADIENT_RADIAL : GRADIENT_LINEAR;
			const float gradientBias = mouse.x / float(GFX_SX);
			const float gradientScale = gradientBias == 1.f ? 0.f : 1.f / (1.f - gradientBias);
			hqSetGradient(gradientType, cmat, colorWhite, colorBlack, COLOR_MUL, gradientBias, gradientScale);
			hqSetTexture(tmat, getTexture("happysun.jpg"));
			for (int i = 0; i < 360; i += 20)
			{
				gxPushMatrix();
				gxRotatef(i + framework.time, 0, 0, 1);
				hqBegin(HQ_LINES);
				{
					setColor(colorWhite);
					hqLine(0, 0, 10, GFX_SX, GFX_SY, 50);
				}
				hqEnd();
				gxPopMatrix();
			}
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < kNumCircles; ++i)
				{
					gxColor4f(1, 1, 1, Calc::Clamp(circles[i][2], 0.f, 1.f));
					hqFillCircle(circles[i][0], circles[i][1], 20.f);
				}
			}
			hqEnd();
			hqClearGradient();
			hqClearTexture();
		}
		framework.endDraw();
		
	}
	while (!keyboard.wentDown(SDLK_SPACE));
}

//

static void testCamera3d()
{
	Camera3d camera;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	enum State
	{
		kState_Play,
		kState_Menu
	};
	
	State state = kState_Play;
	
	UiState uiState;
	uiState.sx = 260;
	uiState.x = (GFX_SX - uiState.sx) / 2;
	uiState.y = GFX_SY * 2 / 3;
	
	do
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		if (state == kState_Play)
		{
			if (keyboard.wentDown(SDLK_TAB))
				state = kState_Menu;
			else
			{
				camera.tick(dt, true);
			}
		}
		else if (state == kState_Menu)
		{
			if (keyboard.wentDown(SDLK_TAB))
				state = kState_Play;
			else
			{
			 
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				
				gxPushMatrix();
				{
					Mat4x4 cameraMatrix = camera.getViewMatrix();
					//Mat4x4 cameraMatrix = camera.getWorldMatrix();
					cameraMatrix.SetTranslation(0, 0, 0);
					gxMultMatrixf(cameraMatrix.m_v);
					gxMultMatrixf(cameraMatrix.m_v);
					gxMultMatrixf(cameraMatrix.m_v);
					gxScalef(.3f, .3f, .3f);
					
					setColor(colorWhite);
					gxSetTexture(getTexture("happysun.jpg"));
					drawRect3d(0, 1);
					drawRect3d(2, 0);
					gxSetTexture(0);
				}
				gxPopMatrix();
				
				setColor(colorWhite);
				gxSetTexture(getTexture("picture.jpg"));
				if (mouse.isDown(BUTTON_LEFT))
					drawGrid3dLine(100, 100, 0, 2);
				else
					drawGrid3d(10, 10, 0, 2);
				gxSetTexture(0);
				
				glDisable(GL_DEPTH_TEST);
			}
			camera.popViewMatrix();
			
			//
			
			projectScreen2d();
			
			if (state == kState_Play)
			{
				drawText(30, 30, 24, +1, +1, "press TAB to open menu");
			}
			else if (state == kState_Menu)
			{
				setColor(31, 63, 127, 127);
				drawRect(0, 0, GFX_SX, GFX_SY);
				
				makeActive(&uiState, true, true);
				pushMenu("camera");
				{
					doLabel("camera", 0.f);
					doTextBox(fov, "field of view", dt);
					doTextBox(near, "near distance", dt);
					doTextBox(far, "far distance", dt);
				}
				popMenu();
			}
			
			popFontMode();
		}
		framework.endDraw();

	}
	while (!keyboard.wentDown(SDLK_SPACE));
}

//

static void testVfxNodeCreation()
{
	for (VfxNodeTypeRegistration * registration = g_vfxNodeTypeRegistrationList; registration != nullptr; registration = registration->next)
	{
		const int64_t t1 = g_TimerRT.TimeUS_get();
		
		VfxNodeBase * vfxNode = registration->create();
		
		GraphNode node;
		
		vfxNode->initSelf(node);
		
		vfxNode->init(node);
		
		// todo : check if vfx node is created properly
		
		delete vfxNode;
		vfxNode = nullptr;
		
		const int64_t t2 = g_TimerRT.TimeUS_get();
		
		logDebug("node create/destroy took %dus. nodeType=%s", t2 - t1, registration->typeName.c_str());
	}
}

//

static void testDynamicInputs()
{
	VfxGraph g;
	
	VfxNodeBase * node1 = new VfxNodeBase();
	VfxNodeBase * node2 = new VfxNodeBase();
	
	g.nodes[0] = node1;
	g.nodes[1] = node2;
	
	float values[32];
	for (int i = 0; i < 32; ++i)
		values[i] = i;
	
	node1->resizeSockets(1, 2);
	node2->resizeSockets(2, 1);
	
	node1->addInput(0, kVfxPlugType_Float);
	node1->addOutput(0, kVfxPlugType_Float, &values[0]);
	node1->addOutput(1, kVfxPlugType_Float, &values[1]);
	node2->addInput(0, kVfxPlugType_Float);
	node2->addInput(1, kVfxPlugType_Float);
	node2->addOutput(0, kVfxPlugType_Float, &values[16]);
	
	node1->tryGetInput(0)->connectTo(*node2->tryGetOutput(0));
	node2->tryGetInput(0)->connectTo(*node1->tryGetOutput(0));
	node2->tryGetInput(1)->connectTo(*node1->tryGetOutput(1));
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "a";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketIndex = 0;
		g.dynamicData->links.push_back(link);
	}
	
	{
		VfxDynamicLink link;
		link.srcNodeId = 0;
		link.srcSocketName = "b";
		link.srcSocketIndex = -1;
		link.dstNodeId = 1;
		link.dstSocketName = "a";
		link.dstSocketIndex = -1;
		g.dynamicData->links.push_back(link);
	}
	
	g_currentVfxGraph = &g;
	{
		VfxNodeBase::DynamicInput inputs[2];
		inputs[0].name = "a";
		inputs[0].type = kVfxPlugType_Float;
		inputs[1].name = "b";
		inputs[1].type = kVfxPlugType_Float;
		
		node1->setDynamicInputs(inputs, 2);
		
		node1->setDynamicInputs(nullptr, 0);
		
		node1->setDynamicInputs(inputs, 2);
		
		//
		
		float value = 1.f;
		
		VfxNodeBase::DynamicOutput outputs[1];
		outputs[0].name = "a";
		outputs[0].type = kVfxPlugType_Float;
		outputs[0].mem = &value;
		
		node2->setDynamicOutputs(outputs, 1);
		
		node2->setDynamicOutputs(nullptr, 0);
		
		node2->setDynamicOutputs(outputs, 1);
	}
	g_currentVfxGraph = nullptr;
}

//

#include "Path.h"

static std::string filedrop;

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const Path basepath(getDirectory());
		const Path filepath(args.getString("file", ""));
		
		Path relativePath;
		relativePath.MakeRelative(basepath, filepath);
		
		filedrop = relativePath.ToString();
	}
}

static void handleRealTimeEdit(const std::string & filename)
{
	if (String::StartsWith(filename, "fsfx/"))
	{
		// todo : reload shader
	}
}

//

int main(int argc, char * argv[])
{
	framework.enableRealTimeEditing = true;
	
	//framework.minification = 2;
	
	framework.enableDepthBuffer = false;
	framework.enableDrawTiming = false;
	//framework.enableProfiling = true;
	
	//framework.fullscreen = true;
	
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	framework.realTimeEditCallback = handleRealTimeEdit;
	
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
		
		//testGradientShader();
		
		//testCamera3d();
		
		//testMain();
		
		//testVfxNodeCreation();
		
		//testDynamicInputs();
		
		//codevember1();
		
		//
		
	#ifndef DEBUG
		framework.fillCachesWithPath(".", true);
	#endif
		
		//
		
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
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
		
		SDL_StartTextInput();
		
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
			
			g_oscEndpointMgr.tick();
			
			//
			
			bool inputIsCaptured = false;
			
			//
			
			g_currentVfxGraph = realTimeConnection->vfxGraph;
			
			if (filedrop.empty() == false)
			{
				graphEdit->load(filedrop.c_str());
				
				graphEdit->idleTime = 0.f;
				
				filedrop.clear();
			}
			
			g_currentVfxGraph = nullptr;
			
			//
			
			g_currentVfxGraph = realTimeConnection->vfxGraph;
			
			inputIsCaptured |= graphEdit->tick(dt, inputIsCaptured);
			
			g_currentVfxGraph = nullptr;
			
			if (inputIsCaptured == false)
			{
				// todo : move this to graph edit ?
				// todo : dynamically get src/dst socket indices from type definition
				// todo : pop up a node type select dialog ?
				
				if (mouse.wentDown(BUTTON_LEFT) && keyboard.isDown(SDLK_RSHIFT))
				{
					GraphEdit::HitTestResult hitTestResult;
					
					if (graphEdit->hitTest(graphEdit->mousePosition.x, graphEdit->mousePosition.y, hitTestResult))
					{
						if (hitTestResult.hasLink)
						{
							auto linkTypeDefinition = graphEdit->tryGetLinkTypeDefinition(hitTestResult.link->id);
							
							if (linkTypeDefinition != nullptr)
							{
								if (linkTypeDefinition->srcTypeName == "draw" && linkTypeDefinition->dstTypeName == "draw")
								{
									GraphNodeSocketLink link = *hitTestResult.link;
									
									GraphEdit::LinkPath linkPath;
									
									if (graphEdit->getLinkPath(link.id, linkPath))
									{
										GraphNodeId nodeId;
										
										if (graphEdit->tryAddNode(
											"draw.fsfx-v2",
											graphEdit->mousePosition.x,
											graphEdit->mousePosition.y,
											true, &nodeId))
										{
											if (true)
											{
												GraphNodeSocketLink link1;
												link1.id = graphEdit->graph->allocLinkId();
												link1.srcNodeId = link.srcNodeId;
												link1.srcNodeSocketIndex = link.srcNodeSocketIndex;
												link1.srcNodeSocketName = link.srcNodeSocketName;
												link1.dstNodeId = nodeId;
												link1.dstNodeSocketIndex = 0;
												link1.dstNodeSocketName = "any";
												graphEdit->graph->addLink(link1, true);
											}
											
											if (true)
											{
												GraphNodeSocketLink link2;
												link2.id = graphEdit->graph->allocLinkId();
												link2.srcNodeId = nodeId;
												link2.srcNodeSocketIndex = 0;
												link2.srcNodeSocketName = "before";
												link2.dstNodeId = link.dstNodeId;
												link2.dstNodeSocketIndex = link.dstNodeSocketIndex;
												link2.dstNodeSocketName = link.dstNodeSocketName;
												graphEdit->graph->addLink(link2, true);
											}
										}
									}
								}
							}
						}
					}
					
					inputIsCaptured = true;
				}
			}
			
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
				
				Assert(g_currentVfxGraph == nullptr);
				g_currentVfxGraph = vfxGraph;
				{
					graphEdit->tickVisualizers(dt);
				}
				g_currentVfxGraph = nullptr;
				
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
						glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
					
						composite.setTexture("source", 0, graphEditSurface->getTexture(), false, true);
						composite.setImmediate("opacity", hideTime);
						
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					popBlend();
					clearShader();
				}
			}
			framework.endDraw();
		}
		
		SDL_StopTextInput();
		
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
