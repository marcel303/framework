#include "Atlas_ImageInfo.h"
#include "GameState.h"
#include "ParticleEffect.h"
#include "TextureAtlas.h"

typedef struct ParticleDefault
{
	Vec2F m_Direction;
	float m_Speed;
} ParticleDefault;

typedef struct ParticleRotMove
{
	Vec2F m_Direction;
	float m_AngleSpeed;
} ParticleRotMove;

//

Particle::Particle()
{
	m_UpdateCB = 0;
	m_RenderCB = 0;
	m_Life = 0.0f;
	m_Image = 0;
	m_VectorShape = 0;
	m_Color = SpriteColors::White;
}

void Particle::Setup(float x, float y, float life, float sx, float sy, float angle)
{
	m_Position[0] = x;
	m_Position[1] = y;
	m_Life = life;
	m_LifeBeginRcp = 1.0f / life;
	m_Sx = sx;
	m_Sy = sy;
	m_Angle = angle;
	m_Color = SpriteColors::White;
}

//

void Particle_Default_Setup(Particle* particle, float x, float y, float life, float sx, float sy, float angle, float speed)
{
	ParticleDefault* data = (ParticleDefault*)particle->Data_get();
	
	particle->m_UpdateCB = Particle_Default_Update;
	particle->Setup(x, y, life, sx, sy, angle);
	
	data->m_Speed = speed;
	Calc::SinCos_Fast(angle, data->m_Direction.m_V + 1, data->m_Direction.m_V + 0);
}

void Particle_Default_Update(Particle* particle, float dt)
{
	ParticleDefault* data = (ParticleDefault*)particle->Data_get();
	
	particle->m_Position += data->m_Direction * data->m_Speed * dt;
	
	const float v = particle->m_Life * particle->m_LifeBeginRcp;
		
	const int c = (int)(v * 255.0f);
	
	particle->m_Color.v[3] = c;// = SpriteColor_Make(c, c, c, c);
}

void Particle_RotMove_Setup(Particle* particle, float x, float y, float life, float sx, float sy, float angle, float vx, float vy, float vAngle)
{
	ParticleRotMove* data = (ParticleRotMove*)particle->Data_get();
	
	particle->m_UpdateCB = Particle_RotMove_Update;
	particle->Setup(x, y, life, sx, sy, angle);
	
	data->m_AngleSpeed = vAngle;
	data->m_Direction.Set(vx, vy);
}

void Particle_RotMove_Update(Particle* particle, float dt)
{
	ParticleRotMove* data = (ParticleRotMove*)particle->Data_get();
	
	particle->m_Position += data->m_Direction * dt;
	particle->m_Angle += data->m_AngleSpeed * dt;
	
	const float v = particle->m_Life * particle->m_LifeBeginRcp;
		
	const int c = (int)(v * 255.0f);
	
	particle->m_Color.v[3] = c;// = SpriteColor_Make(c, c, c, c);
}

//

ParticleEffect::ParticleEffect()
{
	m_ParticleIndex = -1;
	
	m_Sprite.Allocate(4, 6);
	
	const float size = 0.5f;
	const SpriteColor color = SpriteColors::White;
	
	m_Sprite.m_Vertices[0].m_Coord.x = -size;
	m_Sprite.m_Vertices[0].m_Coord.y = -size;
	m_Sprite.m_Vertices[1].m_Coord.x = +size;
	m_Sprite.m_Vertices[1].m_Coord.y = -size;
	m_Sprite.m_Vertices[2].m_Coord.x = +size;
	m_Sprite.m_Vertices[2].m_Coord.y = +size;
	m_Sprite.m_Vertices[3].m_Coord.x = -size;
	m_Sprite.m_Vertices[3].m_Coord.y = +size;
	
	m_Sprite.m_Vertices[0].m_TexCoord.u = 0.0f;
	m_Sprite.m_Vertices[0].m_TexCoord.v = 0.0f;
	m_Sprite.m_Vertices[1].m_TexCoord.u = 1.0f;
	m_Sprite.m_Vertices[1].m_TexCoord.v = 0.0f;
	m_Sprite.m_Vertices[2].m_TexCoord.u = 1.0f;
	m_Sprite.m_Vertices[2].m_TexCoord.v = 1.0f;
	m_Sprite.m_Vertices[3].m_TexCoord.u = 0.0f;
	m_Sprite.m_Vertices[3].m_TexCoord.v = 1.0f;
	
	m_Sprite.m_Vertices[0].m_Color = color;
	m_Sprite.m_Vertices[1].m_Color = color;
	m_Sprite.m_Vertices[2].m_Color = color;
	m_Sprite.m_Vertices[3].m_Color = color;
	
	m_Sprite.m_Indices[0] = 0;
	m_Sprite.m_Indices[1] = 1;
	m_Sprite.m_Indices[2] = 2;
	m_Sprite.m_Indices[3] = 0;
	m_Sprite.m_Indices[4] = 2;
	m_Sprite.m_Indices[5] = 3;
}

void ParticleEffect::Initialize(int dataSetId)
{
	m_DataSetId = dataSetId;
}

void ParticleEffect::Update(float dt)
{
	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		if (m_Particles[i].m_Life <= 0.0f)
			continue;

		m_Particles[i].m_UpdateCB(&m_Particles[i], dt);
		
		m_Particles[i].m_Life -= dt;
	}
}

void ParticleEffect::Render(SpriteGfx* spriteGfx)
{
	g_GameState->DataSetActivate(m_DataSetId);

	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		const Particle& p = m_Particles[i];
		
		if (p.m_Life <= 0.0f)
			continue;
		
		if (p.m_RenderCB)
		{
			p.m_RenderCB(p);
		}
		else if (p.m_VectorShape)
		{
			// disallow dataset changes while rendering particles; would impact performance too much
			Assert(p.m_VectorShape->m_TextureAtlas->m_Texture->m_DataSetId == m_DataSetId);

			g_GameState->Render(p.m_VectorShape, p.m_Position, p.m_Angle, p.m_Color);
		}
		else
		{
			if (p.m_Image)
			{
				m_Sprite.m_Vertices[0].m_TexCoord.u = p.m_Image->m_TexCoord[0];
				m_Sprite.m_Vertices[0].m_TexCoord.v = p.m_Image->m_TexCoord[1];
				m_Sprite.m_Vertices[1].m_TexCoord.u = p.m_Image->m_TexCoord[0] + p.m_Image->m_TexSize[0];
				m_Sprite.m_Vertices[1].m_TexCoord.v = p.m_Image->m_TexCoord[1];
				m_Sprite.m_Vertices[2].m_TexCoord.u = p.m_Image->m_TexCoord[0] + p.m_Image->m_TexSize[0];
				m_Sprite.m_Vertices[2].m_TexCoord.v = p.m_Image->m_TexCoord[1] + p.m_Image->m_TexSize[1];
				m_Sprite.m_Vertices[3].m_TexCoord.u = p.m_Image->m_TexCoord[0];
				m_Sprite.m_Vertices[3].m_TexCoord.v = p.m_Image->m_TexCoord[1] + p.m_Image->m_TexSize[1];
			}
			else
			{
				m_Sprite.m_Vertices[0].m_TexCoord.u = 0.0f;
				m_Sprite.m_Vertices[0].m_TexCoord.v = 0.0f;
				m_Sprite.m_Vertices[1].m_TexCoord.u = 1.0f;
				m_Sprite.m_Vertices[1].m_TexCoord.v = 0.0f;
				m_Sprite.m_Vertices[2].m_TexCoord.u = 1.0f;
				m_Sprite.m_Vertices[2].m_TexCoord.v = 1.0f;
				m_Sprite.m_Vertices[3].m_TexCoord.u = 0.0f;
				m_Sprite.m_Vertices[3].m_TexCoord.v = 1.0f;
			}
			
			for (int j = 0; j < m_Sprite.m_VertexCount; ++j)
				m_Sprite.m_Vertices[j].m_Color = p.m_Color;
			
			spriteGfx->WriteSprite(m_Sprite, p.m_Position[0], p.m_Position[1], p.m_Angle, p.m_Sx, p.m_Sy);
		}
	}
	
//	spriteGfx->Flush();
}

Particle& ParticleEffect::Allocate(const Atlas_ImageInfo* image, const VectorShape* vectorShape, ParticleUpdateCB updateCB)
{
	// disallow dataset changes while rendering particles; would impact performance too much
	//if (image != 0)
	//	Assert(p.m_VectorShape->m_TextureAtlas->m_Texture->m_DataSetId == m_DataSetId);
	if (vectorShape != 0)
	{
		Assert(vectorShape->m_TextureAtlas->m_Texture->m_DataSetId == m_DataSetId);
	}

	m_ParticleIndex = m_ParticleIndex + 1;
	
	if (m_ParticleIndex >= MAX_PARTICLES)
		m_ParticleIndex = 0;
	
	if (updateCB == 0)
		updateCB = Particle_Default_Update;
	
	m_Particles[m_ParticleIndex].m_Image = image;
	m_Particles[m_ParticleIndex].m_VectorShape = vectorShape;
	m_Particles[m_ParticleIndex].m_UpdateCB = updateCB;
	m_Particles[m_ParticleIndex].m_RenderCB = 0;
	
	return m_Particles[m_ParticleIndex];
}

void ParticleEffect::Clear()
{
	for (int i = 0; i < MAX_PARTICLES; ++i)
		m_Particles[i].m_Life = 0.0f;
}

void ParticleEffect::ForEach(ParticleForEachCB cb)
{
	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		const Particle& p = m_Particles[i];
		
		if (p.m_Life <= 0.0f)
			continue;
		
		cb(p);
	}
}
