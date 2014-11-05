#include "GraphicsDevice.h"
#include "ResShader.h"

ResShader::ResShader() : Res()
{
	SetType(RES_SHADER);

	m_depthTestEnabled = true;
	m_depthTestFunc = CMP_LE;
	m_lightEnabled = false;
	m_fogEnabled = false;
	m_cullMode = CULL_NONE;
	m_blendEnabled = false;
	m_blendSrc = BLEND_SRC;
	m_blendDst = BLEND_DST;
	m_alphaTestEnabled = false;
	m_alphaTestFunc = CMP_GE;
	m_alphaTestRef = 0.5f;
	m_stencilTestEnabled = false;
	m_stencilTestFunc = CMP_ALWAYS;
	m_stencilTestRef = 0x00;
	m_stencilTestMask = 0xFF;
	m_stencilTestOnPass = ZERO;
	m_stencilTestOnFail = ZERO;
	m_stencilTestOnZFail = ZERO;
	m_writeColor = true;
	m_writeDepth = true;

	m_states = ~uint32_t(ST_DEPTHTEST | ST_CULL | ST_WRITE_COLOR | ST_WRITE_DEPTH);
	//m_states = 0;
}

void ResShader::InitDepthTest(bool enabled, TEST_FUNC func)
{
	m_states |= ST_DEPTHTEST;
	m_depthTestEnabled = enabled;
	m_depthTestFunc = func;
}

void ResShader::InitLight(bool enabled)
{
	m_states |= ST_LIGHT;
	m_lightEnabled = enabled;
}

void ResShader::InitFog(bool enabled)
{
	m_states |= ST_FOG;
	m_fogEnabled = enabled;
}

void ResShader::InitCull(CULL_MODE mode)
{
	m_states |= ST_CULL;
	m_cullMode = mode;
}

void ResShader::InitBlend(bool enabled, BLEND_OP src, BLEND_OP dst)
{
	m_states |= ST_BLEND;
	m_blendEnabled = enabled;
	m_blendSrc = src;
	m_blendDst = dst;
}

void ResShader::InitAlphaTest(bool enabled, TEST_FUNC func, float ref)
{
	m_states |= ST_ALPHATEST;
	m_alphaTestEnabled = enabled;
	m_alphaTestFunc = func;
	m_alphaTestRef = ref;
}

void ResShader::InitStencilTest(bool enabled, TEST_FUNC func, uint8_t ref, uint8_t mask, STENCIL_OP onPass, STENCIL_OP onFail, STENCIL_OP onZFail)
{
	m_states |= ST_STENCILTEST;
	m_stencilTestEnabled = enabled;
	m_stencilTestFunc = func;
	m_stencilTestRef = ref;
	m_stencilTestMask = mask;
	m_stencilTestOnPass = onPass;
	m_stencilTestOnFail = onFail;
	m_stencilTestOnZFail = onZFail;
}

void ResShader::InitWriteColor(bool write)
{
	m_states |= ST_WRITE_COLOR;
	m_writeColor = write;
}

void ResShader::InitWriteDepth(bool write)
{
	m_states |= ST_WRITE_DEPTH;
	m_writeDepth = write;
}

void ResShader::Apply(GraphicsDevice* gfx, bool setPrograms) const
{
	Assert(gfx);

	if (m_states & ST_DEPTHTEST)
	{
		if (m_depthTestEnabled)
		{
			gfx->RS(RS_DEPTHTEST, 1);
			gfx->RS(RS_DEPTHTEST_FUNC, m_depthTestFunc);
		}
		else
			gfx->RS(RS_DEPTHTEST, 0);
	}

	// TODO
	//if (m_states & ST_LIGHT)
	//if (m_states & ST_FOG)

	if (m_states & ST_CULL)
	{
		gfx->RS(RS_CULL, m_cullMode);
	}

	if (m_states & ST_BLEND)
	{
		if (m_blendEnabled)
		{
			gfx->RS(RS_BLEND_SRC, m_blendSrc);
			gfx->RS(RS_BLEND_DST, m_blendDst);
			gfx->RS(RS_BLEND, 1);
		}
		else
			gfx->RS(RS_BLEND, 0);
	}

	if (m_states & ST_ALPHATEST)
	{
		if (m_alphaTestEnabled)
		{
			gfx->RS(RS_ALPHATEST_FUNC, m_alphaTestFunc);
			gfx->RS(RS_ALPHATEST_REF, *(int*)&m_alphaTestRef);
			gfx->RS(RS_ALPHATEST, 1);
		}
		else
			gfx->RS(RS_ALPHATEST, 0);
	}

	if (m_states & ST_STENCILTEST)
	{
		if (m_stencilTestEnabled)
		{
			gfx->RS(RS_STENCILTEST_FUNC, m_stencilTestFunc);
			gfx->RS(RS_STENCILTEST_REF, m_stencilTestRef);
			gfx->RS(RS_STENCILTEST_MASK, m_stencilTestMask);
			gfx->RS(RS_STENCILTEST_ONPASS, m_stencilTestOnPass);
			gfx->RS(RS_STENCILTEST_ONFAIL, m_stencilTestOnFail);
			gfx->RS(RS_STENCILTEST_ONZFAIL, m_stencilTestOnZFail);
			gfx->RS(RS_STENCILTEST, 1);
		}
		else
			gfx->RS(RS_STENCILTEST, 0);
	}

	if (m_states & ST_WRITE_COLOR)
		gfx->RS(RS_WRITE_COLOR, m_writeColor);
	if (m_states & ST_WRITE_DEPTH)
		gfx->RS(RS_WRITE_DEPTH, m_writeDepth);

	if (setPrograms)
	{
		gfx->SetVS(m_vs.get());
		gfx->SetPS(m_ps.get());
	}
}
