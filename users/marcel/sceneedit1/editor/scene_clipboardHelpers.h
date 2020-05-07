#pragma once

#include <string>

// forward declarations

struct Scene;
struct SceneNode;

class LineReader;
class LineWriter;

// copy/paste utilities

bool copySceneNodeToText(const TypeDB & typeDB, const SceneNode & node, LineWriter & line_writer, int indent);
bool pasteSceneNodeFromText(const TypeDB & typeDB, LineReader & line_reader, SceneNode & node);

bool copySceneNodeTreeToText(const TypeDB & typeDB, const Scene & scene, const int rootNodeId, LineWriter & line_writer, int indent);
bool pasteSceneNodeTreeFromText(const TypeDB & typeDB, LineReader & line_reader, Scene & scene);

