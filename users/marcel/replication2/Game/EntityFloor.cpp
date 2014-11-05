#include "EntityFloor.h"
#include "Groups.h"
#include "Mesh.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "ShapeBuilder.h"

#include "ProcTexcoordMatrix2DAutoAlign.h"

EntityFloor::EntityFloor(Vec3 position, Vec3 size, int orientation) : Entity()
{
	SetClassName("Floor");
	EnableCaps(CAP_STATIC_PHYSICS);

	m_floor_NS = new Floor_NS(this);

	m_position = position;
	m_size = size;
	m_orientation = orientation;
}

EntityFloor::~EntityFloor()
{
	delete m_floor_NS;
	m_floor_NS = 0;
}

void EntityFloor::PostCreate()
{
	Initialize(m_position, m_size, m_orientation);
	m_cdCube = CD::ShObject(new CD::Cube());
	m_phyObject.AddGeometry(m_cdCube);

	//m_shader->InitDepthTest(true, CMP_LE);
	//m_shader->InitCull(true, CULL_CCW);
	m_shader->m_vs = RESMGR.GetVS("data/shaders/floor_vs.cg");
	m_shader->m_ps = RESMGR.GetPS("data/shaders/floor_ps.cg");

	m_tex = RESMGR.GetTex("data/textures/floor.png");
}

void EntityFloor::Initialize(Vec3 position, Vec3 size, int orientation)
{
	m_position = position;
	m_size = size;
	m_orientation = orientation;

	m_phyObject.Initialize(GRP_WORLD_STATIC, m_position, Vec3(0.0f, 0.0f, 0.0f), false, false, this);
}

Mat4x4 EntityFloor::GetTransform() const
{
	return Entity::GetTransform();
}

void EntityFloor::UpdateLogic(float dt)
{
}

void EntityFloor::UpdateAnimation(float dt)
{
}

void EntityFloor::UpdateRender()
{
	Vec4 ltPos = Vec4(sin(m_scene->m_time / 10.0f) * 100.0f, 20.0f, cos(m_scene->m_time / 10.0f) * 100.0f, 1.0f);

	m_shader->m_vs->p["wvp"] =
		Renderer::I().MatP().Top() *
		Renderer::I().MatV().Top() *
		Renderer::I().MatW().Top();
	m_shader->m_vs->p["w"] =
		Renderer::I().MatW().Top();
	m_shader->m_vs->p["lt_pos"] = ltPos;
	m_shader->m_vs->p["eye_pos"] = Renderer::I().MatV().Top().CalcInv().Mul(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	m_shader->m_ps->p["tex"] = m_tex.get();
	m_shader->m_ps->p["t"] = m_scene->GetTime();
}

void EntityFloor::Render()
{
	Renderer::I().RenderMesh(m_mesh);
}

// FIXME: Require Initialize or smt to create collision shape.
void EntityFloor::OnSceneAdd(Scene* scene)
{
	Initialize(m_phyObject.m_position, m_size, m_orientation);
	static_cast<CD::Cube*>(m_cdCube.get())->Setup(- m_size / 2.0f, m_size / 2.0f);

	ShapeBuilder sb;

	sb.PushScaling(m_size / 2.0f);
	{
		//sb.PushScaling(Vec3(1.0f, m_orientation, 1.0f));
		{
			Mesh temp;

			sb.CreateCube(&g_alloc, temp, FVF_XYZ | FVF_TEX1 | FVF_NORMAL);
			//sb.CreateGridCube(20, 20, temp, FVF_XYZ | FVF_TEX1 | FVF_NORMAL); // FIXME, not needed with per pixel lighting.
			
			sb.CalculateNormals(temp);

			ProcTexcoordMatrix2DAutoAlign procTex;
			Mat4x4 texMat;
			texMat.MakeScaling(0.1f, 0.1f, 0.1f);
			procTex.SetMatrix(texMat);
			sb.CalculateTexcoords(temp, 0, &procTex);

			sb.ConvertToIndexed(&g_alloc, temp, m_mesh);
		}
		//sb.Pop();
	}
	sb.Pop();

	Entity::OnSceneAdd(scene);
}

void EntityFloor::OnSceneRemove(Scene* scene)
{
	Entity::OnSceneRemove(scene);
}
