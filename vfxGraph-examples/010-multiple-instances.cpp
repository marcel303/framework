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
#include "graph.h"
#include "graph_typeDefinitionLibrary.h"
#include "vfxGraph.h"
#include <algorithm>

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 512;

const int GRID_SX = 4;
const int GRID_SY = 2;

const int ELEM_SX = GFX_SX / GRID_SX;
const int ELEM_SY = GFX_SY / GRID_SY;

//

struct GridElem
{
	VfxGraph * vfxGraph;

	GridElem()
		: vfxGraph(nullptr)
	{
	}

	~GridElem()
	{
		delete vfxGraph;
		vfxGraph = nullptr;
	}
};

struct Grid
{
	GridElem elem[GRID_SX][GRID_SY];
	
	void load(const Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
	{
		const char * filenames[] =
		{
			"testPetals.xml",
			"drawTest.xml",
			"drawTest2.xml",
			"draw3dTest2.xml",
			"testOscilloscope3.xml"
		};
		
		for (int x = 0; x < GRID_SX; ++x)
		{
			for (int y = 0; y < GRID_SY; ++y)
			{
				const int numFilenames = sizeof(filenames) / sizeof(filenames[0]);
				const int index = rand() % numFilenames;
				
				const char * filename = filenames[index];

				Graph graph;
				graph.load(filename, &typeDefinitionLibrary);

				VfxGraph * vfxGraph = constructVfxGraph(graph, &typeDefinitionLibrary);
				
				elem[x][y].vfxGraph = vfxGraph;
			}
		}
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.enableRealTimeEditing = true;
	
	if (framework.init(GFX_SX, GFX_SY))
	{
		Graph_TypeDefinitionLibrary typeDefinitionLibrary;
		createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
		
		Grid * grid = new Grid();
		grid->load(typeDefinitionLibrary);

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (mouse.wentDown(BUTTON_LEFT))
			{
				delete grid;
				grid = nullptr;
				
				//
				
				grid = new Grid();
				grid->load(typeDefinitionLibrary);
			}
			
			const float timeStep = std::min(framework.timeStep, 1.f / 15.f);

			for (int x = 0; x < GRID_SX; ++x)
				for (int y = 0; y < GRID_SY; ++y)
					grid->elem[x][y].vfxGraph->tick(ELEM_SX, ELEM_SY, timeStep);

			framework.beginDraw(0, 0, 0, 0);
			{
				for (int x = 0; x < GRID_SX; ++x)
				{
					for (int y = 0; y < GRID_SY; ++y)
					{
						gxPushMatrix();
						{
							gxTranslatef(x * ELEM_SX, y * ELEM_SY, 0);

							grid->elem[x][y].vfxGraph->draw();
						}
						gxPopMatrix();
					}
				}
			}
			framework.endDraw();
		}

		delete grid;
		grid = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
