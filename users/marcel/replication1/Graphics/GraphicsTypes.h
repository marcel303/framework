#ifndef GRAPHICSTYPES_H
#define GRAPHICSTYPES_H
#pragma once

// Buffers.

enum BUFFER_NAME
{
	BUFFER_COLOR   = 0x01,
	BUFFER_STENCIL = 0x02,
	BUFFER_DEPTH   = 0x04,
	BUFFER_ALL     = 0xFF
};

// Render states.

enum RENDER_STATE
{
	RS_ALPHATEST,
	RS_ALPHATEST_FUNC,
	RS_ALPHATEST_REF,
	RS_BLEND,
	RS_BLEND_SRC,
	RS_BLEND_DST,
	RS_CULL,
	RS_DEPTHTEST,
	RS_DEPTHTEST_FUNC,
	RS_SCISSORTEST, // FIXME, TODO, implement!
	RS_STENCILTEST,
	RS_STENCILTEST_FUNC,
	RS_STENCILTEST_REF,
	RS_STENCILTEST_MASK,
	RS_STENCILTEST_ONPASS,
	RS_STENCILTEST_ONFAIL,
	RS_STENCILTEST_ONZFAIL,
	RS_WRITE_COLOR,
	RS_WRITE_DEPTH,
	RS_ENUM_SIZE
};

enum BLEND_OP
{
	BLEND_ONE,
	BLEND_ZERO,
	BLEND_SRC,
	BLEND_DST,
	BLEND_SRC_COLOR,
	BLEND_DST_COLOR,
	BLEND_INV_SRC,
	BLEND_INV_DST,
	BLEND_INV_SRC_COLOR,
	BLEND_INV_DST_COLOR
};

enum CULL_MODE
{
	CULL_NONE,
	CULL_CW,
	CULL_CCW
};

enum TEST_FUNC
{
	CMP_ALWAYS,
	CMP_EQ,
	CMP_L,
	CMP_LE,
	CMP_G,
	CMP_GE
};

enum STENCIL_OP
{
	INC,
	DEC,
	ZERO
};

// Sampler states.

enum SAMPLE_STATE
{
	SS_FILTER
};

enum FILTER
{
	FILTER_POINT,
	FILTER_INTERPOLATE
};

// Matrices.

enum MATRIX_NAME
{
	MAT_WRLD,
	MAT_VIEW,
	MAT_PROJ
};

// Primitive types.

// FIXME/TODO: Make sure each and every PT is implemented in D3D/GL..

enum PRIMITIVE_TYPE
{
	PT_TRIANGLE_LIST,
	PT_TRIANGLE_FAN,
	PT_TRIANGLE_STRIP,
	PT_LINE_LIST,
	PT_LINE_STRIP
};

class GraphicsOptions
{
public:
	GraphicsOptions() { }
	GraphicsOptions(int sx, int sy, bool fullscreen, bool createRenderTarget) :
		Sx(sx), Sy(sy), Fullscreen(fullscreen), CreateRenderTarget(createRenderTarget)
	{
	}

	int Sx;
	int Sy;
	bool Fullscreen;
	bool CreateRenderTarget;
};

#endif
