#include "Calc.h"
#include "EntityBrick.h"
#include "Groups.h"
#include "Mesh.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "ShapeBuilder.h"

#include "ProcTexcoordMatrix2DAutoAlign.h"

DEFINE_ENTITY(EntityBrick, Brick);

EntityBrick::EntityBrick()
	: Entity()
{
	SetClassName("Brick");
	EnableCaps(CAP_STATIC_PHYSICS);

	m_brick_NS = new Brick_NS(this, 0, MAX_HP, 1.f);

	m_indestructible = false;

	m_phyObject.Initialize(GRP_BRICK, Vec3(), Vec3(), false, false, this);
	m_cdCube = CD::ShObject(new CD::Cube());
	m_phyObject.AddGeometry(m_cdCube);

	//m_shader->InitDepthTest(true, CMP_LE);
	//m_shader->InitCull(CULL_CW);
	//m_shader->InitAlphaTest(true, CMP_GE, 0.5f);
	m_shader->m_vs = RESMGR.GetVS("data/shaders/brick_vs.cg");
	m_shader->m_ps = RESMGR.GetPS("data/shaders/brick_ps.cg");

	m_tex = RESMGR.GetTex("data/textures/brick-color.png");
	m_nmap = RESMGR.GetTex("data/textures/brick-normal.png");
	m_hmap = RESMGR.GetTex("data/textures/brick-height.png");

	AddAABBObject(&m_mesh);
}

EntityBrick::~EntityBrick()
{
	delete m_brick_NS;
	m_brick_NS = 0;
}

void EntityBrick::Initialize(Vec3 position, Vec3 size, bool indestructible)
{
	m_phyObject.SetPosition(position);
	static_cast<CD::Cube*>(m_cdCube.get())->Setup(-size / 2.0f, size / 2.0f);

	m_size = size;
	m_indestructible = indestructible;
}

Mat4x4 EntityBrick::GetTransform() const
{
	Mat4x4 matW;

	matW.MakeIdentity();

	return Entity::GetTransform() * matW;
}

void EntityBrick::UpdateLogic(float dt)
{
	Entity::UpdateLogic(dt);
}

void EntityBrick::UpdateAnimation(float dt)
{
	if (!IsIndestructible())
	{
		m_brick_NS->ChangeLife(- dt / MAX_LIFE);

		Vec3 position = m_phyObject.m_position;

		position[1] = cos(m_brick_NS->GetLife() * Calc::mPI / 2.0f) * m_size[1] - m_size[1] * 0.5f;

		m_phyObject.SetPosition(position);
	}
}

void EntityBrick::UpdateRender()
{
	//Mat4x4 matW = GetTransform();

	m_shader->m_ps->p["hp"] = m_brick_NS->GetHP() / float(MAX_HP);

	m_shader->m_vs->p["wvp"] =
		Renderer::I().MatP().Top() *
		Renderer::I().MatV().Top() *
		Renderer::I().MatW().Top();
	m_shader->m_vs->p["w"] =
		Renderer::I().MatW().Top();
	m_shader->m_vs->p["w_inv"] =
		Renderer::I().MatW().Top().CalcInv();
	m_shader->m_vs->p["eye_pos"] = Renderer::I().MatV().Top().CalcInv().Mul(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	m_shader->m_vs->p["t"] = m_scene->GetTime();
	m_shader->m_ps->p["tex"] = m_tex.get();
	m_shader->m_ps->p["nmap"] = m_nmap.get();
	m_shader->m_ps->p["hmap"] = m_hmap.get();

	Vec3 color;
	if (IsFortified())
		color = Vec3(0.25f, 0.0f, 0.5f);
	else
		color = Vec3(0.0f, 0.0f, 0.0f);

	m_shader->m_ps->p["color"] = Vec4(color, 0.0f);
}

void EntityBrick::Render()
{
	Renderer::I().RenderMesh(m_mesh);
}

// FIXME: Require Initialize or smt to create collision shape.
void EntityBrick::OnSceneAdd(Scene* scene)
{
	Initialize(m_phyObject.m_position, m_size, m_indestructible);

	ShapeBuilder sb;
	sb.PushScaling(m_size / 2.0f);
	{
#if 1
		//int shape = rand() % 2;
		int shape = 0;

		if (shape == 0) sb.CreateCube(&g_alloc, m_mesh, FVF_XYZ | FVF_TEX4 | FVF_NORMAL);
		if (shape == 1) sb.CreateDonut(&g_alloc, 30, 20, 0.8f, 0.2f, m_mesh, FVF_XYZ | FVF_TEX4 | FVF_NORMAL);
#else
		std::vector<Mesh*> meshes;

		const float pillarSize = 0.1f;
		#define PILLAR(x, z) { Mesh* temp = new Mesh(); sb.PushTranslation(Vec3(x, 0.0f, z)); sb.PushScaling(Vec3(pillarSize, 1.0f, pillarSize)); sb.CreateCube(&g_alloc, *temp, FVF_XYZ | FVF_TEX4 | FVF_NORMAL); meshes.push_back(temp); sb.Pop(); sb.Pop(); }
		for (int i = 0; i <= 3; ++i)
			for (int j = 0; j <= 3; ++j)
				PILLAR((i / 3.0f - 0.5f) * 2.0f, (j / 3.0f - 0.5f) * 2.0f);

		sb.Merge(&g_alloc, meshes, m_mesh);

		for (size_t i = 0; i < meshes.size(); ++i)
			delete meshes[i];
#endif

		sb.CalculateNormals(m_mesh);

		ProcTexcoordMatrix2DAutoAlign procTex;
		Mat4x4 texMat;
		texMat.MakeScaling(1.0f / m_size[0], 1.0f / m_size[1], 1.0f / m_size[2]);
		Mat4x4 texMatPos;
		texMatPos.MakeTranslation(0.5f, 0.5f, 0.5f);
		texMat = texMatPos * texMat;
		procTex.SetMatrix(texMat);
		sb.CalculateTexcoords(m_mesh, 0, &procTex);

		sb.CalculateCoordinateFrameFromUV(m_mesh, 0, 1);
	}
	sb.Pop();

	Entity::OnSceneAdd(scene);
}

void EntityBrick::OnSceneRemove(Scene* scene)
{
	Entity::OnSceneRemove(scene);
}

void EntityBrick::ScriptDamageAntiBrick(int amount)
{
	Damage(amount);
}

bool EntityBrick::IsIndestructible()
{
	return m_indestructible != 0;
}

bool EntityBrick::IsFortified()
{
	return m_brick_NS->GetFort() != 0;
}

void EntityBrick::Damage(int amount)
{
	if (IsIndestructible() || IsFortified())
		return;

	m_brick_NS->ChangeHP(amount);

	if (m_brick_NS->GetHP() == 0)
		GetScene()->RemoveEntityQueued(Self());
}

void EntityBrick::Fort(int amount)
{
	if (IsIndestructible())
		return;

	m_brick_NS->ChangeFort(amount);
}
