// sceneedit
#define DEFINE_COMPONENT_TYPES
#include "components/gltfComponent.h"
#include "components/modelComponent.h"
#include "components/transformComponent.h"
#include "helpers2.h"

// ecs-scene
#include "helpers.h"
#include "scene.h"
#include "sceneIo.h"

// ecs-component
#include "componentType.h"
#include "componentTypeDB.h"

// framework
#include "framework.h"
#include "framework-camera.h"

// libreflection
#include "reflection.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	// initialize framework and make sure real-time editing is enabled, so we can detect file changes
	
	framework.enableRealTimeEditing = true;
	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;

	// initialize type database
	
	TypeDB typeDB;
	registerBuiltinTypes(typeDB);
	registerComponentTypes(typeDB, g_componentTypeDB);
	
	// load the scene
	
	Scene scene;
	
	const char * path = "textfiles/scene-v1.txt";
	
	scene.createRootNode();
	if (parseSceneFromFile(typeDB, path, scene) == false || scene.initComponents() == false)
	{
		scene.freeAllNodesAndComponents();
		scene.createRootNode();
	}

	Camera3d camera;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		// check if the scene file has changed. if so, reload the scene
		
		if (framework.fileHasChanged(path))
		{
			scene.freeAllNodesAndComponents();

			scene.createRootNode();
			if (parseSceneFromFile(typeDB, path, scene) == false || scene.initComponents() == false)
			{
				scene.freeAllNodesAndComponents();
				scene.createRootNode();
			}
		}

		// tick all of the registered component managers
		
		for (auto * type : g_componentTypeDB.componentTypes)
		{
			type->componentMgr->tick(framework.timeStep);
		}
		
		g_transformComponentMgr.calculateTransforms(scene);
		
		camera.tick(framework.timeStep, true);

		// draw
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw model component type properties (for reference while editing)
		
			auto * componentType = g_componentTypeDB.findComponentType("ModelComponent");
		
			if (componentType != nullptr)
			{
				int y = 0;
				
				setColor(colorWhite);
				for (auto * member = componentType->members_head; member != nullptr; member = member->next)
					drawText(4, y += 18, 12, +1, +1, "%s", member->name);
			}
			
			// draw models
			
			projectPerspective3d(70.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			pushDepthTest(true, DEPTH_LESS);
			{
				// draw models
				
				g_modelComponentMgr.draw();
				
				g_gltfComponentMgr.drawOpaque();
				g_gltfComponentMgr.drawTranslucent();
			}
			popDepthTest();
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	scene.freeAllNodesAndComponents();

	framework.shutdown();

	return 0;
}
