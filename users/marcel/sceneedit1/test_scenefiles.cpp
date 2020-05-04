#include "framework.h"
#include "helpers.h"
#include "helpers2.h"
#include "Log.h"
#include "Path.h"
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
	
	const char * path = "textfiles/scene-v1.txt";
	const std::string basePath = Path::GetDirectory(path);

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(path, lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return false;
	}
	
	Scene scene;
	scene.createRootNode();
	
	if (!parseSceneFromLines(g_typeDB, lines, basePath.c_str(), scene))
	{
		LOG_ERR("failed to parse scene from lines", 0);
		return false;
	}

	LOG_DBG("success!", 0);

	return true;
}
