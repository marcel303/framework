#include "framework.h"

#if ENABLE_METAL

#include "renderTarget.h"
#include "metal.h"

void Surface::setSwizzle(int r, int g, int b, int a)
{
	for (int i = 0; i < 2; ++i)
	{
		m_colorTarget[i]->setSwizzle(r, g, b, a);
	}
}

void Surface::blitTo(Surface * surface) const
{
	pushSurface(surface, true);
	{
		pushDepthTest(false, DEPTH_LESS, false);
		pushColorWriteMask(1, 1, 1, 1);
		pushBlend(BLEND_OPAQUE);
		{
			Shader shader("engine/gx-metal/surface-blit");
			setShader(shader);
			{
				shader.setTexture("source", 0, getTexture(), false, true);
				drawRect(0, 0, surface->getWidth(), surface->getHeight());
			}
			clearShader();
		}
		popBlend();
		popColorWriteMask();
		popDepthTest();
	}
	popSurface();
}

#endif
