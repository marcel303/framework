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

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#elif defined(ANDROID)
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

#include "renderTarget.h"
#include "StringEx.h" // strcpy_s

// -- render passes --

static const int kMaxColorTargets = 8;

static GLuint s_frameBufferId = 0;

static GLuint s_oldReadBuffer = 0;
static GLuint s_oldDrawBuffer = 0;

extern bool s_renderPassIsBackbufferPass;

//

void beginRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName, const float backingScale)
{
	beginRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName, backingScale);
}

void beginRenderPass(ColorTarget ** targets, const int numTargets, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName, const float backingScale)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	Assert(s_frameBufferId == 0);
	Assert(s_oldReadBuffer == 0);
	Assert(s_oldDrawBuffer == 0);
	AssertBackingScaleConstraint(backingScale);
	
	// capture the currently bound framebuffers. we will restore them at the end of endRenderPass
	
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&s_oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&s_oldDrawBuffer);
	checkErrorGL();
	
	//
	
	glGenFramebuffers(1, &s_frameBufferId);
	checkErrorGL();

	if (s_frameBufferId == 0)
	{
		logError("failed to create frame buffer object");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, s_frameBufferId);
	checkErrorGL();
	
	s_renderPassIsBackbufferPass = false;
	
	int backingSx = 0;
	int backingSy = 0;
	
	// specify the color and depth attachment(s)
	
	GLenum drawBuffers[kMaxColorTargets];
	int numDrawBuffers = 0;
	
	for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
	{
		if (targets[i] != nullptr)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, targets[i]->getTextureId(), 0);
			checkErrorGL();
			
			if (targets[i]->getWidth() > backingSx)
				backingSx = targets[i]->getWidth();
			if (targets[i]->getHeight() > backingSy)
				backingSy = targets[i]->getHeight();
			
			drawBuffers[numDrawBuffers++] = GL_COLOR_ATTACHMENT0 + i;
		}
		else
		{
			drawBuffers[numDrawBuffers++] = GL_NONE;
		}
	}
	
	// note : glDrawBuffers applies to the current GL_DRAW_FRAMEBUFFER only
	glDrawBuffers(numDrawBuffers, drawBuffers);
	checkErrorGL();
	
	if (depthTarget != nullptr)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getTextureId_unchecked(), 0);
		checkErrorGL();
		
		if (depthTarget->getWidth() > backingSx)
			backingSx = depthTarget->getWidth();
		if (depthTarget->getHeight() > backingSy)
			backingSy = depthTarget->getHeight();
	}
	
#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	// check if all went well
	// note : we only do this when debugging OpenGL, as this call can be rather expensive
	
	const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		logError("failed to init frame buffer. status: %d", status);
	}
#endif

	// disable draw rect
	
	clearDrawRect();
	
	// clear targets when requested
	
	if (glClearBufferfv != nullptr)
	{
		if (clearColor)
		{
			for (int i = 0; i < numTargets; ++i)
			{
				if (targets[i] != nullptr)
				{
					const Color & color = targets[i]->getClearColor();
					glClearBufferfv(GL_COLOR, i, &color.r);
					checkErrorGL();
				}
			}
		}
		
		if (clearDepth && depthTarget != nullptr)
		{
			const float depth = depthTarget->getClearDepth();
			glClearBufferfi(GL_DEPTH_STENCIL, 0, depth, 0x00);
			checkErrorGL();
		}
	}
	else
	{
		if (clearColor)
		{
			pushColorWriteMask(1, 1, 1, 1);
			for (int i = 0; i < numTargets; ++i)
			{
				GLenum drawBuffer = GL_COLOR_ATTACHMENT0 + i;
				glDrawBuffers(1, &drawBuffer);

				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT);
				checkErrorGL();
			}
			popColorWriteMask();
			
			glDrawBuffers(numDrawBuffers, drawBuffers);
			checkErrorGL();
		}
	
		if (clearDepth && depthTarget != nullptr)
		{
		#if ENABLE_DESKTOP_OPENGL
			glClearDepth(depthTarget->getClearDepth());
			checkErrorGL();
		#else
			glClearDepthf(depthTarget->getClearDepth());
			checkErrorGL();
		#endif
			
			//
			
			glClearStencil(0x00);
			glStencilMask(0xff);
			checkErrorGL();
			
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			checkErrorGL();
			
			glStencilMaskSeparate(GL_FRONT, globals.frontStencilState.writeMask);
			glStencilMaskSeparate(GL_BACK, globals.backStencilState.writeMask);
			checkErrorGL();
		}
	}
	
	// update globals
	
	globals.renderPass.isActive = true;
	globals.renderPass.backingSx = backingSx;
	globals.renderPass.backingSy = backingSy;
	globals.renderPass.backingScale = backingScale;
	globals.renderPass.viewportSx = float(backingSx) / backingScale;
	globals.renderPass.viewportSy = float(backingSy) / backingScale;
	
	pushContentScale(backingScale);
	
	// update viewport
	
	glViewport(0, 0, backingSx, backingSy);
	
	// apply transform
	
	applyTransform();
}

void beginBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName, const float backingScale)
{
	Assert(s_frameBufferId == 0);
	AssertBackingScaleConstraint(backingScale);
	
	// capture the currently bound framebuffers. we will restore them at the end of endRenderPass
	
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&s_oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&s_oldDrawBuffer);
	checkErrorGL();
	
	// note : we don't have to bind a new framebuffer. we assume here the currently bound framebuffer, is
	//        already the backbuffer. this must be true, as render passes are not allowed to overlap, and
	//        the backbuffer should be bound (restored) at the end of each render pass
	
	s_frameBufferId = 0;
	
	s_renderPassIsBackbufferPass = true;
	
	// disable draw rect
	
	clearDrawRect();
	
	//
	
	int clearFlags = 0;

	if (clearColor)
	{
		glClearColor(color.r, color.g, color.b, color.a);
		checkErrorGL();
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}

	if (clearDepth)
	{
	#if ENABLE_DESKTOP_OPENGL
		glClearDepth(depth);
		checkErrorGL();
	#else
		glClearDepthf(depth);
		checkErrorGL();
	#endif
		glClearStencil(0);
		glStencilMask(0xff); // note : glClear is affected by the stencil write mask
		checkErrorGL();
		
		clearFlags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	}

	if (clearFlags)
	{
		glClear(clearFlags);
		checkErrorGL();
	}
	
	if (clearDepth)
	{
		// restore the stencil write mask
		glStencilMaskSeparate(GL_FRONT, globals.frontStencilState.writeMask);
		glStencilMaskSeparate(GL_BACK, globals.backStencilState.writeMask);
		checkErrorGL();
	}

	int backingSx = 0;
	int backingSy = 0;
#if FRAMEWORK_USE_SDL
	if (globals.currentWindow->getWindow())
		SDL_GL_GetDrawableSize(globals.currentWindow->getWindow(), &backingSx, &backingSy);
	else
		AssertMsg(false, "beginBackbufferRenderPass called when no SDL window set");
#else
	AssertMsg(false, "beginBackbufferRenderPass called when no backbuffer is available");
#endif

	// update globals
	
	globals.renderPass.isActive = true;
	globals.renderPass.backingSx = backingSx;
	globals.renderPass.backingSy = backingSy;
	globals.renderPass.backingScale = backingScale;
	globals.renderPass.viewportSx = float(backingSx) / backingScale;
	globals.renderPass.viewportSy = float(backingSy) / backingScale;
	
	pushContentScale(backingScale);
	
	// update viewport
	
	glViewport(0, 0, backingSx, backingSy);
	
	// apply transform
	
	applyTransform();
}

void endRenderPass()
{
	// note : for MSAA, SDL uses a separate framebuffer. we ask/store the framebuffer used by the current windows, and set that instead of the framebuffer with id zero

	popContentScale();
	
	globals.renderPass.isActive = false;
	
	// restore the previous frame buffer bindings
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_oldReadBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_oldDrawBuffer);
	checkErrorGL();
	
	s_oldReadBuffer = 0;
	s_oldDrawBuffer = 0;
	
	// free the temporary framebuffer we rendered into
	
	if (s_frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &s_frameBufferId);
		checkErrorGL();
		
		s_frameBufferId = 0;
	}
	
#if false
	// unbind textures. we must do this to ensure no render target texture is currently bound as a texture
	// as this would cause issues where the driver may perform an optimization where it detects no texture
	// state change happened in a future gxSetTexture or Shader::setTexture call (because the texture ids
	// are the same), making it fail to flush GPU render target caches, fail to perform texture decompression,
	// fail to perform whatever is needed to transition a render target texture from being 'renderable' resource
	// to being a shader accessible resource

	for (int i = 0; i < 8; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}

	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
	
	globals.gxShaderIsDirty = true;
#endif

	// update viewport
	
	int backingSx = 0;
	int backingSy = 0;
#if FRAMEWORK_USE_SDL
	if (globals.currentWindow->getWindow())
		SDL_GL_GetDrawableSize(globals.currentWindow->getWindow(), &backingSx, &backingSy);
#endif

	glViewport(0, 0, backingSx, backingSy);
	
	// apply transform
	
	applyTransform();
}

// --- render passes stack ---

#include <stack>

struct RenderPassData
{
	ColorTarget * target[kMaxColorTargets];
	int numTargets = 0;
	DepthTarget * depthTarget = nullptr;
	bool isBackbufferPass = false;
	char passName[32] = { };
	float backingScale;
};

static std::stack<RenderPassData> s_renderPasses;

void pushRenderPass(
	ColorTarget * target,
	const bool clearColor,
	DepthTarget * depthTarget,
	const bool clearDepth,
	const char * passName,
	const float backingScale)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName, backingScale);
}

void pushRenderPass(
	ColorTarget ** targets,
	const int numTargets,
	const bool clearColor,
	DepthTarget * depthTarget,
	const bool clearDepth,
	const char * passName,
	const float backingScale)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	pushDrawRect();
	
	pushContentScale(getContentScale());
	
	// end the current pass (if any) and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginRenderPass(targets, numTargets, clearColor, depthTarget, clearDepth, passName, backingScale);
	
	// record the current render pass information in the render passes stack
	
	RenderPassData pd;
	for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
		pd.target[pd.numTargets++] = targets[i];
	pd.depthTarget = depthTarget;
	strcpy_s(pd.passName, sizeof(pd.passName), passName);
	pd.backingScale = backingScale;
	
	s_renderPasses.push(pd);
}

void pushBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName, const float backingScale)
{
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	pushDrawRect();
	
	pushContentScale(getContentScale());
	
	// end the current pass (if any) and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginBackbufferRenderPass(clearColor, color, clearDepth, depth, passName, backingScale);
	
	// record the current render pass information in the render passes stack
	
	RenderPassData pd;
	pd.isBackbufferPass = true;
	strcpy_s(pd.passName, sizeof(pd.passName), passName);
	pd.backingScale = backingScale;
	
	s_renderPasses.push(pd);
}

void popRenderPass()
{
	// end the current pass
	
	endRenderPass();
	
	s_renderPasses.pop();
	
	// check if there was a previous pass. if so, begin a continuation of it
	
	if (s_renderPasses.empty())
	{
		s_renderPassIsBackbufferPass = false;
	}
	else
	{
		auto & new_pd = s_renderPasses.top();
		
		if (new_pd.isBackbufferPass)
		{
			beginBackbufferRenderPass(false, colorBlackTranslucent, false, 0.f, new_pd.passName, new_pd.backingScale);
		}
		else
		{
			beginRenderPass(new_pd.target, new_pd.numTargets, false, new_pd.depthTarget, false, new_pd.passName, new_pd.backingScale);
		}
	}
	
	// restore state
	
	gxMatrixMode(GX_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPopMatrix();
	
	popDrawRect();
	
	popContentScale();
}

#endif
