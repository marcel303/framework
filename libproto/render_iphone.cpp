#include <cmath>
#include "input.h"
#include "Mat4x4.h"
#include "OpenGLCompat.h"
#include "render_iphone.h"
#include "SpriteGfx.h"
#include "Types.h"

#ifndef RENDER_IOS_NOTILT
#include "TiltReader.h"
#endif

#define BLEND SetBlendEnabled(color.a != 1.0f)
#define TILT_ANGLE 35.0f

static void SetBlendEnabled(bool enabled);

void RenderIphone::Init(int sx, int sy)
{
	mSize[0] = sx;
	mSize[1] = sy;
	mSpriteGfx = new SpriteGfx(1000, 3000);
#ifndef RENDER_IOS_NOTILT
	mTiltReader = new TiltReader();
#endif
}

void RenderIphone::MakeCurrent()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, mSize[0], mSize[1], 0.0f, -1000.0f, +1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (mSize[0] > mSize[1])
	{
		glTranslatef(mSize[0], 0.0f, 0.0f);
		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	}
}

void RenderIphone::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderIphone::Present()
{
	mSpriteGfx->Flush();
	
	//
	
#ifndef RENDER_IOS_NOTILT
	Vec3 tilt = mTiltReader->GravityVector_get();
	Mat4x4 matRot;
	Vec3 tilt1 = Vec3(-tilt[1], -tilt[0], tilt[2]);
	matRot.MakeRotationX(Calc::DegToRad(TILT_ANGLE));
	tilt1 = matRot.Mul(tilt1);
	Vec2F tilt2;
	tilt2 = Vec2F(tilt1[0], tilt1[1]);

/*	Vec2F tilt1 = mTiltReader->DirectionXY_get();
	Vec2F tilt2 = Vec2F::FromAngle(tilt1.ToAngle() + Calc::mPI2) * tilt1.Length_get();
	tilt2[1] = -tilt2[1];*/
	gKeyboard.TiltSet(tilt2);
#endif
}

void RenderIphone::Fade(int amount)
{
	glBlendEquationOES(GL_FUNC_REVERSE_SUBTRACT_OES);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	
	const float x1 = 0.0f;
	const float y1 = 0.0f;
	const float x2 = mSize[1];
	const float y2 = mSize[0];

	const float c = amount / 255.0f;
	
	const int rgba = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, c).rgba;
	
	mSpriteGfx->Reserve(4, 6);
	mSpriteGfx->WriteBegin();
	mSpriteGfx->WriteVertex(x1, y1, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x2, y1, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x2, y2, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x1, y2, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteIndex3(0, 1, 2);
	mSpriteGfx->WriteIndex3(0, 2, 3);
	mSpriteGfx->WriteEnd();
	
	mSpriteGfx->Flush();
	
	glBlendEquationOES(GL_FUNC_ADD_OES);
}

void RenderIphone::Point(float x, float y, Color color)
{
	float s = 1.0f;
	
	Quad(x - s, y - s, x + s, y + s, color);
}

void RenderIphone::Line(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;

	DoLine(x1, y1, x2, y2, color);
	
	mSpriteGfx->Flush();
}

void RenderIphone::Circle(float x, float y, float r, Color color)
{
	r = std::floor(r);

	BLEND;

	const int rgba = SpriteColor_MakeF(color.r, color.g, color.b, color.a).rgba;
	
	const int n = (int)std::ceil(r * 3.14f);
	
	mSpriteGfx->Reserve(1 + n, n * 3);
	mSpriteGfx->WriteBegin();
	mSpriteGfx->WriteVertex(x, y, rgba, 0.0f, 0.0f);
	for (int i = 0; i < n; ++i)
	{
		const float a = i / (float)n * (float)M_PI * 2.0f;
		const float x2 = x + std::sin(a) * r;
		const float y2 = y + std::cos(a) * r;
		mSpriteGfx->WriteVertex(x2, y2, rgba, 0.0f, 0.0f);
		mSpriteGfx->WriteIndex3(0, 1 + i, 1 + ((i + 1) % n));
	}
	mSpriteGfx->WriteEnd();
	
	mSpriteGfx->Flush();
}

void RenderIphone::Arc(float x, float y, float angle1, float angle2, float r, Color color)
{
	BLEND;
	
	r = std::floor(r);

	const int n = (int)std::ceil(r * 3.14f);
	
	for (int i = 0; i < n; ++i)
	{
		const float a1 = angle1 + (angle2 - angle1) / (float)n * (i + 0);
		const float a2 = angle1 + (angle2 - angle1) / (float)n * (i + 1);
		const float x1 = x + std::sin(a1) * r;
		const float y1 = y + std::cos(a1) * r;
		const float x2 = x + std::sin(a2) * r;
		const float y2 = y + std::cos(a2) * r;
		DoLine(x1, y1, x2, y2, color);
	}
	
	mSpriteGfx->Flush();
}

void RenderIphone::Quad(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;
	
	const int rgba = SpriteColor_MakeF(color.r, color.g, color.b, color.a).rgba;
	
	mSpriteGfx->Reserve(4, 6);
	mSpriteGfx->WriteBegin();
	mSpriteGfx->WriteVertex(x1, y1, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x2, y1, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x2, y2, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(x1, y2, rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteIndex3(0, 1, 2);
	mSpriteGfx->WriteIndex3(0, 2, 3);
	mSpriteGfx->WriteEnd();
	
	mSpriteGfx->Flush();
}

void RenderIphone::QuadTex(float x1, float y1, float x2, float y2, Color color, float u1, float v1, float u2, float v2)
{
	BLEND;
	
	const int rgba = SpriteColor_MakeF(color.r, color.g, color.b, color.a).rgba;
	
	mSpriteGfx->Reserve(4, 6);
	mSpriteGfx->WriteBegin();
	mSpriteGfx->WriteVertex(x1, y1, rgba, u1, v1);
	mSpriteGfx->WriteVertex(x2, y1, rgba, u2, v1);
	mSpriteGfx->WriteVertex(x2, y2, rgba, u2, v2);
	mSpriteGfx->WriteVertex(x1, y2, rgba, u1, v2);
	mSpriteGfx->WriteIndex3(0, 1, 2);
	mSpriteGfx->WriteIndex3(0, 2, 3);
	mSpriteGfx->WriteEnd();
	
	mSpriteGfx->Flush();
}

void RenderIphone::Text(float x, float y, Color color, const char* format, ...)
{
	// nop
}

void RenderIphone::DoLine(float x1, float y1, float x2, float y2, Color color)
{
	const int rgba = SpriteColor_MakeF(color.r, color.g, color.b, color.a).rgba;
	
	const Vec2F delta(x2 - x1, y2 - y1);
	const float angle = delta.ToAngle();
	const float tangentAngle = angle + Calc::mPI2;
	const float s = 0.5f;
	const Vec2F tangent = Vec2F::FromAngle(tangentAngle) * s;
	
	const Vec2F p1(x1, y1);
	const Vec2F p2(x2, y2);
	
	const Vec2F p11 = p1 + tangent;
	const Vec2F p21 = p2 + tangent;
	const Vec2F p22 = p2 - tangent;
	const Vec2F p12 = p1 - tangent;
	
	mSpriteGfx->Reserve(4, 6);
	mSpriteGfx->WriteBegin();
	mSpriteGfx->WriteVertex(p11[0], p11[1], rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(p21[0], p21[1], rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(p22[0], p22[1], rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteVertex(p12[0], p12[1], rgba, 0.0f, 0.0f);
	mSpriteGfx->WriteIndex3(0, 1, 2);
	mSpriteGfx->WriteIndex3(0, 2, 3);
	mSpriteGfx->WriteEnd();
}

static void SetBlendEnabled(bool enabled)
{
	if (enabled)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}
