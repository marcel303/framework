#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>
#import <OpenGLES/ES1/gl.h>

#define MAX_PARTICLES 1000

static void CheckError()
{
	GLenum error = glGetError();
	
	if (error == GL_NO_ERROR)
		return;
	
	NSLog(@"error");
	
	assert(false);
}

typedef struct Rgba
{
	union
	{
		uint32_t c;
		uint8_t v[4];
	};
};

static inline Rgba Rgba_Make(int r, int g, int b, int a)
{
	Rgba result;
	
	result.v[0] = r;
	result.v[1] = g;
	result.v[2] = b;
	result.v[3] = a;
	
	return result;
}

class Particle
{
public:
	void Setup(float x, float y, float vx, float vy, Rgba c)
	{
		alive = 1;
		this->x = x;
		this->y = y;
		this->vx = vx;
		this->vy = vy;
		this->c = c;
	}
	
	int alive;
	float x;
	float y;
	float vx;
	float vy;
	Rgba c;
};

typedef struct Vertex
	{
		float x;
		float y;
		uint32_t c;
		float u;
		float v;
	};

class TouchState
{
public:
	TouchState()
	{
		enabled = false;
	}
	
	bool enabled;
	float x;
	float y;
};

class Rain
{
public:
	Rain()
	{
		mParticleIndex = 0;
		mVertexBase = 0;
		mIndexBase = 0;
	}
	
	void Initialize()
	{
		bzero(mParticles, sizeof(Particle) * MAX_PARTICLES);
	}
	
	void Update(float dt)
	{
		Particle* p = mParticles;
		
		const float falloff = powf(0.3f, dt);
		
		for (int i = 0; i < MAX_PARTICLES; ++i, ++p)
		{
			if (!p->alive)
				continue;
			
			float ax = 0.0f;
			float ay = 0.0f;
			
			// todo: apply finger acceleration
			
			for (int j = 0; j < 5; ++j)
			{
				if (!mTouches[j].enabled)
					continue;
				
				float dx = p->x - mTouches[j].x;
				float dy = p->y - mTouches[j].y;
				
				float distanceSq = dx * dx + dy * dy;
				
				float radius = 50.0f;
				float radiusSq = radius * radius;
				
				if (distanceSq > radiusSq)
					continue;
				
				float distance = sqrtf(distanceSq);
				
				float nx = dx / distance;
				float ny = dy / distance;
				
				float depth = radius - distance;
				
				p->x += nx * depth;
				p->y += ny * depth;
				
				float vd = p->vx * nx + p->vy * ny;
				
				p->vx -= vd * nx * 1.1f;
				p->vy -= vd * ny * 1.1f;
			}
			
			// todo: apply gravity acceleration
			
			ax += 5.0f;
			ay += 500.0f;
			
			p->vx += ax * dt;
			p->vy += ay * dt;
			p->x += p->vx * dt;
			p->y += p->vy * dt;
			
			p->vx *= falloff;
			p->vy *= falloff;
			
			// todo: check if particle outside screen
		}
	}
	
	void Render()
	{
		Particle* p = mParticles;
		
		for (int i = 0; i < MAX_PARTICLES; ++i, ++p)
		{
			if (!p->alive)
				continue;
			
			RenderQuad(p->x, p->y, 3.0f, 10.0f, p->c.c);
		}
		
		if (mIndexBase == 0)
			return;
		
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);	
		CheckError();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		CheckError();
		
		glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &mVertices->x);
		CheckError();
		glEnableClientState(GL_VERTEX_ARRAY);
		CheckError();
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &mVertices->c);
		CheckError();
		glEnableClientState(GL_COLOR_ARRAY);
		CheckError();
		
		glDrawElements(GL_TRIANGLES, mIndexBase, GL_UNSIGNED_SHORT, mIndices);
		CheckError();
		
		glDisable(GL_BLEND);
		CheckError();
//		NSLog(@"render %d", mIndexBase);
		
		mVertexBase = 0;
		mIndexBase = 0;
	}
	
	inline void RenderQuad(float x, float y, float sx, float sy, uint32_t c)
	{
		Vertex* vertex = mVertices + mVertexBase;
		
		vertex->x = x - sx;
		vertex->y = y - sy;
		vertex->c = c;
		vertex->u = 0.0f;
		vertex->v = 0.0f;
		
		vertex++;
		
		vertex->x = x + sx;
		vertex->y = y - sy;
		vertex->c = c;
		vertex->u = 1.0f;
		vertex->v = 0.0f;
		
		vertex++;
		
		vertex->x = x + sx;
		vertex->y = y + sy;
		vertex->c = c;
		vertex->u = 1.0f;
		vertex->v = 1.0f;
		
		vertex++;
		
		vertex->x = x - sx;
		vertex->y = y + sy;
		vertex->c = c;
		vertex->u = 0.0f;
		vertex->v = 1.0f;
		
		uint16_t* index = mIndices + mIndexBase;
		
		index[0] = mVertexBase + 0;
		index[1] = mVertexBase + 1;
		index[2] = mVertexBase + 2;
		index[3] = mVertexBase + 0;
		index[4] = mVertexBase + 2;
		index[5] = mVertexBase + 3;
		
		mVertexBase += 4;
		mIndexBase += 6;
	}
	
	Particle* Allocate()
	{
		Particle* p = mParticles + mParticleIndex;
		
		mParticleIndex++;
		
		if (mParticleIndex >= MAX_PARTICLES)
			mParticleIndex = 0;
		
		return p;
	}
	
/*	void Enable(int index, float x, float y)
	{
		mTouches[index].enabled = true;
		mTouches[index].x = x;
		mTouches[index].y = y;
	}*/
	
	TouchState mTouches[5];
	
	Particle mParticles[MAX_PARTICLES];
	int mParticleIndex;
	
	Vertex mVertices[MAX_PARTICLES * 4];
	uint16_t mIndices[MAX_PARTICLES * 6];
	
	int mVertexBase;
	int mIndexBase;
};
