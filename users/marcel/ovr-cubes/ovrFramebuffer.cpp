#include "Log.h"

#include "ovrFramebuffer.h"
#include "opengl-ovr.h"

#include <VrApi.h>
#include <VrApi_Types.h>

static void checkErrorGL()
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        LOG_ERR("GL error: %s", GlErrorString(error));
}

bool ovrFramebuffer::init(
    const bool in_useMultiview,
    const GLenum in_colorFormat,
    const int in_width,
    const int in_height,
    const int in_multisamples)
{
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glRenderbufferStorageMultisampleEXT");
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glFramebufferTexture2DMultisampleEXT");

    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
            (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
                    "glFramebufferTextureMultiviewOVR");
    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
            (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)eglGetProcAddress(
                    "glFramebufferTextureMultisampleMultiviewOVR");

    Width = in_width;
    Height = in_height;
    Multisamples = in_multisamples;
    UseMultiview = in_useMultiview && (glFramebufferTextureMultiviewOVR != nullptr);

    ColorTextureSwapChain = vrapi_CreateTextureSwapChain3(
        in_useMultiview
        ? VRAPI_TEXTURE_TYPE_2D_ARRAY
        : VRAPI_TEXTURE_TYPE_2D,
        in_colorFormat,
        in_width,
        in_height,
        1,
        3);

    TextureSwapChainLength = vrapi_GetTextureSwapChainLength(ColorTextureSwapChain);
    DepthBuffers = new GLuint[TextureSwapChainLength];
    FrameBuffers = new GLuint[TextureSwapChainLength];

    LOG_DBG("        frameBuffer->UseMultiview = %d", UseMultiview);

    for (int i = 0; i < TextureSwapChainLength; i++)
    {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(ColorTextureSwapChain, i);
        const GLenum colorTextureTarget = UseMultiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

        glBindTexture(colorTextureTarget, colorTexture);
        checkErrorGL();

        if (glExtensions.EXT_texture_border_clamp)
        {
            glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            checkErrorGL();

            const GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor);
            checkErrorGL();
        }
        else
        {
            // Just clamp to edge. However, this requires manually clearing the border
            // around the layer to clear the edge texels.
            glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            checkErrorGL();
        }

        glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        checkErrorGL();

        glBindTexture(colorTextureTarget, 0);
        checkErrorGL();

        if (in_useMultiview)
        {
            // Create the depth buffer texture.
            glGenTextures(1, &DepthBuffers[i]);
            glBindTexture(GL_TEXTURE_2D_ARRAY, DepthBuffers[i]);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, in_width, in_height, 2);
            checkErrorGL();
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            // Create the frame buffer.
            glGenFramebuffers(1, &FrameBuffers[i]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FrameBuffers[i]);
            if (in_multisamples > 1 && glFramebufferTextureMultisampleMultiviewOVR != nullptr)
            {
                glFramebufferTextureMultisampleMultiviewOVR(
                        GL_DRAW_FRAMEBUFFER,
                        GL_DEPTH_ATTACHMENT,
                        DepthBuffers[i],
                        0 /* level */,
                        in_multisamples /* samples */,
                        0 /* baseViewIndex */,
                        2 /* numViews */);
                checkErrorGL();

                glFramebufferTextureMultisampleMultiviewOVR(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        colorTexture,
                        0 /* level */,
                        in_multisamples /* samples */,
                        0 /* baseViewIndex */,
                        2 /* numViews */);
                checkErrorGL();
            }
            else
            {
                glFramebufferTextureMultiviewOVR(
                        GL_DRAW_FRAMEBUFFER,
                        GL_DEPTH_ATTACHMENT,
                        DepthBuffers[i],
                        0 /* level */,
                        0 /* baseViewIndex */,
                        2 /* numViews */);
                checkErrorGL();

                glFramebufferTextureMultiviewOVR(
                        GL_DRAW_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        colorTexture,
                        0 /* level */,
                        0 /* baseViewIndex */,
                        2 /* numViews */);
                checkErrorGL();
            }

            const GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            checkErrorGL();

            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
            {
                LOG_ERR("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        }
        else // !UseMultiview
        {
            if (in_multisamples > 1 &&
                glRenderbufferStorageMultisampleEXT != nullptr &&
                glFramebufferTexture2DMultisampleEXT != nullptr)
            {
                // Create multisampled depth buffer.
                glGenRenderbuffers(1, &DepthBuffers[i]);
                glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffers[i]);
                glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, in_multisamples, GL_DEPTH_COMPONENT24, in_width, in_height);
                checkErrorGL();
                glBindRenderbuffer(GL_RENDERBUFFER, 0);

                // Create the frame buffer.
                // NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
                glGenFramebuffers(1, &FrameBuffers[i]);
                glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffers[i]);
                glFramebufferTexture2DMultisampleEXT(
                        GL_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D,
                        colorTexture,
                        0,
                        in_multisamples);
                checkErrorGL();

                glFramebufferRenderbuffer(
                        GL_FRAMEBUFFER,
                        GL_DEPTH_ATTACHMENT,
                        GL_RENDERBUFFER,
                        DepthBuffers[i]);
                checkErrorGL();

                const GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                checkErrorGL();

                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
                {
                    LOG_ERR("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            }
            else
            {
                // Create depth buffer.
                glGenRenderbuffers(1, &DepthBuffers[i]);
                glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffers[i]);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, in_width, in_height);
                checkErrorGL();
                glBindRenderbuffer(GL_RENDERBUFFER, 0);

                // Create the frame buffer.
                glGenFramebuffers(1, &FrameBuffers[i]);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FrameBuffers[i]);
                glFramebufferRenderbuffer(
                        GL_DRAW_FRAMEBUFFER,
                        GL_DEPTH_ATTACHMENT,
                        GL_RENDERBUFFER,
                        DepthBuffers[i]);
                checkErrorGL();
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
                checkErrorGL();

                const GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
                {
                    LOG_ERR("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            }
        }
    }

    return true;
}

void ovrFramebuffer::shut()
{
    glDeleteFramebuffers(TextureSwapChainLength, FrameBuffers);
    checkErrorGL();

    if (UseMultiview)
    {
        glDeleteTextures(TextureSwapChainLength, DepthBuffers);
        checkErrorGL();
    }
    else
    {
        glDeleteRenderbuffers(TextureSwapChainLength, DepthBuffers);
        checkErrorGL();
    }

	vrapi_DestroyTextureSwapChain(ColorTextureSwapChain);

    delete [] DepthBuffers;
    delete [] FrameBuffers;

    *this = ovrFramebuffer();
}

//

void ovrFramebuffer_SetCurrent(ovrFramebuffer * frameBuffer)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex]);
	checkErrorGL();
}

void ovrFramebuffer_SetNone()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	checkErrorGL();
}

void ovrFramebuffer_Resolve(ovrFramebuffer * frameBuffer)
{
	// Discard the depth buffer, so the tiler won't need to write it back out to memory.
	const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
	checkErrorGL();

	// Flush this frame worth of commands.
	glFlush();
}

void ovrFramebuffer_Advance(ovrFramebuffer * frameBuffer)
{
	// Advance to the next texture from the set.
	frameBuffer->TextureSwapChainIndex = (frameBuffer->TextureSwapChainIndex + 1) % frameBuffer->TextureSwapChainLength;
}

void ovrFramebuffer_ClearBorder(ovrFramebuffer * frameBuffer)
{
	// Clear to fully opaque black.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// bottom
	glScissor(0, 0, frameBuffer->Width, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	checkErrorGL();

	// top
	glScissor(0, frameBuffer->Height - 1, frameBuffer->Width, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	checkErrorGL();

	// left
	glScissor(0, 0, 1, frameBuffer->Height);
	glClear(GL_COLOR_BUFFER_BIT);
	checkErrorGL();

	// right
	glScissor(frameBuffer->Width - 1, 0, 1, frameBuffer->Height);
	glClear(GL_COLOR_BUFFER_BIT);
	checkErrorGL();
}
