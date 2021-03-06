/*
	Copyright (C) 2020 Marcel Smit
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
#include "graphEdit.h"
#include "testBase.h"
#include "vfxGraph.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodeBase.h"

#include "audioGraph.h"
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioUi.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "oscEndpointMgr.h"
#include "pcmDataCache.h"
#include <algorithm>
#include <cmath>

#include "ui.h"

#define ENABLE_AUDIO_RTE 1

extern const int GFX_SX;
extern const int GFX_SY;

static PcmDataCache s_pcmDataCache;

static AudioMutex * s_audioMutex = nullptr;
static AudioUpdateHandler * s_audioUpdateHandler = nullptr;
static PortAudioObject * s_paObject = nullptr;
static AudioVoiceManagerBasic * s_audioVoiceMgr = nullptr;
#if ENABLE_AUDIO_RTE
static AudioGraphManager_RTE * s_audioGraphMgr = nullptr;
#else
static AudioGraphManager_Basic * s_audioGraphMgr = nullptr;
#endif

extern OscEndpointMgr g_oscEndpointMgr;

static void initAudioGraph();
static void shutAudioGraph();

struct CreatureTrail
{
	const static int kMaxHistory = 100;
	
	float x[kMaxHistory];
	float y[kMaxHistory];
	
	int size = 0;
	int next = 0;
	
	void add(const float _x, const float _y)
	{
		x[next] = _x;
		y[next] = _y;
		
		next = next + 1 == kMaxHistory ? 0 : next + 1;
		size = size == kMaxHistory ? kMaxHistory : size + 1;
	}
	
	void draw(const float strokeSize) const
	{
		if (size >= 2)
		{
			pushBlend(BLEND_MAX);
			pushColorPost(POST_PREMULTIPLY_RGB_WITH_ALPHA);
			hqBegin(HQ_LINES, true);
			
			int index = size == kMaxHistory ? next : 0;
			
			for (int i = 0; i < size - 1; ++i)
			{
				const int index1 = index;
				index = index + 1 == kMaxHistory ? 0 : index + 1;
				const int index2 = index;
				
				setAlphaf(i / float(kMaxHistory));
				hqLine(
					x[index1],
					y[index1],
					strokeSize,
					x[index2],
					y[index2],
					strokeSize);
			}
			
			hqEnd();
			popColorPost();
			popBlend();
		}
	}
};

struct Creature
{
	float x = 0.f;
	float y = 0.f;
	float angle = 0.f;
	CreatureTrail trail;
	
	void tick(const float channelValue, const float moveSpeed, const float angleSpeed, const float dt)
	{
		x = lerp(0.f, x, powf(.8f, dt));
		y = lerp(0.f, y, powf(.8f, dt));
		
		angle += channelValue * angleSpeed * dt;
		
		x += std::cos(angle) * moveSpeed * dt;
		y += std::sin(angle) * moveSpeed * dt;
		
		trail.add(x, y);
	}
	
	void draw(const float strokeSize) const
	{
		trail.draw(strokeSize);
	}
};

struct VfxNodeCreature : VfxNodeBase
{
	const static int kMaxCreatures = 256;
	
	enum Input
	{
		kInput_Channel,
		kInput_MoveSpeed,
		kInput_AngleSpeed,
		kInput_StrokeSize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_COUNT
	};
	
	Creature creatures[kMaxCreatures];
	
	VfxNodeCreature()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Channel, kVfxPlugType_Channel);
		addInput(kInput_MoveSpeed, kVfxPlugType_Float);
		addInput(kInput_AngleSpeed, kVfxPlugType_Float);
		addInput(kInput_StrokeSize, kVfxPlugType_Float);
		addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	}
	
	virtual void tick(const float dt) override
	{
		const VfxChannel * channel = getInputChannel(kInput_Channel, nullptr);
		const float moveSpeed = getInputFloat(kInput_MoveSpeed, 0.f);
		const float angleSpeed = getInputFloat(kInput_AngleSpeed, 0.f);
		
		if (channel == nullptr)
		{
			return;
		}
		
		const int numCreatures = std::min(kMaxCreatures, channel->size);
		
		for (int i = 0; i < numCreatures; ++i)
		{
			creatures[i].tick(channel->data[i], moveSpeed, angleSpeed, dt);
		}
	}
	
	virtual void draw() const override
	{
		const float strokeSize = getInputFloat(kInput_StrokeSize, 1.f);
		
		for (int i = 0; i < kMaxCreatures; ++i)
		{
			const float hue = i / float(kMaxCreatures);
			setColor(Color::fromHSL(hue, 1.f, .5f));
			
			creatures[i].draw(strokeSize);
		}
	}
};

const int VfxNodeCreature::kMaxCreatures;

VFX_NODE_TYPE(VfxNodeCreature)
{
	typeName = "creature";
	
	in("channels", "channels");
	in("moveSpeed", "float");
	in("angleSpeed", "float");
	in("strokeSize", "float", "1");
	out("draw", "draw");
}

#include "Path.h"

struct FileWindow
{
	const static int item_y1 = 10 + 20;
	const static int item_x1 = 100;
	const static int item_sy = 20;
	const static int item_sx = 180;
	const static int item_spacing = 4;
	
	struct Touches
	{
		int numTouches;
		SDL_FingerID touchId[2];
		float moveY;
		
		Touches()
		{
			memset(this, 0, sizeof(*this));
		}
		
		void tick()
		{
			moveY = 0.f;
			
			if (!framework.windowIsActive)
			{
				reset();
			}
			else
			{
				for (auto & e : framework.events)
				{
					if (e.type == SDL_FINGERDOWN && numTouches < 2)
					{
						touchId[numTouches++] = e.tfinger.fingerId;
					}
					else if (e.type == SDL_FINGERUP)
					{
						for (int i = 0; i < numTouches; ++i)
						{
							if (touchId[i] == e.tfinger.fingerId)
							{
								for (int j = i + 1; j < numTouches; ++j)
									touchId[j - 1] = touchId[j];
								numTouches--;
								break;
							}
						}
					}
					else if (e.type == SDL_FINGERMOTION)
					{
						for (int i = 0; i < numTouches; ++i)
						{
							if (touchId[i] == e.tfinger.fingerId)
							{
								moveY += e.tfinger.dy;
								break;
							}
						}
					}
				}
			}
		}
		
		void reset()
		{
			numTouches = 0;
		}
	};
	
	Window window;
	std::vector<std::string> filenames;
	
	GraphEdit * graphEdit;
	
	int hoverIndex;
	int pressIndex;
	
	float scrollY;
	float desiredScrollY;
	
	Touches touches;
	
	FileWindow(GraphEdit * _graphEdit)
		: window("Files", 200, 600, true)
		, filenames()
		, graphEdit(_graphEdit)
		, hoverIndex(-1)
		, pressIndex(-1)
		, scrollY(0.f)
		, desiredScrollY(0.f)
	{
		window.setPosition(10, 100);
		
		std::vector<std::string> paths;
		paths = listFiles("vfxgraphs", false);
		std::sort(paths.begin(), paths.end());
		
		for (auto & path : paths)
		{
			if (Path::GetExtension(path, true) == "xml")
				filenames.push_back(path);
		}
	}
	
	void windowToView(const float x, const float y, float & viewX, float & viewY) const
	{
		viewX = x;
		viewY = y - scrollY;
	}
	
	void viewToItemIndex(const float x, const float y, int & itemIndex) const
	{
		itemIndex = (int)std::round((y - item_y1) / (item_sy + item_spacing));
		
		float viewX;
		float viewY;
		itemIndexToView(itemIndex, viewX, viewY);
		float dx = x - viewX;
		float dy = y - viewY;
		if (dx < -item_sx/2.f || dx > +item_sx/2.f ||
			dy < -item_sy/2.f || dy > +item_sy/2.f)
		{
			itemIndex = -1;
		}
		
		if (itemIndex < 0 || itemIndex >= filenames.size())
			itemIndex = -1;
	}
	
	void itemIndexToView(const int itemIndex, float & x, float & y) const
	{
		x = item_x1;
		y = itemIndex * (item_sy + item_spacing) + item_y1;
	}
	
	void tick(const float dt)
	{
		pushWindow(window);
		{
			touches.tick();
			
			if (touches.numTouches == 2)
			{
				const float amount = touches.moveY * 400.f;
				
				desiredScrollY += amount;
				scrollY += amount;
			}
			else if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			{
				desiredScrollY += mouse.dy;
				scrollY += mouse.dy;
			}
			else
			{
				if (keyboard.wentDown(SDLK_UP))
					desiredScrollY -= window.getHeight() * 2 / 3.f;
				if (keyboard.wentDown(SDLK_DOWN))
					desiredScrollY += window.getHeight() * 2 / 3.f;
				
				const float retain = std::pow(.01f, dt);
				
				const float minScrollY = - int(filenames.size()) * (item_sy + item_spacing) + window.getHeight() - item_y1;
				if (desiredScrollY > 0.f)
					desiredScrollY = lerp(0.f, desiredScrollY, retain);
				if (desiredScrollY < 0.f && desiredScrollY < minScrollY)
					desiredScrollY = lerp(minScrollY, desiredScrollY, retain);
				
				scrollY = lerp(desiredScrollY, scrollY, retain);
			}
			
			const float windowX = mouse.x;
			const float windowY = mouse.y;
			float viewX;
			float viewY;
			windowToView(windowX, windowY, viewX, viewY);
			
			int itemIndex;
			viewToItemIndex(viewX, viewY, itemIndex);
			
			hoverIndex = itemIndex;
			
			if (mouse.wentDown(BUTTON_LEFT))
				pressIndex = hoverIndex;
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				if (pressIndex == hoverIndex)
				{
					if (pressIndex >= 0 && pressIndex < filenames.size())
					{
						const std::string & filename = filenames[pressIndex];
						
						graphEdit->load(filename.c_str());
					}
				}
				
				pressIndex = -1;
			}
		}
		popWindow();
	}
	
	void draw()
	{
		pushWindow(window);
		{
			const int c = framework.windowIsActive ? 30 : 20;
			
			framework.beginDraw(c, c, c, 0);
			{
				setFont("calibri.ttf");
				
				gxTranslatef(0, scrollY, 0);
				
				if (touches.numTouches != 2 && hoverIndex != -1)
				{
					float hoverX;
					float hoverY;
					itemIndexToView(hoverIndex, hoverX, hoverY);
					
					setColor(0, 0, 255);
					hqBegin(HQ_FILLED_ROUNDED_RECTS);
					{
						hqFillRoundedRect(
							hoverX - item_sx/2.f,
							hoverY - item_sy/2.f,
							hoverX + item_sx/2.f,
							hoverY + item_sy/2.f, 4.f);
					}
					hqEnd();
				}
				
				beginTextBatch();
				{
					int x = item_x1;
					int y = item_y1;
					
					int itemIndex = 0;
					
					for (auto & filename : filenames)
					{
						setAlpha(255);
						if (itemIndex == pressIndex && pressIndex == hoverIndex)
							setLumi(50);
						else if (itemIndex == hoverIndex)
							setLumi(255);
						else
							setLumi(200);
						drawText(x, y, 14, 0, 0, "%s", Path::GetFileName(filename).c_str());
						
						y += item_sy + item_spacing;
						
						itemIndex++;
					}
				}
				endTextBatch();
			}
			framework.endDraw();
		}
		popWindow();
	}
};

void testVfxGraph()
{
	setAbout("This example shows Vfx Graph in action!");
	
	s_pcmDataCache.addPath("../../audioGraph-examples/data/testsounds", true, true, true);

	initAudioGraph();
	
	VfxGraph * vfxGraph = nullptr;
	
	RealTimeConnection rtc(vfxGraph);
	
	rtc.vfxGraphContext->addSystem<AudioGraphManager>(s_audioGraphMgr);
	rtc.vfxGraphContext->addSystem<AudioVoiceManager>(s_audioVoiceMgr);
	
	Graph_TypeDefinitionLibrary tdl;
	createVfxTypeDefinitionLibrary(tdl, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);
	
	GraphEdit graphEdit(GFX_SX, GFX_SY, &tdl, &rtc);
	
	//graphEdit.load("vfxgraphs/mlworkshopVc.xml");
	graphEdit.load("vfxgraphs/mlworkshopVd.xml");
	//graphEdit.load("vfxgraphs/polyphonic.xml");
	
	FileWindow fileWindow(&graphEdit);
	
	int editor = 0;

	do
	{
		framework.process();
		
		if (keyboard.isDown(SDLK_LGUI) && keyboard.wentDown(SDLK_e))
			editor = 1 - editor;
		
		fileWindow.tick(framework.timeStep);
		
		g_oscEndpointMgr.tick();
		
		// graph edit may change the graph, so we tick it first
		graphEdit.tick(framework.timeStep, editor != 0);
		
		s_audioGraphMgr->tickMain();
		
	#if ENABLE_AUDIO_RTE
		s_audioGraphMgr->tickEditor(GFX_SX, GFX_SY, framework.timeStep, editor != 1);
	#endif
		
		// update the vfx graph!
		if (rtc.vfxGraph != nullptr)
			rtc.vfxGraph->tick(GFX_SX, GFX_SY, framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the vfx graph. this may update image outputs, so tick it before the visualizers
			if (rtc.vfxGraph != nullptr)
				rtc.vfxGraph->draw();

			// update the visualizers before we draw the editor
			graphEdit.tickVisualizers(framework.timeStep);
			
			// draw the editor!
		
		#if ENABLE_AUDIO_RTE
			// draw the editor!
			if (editor == 0)
				graphEdit.draw();
			else
				s_audioGraphMgr->drawEditor(GFX_SX, GFX_SY);
		#else
			graphEdit.draw();
		#endif
			
		#if ENABLE_AUDIO_RTE
			makeActive(graphEdit.uiState, true, true);
			
			const int bottomY = GFX_SY - 90;
			const int topY = bottomY - kUiItemHeight /* switch button */ - 10 - kUiItemHeight * s_audioGraphMgr->files.size() /* audio graphs */ - kUiItemHeight /* vfx graph */;
			
			pushMenu("graph-select");
			{
				g_drawX = 10;
				g_drawY = topY;
				if (doAudioGraphSelect(*s_audioGraphMgr))
					editor = 1;
				if (doButton(graphEdit.documentInfo.filename.c_str()))
					editor = 0;
			}
			popMenu();
			
			pushMenu("editor-type");
			{
				g_drawX = 10;
				g_drawY = bottomY - kUiItemHeight;
				if (doButton("Switch editor type"))
					editor = 1 - editor;
			}
			popMenu();
			
			pushMenu("graph-instance-select");
			{
				g_drawY = topY;
				g_drawX += graphEdit.uiState->sx + 10;
				if (editor == 1)
				{
					static std::string activeAudioGraphInstanceName;
					doAudioGraphInstanceSelect(*s_audioGraphMgr, activeAudioGraphInstanceName, true);
				}
			}
			popMenu();
		#endif
			
			drawTestUi();
		}
		framework.endDraw();
		
		fileWindow.draw();
	} while (tickTestUi());

	delete vfxGraph;
	vfxGraph = nullptr;
	
	shutAudioGraph();
}

static void initAudioGraph()
{
	Assert(s_audioMutex == nullptr);
	s_audioMutex = new AudioMutex();
	s_audioMutex->init();
	
	Assert(s_audioVoiceMgr == nullptr);
	s_audioVoiceMgr = new AudioVoiceManagerBasic();
	s_audioVoiceMgr->init(s_audioMutex, 64);
	s_audioVoiceMgr->outputStereo = true;
	
	Assert(s_audioGraphMgr == nullptr);
#if ENABLE_AUDIO_RTE
	s_audioGraphMgr = new AudioGraphManager_RTE(GFX_SX, GFX_SY);
#else
	s_audioGraphMgr = new AudioGraphManager_Basic(true);
#endif
	s_audioGraphMgr->init(s_audioMutex, s_audioVoiceMgr);

	s_audioGraphMgr->context->addObject(&s_pcmDataCache, "PCM data cache");
	
	Assert(s_audioUpdateHandler == nullptr);
	s_audioUpdateHandler = new AudioUpdateHandler();
	s_audioUpdateHandler->init(s_audioMutex, nullptr, 0);
	s_audioUpdateHandler->audioGraphMgr = s_audioGraphMgr;
	s_audioUpdateHandler->voiceMgr = s_audioVoiceMgr;
	
	Assert(s_paObject == nullptr);
	s_paObject = new PortAudioObject();
	s_paObject->init(SAMPLE_RATE, 2, 1, AUDIO_UPDATE_SIZE, s_audioUpdateHandler);
}

static void shutAudioGraph()
{
	if (s_paObject != nullptr)
	{
		s_paObject->shut();
		delete s_paObject;
		s_paObject = nullptr;
	}
	
	if (s_audioUpdateHandler != nullptr)
	{
		s_audioUpdateHandler->shut();
		delete s_audioUpdateHandler;
		s_audioUpdateHandler = nullptr;
	}
	
	if (s_audioGraphMgr != nullptr)
	{
		s_audioGraphMgr->shut();
		delete s_audioGraphMgr;
		s_audioGraphMgr = nullptr;
	}
	
	if (s_audioVoiceMgr != nullptr)
	{
		s_audioVoiceMgr->shut();
		delete s_audioVoiceMgr;
		s_audioVoiceMgr = nullptr;
	}
	
	if (s_audioMutex != nullptr)
	{
		s_audioMutex->shut();
		delete s_audioMutex;
		s_audioMutex = nullptr;
	}
}
