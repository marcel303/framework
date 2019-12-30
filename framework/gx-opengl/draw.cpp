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

#if !USE_LEGACY_OPENGL

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "gx_mesh.h"
#include "internal.h"
#include "Quat.h"
#include "shaders.h" // registerBuiltinShaders
#include <map>

#if defined(MACOS)
    #define INDEX_TYPE GL_UNSIGNED_INT
#else
    #define INDEX_TYPE GL_UNSIGNED_SHORT
#endif

#if INDEX_TYPE == GL_UNSIGNED_INT
	typedef unsigned int glindex_t;
#else
	typedef unsigned short glindex_t;
#endif

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

extern std::map<std::string, std::string> s_shaderSources; // todo : can this be exposed/determined more nicely?

class GxMatrixStack
{
public:
	static const int kSize = 32;
	Mat4x4 stack[kSize];
	int stackDepth;
	bool isDirty;
	
	GxMatrixStack()
	{
		stackDepth = 0;
		stack[0].MakeIdentity();
		
		isDirty = true;
	}
	
	void push()
	{
		fassert(stackDepth + 1 < kSize);
		stackDepth++;
		stack[stackDepth] = stack[stackDepth - 1];
	}
	
	void pop()
	{
		fassert(stackDepth > 0);
		stackDepth--;
		
		isDirty = true;
	}
	
	const Mat4x4 & get() const
	{
		return stack[stackDepth];
	}
	
	Mat4x4 & getRw()
	{
		isDirty = true;
		
		return stack[stackDepth];
	}
	
	void makeDirty()
	{
		isDirty = true;
	}
};

static GxMatrixStack s_gxModelView;
static GxMatrixStack s_gxProjection;
static GxMatrixStack * s_gxMatrixStack = &s_gxModelView;

void gxMatrixMode(GX_MATRIX mode)
{
	switch (mode)
	{
		case GX_MODELVIEW:
			s_gxMatrixStack = &s_gxModelView;
			break;
		case GX_PROJECTION:
			s_gxMatrixStack = &s_gxProjection;
			break;
		default:
			fassert(false);
			break;
	}
}

GX_MATRIX gxGetMatrixMode()
{
	if (s_gxMatrixStack == &s_gxModelView)
		return GX_MODELVIEW;
	if (s_gxMatrixStack == &s_gxProjection)
		return GX_PROJECTION;
	else
	{
		Assert(false);
		return GX_MODELVIEW;
	}
}

void gxPopMatrix()
{
	s_gxMatrixStack->pop();
}

void gxPushMatrix()
{
	s_gxMatrixStack->push();
}

void gxLoadIdentity()
{
	s_gxMatrixStack->getRw().MakeIdentity();
}

void gxLoadMatrixf(const float * m)
{
	memcpy(s_gxMatrixStack->getRw().m_v, m, sizeof(float) * 16);
}

void gxGetMatrixf(GX_MATRIX mode, float * m)
{
	switch (mode)
	{
		case GX_PROJECTION:
			memcpy(m, s_gxProjection.get().m_v, sizeof(float) * 16);
			break;
		case GX_MODELVIEW:
			memcpy(m, s_gxModelView.get().m_v, sizeof(float) * 16);
			break;
		default:
			fassert(false);
			break;
	}
}

void gxSetMatrixf(GX_MATRIX mode, const float * m)
{
	switch (mode)
	{
		case GX_PROJECTION:
			memcpy(s_gxProjection.getRw().m_v, m, sizeof(float) * 16);
			break;
		case GX_MODELVIEW:
			memcpy(s_gxModelView.getRw().m_v, m, sizeof(float) * 16);
			break;
		default:
			fassert(false);
			break;
	}
}

void gxMultMatrixf(const float * _m)
{
	Mat4x4 m;
	memcpy(m.m_v, _m, sizeof(m.m_v));

	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxTranslatef(float x, float y, float z)
{
	Mat4x4 m;
	m.MakeTranslation(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxRotatef(float angle, float x, float y, float z)
{
	Quat q;
	q.fromAxisAngle(Vec3(x, y, z), angle * M_PI / 180.f);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * q.toMatrix();
}

void gxScalef(float x, float y, float z)
{
	Mat4x4 m;
	m.MakeScaling(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxValidateMatrices()
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);
	
	//printf("validate1\n");
	
#if VS_USE_LEGACY_MATRICES
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(s_gxProjection.get().m_v);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(s_gxModelView.get().m_v);
#else
	if (globals.shader && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);

		const ShaderCacheElem & shaderElem = shader->getCacheElem();
		
		// check if matrices are dirty
		
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index, s_gxModelView.get().m_v);
			//printf("validate2\n");
		}
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty || s_gxProjection.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index, (s_gxProjection.get() * s_gxModelView.get()).m_v);
			//printf("validate3\n");
		}
		if ((globals.gxShaderIsDirty || s_gxProjection.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index, s_gxProjection.get().m_v);
			//printf("validate4\n");
		}
	}
#endif
	
	s_gxModelView.isDirty = false;
	s_gxProjection.isDirty = false;
}

struct GxVertex
{
	float px, py, pz, pw;
	float nx, ny, nz;
	float cx, cy, cz, cw;
	float tx, ty;
};

#define GX_BUFFER_DRAW_MODE GL_DYNAMIC_DRAW
//#define GX_BUFFER_DRAW_MODE GL_STREAM_DRAW
#if defined(MACOS)
    #define GX_USE_ELEMENT_ARRAY_BUFFER 1
#else
    #define GX_USE_ELEMENT_ARRAY_BUFFER 0
#endif

static Shader s_gxShader;
static GLuint s_gxVertexArrayObject = 0;
static GLuint s_gxVertexBufferObject = 0;
static GLuint s_gxIndexBufferObject = 0;
static GxVertex s_gxVertexBuffer[1024*16];

static GX_PRIMITIVE_TYPE s_gxPrimitiveType = GX_INVALID_PRIM;
static GxVertex * s_gxVertices = 0;
static int s_gxVertexCount = 0;
static int s_gxMaxVertexCount = 0;
static int s_gxPrimitiveSize = 0;
static GxVertex s_gxVertex = { };
static bool s_gxTextureEnabled = false;

static GX_PRIMITIVE_TYPE s_gxLastPrimitiveType = GX_INVALID_PRIM;
static int s_gxLastVertexCount = -1;

// for gxSetVertexBuffer, gxDrawIndexedPrimitives
static GLuint s_gxVertexArrayObjectForCustomDraw = 0;

static const GxVertexInput s_gxVsInputs[] =
{
	{ VS_POSITION, 4, GX_ELEMENT_FLOAT32, 0, offsetof(GxVertex, px), 0 },
	{ VS_NORMAL,   3, GX_ELEMENT_FLOAT32, 0, offsetof(GxVertex, nx), 0 },
	{ VS_COLOR,    4, GX_ELEMENT_FLOAT32, 0, offsetof(GxVertex, cx), 0 },
	{ VS_TEXCOORD, 2, GX_ELEMENT_FLOAT32, 0, offsetof(GxVertex, tx), 0 }
};
const int numGxVsInputs = sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]);

void gxEmitVertex();

void gxInitialize()
{
	fassert(s_shaderSources.empty());
	
	registerBuiltinShaders();

	s_gxShader.load("engine/Generic", "engine/Generic.vs", "engine/Generic.ps");
	
	memset(&s_gxVertex, 0, sizeof(s_gxVertex));
	s_gxVertex.cx = 1.f;
	s_gxVertex.cy = 1.f;
	s_gxVertex.cz = 1.f;
	s_gxVertex.cw = 1.f;

	fassert(s_gxVertexBufferObject == 0);
	glGenBuffers(1, &s_gxVertexBufferObject);
	
	fassert(s_gxIndexBufferObject == 0);
	glGenBuffers(1, &s_gxIndexBufferObject);
	
	// create vertex array
	fassert(s_gxVertexArrayObject == 0);
	glGenVertexArrays(1, &s_gxVertexArrayObject);
	checkErrorGL();

	glBindVertexArray(s_gxVertexArrayObject);
	checkErrorGL();
	{
	#if GX_USE_ELEMENT_ARRAY_BUFFER
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject);
		checkErrorGL();
	#endif
		glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject);
		checkErrorGL();
		bindVsInputs(s_gxVsInputs, numGxVsInputs, sizeof(GxVertex));
	}
	glBindVertexArray(0);
	checkErrorGL();
	
	// create vertex array for custom draw
	fassert(s_gxVertexArrayObjectForCustomDraw == 0);
	glGenVertexArrays(1, &s_gxVertexArrayObjectForCustomDraw);
	checkErrorGL();
	
#if ENABLE_DESKTOP_OPENGL
	// enable seamless cube map sampling along the edges
	
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
}

void gxShutdown()
{
#if ENABLE_DESKTOP_OPENGL
	glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
	
	if (s_gxVertexArrayObjectForCustomDraw != 0)
	{
		glDeleteVertexArrays(1, &s_gxVertexArrayObjectForCustomDraw);
		s_gxVertexArrayObjectForCustomDraw = 0;
	}
	
	if (s_gxVertexArrayObject != 0)
	{
		glDeleteVertexArrays(1, &s_gxVertexArrayObject);
		s_gxVertexArrayObject = 0;
	}
	
	if (s_gxVertexBufferObject != 0)
	{
		glDeleteBuffers(1, &s_gxVertexBufferObject);
		s_gxVertexBufferObject = 0;
	}

	if (s_gxIndexBufferObject != 0)
	{
		glDeleteBuffers(1, &s_gxIndexBufferObject);
		s_gxIndexBufferObject = 0;
	}
	
	s_gxPrimitiveType = GX_INVALID_PRIM;
	s_gxVertices = nullptr;
	s_gxVertexCount = 0;
	s_gxMaxVertexCount = 0;
	s_gxPrimitiveSize = 0;
	s_gxVertex = GxVertex();
	s_gxTextureEnabled = false;
	
	s_gxLastPrimitiveType = GX_INVALID_PRIM;
	s_gxLastVertexCount = -1;
}

static GLenum toOpenGLPrimitiveType(const GX_PRIMITIVE_TYPE primitiveType)
{
	switch (primitiveType)
	{
	case GX_POINTS:
		return GL_POINTS;
	case GX_LINES:
		return GL_LINES;
	case GX_LINE_LOOP:
		return GL_LINE_LOOP;
	case GX_LINE_STRIP:
		return GL_LINE_STRIP;
	case GX_TRIANGLES:
		return GL_TRIANGLES;
	case GX_TRIANGLE_FAN:
		return GL_TRIANGLE_FAN;
	case GX_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
#if ENABLE_DESKTOP_OPENGL
// todo : add check for legacy OpenGL here. since we cannot draw quads in modern OpenGL either
	case GX_QUADS:
		return GL_QUADS;
#endif
	default:
		Assert(false);
		return GL_INVALID_ENUM;
	}
}

static void gxFlush(bool endOfBatch)
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	if (s_gxVertexCount)
	{
		const GX_PRIMITIVE_TYPE primitiveType = s_gxPrimitiveType;

		Shader genericShader("engine/Generic");
		
		Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;

		setShader(shader);
		
		gxValidateMatrices();

		glBindVertexArray(s_gxVertexArrayObject);
		checkErrorGL();
		
		glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GxVertex) * s_gxVertexCount, s_gxVertices, GX_BUFFER_DRAW_MODE);
		checkErrorGL();
		
		bool indexed = false;
		glindex_t * indices = 0;
		int numElements = s_gxVertexCount;
		int numIndices = 0;

	#if !GX_USE_ELEMENT_ARRAY_BUFFER
		bool needToRegenerateIndexBuffer = true;
	#else
		bool needToRegenerateIndexBuffer = false;
        
		if (s_gxPrimitiveType != s_gxLastPrimitiveType || s_gxVertexCount != s_gxLastVertexCount)
		{
			s_gxLastPrimitiveType = s_gxPrimitiveType;
			s_gxLastVertexCount = s_gxVertexCount;

			needToRegenerateIndexBuffer = true;
		}
	#endif
	
		// convert quads to triangles
		
		if (s_gxPrimitiveType == GX_QUADS)
		{
			fassert(s_gxVertexCount <= 65536);
		
		#if GX_USE_ELEMENT_ARRAY_BUFFER
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject);
		#endif
			
			// todo: use triangle strip + compute index buffer once at init time
			
			const int numQuads = s_gxVertexCount / 4;
			numIndices = numQuads * 6;

			if (needToRegenerateIndexBuffer)
			{
				indices = (glindex_t*)alloca(sizeof(glindex_t) * numIndices);

				glindex_t * __restrict indexPtr = indices;
				glindex_t baseIndex = 0;
			
				for (int i = 0; i < numQuads; ++i)
				{
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 1;
					*indexPtr++ = baseIndex + 2;
				
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 2;
					*indexPtr++ = baseIndex + 3;
				
					baseIndex += 4;
				}
			
			#if GX_USE_ELEMENT_ARRAY_BUFFER
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glindex_t) * numIndices, indices, GX_BUFFER_DRAW_MODE);
				checkErrorGL();
			#endif
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
		
		const ShaderCacheElem & shaderElem = shader.getCacheElem();
		
		if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
		{
			shader.setImmediate(
				shaderElem.params[ShaderCacheElem::kSp_Params].index,
				s_gxTextureEnabled ? 1 : 0,
				globals.colorMode,
				globals.colorPost,
				globals.colorClamp);
		}

		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
				shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
		}
		
		if (shader.isValid())
		{
			const GLenum glPrimitiveType = toOpenGLPrimitiveType(s_gxPrimitiveType);

			if (indexed)
			{
			#if GX_USE_ELEMENT_ARRAY_BUFFER
				glDrawElements(glPrimitiveType, numElements, INDEX_TYPE, 0);
			#else
				glDrawElements(glPrimitiveType, numElements, INDEX_TYPE, indices);
			#endif
				checkErrorGL();
			}
			else
			{
				glDrawArrays(glPrimitiveType, 0, numElements);
				checkErrorGL();
			}
		}
		else
		{
			logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
		}
		
		if (endOfBatch)
		{
			s_gxVertexCount = 0;
		}
		else
		{
			switch (s_gxPrimitiveType)
			{
				case GX_LINE_LOOP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 1;
					break;
				case GX_LINE_STRIP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 1;
					break;
				case GX_TRIANGLE_FAN:
					s_gxVertices[0] = s_gxVertices[0];
					s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 2;
					break;
				case GX_TRIANGLE_STRIP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 2];
					s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 2;
					break;
				default:
					s_gxVertexCount = 0;
			}
		}
		
		globals.gxShaderIsDirty = false;

		s_gxPrimitiveType = primitiveType;
	}
	
	if (endOfBatch)
		s_gxVertices = 0;
}

void gxBegin(GX_PRIMITIVE_TYPE primitiveType)
{
	s_gxPrimitiveType = primitiveType;
	s_gxVertices = s_gxVertexBuffer;
	s_gxMaxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	
	switch (primitiveType)
	{
		case GX_TRIANGLES:
			s_gxPrimitiveSize = 3;
			break;
		case GX_QUADS:
			s_gxPrimitiveSize = 4;
			break;
		case GX_LINES:
			s_gxPrimitiveSize = 2;
			break;
		case GX_LINE_LOOP:
			s_gxPrimitiveSize = 1;
			break;
		case GX_LINE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		case GX_POINTS:
			s_gxPrimitiveSize = 1;
			break;
		case GX_TRIANGLE_FAN:
			s_gxPrimitiveSize = 1;
			break;
		case GX_TRIANGLE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		default:
			fassert(false);
	}
	
	fassert(s_gxVertexCount == 0);
}

void gxEnd()
{
	gxFlush(true);
}

void gxEmitVertices(GX_PRIMITIVE_TYPE primitiveType, int numVertices)
{
	fassert(primitiveType == GX_POINTS || primitiveType == GX_LINES || primitiveType == GX_TRIANGLES || primitiveType == GX_TRIANGLE_STRIP);
	
// todo : add to shaders struct, to avoid constant resource lookups here and at gxFlush
	Shader genericShader("engine/Generic");
	
	Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;

	setShader(shader);

	gxValidateMatrices();

	//

	glBindVertexArray(s_gxVertexArrayObject);
	checkErrorGL();

	//

	const ShaderCacheElem & shaderElem = shader.getCacheElem();

	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1 : 0,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}

	if (globals.gxShaderIsDirty)
	{
		if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
			shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
	}

	//

	if (shader.isValid())
	{
		glDrawArrays(toOpenGLPrimitiveType(primitiveType), 0, numVertices);
		checkErrorGL();
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}

	globals.gxShaderIsDirty = false;
}

void gxEmitVertex()
{
	s_gxVertices[s_gxVertexCount++] = s_gxVertex;
	
	if (s_gxVertexCount + s_gxPrimitiveSize > s_gxMaxVertexCount)
	{
		if (s_gxVertexCount % s_gxPrimitiveSize == 0)
		{
			gxFlush(false);
		}
	}
}

void gxColor4f(float r, float g, float b, float a)
{
	s_gxVertex.cx = r;
	s_gxVertex.cy = g;
	s_gxVertex.cz = b;
	s_gxVertex.cw = a;
}

void gxColor4fv(const float * rgba)
{
	s_gxVertex.cx = rgba[0];
	s_gxVertex.cy = rgba[1];
	s_gxVertex.cz = rgba[2];
	s_gxVertex.cw = rgba[3];
}

static const float s255 = 1.f / 255.f;

inline float scale255(const float v)
{
	return v * s255;
}

void gxColor3ub(int r, int g, int b)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		1.f);
}

void gxColor4ub(int r, int g, int b, int a)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		scale255(a));
}

void gxTexCoord2f(float u, float v)
{
	s_gxVertex.tx = u;
	s_gxVertex.ty = v;
}

void gxNormal3f(float x, float y, float z)
{
	s_gxVertex.nx = x;
	s_gxVertex.ny = y;
	s_gxVertex.nz = z;
}

void gxNormal3fv(const float * v)
{
	gxNormal3f(v[0], v[1], v[2]);
}

void gxVertex2f(float x, float y)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = 0.f;
	s_gxVertex.pw = 1.f;

	gxEmitVertex();
}

void gxVertex3f(float x, float y, float z)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = 1.f;
	
	gxEmitVertex();
}

void gxVertex4f(float x, float y, float z, float w)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = w;

	gxEmitVertex();
}

void gxVertex3fv(const float * v)
{
	gxVertex3f(v[0], v[1], v[2]);
}

void gxVertex4fv(const float * v)
{
	gxVertex4f(v[0], v[1], v[2], v[3]);
}

void gxSetTexture(GxTextureId texture)
{
	glActiveTexture(GL_TEXTURE0);
	
	if (texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
		
		s_gxTextureEnabled = true;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
		
		s_gxTextureEnabled = false;
	}
}

static GLenum toOpenGLSampleFilter(const GX_SAMPLE_FILTER filter)
{
	if (filter == GX_SAMPLE_NEAREST)
		return GL_NEAREST;
	else if (filter == GX_SAMPLE_LINEAR)
		return GL_LINEAR;
	else
	{
		fassert(false);
		return GL_NEAREST;
	}
}

void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp)
{
	if (s_gxTextureEnabled)
	{
		glActiveTexture(GL_TEXTURE0);
		checkErrorGL();
		
		const GLenum openglFilter = toOpenGLSampleFilter(filter);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, openglFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, openglFilter);
		checkErrorGL();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		checkErrorGL();
	}
}

void gxGetTextureSize(GxTextureId texture, int & width, int & height)
{
	// todo : use glGetTextureLevelParameteriv. upgrade GLEW ?

/*
	if (glGetTextureLevelParameteriv != nullptr)
	{
		glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
		glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
		checkErrorGL();
	}
	else
*/
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
	#if ENABLE_DESKTOP_OPENGL
		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
		checkErrorGL();
	#else
		// todo : gles : implement gxGetTextureSize
		AssertMsg(false, "not implemented. fetch of GL_TEXTURE_WIDTH/_HEIGHT is not available in non-desktop OpenGL", 0);
	#endif
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
	}
}

GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId id)
{
	int internalFormat = 0;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

#if ENABLE_DESKTOP_OPENGL
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
	checkErrorGL();
#else
	// todo : gles : implement gxGetTextureFormat
	AssertMsg(false, "not implemented. fetch of GL_TEXTURE_INTERNAL_FORMAT is not available in non-desktop OpenGL", 0);
#endif

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	// translate OpenGL format to GX format

	if (internalFormat == GL_R8) return GX_R8_UNORM;
	if (internalFormat == GL_RG8) return GX_RG8_UNORM;
	if (internalFormat == GL_R16F) return GX_R16_FLOAT;
	if (internalFormat == GL_R32F) return GX_R32_FLOAT;
	if (internalFormat == GL_RGB8) return GX_RGB8_UNORM;
	if (internalFormat == GL_RGBA8) return GX_RGBA8_UNORM;

	return GX_UNKNOWN_FORMAT;
}

#else // USE_LEGACY_OPENGL

#include "shaders.h" // registerBuiltinShaders

// for gxSetVertexBuffer, gxDrawIndexedPrimitives
static GLuint s_gxVertexArrayObjectForCustomDraw = 0;

void gxInitialize()
{
	// create vertex array for custom draw
	fassert(s_gxVertexArrayObjectForCustomDraw == 0);
	glGenVertexArrays(1, &s_gxVertexArrayObjectForCustomDraw);
	checkErrorGL();
	
	registerBuiltinShaders();
}

void gxShutdown()
{
	if (s_gxVertexArrayObjectForCustomDraw != 0)
	{
		glDeleteVertexArrays(1, &s_gxVertexArrayObjectForCustomDraw);
		s_gxVertexArrayObjectForCustomDraw = 0;
	}
}

void gxGetMatrixf(GX_MATRIX mode, float * m)
{
	switch (mode)
	{
	case GX_PROJECTION:
		glGetFloatv(GL_PROJECTION_MATRIX, m);
		checkErrorGL();
		break;

	case GX_MODELVIEW:
		glGetFloatv(GL_MODELVIEW_MATRIX, m);
		checkErrorGL();
		break;

	default:
		fassert(false);
		break;
	}
}

void gxSetMatrixf(GX_MATRIX mode, const float * m)
{
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		switch (mode)
		{
		case GX_PROJECTION:
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(m);
			checkErrorGL();
			break;

		case GX_MODELVIEW:
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(m);
			checkErrorGL();
			break;

		default:
			fassert(false);
			break;
		}
	}
	gxMatrixMode(restoreMatrixMode);
}

GX_MATRIX gxGetMatrixMode()
{
	GLint mode = 0;
	
	glGetIntegerv(GL_MATRIX_MODE, &mode);
	checkErrorGL();
	
	return (GX_MATRIX)mode;
}

void gxBegin(GLenum type)
{
	glBegin(type);
	checkErrorGL();
}

void gxEnd()
{
	glEnd();
	checkErrorGL();
}

void gxSetTexture(GxTextureId texture)
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
	}
	else
	{
	#if USE_LEGACY_OPENGL
		glDisable(GL_TEXTURE_2D);
		checkErrorGL();
	#endif
	}
}

static GLenum toOpenGLSampleFilter(const GX_SAMPLE_FILTER filter)
{
	if (filter == GX_SAMPLE_NEAREST)
		return GL_NEAREST;
	else if (filter == GX_SAMPLE_LINEAR)
		return GL_LINEAR;
	else
	{
		fassert(false);
		return GL_NEAREST;
	}
}

void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp)
{
	if (glIsEnabled(GL_TEXTURE_2D))
	{
		const GLenum openglFilter = toOpenGLSampleFilter(filter);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, openglFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, openglFilter);
		checkErrorGL();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		checkErrorGL();
	}
}

void gxGetTextureSize(GxTextureId texture, int & width, int & height)
{
	// todo : use glGetTextureLevelParameteriv. upgrade GLEW ?

/*
	if (glGetTextureLevelParameteriv != nullptr)
	{
		glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
		glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
		checkErrorGL();
	}
	else
*/
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
	}
}

GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId id)
{
	int internalFormat = 0;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	// translate OpenGL format to GX format

	if (internalFormat == GL_R8) return GX_R8_UNORM;
	if (internalFormat == GL_RG8) return GX_RG8_UNORM;
	if (internalFormat == GL_R16F) return GX_R16_FLOAT;
	if (internalFormat == GL_R32F) return GX_R32_FLOAT;
	if (internalFormat == GL_RGB8) return GX_RGB8_UNORM;
	if (internalFormat == GL_RGBA8) return GX_RGBA8_UNORM;

	return GX_UNKNOWN_FORMAT;
}

#endif

#include "gx_mesh.h" // GxVertexInput

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	// make sure to disable old attributes, to avoid reading from stale memory

// todo : add a constant for the maximum number of vertex inputs

	for (int i = 0; i < 16; ++i)
	{
		glDisableVertexAttribArray(i);
		checkErrorGL();
	}
	
	for (int i = 0; i < numVsInputs; ++i)
	{
		//logDebug("i=%d, id=%d, num=%d, type=%d, norm=%d, stride=%d, offset=%p\n", i, vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)vsInputs[i].offset);
		
		const GLenum type =
			vsInputs[i].type == GX_ELEMENT_FLOAT32 ? GL_FLOAT :
			vsInputs[i].type == GX_ELEMENT_UINT8 ? GL_UNSIGNED_BYTE :
			GL_INVALID_ENUM;

		Assert(type != GL_INVALID_ENUM);
		if (type == GL_INVALID_ENUM)
			continue;
		
		const int stride = vsStride ? vsStride : vsInputs[i].stride;
		Assert(stride != 0);
		if (stride == 0)
			continue;
		
		glEnableVertexAttribArray(vsInputs[i].id);
		checkErrorGL();
		
		glVertexAttribPointer(vsInputs[i].id, vsInputs[i].numComponents, type, vsInputs[i].normalize, stride, (void*)(intptr_t)vsInputs[i].offset);
		checkErrorGL();
	}
}

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	// bind the specified vertex buffer and vertex buffer bindings
	
	glBindVertexArray(s_gxVertexArrayObjectForCustomDraw);
	checkErrorGL();
	
	glBindBuffer(GL_ARRAY_BUFFER, buffer->getOpenglVertexArray());
	checkErrorGL();
	
	bindVsInputs(vsInputs, numVsInputs, vsStride);
}

void gxDrawIndexedPrimitives(const GX_PRIMITIVE_TYPE type, const int numElements, const GxIndexBuffer * indexBuffer)
{
	Assert(type == GX_TRIANGLES); // todo : translate primitive type
	if (type != GX_TRIANGLES)
		return;
	
	Shader genericShader("engine/Generic");

	Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;

	setShader(shader);

	gxValidateMatrices();

	const GLenum indexType =
		indexBuffer->getFormat() == GX_INDEX_16
		? GL_UNSIGNED_SHORT
		: GL_UNSIGNED_INT;

	const int numIndices = indexBuffer->getNumIndices();

	const ShaderCacheElem & shaderElem = shader.getCacheElem();
	
	if (shader.isValid())
	{
		if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
		{
			shader.setImmediate(
				shaderElem.params[ShaderCacheElem::kSp_Params].index,
				s_gxTextureEnabled ? 1 : 0,
				globals.colorMode,
				globals.colorPost,
				globals.colorClamp);
		}

		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
				shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
		}
		
		//
		
		glBindVertexArray(s_gxVertexArrayObjectForCustomDraw);
		checkErrorGL();
		
		Assert(indexBuffer->getOpenglIndexArray() != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getOpenglIndexArray());
		checkErrorGL();
		
		glDrawElements(GL_TRIANGLES, numIndices, indexType, 0);
		checkErrorGL();
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}

	if (&shader == &genericShader)
	{
		clearShader(); // todo : remove. here since Shader dtor doesn't clear globals.shader yet when it's the current shader
	}

	globals.gxShaderIsDirty = false;
}

#endif
