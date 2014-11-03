#ifndef RESSHADER_H
#define RESSHADER_H
#pragma once

#include "Res.h"
#include "ResPS.h"
#include "ResVS.h"
#include "Types.h"

#include "GraphicsDevice.h" // FIXME, cpp

class ResShader : public Res
{
public:
	ResShader();

	void InitDepthTest(bool enabled, TEST_FUNC func);
	void InitLight(bool enabled);
	void InitFog(bool enabled);
	void InitCull(CULL_MODE mode);
	void InitBlend(bool enabled, BLEND_OP src, BLEND_OP dst);
	void InitAlphaTest(bool enabled, TEST_FUNC func, float ref);
	void InitStencilTest(bool enabled, TEST_FUNC func, uint8_t ref, uint8_t mask, STENCIL_OP onPass, STENCIL_OP onFail, STENCIL_OP onZFail);
	void InitWriteColor(bool write);
	void InitWriteDepth(bool write);

	void Apply(GraphicsDevice* gfx, bool setPrograms = true) const;

	ShVS m_vs;
	ShPS m_ps;

	enum STATES
	{
		ST_DEPTHTEST   = 0x1 << 0,
		ST_LIGHT       = 0x1 << 1,
		ST_FOG         = 0x1 << 2,
		ST_CULL        = 0x1 << 3,
		ST_BLEND       = 0x1 << 4,
		ST_ALPHATEST   = 0x1 << 5,
		ST_STENCILTEST = 0x1 << 6,
		ST_WRITE_COLOR = 0x1 << 7,
		ST_WRITE_DEPTH = 0x1 << 8
	};

	uint32_t m_states;

	bool      m_depthTestEnabled;
	TEST_FUNC m_depthTestFunc;

	bool      m_lightEnabled;

	bool      m_fogEnabled;

	CULL_MODE m_cullMode;

	bool     m_blendEnabled;
	BLEND_OP m_blendSrc;
	BLEND_OP m_blendDst;

	bool      m_alphaTestEnabled;
	TEST_FUNC m_alphaTestFunc;
	float     m_alphaTestRef;

	bool         m_stencilTestEnabled;
	TEST_FUNC    m_stencilTestFunc;
	uint8_t      m_stencilTestRef;
	uint8_t      m_stencilTestMask;
	STENCIL_OP m_stencilTestOnPass;
	STENCIL_OP m_stencilTestOnFail;
	STENCIL_OP m_stencilTestOnZFail;

	bool m_writeColor;
	bool m_writeDepth;
};

#endif
