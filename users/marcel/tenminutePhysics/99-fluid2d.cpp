#include "framework.h"
	
#define VIEW_SX 800
#define VIEW_SY 600

static const auto drawOrig = Vec2(VIEW_SX/2.f, VIEW_SY - 20.f);
static const auto drawScale = 200.f;

// global params

static const float gravity = -10.f;
static const float particleRadius = .01f;
static const bool unilateral = true;
static const float viscosity = 0.f;

static const float timeStep = .01f;
static const int numIters = 1;
static const int numSubSteps = 10;
	
static const int maxParticles = 10000;

static int numX = 10;
static int numY = 1000;
static int numParticles = numX * numY;

// boundary

static const float width  = 1.f;
static const float height = 2.f;

struct Boundary
{
	float left;
	float right;
	float bottom;
	float top;
};

const std::vector<Boundary> boundaries =
{
	{ -width * .5f - .1f, -width * .5f,       -.01f, height },
	{ +width * .5f,       +width * .5f + .1f, -.01f, height },
};

const Boundary fluidOrig = { -.3f, 0.f, 1.8f, 0.f };

// derived params

static const float particleDiameter = 2.f * particleRadius;
static const float restDensity = 1.f / (particleDiameter * particleDiameter);
static const float kernelRadius = 3.f * particleRadius;
static const float h2 = kernelRadius * kernelRadius;
static const float kernelScale = 4.f / (float(M_PI) * h2 * h2 * h2 * h2);
// 2d poly6 (SPH based shallow water simulation

static const float gridSpacing = kernelRadius * 1.5f;
static const float invGridSpacing = 1.f / gridSpacing;

static const float maxVel = .4f * particleRadius;
	
struct
{
	float pos [2 * maxParticles] = { };
	float prev[2 * maxParticles] = { };
	float vel [2 * maxParticles] = { };
} particles;

struct Vector
{
	std::vector<int> vals;
	int maxSize = 0;
	int size = 0;
	
	Vector(const int size)
	{
		this->vals.resize(size);
		this->maxSize = size;
		this->size = 0;
	}
	
	void clear()
	{
		this->size = 0;
	}
	
	void pushBack(const int val)
	{
		if (this->size >= this->maxSize)
		{
			this->maxSize *= 2;
			auto old = this->vals;
			this->vals.resize(this->maxSize);
			for (int i = 0; i < old.size(); ++i)
				this->vals[i] = old[i];
		}
		
		this->vals[this->size++] = val;
	}
};

static const int hashSize = 370111;

struct
{
	uint32_t size = hashSize;

	int first[hashSize] = { };
	int marks[hashSize] = { };
	int currentMark = 0;

	int next[maxParticles] = { };
	
	Boundary orig { -100.f, 0.f, -1.f, 0.f };
} hash;

static void setup(const int initNumX, const int initNumY)
{
	if (initNumX * initNumY > maxParticles)
		return;
		
	numX = initNumX;
	numY = initNumY;
	numParticles = numX * numY;

	int nr = 0;
	
	for (int j = 0; j < numY; j++)
	{
		for (int i = 0; i < numX; i++)
		{
			particles.pos[nr] = fluidOrig.left + i * particleDiameter;
			particles.pos[nr] += 0.00001 * (j % 2);
			particles.pos[nr + 1] = fluidOrig.bottom + j * particleDiameter;
			particles.vel[nr] = 0.0;
			particles.vel[nr + 1] = 0.0;
			
			nr += 2;
		}
	}
	
	for (int i = 0; i < hashSize; i++)
	{
		hash.first[i] = -1;
		hash.marks[i] = 0;
	}
}

static void solveBoundaries()
{
	const float minX = VIEW_SX * .5f / drawScale;
	
	for (int i = 0; i < numParticles; i++)
	{
		const float px = particles.pos[2 * i    ];
		const float py = particles.pos[2 * i + 1];
		
		if (py < 0.0)		// ground
		{
			particles.pos[2 * i + 1] = 0.f;
//				particles.pos[2 * i] = 0.f;
		}
		
		if (px < -minX)
			particles.pos[2 * i] = -minX;
		if (px > +minX)
			particles.pos[2 * i] = +minX;
			
		for (int j = 0; j < boundaries.size(); j++)
		{
			auto & b = boundaries[j];
			
			if (px < b.left || px > b.right || py < b.bottom || py > b.top)
				continue;
			
			float dx, dy;
			if (px < (b.left + b.right) * .5f)
				dx = b.left - px;
			else
				dx = b.right - px;
				
			if (py < (b.bottom + b.top) * .5f)
				dy = b.bottom - py;
			else
				dy = b.top - py;
				
			if (fabsf(dx) < fabsf(dy))
				particles.pos[2 * i    ] += dx;
			else
				particles.pos[2 * i + 1] += dy;
		}
	}
}

// -----------------------------------------------------------------------------------

static int firstNeighbor[maxParticles + 1];
static Vector neighbors(10 * maxParticles);
			
static void findNeighbors()
{
	// hash particles
	
	hash.currentMark++;
	
	for (int i = 0; i < numParticles; i++)
	{
		const float px = particles.pos[2 * i    ];
		const float py = particles.pos[2 * i + 1];
		
		//const uint32_t gx = int(floorf((px - hash.orig.left  ) * invGridSpacing));
		//const uint32_t gy = int(floorf((py - hash.orig.bottom) * invGridSpacing));
		const uint32_t gx = uint32_t((px - hash.orig.left  ) * invGridSpacing);
		const uint32_t gy = uint32_t((py - hash.orig.bottom) * invGridSpacing);
		
		const uint32_t h = uint32_t((gx * 92837111) ^ (gy * 689287499)) % hash.size;
					
		if (hash.marks[h] != hash.currentMark)
		{
			hash.marks[h] = hash.currentMark;
			hash.first[h] = -1;
		}

		hash.next[i] = hash.first[h];
		hash.first[h] = i;
	}
	
	// collect neighbors
	
	neighbors.clear();

	const float h2 = gridSpacing * gridSpacing;

	for (int i = 0; i < numParticles; i++)
	{
		firstNeighbor[i] = neighbors.size;
		
		const float px = particles.pos[2 * i];
		const float py = particles.pos[2 * i + 1];
		
		const uint32_t gx = uint32_t((px - hash.orig.left  ) * invGridSpacing);
		const uint32_t gy = uint32_t((py - hash.orig.bottom) * invGridSpacing);
		
		for (int x = gx - 1; x <= gx + 1; x++)
		{
			for (int y = gy - 1; y <= gy + 1; y++)
			{
				const uint32_t h = uint32_t((x * 92837111) ^ (y * 689287499)) % hash.size;
					
				if (hash.marks[h] != hash.currentMark)
					continue;
			
				int id = hash.first[h];
				
				while (id >= 0)
				{
					const float dx = particles.pos[2 * id    ] - px;
					const float dy = particles.pos[2 * id + 1] - py;
					
					if (dx * dx + dy * dy < h2)
						neighbors.pushBack(id);

					id = hash.next[id];
				}
			}
		}
	}
	
	firstNeighbor[numParticles] = neighbors.size;
}

// -----------------------------------------------------------------------------------

static float grads[1000] = { };

static bool sand = false;
	
static void solveFluid()
{
	const float h = kernelRadius;
	const float h2 = h * h;
	
	float avgRho = 0.f;

	for (int i = 0; i < numParticles; i++)
	{
		const float px = particles.pos[2 * i    ];
		const float py = particles.pos[2 * i + 1];

		const int first = firstNeighbor[i];
		const int num = firstNeighbor[i + 1] - first;

		float rho = 0.f;
		float sumGrad2 = 0.f;

		float gradix = 0.f;
		float gradiy = 0.f;
		
		for (int j = 0; j < num; j++)
		{
			const int id = neighbors.vals[first + j];
			
			float nx = particles.pos[2 * id    ] - px;
			float ny = particles.pos[2 * id + 1] - py;
			const float r = sqrtf(nx * nx + ny * ny);
			
			if (r > 0.f)
			{
				nx /= r;
				ny /= r;
			}
				
			if (sand)
			{
				if (r < 2.f * particleRadius)
				{
					const float d = .5f * (2.f * particleRadius - r);
					
					particles.pos[2 * i     ] -= nx * d;
					particles.pos[2 * i  + 1] -= ny * d;
					particles.pos[2 * id    ] += nx * d;
					particles.pos[2 * id + 1] += ny * d;
					
					/*
					var tx = ny;
					var ty = -nx;
					var vx0 = particles.pos[2 * id] - paricles.prev[2 * id];
					var vy0 = particles.pos[2 * id + 1] - paricles.prev[2 * id + 1];
					*/
				}
				
				continue;
			}
			
			if (r > h)
			{
				grads[2 * j    ] = 0.f;
				grads[2 * j + 1] = 0.f;
			}
			else
			{
				const float r2 = r * r;
				const float w = (h2 - r2);
				rho += kernelScale * w * w * w;
				const float grad = (kernelScale * 3.f * w * w * (-2.f * r)) / restDensity;
				grads[2 * j    ] = nx * grad;
				grads[2 * j + 1] = ny * grad;
				gradix -= nx * grad;
				gradiy -= ny * grad;
				sumGrad2 += grad * grad;
			}
		}
		
		sumGrad2 += (gradix * gradix + gradiy * gradiy);

		avgRho += rho;

		const float C = rho / restDensity - 1.f;
		if (unilateral && C < 0.f)
			continue;

		const float lambda = -C / (sumGrad2 + .0001f);

		for (int j = 0; j < num; j++)
		{
			const int id = neighbors.vals[first + j];
			
			if (id == i)
			{
				particles.pos[2 * id    ] += lambda * gradix;
				particles.pos[2 * id + 1] += lambda * gradiy;
			
			}
			else
			{
				particles.pos[2 * id    ] += lambda * grads[2 * j    ];
				particles.pos[2 * id + 1] += lambda * grads[2 * j + 1];
			}
		}
	}
}

// -----------------------------------------------------------------------------------
	
static void applyViscosity(const int pnr, const float dt)
{
	const int first = firstNeighbor[pnr];
	const int num = firstNeighbor[pnr + 1] - first;

	if (num == 0)
		return;

	float avgVelX = 0.f;
	float avgVelY = 0.f;
		
	for (int j = 0; j < num; j++)
	{
		const int id = neighbors.vals[first + j];
		
		avgVelX += particles.vel[2 * id    ];
		avgVelY += particles.vel[2 * id + 1];
	}
			
	avgVelX /= num;
	avgVelY /= num;
	
	const float deltaX = avgVelX - particles.vel[2 * pnr    ];
	const float deltaY = avgVelY - particles.vel[2 * pnr + 1];
	
	particles.vel[2 * pnr    ] += viscosity * deltaX;
	particles.vel[2 * pnr + 1] += viscosity * deltaY;
}

// -----------------------------------------------------------------------------------
	
static void simulate()
{
	findNeighbors();
	
	const float dt = timeStep / numSubSteps;
	
	for (int step = 0; step < numSubSteps; step ++)
	{
		// predict
		
		for (int i = 0; i < numParticles; i++)
		{
			particles.vel[2 * i + 1] += gravity * dt;
			
			particles.prev[2 * i    ] = particles.pos[2 * i    ];
			particles.prev[2 * i + 1] = particles.pos[2 * i + 1];
			
			particles.pos[2 * i    ] += particles.vel[2 * i    ] * dt;
			particles.pos[2 * i + 1] += particles.vel[2 * i + 1] * dt;
		}

		// solve
		
		solveBoundaries();
		solveFluid();
		
		// derive velocities
		
		for (int i = 0; i < numParticles; i++)
		{
			float vx = particles.pos[2 * i    ] - particles.prev[2 * i    ];
			float vy = particles.pos[2 * i + 1] - particles.prev[2 * i + 1];
			
			// CFL
			
			const float v = sqrtf(vx * vx + vy * vy);
			
			if (v > maxVel)
			{
				vx *= maxVel / v;
				vy *= maxVel / v;
				particles.pos[2 * i    ] = particles.prev[2 * i    ] + vx;
				particles.pos[2 * i + 1] = particles.prev[2 * i + 1] + vy;
			}
			
			particles.vel[2 * i    ] = vx / dt;
			particles.vel[2 * i + 1] = vy / dt;
			
			applyViscosity(i, dt);
		}
	}
}

// -----------------------------------------------------------------------------------

static void draw()
{
	// particles
	
	hqBegin(HQ_FILLED_CIRCLES);
	{
		for (int i = 0; i < numParticles; i++)
		{
			const float px = drawOrig[0] + particles.pos[i * 2    ] * drawScale;
			const float py = drawOrig[1] - particles.pos[i * 2 + 1] * drawScale;

			if (((i / 1000) % 2) == 0)
				setColor(colorBlue);
			else
				setColor(colorRed);

			hqFillCircle(px, py, particleRadius * drawScale);
		}
	}
	hqEnd();
	
	// boundaries

	setColor(200, 200, 200);
	
	for (int i = 0; i < boundaries.size(); i++)
	{
		const Boundary & b = boundaries[i];
		
		const float left = drawOrig[0] + b.left * drawScale;
		const float right = drawOrig[0] + b.right * drawScale;
		const float top = drawOrig[1] - b.top * drawScale;
		const float bottom = drawOrig[1] - b.bottom * drawScale;
		
		drawRectLine(left, top, right, bottom);
	}
	
	drawLine(0, drawOrig[1], VIEW_SX, drawOrig[1]);
}
	
// -----------------------------------------------------------------------------------
	
#include "Timer.h"

int timeFrames = 0;
int timeSum = 0;

void step()
{
	auto startTime = g_TimerRT.TimeUS_get();
	
	simulate();
	
	auto endTime = g_TimerRT.TimeUS_get();

	timeSum += endTime - startTime;
	timeFrames++;
	
	if (timeFrames > 10)
	{
		timeSum /= timeFrames;
		logDebug("time: %.2fms", timeSum / 1000.f);
		timeFrames = 0;
		timeSum = 0;
	}
	
	draw();
}

/*
// todo : viscosity slider
document.getElementById("viscositySlider").oninput = function() {
	viscosity = this.value / 15;
//		document.getElementById("viscosity").innerHTML = viscosity.toString();
}
*/

// main

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	setup(10, 1000);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		step();
		
		framework.beginDraw(20, 20, 20, 0);
		{
			draw();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
