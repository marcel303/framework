#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "StringEx.h"
#include "ui.h"
#include "vfxGraph.h"
#include "vfxGraphManager.h"
#include "vfxGraphRealTimeConnection.h"
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxUi.h"

// todo : let inputs set if visible in node editor or not
// if not, node editor behavior changes:
// - node gets highlighted when dragging a link endpoint over it
// - a list pops up asking to connect to one of the inputs when released

// todo : changed files in framework
// todo : + use corrrect osc sheet file
// todo : + detect osc sheet changes

const int VIEW_SX = 700;
const int VIEW_SY = 800;

//

static VfxGraphManager * s_vfxGraphMgr = nullptr;

struct Creature
{
	VfxGraphInstance * vfxInstance = nullptr;
	
	Vec2 currentPos;
	Vec2 desiredPos;
	
	float moveTimer = 0.f;
	
	void init(const int id)
	{
		const char * filenames[2] =
		{
			"test1.xml",
			"test2.xml"
		};
		
		const char * filename = filenames[id % 2];
		
		vfxInstance = s_vfxGraphMgr->createInstance(filename, 64, 64);
		
		vfxInstance->vfxGraph->memory.setMems("id", String::FormatC("%d", id + 1).c_str());
		
		vfxInstance->vfxGraph->memory.setMemf("pos", currentPos[0], currentPos[1]);
	}
	
	void shut()
	{
		s_vfxGraphMgr->free(vfxInstance);
		Assert(vfxInstance == nullptr);
	}
	
	void tick(const float dt)
	{
		moveTimer -= dt;
		
		if (moveTimer <= 0.f)
		{
			moveTimer = random(0.f, 10.f);
			
			desiredPos.Set(random(-10.f, +10.f), random(-10.f, +10.f));
		}
		
		const float retainPerSecond = .7f;
		const float retain = powf(retainPerSecond, dt);
		const float attain = 1.f - retain;
		
		currentPos = currentPos * retain + desiredPos * attain;
		
		vfxInstance->vfxGraph->memory.setMemf("pos", currentPos[0], currentPos[1]);
	}
	
	void draw() const
	{
		Assert(vfxInstance->texture != 0);
		gxSetTexture(vfxInstance->texture);
		{
			pushBlend(BLEND_OPAQUE);
			
			setColor(colorWhite);
			drawRect(
				currentPos[0] - 1.f, currentPos[1] - 1.f,
				currentPos[0] + 1.f, currentPos[1] + 1.f);
			
			popBlend();
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
			gxScalef(40, 40, 1);
			
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
#else
	changeDirectory(SDL_GetBasePath());
#endif
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	// create vfx type definition library
	
	Graph_TypeDefinitionLibrary typeDefinitionLibrary;
	createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
	
	// create vfx graph manager
	
	//VfxGraphManager_Basic vfxGraphMgr(true);
	VfxGraphManager_RTE vfxGraphMgr(VIEW_SX, VIEW_SY);
	s_vfxGraphMgr = &vfxGraphMgr;
	
	// setup world

	World world;
	world.init();
	
	// select instance for editing
	
	vfxGraphMgr.selectInstance(world.creatures[0].vfxInstance);
	
	UiState uiState;
	uiState.x = 20;
	uiState.y = VIEW_SY - 140;
	uiState.sx = 200;
	std::string activeInstanceName;
	
	auto doMenus = [&](const bool doActions, const bool doDraw)
	{
		makeActive(&uiState, doActions, doDraw);
		pushMenu("vfx select");
		doVfxGraphInstanceSelect(vfxGraphMgr, activeInstanceName);
		popMenu();
	};
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		// update menus
		
		doMenus(true, false);
		
		inputIsCaptured |= uiState.activeElem != nullptr;
		
		// update the graph editor
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(VIEW_SX, VIEW_SY, dt, inputIsCaptured);
		
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
			
			vfxGraphMgr.drawEditor(VIEW_SX, VIEW_SY);
			
			// draw the menus
			
			doMenus(false, true);
		}
		framework.endDraw();
	}
	
	world.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
