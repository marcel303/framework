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

#include "framework.h"
#include "graph.h"
#include "StringEx.h"

#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxTypes.h"

#include "mediaplayer/MPUtil.h"
#include "../libparticle/ui.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "vfxNodes/vfxNodeBase.h"

#include "audioapp.h"

using namespace tinyxml2;

#define FILENAME "dancehack.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

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

struct VideoApp
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	RealTimeConnection * realTimeConnection = nullptr;
	
	GraphEdit * graphEdit = nullptr;
	
	VfxGraph * vfxGraph = nullptr;
	
	Surface * worldSurface = nullptr;
	Surface * graphSurface = nullptr;
	
	float realtimePreviewAnim = 1.f;
	
	double vflip = 1.0;
	
	void init()
	{
		typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
		//
		
		realTimeConnection = new RealTimeConnection();
		
		//
		
		graphEdit = new GraphEdit(typeDefinitionLibrary);
		
		graphEdit->realTimeConnection = realTimeConnection;

		vfxGraph = new VfxGraph();
		
		realTimeConnection->vfxGraph = vfxGraph;
		realTimeConnection->vfxGraphPtr = &vfxGraph;
		
		//
		
		g_currentVfxGraph = realTimeConnection->vfxGraph;
		
		graphEdit->load(FILENAME);
		
		g_currentVfxGraph = nullptr;
		
		//
		
		worldSurface = new Surface(GFX_SX, GFX_SY, false);
		graphSurface = new Surface(GFX_SX, GFX_SY, false);
	}
	
	void tick(const float dt, bool & inputIsCaptured)
	{
		g_oscEndpointMgr.tick();
	
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
								GraphLink link = *hitTestResult.link;
								
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
											GraphLink link1;
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
											GraphLink link2;
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
		
		if (vfxGraph != nullptr)
		{
			vfxGraph->tick(GFX_SX, GFX_SY, dt);
		}
		
		// update vflip effect
		
		const double targetVflip = graphEdit->dragAndZoom.zoom < 0.f ? -1.0 : +1.0;
		const double vflipMix = std::pow(.001, dt);
		vflip = vflip * vflipMix + targetVflip * (1.0 - vflipMix);
	}
	
	void draw(const float dt)
	{
		pushSurface(worldSurface);
		{
			worldSurface->clear(0, 0, 0, 255);
			if (vfxGraph != nullptr)
			{
				gxPushMatrix();
				{
					gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0);
					gxScalef(1.f, vflip, 1.f);
					gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0);
					
					vfxGraph->draw(GFX_SX, GFX_SY);
				}
				gxPopMatrix();
			}

			Assert(g_currentVfxGraph == nullptr);
			g_currentVfxGraph = vfxGraph;
			{
				graphEdit->tickVisualizers(dt);
			}
			g_currentVfxGraph = nullptr;
		}
		popSurface();

		pushSurface(graphSurface);
		{
			graphSurface->clear(0, 0, 0, 255);
			pushBlend(BLEND_ADD);
			
			glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
			
			graphEdit->draw();
			
			popBlend();
		}
		popSurface();
	}
	
	void shut()
	{
		delete worldSurface;
		worldSurface = nullptr;
		
		delete graphSurface;
		graphSurface = nullptr;
	
		delete graphEdit;
		graphEdit = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete typeDefinitionLibrary;
		typeDefinitionLibrary = nullptr;
	}
};

//

AudioApp * g_audioApp = nullptr;

struct VfxNodeAudioApp : VfxNodeBase
{
	enum Input
	{
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture imageOutput;
	
	VfxNodeAudioApp()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	}
	
	virtual void tick(const float dt)
	{
		if (isPassthrough || g_audioApp == nullptr)
		{
			imageOutput.texture = 0;
			return;
		}
		
		imageOutput.texture = g_audioApp->worldSurface->getTexture();
	}
};

VFX_NODE_TYPE(VfxNodeAudioApp)
{
	typeName = "audioApp";
	
	out("image", "image");
}

//

int main(int argc, char * argv[])
{
	framework.enableRealTimeEditing = true;
	
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
		
		//
		
	#ifndef DEBUG
		framework.fillCachesWithPath(".", true);
	#endif
		
		//
		
		AudioApp audioApp;
		
		VideoApp videoApp;
		
		audioApp.init();
		
		videoApp.init();
		
		g_audioApp = &audioApp;
		
		int selectedApp = 0;
		
		//
		
		SDL_StartTextInput();
		
		while (!framework.quitRequested)
		{
			vfxCpuTimingBlock(tick);
			vfxGpuTimingBlock(tick);
			
			framework.process();
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_LALT) && keyboard.isDown(SDLK_LGUI))
				selectedApp = (selectedApp + 1) % 2;
			
			// avoid excessively large time steps by clamping it to 1/15th of a second, simulating a minimum of 15 fps
			
			framework.timeStep = std::min(framework.timeStep, 1.f / 15.f);
			
			const float dt = framework.timeStep;
			
			//
			
			bool inputIsCaptured = false;
			
			if (selectedApp == 0)
				audioApp.tick(dt, inputIsCaptured);
			
			if (selectedApp == 1)
				videoApp.tick(dt, inputIsCaptured);
			
			inputIsCaptured = true;
			
			if (selectedApp != 0)
				audioApp.tick(dt, inputIsCaptured);
			
			if (selectedApp != 1)
				videoApp.tick(dt, inputIsCaptured);
			
			//
			
			/*
			if (videoApp.graphEdit->state == GraphEdit::kState_Hidden)
				SDL_ShowCursor(0);
			else
				SDL_ShowCursor(1);
			*/
			
			framework.beginDraw(0, 0, 0, 255);
			{
				vfxGpuTimingBlock(draw);
				
				audioApp.draw();
				
				videoApp.draw(dt);
				
				const float hideTime = videoApp.graphEdit->hideTime;
				
				if (hideTime > 0.f)
				{
					Shader composite("composite");
					setShader(composite);
					pushBlend(BLEND_OPAQUE);
					{
						const GLuint worldTexture =
							selectedApp == 0
							? audioApp.worldSurface->getTexture()
							: videoApp.worldSurface->getTexture();
						
						const GLuint graphTexture =
							selectedApp == 0
							? audioApp.graphSurface->getTexture()
							: videoApp.graphSurface->getTexture();
						
						composite.setTexture("sourceWorld", 0, worldTexture, false, true);
						composite.setTexture("sourceGraph", 1, graphTexture, false, true);
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
		
		videoApp.shut();
		
		audioApp.shut();
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
