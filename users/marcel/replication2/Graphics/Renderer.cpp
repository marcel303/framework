#include "Renderer.h"

const Mat4x4 Renderer::m_cubeSSMMatrix[6] =
{
	// CUBE_X_POS:
	Mat4x4(
		 0,  0, -1,  0,
		 0, -1,  0,  0,
		+1,  0,  0,  0,
		 0,  0,  0, +1),
	// CUBE_Y_POS:
	Mat4x4(
		+1, 0,  0,   0,
		 0,  0, +1,  0,
		 0,  +1, 0,  0,
		 0,  0,  0, +1),
	// CUBE_Z_POS:
	Mat4x4(
		+1,  0,  0,  0,
		 0, -1,  0,  0,
		 0,  0, +1,  0,
		 0,  0,  0, +1),
	// CUBE_X_NEG:
	Mat4x4(
		 0,  0, +1,  0,
		 0, -1,  0,  0,
		-1,  0,  0,  0,
		 0,  0,  0, +1),
	// CUBE_Y_NEG:
	Mat4x4(
		+1,  0,  0,  0,
		 0,  0, -1,  0,
		 0, -1,  0,  0,
		 0,  0,  0, +1),
	// CUBE_Z_NEG:
	Mat4x4(
		-1,  0,  0,  0,
		 0, -1,  0,  0,
		 0,  0, -1,  0,
		 0,  0,  0, +1)
};

Renderer& Renderer::I()
{
	static Renderer renderer;
	return renderer;
}

void Renderer::Initialize()
{
	m_quad = new Mesh();

	m_quad->Initialize(&g_alloc, PT_TRIANGLE_LIST, false, 4, FVF_XYZ | FVF_TEX1, 6);
	ResVB& vb = *m_quad->GetVB();
	ResIB& ib = *m_quad->GetIB();
	vb.position[0] = Vec3(-1.0f, -1.0f, 0.0f);
	vb.position[1] = Vec3(+1.0f, -1.0f, 0.0f);
	vb.position[2] = Vec3(+1.0f, +1.0f, 0.0f);
	vb.position[3] = Vec3(-1.0f, +1.0f, 0.0f);
	vb.tex[0][0] = Vec2(0.0f, 0.0f);
	vb.tex[0][1] = Vec2(1.0f, 0.0f);
	vb.tex[0][2] = Vec2(1.0f, 1.0f);
	vb.tex[0][3] = Vec2(0.0f, 1.0f);
	ib.index[0] = 0;
	ib.index[1] = 1;
	ib.index[2] = 2;
	ib.index[3] = 0;
	ib.index[4] = 2;
	ib.index[5] = 3;

	INITSET(true);
}

void Renderer::Shutdown()
{
	SAFE_FREE(m_quad);

	INITSET(false);
}

void Renderer::RenderMesh(Mesh& mesh)
{
	INITCHECK(true);

	if (mesh.IsIndexed())
		m_gfx->SetIB(mesh.GetIB());
	else
		m_gfx->SetIB(0);
	m_gfx->SetVB(mesh.GetVB());
	m_gfx->Draw(mesh.GetPT());
}

Renderer::Renderer()
{
	INITINIT;

	m_gfx = 0;

	m_matW.SetHandler(this);
	m_matV.SetHandler(this);
	m_matP.SetHandler(this);
}

Renderer::~Renderer()
{
	INITCHECK(false);
}

void Renderer::RenderQuad()
{
	INITCHECK(true);

	RenderMesh(*m_quad);
}

void Renderer::OnMatrixUpdate(MatrixStack* stack, const Mat4x4& mat)
{
	MATRIX_NAME temp = MAT_WRLD;

	if (stack == &m_matW) temp = MAT_WRLD;
	if (stack == &m_matV) temp = MAT_VIEW;
	if (stack == &m_matP) temp = MAT_PROJ;

	if (m_gfx)
		m_gfx->SetMatrix(temp, mat);
}
