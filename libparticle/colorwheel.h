#pragma once

class ColorWheel
{
	static void getTriangleAngles(float a[3]);
	static void getTriangleCoords(float xy[6]);

public:
	static const int size = 100;
	static const int wheelThickness = 20;
	static const int barHeight = 16;
	static const int barX1 = -size;
	static const int barY1 = +size + 4;
	static const int barX2 = +size;
	static const int barY2 = +size + 4 + barHeight;

	float hue;
	float baryWhite;
	float baryBlack;
	float opacity;

	enum State
	{
		kState_Idle,
		kState_SelectHue,
		kState_SelectSatValue,
		kState_SelectOpacity
	};

	State state;

	ColorWheel();

	void tick(const float mouseX, const float mouseY, const bool mouseWentDown, const bool mouseIsDown, const float dt);
	void draw();

	int getSx() const { return size * 2; }
	int getSy() const { return size * 2 + 16; }

	bool intersectWheel(const float x, const float y, float & hue) const;
	bool intersectTriangle(const float x, const float y, float & saturation, float & value) const;
	bool intersectBar(const float x, const float y, float & t) const;

	void fromColor(const float r, const float g, const float b, const float a);
	void toColor(const float hue, const float saturation, const float value, float & r, float & g, float & b, float & a) const;
	void toColor(float & r, float & g, float & b, float & a) const;
};
