#include "framework.h"
#include "helpers.h"
#include "Log.h"
#include "scene.h"
#include "template.h"
#include "scene_fromText.h"
#include "TextIO.h"
#include <map>
#include <string>
#include <vector>

bool test_scenefiles()
{
	if (!framework.init(640, 480))
		return false;

	registerBuiltinTypes();
	registerComponentTypes();

	// load scene description text file
	
	changeDirectory("textfiles"); // todo : use a nicer solution to handling relative paths

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load("scene-v1.txt", lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return false;
	}
	
	Scene scene;
	scene.createRootNode();
	
	if (!parseSceneFromLines(g_typeDB, lines, scene))
	{
		LOG_ERR("failed to parse scene from lines", 0);
		return false;
	}

	LOG_DBG("success!", 0);
	
	exit(0);

	return true;
}
