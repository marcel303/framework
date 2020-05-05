#pragma once

#include <string>

struct SceneNode;

bool node_to_text(const SceneNode & node, std::string & text);
bool node_from_text(const char * text, SceneNode & node);
