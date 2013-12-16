#pragma once

#include "Mat4x4.h"
#include "Res.h"

enum BlendMode
{
	BlendMode_Normal_Opaque,
	BlendMode_Normal_Opaque_Add,
	BlendMode_Normal_Transparent,
	BlendMode_Normal_Transparent_Add,
	BlendMode_Additive,
	BlendMode_Subtractive
};

enum MatrixType
{
	MatrixType_Projection,
	MatrixType_World
};

class GraphicsI
{
public:
	virtual ~GraphicsI();

	virtual void MakeCurrent() = 0;
	virtual void Clear(float r, float g, float b, float a) = 0;
	virtual void BlendModeSet(BlendMode blendMode) = 0;
	virtual void MatrixSet(MatrixType type, const Mat4x4& mat) = 0;
	virtual void MatrixPush(MatrixType type) = 0;
	virtual void MatrixPop(MatrixType type) = 0;
	virtual void MatrixTranslate(MatrixType type, float x, float y, float z) = 0;
	virtual void MatrixRotate(MatrixType type, float angle, float x, float y, float z) = 0;
	virtual void MatrixScale(MatrixType type, float x, float y, float z) = 0;
	virtual void TextureSet(Res* texture) = 0;
	virtual void TextureIsEnabledSet(bool value) = 0;
	virtual void TextureDestroy(Res* res) = 0;
	virtual void TextureCreate(Res* res) = 0;
	virtual void Present() = 0;
};
