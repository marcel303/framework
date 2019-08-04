#import "metal.h"
#import "surface.h"

#define TODO 0

void Surface::destruct()
{
	m_size[0] = 0;
	m_size[1] = 0;
	m_backingScale = 1;
	
	m_bufferId = 0;
	
#if TODO
	for (int i = 0; i < 2; ++i)
	{
		if (m_buffer[i])
		{
			glDeleteFramebuffers(1, &m_buffer[i]);
			m_buffer[i] = 0;
			checkErrorGL();
		}
	}
	
	for (int i = 0; i < (m_colorIsDoubleBuffered ? 2 : 1); ++i)
	{
		if (m_colorTexture[i])
		{
			glDeleteTextures(1, &m_colorTexture[i]);
			m_colorTexture[i] = 0;
			checkErrorGL();
		}
	}
	
	m_colorTexture[0] = 0;
	m_colorTexture[1] = 0;
	
	for (int i = 0; i < (m_depthIsDoubleBuffered ? 2 : 1); ++i)
	{
		if (m_depthTexture[i])
		{
			glDeleteTextures(1, &m_depthTexture[i]);
			m_depthTexture[i] = 0;
			checkErrorGL();
		}
	}
#endif

	m_depthTexture[0] = 0;
	m_depthTexture[1] = 0;
	
	m_format = (SURFACE_FORMAT)-1;
	
	m_colorIsDoubleBuffered = false;
	m_depthIsDoubleBuffered = false;
}

Surface::Surface()
{
}

Surface::Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer, bool doubleBuffered)
{
	init(sx, sy, highPrecision ? SURFACE_RGBA16F : SURFACE_RGBA8, withDepthBuffer, doubleBuffered);
}

Surface::Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format)
{
	init(sx, sy, format, withDepthBuffer, doubleBuffered);
}

Surface::~Surface()
{
	destruct();
}

void Surface::swapBuffers()
{

}

bool Surface::init(const SurfaceProperties & properties)
{
	return false;
}

bool Surface::init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered)
{
	return false;
}

void Surface::setSwizzle(int r, int g, int b, int a)
{

}

GxTextureId Surface::getTexture() const
{
	return m_colorTexture[m_bufferId];
}

bool Surface::hasDepthTexture() const
{
	return m_depthTexture[0] != 0;
}

GxTextureId Surface::getDepthTexture() const
{
	return m_depthTexture[m_bufferId];
}

int Surface::getWidth() const
{
	return m_size[0];
}

int Surface::getHeight() const
{
	return m_size[1];
}

int Surface::getBackingScale() const
{
	return m_backingScale;
}

SURFACE_FORMAT Surface::getFormat() const
{
	return m_format;
}

void Surface::clear(int r, int g, int b, int a)
{

}

void Surface::clearf(float r, float g, float b, float a)
{

}

void Surface::clearDepth(float d)
{

}
