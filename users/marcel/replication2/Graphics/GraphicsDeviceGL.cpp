#include "DisplaySDL.h"
#include "GraphicsDeviceGL.h"

#define TRUE_RT 1

static GLenum ConvPrimitiveType(PRIMITIVE_TYPE pt);
static GLenum ConvFunc(int func);
static GLenum ConvBlendOp(int op);
static GLenum ConvCubeFace(int face);

GraphicsDeviceGL::GraphicsDeviceGL() : GraphicsDevice()
{
	INITINIT;

	m_display = 0;
	m_cgContext = 0;
	m_fboId = 0;

	m_matW.MakeIdentity();
	m_matV.MakeIdentity();
	m_matP.MakeIdentity();

	m_rt  = 0;
	memset(m_rts, 0, sizeof(m_rts));
	m_rtd = 0;
	m_ib  = 0;
	m_vb  = 0;
	m_vs  = 0;
	m_ps  = 0;

	for (int i = 0; i < MAX_TEX; ++i)
		m_tex[i] = 0;

	m_stAlphaFunc = CMP_GE;
	m_stAlphaRef = 0.5f;
	m_stBlendSrc = GL_SRC_ALPHA;
	m_stBlendDst = GL_DST_ALPHA;
}

GraphicsDeviceGL::~GraphicsDeviceGL()
{
	INITCHECK(false);

	/*while (m_cache.size() > 0)
		UnLoad(m_cache.begin()->first);*/
}

void GraphicsDeviceGL::Initialize(const GraphicsOptions& options)
{
	INITCHECK(false);

	// Initialize SDL display.
	m_display = new DisplaySDL(0, 0, options.Sx, options.Sy, options.Fullscreen, true);

	const GLubyte* temp = glGetString(GL_EXTENSIONS);
	std::string ext;
	do
	{
		if (*temp == ' ' || *temp == 0)
		{
			m_extensions[ext] = 1;
			ext.clear();
		}
		else
			ext.push_back(*temp);
		temp++;
	} while (*temp);

	//for (std::map<std::string, int>::iterator i = m_extensions.begin(); i != m_extensions.end(); ++i)
	//	printf("Extension: %s.\n", i->first.c_str());

	#define INIT_EXT(fn, type, name) {  fn = (type)wglGetProcAddress(name); if (!fn) printf("Not found: %s.\n", name); }

	// Mutli texture.
	INIT_EXT(glActiveTexture,           PFNGLACTIVETEXTUREPROC,              "glActiveTextureARB"          );
	INIT_EXT(glClientActiveTexture,     PFNGLCLIENTACTIVETEXTUREPROC,        "glClientActiveTextureARB"    );
	// VBO.
	INIT_EXT(glGenBuffers,              PFNGLGENBUFFERSPROC,                 "glGenBuffersARB"             );
	INIT_EXT(glDeleteBuffers,           PFNGLDELETEBUFFERSPROC,              "glDeleteBuffersARB"          );
	INIT_EXT(glBindBuffer,              PFNGLBINDBUFFERPROC,                 "glBindBufferARB"             );
	INIT_EXT(glBufferData,              PFNGLBUFFERDATAPROC,                 "glBufferDataARB"             );
	INIT_EXT(glBufferSubData,           PFNGLBUFFERSUBDATAPROC,              "glBufferSubDataARB"          );
	// Frame buffer.
	INIT_EXT(glGenFramebuffers,         PFNGLGENFRAMEBUFFERSEXTPROC,         "glGenFramebuffersEXT"        );
	INIT_EXT(glDeleteFramebuffers,      PFNGLDELETEFRAMEBUFFERSEXTPROC,      "glDeleteFramebuffersEXT"     );
	INIT_EXT(glBindFramebuffer,         PFNGLBINDFRAMEBUFFEREXTPROC,         "glBindFramebufferEXT"        );
	INIT_EXT(glCheckFramebufferStatus,  PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC,  "glCheckFramebufferStatusEXT" );
	INIT_EXT(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, "glFramebufferRenderbufferEXT");
	INIT_EXT(glFramebufferTexture2D,    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC,    "glFramebufferTexture2DEXT"   );
	INIT_EXT(glBlitFramebuffer,         PFNGLBLITFRAMEBUFFEREXTPROC,         "glBlitFramebufferEXT"        );
	INIT_EXT(glDrawBuffers,             PFNGLDRAWBUFFERSPROC,                "glDrawBuffers"               );
	//
	INIT_EXT(glGenerateMipmap,          PFNGLGENERATEMIPMAPEXTPROC,          "glGenerateMipmapEXT"         );
	// Render buffer.
	INIT_EXT(glGenRenderbuffers,        PFNGLGENRENDERBUFFERSEXTPROC,        "glGenRenderbuffersEXT"       );
	INIT_EXT(glDeleteRenderbuffers,     PFNGLDELETERENDERBUFFERSEXTPROC,     "glDeleteRenderbuffersEXT"    );
	INIT_EXT(glRenderbufferStorage,     PFNGLRENDERBUFFERSTORAGEEXTPROC,     "glRenderbufferStorageEXT"    );
	INIT_EXT(glBindRenderbuffer,        PFNGLBINDRENDERBUFFEREXTPROC,        "glBindRenderbufferEXT"       );

	CheckError();

	m_cgContext = cgCreateContext();
	CheckError();

	glGenFramebuffers(1, &m_fboId);
	CheckError();

	#if !defined(FDEBUG) && 0
	cgGLSetDebugMode(CG_FALSE);
	CheckError();
	#endif

	ResetStates();

	INITSET(true);
}

void GraphicsDeviceGL::Shutdown()
{
	INITCHECK(true);

#if 0
	// Reset stuff.
	SetRT(0);
	SetIB(0);
	SetVB(0);
	for (int i = 0; i < MAX_TEX; ++i)
		SetTex(i, 0);
	SetVS(0);
	SetPS(0);
#endif

	FASSERT(m_rt == 0);
	for (int i = 0; i < 4; ++i)
		FASSERT(m_rts[i] == 0);
	FASSERT(m_rtd == 0);
	FASSERT(m_ib == 0);
	FASSERT(m_vb == 0);
	for (int i = 0; i < MAX_TEX; ++i)
		FASSERT(m_tex[i] == 0);
	FASSERT(m_vs == 0);
	FASSERT(m_ps == 0);

	FASSERT(m_cache.size() == 0);

	glDeleteFramebuffers(1, &m_fboId);
	m_fboId = 0;

	cgDestroyContext(m_cgContext);
	m_cgContext = 0;
	CheckError();

	SAFE_FREE(m_display);

	INITSET(false);
}

Display* GraphicsDeviceGL::GetDisplay()
{
	return m_display;
}

void GraphicsDeviceGL::SceneBegin()
{
	// TODO, move to engine..
	m_display->Update();
}

void GraphicsDeviceGL::SceneEnd()
{
}

void GraphicsDeviceGL::Clear(int buffers, float r, float g, float b, float a, float z)
{
	GLbitfield glBuffers = 0;

	if (buffers & BUFFER_COLOR)
	{
		glClearColor(r, g, b, a);
		glBuffers |= GL_COLOR_BUFFER_BIT;
	}
	if (buffers & BUFFER_DEPTH)
	{
		glClearDepth(z);
		glBuffers |= GL_DEPTH_BUFFER_BIT;
	}

	glClear(glBuffers);
	CheckError();
}

void GraphicsDeviceGL::Draw(PRIMITIVE_TYPE type)
{
	GLenum glType = ConvPrimitiveType(type);

	if (m_ib == 0)
	{
		glDrawArrays(glType, 0, m_vb->m_vCnt);
	}
	else
	{
		if (glGenBuffers)
			glDrawElements(glType, m_ib->GetIndexCnt(), GL_UNSIGNED_SHORT, 0);
		else
			glDrawElements(glType, m_ib->GetIndexCnt(), GL_UNSIGNED_SHORT, m_ib->index);
	}

	if (0)
	{
		int mode = 1;
		//if (m_vb->m_fvf & FVF_XYZ && m_vb->m_fvf & FVF_NORMAL)
		if (mode == 1 && (m_vb->m_fvf & FVF_XYZ) && (m_vb->m_fvf & FVF_TEX1))
		if (mode == 2 && (m_vb->m_fvf & FVF_XYZ) && (m_vb->m_fvf & FVF_TEX4))
		{
			glBegin(GL_LINES);
			{
				for (uint32_t i = 0; i < m_vb->m_vCnt; ++i)
				{
					Vec3 p1 = m_vb->position[i];
					//Vec3 n1 = m_vb->tex[0][i].XYZ();
					//Vec3& n = m_vb->normal[i];
					//glNormal3fv(&n[0]);
					Vec3 n1 = m_vb->tex[1][i].XYZ();
					Vec3 n2 = m_vb->tex[2][i].XYZ();
					Vec3 n3 = m_vb->tex[3][i].XYZ();
					Vec3 v1 = p1 + n1;
					Vec3 v2 = p1 + n2;
					Vec3 v3 = p1 + n3;
					glColor3f(1.0f, 0.0f, 0.0f);
					glVertex3fv(&p1[0]);
					glVertex3fv(&v1[0]);
					if (mode == 2)
					{
						glColor3f(0.0f, 1.0f, 0.0f);
						glVertex3fv(&p1[0]);
						glVertex3fv(&v2[0]);
						glColor3f(0.0f, 0.0f, 1.0f);
						glVertex3fv(&p1[0]);
						glVertex3fv(&v3[0]);
					}
				}
				glColor3f(1.0f, 1.0f, 1.0f);
			}
			glEnd();
		}
	}

	CheckError();
}

void GraphicsDeviceGL::Present()
{
	SDL_GL_SwapBuffers();
	CheckError();
}

void GraphicsDeviceGL::Resolve(ResTexR* rt)
{
	SetRTM(rt, 0, 0, 0, 1, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_fboId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);
	glBlitFramebuffer(
		0, 0, m_display->GetWidth(), m_display->GetHeight(),
		0, 0, m_display->GetWidth(), m_display->GetHeight(),
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fboId);
}

void GraphicsDeviceGL::Copy(ResBaseTex* out_tex)
{
	FASSERT(out_tex);

	switch (out_tex->m_type)
	{
	case RES_TEXCF:
		{
			ResTexCF* texCF = (ResTexCF*)out_tex;
			ResTexCR* texCR = texCF->m_cube;

			Validate(texCR);

			DataTexCR* dataCR = (DataTexCR*)m_cache[texCR];

			glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, dataCR->m_texID);
			CheckError();

			int face = ConvCubeFace(texCF->m_face);

			glCopyTexImage2D(face, 0, GL_RGBA8, 0, 0, texCR->GetW(), texCR->GetH(), 0);
			CheckError();
		}
		break;
	default:
		FASSERT(0);
	}
}

void GraphicsDeviceGL::SetScissorRect(int x1, int y1, int x2, int y2)
{
	// FIXME, TODO.
}

void GraphicsDeviceGL::RS(int state, int value)
{
	switch (state)
	{
	case RS_DEPTHTEST:
		if (value)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		break;
	case RS_DEPTHTEST_FUNC:
		glDepthFunc(ConvFunc(value));
		break;
	case RS_CULL:
		if (value == CULL_NONE)
			glDisable(GL_CULL_FACE);
		else
		{
			// TODO: Check cull modes!
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW);

			int cullFace;

			if (value == CULL_CW)
				cullFace = GL_BACK;
			else if (value == CULL_CCW)
				cullFace = GL_FRONT;
			else
				FASSERT(0);

			glCullFace(cullFace);
		}
		break;
	case RS_ALPHATEST:
		if (value)
			glEnable(GL_ALPHA_TEST);
		else
			glDisable(GL_ALPHA_TEST);
		break;
	case RS_ALPHATEST_FUNC:
		m_stAlphaFunc = ConvFunc(value);
		glAlphaFunc(m_stAlphaFunc, m_stAlphaRef);
		break;
	case RS_ALPHATEST_REF:
		m_stAlphaRef = *(float*)&value;
		glAlphaFunc(m_stAlphaFunc, m_stAlphaRef);
		break;
	case RS_STENCILTEST:
		if (value)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
		break;
		// TODO: Implement stencil test.
	case RS_STENCILTEST_FUNC:
		break;
	case RS_STENCILTEST_MASK:
		break;
	case RS_STENCILTEST_REF:
		break;
	case RS_STENCILTEST_ONPASS:
		break;
	case RS_STENCILTEST_ONFAIL:
		break;
	case RS_STENCILTEST_ONZFAIL:
		break;
	case RS_BLEND:
		if (value)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
		break;
	case RS_BLEND_SRC:
		m_stBlendSrc = ConvBlendOp(value);
		glBlendFunc(m_stBlendSrc, m_stBlendDst);
		break;
	case RS_BLEND_DST:
		m_stBlendDst = ConvBlendOp(value);
		glBlendFunc(m_stBlendSrc, m_stBlendDst);
		break;
	case RS_SCISSORTEST:
		// FIXME TODO.
		break;
	case RS_WRITE_DEPTH:
		glDepthMask(value);
		break;
	case RS_WRITE_COLOR:
		glColorMask(value, value, value, value);
		break;
	default:
		FASSERT(0);
		break;
	}

	CheckError();
}

void GraphicsDeviceGL::SS(int sampler, int state, int value)
{
	switch (state)
	{
	case SS_FILTER:
		// TODO: Implement.
		break;
	default:
		FASSERT(0);
		break;
	}

	CheckError();
}

Mat4x4 GraphicsDeviceGL::GetMatrix(MATRIX_NAME matID)
{
	Mat4x4 result;

	switch (matID)
	{
	case MAT_WRLD:
		result = m_matW;
		break;
	case MAT_VIEW:
		result = m_matV;
		break;
	case MAT_PROJ:
		result = m_matP;
		break;
	}

	return result;
}

void GraphicsDeviceGL::SetMatrix(MATRIX_NAME matID, const Mat4x4& mat)
{
	switch (matID)
	{
	case MAT_WRLD:
		m_matW = mat;
		m_matWV = m_matV * m_matW;
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&m_matWV);
		break;
	case MAT_VIEW:
		m_matV = mat;
		m_matWV = m_matV * m_matW;
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&m_matWV);
		break;
	case MAT_PROJ:
		m_matP = mat;
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((GLfloat*)&m_matP);
		break;
	}

	CheckError();
}

int GraphicsDeviceGL::GetRTW()
{
	if (m_rt)
		return m_rt->GetW();
	else
		//return m_display->GetWidth();
		return SDL_GetVideoInfo()->current_w;
}

int GraphicsDeviceGL::GetRTH()
{
	if (m_rt)
		return m_rt->GetH();
	else
		//return m_display->GetHeight();
		return SDL_GetVideoInfo()->current_h;
}

void GraphicsDeviceGL::SetRT(ResTexR* rt)
{
	if (rt == m_rt)
	{
		return;
	}

	if (rt == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		CheckError();

		glReadBuffer(GL_BACK);
		glDrawBuffer(GL_BACK);
		CheckError();

		//int sx = m_display->GetWidth();
		//int sy = m_display->GetHeight();
		int sx = SDL_GetVideoInfo()->current_w;
		int sy = SDL_GetVideoInfo()->current_h;

		glViewport(0, 0, sx, sy);
		CheckError();
	}
	else
	{
		Validate(rt);

		if (rt->m_type == RES_TEXR || rt->m_type == RES_TEXD || rt->m_type == RES_TEXRECTR)
		{
			DataTexR* data = (DataTexR*)m_cache[rt];

			glBindFramebuffer(GL_FRAMEBUFFER_EXT, data->m_fboID);
			CheckError();

			glViewport(0, 0, rt->GetW(), rt->GetH());
			CheckError();
		}
		if (rt->m_type == RES_TEXCF) // FIXME: RES_TEXCRF
		{
			ResTexCF* texCF = (ResTexCF*)rt;
			ResTexCR* texCR = texCF->m_cube;

			Validate(texCR);

			//DataTexCF* dataCF = (DataTexCF*)m_cache[rt];
			DataTexCR* dataCR = (DataTexCR*)m_cache[texCR];

			glBindFramebuffer(GL_FRAMEBUFFER_EXT, dataCR->m_fboID);
			CheckError();

			GLenum face = ConvCubeFace(texCF->m_face);

			if (texCR->m_target == TEXR_COLOR || texCR->m_target == TEXR_COLOR32F)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, face, dataCR->m_texID, 0);
				CheckError();

				glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dataCR->m_rbDepthID);
				CheckError();
			}

			if (texCR->m_target == TEXR_DEPTH)
			{
				glReadBuffer(GL_NONE);
				glDrawBuffer(GL_NONE);
				CheckError();

				glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, face, dataCR->m_texID, 0);
				CheckError();

				//glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dataCR->m_rbDepthID);
				//CheckError();
			}

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	
			FASSERT(status == GL_FRAMEBUFFER_COMPLETE_EXT);

			glViewport(0, 0, texCR->GetW(), texCR->GetH());
			CheckError();
		}
	}

	m_rt = rt;
}

void GraphicsDeviceGL::SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd)
{
	FASSERT(numRenderTargets <= 4);

	ResTexR* rts[4] = { rt1, rt2, rt3, rt4 };
	GLenum mrt[4] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT };

	for (int i = numRenderTargets; i < 4; ++i)
	{
		rts[i] = 0;
	}

	for (int i = 0; i < 4; ++i)
		if (!rts[i])
			mrt[i] = GL_NONE;

	bool allEqual = true;
	bool allEmpty = true;

	for (int i = 0; i < 4; ++i)
	{
		allEqual &= rts[i] == m_rts[i];
		allEmpty &= rts[i] == 0;
	}

	allEqual &= rtd == m_rtd;

	if (allEqual)
	{
		return;
	}

	int sx = -1;
	int sy = -1;

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fboId);
	CheckError();

	for (int i = 0; i < 4; ++i)
	{
		if (rts[i])
		{
			if (sx < 0)
			{
				sx = rts[i]->GetW();
				sy = rts[i]->GetH();
			}
			else
			{
				FASSERT(sx == rts[i]->GetW());
				FASSERT(sy == rts[i]->GetH());
			}
		}

		if (rts[i] == m_rts[i])
			continue;

		m_rts[i] = rts[i];

		ResTexR* rt = rts[i];

		if (rt)
		{
			Validate(rt);

			if (rt->m_type == RES_TEXCF) // FIXME: RES_TEXCRF
			{
				FASSERT(false); // Untested

				ResTexCF* texCF = (ResTexCF*)rt;
				ResTexCR* texCR = texCF->m_cube;

				Validate(texCR);

				//DataTexCF* dataCF = (DataTexCF*)m_cache[rt];
				DataTexCR* dataCR = (DataTexCR*)m_cache[texCR];

				GLenum face = ConvCubeFace(texCF->m_face);

				glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, face, dataCR->m_texID, 0);
				CheckError();
			}
			else if (rt->m_type == RES_TEXR || rt->m_type == RES_TEXRECTR)
			{
				DataTexR* data = (DataTexR*)m_cache[rt];

				glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, data->m_texID, 0);
				CheckError();
			}
			else
			{
				FASSERT(false);
			}
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, 0, 0);
			CheckError();
		}
	}

	if (rtd)
	{
		if (sx < 0)
		{
			sx = rtd->GetW();
			sy = rtd->GetH();
		}
		else
		{
			FASSERT(sx == rtd->GetW());
			FASSERT(sy == rtd->GetH());
		}
	}

	if (rtd != m_rtd)
	{
		m_rtd = rtd;

		if (rtd)
		{
			Validate(rtd);

			DataTexR* data = (DataTexR*)m_cache[rtd];

			glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, data->m_texID, 0);
			CheckError();
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
			CheckError();
		}
	}

	if (allEmpty)
	{
		glReadBuffer(GL_NONE);
		CheckError();
		glDrawBuffer(GL_NONE);
		CheckError();
	}
	else
	{
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		CheckError();
		glDrawBuffers(numRenderTargets, mrt);
		CheckError();
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	if (numRenderTargets == 0 && rtd == 0)
		FASSERT(status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT);
	else
		FASSERT(status == GL_FRAMEBUFFER_COMPLETE_EXT);

	if (sx < 0)
	{
		sx = 0;
		sy = 0;
	}

	glViewport(0, 0, sx, sy);
	CheckError();
}

void GraphicsDeviceGL::SetIB(ResIB* ib)
{
	if (ib == 0)
	{
		if (glGenBuffers)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			CheckError();
		}
	}
	else
	{
		Validate(ib);

		DataIB* data = (DataIB*)m_cache[ib];

		if (glGenBuffers)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->m_vboID);
			CheckError();
		}
	}

	m_ib = ib;
}

void GraphicsDeviceGL::SetVB(ResVB* vb)
{
	// Disable client states.
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	for (int i = 0; i < MAX_TEX; ++i)
	{
		glClientActiveTexture(GL_TEXTURE0 + i);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	CheckError();

	if (vb == 0)
	{
		if (glGenBuffers)
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CheckError();
		}
	}
	else
	{
		Validate(vb);

		DataVB* data = (DataVB*)m_cache[vb];

		if (glGenBuffers)
		{
			glBindBuffer(GL_ARRAY_BUFFER, data->m_vboID);
			CheckError();

			// Set offsets into VBO.
			int stream = 0;

			if (vb->m_fvf & FVF_XYZ)
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)data->m_streamOffsets[stream]);
				stream++;
				CheckError();
			}

			if (vb->m_fvf & FVF_NORMAL)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, (GLvoid*)data->m_streamOffsets[stream]);
				stream++;
				CheckError();
			}

			if (vb->m_fvf & FVF_COLOR)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				//glColorPointer(4, GL_UNSIGNED_BYTE, 0, (GLvoid*)data->m_streamOffsets[stream]);
				glColorPointer(4, GL_FLOAT, 0, (GLvoid*)data->m_streamOffsets[stream]);
				stream++;
				CheckError();
			}

			for (uint32_t i = 0; i < vb->m_texCnt; ++i)
			{
				glClientActiveTexture(GL_TEXTURE0 + i);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(4, GL_FLOAT, 0, (GLvoid*)data->m_streamOffsets[stream]);
				stream++;
				CheckError();
			}
		}
		else
		{
			if (vb->m_fvf & FVF_XYZ)
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, vb->position);
				CheckError();
			}

			if (vb->m_fvf & FVF_NORMAL)
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, vb->normal);
				CheckError();
			}

			if (vb->m_fvf & FVF_COLOR)
			{
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_FLOAT, 0, vb->color);
				CheckError();
			}

			for (uint32_t i = 0; i < vb->m_texCnt; ++i)
			{
				glClientActiveTexture(GL_TEXTURE0 + i);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(4, GL_FLOAT, 0, vb->tex[i]);
				CheckError();
			}
		}
	}

	m_vb = vb;
}

void GraphicsDeviceGL::SetTex(int sampler, ResBaseTex* tex)
{
#if 0
	for (int i = 0; i < MAX_TEX; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + sampler);
		glClientActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
		CheckError();
		glDisable(GL_TEXTURE_2D);
		CheckError();
	}
#endif

	glClientActiveTexture(GL_TEXTURE0 + sampler);
	glActiveTexture(GL_TEXTURE0 + sampler);

	if (tex == 0)
	{
		if (m_tex[sampler])
		{
			switch (m_tex[sampler]->m_type)
			{
			case RES_TEX:
				glBindTexture(GL_TEXTURE_2D, 0);
				CheckError();
				glDisable(GL_TEXTURE_2D);
				CheckError();
				break;
			case RES_TEXD:
				glBindTexture(GL_TEXTURE_2D, 0);
				CheckError();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				CheckError();
				glDisable(GL_TEXTURE_2D);
				CheckError();
				break;
			case RES_TEXR:
				glBindTexture(GL_TEXTURE_2D, 0);
				CheckError();
				glDisable(GL_TEXTURE_2D);
				CheckError();
				break;
			case RES_TEXRECTR:
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				CheckError();
				glDisable(GL_TEXTURE_RECTANGLE_ARB);
				CheckError();
				break;
			case RES_TEXCR:
				glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, 0);
				CheckError();
				glDisable(GL_TEXTURE_CUBE_MAP_EXT); 
				CheckError();
				break;
			default:
				FASSERT(0);
				break;
			}
		}
	}
	else
	{
		Validate(tex);

		DataTex* data = (DataTex*)m_cache[tex];

		switch (tex->m_type)
		{
		case RES_TEX:
			glBindTexture(GL_TEXTURE_2D, data->m_texID);
			CheckError();

			if (glGenerateMipmap)
			{
#if 1
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
				CheckError();
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				CheckError();
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			CheckError();

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2);
			//CheckError();

			glEnable(GL_TEXTURE_2D);
			CheckError();
			break;
		case RES_TEXD:
			glBindTexture(GL_TEXTURE_2D, data->m_texID);
			CheckError();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			CheckError();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			CheckError();
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 0.5f);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			CheckError();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			CheckError();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			CheckError();

			glEnable(GL_TEXTURE_2D);
			CheckError();
			break;
		case RES_TEXR:
			{
				ResTexR* texR = (ResTexR*)tex;

				glBindTexture(GL_TEXTURE_2D, data->m_texID);
				CheckError();

#if 0
				if (texR->m_target == TEXR_COLOR)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					CheckError();
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					CheckError();
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					CheckError();
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					CheckError();
				}
#else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();
#endif

				glEnable(GL_TEXTURE_2D);
				CheckError();
			}
			break;
		case RES_TEXRECTR:
			{
				ResTexR* texR = (ResTexR*)tex;

				//glBindTexture(GL_TEXTURE_RECTANGLE_ARB, data->m_texID);
				glBindTexture(GL_TEXTURE_2D, data->m_texID);
				CheckError();

#if 1
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();
#else
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();
#endif

				//glEnable(GL_TEXTURE_RECTANGLE_ARB);
				glEnable(GL_TEXTURE_2D);
				CheckError();
			}
			break;
		case RES_TEXCR:
			{
				glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, data->m_texID);
				CheckError();

				glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();

				glEnable(GL_TEXTURE_CUBE_MAP_EXT); 
				CheckError();
			}
			break;
		default:
			FASSERT(0);
			break;
		}
	}

	m_tex[sampler] = tex;
}

void GraphicsDeviceGL::SetVS(ResVS* vs)
{
	if (vs == 0)
	{
		if (m_vs)
		{
			DataVS* data = (DataVS*)m_cache[m_vs];

			cgGLDisableProfile(data->m_profile);
			CheckError();
		}
	}
	else
	{
		int changed = Validate(vs);

		DataVS* data = (DataVS*)m_cache[vs];

		if (changed || vs != m_vs)
		{
			cgGLBindProgram(data->m_program);
			CheckError();

			cgGLEnableProfile(data->m_profile);
			CheckError();
		}

		// Apply parameters.
		for (ShaderParamList::ParamCollItr i = vs->p.m_parameters.begin(); i != vs->p.m_parameters.end(); ++i)
		{
			const std::string& name = i->first;
			const ShaderParam& param = i->second;

			const CGparameter p = data->GetParameter(name);
			CheckError();

			if (p)
			{
				switch (param.m_type)
				{
				case SHPARAM_FLOAT:  cgGLSetParameter1fv(p, param.m_v);      break;
				case SHPARAM_VEC3:   cgGLSetParameter3fv(p, param.m_v);      break;
				case SHPARAM_VEC4:   cgGLSetParameter4fv(p, param.m_v);      break;
				case SHPARAM_MAT4X4: cgGLSetMatrixParameterfr(p, param.m_v); break;
				case SHPARAM_TEX:    FASSERT(0);                             break; // Not supporter ATM..
				}
			}

			CheckError();
		}
	}

	m_vs = vs;
}

void GraphicsDeviceGL::SetPS(ResPS* ps)
{
	if (ps == 0)
	{
		if (m_ps)
		{
			DataPS* data = (DataPS*)m_cache[m_ps];

			for (int i = 0; i < MAX_TEX; ++i)
				SetTex(i, 0);

			cgGLDisableProfile(data->m_profile);
			CheckError();
		}
	}
	else
	{
		int changed = Validate(ps);

		DataPS* data = (DataPS*)m_cache[ps];

		if (changed || ps != m_ps)
		{
			cgGLEnableProfile(data->m_profile);
			CheckError();

			cgGLBindProgram(data->m_program);
			CheckError();
		}

		// Apply parameters.
		for (ShaderParamList::ParamCollItr i = ps->p.m_parameters.begin(); i != ps->p.m_parameters.end(); ++i)
		{
			const std::string& name = i->first;
			const ShaderParam& param = i->second;

			const CGparameter p = data->GetParameter(name);
			CheckError();

			if (p == 0)
			{
				//printf("Warning: Cg parameter not found: %s.\n", name.c_str());
				continue;
			}

			switch (param.m_type)
			{
			case SHPARAM_FLOAT:  cgGLSetParameter1fv(p, param.m_v);      break;
			case SHPARAM_VEC3:   cgGLSetParameter3fv(p, param.m_v);      break;
			case SHPARAM_VEC4:   cgGLSetParameter4fv(p, param.m_v);      break;
			case SHPARAM_MAT4X4: cgGLSetMatrixParameterfr(p, param.m_v); break;
			case SHPARAM_TEX:
				{
					//uint32_t r = cgGetParameterResourceIndex(p);
					uint32_t r = cgGLGetTextureEnum(p) - GL_TEXTURE0;
					CheckError();

					if (r >= 0 && r < 8)
					{
						//FASSERT(r == CG_TEXUNIT0);
						SetTex(r, (ResTex*)param.m_p);
					}
				}
				break;
			}
			CheckError();
		}
	}

	m_ps = ps;
}

void GraphicsDeviceGL::OnResInvalidate(Res* res)
{
	FASSERT(res);

	UnLoad(res);
}

void GraphicsDeviceGL::OnResDestroy(Res* res)
{
	FASSERT(res);

	if (res == m_rt)
		SetRT(0);
	if (res == m_ib)
		SetIB(0);
	if (res == m_vb)
		SetVB(0);
	for (int i = 0; i < MAX_TEX; ++i)
		if (res == m_tex[i])
			SetTex(i, 0);
	if (res == m_vs)
		SetVS(0);
	if (res == m_ps)
		SetPS(0);

	UnLoad(res);
}

int GraphicsDeviceGL::Validate(Res* res)
{
	FASSERT(res);

	if (m_cache.count(res) == 0)
	{
		UpLoad(res);
		return 1;
	}

	return 0;
}

void GraphicsDeviceGL::UpLoad(Res* res)
{
	INITCHECK(true);

	FASSERT(res);
	FASSERT(m_cache.count(res) == 0);

	void* data = 0;

	switch (res->m_type)
	{
	case RES_IB:
		data = UpLoadIB((ResIB*)res);
		break;
	case RES_VB:
		data = UpLoadVB((ResVB*)res);
		break;
	case RES_TEX:
		data = UpLoadTex((ResTex*)res);
		break;
	case RES_TEXD:
		data = UpLoadTexR((ResTexD*)res, false, true, false);
		break;
	case RES_TEXR:
		if (((ResTexR*)res)->m_target == TEXR_COLOR)
			data = UpLoadTexR((ResTexR*)res, true, false, false);
		if (((ResTexR*)res)->m_target == TEXR_COLOR32F)
			data = UpLoadTexR((ResTexR*)res, true, false, false);
		if (((ResTexR*)res)->m_target == TEXR_DEPTH)
			data = UpLoadTexR((ResTexR*)res, false, true, false);
		break;
	case RES_TEXRECTR:
		if (((ResTexR*)res)->m_target == TEXR_COLOR)
			data = UpLoadTexR((ResTexR*)res, true, false, true);
		if (((ResTexR*)res)->m_target == TEXR_COLOR32F)
			data = UpLoadTexR((ResTexR*)res, true, false, true);
		if (((ResTexR*)res)->m_target == TEXR_DEPTH)
			data = UpLoadTexR((ResTexR*)res, false, true, true);
		break;
	case RES_TEXCR:
		data = UpLoadTexCR((ResTexCR*)res);
		break;
	case RES_TEXCF:
		data = UpLoadTexCF((ResTexCF*)res); // FIXME, TEXCRF.
		break;
	case RES_VS:
		data = UpLoadVS((ResVS*)res);
		break;
	case RES_PS:
		data = UpLoadPS((ResPS*)res);
		break;
	default:
		FASSERT(0);
		break;
	}

	m_cache[res] = data;

	res->AddUser(this);

	//printf("UpLoad: Cache size: %d.\n", (int)m_cache.size());
}

void GraphicsDeviceGL::UnLoad(Res* res)
{
	INITCHECK(true);

	FASSERT(res);
	FASSERT(m_cache.count(res) != 0);

	switch (res->m_type)
	{
	case RES_IB:
		UnLoadIB((ResIB*)res);
		break;
	case RES_VB:
		UnLoadVB((ResVB*)res);
		break;
	case RES_TEX:
		UnLoadTex((ResTex*)res);
		break;
	case RES_TEXD:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXR:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXRECTR:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXCR:
		UnLoadTexCR((ResTexCR*)res);
		break;
	case RES_TEXCF:
		UnLoadTexCR((ResTexCR*)res);
		break;
	case RES_VS:
		UnLoadVS((ResVS*)res);
		break;
	case RES_PS:
		UnLoadPS((ResPS*)res);
		break;
	default:
		FASSERT(0);
		break;
	}

	res->RemoveUser(this);

	m_cache.erase(res);

	//printf("UnLoad: Cache size: %d.\n", (int)m_cache.size());
}

void* GraphicsDeviceGL::UpLoadIB(ResIB* ib)
{
	DataIB* data = new DataIB();

	if (glGenBuffers)
	{
		glGenBuffers(1, &data->m_vboID);
		CheckError();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->m_vboID);
		CheckError();

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib->GetIndexCnt() * sizeof(uint16_t), ib->index, GL_STATIC_DRAW);
		CheckError();
	}

	return data;
}

void* GraphicsDeviceGL::UpLoadVB(ResVB* vb)
{
	DataVB* data = new DataVB();

	if (glGenBuffers)
	{
		glGenBuffers(1, &data->m_vboID);
		CheckError();

		glBindBuffer(GL_ARRAY_BUFFER, data->m_vboID);
		CheckError();

		int size = 0;

		if (vb->m_fvf & FVF_XYZ)      size += 3 * sizeof(float);
		if (vb->m_fvf & FVF_NORMAL)   size += 3 * sizeof(float);
		//if (vb->m_fvf & FVF_COLOR)    size += 4 * sizeof(uint8);
		if (vb->m_fvf & FVF_COLOR)    size += 4 * sizeof(float);

		size += 4 * sizeof(float) * vb->m_texCnt;

		size *= vb->m_vCnt;

		glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);
		CheckError();

		int offset = 0;

		#define ADD_STREAM(offset, size, ptr) { glBufferSubData(GL_ARRAY_BUFFER, offset, size, ptr); CheckError(); data->m_streamOffsets[data->m_streamCnt] = offset; data->m_streamCnt++; offset += size; }

		if (vb->m_fvf & FVF_XYZ)
			ADD_STREAM(offset, 3 * sizeof(float) * vb->m_vCnt, vb->position)
		if (vb->m_fvf & FVF_NORMAL)
			ADD_STREAM(offset, 3 * sizeof(float) * vb->m_vCnt, vb->normal)
		if (vb->m_fvf & FVF_COLOR)
			ADD_STREAM(offset, 4 * sizeof(float) * vb->m_vCnt, vb->color)
		for (uint32_t i = 0; i < vb->m_texCnt; ++i)
			ADD_STREAM(offset, 4 * sizeof(float) * vb->m_vCnt, vb->tex[i])
	}

	return data;
}

void* GraphicsDeviceGL::UpLoadTex(ResTex* tex)
{
	DataTex* data = new DataTex();

	glGenTextures(1, &data->m_texID);
	CheckError();

	glBindTexture(GL_TEXTURE_2D, data->m_texID);
	CheckError();

	if (glGenerateMipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		CheckError();
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->GetData());
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->GetData());
	CheckError();

	if (glGenerateMipmap)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
		CheckError();
	}

	return data;
}

void* GraphicsDeviceGL::UpLoadTexR(ResTexR* tex, bool texColor, bool texDepth, bool rect)
{
	if (TRUE_RT)
	{
		FASSERT(!(texColor && texDepth));

		DataTexR* data = new DataTexR();

		if (texColor)
		{
			GLenum texture = GL_TEXTURE_2D;

			glGenTextures(1, &data->m_texID);
			CheckError();

			glBindTexture(texture, data->m_texID);
			CheckError();

			GLenum format = GL_RGBA8;

			if (tex->m_target == TEXR_COLOR) format = GL_RGBA8;
			if (tex->m_target == TEXR_COLOR32F) format = GL_RGBA32F_ARB;

			glTexImage2D(texture, 0, format, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			CheckError();

			if (texture == GL_TEXTURE_2D)
			{
				if (tex->m_target == TEXR_COLOR)
				{
					glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameterf(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameterf(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
					CheckError();
				}
				else
				{
					glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					CheckError();
					glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					CheckError();
				}
			}
			else
			{
				glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();
			}
		}

		if (texDepth)
		{
			GLenum texture = GL_TEXTURE_2D;

			glGenTextures(1, &data->m_texID);
			CheckError();

			glBindTexture(texture, data->m_texID);
			CheckError();

			glTexImage2D(texture, 0, GL_DEPTH_COMPONENT, tex->GetW(), tex->GetH(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			CheckError();

			glTexParameterf(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			CheckError();
		}

		return data;
	}
	else
	{
		DataTexR* data = new DataTexR();

		// Create frame buffer.
		glGenFramebuffers(1, &data->m_fboID);
		CheckError();

		glBindFramebuffer(GL_FRAMEBUFFER_EXT, data->m_fboID);
		CheckError();

		GLenum texture = GL_TEXTURE_2D;

		if (rect)
			texture = GL_TEXTURE_RECTANGLE_ARB;

		if (texColor)
		{
			// Create color buffer.
			glGenTextures(1, &data->m_texID);
			CheckError();

			glBindTexture(texture, data->m_texID);
			CheckError();

			GLenum format = GL_RGBA8;

			if (tex->m_target == TEXR_COLOR) format = GL_RGBA8;
			if (tex->m_target == TEXR_COLOR32F) format = GL_RGBA32F_ARB;

			glTexImage2D(texture, 0, format, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			CheckError();

			if (texture == GL_TEXTURE_2D)
			{
				if (tex->m_target == TEXR_COLOR)
				{
					glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameterf(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameterf(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
					CheckError();
				}
				else
				{
					glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					CheckError();
					glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					CheckError();
				}
			}
			else
			{
				glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				CheckError();
				glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				CheckError();
			}

			// Attach color buffer to frame buffer.
			glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, texture, data->m_texID, 0);
			CheckError();
		}
		else
		{
	#if 1
			// Create color buffer.
			glGenRenderbuffers(1, &data->m_rbColorID);
			CheckError();

			glBindRenderbuffer(GL_RENDERBUFFER_EXT, data->m_rbColorID);
			CheckError();

			glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_RGBA8, tex->GetW(), tex->GetH());
			CheckError();

			// Attach color buffer to frame buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, data->m_rbColorID);
			CheckError();
	#endif
		}

		if (texDepth)
		{
			// Create depth buffer.
			glGenTextures(1, &data->m_texID);
			CheckError();

			glBindTexture(texture, data->m_texID);
			CheckError();

			glTexImage2D(texture, 0, GL_DEPTH_COMPONENT, tex->GetW(), tex->GetH(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			CheckError();

			glTexParameterf(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			CheckError();

			// Attach depth buffer to frame buffer.
			glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, texture, data->m_texID, 0);
			CheckError();
		}
		else
		{
	#if 1
			// Create depth buffer.
			glGenRenderbuffers(1, &data->m_rbDepthID);
			CheckError();

			glBindRenderbuffer(GL_RENDERBUFFER_EXT, data->m_rbDepthID);
			CheckError();

			glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, tex->GetW(), tex->GetH());
			CheckError();

			// Attach depth buffer to frame buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, data->m_rbDepthID);
			CheckError();
	#endif
		}

		/*
		// Create stencil buffer.
		glGenRenderbuffers(1, &data->m_stencilID);
		CheckError();

		glBindRenderbuffer(GL_RENDERBUFFER_EXT, data->m_stencilID);
		CheckError();

		glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, tex->GetW(), tex->GetH());
		CheckError();

		// Attach depth buffer to frame buffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, data->m_stencilID);
		CheckError();
		*/

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
		
		FASSERT(status == GL_FRAMEBUFFER_COMPLETE_EXT);

		glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		CheckError();

		return data;
	}
}

// TODO: Create one FBO only, managed by device.

void* GraphicsDeviceGL::UpLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = new DataTexCR();

	// Create frame buffer.
	glGenFramebuffers(1, &data->m_fboID);
	CheckError();

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, data->m_fboID);
	CheckError();

	glGenTextures(1, &data->m_texID);
	CheckError();

	glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, data->m_texID);
	CheckError();

	int internalFormat = 0;
	int format = 0;
	int type = 0;

	switch (tex->m_target)
	{
	case TEXR_COLOR:
		internalFormat = GL_RGBA16F_ARB;
		//internalFormat = GL_RGB16F_ARB;
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	case TEXR_DEPTH:
		internalFormat = GL_DEPTH_COMPONENT24;
		format = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
		break;
	}

	for (int i = 0; i < 6; ++i)
	{
		Validate(tex->m_faces[i].get());

		GLenum face = ConvCubeFace(tex->m_faces[i]->m_face);

		// TODO: Make internal texture format settable in texture resource.
		// TODO: Single channel float format = ??

		//glTexImage2D(face, 0, GL_RGBA16F_ARB, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//glTexImage2D(face, 0, GL_RGBA32F_ARB, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//glTexImage2D(face, 0, GL_RGBA8, tex->GetW(), tex->GetH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//CheckError();
		
		glTexImage2D(face, 0, internalFormat, tex->GetW(), tex->GetH(), 0, format, type, 0);
		CheckError();
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	CheckError();

	if (tex->m_target == TEXR_COLOR)
	{
		// Create depth buffer.
		glGenRenderbuffers(1, &data->m_rbDepthID);
		CheckError();

		glBindRenderbuffer(GL_RENDERBUFFER_EXT, data->m_rbDepthID);
		CheckError();

		glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, tex->GetW(), tex->GetH());
		CheckError();
	}

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	CheckError();

	return data;
}

void* GraphicsDeviceGL::UpLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = new DataTexCF();

	//glGenTextures(1, &data->m_texID);
	
	return data;
}

void* GraphicsDeviceGL::UpLoadVS(ResVS* vs)
{
	DataVS* data = new DataVS();

	data->m_profile = cgGLGetLatestProfile(CG_GL_VERTEX);

	cgGLSetOptimalOptions(data->m_profile);

	data->m_program = cgCreateProgram(m_cgContext, CG_SOURCE, vs->m_text.c_str(), data->m_profile, 0, 0);
	CheckError();

	cgCompileProgram(data->m_program);
	CheckError();

	cgGLLoadProgram(data->m_program);
	CheckError();

	return data;
}

void* GraphicsDeviceGL::UpLoadPS(ResPS* ps)
{
	DataPS* data = new DataPS();

	data->m_profile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

	cgGLSetOptimalOptions(data->m_profile);

	data->m_program = cgCreateProgram(m_cgContext, CG_SOURCE, ps->m_text.c_str(), data->m_profile, 0, 0);
	CheckError();

	cgCompileProgram(data->m_program);
	CheckError();

	cgGLLoadProgram(data->m_program);
	CheckError();

	return data;
}

void GraphicsDeviceGL::UnLoadIB(ResIB* ib)
{
	DataIB* data = (DataIB*)m_cache[ib];

	m_cache.erase(ib);

	if (glGenBuffers)
	{
		glDeleteBuffers(1, &data->m_vboID);
		CheckError();
	}

	delete data;
}

void GraphicsDeviceGL::UnLoadVB(ResVB* vb)
{
	DataVB* data = (DataVB*)m_cache[vb];

	m_cache.erase(vb);

	if (glGenBuffers)
	{
		glDeleteBuffers(1, &data->m_vboID);
		CheckError();
	}

	delete data;
}

void GraphicsDeviceGL::UnLoadTex(ResTex* tex)
{
	DataTex* data = (DataTex*)m_cache[tex];

	m_cache.erase(tex);

	glDeleteTextures(1, &data->m_texID);
	CheckError();

	delete data;
}

void GraphicsDeviceGL::UnLoadTexR(ResTexR* tex)
{
	DataTexR* data = (DataTexR*)m_cache[tex];

	m_cache.erase(tex);

	glDeleteTextures(1, &data->m_texID);
	CheckError();

	glDeleteFramebuffers(1, &data->m_fboID);
	CheckError();

	delete data;
}

void GraphicsDeviceGL::UnLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = (DataTexCR*)m_cache[tex];

	m_cache.erase(tex);

	glDeleteTextures(1, &data->m_texID);
	CheckError();

	delete data;
}

void GraphicsDeviceGL::UnLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = (DataTexCF*)m_cache[tex];

	delete data;
}

void GraphicsDeviceGL::UnLoadVS(ResVS* vs)
{
	DataVS* data = (DataVS*)m_cache[vs];

	m_cache.erase(vs);

	// TODO

	delete data;
}

void GraphicsDeviceGL::UnLoadPS(ResPS* ps)
{
	DataPS* data = (DataPS*)m_cache[ps];

	m_cache.erase(ps);

	// TODO

	delete data;
}

#ifdef FDEBUG
void GraphicsDeviceGL::CheckError()
{
	int error = glGetError();
	if (error)
	{
		printf("OpenGL error 0x%08x.\n", error);
		//FASSERT(0); // fixme
	}

	CGerror cgError = cgGetError();
	if (cgError != CG_NO_ERROR && cgError != CG_INVALID_PARAMETER_ERROR)
	{
		printf("Cg error 0x%08x.\n", cgError);
		const char* errorString = cgGetErrorString(cgError);
		if (errorString)
			printf("Cg error string: %s.\n", errorString);
		const char* listing = cgGetLastListing(m_cgContext);
		if (listing)
			printf("Cg listing: %s.\n", listing);
		//FASSERT(0); // fixme
	}
}
#endif

static GLenum ConvPrimitiveType(PRIMITIVE_TYPE pt)
{
	switch (pt)
	{
	case PT_TRIANGLE_LIST: return GL_TRIANGLES;
	case PT_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
	case PT_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
	case PT_LINE_LIST: return GL_LINES;
	case PT_LINE_STRIP: return GL_LINE_STRIP;
	default: FASSERT(0); return 0;
	}
}

static GLenum ConvFunc(int func)
{
	switch (func)
	{
	case CMP_EQ: return GL_EQUAL;
	case CMP_L:  return GL_LESS;
	case CMP_LE: return GL_LEQUAL;
	case CMP_G:  return GL_GREATER;
	case CMP_GE: return GL_GEQUAL;
	default: FASSERT(0); return 0;
	}
}

static GLenum ConvBlendOp(int op)
{
	switch (op)
	{
	case BLEND_ONE:           return GL_ONE;
	case BLEND_ZERO:          return GL_ZERO;
	case BLEND_SRC:           return GL_SRC_ALPHA;
	case BLEND_DST:           return GL_DST_ALPHA;
	case BLEND_SRC_COLOR:     return GL_SRC_COLOR;
	case BLEND_DST_COLOR:     return GL_DST_COLOR;
	case BLEND_INV_SRC:       return GL_ONE_MINUS_SRC_ALPHA;
	case BLEND_INV_DST:       return GL_ONE_MINUS_DST_ALPHA;
	case BLEND_INV_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
	case BLEND_INV_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
	default: FASSERT(0); return 0;
	}
}

static GLenum ConvCubeFace(int face)
{
	switch (face)
	{
	case CUBE_X_POS: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	case CUBE_Y_POS: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
	case CUBE_Z_POS: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
	case CUBE_X_NEG: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
	case CUBE_Y_NEG: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
	case CUBE_Z_NEG: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
	default: FASSERT(0); return 0;
	}
}

CGparameter GraphicsDeviceGL::DataVS::GetParameter(const std::string& name)
{
	std::map<std::string, CGparameter>::iterator i = m_parameters.find(name);

	if (i != m_parameters.end())
		return i->second;
	else
	{
		CGparameter p = cgGetNamedParameter(m_program, name.c_str());
		m_parameters[name] = p;
		return p;
	}
}

CGparameter GraphicsDeviceGL::DataPS::GetParameter(const std::string& name)
{
	std::map<std::string, CGparameter>::iterator i = m_parameters.find(name);

	if (i != m_parameters.end())
		return i->second;
	else
	{
		CGparameter p = cgGetNamedParameter(m_program, name.c_str());
		m_parameters[name] = p;
		return p;
	}
}