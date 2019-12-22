#include "framework.h"

#if ENABLE_OPENGL

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#else
	#include <GL/glew.h>
#endif

#include "renderTarget.h"

// -- render passes --

static const int kMaxColorTargets = 8;

static GLuint s_frameBufferId = 0;

extern bool s_renderPassIsBackbufferPass;

void beginRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName)
{
	beginRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName);
}

void beginRenderPass(ColorTarget ** targets, const int numTargets, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	Assert(s_frameBufferId == 0);
	
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
	
	int viewportSx = 0;
	int viewportSy = 0;
	
	// specify the color and depth attachment(s)
	
	GLenum drawBuffers[kMaxColorTargets];
	int numDrawBuffers = 0;
	
	for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
	{
		if (targets[i] != nullptr)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, targets[i]->getTextureId(), 0);
			checkErrorGL();
			
			if (targets[i]->getWidth() > viewportSx)
				viewportSx = targets[i]->getWidth();
			if (targets[i]->getHeight() > viewportSy)
				viewportSy = targets[i]->getHeight();
			
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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getTextureId(), 0);
		checkErrorGL();
		
		if (depthTarget->getWidth() > viewportSx)
			viewportSx = depthTarget->getWidth();
		if (depthTarget->getHeight() > viewportSy)
			viewportSy = depthTarget->getHeight();
	}
	
#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	// check if all went well
	// note : we only do this when debugging OpenGL, as this call can be rather expensive
	
	const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		logError("failed to init surface. status: %d", status);
	}
#endif

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
			glClearBufferfv(GL_DEPTH, 0, &depth);
			checkErrorGL();
		}
	}
	else
	{
		int clearFlags = 0;
		
		if (clearColor)
		{
		// todo : use extended clear color function
			Assert(false);
			glClearColor(0, 0, 0, 0);
			clearFlags |= GL_COLOR_BUFFER_BIT;
		}
	
		if (clearDepth && depthTarget != nullptr)
		{
		#if ENABLE_DESKTOP_OPENGL
			glClearDepth(depthTarget->getClearDepth());
		#else
			glClearDepthf(depthTarget->getClearDepth());
		#endif
			clearFlags |= GL_DEPTH_BUFFER_BIT;
		}
	
		if (clearFlags)
		{
			glClear(clearFlags);
			checkErrorGL();
		}
	}
	
	// set viewport and apply transform
	
// todo : applyTransformWithViewportSize should be called here
// todo : getCurrentViewportSize should be updated to know about the active render targets, otherwise applyTransformWithViewportSize will be passed the incorrect size

	glViewport(0, 0, viewportSx, viewportSy);
	
	applyTransform();
}

void beginBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName)
{
	Assert(s_frameBufferId == 0);
	
	s_frameBufferId = 0;
	
	glBindFramebuffer(GL_FRAMEBUFFER, s_frameBufferId);
	checkErrorGL();
	
	//
	
	s_renderPassIsBackbufferPass = true;
	
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
		clearFlags |= GL_DEPTH_BUFFER_BIT;
	}

	if (clearFlags)
	{
		glClear(clearFlags);
		checkErrorGL();
	}

	// todo : set viewport and apply transform

	applyTransform();
	
// todo : applyTransformWithViewportSize should be called here
// todo : getCurrentViewportSize should be updated to know about the active render targets, otherwise applyTransformWithViewportSize will be passed the incorrect size
}

void endRenderPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkErrorGL();
	
	if (s_frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &s_frameBufferId);
		checkErrorGL();
		
		s_frameBufferId = 0;
	}
	
	//
	
	applyTransform(); // for viewport
}

// --- render passes stack ---

#include <stack>

struct RenderPassData
{
	ColorTarget * target[kMaxColorTargets];
	int numTargets = 0;
	DepthTarget * depthTarget = nullptr;
	bool isBackbufferPass = false;
};

static std::stack<RenderPassData> s_renderPasses;

void pushRenderPass(
	ColorTarget * target,
	const bool clearColor,
	DepthTarget * depthTarget,
	const bool clearDepth,
	const char * passName)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName);
}

void pushRenderPass(
	ColorTarget ** targets,
	const int numTargets,
	const bool clearColor,
	DepthTarget * depthTarget,
	const bool clearDepth,
	const char * passName)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	// end the current pass (if any) and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginRenderPass(targets, numTargets, clearColor, depthTarget, clearDepth, passName);
	
	// record the current render pass information in the render passes stack
	
	RenderPassData pd;
	
	for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
		pd.target[pd.numTargets++] = targets[i];
	
	pd.depthTarget = depthTarget;
	
	s_renderPasses.push(pd);
}

void pushBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName)
{
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	// end the current pass (if any) and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginBackbufferRenderPass(clearColor, color, clearDepth, depth, passName);
	
	// record the current render pass information in the render passes stack
	
	RenderPassData pd;
	
	pd.isBackbufferPass = true;
	
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
			beginBackbufferRenderPass(false, colorBlackTranslucent, false, 0.f, "(cont)"); // todo : pass name
		}
		else
		{
			beginRenderPass(new_pd.target, new_pd.numTargets, false, new_pd.depthTarget, false, "(cont)"); // todo : pass name
		}
	}

	// setup viewport
	
	applyTransform(); // for viewport
	
	// restore state
	
	gxMatrixMode(GX_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPopMatrix();
}

#endif
