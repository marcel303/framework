#include "helpers2.h"
#include "scene.h"
#include "scene_fromText.h"

#include "Log.h"
#include "Path.h"
#include "TextIO.h"

#include "framework.h" // setupPaths

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	registerBuiltinTypes(g_typeDB);
	registerComponentTypes(g_typeDB);

	// load scene description text file
	
	const char * path = "textfiles/scene-v1.txt";
	const std::string basePath = Path::GetDirectory(path);

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(path, lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return -1;
	}
	
	// parse the scene
	
	Scene scene;
	scene.createRootNode();
	
	LineReader line_reader(lines, 0, 0);
	
	if (!parseSceneFromLines(g_typeDB, line_reader, basePath.c_str(), scene))
	{
		LOG_ERR("failed to parse scene from lines", 0);
		return -1;
	}

	LOG_INF("success!", 0);
	
	// empty the scene before we quit the app
	
	scene.freeAllNodesAndComponents();

	return 0;
}
