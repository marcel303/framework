#include "Renderer.h"
#include "XiLink.h"
#include "XiPart.h"

namespace Xi
{
	Part::Part()
	{
		PartType = PartType_Body;
		RotationSpeed = 0.0f;

		HitPoints = 100;
		IsAlive = TRUE;

		SelectionIndex = g_SelectionMap.Allocate();
		g_SelectionMap.Set(SelectionIndex, this);

//		Sprite = g_SpriteMgr.Load("sprite01.bmp");
	}

	Part::~Part()
	{
		g_SelectionMap.Free(SelectionIndex);
	}

	Part* Part::Copy()
	{
		Part* result = new Part();

		result->PartType = PartType;
		result->Shape = Shape;
		result->BoundingShape = BoundingShape;
		result->Links = Links;
		result->HitPoints = HitPoints;

		return result;
	}

	void Part::Update()
	{
		for (size_t i = 0; i < Links.size(); ++i)
		{
			Links[i]->Update();
		}

		UpdateTransform();
	}

	void Part::UpdateTransform()
	{
		for (size_t i = 0; i < Shape.Outline.size(); ++i)
		{
			Shape.OutlineT[i] = g_Renderer.Project(Shape.Outline[i]);
		}

		for (size_t i = 0; i < Shape.Triangles.size(); ++i)
		{
			Shape.Triangles[i].PointsT[0] = g_Renderer.Project(Shape.Triangles[i].Points[0]);
			Shape.Triangles[i].PointsT[1] = g_Renderer.Project(Shape.Triangles[i].Points[1]);
			Shape.Triangles[i].PointsT[2] = g_Renderer.Project(Shape.Triangles[i].Points[2]);
		}
	}

	static int ToColor(const Vec3F& rgb)
	{
		int v[3];
		
		v[0] = MidI(rgb[0] * 255.0f, 0, 255);
		v[1] = MidI(rgb[1] * 255.0f, 0, 255);
		v[2] = MidI(rgb[2] * 255.0f, 0, 255);

		return makecol(v[0], v[1], v[2]);
	}

	void Part::RenderSB(SelectionBuffer* sb)
	{
		for (size_t i = 0; i < Shape.Triangles.size(); ++i)
		{
			const Shape_Triangle& triangle = Shape.Triangles[i];

			sb->Scan_Triangle(
				triangle.PointsT,
				SelectionIndex);
		}

		for (size_t i = 0; i < Links.size(); ++i)
		{
			Links[i]->RenderSB(sb);
		}
	}

	void Part::Render(BITMAP* buffer)
	{
		int co;
		int cf;

		Vec3F rgbDef(0.0f, 1.0f, 0.0f);
		Vec3F rgbHit(1.5f, 1.0f, 0.0f);

		switch (PartType)
		{
		case PartType_Body:
			{
				float t1 = HitTimer.Progress_get();
				float t2 = 1.0f - t1;
				Vec3F rgb = rgbDef * t1 + rgbHit * t2;
				co = ToColor(rgb);
				cf = ToColor(rgb * 0.25f);
			}
			break;
		case PartType_Weapon:
			co = makecol(HitTimer.Progress_get() * 255.0f, HitTimer.Progress_get() * 255.0f, HitTimer.Progress_get() * 255.0f);
			cf = makecol(
				127 + HitTimer.Progress_get() * 127.0f, 
				127 + HitTimer.Progress_get() * 127.0f, 
				127 + HitTimer.Progress_get() * 127.0f);
			break;
		}

		g_Renderer.PolyFill(
			buffer,
			Shape.OutlineT,
			cf);

		g_Renderer.Poly(
			buffer,
			Shape.OutlineT,
			co);

//		Sprite->Render(buffer, Vec2(0.0f, 0.0f), 0.0f);

		for (size_t i = 0; i < Links.size(); ++i)
		{
			Links[i]->Render(buffer);
		}

		HitTimer.Tick();
	}

	void Part::Hit()
	{
		HitPoints--;
		HitTimer.Start(AnimationTimerMode_FrameBased, 40);

		if (HitPoints < 0)
		{
			HitPoints = 0;
			IsAlive = FALSE;
		}
	}
};
