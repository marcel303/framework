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
#include "internal.h"

#if ENABLE_HQ_PRIMITIVES

static void setShader_HqLines()
{
	setShader(globals.builtinShaders->hqLine.get());
}

static void setShader_HqFilledTriangles()
{
	setShader(globals.builtinShaders->hqFilledTriangle.get());
}

static void setShader_HqFilledCircles()
{
	setShader(globals.builtinShaders->hqFilledCircle.get());
}

static void setShader_HqFilledRects()
{
	setShader(globals.builtinShaders->hqFilledRect.get());
}

static void setShader_HqFilledRoundedRects()
{
	setShader(globals.builtinShaders->hqFilledRoundedRect.get());
}

static void setShader_HqStrokedTriangles()
{
	setShader(globals.builtinShaders->hqStrokeTriangle.get());
}

static void setShader_HqStrokedCircles()
{
	setShader(globals.builtinShaders->hqStrokedCircle.get());
}

static void setShader_HqStrokedRects()
{
	setShader(globals.builtinShaders->hqStrokedRect.get());
}

static void setShader_HqStrokedRoundedRects()
{
	setShader(globals.builtinShaders->hqStrokedRoundedRect.get());
}

static void applyHqShaderConstants()
{
	Shader & shader = *static_cast<Shader*>(globals.shader);
	
	fassert(shader.getType() == SHADER_VSPS);
	
	const ShaderCacheElem & shaderElem = shader.getCacheElem();
	
	if (shaderElem.params[ShaderCacheElem::kSp_ShadingParams].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_ShadingParams].index,
			globals.hqGradientType,
			globals.hqTextureEnabled,
			globals.hqUseScreenSize);
	}
	
	if (globals.hqGradientType != GRADIENT_NONE)
	{
		if (shaderElem.params[ShaderCacheElem::kSp_GradientMatrix].index != -1)
		{
			// note: we multiply the gradient matrix with the inverse of the modelView matrix here,
			// so we go back from screen-space coordinates inside the shader to local coordinates
			// for the gradient. this makes it much more easy to define gradients on elements,
			// since you can freely scale and translate them around without having to worry
			// about how this affects your gradient
			
			Mat4x4 modelView;
			gxGetMatrixf(GX_MODELVIEW, modelView.m_v);
			
			const Mat4x4 gmat = globals.hqGradientMatrix * modelView.CalcInv();
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_GradientMatrix].index, gmat.m_v);
		}
		
		if (shaderElem.params[ShaderCacheElem::kSp_GradientInfo].index != -1)
		{
			Mat4x4 gradientInfo;
			gradientInfo(0, 0) = globals.hqGradientType;
			gradientInfo(0, 1) = globals.hqGradientBias;
			gradientInfo(0, 2) = globals.hqGradientScale;
			gradientInfo(0, 3) = globals.hqGradientColorMode;
			gradientInfo(1, 0) = globals.hqGradientColor1.r;
			gradientInfo(1, 1) = globals.hqGradientColor1.g;
			gradientInfo(1, 2) = globals.hqGradientColor1.b;
			gradientInfo(1, 3) = globals.hqGradientColor1.a;
			gradientInfo(2, 0) = globals.hqGradientColor2.r;
			gradientInfo(2, 1) = globals.hqGradientColor2.g;
			gradientInfo(2, 2) = globals.hqGradientColor2.b;
			gradientInfo(2, 3) = globals.hqGradientColor2.a;
			gradientInfo(3, 0) = 0.f;
			gradientInfo(3, 1) = 0.f;
			gradientInfo(3, 2) = 0.f;
			gradientInfo(3, 3) = 0.f;
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_GradientInfo].index, gradientInfo.m_v);
		}
	}
	
	if (globals.hqTextureEnabled)
	{
		shader.setTexture("source", 0, globals.hqTexture, true, false);
		
		if (shaderElem.params[ShaderCacheElem::kSp_TextureMatrix].index != -1)
		{
			const Mat4x4 & tmat = globals.hqTextureMatrix;
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_TextureMatrix].index, tmat.m_v);
		}
	}

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	//shader.setImmediate("disableOptimizations", cosf(framework.time * 6.28f) < 0.f ? 0.f : 1.f);
	//shader.setImmediate("disableAA", cosf(framework.time) < 0.f ? 0.f : 1.f);

	shader.setImmediate("disableOptimizations", 0.f);
	shader.setImmediate("disableAA", 0.f);
	shader.setImmediate("_debugHq", 0.f);
#endif
}

void hqBegin(HQ_TYPE type, bool useScreenSize)
{
	globals.hqUseScreenSize = useScreenSize;

	switch (type)
	{
	case HQ_LINES:
		setShader_HqLines();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_TRIANGLES:
		setShader_HqFilledTriangles();
		applyHqShaderConstants();
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		setShader_HqFilledCircles();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_RECTS:
		setShader_HqFilledRects();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		setShader_HqFilledRoundedRects();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		setShader_HqStrokedTriangles();
		applyHqShaderConstants();
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_STROKED_CIRCLES:
		setShader_HqStrokedCircles();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_RECTS:
		setShader_HqStrokedRects();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		setShader_HqStrokedRoundedRects();
		applyHqShaderConstants();
		gxBegin(GX_QUADS);
		break;

	default:
		fassert(false);
		break;
	}
}

void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize)
{
	setShader(shader);
	
	globals.hqUseScreenSize = useScreenSize;

	applyHqShaderConstants();

	switch (type)
	{
	case HQ_LINES:
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_STROKED_CIRCLES:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GX_QUADS);
		break;
		
	case HQ_STROKED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	default:
		fassert(false);
		break;
	}
}

void hqEnd()
{
	gxEnd();

	clearShader();
}

#else

static HQ_TYPE s_hqType;

static float s_hqScale;

void hqBegin(HQ_TYPE type, bool useScreenSize)
{
	if (useScreenSize)
	{
		Mat4x4 matM;
		
		gxGetMatrixf(GX_MODELVIEW, matM.m_v);
		
		const float scale = matM.GetAxis(0).CalcSize();
		
		s_hqScale = 1.f / scale;
	}
	else
	{
		s_hqScale = 1.f;
	}

	//

	if (globals.hqGradientType != GRADIENT_NONE)
	{
		pushColor();
		setColor(globals.hqGradientColor1);
	}
	
	//
	
	switch (type)
	{
	case HQ_LINES:
		gxBegin(GX_LINES);
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GX_LINES);
		break;

	case HQ_STROKED_CIRCLES:
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GX_LINES);
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		gxBegin(GX_LINES);
		break;

	default:
		fassert(false);
		break;
	}
	
	s_hqType = type;
	
	globals.hqUseScreenSize = useScreenSize;
}

void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize)
{
	setShader(shader);
	
	hqBegin(type, useScreenSize);
}

void hqEnd()
{
	switch (s_hqType)
	{
	case HQ_LINES:
		gxEnd();
		break;

	case HQ_FILLED_TRIANGLES:
		gxEnd();
		break;

	case HQ_FILLED_CIRCLES:
		break;

	case HQ_FILLED_RECTS:
		gxEnd();
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxEnd();
		break;

	case HQ_STROKED_TRIANGLES:
		gxEnd();
		break;

	case HQ_STROKED_CIRCLES:
		break;

	case HQ_STROKED_RECTS:
		gxEnd();
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		gxEnd();
		break;

	default:
		fassert(false);
		break;
	}

	//

	if (globals.hqGradientType != GRADIENT_NONE)
	{
		popColor();
	}
}

#endif

void hqSetGradient(GRADIENT_TYPE gradientType, const Mat4x4 & matrix, const Color & color1, const Color & color2, const COLOR_MODE colorMode, const float bias, const float scale)
{
	globals.hqGradientType = gradientType;
	globals.hqGradientMatrix = matrix;
	globals.hqGradientColor1 = color1;
	globals.hqGradientColor2 = color2;
	globals.hqGradientColorMode = colorMode;
	globals.hqGradientBias = bias;
	globals.hqGradientScale = scale;
}

void hqSetGradient(GRADIENT_TYPE gradientType, Vec2Arg from, Vec2Arg to, const Color & color1, const Color & color2, const COLOR_MODE colorMode, const float bias, const float scale)
{
	const Vec2 delta = to - from;
	const float angle = atan2f(delta[1], delta[0]);
	const float distance = delta.CalcSize();
	
	hqSetGradient(
		gradientType,
		Mat4x4(true)
			//.Scale(1.f / distance, 1.f / distance, 1)
			.RotateZ(angle)
			.Scale(1.f / distance, 1.f / distance, 1)
			.Translate(-from[0], -from[1], 0),
		color1,
		color2,
		colorMode,
		bias,
		scale);
}

void hqClearGradient()
{
	globals.hqGradientType = GRADIENT_NONE;
}

void hqSetTexture(const GxTextureId texture, const Mat4x4 & matrix)
{
	globals.hqTextureEnabled = true;
	globals.hqTexture = texture;
	globals.hqTextureMatrix = matrix;
}

void hqSetTextureScreen(const GxTextureId texture, float x1, float y1, float x2, float y2)
{
	const Mat4x4 matrix =
		Mat4x4(true)
			.Scale(1.f / fmaxf(x2 - x1, 1e-6f), 1.f / fmaxf(y2 - y1, 1e-6f), 1.f)
			.Translate(-x1, -y1, 0.f);
	
	hqSetTexture(texture, matrix);
}

void hqClearTexture()
{
	globals.hqTextureEnabled = false;
	globals.hqTexture = 0;
}

#if ENABLE_HQ_PRIMITIVES

void hqLine(float x1, float y1, float strokeSize1, float x2, float y2, float strokeSize2)
{
	gxTexCoord2f(strokeSize1, strokeSize2);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	gxTexCoord2f(x3, y3);
	for (int i = 0; i < 3; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillCircle(float x, float y, float radius)
{
	gxTexCoord2f(radius, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex2f(x, y);
}

void hqFillRect(float x1, float y1, float x2, float y2)
{
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillRoundedRect(float x1, float y1, float x2, float y2, float radius)
{
	gxTexCoord2f(radius, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqStrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float stroke)
{
	gxNormal3f(x3, y3, stroke); // todo : use color ?
	for (int i = 0; i < 3; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqStrokeCircle(float x, float y, float radius, float stroke)
{
	gxTexCoord2f(radius, stroke);
	for (int i = 0; i < 4; ++i)
		gxVertex2f(x, y);
}

void hqStrokeRect(float x1, float y1, float x2, float y2, float stroke)
{
	gxTexCoord2f(stroke, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqStrokeRoundedRect(float x1, float y1, float x2, float y2, float radius, float stroke)
{
	gxTexCoord2f(radius, stroke);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

#else

// these are really shitty regular OpenGL approximations to the HQ primitives. don't expect much when using them!

void hqLine(float x1, float y1, float strokeSize1, float x2, float y2, float strokeSize2)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
}

void hqFillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
	gxVertex2f(x3, y3);
}

void hqFillCircle(float x, float y, float radius)
{
	radius *= s_hqScale;
	
	const int numSegments = radius * 6.f + 4.f;
	
	fillCircle(x, y, radius, numSegments);
}

void hqFillRect(float x1, float y1, float x2, float y2)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
}

void hqFillRoundedRect(float x1, float y1, float x2, float y2, float radius)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
}

void hqStrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float stroke)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
	
	gxVertex2f(x2, y2);
	gxVertex2f(x3, y3);
	
	gxVertex2f(x3, y3);
	gxVertex2f(x1, y1);
}

void hqStrokeCircle(float x, float y, float radius, float stroke)
{
	radius *= s_hqScale;
	
	const int numSegments = radius * 6.f + 4.f;
	
	drawCircle(x, y, radius, numSegments);
}

void hqStrokeRect(float x1, float y1, float x2, float y2, float stroke)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
}

void hqStrokeRoundedRect(float x1, float y1, float x2, float y2, float radius, float stroke)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
}

#endif
