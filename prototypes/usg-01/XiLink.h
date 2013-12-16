#pragma once

#include <allegro.h>
#include "SelectionBuffer.h"
#include "types.h"

namespace Xi
{
	class Part;

	class Link
	{
	public:
		Link();

		void Update();
		void RenderSB(SelectionBuffer* sb);
		void Render(BITMAP* buffer);
		void TF_Push();
		void TF_Pop();

		Vec2F Position;;
		float Rotation;
		float RotationMin;
		float RotationMax;
		BOOL Mirrored;
		Part* Part;
	};
};
