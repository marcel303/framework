#pragma once

#include "types.h"
#include "XiLink.h"

namespace Xi
{
	class Core
	{
	public:
		Core();

		void Update();
		void RenderSB(SelectionBuffer* sb);
		void Render(BITMAP* buffer);

		Vec2T Position;

		Link* Root;
	};
};
