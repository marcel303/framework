#include "Renderer.h"
#include "XiLink.h"
#include "XiPart.h"

namespace Xi
{
	Link::Link()
	{
		Position = Vec2F(0.0f, 0.0f);
		Rotation = 0.0f;
		RotationMin = 0.0f;
		RotationMax = 0.0f;
		Mirrored = false;
		Part = 0;
	}

	void Link::Update()
	{
		TF_Push();

		if (Part)
		{
			Part->Update();

			if (!Part->IsAlive)
			{
				// todo: remove event -> use MQ.

				//delete Part;
				Part = 0;
			}
		}

		TF_Pop();

		// todo: make rotation property of Part.
		if (Part)
		{
			Rotation += Part->RotationSpeed;
		}

		// Constrain part rotation.

		if (Rotation < RotationMin)
			Rotation = RotationMin;

		if (Rotation > RotationMax)
			Rotation = RotationMax;
	}

	void Link::RenderSB(SelectionBuffer* sb)
	{
		TF_Push();

		if (Part)
		{
			Part->RenderSB(sb);
		}

		TF_Pop();
	}

	void Link::Render(BITMAP* buffer)
	{
		TF_Push();

		g_Renderer.Circle(buffer, Vec2F(0.0f, 0.0f), 3.0f, makecol(127, 127, 127));

		if (Part)
			Part->Render(buffer);

		TF_Pop();
	}

	void Link::TF_Push()
	{
		g_Renderer.PushT(Position);

		if (Mirrored)
			g_Renderer.PushS(-1.0f, 1.0f);

		g_Renderer.PushR(Rotation);
	}

	void Link::TF_Pop()
	{
		g_Renderer.Pop();

		if (Mirrored)
			g_Renderer.Pop();

		g_Renderer.Pop();
	}
};
