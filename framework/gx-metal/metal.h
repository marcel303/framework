#pragma once

#include "framework.h"

#define REMOVEME 0

#if REMOVEME
struct SDL_Window;
#endif

class ColorTarget;
class DepthTarget;

#if REMOVEME
class GxVertexBuffer;
class Shader;
#endif

void metal_init();
void metal_attach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_draw_begin(const float r, const float g, const float b, const float a);
void metal_draw_end();
void metal_set_viewport(const int sx, const int sy);

#if REMOVEME
typedef int GxTextureId;

// -- render states --

enum BLEND_MODE // setBlend
{
	BLEND_OPAQUE,
	BLEND_ALPHA,
	BLEND_PREMULTIPLIED_ALPHA,
	BLEND_PREMULTIPLIED_ALPHA_DRAW,
	BLEND_ADD,
	BLEND_ADD_OPAQUE,
	BLEND_SUBTRACT,
	BLEND_INVERT,
	BLEND_MUL,
	BLEND_MIN,
	BLEND_MAX
};

enum DEPTH_TEST
{
	DEPTH_EQUAL,
	DEPTH_LESS,
	DEPTH_LEQUAL,
	DEPTH_GREATER,
	DEPTH_GEQUAL,
	DEPTH_ALWAYS
};

enum CULL_MODE
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK
};

enum CULL_WINDING
{
	CULL_CCW,
	CULL_CW
};

#include "surface.h"

void pushSurface(Surface * surface);
void popSurface();
#endif

void pushRenderPass(ColorTarget * target, DepthTarget * depthTarget, const bool clearDepth);
void pushRenderPass(ColorTarget ** targets, const int numTargets, DepthTarget * depthTarget, const bool clearDepth);
void popRenderPass();

#if REMOVEME
void setBlend(BLEND_MODE blendMode);
void setLineSmooth(bool enabled);
void setWireframe(bool enabled);
void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled = true);
void setCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding);

void setShader(Shader & shader);
void clearShader();
#endif

#if REMOVEME
// -- gpu resources --

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp);

void freeTexture(GxTextureId & textureId);

// -- gx api implementation --

enum GX_PRIMITIVE_TYPE
{
	GX_INVALID_PRIM = -1,
	GX_POINTS,
	GX_LINES,
	GX_LINE_LOOP,
	GX_LINE_STRIP,
	GX_TRIANGLES,
	GX_TRIANGLE_FAN,
	GX_TRIANGLE_STRIP,
	GX_QUADS
};

enum GX_MATRIX
{
	GX_MODELVIEW,
	GX_PROJECTION
};

enum GX_SAMPLE_FILTER
{
	GX_SAMPLE_NEAREST,
	GX_SAMPLE_LINEAR
};

#include "mesh.h" // todo : remove

void gxMatrixMode(GX_MATRIX mode);
GX_MATRIX gxGetMatrixMode();
void gxPopMatrix();
void gxPushMatrix();
void gxLoadIdentity();
void gxLoadMatrixf(const float * m);
void gxGetMatrixf(GX_MATRIX mode, float * m);
void gxSetMatrixf(GX_MATRIX mode, const float * m);
void gxMultMatrixf(const float * m);
void gxTranslatef(float x, float y, float z);
void gxRotatef(float angle, float x, float y, float z);
void gxScalef(float x, float y, float z);
void gxValidateMatrices();

void gxInitialize();
void gxShutdown();
void gxBegin(GX_PRIMITIVE_TYPE primitiveType);
void gxEnd();
void gxEmitVertices(int primitiveType, int numVertices);
void gxColor4f(float r, float g, float b, float a);
void gxColor4fv(const float * rgba);
void gxColor3ub(int r, int g, int b);
void gxColor4ub(int r, int g, int b, int a);
void gxTexCoord2f(float u, float v);
void gxNormal3f(float x, float y, float z);
void gxNormal3fv(const float * v);
void gxVertex2f(float x, float y);
void gxVertex3f(float x, float y, float z);
void gxVertex3fv(const float * v);
void gxVertex4f(float x, float y, float z, float w);
void gxVertex4fv(const float * v);
void gxSetTexture(GxTextureId texture);
void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp);
#endif

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

// -- render states --

struct __attribute__((packed)) RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
	
	GxVertexInput vertexInputs[8] = { };
	int vertexInputCount = 0;
	int vertexStride = 0;
};

extern RenderPipelineState renderState;
