#pragma once

#include <string>
#include <vector>

// forward declarations

struct Scene;
struct SceneNode;

class LineReader;
class LineWriter;

// copy/paste utilities

bool copySceneNodeToLines(const TypeDB & typeDB, const SceneNode & node, LineWriter & line_writer, int indent);
bool pasteSceneNodeFromLines(const TypeDB & typeDB, LineReader & line_reader, SceneNode & node);

bool copySceneNodeTreeToLines(const TypeDB & typeDB, const Scene & scene, const int rootNodeId, LineWriter & line_writer, int indent);
bool pasteSceneNodeTreeFromLines(const TypeDB & typeDB, LineReader & line_reader, Scene & scene);

// clipboard utilities

std::vector<std::string> linesFromClipboard();
