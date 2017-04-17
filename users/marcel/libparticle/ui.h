#pragma once

extern void drawRectCheckered(float x1, float y1, float x2, float y2, float scale);
extern void drawUiCircle(const float x, const float y, const float radius, const float r, const float g, const float b, const float a);

extern void hlsToRGB(float hue, float lum, float sat, float & r, float & g, float & b);
extern void rgbToHSL(float r, float g, float b, float & hue, float & lum, float & sat);
