#ifndef GRAPHICSDEVICEGL_H
#define GRAPHICSDEVICEGL_H
#pragma once

#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "Debug.h"
#include "Display.h"
#include "GraphicsDevice.h"

/*
#include "GraphicsTypes.h"
#include "Mat4x4.h"
#include "ResBaseTex.h"
#include "ResIB.h"
#include "ResPS.h"
#include "ResTex.h"
#include "ResTexCR.h"
#include "ResTexD.h"
#include "ResTexR.h"
#include "ResUser.h"
#include "ResVB.h"
#include "ResVS.h"
*/

#ifndef GL_DRAW_FRAMEBUFFER_EXT
	#define GL_READ_FRAMEBUFFER_EXT                0x8CA8
	#define GL_DRAW_FRAMEBUFFER_EXT                0x8CA9
	typedef void (APIENTRYP PFNGLBLITFRAMEBUFFEREXTPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
#endif

class GraphicsDeviceGL : public GraphicsDevice
{
public:
	// Multi texture.
	PFNGLACTIVETEXTUREPROC              glActiveTexture;
	PFNGLCLIENTACTIVETEXTUREPROC        glClientActiveTexture;
	// VBO.
	PFNGLGENBUFFERSPROC                 glGenBuffers;
	PFNGLDELETEBUFFERSPROC              glDeleteBuffers;
	PFNGLBINDBUFFERPROC                 glBindBuffer;
	PFNGLBUFFERDATAPROC                 glBufferData;
	PFNGLBUFFERSUBDATAPROC              glBufferSubData;
	PFNGLGENERATEMIPMAPEXTPROC          glGenerateMipmap;
	// Frame buffer.
	PFNGLGENFRAMEBUFFERSEXTPROC         glGenFramebuffers;
	PFNGLDELETEFRAMEBUFFERSEXTPROC      glDeleteFramebuffers;
	PFNGLBINDFRAMEBUFFEREXTPROC         glBindFramebuffer;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    glFramebufferTexture2D;
	PFNGLBLITFRAMEBUFFEREXTPROC         glBlitFramebuffer;
	PFNGLDRAWBUFFERSPROC                glDrawBuffers;
	// Render buffer.
	PFNGLGENRENDERBUFFERSEXTPROC        glGenRenderbuffers;
	PFNGLDELETERENDERBUFFERSEXTPROC     glDeleteRenderbuffers;
	PFNGLRENDERBUFFERSTORAGEEXTPROC     glRenderbufferStorage;
	PFNGLBINDRENDERBUFFEREXTPROC        glBindRenderbuffer;
	// Coordinate frame.
	PFNGLTANGENTPOINTEREXTPROC          glTangentPointer;
	PFNGLBINORMALPOINTEREXTPROC         glBinormalPointer;

	GraphicsDeviceGL();
	virtual ~GraphicsDeviceGL();

	virtual void Initialize(const GraphicsOptions& options);
	virtual void Shutdown();

	virtual Display* GetDisplay();

	virtual void SceneBegin();
	virtual void SceneEnd();

	virtual void Clear(int buffers, float r, float g, float b, float a, float z);
	virtual void Draw(PRIMITIVE_TYPE type);
	virtual void Present();
	virtual void Resolve(ResTexR* rt);
	virtual void Copy(ResBaseTex* out_tex);
	virtual void SetScissorRect(int x1, int y1, int x2, int y2);

	virtual void RS(int state, int value);
	virtual void SS(int sampler, int state, int value);
	virtual Mat4x4 GetMatrix(MATRIX_NAME matID);
	virtual void SetMatrix(MATRIX_NAME matID, const Mat4x4& mat);

	virtual int GetRTW();
	virtual int GetRTH();

	class DataIB
	{
	public:
		inline DataIB()
		{
			m_vboID = 0;
		}

		GLuint m_vboID;
	};

	class DataVB
	{
	public:
		inline DataVB()
		{
			m_vboID = 0;

			m_streamCnt = 0;
		}

		const static int MAX_STREAMS = 16;

		GLuint m_vboID;

		int m_streamOffsets[MAX_STREAMS];
		int m_streamCnt;
	};

	class DataTex
	{
	public:
		inline DataTex()
		{
			m_texID = 0;
		}

		GLuint m_texID;
	};

	class DataTexR : public DataTex
	{
	public:
		inline DataTexR() : DataTex()
		{
			m_fboID = 0;
			m_rbColorID = 0;
			m_rbDepthID = 0;
		}

		GLuint m_fboID;
		GLuint m_rbColorID;
		GLuint m_rbDepthID;
	};

	class DataTexCR : public DataTex
	{
	public:
		inline DataTexCR() : DataTex()
		{
			m_fboID = 0;
			m_rbDepthID = 0;
		}

		GLuint m_fboID;
		GLuint m_rbDepthID;
	};

	class DataTexCF
	{
	public:
	};

	class DataVS
	{
	public:
		CGparameter GetParameter(const std::string& name);

		CGprogram m_program;
		CGprofile m_profile;
		std::map<std::string, CGparameter> m_parameters;
	};

	class DataPS
	{
	public:
		CGparameter GetParameter(const std::string& name);

		CGprogram m_program;
		CGprofile m_profile;
		std::map<std::string, CGparameter> m_parameters;
	};

	virtual void SetRT(ResTexR* rt);
	virtual void SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd);
	virtual void SetIB(ResIB* ib);
	virtual void SetVB(ResVB* vb);
	virtual void SetTex(int sampler, ResBaseTex* tex);

	virtual void SetVS(ResVS* vs);
	virtual void SetPS(ResPS* ps);

	virtual void OnResInvalidate(Res* res);
	virtual void OnResDestroy(Res* res);

	int Validate(Res* res);

	void UpLoad(Res* res);
	void UnLoad(Res* res);

	void* UpLoadIB(ResIB* ib);
	void* UpLoadVB(ResVB* vb);
	void* UpLoadTex(ResTex* tex);
	void* UpLoadTexR(ResTexR* tex, bool texColor, bool texDepth, bool rect);
	void* UpLoadTexCR(ResTexCR* tex);
	void* UpLoadTexCF(ResTexCF* tex);
	void* UpLoadVS(ResVS* vs);
	void* UpLoadPS(ResPS* ps);

	void UnLoadIB(ResIB* ib);
	void UnLoadVB(ResVB* vb);
	void UnLoadTex(ResTex* tex);
	void UnLoadTexR(ResTexR* tex); // todo: gree all
	void UnLoadTexCR(ResTexCR* tex);
	void UnLoadTexCF(ResTexCF* tex);
	void UnLoadVS(ResVS* vs);
	void UnLoadPS(ResPS* ps);

#ifdef FDEBUG
	void CheckError();
#else
	inline void CheckError() { }
#endif

	const static int MAX_TEX = 8;

	INITSTATE;

	Display* m_display;
	CGcontext m_cgContext;
	GLuint m_fboId;

	Mat4x4 m_matW;
	Mat4x4 m_matV;
	Mat4x4 m_matP;
	Mat4x4 m_matWV; // Derived from m_matW and m_matV.

	ResTexR* m_rt;
	ResTexR* m_rts[4];
	ResTexD* m_rtd;
	ResIB* m_ib;
	ResVB* m_vb;
	ResBaseTex* m_tex[MAX_TEX];
	ResVS* m_vs;
	ResPS* m_ps;

	std::map<Res*, void*> m_cache;

	// States.
	int m_stAlphaFunc;
	float m_stAlphaRef;
	int m_stBlendSrc;
	int m_stBlendDst;

	std::map<std::string, int> m_extensions;
};

#endif
