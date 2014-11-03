#include <algorithm>
#include "Entity.h"
#include "Frustum.h" // fixme, h.
#include "Renderer.h"
#include "RenderList.h"

// FIXME, ought not be global.
Mx::Plane RenderList::m_cameraPlane;

RenderList::RenderList()
{
	m_shader = 0;
}

RenderList::~RenderList()
{
	FASSERT(m_entities.size() == 0);
}

void RenderList::Add(Entity* entity)
{
	FASSERT(entity);

	m_entities.push_back(entity);
}

void RenderList::Remove(Entity* entity)
{
	FASSERT(entity);

	m_entities.erase(Find(entity));
}

void RenderList::Render(int renderMask)
{
	{
		Mat4x4 mat = Renderer::I().MatV().Top().CalcInv();
		Vec3 position = mat.Mul4(Vec3(0.0f, 0.0f, 0.0f));
		Vec3 orientation = mat.Mul(Vec3(0.0f, 0.0f, 1.0f)).CalcNormalized();
		float distance = orientation * position;
		m_cameraPlane.Setup(orientation, distance);
	}

	//FIXME, Revert.
	//Sort();

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	if (m_shader)
	{
		m_shader->Apply(gfx);
	}

	Mat4x4 matP = Renderer::I().MatP().Top();
	Mat4x4 matV = Renderer::I().MatV().Top();
	Mat4x4 matW = Renderer::I().MatW().Top();
	Mat4x4 matWVP = matP * matV * matW;
	Mat4x4 matWV = matV * matW;

	// Calculate frustum.
	Frustum frustum;
	frustum.Setup(matV, 1.0f, 1000.0f); // fixme, set near/far or pass frustum.

	for (EntityCollItr i = m_entities.begin(); i != m_entities.end(); ++i)
	{
		Entity* e = *i;

		if (!(renderMask & RM_SOLID) && !(e->m_shader->m_blendEnabled))
			continue;
		if (!(renderMask & RM_BLENDED) && (e->m_shader->m_blendEnabled))
			continue;

		Mat4x4 transform = e->GetTransform();

		#if 0
		// todo: fix incompat w/ lighting.
		if (e->m_hasAABB)
			if (frustum.Classify(e->m_aabb, transform) == CLASS_OUT)
				continue;
		#endif

		Renderer::I().MatW().Push(transform);
		{
			if (!m_shader)
			{
				e->UpdateRender();
				e->m_shader->Apply(gfx);
			}
			else
			{
				m_shader->m_vs->p["wvp"] = matWVP * transform;
				m_shader->m_vs->p["wv"] = matWV * transform;
				m_shader->m_vs->p["w"] = Renderer::I().MatW().Top();

				m_shader->Apply(gfx);
			}
			
			e->Render();
		}
		Renderer::I().MatW().Pop();
	}
}

void RenderList::SetOverrideShader(ResShader* shader)
{
	m_shader = shader;
}

void RenderList::Sort()
{
	std::sort(m_entities.begin(), m_entities.end(), Compare);
}

bool RenderList::Compare(const Entity* e1, const Entity* e2)
{
	// Render blended objects last.
	if (!(e1->m_shader->m_blendEnabled) && (e2->m_shader->m_blendEnabled))
		return true;
	if ((e1->m_shader->m_blendEnabled) && !(e2->m_shader->m_blendEnabled))
		return false;

	// Group objects with same shader.
	/*fixme, revert if (e1->m_shader.get() < e2->m_shader.get())
		return true;
	if (e1->m_shader.get() > e2->m_shader.get())
		return false;*/

	// Sort by depth. Render nearer objects first to maximize z-fail.
	float z1 = m_cameraPlane * e1->GetPosition();
	float z2 = m_cameraPlane * e2->GetPosition();

	if (z1 < z2)
		return true;
	if (z1 > z2)
		return false;

	// Make sort deterministic.
	if (e1 < e2)
		return true;
	if (e1 > e2)
		return false;

	return false;
}

RenderList::EntityCollItr RenderList::Find(Entity* entity)
{
	return std::find(m_entities.begin(), m_entities.end(), entity);
}
