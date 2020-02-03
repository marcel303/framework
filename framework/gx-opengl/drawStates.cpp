/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

#if ENABLE_OPENGL

#include "internal.h"

static Stack<int, 32> colorWriteStack(0xf);

static int s_colorWriteMask = 0xf;

void setBlend(BLEND_MODE blendMode)
{
	globals.blendMode = blendMode;
	
	switch (blendMode)
	{
	case BLEND_OPAQUE:
		glDisable(GL_BLEND);
		break;
	case BLEND_ALPHA:
		// note : source alpha is set to ZERO!
		// assuming the destination surface starts at 100% alpha, sussively multiplication by 1-srcA will yield an inverse opacity value stored inside the destination alpha. the destination may then be blended using an inverted premultiplied-alpha blend mode for correctly composing the surface on top of something else
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA_DRAW:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_ADD:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_ADD_OPAQUE:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case BLEND_SUBTRACT:
		glEnable(GL_BLEND);
		fassert(glBlendEquation);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_INVERT:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		break;
	case BLEND_MUL:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
	case BLEND_MIN:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_MIN);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case BLEND_MAX:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_MAX);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	}
}

void setLineSmooth(bool enabled)
{
#if ENABLE_DESKTOP_OPENGL
	if (enabled)
	{
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		checkErrorGL();
	}
	else
	{
		glDisable(GL_LINE_SMOOTH);
		checkErrorGL();
	}
#else
	// note : gles doesn't support line smooth
#endif
}

void setWireframe(bool enabled)
{
#if ENABLE_DESKTOP_OPENGL
	glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
	checkErrorGL();
#else
	// note : gles doesn't support wireframe
#endif
}

void setColorWriteMask(int r, int g, int b, int a)
{
	s_colorWriteMask =
		((r ? 1 : 0) << 0) |
		((g ? 1 : 0) << 1) |
		((b ? 1 : 0) << 2) |
		((a ? 1 : 0) << 3);
	
	glColorMask(r, g, b, a);
}

void setColorWriteMaskAll()
{
	setColorWriteMask(1, 1, 1, 1);
}

void pushColorWriteMask(int r, int g, int b, int a)
{
	colorWriteStack.push(s_colorWriteMask);
	
	setColorWriteMask(r, g, b, a);
}

void popColorWriteMask()
{
	const int colorWriteMask = colorWriteStack.popValue();
	
	const int r = (colorWriteMask >> 0) & 1;
	const int g = (colorWriteMask >> 1) & 1;
	const int b = (colorWriteMask >> 2) & 1;
	const int a = (colorWriteMask >> 3) & 1;
	
	setColorWriteMask(r, g, b, a);
}

static GLenum toOpenGLDepthFunc(DEPTH_TEST test)
{
	switch (test)
	{
	case DEPTH_EQUAL:
		return GL_EQUAL;
	case DEPTH_LESS:
		return GL_LESS;
	case DEPTH_LEQUAL:
		return GL_LEQUAL;
	case DEPTH_GREATER:
		return GL_GREATER;
	case DEPTH_GEQUAL:
		return GL_GEQUAL;
	case DEPTH_ALWAYS:
		return GL_ALWAYS;
	}
	
	AssertMsg(false, "unknown DEPTH_TEST", 0);
	return GL_ALWAYS;
}

void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	globals.depthTestEnabled = enabled;
	globals.depthTest = test;
	globals.depthTestWriteEnabled = writeEnabled;
	
	if (enabled)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(toOpenGLDepthFunc(test));
		checkErrorGL();
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		checkErrorGL();
	}
	
	glDepthMask(writeEnabled ? GL_TRUE : GL_FALSE);
	checkErrorGL();
}

void setDepthBias(float depthBias, float slopeScale)
{
	if (depthBias == 0.f && slopeScale == 0.f)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
	else
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(slopeScale, depthBias);
	}
}

void setAlphaToCoverage(bool enabled)
{
	if (enabled)
	{
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		checkErrorGL();
	}
	else
	{
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		checkErrorGL();
	}
}

static GLenum translateStencilOp(const GX_STENCIL_OP op)
{
	switch (op)
	{
	case GX_STENCIL_OP_KEEP:
		return GL_KEEP;
	case GX_STENCIL_OP_REPLACE:
		return GL_REPLACE;
	case GX_STENCIL_OP_ZERO:
		return GL_ZERO;
	case GX_STENCIL_OP_INC:
		return GL_INCR;
	case GX_STENCIL_OP_DEC:
		return GL_DECR;
	case GX_STENCIL_OP_INC_WRAP:
		return GL_INCR_WRAP;
	case GX_STENCIL_OP_DEC_WRAP:
		return GL_DECR_WRAP;
	}
	
	AssertMsg(false, "unknown GX_STENCIL_OP", 0);
	return GL_KEEP;
}

static GLenum translateStencilFunc(const GX_STENCIL_FUNC func)
{
	switch (func)
	{
	case GX_STENCIL_FUNC_NEVER:
		return GL_NEVER;
	case GX_STENCIL_FUNC_LESS:
		return GL_LESS;
	case GX_STENCIL_FUNC_LEQUAL:
		return GL_LEQUAL;
	case GX_STENCIL_FUNC_GREATER:
		return GL_GREATER;
	case GX_STENCIL_FUNC_GEQUAL:
		return GL_GEQUAL;
	case GX_STENCIL_FUNC_EQUAL:
		return GL_EQUAL;
	case GX_STENCIL_FUNC_NOTEQUAL:
		return GL_NOTEQUAL;
	case GX_STENCIL_FUNC_ALWAYS:
		return GL_ALWAYS;
	}
	
	return GL_ALWAYS;
}

void clearStencil(uint8_t value)
{
	glClearStencil(value);
	glClear(GL_STENCIL_BUFFER_BIT);
}

void setStencilTest(const StencilState & front, const StencilState & back)
{
	// capture current stencil state
	
	globals.stencilEnabled = true;
	globals.frontStencilState = front;
	globals.backStencilState = back;
	
	// update OpenGL state
	
	const GLenum sides[2] = { GL_FRONT, GL_BACK };
	const StencilState * states[2] = { &front, &back };

	for (int i = 0; i < 2; ++i)
	{
		const GLenum side = sides[i];
		const StencilState & state = *states[i];

		glStencilFuncSeparate(side, translateStencilFunc(state.compareFunc), state.compareRef, state.compareMask);
		glStencilOpSeparate(side, translateStencilOp(state.onStencilFail), translateStencilOp(state.onDepthFail), translateStencilOp(state.onDepthStencilPass));
		glStencilMaskSeparate(side, state.writeMask);
	}
	
	glEnable(GL_STENCIL_TEST);
}

void clearStencilTest()
{
	// capture current stencil state
	
	globals.stencilEnabled = false;
	
	// update OpenGL state
	
	glDisable(GL_STENCIL_TEST);
}

void setCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	globals.cullMode = mode;
	globals.cullWinding = frontFaceWinding;
	
	if (mode == CULL_NONE)
	{
		glDisable(GL_CULL_FACE);
		checkErrorGL();
	}
	else
	{
		const GLenum face =
			mode == CULL_FRONT
			? GL_FRONT
			: GL_BACK;
		
		glEnable(GL_CULL_FACE);
		glCullFace(face);
		checkErrorGL();
		
		const GLenum winding =
			frontFaceWinding == CULL_CCW
			? GL_CCW
			: GL_CW;
		
		glFrontFace(winding);
		checkErrorGL();
	}
}

#endif
