#pragma once

enum SURFACE_FORMAT
{
	SURFACE_RGBA8,
	SURFACE_RGBA16F,
	SURFACE_RGBA32F,
	SURFACE_R8,
	SURFACE_R16F,
	SURFACE_R32F,
	SURFACE_RG16F,
	SURFACE_RG32F
};

enum DEPTH_FORMAT
{
	DEPTH_FLOAT16,
	DEPTH_FLOAT32
};

class SurfaceProperties
{
public:
	struct
	{
		int width = 0;
		int height = 0;
		int backingScale = 0; // the backing scale is a multiplier applied to the size of the surface. 0 = automatically select the backing scale, any other value will be used to multiply the width and height of the storage used to back the surface
		
		void init(const int sx, const int sy)
		{
			width = sx;
			height = sy;
		}
		
		void setBackingScale(const int in_backingScale)
		{
			backingScale = in_backingScale;
		}
	} dimensions;
	
	struct
	{
		bool enabled = false;
		SURFACE_FORMAT format = SURFACE_RGBA8;
		int swizzle[4] = { 0, 1, 2, 3 };
		bool doubleBuffered = false;
		
		void init(const SURFACE_FORMAT in_format, const bool in_doubleBuffered)
		{
			enabled = true;
			format = in_format;
			doubleBuffered = in_doubleBuffered;
		}
		
		void setSwizzle(const int r, const int g, const int b, const int a)
		{
			swizzle[0] = r;
			swizzle[1] = g;
			swizzle[2] = b;
			swizzle[3] = a;
		}
	} colorTarget;
	
	struct
	{
		bool enabled = false;
		DEPTH_FORMAT format = DEPTH_FLOAT32;
		bool doubleBuffered = false;
		
		void init(const DEPTH_FORMAT in_format, const bool in_doubleBuffered)
		{
			enabled = true;
			format = in_format;
			doubleBuffered = in_doubleBuffered;
		}
	} depthTarget;
};

class Surface
{
	int m_size[2] = { 0, 0 };
	int m_backingScale = 0; // backing scale puts a multiplier on the physical size (in pixels) of the surface. it's like MSAA, but fully super-sampled. it's used t orender to retina screens, where the 'resolve' operation just copies pixels 1:1, where a resolve onto a non-retina screen would downsample the surface instead
	
	SURFACE_FORMAT m_format;
	int m_bufferId = 0;
	bool m_colorIsDoubleBuffered = false;
	bool m_depthIsDoubleBuffered = false;
	GxTextureId m_colorTexture[2] = { 0, 0 };
	GxTextureId m_depthTexture[2] = { 0, 0 };
	
	void destruct();
	
public:
	Surface();
	explicit Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer = false, bool doubleBuffered = true);
	explicit Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format);
	~Surface();
	
	void swapBuffers();

	bool init(const SurfaceProperties & properties);
	bool init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered);
	void setSwizzle(int r, int g, int b, int a);
	
	GxTextureId getTexture() const;
	bool hasDepthTexture() const;
	GxTextureId getDepthTexture() const;
	int getWidth() const;
	int getHeight() const;
	int getBackingScale() const;
	SURFACE_FORMAT getFormat() const;
	
	void clear(int r = 0, int g = 0, int b = 0, int a = 0);
	void clearf(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f);
	void clearDepth(float d);
};
