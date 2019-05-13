#pragma once

struct SDL_Window;

void metal_init();
void metal_attach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_draw_begin(const float r, const float g, const float b, const float a);
void metal_draw_end();

// --- gx api implementation ---

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
