#pragma once

#include "template.h"
#include <map>
#include <string>
#include <vector>

struct Scene;

bool parseSceneFromLines(std::vector<std::string> & lines, Scene & out_scene);

bool parseSceneObjectFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates);
bool parseSceneObjectStructureFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates);
