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
#include "TextIO.h"

#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxNodes/vfxNodeVfxGraph.h"
#include "vfxTypes.h"

#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "mediaplayer/MPUtil.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "ui.h"
#include "vfxNodeBase.h"
#include <algorithm>
#include <cmath>

using namespace tinyxml2;

//#define FILENAME "graph.xml"
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
//#define FILENAME "testVfxGraph.xml"
//#define FILENAME "sampleTest.xml"
//#define FILENAME "midiTest.xml"
//#define FILENAME "draw3dTest.xml"
//#define FILENAME "nodeDataTest.xml"
//#define FILENAME "fsfxv2Test.xml"
//#define FILENAME "kinectTest2.xml"
//#define FILENAME "testOscilloscope.xml"
//#define FILENAME "testVisualizers.xml"
//#define FILENAME "testRibbon3.xml"
#define FILENAME "testPetals.xml"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

extern void testDynamicInputs();
extern void testRoutingEditor();
extern void testVfxNodeCreation();

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

struct VfxNodeTestDynamicSockets : VfxNodeBase
{
	enum Inputs
	{
		kInput_TestInputs,
		kInput_TestOutputs,
		kInput_COUNT
	};
	
	enum Outputs
	{
		kOutput_COUNT
	};
	
	VfxNodeTestDynamicSockets()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_TestInputs, kVfxPlugType_Bool);
		addInput(kInput_TestOutputs, kVfxPlugType_Bool);
	}
	
	virtual void tick(const float dt)
	{
		const bool testInputs = getInputBool(kInput_TestInputs, false);
		const bool testOutputs = getInputBool(kInput_TestOutputs, false);
		
		if (testInputs)
		{
			// test dynamically growing and shrinking list of inputs as a stress test
			
			const int kNumInputs = std::round(std::sin(framework.time) * 4.f + 5.f);

			DynamicInput inputs[kNumInputs];
			
			for (int i = 0; i < kNumInputs; ++i)
			{
				char name[32];
				sprintf_s(name, sizeof(name), "dynamic%d", i + 1);
				
				inputs[i].name = name;
				inputs[i].type = kVfxPlugType_Float;
			}
			
			setDynamicInputs(inputs, kNumInputs);
		}

		if (testOutputs)
		{
			// test dynamically growing and shrinking list of inputs as a stress test
			
			const int kNumOutputs = std::round(std::sin(framework.time) * 4.f + 5.f);

			DynamicOutput outputs[kNumOutputs];
			
			static float value = 1.f;
			
			for (int i = 0; i < kNumOutputs; ++i)
			{
				char name[32];
				sprintf_s(name, sizeof(name), "dynamic%d", i + 1);
				
				outputs[i].name = name;
				outputs[i].type = kVfxPlugType_Float;
				outputs[i].mem = &value;
			}
			
			setDynamicOutputs(outputs, kNumOutputs);
		}
	}
};

VFX_NODE_TYPE(VfxNodeTestDynamicSockets)
{
	typeName = "test.dynamicSockets";
	in("testInputs", "bool", "0");
	in("testOutputs", "bool", "0");
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

//

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	framework.enableRealTimeEditing = true;
	
	framework.enableDepthBuffer = false;
	//framework.enableDrawTiming = true;
	//framework.enableProfiling = true;
	
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	
	if (framework.init(GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();
		
		vfxSetThreadName("Main Thread");
		
		//testVfxNodeCreation();
		
		//testDynamicInputs();
		
		//testRoutingEditor();
		
		//
		
	#ifndef DEBUG
		framework.fillCachesWithPath(".", true);
	#endif
		
		//
		
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
		
		//
		
		if (false)
		{
			// stress test vfx graph creation and destruction. enable this code path when profiling only
			
			Graph graph;
			
			XMLDocument document;
			
			if (document.LoadFile(FILENAME) == XML_SUCCESS)
			{
				const XMLElement * xmlGraph = document.FirstChildElement("graph");
				
				if (xmlGraph != nullptr)
				{
					graph.loadXml(xmlGraph, typeDefinitionLibrary);
					
					int i = 0;
					
					for (;;)
					{
						auto t1 = g_TimerRT.TimeUS_get();
						
						auto vfxGraph = constructVfxGraph(graph, typeDefinitionLibrary);
						
						vfxGraph->tick(GFX_SX, GFX_SY, 0.f);
						
						delete vfxGraph;
						vfxGraph = nullptr;
						
						auto t2 = g_TimerRT.TimeUS_get();
						printf("construct %003d + tick took %dus\n", i, int(t2 - t1));
						
						++i;
					}
				}
			}
		}
		
		//
		
		VfxGraph * vfxGraph = new VfxGraph();
		
		RealTimeConnection * realTimeConnection = new RealTimeConnection(vfxGraph);
		
		GraphEdit * graphEdit = new GraphEdit(GFX_SX, GFX_SY, typeDefinitionLibrary, realTimeConnection);
		
		//
		
		struct WindowBase
		{
			virtual ~WindowBase()
			{
			}
			
			virtual void process(const float dt) = 0;
			
			virtual bool getQuitRequested() const = 0;
		};
		
		struct GraphEditWindow : WindowBase
		{
			Window * window = nullptr;
			
			VfxGraph * vfxGraph = nullptr;
			
			RealTimeConnection * realTimeConnection = nullptr;
			
			GraphEdit * graphEdit = nullptr;
			
			GraphEditWindow(GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary, const char * filename)
			{
				window = new Window(filename, 640, 480, true);
				
				vfxGraph = new VfxGraph();
				
				realTimeConnection = new RealTimeConnection(vfxGraph);
				
				graphEdit = new GraphEdit(window->getWidth(), window->getHeight(), typeDefinitionLibrary, realTimeConnection);
				
				graphEdit->load(filename);
			}
			
			virtual ~GraphEditWindow() override
			{
				delete graphEdit;
				graphEdit = nullptr;
				
				delete realTimeConnection;
				realTimeConnection = nullptr;
				
				delete vfxGraph;
				vfxGraph = nullptr;
				
				delete window;
				window = nullptr;
			}
			
			virtual void process(const float dt) override
			{
				if (window->hasFocus())
				{
					pushWindow(*window);
					{
						const int sx = window->getWidth();
						const int sy = window->getHeight();
						
						graphEdit->displaySx = sx;
						graphEdit->displaySy = sy;
						
						graphEdit->tick(dt, false);
						
						vfxGraph->tick(sx, sy, dt);
						
						framework.beginDraw(0, 0, 0, 0);
						{
							vfxGraph->draw(sx, sy);
							
							graphEdit->tickVisualizers(dt);
							
							graphEdit->draw();
						}
						framework.endDraw();
					}
					popWindow();
				}
			}
			
			virtual bool getQuitRequested() const override
			{
				return window->getQuitRequested();
			}
		};
		
		struct ShaderEditWindow : WindowBase
		{
			std::string filename;
			
			Window * window = nullptr;
			
			FrameworkImGuiContext uiContext;
			
			TextEditor textEditor;
			
			TextIO::LineEndings lineEndings = TextIO::kLineEndings_Unix;
			
			bool textIsValid = false;
			
			bool quitRequested = false;
			
			ShaderEditWindow(const char * in_filename)
			{
				filename = in_filename;
				
				window = new Window(filename.c_str(), 640, 480, true);
				
				uiContext.init();
				
				textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
				
				load(filename.c_str());
			}
			
			virtual ~ShaderEditWindow() override
			{
				uiContext.shut();
				
				delete window;
				window = nullptr;
			}
			
			bool load(const char * filename)
			{
				textEditor.SetText("");
				
				textIsValid = false;
				
				//
				
				std::vector<std::string> lines;
				
				if (TextIO::load(filename, lines, lineEndings) == false)
				{
					logError("failed to read file contents");
					
					return false;
				}
				else
				{
					textEditor.SetTextLines(lines);
					
					textIsValid = true;
					
					return true;
				}
			}
			
			virtual void process(const float dt) override
			{
				if (window->hasFocus())
				{
					pushWindow(*window);
					{
						const int sx = window->getWidth();
						const int sy = window->getHeight();
						
						bool inputIsCaptured = false;
						
						uiContext.processBegin(dt, sx, sy, inputIsCaptured);
						{
							ImGui::SetNextWindowPos(ImVec2(0, 0));
							ImGui::SetNextWindowSize(ImVec2(sx, sy));
							
							if (ImGui::Begin("Text Editor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar))
							{
							#if 1
								if (ImGui::BeginMenuBar())
								{
									if (ImGui::BeginMenu("File"))
									{
										if (ImGui::MenuItem("Save"))
										{
											auto lines = textEditor.GetTextLines();
											
											if (TextIO::save(filename.c_str(), lines, lineEndings) == false)
											{
												logError("failed to save shader file");
											}
										}
										
										ImGui::Separator();
										if (ImGui::MenuItem("Reload"))
											load(filename.c_str());
										
										ImGui::Separator();
										quitRequested = ImGui::MenuItem("Close");
										
										ImGui::EndMenu();
									}
									
									if (ImGui::BeginMenu("Edit"))
									{
										if (ImGui::MenuItem("Undo"))
											textEditor.Undo();
										if (ImGui::MenuItem("Redo"))
											textEditor.Redo();
										
										ImGui::EndMenu();
									}
									
									if (ImGui::BeginMenu("View"))
									{
										if (ImGui::BeginMenu("Line Endings"))
										{
											if (ImGui::MenuItem("Unix (LF)", nullptr, lineEndings == TextIO::kLineEndings_Unix))
												lineEndings = TextIO::kLineEndings_Unix;
											if (ImGui::MenuItem("Windows (CRLF)", nullptr, lineEndings == TextIO::kLineEndings_Windows))
												lineEndings = TextIO::kLineEndings_Windows;
											
											ImGui::EndMenu();
										}
										
										ImGui::EndMenu();
									}
									
									ImGui::EndMenuBar();
								}
							#endif
							
								textEditor.Render("Pixel Shader");
								
								ImGui::End();
							}
						}
						uiContext.processEnd();
						
						framework.beginDraw(0, 0, 0, 0);
						{
							uiContext.draw();
						}
						framework.endDraw();
					}
					popWindow();
				}
			}
			
			virtual bool getQuitRequested() const override
			{
				return window->getQuitRequested() || quitRequested;
			}
		};
		
		std::list<WindowBase*> windows;
		
		graphEdit->handleNodeDoubleClicked = [&](const GraphNodeId nodeId)
		{
			auto node = graphEdit->tryGetNode(nodeId);
			
			if (node == nullptr)
				return;
			
			//
			
			if (node->typeName == "vfxGraph")
			{
				auto value = node->inputValues.find("file");
				
				if (value != node->inputValues.end())
				{
					const char * filename = value->second.c_str();
					
					GraphEditWindow * window = new GraphEditWindow(typeDefinitionLibrary, filename);
					
					windows.push_back(window);
				}
			}
			else if (node->typeName == "draw.fsfx")
			{
				auto value = node->inputValues.find("shader");
				
				if (value != node->inputValues.end())
				{
					const std::string filename = value->second + ".ps";
					
					ShaderEditWindow * window = new ShaderEditWindow(filename.c_str());
					
					windows.push_back(window);
				}
			}
		};
		
		//
		
		graphEdit->load(FILENAME);
		
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
			
			if (filedrop.empty() == false)
			{
				graphEdit->load(filedrop.c_str());
				
				graphEdit->idleTime = 0.f;
				
				filedrop.clear();
			}
			
			//
			
			inputIsCaptured |= graphEdit->tick(dt, inputIsCaptured);
			
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
				
				vfxGraph->tick(GFX_SX, GFX_SY, timeStep);
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
						
						vfxGraph->draw(GFX_SX, GFX_SY);
					}
					gxPopMatrix();
				}
				
				graphEdit->tickVisualizers(dt);
				
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
			
			//
			
			for (auto i = windows.begin(); i != windows.end(); )
			{
				WindowBase *& window = *i;
				
				window->process(dt);
				
				if (window->getQuitRequested())
				{
					delete window;
					window = nullptr;
					
					i = windows.erase(i);
				}
				else
					++i;
			}
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
