#include "gpuParticleSystem.h"

static const int kMaxParticleBufferSx = 1024;

void GpuParticleSystem::init(const int in_numParticles, const int in_sx, const int in_sy)
{
	numParticles = in_numParticles;
	
	bounds.min.Set(0.f, 0.f);
	bounds.max.Set(in_sx, in_sy);
	
	// note : position and velocity need to be double buffered as they are feedbacking onto themselves
	
	const int sx = kMaxParticleBufferSx;
	const int sy = (numParticles + kMaxParticleBufferSx - 1) / kMaxParticleBufferSx;
	p.init(sx, sy, SURFACE_RGBA32F, false, true);
	p.setName("ParticleSystem.positionsAndVelocities");
	
	for (int i = 0; i < 2; ++i)
	{
		p.clear();
		p.swapBuffers();
	}
	
	setColorClamp(false);
	pushSurface(&p);
	{
		pushBlend(BLEND_OPAQUE);
		gxBegin(GX_POINTS);
		for (int x = 0; x < p.getWidth(); ++x)
		{
			for (int y = 0; y < p.getHeight(); ++y)
			{
				setColorf(rand() % in_sx, rand() % in_sy, 0, 0);
				gxVertex2f(x + .5f, y + 0.5f);
			}
		}
		gxEnd();
		popBlend();
	}
	popSurface();
	
	particleTexture = generateParticleTexture();
}

void GpuParticleSystem::shut()
{
	freeTexture(particleTexture);
	
	p.free();
	
	numParticles = 0;
}

GxTextureId GpuParticleSystem::generateParticleTexture() const
{
	float data[16][16][2];
	
	for (int y = 0; y < 16; ++y)
	{
		for (int x = 0; x < 16; ++x)
		{
			const float dx = x / 15.f * 2.f - 1.f;
			const float dy = y / 15.f * 2.f - 1.f;
			const float d = hypotf(dx, dy);
			
			const float vx = d <= 1.f ? dx : 0.f;
			const float vy = d <= 1.f ? dy : 0.f;
			
			data[y][x][0] = vx;
			data[y][x][1] = vy;
		}
	}
	
	return createTextureFromRG32F(data, 16, 16, true, true);
}

void GpuParticleSystem::setBounds(const float minX, const float minY, const float maxX, const float maxY)
{
	bounds.enabled = true;
	bounds.min.Set(minX, minY);
	bounds.max.Set(maxX, maxY);
}

void GpuParticleSystem::drawParticleVelocity(const int in_numParticles) const
{
	if (in_numParticles <= 0)
		return;
	
	const int numParticles =
		in_numParticles < this->numParticles
		? in_numParticles
		: this->numParticles;
	
	Shader shader("particle-draw-field");
	setShader(shader);
	{
		shader.setTexture("p", 0, p.getTexture(), false, true);
		shader.setTexture("particleTexture", 1, particleTexture, true, true);
		shader.setImmediate("particleSize", dimensions.velocitySize);
		shader.setImmediate("strength", repulsion.strength);
		gxEmitVertices(GX_TRIANGLES, numParticles * 6);
	}
	clearShader();
}

void GpuParticleSystem::drawParticleColor(const int in_numParticles) const
{
	if (in_numParticles <= 0)
		return;
	
	const int numParticles =
		in_numParticles < this->numParticles
		? in_numParticles
		: this->numParticles;
	
	Shader shader("particle-draw-color");
	setShader(shader);
	{
		shader.setTexture("p", 0, p.getTexture(), false, true);
		shader.setImmediate("particleSize", dimensions.colorSize);
		shader.setImmediate("baseColor", baseColor.r, baseColor.g, baseColor.b, baseColor.a);
		shader.setImmediate("lightAmount", drawColor.lightAmount);
		shader.setImmediate("particleSizeThreshold", drawColor.sizeThreshold);
		gxEmitVertices(GX_TRIANGLES, numParticles * 6);
	}
	clearShader();
}

void GpuParticleSystem::updateParticles(const GxTextureId flowfield, const float dt)
{
	const GxTextureId pTex = p.getTexture();
	
	p.swapBuffers();
	
	pushSurface(&p);
	{
		pushBlend(BLEND_OPAQUE);
		Shader shader("particle-update");
		setShader(shader);
		{
			shader.setTexture("p", 0, pTex, false, true);
			shader.setTexture("flowfield", 1, flowfield, true, true);
			shader.setImmediate("drag", powf(1.f - drag, dt));
			shader.setImmediate("grav_pos", gravity.origin[0], gravity.origin[1]);
			shader.setImmediate("grav_force", gravity.strength);
			shader.setImmediate("flow_strength", flow.strength);
			shader.setImmediate("bounds", bounds.min[0], bounds.min[1], bounds.max[0], bounds.max[1]);
			shader.setImmediate("boundsParams", bounds.xMode, bounds.yMode, bounds.enabled, 0.f);
			drawRect(0, 0, p.getWidth(), p.getHeight());
		}
		clearShader();
		popBlend();
	}
	popSurface();
}
