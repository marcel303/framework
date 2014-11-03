#include "Calc.h"
#include "EntityBrick.h"
#include "EntityParticles.h"
#include "Mesh.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "ShapeBuilder.h"

#include "ProcTexcoordMatrix2DAutoAlign.h"

EntityBrick::EntityBrick(Vec3 position, Vec3 size, int indestructible) : Entity()
{
	SetClassName("Brick");
	EnableCaps(CAP_STATIC_PHYSICS);

	AddParameter(Parameter(PARAM_FLOAT32, "sx",    REP_ONCE,     COMPRESS_NONE, &m_size[0]));
	AddParameter(Parameter(PARAM_FLOAT32, "sy",    REP_ONCE,     COMPRESS_NONE, &m_size[1]));
	AddParameter(Parameter(PARAM_FLOAT32, "sz",    REP_ONCE,     COMPRESS_NONE, &m_size[2]));
	AddParameter(Parameter(PARAM_INT8,    "fort",  REP_ONCHANGE, COMPRESS_NONE, &m_fort));
	AddParameter(Parameter(PARAM_INT8,    "hp",    REP_ONCHANGE, COMPRESS_NONE, &m_hp));
	// NOTE: Change life to onchange? Doesn't need continuous updates if indestructible.
	AddParameter(Parameter(PARAM_TWEENF,  "life",  REP_ONCHANGE, COMPRESS_U16,  &m_life));
	AddParameter(Parameter(PARAM_INT8,    "indes", REP_ONCE,     COMPRESS_NONE, &m_indestructible));

	m_size = size;
	m_position = position;
	m_indestructible = indestructible;

	m_fort = 0;
	m_hp = MAX_HP;
	m_life = 1.0f;
}

EntityBrick::~EntityBrick()
{
}

void EntityBrick::PostCreate()
{
	Entity::PostCreate();

	m_phyObject.Initialize(2, m_position, Vec3(0.0f, 0.0f, 0.0f), false, false, this);
	m_cdCube = CD::ShObject(new CD::Cube());
	m_phyObject.AddGeometry(m_cdCube);

	Initialize();

	//m_shader->InitDepthTest(true, CMP_LE);
	//m_shader->InitCull(CULL_CW);
	m_shader->InitAlphaTest(true, CMP_GE, 0.5f);
	m_shader->m_vs = RESMGR.GetVS("shaders/box_vs.cg");
	m_shader->m_ps = RESMGR.GetPS("shaders/box_ps.cg");

	int tex = rand() % 3;

	switch (tex)
	{
	case 0:
		m_tex = RESMGR.GetTex("textures/nveye-color.png");
		m_texFort = RESMGR.GetTex("textures/fort.png");
		m_nmap = RESMGR.GetTex("textures/nveye-normal.png");
		m_hmap = RESMGR.GetTex("textures/nveye-height.png");
		break;
	case 1:
		m_tex = RESMGR.GetTex("textures/fan-color.png");
		m_texFort = RESMGR.GetTex("textures/fort.png");
		m_nmap = RESMGR.GetTex("textures/fan-normal.png");
		m_hmap = RESMGR.GetTex("textures/fan-height.png");
		break;
	case 2:
		m_tex = RESMGR.GetTex("textures/face_diffuse.jpg");
		m_texFort = RESMGR.GetTex("textures/face_diffuse.jpg");
		m_nmap = RESMGR.GetTex("textures/face_normal.jpg");
		m_hmap = RESMGR.GetTex("textures/face_height.jpg");
		break;
	}

	AddAABBObject(&m_mesh);
}

void EntityBrick::Initialize()
{
	m_phyObject.Initialize(2, m_position, Vec3(0.0f, 0.0f, 0.0f), false, false, this);

	static_cast<CD::Cube*>(m_cdCube.get())->Setup(- m_size / 2.0f, m_size / 2.0f);
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
		m_life -= dt / MAX_LIFE;
		if (m_life < 0.0f)
			m_life = 0.0f;

		Vec3 position = m_phyObject.m_position;

		position[1] = cos(m_life * Calc::mPI / 2.0f) * m_size[1] - m_size[1] * 0.5f;

		m_phyObject.SetPosition(position);
	}

	//static_cast<CD::Cube*>(m_cdCube.get())->Setup(- m_size / 2.0f, m_size / 2.0f);
}

void EntityBrick::UpdateRender()
{
	//Mat4x4 matW = GetTransform();

	m_shader->m_ps->p["hp"] = m_hp / float(MAX_HP);

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
	//if (m_fort)
	//	m_shader->m_ps->p["tex"] = m_texFort.get();
	//else
		m_shader->m_ps->p["tex"] = m_tex.get();
	m_shader->m_ps->p["nmap"] = m_nmap.get();
	m_shader->m_ps->p["hmap"] = m_hmap.get();

	Vec3 color;
	if (IsFortified())
		//color = Vec3(0.0f, 0.0f, 0.1f);
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
	m_position = m_phyObject.m_position;
	Initialize();
	static_cast<CD::Cube*>(m_cdCube.get())->Setup(- m_size / 2.0f, m_size / 2.0f);

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
		//texMat.MakeScaling(0.1f, 0.1f, 0.1f);
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
	return m_fort != 0;
}

void EntityBrick::Damage(int amount)
{
	if (IsIndestructible() || IsFortified())
		return;

	m_hp -= amount;

	if (m_hp <= 0)
	{
		m_hp = 0;
		GetScene()->RemoveEntityQueued(Self());

		// FIXME: Shld rlly send replicated message instead of abusing replication.
		EntityParticles* particles = new EntityParticles(m_phyObject.m_position, 20, 20.0f);
		particles->PostCreate(); // fixme, use factory.
		m_scene->AddEntity(ShEntity(particles));
	}

	m_parameters["hp"]->Invalidate();
}

void EntityBrick::Fort(int amount)
{
	if (IsIndestructible())
		return;

	m_fort += amount;

	m_parameters["fort"]->Invalidate();
}
