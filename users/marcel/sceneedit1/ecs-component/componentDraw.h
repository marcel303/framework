#pragma once

#include "Vec3.h"
#include <stdint.h>

class Mat4x4;

class SceneNodeComponent;

struct ComponentDrawColor
{
	float rgba[4];
};

struct ComponentDraw
{
	bool isSelected = false;
	
	SceneNodeComponent * sceneNodeComponent;
	
	virtual ComponentDrawColor makeColor(const int r, const int g, const int b, const int a = 255) = 0;
	virtual void color(const ComponentDrawColor & color) = 0;
	
	virtual void line(Vec3Arg from, Vec3Arg to) = 0;
	virtual void line(
		float x1, float y1, float z1,
		float x2, float y2, float z2) = 0;
	
	virtual void lineRect(float x1, float y1, float x2, float y2) = 0;
	virtual void lineCircle(float x, float y, float radius) = 0;
	virtual void lineCube(Vec3Arg position, Vec3Arg extents) = 0;
	
	virtual void pushMatrix() = 0;
	virtual void popMatrix() = 0;
	virtual void multMatrix(const Mat4x4 & m) = 0;
	virtual void translate(Vec3Arg t) = 0;
	virtual void rotate(float angle, Vec3Arg axis) = 0;
};

