/*
	Copyright (C) 2017 Marcel Smit
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

static Stack<BLEND_MODE, 32> blendModeStack(BLEND_ALPHA);
static Stack<bool, 32> lineSmoothStack(false);
static Stack<bool, 32> wireframeStack(false);
static Stack<DepthTestInfo, 32> depthTestStack(DepthTestInfo { false, DEPTH_LESS, true });
static Stack<CullModeInfo, 32> cullModeStack(CullModeInfo { CULL_NONE, CULL_CCW });

void setBlend(BLEND_MODE blendMode)
{
	globals.blendMode = blendMode;
	
	switch (blendMode)
	{
	case BLEND_OPAQUE:
		glDisable(GL_BLEND);
		break;
	case BLEND_ALPHA:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA_DRAW:
	// todo : remove ?
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
	default:
		fassert(false);
		break;
	}
}

void pushBlend(BLEND_MODE blendMode)
{
	blendModeStack.push(globals.blendMode);
	
	setBlend(blendMode);
}

void popBlend()
{
	const BLEND_MODE blendMode = blendModeStack.popValue();
	
	setBlend(blendMode);
}

void setLineSmooth(bool enabled)
{
#if ENABLE_DESKTOP_OPENGL
	// todo : gles : implement setLineSmooth
	
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
#endif
}

void pushLineSmooth(bool enabled)
{
	lineSmoothStack.push(globals.lineSmoothEnabled);
	
	setLineSmooth(enabled);
}

void popLineSmooth()
{
	const bool value = lineSmoothStack.popValue();
	
	setLineSmooth(value);
}

void setWireframe(bool enabled)
{
#if ENABLE_DESKTOP_OPENGL
	// todo : gles : implement setWireframe
	
	glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
	checkErrorGL();
#else
	// todo : what to do here for OpenGLES ?
#endif
}

void pushWireframe(bool enabled)
{
	wireframeStack.push(globals.wireframeEnabled);
	
	setWireframe(enabled);
}

void popWireframe()
{
	const bool value = wireframeStack.popValue();
	
	setWireframe(value);
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
	default:
		Assert(false);
		return GL_LESS;
	}
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

void pushDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	const DepthTestInfo info =
	{
		globals.depthTestEnabled,
		globals.depthTest,
		globals.depthTestWriteEnabled
	};
	
	depthTestStack.push(info);
	
	setDepthTest(enabled, test, writeEnabled);
}

void popDepthTest()
{
	const DepthTestInfo depthTestInfo = depthTestStack.popValue();
	
	setDepthTest(depthTestInfo.testEnabled, depthTestInfo.test, depthTestInfo.writeEnabled);
}

void pushDepthWrite(bool enabled)
{
	pushDepthTest(globals.depthTestEnabled, globals.depthTest, enabled);
}

void popDepthWrite()
{
	popDepthTest();
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

void pushCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	const CullModeInfo info =
	{
		globals.cullMode,
		globals.cullWinding
	};
	
	cullModeStack.push(info);
	
	setCullMode(mode, frontFaceWinding);
}

void popCullMode()
{
	const CullModeInfo cullMode = cullModeStack.popValue();
	
	setCullMode(cullMode.mode, cullMode.winding);
}

#endif
