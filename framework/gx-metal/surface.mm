#include "framework.h"
#include "renderTarget.h"
#include "metal.h"

void Surface::blitTo(Surface * surface) const
{
#if 1
	pushSurface(surface);
	{
		pushDepthTest(false, DEPTH_LESS, false);
		pushColorWriteMask(1, 1, 1, 1);
		{
		// todo : use a dedicated shader for this
			setColor(colorWhite);
			gxSetTexture(getTexture());
			drawRect(0, 0, getWidth(), getHeight());
			gxSetTexture(0);
		}
		popColorWriteMask();
		popDepthTest();
	}
	popSurface();
#else
	id<MTLTexture> src_texture = (id<MTLTexture>)m_colorTarget[m_bufferId]->getMetalTexture();
	id<MTLTexture> dst_texture = (id<MTLTexture>)surface->m_colorTarget[surface->m_bufferId]->getMetalTexture();
	
	// note : src pixel format must equal dst pixel format
	
	metal_copy_texture_to_texture(src_texture, 0, 0, 0, src_texture.width, src_texture.height, dst_texture, 0, 0, dst_texture.pixelFormat);
#endif
}
