#pragma once

#include <string>

struct SceneNode;

bool node_to_clipboard_text(const SceneNode & node, std::string & text);
bool node_from_clipboard_text(const char * text, SceneNode & node);
