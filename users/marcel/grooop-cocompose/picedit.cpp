#pragma once

#include "Calc.h"
#include "picedit.h"
#include "Traveller.h"

PicEdit::DrawShared::DrawShared()
{
	memset(this, 0, sizeof(*this));
}

void PicEdit::drawBegin(const bool fixed, const float x, const float y)
{
	drawState.fixed = fixed;
	drawState.beginX = x;
	drawState.beginY = y;
	drawState.endX = x;
	drawState.endY = y;

	if (drawType == kDrawType_Brush)
	{
		traveller.Setup(1.f, drawMoveCB, this);
		traveller.Begin(x, y);
	}
}

void PicEdit::drawEnd()
{
	// compose editing surface onto main surface

	pushSurface(&surfaceComposed);
	{
		gxSetTexture(surfaceEditing.getTexture());
		{
			setBlend(BLEND_PREMULTIPLIED_ALPHA);
			{
				setColor(colorWhite);
				drawRect(0, 0, surfaceComposed.getWidth(), surfaceComposed.getHeight());
			}
			setBlend(BLEND_ALPHA);
		}
		gxSetTexture(0);
	}
	popSurface();

	// clear the editing surface

	surfaceEditing.clear(0, 0, 0, 0);

	// reset draw state

	drawState = DrawShared();
	traveller = Traveller();
}

void PicEdit::drawMoveCB(void * obj, const TravelEvent & e)
{
	PicEdit * self = (PicEdit*)obj;

	self->drawMove(e.x, e.y, e.dx, e.dy);
}

void PicEdit::drawMove(const float x, const float y, const float moveX, const float moveY)
{
	static float v = 0.f;
	const float dx = drawState.endX - x;
	const float dy = drawState.endY - y;
	const float ds = std::sqrtf(dx * dx + dy * dy);
	drawState.color = Color::fromHSL(v / 360.f, 1.f, .5f);
	v += ds * .1f;

	drawState.endX = x;
	drawState.endY = y;

	setColor(drawState.color);
	setBlend(BLEND_PREMULTIPLIED_ALPHA_DRAW);

	switch (drawType)
	{
	case kDrawType_Brush:
		pushSurface(&surfaceEditing);
		{
#if 1
			Sprite sprite("brush.png");
			sprite.pivotX = sprite.getWidth() / 2.f;
			sprite.pivotY = sprite.getHeight() / 2.f;
			sprite.drawEx(drawState.endX, drawState.endY, Calc::RadToDeg(-std::atan2f(moveX, moveY)), .4f, .4f, false, FILTER_LINEAR);
#else
			fillCircle(
				drawState.endX,
				drawState.endY,
				10,
				100);
#endif
		}
		popSurface();
		break;
	case kDrawType_Rect:
	{
		surfaceEditing.clear(0, 0, 0, 0);
		pushSurface(&surfaceEditing);
		{
			if (drawState.fixed)
			{
				const float dx = drawState.endX - drawState.beginX;
				const float dy = drawState.endY - drawState.beginY;
				const float size = std::min(std::fabsf(dx), std::fabsf(dy));

				const float x = std::min(drawState.beginX, drawState.endX);
				const float y = std::min(drawState.beginY, drawState.endY);

				drawRect(
					drawState.beginX,
					drawState.beginY,
					drawState.beginX + Calc::Sign(dx) * size,
					drawState.beginY + Calc::Sign(dy) * size);
			}
			else
			{
				drawRect(
					drawState.beginX,
					drawState.beginY,
					drawState.endX,
					drawState.endY);
			}
		}
		popSurface();
	}
	break;
	case kDrawType_Ellipse:
	{
		surfaceEditing.clear(0, 0, 0, 0);
		pushSurface(&surfaceEditing);
		{
			const float dx = std::fabsf(drawState.endX - drawState.beginX);
			const float dy = std::fabsf(drawState.endY - drawState.beginY);
			const float radius = std::min(dx, dy);

			fillCircle(
				drawState.beginX,
				drawState.beginY,
				radius,
				radius * Calc::m2PI);
		}
		popSurface();
	}
	break;
	case kDrawType_Line:
	{
		surfaceEditing.clear(0, 0, 0, 0);
		pushSurface(&surfaceEditing);
		{
			if (drawState.fixed)
			{
				const float dx = drawState.endX - drawState.beginX;
				const float dy = drawState.endY - drawState.beginY;

				if (std::fabsf(dx) > std::fabsf(dy))
				{
					drawLine(
						drawState.beginX,
						drawState.beginY,
						drawState.endX,
						drawState.beginY);
				}
				else
				{
					drawLine(
						drawState.beginX,
						drawState.beginY,
						drawState.beginX,
						drawState.endY);
				}
			}
			else
			{
				drawLine(
					drawState.beginX,
					drawState.beginY,
					drawState.endX,
					drawState.endY);
			}
		}
		popSurface();
	}
	break;
	}
}

void PicEdit::drawCancel()
{
	// clear the editing surface

	surfaceEditing.clear(0, 0, 0, 0);

	// reset draw state

	drawState = DrawShared();
	traveller = Traveller();
}

PicEdit::PicEdit(const int sx, const int sy, const Surface * source)
	: surfaceComposed(sx, sy, true)
	, surfaceEditing(sx, sy, true)
	, state(kState_Idle)
	, drawType(kDrawType_Brush)
{
	surfaceComposed.clear(0, 0, 0, 0);
	surfaceEditing.clear(0, 0, 0, 0);

	if (source != nullptr)
	{
		source->blitTo(&surfaceComposed);
	}
}

void PicEdit::tick(const float dt)
{
	switch (state)
	{
	case kState_Idle:
		if (mouse.wentDown(BUTTON_LEFT))
		{
			state = kState_Draw;

			const bool fixed = keyboard.isDown(SDLK_LSHIFT);
			const float x = mouse.x;
			const float y = mouse.y;

			drawBegin(fixed, x, y);
		}
		else if (keyboard.wentDown(SDLK_b))
		{
			drawType = kDrawType_Brush;
		}
		else if (keyboard.wentDown(SDLK_r))
		{
			drawType = kDrawType_Rect;
		}
		else if (keyboard.wentDown(SDLK_c))
		{
			drawType = kDrawType_Ellipse;
		}
		else if (keyboard.wentDown(SDLK_l))
		{
			drawType = kDrawType_Line;
		}
		break;
	case kState_ColorSelect:
		break;
	case kState_Draw:
	{
		// todo : evaluate mouse movement

		const float x = mouse.x;
		const float y = mouse.y;

		if (drawType == kDrawType_Brush)
		{
			traveller.Update(x, y);
		}
		else
		{
			drawMove(x, y, 0.f, 0.f);
		}

		if (mouse.wentUp(BUTTON_LEFT))
		{
			drawEnd();

			state = kState_Idle;
		}
	}
	break;
	}
}

void PicEdit::drawEditingSurface(const float alpha) const
{
	Shader shader("picedit-compose", "effect.vs", "picedit-compose.ps");
	setShader(shader);
	{
		shader.setTexture("layer1", 0, surfaceComposed.getTexture(), false, true);
		shader.setTexture("layer2", 1, surfaceEditing.getTexture(), false, true);
		shader.setImmediate("mode", keyboard.isDown(SDLK_e) ? 1 : 0);
		
		setBlend(BLEND_OPAQUE);
		{
			setColor(colorWhite);
			drawRect(0, 0, surfaceComposed.getWidth(), surfaceComposed.getHeight());
		}
		setBlend(BLEND_ALPHA);
	}
	clearShader();

#if 0
	gxSetTexture(surfaceComposed.getTexture());
	{
		setBlend(BLEND_OPAQUE);
		{
			setColor(colorWhite);
			drawRect(0, 0, surfaceComposed.getWidth(), surfaceComposed.getHeight());
		}
		setBlend(BLEND_ALPHA);
	}
	gxSetTexture(0);

	gxSetTexture(surfaceEditing.getTexture());
	{
		//setBlend(BLEND_OPAQUE);
		setBlend(BLEND_PREMULTIPLIED_ALPHA);
		{
			setColor(colorWhite);
			drawRect(0, 0, surfaceEditing.getWidth(), surfaceEditing.getHeight());
		}
		setBlend(BLEND_ALPHA);
	}
	gxSetTexture(0);
#endif
}

void PicEdit::draw(const float alpha) const
{
	Surface surface(surfaceComposed.getWidth(), surfaceComposed.getHeight(), false);

	pushSurface(&surface);
	{
		drawEditingSurface(alpha);

		switch (state)
		{
		case kState_Idle:
			break;
		case kState_Draw:
			break;
		case kState_ColorSelect:
			break;
		}
	}
	popSurface();

	gxSetTexture(surface.getTexture());
	{
		setBlend(BLEND_ALPHA);
		setColorf(1.f, 1.f, 1.f, alpha);
		drawRect(0, 0, surface.getWidth(), surface.getHeight());
	}
	gxSetTexture(0);
}

void PicEdit::blitTo(Surface * surface) const
{
	surfaceComposed.blitTo(surface);
}

GLuint PicEdit::getTexture() const
{
	return surfaceComposed.getTexture();
}
