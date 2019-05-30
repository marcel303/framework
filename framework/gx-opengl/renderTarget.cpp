#include <GL/glew.h>
#include "framework.h"

#if ENABLE_OPENGL

#import "renderTarget.h"

static GLenum translateColorFormat(const SURFACE_FORMAT format)
{
	GLenum glFormat = GL_INVALID_ENUM;

	if (format == SURFACE_RGBA8)
		glFormat = GL_RGBA8;
	if (format == SURFACE_RGBA16F)
		glFormat = GL_RGBA16F;
	if (format == SURFACE_RGBA32F)
		glFormat = GL_RGBA32F;
	if (format == SURFACE_R8)
		glFormat = GL_R8;
	if (format == SURFACE_R16F)
		glFormat = GL_R16F;
	if (format == SURFACE_R32F)
		glFormat = GL_R32F;
	if (format == SURFACE_RG16F)
		glFormat = GL_RG16F;
	if (format == SURFACE_RG32F)
		glFormat = GL_RG32F;
	
	return glFormat;
}

ColorTarget::~ColorTarget()
{

}

bool ColorTarget::init(const ColorTargetProperties & in_properties)
{
	bool result = true;
	
	properties = in_properties;
	
	// allocate storage

	fassert(m_colorTextureId == 0);
	glGenTextures(1, &m_colorTextureId);
	checkErrorGL();

	if (m_colorTextureId == 0)
		result = false;
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_colorTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		const GLenum glFormat = translateColorFormat(properties.format);

	#if USE_LEGACY_OPENGL
		GLenum uploadFormat = GL_INVALID_ENUM;
		GLenum uploadType = GL_INVALID_ENUM;

		if (properties.format == SURFACE_RGBA8)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_UNSIGNED_BYTE;
		}
		if (properties.format == SURFACE_RGBA16F)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_RGBA32F)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_R8)
		{
			uploadFormat = GL_RED;
			uploadType = GL_UNSIGNED_BYTE;
		}
		if (properties.format == SURFACE_R16F)
		{
			uploadFormat = GL_RED;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_R32F)
		{
			uploadFormat = GL_RED;
			uploadType = GL_FLOAT;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, properties.dimensions.width, properties.dimensions.height, 0, uploadFormat, uploadType, nullptr);
		checkErrorGL();
	#else
		glTexStorage2D(GL_TEXTURE_2D, 1, glFormat, properties.dimensions.width, properties.dimensions.height);
		checkErrorGL();
	#endif

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
	// todo : properly restore previous texture
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}
	
	if (result == false)
	{
		if (m_colorTextureId != 0)
		{
			glDeleteTextures(1, &m_colorTextureId);
			m_colorTextureId = 0;
		}
	}
	
	return result;
}

//

static GLenum translateDepthFormat(const DEPTH_FORMAT format)
{
	GLenum glFormat = GL_INVALID_ENUM;

	if (format == DEPTH_FLOAT16)
		glFormat = GL_DEPTH_COMPONENT16;
	if (format == DEPTH_FLOAT32)
		glFormat = GL_DEPTH_COMPONENT32;
	
	return glFormat;
}

DepthTarget::~DepthTarget()
{

}

bool DepthTarget::init(const DepthTargetProperties & in_properties)
{
	bool result = true;
	
	properties = in_properties;
	
	fassert(m_depthTextureId == 0);
	glGenTextures(1, &m_depthTextureId);
	checkErrorGL();

	if (m_depthTextureId == 0)
		result = false;
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		const GLenum glFormat = translateDepthFormat(properties.format);

	#if USE_LEGACY_OPENGL
		GLenum uploadFormat = GL_DEPTH_COMPONENT;
		GLenum uploadType = GL_FLOAT;

		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, properties.dimensions.width, properties.dimensions.height, 0, uploadFormat, uploadType, 0);
		checkErrorGL();
	#else
		glTexStorage2D(GL_TEXTURE_2D, 1, glFormat, properties.dimensions.width, properties.dimensions.height);
		checkErrorGL();
	#endif

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
	// todo : properly restore previous texture
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}
	
	if (result == false)
	{
		if (m_depthTextureId != 0)
		{
			glDeleteTextures(1, &m_depthTextureId);
			m_depthTextureId = 0;
		}
	}
	
	return result;
}

// -- render passes --

#include <stack>

struct RenderPassData
{
	GLuint frameBufferId = 0;
};

std::stack<RenderPassData> s_renderPasses;

void pushRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName);
}

void pushRenderPass(ColorTarget ** targets, const int numTargets, const bool in_clearColor, DepthTarget * depthTarget, const bool in_clearDepth, const char * passName)
{
	Assert(numTargets >= 0 && numTargets <= 4);
	
	RenderPassData pd;
	
	glGenFramebuffers(1, &pd.frameBufferId);
	checkErrorGL();

	if (pd.frameBufferId == 0)
		logError("failed to create frame buffer object");
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pd.frameBufferId);
		checkErrorGL();
		
		int viewportSx = 0;
		int viewportSy = 0;
		
		// specify the color and depth attachment(s)
		
		for (int i = 0; i < numTargets; ++i)
		{
			if (targets[i] != nullptr)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, targets[i]->getTextureId(), 0);
				checkErrorGL();
				
				if (targets[i]->getWidth() > viewportSx)
					viewportSx = targets[i]->getWidth();
				if (targets[i]->getHeight() > viewportSy)
					viewportSy = targets[i]->getHeight();
			}
		}
		
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
		
		int clearFlags = 0;
	
		if (in_clearColor)
		{
		// todo : use extended clear color function
			glClearColor(0, 0, 0, 0);
			clearFlags |= GL_COLOR_BUFFER_BIT;
		}
	
		if (in_clearDepth && depthTarget != nullptr)
		{
			glClearDepth(depthTarget->getClearDepth());
			clearFlags |= GL_DEPTH_BUFFER_BIT;
		}
	
		if (clearFlags)
		{
			glClear(clearFlags);
			checkErrorGL();
		}
	
		// begin encoding
		
		s_renderPasses.push(pd);
		
		// todo : set viewport
		
	// todo : applyTransformWithViewportSize should be called here
	// todo : getCurrentViewportSize should be updated to know about the active render targets, otherwise applyTransformWithViewportSize will be passed the incorrect size
	
		glViewport(0, 0, viewportSx, viewportSy);
		
		// todo : set blend mode
	}
}

void popRenderPass()
{
	auto & old_pd = s_renderPasses.top();
	
	if (old_pd.frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &old_pd.frameBufferId);
		checkErrorGL();
	}
	
	s_renderPasses.pop();
	
	if (s_renderPasses.empty())
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		checkErrorGL();
	}
	else
	{
		auto & new_pd = s_renderPasses.top();
		
		glBindFramebuffer(GL_FRAMEBUFFER, new_pd.frameBufferId);
		checkErrorGL();
	}
}


#endif
