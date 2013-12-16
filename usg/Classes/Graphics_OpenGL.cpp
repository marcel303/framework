#include "Exception.h"
#include "Graphics_OpenGL.h"
#include "OpenGLCompat.h"
#include "OpenGLUtil.h"

static void ResetBlendMode();
static void MatrixActivate(MatrixType type);

#if defined(WIN32)
static PFNGLBLENDEQUATIONPROC _glBlendEquation = 0;
#endif

#if SIMULATE_STACK
static uint32_t MatrixModeToIdx(MatrixType type)
{
	if (type == MatrixType_Projection)
		return 0;
	else if (type == MatrixType_World)
		return 1;

	LOG_ERR("unknown matrix mode: %d", type);
	
	return 0;
}
static uint32_t MatrixModeToName(MatrixType type)
{
	if (type == MatrixType_Projection)
		return GL_PROJECTION_MATRIX;
	else if (type == MatrixType_World)
		return GL_MODELVIEW_MATRIX;
	
	LOG_ERR("unknown matrix mode: %d", type);
	
	return GL_PROJECTION_MATRIX;
}
#endif

Graphics_OpenGL::Graphics_OpenGL()
{
#if SIMULATE_STACK
	m_stackSize[0] = m_stackSize[1] = 0;
#endif
}

Graphics_OpenGL::~Graphics_OpenGL()
{
}

void Graphics_OpenGL::MakeCurrent()
{
}

void Graphics_OpenGL::Clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Graphics_OpenGL::BlendModeSet(BlendMode mode)
{
#ifdef WIN32
	if (_glBlendEquation == 0)
	{
		_glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");
	
		if (_glBlendEquation == 0)
			throw ExceptionVA("unable to find required OpenGL extension(s)");
	}
#endif

	ResetBlendMode();
	
	switch (mode)
	{
	case BlendMode_Additive:
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_CHECKERROR();
		break;
		
	case BlendMode_Normal_Opaque:
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_CHECKERROR();
		break;

	case BlendMode_Normal_Opaque_Add:
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		GL_CHECKERROR();
		break;
		
	case BlendMode_Normal_Transparent:
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		GL_CHECKERROR();
		break;
		
	case BlendMode_Normal_Transparent_Add:
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		GL_CHECKERROR();
		break;
		
	case BlendMode_Subtractive:
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#if defined(IPHONEOS) || defined(BBOS)
		glBlendEquationOES(GL_FUNC_REVERSE_SUBTRACT_OES);
#elif defined(LINUX) || defined(MACOS)
		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
#elif defined(WIN32)
		_glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
#else
		#error
#endif
		GL_CHECKERROR();
		break;
		
	default:
		throw ExceptionNA();
	}
}

void Graphics_OpenGL::MatrixSet(MatrixType type, const Mat4x4& mat)
{
	MatrixActivate(type);
	glLoadIdentity();
	glMultMatrixf(mat.m_v);
}

void Graphics_OpenGL::MatrixPush(MatrixType type)
{
	MatrixActivate(type);
#if SIMULATE_STACK
	uint32_t idx = MatrixModeToIdx(type);
	glGetFloatv(MatrixModeToName(type), m_stack[idx][m_stackSize[idx]].m_values);
	m_stackSize[idx]++;
#else
	glPushMatrix();
#endif
	GL_CHECKERROR();
}

void Graphics_OpenGL::MatrixPop(MatrixType type)
{
	MatrixActivate(type);
#if SIMULATE_STACK
	uint32_t idx = MatrixModeToIdx(type);
	Assert(m_stackSize[idx] >= 1);
	m_stackSize[idx]--;
	glLoadMatrixf(m_stack[idx][m_stackSize[idx]].m_values);
#else
	glPopMatrix();
#endif
	GL_CHECKERROR();
}

void Graphics_OpenGL::MatrixTranslate(MatrixType type, float x, float y, float z)
{
	MatrixActivate(type);
	glTranslatef(x, y, z);
}

void Graphics_OpenGL::MatrixRotate(MatrixType type, float angle, float x, float y, float z)
{
	angle = Calc::RadToDeg(angle);
	
	MatrixActivate(type);
	glRotatef(angle, x, y, z);
}

void Graphics_OpenGL::MatrixScale(MatrixType type, float x, float y, float z)
{
	MatrixActivate(type);
	glScalef(x, y, z);
}

void Graphics_OpenGL::TextureSet(Res* texture)
{
	OpenGLUtil::SetTexture(texture);
}

void Graphics_OpenGL::TextureIsEnabledSet(bool value)
{
	if (value)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
}

void Graphics_OpenGL::TextureCreate(Res* res)
{
	OpenGLUtil::CreateTexture(res);
}

void Graphics_OpenGL::TextureDestroy(Res* res)
{
	OpenGLUtil::DestroyTexture(res);
}

void Graphics_OpenGL::Present()
{
}

static void ResetBlendMode()
{
#ifdef IPHONEOS
	glBlendEquationOES(GL_FUNC_ADD_OES);
	GL_CHECKERROR();
#elif defined(WIN32)
	_glBlendEquation(GL_FUNC_ADD);
	GL_CHECKERROR();
#elif defined(LINUX)
	glBlendEquation(GL_FUNC_ADD);
	GL_CHECKERROR();
#elif defined(MACOS)
	glBlendEquation(GL_FUNC_ADD);
	GL_CHECKERROR();
#elif defined(BBOS)
	glBlendEquationOES(GL_FUNC_ADD_OES);
	GL_CHECKERROR();
#else
	#error
#endif
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void MatrixActivate(MatrixType type)
{
	if (type == MatrixType_Projection)
		glMatrixMode(GL_PROJECTION);
	else if (type == MatrixType_World)
		glMatrixMode(GL_MODELVIEW);
	else
		throw ExceptionVA("unknown matrix type: %d", (int)type);
}
