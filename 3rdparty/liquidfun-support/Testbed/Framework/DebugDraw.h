#pragma once

#include "Box2D/Box2D.h"
#include "framework.h"
#include "StringEx.h"
#include <stdarg.h>

struct DebugDraw : public b2Draw
{
	b2Transform xform;
	
	DebugDraw()
	{
		xform.SetIdentity();
	}

	void pushTransform()
	{
		gxPushMatrix();
		gxTranslatef(xform.p.x, xform.p.y, 0);
		gxRotatef(xform.q.GetAngle(), 0, 0, 1);
	}
	
	void popTransform()
	{
		gxPopMatrix();
	}
	
	virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			gxBegin(GX_LINE_LOOP);
			{
				for (int i = 0; i < vertexCount; ++i)
					gxVertex2f(vertices[i].x, vertices[i].y);
			}
			gxEnd();
		}
		popTransform();
	}

	virtual void DrawFlatPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
	{
		setColor(color.r, color.g, color.b, color.a);
		gxBegin(GX_TRIANGLE_FAN);
		{
			for (int i = 0; i < vertexCount; ++i)
				gxVertex2f(vertices[i].x, vertices[i].y);
		}
		gxEnd();
	}

	virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			gxBegin(GX_TRIANGLE_FAN);
			{
				for (int i = 0; i < vertexCount; ++i)
					gxVertex2f(vertices[i].x, vertices[i].y);
			}
			gxEnd();
		}
		popTransform();
	}

	virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			drawCircle(center.x, center.y, radius, 100);
		}
		popTransform();
	}

	virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			fillCircle(center.x, center.y, radius, 100);
		}
		popTransform();
	}

	virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			drawLine(p1.x, p1.y, p2.x, p2.y);
		}
		popTransform();
	}

	virtual void DrawTransform(const b2Transform& xf) override
	{
		xform = xf;
	}

	virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override
	{
		pushTransform();
		{
			setColorf(color.r, color.g, color.b, color.a);
			fillCircle(p.x, p.y, size / 25.f, 20);
		}
		popTransform();
	}

	void DrawString(int x, int y, const char* format, ...)
	{
		char text[1024];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, sizeof(text), format, args);
		va_end(args);
		
		gxPushMatrix();
		gxLoadIdentity();
		setColor(140, 255, 200);
		drawText(x, y, 12, +1, -1, "%s", text);
		gxPopMatrix();
	}

	void DrawString(const b2Vec2& p, const char* format, ...)
	{
		char text[1024];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, sizeof(text), format, args);
		va_end(args);
		
		gxPushMatrix();
		gxLoadIdentity();
		drawText(p.x, p.y, 12, +1, -1, "%s", text);
		gxPopMatrix();
	}

	void DrawAABB(b2AABB* aabb, const b2Color& color)
	{
		setColorf(color.r, color.g, color.b, color.a);
		drawRect(
			aabb->lowerBound.x, aabb->lowerBound.y,
			aabb->upperBound.x, aabb->upperBound.y);
	}
	
	virtual void DrawParticles(const b2Vec2* positionBuffer, float32 radius, const b2ParticleColor* colorBuffer, int particleCount) override
	{
		setColor(colorWhite);
		
		radius /= 1.6f; // we get less overlapping circles when we make the radius a little smaller
		
		// pre-calculate offsets for drawing circles with the desired radius
		
		const int kNumSegments = 8;
		float circle[kNumSegments + 1][2];
		for (int s = 0; s < kNumSegments + 1; ++s)
		{
			const float angle = float(2.0 * M_PI) * s / float(kNumSegments);
			circle[s][0] = cosf(angle) * radius;
			circle[s][1] = sinf(angle) * radius;
		}
		
		gxBegin(GX_LINES);
		{
			for (int i = 0; i < particleCount; ++i)
			{
				if (colorBuffer != nullptr)
					gxColor3ub(colorBuffer[i].r, colorBuffer[i].g, colorBuffer[i].b);
				
				const b2Vec2& position = positionBuffer[i];
				
				for (int s = 0; s < kNumSegments; ++s)
				{
					gxVertex2f(position.x + circle[s + 0][0], position.y + circle[s + 0][1]);
					gxVertex2f(position.x + circle[s + 1][0], position.y + circle[s + 1][1]);
				}
			}
		}
		gxEnd();
	}

	void Flush()
	{
	}
};
