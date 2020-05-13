#pragma once

#include <GLES3/gl3.h>

struct ovrTextureSwapChain;

struct ovrFramebuffer
{
    int Width = 0;
    int Height = 0;
    int Multisamples = 0;
    int TextureSwapChainLength = 0;
    int TextureSwapChainIndex = 0;
    bool UseMultiview = false;
    ovrTextureSwapChain * ColorTextureSwapChain = nullptr;
    GLuint * DepthBuffers = nullptr;
    GLuint * FrameBuffers = nullptr;

    bool init(
        const bool useMultiview,
        const GLenum colorFormat,
        const int width,
        const int height,
        const int multisamples);
    void shut();
};
