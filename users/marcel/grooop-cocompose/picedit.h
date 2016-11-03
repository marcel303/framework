#pragma once

#include "Calc.h"
#include "framework.h" // todo : move to cpp
#include "Traveller.h"

class PicEdit
{
	enum State
	{
		kState_Idle,
		kState_ColorSelect,
		kState_Draw
	};
	
	enum DrawType
	{
		kDrawType_Brush,
		kDrawType_Rect,
		kDrawType_Ellipse,
		kDrawType_Line,
		kDrawType_COUNT
	};
	
	struct DrawShared
	{
		DrawShared();
		
		bool fixed;
		
		float beginX;
		float beginY;
		float endX;
		float endY;

		Color color;
	};
	
	class PicTraveller
	{
		float lastX, lastY;
		Traveller traveller;

		void updatePosition(const float x, const float y)
		{
			const float falloff = .2f;

			lastX = Calc::Lerp(lastX, x, falloff);
			lastY = Calc::Lerp(lastY, y, falloff);
		}

	public:
		PicTraveller()
			: traveller()
		{
		}

		void begin(const float step, TravelCB travelCB, void * obj, const float x, const float y)
		{
			lastX = x;
			lastY = y;

			traveller.Setup(step, travelCB, obj);
			traveller.Begin(x, y);
		}

		void end(const float x, const float y)
		{
			updatePosition(x, y);

			traveller.End(lastX, lastY);
		}

		void update(const float x, const float y)
		{
			updatePosition(x, y);

			traveller.Update(lastX, lastY);
		}
	};

	Surface surfaceComposed;
	Surface surfaceEditing;
	
	State state;
	DrawType drawType;
	DrawShared drawState;
	Traveller traveller;
	
	void drawBegin(const bool fixed, const float x, const float y);
	static void drawMoveCB(void * obj, const TravelEvent & e);
	void drawMove(const float x, const float y, const float moveX, const float moveY);
	void drawEnd();
	void drawCancel();
	
public:
	PicEdit(const int sx, const int sy, const Surface * source);

	void tick(const float dt);
	
	void drawEditingSurface(const float alpha) const;
	void draw(const float alpha) const;

	void blitTo(Surface * surface) const;
	GLuint getTexture() const;
};
