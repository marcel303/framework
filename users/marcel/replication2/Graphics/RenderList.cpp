#include <algorithm>
#include "Entity.h"
#include "Frustum.h" // fixme, h.
#include "Renderer.h"
#include "RenderList.h"

RenderList::RenderList()
{
	m_shader = 0;
}

RenderList::~RenderList()
{
	Assert(m_entities.size() == 0);
}

void RenderList::Add(Entity* entity)
{
	Assert(entity);

	m_entities.push_back(entity);
}

void RenderList::Remove(Entity* entity)
{
	Assert(entity);

	m_entities.erase(Find(entity));
}

void RenderList::Render(int renderMask)
{
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

RenderList::EntityCollItr RenderList::Find(Entity* entity)
{
	return std::find(m_entities.begin(), m_entities.end(), entity);
}
