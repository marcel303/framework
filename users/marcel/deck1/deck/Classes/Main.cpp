#include "Game.h"
#include "Input.h"
#include "Main.h"
#include "render.h"
#include "render_iphone.h"
#include "World.h"

Main gMain;

Main::Main()
{
}

void Main::Init()
{
	Assert(gGame == 0);
	Assert(gWorld == 0);
	
	gInput = new Input();
	gRender = new RenderIphone();
	gRender->Init(320, 480);
	gGame = new Game();
	gWorld = new World();
}

void Main::Shut()
{
	delete gWorld;
	gWorld = 0;
	
	delete gGame;
	gGame = 0;
	
	delete gRender;
	gRender = 0;
	
	delete gInput;
	gInput = 0;
}
