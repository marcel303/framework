#pragma once

#include "GraphicsI.h"

// OpenGLES doesn't have a matrix stack, so we need to simulate it
#if defined(IPHONEOS) || defined(BBOS)
	#define SIMULATE_STACK 1
#else
	#define SIMULATE_STACK 0
#endif

class Graphics_OpenGL : public GraphicsI
{
public:
	Graphics_OpenGL();
	virtual ~Graphics_OpenGL();

	virtual void MakeCurrent();
	virtual void Clear(float r, float g, float b, float a);
	virtual void BlendModeSet(BlendMode blendMode);
	virtual void MatrixSet(MatrixType type, const Mat4x4& mat);
	virtual void MatrixPush(MatrixType type);
	virtual void MatrixPop(MatrixType type);
	virtual void MatrixTranslate(MatrixType type, float x, float y, float z);
	virtual void MatrixRotate(MatrixType type, float angle, float x, float y, float z);
	virtual void MatrixScale(MatrixType type, float x, float y, float z);
	virtual void TextureSet(Res* texture);
	virtual void TextureIsEnabledSet(bool value);
	virtual void TextureDestroy(Res* res);
	virtual void TextureCreate(Res* res);
	virtual void Present();
	
#if SIMULATE_STACK
	uint32_t m_stackSize[2];
	struct
	{
		float m_values[16];
	} m_stack[2][4];
#endif
};
