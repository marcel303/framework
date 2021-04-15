#pragma once

class Vec3;

void drawCylinder(
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
