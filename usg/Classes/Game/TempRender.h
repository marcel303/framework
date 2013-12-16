#pragma once

#include "Forward.h"
#include "libgg_forward.h"
#include "SpriteGfx.h"
#include "Types.h"

enum TextAlignment
{
	TextAlignment_Left,
	TextAlignment_Right,
	TextAlignment_Center,
	TextAlignment_Top,
	TextAlignment_Bottom
};

void TempRenderBegin(SpriteGfx* gfx);

void RenderGrid(const Vec2F& pos, const Vec2F& size, int segX, int segY, SpriteColor color, const float* texCoord1, const float* texCoord2);
void RenderRect(const Vec2F& pos, const Vec2F& size, float r, float g, float b, float alpha, const AtlasImageMap* image);
void RenderRect(const Vec2F& pos, const Vec2F& size, float r, float g, float b, const AtlasImageMap* image);
void RenderRect(const Vec2F& pos, const Vec2F& size, SpriteColor color, const AtlasImageMap* image);
void RenderRect(const Vec2F& pos, float r, float g, float b, const AtlasImageMap* image);
void RenderRect(const Vec2F& pos, SpriteColor color, const AtlasImageMap* image);
void RenderRect(const Vec2F& pos, const Vec2F& size, SpriteColor color, const float* texCoord1, const float* texCoord2);
void RenderQuad(const Vec2F& min, const Vec2F& max, float r, float g, float b, float alpha, const AtlasImageMap* image);
void RenderQuad(const Vec2F& min, const Vec2F& max, float r, float g, float b, const AtlasImageMap* image);
void RenderQuad(const Vec2F& pos, float size, float r, float g, float b, const AtlasImageMap* image);
void RenderBeam(const Vec2F& p1, const Vec2F& p2, float breadth, const SpriteColor& color, const AtlasImageMap* corner1, const AtlasImageMap* corner2, const AtlasImageMap* body, int imageDisplayScale);
void RenderBeamEx(float scale, const Vec2F& p1, const Vec2F& p2, const SpriteColor& color, const AtlasImageMap* corner1, const AtlasImageMap* corner2, const AtlasImageMap* body, int imageDisplayScale);
void RenderText(Vec2F p, Vec2F s, const FontMap* font, SpriteColor color, TextAlignment alignmentX, TextAlignment alignmentY, bool clamp, const char* text);
void RenderText(const Mat3x2& mat, const FontMap* font, SpriteColor color, const char* text);
void RenderText(const Mat3x2& mat, const FontMap* font, SpriteColor color, TextAlignment alignmentX, TextAlignment alignmentY, const char* text);
void RenderElectricity(const Vec2F& pos1, const Vec2F& pos2, float maxDeviation, int interval, float breadth, const AtlasImageMap* image);
void RenderSweep(const Vec2F& pos, float angle1, float angle2, float size, SpriteColor color, int texture = -1);
void RenderSweep2(const Vec2F& pos, float angle1, float angle2, float size1, float size2, SpriteColor color, int texture = -1);
void RenderCurve(Vec2F pos1, Vec2F tan1, Vec2F pos2, Vec2F tan2, float breadth, SpriteColor color, int accurary, int texture = -1);

void DrawRect(SpriteGfx& gfx, float x, float y, float sx, float sy, float u, float v, float su, float sv, int rgba);
void DrawBarH(SpriteGfx& gfx, Vec2F min, Vec2F max, const AtlasImageMap* image, float value, int rgba);
void DrawBarV(SpriteGfx& gfx, Vec2F min, Vec2F max, const AtlasImageMap* image, float value, int rgba);

void RenderBouncyText(const Vec2F& p1, const Vec2F& delta, const char* text);
void DrawPieQuadThingy(Vec2F min, Vec2F max, float angle1, float angle2, const AtlasImageMap* image, SpriteColor color);
