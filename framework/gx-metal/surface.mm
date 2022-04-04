#include "framework.h"

#if ENABLE_METAL

#include "renderTarget.h"
#include "metal.h"

void Surface::setSwizzle(int r, int g, int b, int a)
{
	logWarning("Surface::setSwizzle not yet implemented");
}

void Surface::blitTo(Surface * surface) const
{
	pushSurface(surface);
	{
		pushDepthTest(false, DEPTH_LESS, false);
		pushColorWriteMask(1, 1, 1, 1);
		pushBlend(BLEND_OPAQUE);
		{
			Shader shader("engine/gx-metal/surface-blit");
			setShader(shader);
			shader.setTexture("source", 0, getTexture(), false, true);
			drawRect(0, 0, getWidth(), getHeight());
			clearShader();
		}
		popBlend();
		popColorWriteMask();
		popDepthTest();
	}
	popSurface();
}

#endif
