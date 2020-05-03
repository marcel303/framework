#pragma once

#include <map>
#include <string>
#include <vector>

class LineReader;
struct Scene;
struct Template;

bool parseSceneFromLines(const TypeDB & typeDB, std::vector<std::string> & lines, Scene & out_scene);

bool parseSceneObjectFromLines(const TypeDB & typeDB, LineReader & line_reader, Scene & out_scene, std::map<std::string, Template> & templates);
bool parseSceneObjectStructureFromLines(const TypeDB & typeDB, LineReader & line_reader, Scene & out_scene, std::map<std::string, Template> & templates);
