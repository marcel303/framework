#pragma once

class Mat4x4;
class Vec3;

class Color;

void fillCylinder(
	const Vec3 & position,
	const int axis,
	const float radius1,
	const float radius2,
	const float length,
	const bool mirrored);

void drawAxisArrow(
	const Vec3 & position,
	const int axis,
	const float radius,
	const float length,
	const float top_radius,
	const float top_length,
	const bool mirrored);

void drawSelectionBox(
	const Mat4x4 & objectToWorld,
	const Vec3 & min,
	const Vec3 & max,
	const Color & color);
