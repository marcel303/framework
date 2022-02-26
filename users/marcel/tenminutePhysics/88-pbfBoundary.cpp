/*
Copyright 2021 Matthias Muller - Ten Minute Physics

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

#define PHYS_TODO 0

#define VIEW_SX 800
#define VIEW_SY 600

static float simMinWidth = 2.f;
static float cScale = 1.f;
static float simWidth = 1.f;
static float simHeight = 1.f;

static void setupGlobals()
{
    simMinWidth = 2.f;
    cScale = fminf(VIEW_SX, VIEW_SY) / simMinWidth;
    simWidth = VIEW_SX / cScale;
    simHeight = VIEW_SY / cScale;
}

static float cX(const float posX)
{
    return posX * cScale;
}

static float cY(const float posY)
{
    return VIEW_SY - posY * cScale;
}

// vector math -------------------------------------------------------

float length(const float x0, const float y0, const float x1, const float y1)
{
    return sqrtf((x0 - x1)*(x0-x1) + (y0-y1)*(y0-y1));
}

struct Mat2
{
	float e00;
	float e01;
	float e10;
	float e11;
	
    Mat2(const float e00_, const float e01_, const float e10_, const float e11_)
    {
        this->e00 = e00_;
        this->e01 = e01_;
        this->e10 = e10_;
        this->e11 = e11_;
    }

    static Mat2 rotation(const float angle)
    {
 	    const float cosA = cosf(angle);
        const float sinA = sinf(angle);

	    return Mat2(cosA, -sinA, sinA,  cosA);
    }
};

struct Vector2
{
	float x = 0.f;
	float y = 0.f;
	
    Vector2(const float x = 0.f, const float y = 0.f)
    {
        this->x = x;
        this->y = y;
    }

    void set(const Vector2 & v)
    {
        this->x = v.x;
        this->y = v.y;
    }

    Vector2 clone()
    {
        return *this;
    }

    Vector2 & add(const Vector2 & v)
    {
        this->x += v.x;
        this->y += v.y;
        return *this;
    }
    
    Vector2 & add(const Vector2 & v, const float s)
    {
        this->x += v.x * s;
        this->y += v.y * s;
        return *this;
    }

    Vector2 & addVectors(const Vector2 & a, const Vector2 & b)
    {
        this->x = a.x + b.x;
        this->y = a.y + b.y;
        return *this;
    }

    Vector2 & subtract(const Vector2 & v)
    {
        this->x -= v.x;
        this->y -= v.y;
        return *this;
    }
    
    Vector2 & subtract(const Vector2 & v, const float s)
    {
        this->x -= v.x * s;
        this->y -= v.y * s;
        return *this;
    }

    Vector2 & subtractVectors(const Vector2 & a, const Vector2 & b)
    {
        this->x = a.x - b.x;
        this->y = a.y - b.y;
        return *this;
    }

    float length() const
    {
        return sqrtf(this->x * this->x + this->y * this->y);
    }

    Vector2 & scale(const float s)
    {
        this->x *= s;
        this->y *= s;
        return *this;
    }
    
    Vector2 & invert()
    {
		this->x = -this->x;
		this->y = -this->y;
		return *this;
	}

    float dot(const Vector2 & v) const
    {
        return this->x * v.x + this->y * v.y;
    }

    Vector2 perp() const
    {
        return Vector2(-this->y, this->x);
    }

    void rotate(const Mat2 & m)
    {
        const float x = m.e00 * this->x + m.e01 * this->y;
	    const float y = m.e10 * this->x + m.e11 * this->y;
	    
        this->x = x;
        this->y = y;
    }
};

struct ConstraintsArray
{
    static const int IS = 2;
    static const int DS = 7;

	std::vector<int> Idx;
	std::vector<float> Data;
	int maxSize = 0;
	int size = 0;
	
    ConstraintsArray(const int size)
    {
        this->Idx.resize(ConstraintsArray::IS*size, 0);
        this->Data.resize(ConstraintsArray::DS*size, 0);
        this->maxSize = size;
        this->size = 0;
    }

    void clear()
    {
        this->size = 0;
    }

    void pushBack(
		const int A,
		const int B,
		const float dvx,
		const float dvy,
		const float nx,
		const float ny,
		const float d_lambda_n,
		const float mu_k,
		const float e)
    {
        if (this->size >= this->maxSize)
        {
            this->maxSize *= 2;
            this->Idx.resize(ConstraintsArray::IS*this->maxSize, 0);
            this->Data.resize(ConstraintsArray::DS*this->maxSize, 0.f);
        }
        
        const int is = ConstraintsArray::IS;
        const int ds = ConstraintsArray::DS;
        
        this->Idx[is*this->size + 0] = A;
        this->Idx[is*this->size + 1] = B;

        this->Data[ds*this->size + 0] = dvx;
        this->Data[ds*this->size + 1] = dvy;
        this->Data[ds*this->size + 2] = nx;
        this->Data[ds*this->size + 3] = ny;
        this->Data[ds*this->size + 4] = d_lambda_n;
        this->Data[ds*this->size + 5] = mu_k;
        this->Data[ds*this->size + 6] = e;
        this->size++;
    }

    int idxA(const int i) {
        return this->Idx[ConstraintsArray::IS*i+ 0];
    }
    int idxB(const int i) {
        return this->Idx[ConstraintsArray::IS*i + 1];
    }
    float dvx(const int i) {
        return this->Data[ConstraintsArray::DS*i + 0];
    }
    float dvy(const int i) {
        return this->Data[ConstraintsArray::DS*i + 1];
    }
    float nx(const int i) {
        return this->Data[ConstraintsArray::DS*i + 2];
    }
    float ny(const int i) {
        return this->Data[ConstraintsArray::DS*i + 3];
    }
    float d_lambda_n(const int i) {
        return this->Data[ConstraintsArray::DS*i + 4];
    }
    float mu_k(const int i) {
        return this->Data[ConstraintsArray::DS*i + 5];
    }
    float e(const int i) {
        return this->Data[ConstraintsArray::DS*i + 6];
    }
};

struct Vector
{
	std::vector<int> vals;
	int maxSize = 0;
	int size = 0;
	
    Vector(const int size)
    {
        this->vals.resize(size, 0);
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
            this->vals.resize(this->maxSize, 0);
        }
        this->vals[this->size++] = val;
    }
};

// boundary class -------------------------------------------------------
struct CircleBoundary
{
	Vector2 pos;
	float radius = 0.f;
	bool inverted = false;
	
    CircleBoundary(const Vector2 & pos_, const float radius_, const bool inverted_)
    {
        this->pos = Vector2(pos_.x, pos_.y);
        this->radius = radius_;
        this->inverted = inverted_;
    }
};

// scene -------------------------------------------------------

struct PhysicsScene
{
	Vector2 gravity = Vector2(0.f, -9.81f);
	float dt = 1.f / 60.f;
	int numSteps = 10;
	float radius = 0.f;
	bool paused = false;
	int numRows = 50;
	int numColumns = 30;
	int numParticles = 0;
	Vector2 boundaryCenter = Vector2(0.f, 0.f);
	float boundaryRadius = 0.f;
	std::vector<CircleBoundary> boundaries;
	float rotation_angle = 0.f;
};

static PhysicsScene physicsScene;

const float particleRadius = .01f;
const float maxVel = .4f * particleRadius;
const float mu_s = .2f; // static friction
const float mu_k = .2f; // dynamic friction
const float e = .2f; // restitution

// if enabled, then using velocity pass as described in
// Detailed Rigid Body Sumulatio with Extended PBD
bool use_velocity_pass = false;

static const int maxParticles = 10000;

static float * allocFloatVec(const int size)
{
	float * result = new float[size];
	std::fill(result, result + size, 0.f);
}

struct Particles
{
	float pos[2 * maxParticles] = { };
    float prev[2 * maxParticles] = { };
    float vel[2 * maxParticles] = { };
};

static Particles particles;

static ConstraintsArray constraints(1);

// -----------------------------------------------------------------------------------

const int hashSize = 370111;

struct Hash
{
    int size = hashSize;

    int first[hashSize] = { };
    int marks[hashSize] = { };
    int currentMark = 0;

	int next[maxParticles] = { };

	float orig_left = -100.f;
	float orig_bottom = -1.f;
};

static Hash hash;

static const float gridSpacing = 2 * 5 * particleRadius;
static const float invGridSpacing = 1.f / gridSpacing;
static int firstNeighbor[maxParticles + 1];
static Vector neighbors(10);// * maxParticles);

void initNeighborsHash()
{
    for (int i = 0; i < hashSize; i++)
    {
        hash.first[i] = -1;
        hash.marks[i] = 0;
    }
}

void findNeighbors()
{
    // hash particles
    hash.currentMark++;

    for (int i = 0; i < physicsScene.numParticles; i++)
    {
        const float px = particles.pos[2 * i];
        const float py = particles.pos[2 * i + 1];

        const uint32_t gx = uint32_t(floorf((px - hash.orig_left  ) * invGridSpacing));
        const uint32_t gy = uint32_t(floorf((py - hash.orig_bottom) * invGridSpacing));

        const uint32_t h = ((gx * 92837111) ^ (gy * 689287499)) % hash.size;

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

    for (int i = 0; i < physicsScene.numParticles; i++)
    {
        firstNeighbor[i] = neighbors.size;

        const float px = particles.pos[2 * i];
        const float py = particles.pos[2 * i + 1];

        const uint32_t gx = uint32_t(floorf((px - hash.orig_left  ) * invGridSpacing));
        const uint32_t gy = uint32_t(floorf((py - hash.orig_bottom) * invGridSpacing));

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
                    {
                        neighbors.pushBack(id);
					}
					
                    id = hash.next[id];
                }
            }
        }
    }
    
    firstNeighbor[physicsScene.numParticles] = neighbors.size;
}

// draw -------------------------------------------------------

static void drawCircle(const float posX, const float posY, const float radius, const bool filled)
{
	if (filled)
		fillCircle(cX(posX), cY(posY), radius * cScale, 10);
	else
		drawCircle(cX(posX), cY(posY), radius * cScale, 10);
}

static void drawCircleV(const Vector2 & pos, const float radius, const bool filled)
{
    drawCircle(pos.x, pos.y, radius, filled);
}

static void draw()
{
	setColor(colorWhite);
	
	hqBegin(HQ_FILLED_CIRCLES);
	{
		for (int i = 0; i < physicsScene.numParticles; ++i)
		{
			const float x = particles.pos[2*i    ];
			const float y = particles.pos[2*i + 1];
			hqFillCircle(cX(x), cY(y), particleRadius * cScale);
		}
	}
    hqEnd();

	setColor(colorRed);

    drawCircleV(physicsScene.boundaryCenter, physicsScene.boundaryRadius, false);

	setColor(0x55, 0x55, 0xff);

	hqBegin(HQ_FILLED_CIRCLES);
	{
		for (int i = 1; i < physicsScene.boundaries.size(); ++i)
		{
			const float x = physicsScene.boundaries[i].pos.x;
			const float y = physicsScene.boundaries[i].pos.y;
			const float r = physicsScene.boundaries[i].radius;
			
			hqFillCircle(cX(x), cY(y), r * cScale);
		}
	}
	hqEnd();
}

// ------------------------------------------------

static void calcStaticFriction(const Vector2 & dp, const Vector2 & n, const float dist, Vector2 & frict)
{
    Vector2 dp_n;
    Vector2 dp_t;

    // perp projection
    const float proj = dp.dot(n);
    dp_n.add(n, proj);

    dp_t.subtractVectors(dp, dp_n);
    const float dp_t_len = dp_t.length();
    // part of velocity update pass

    if (dp_t_len < mu_s * dist)
    {
        frict.x = dp_t.x;
        frict.y = dp_t.y;
    }
}

// calculated static & dynamic friction if not using velocity update pass
// the way Unified Particle Physics describes it in 6.1
static void calcFriction(const Vector2 & dp, const Vector2 & n, const float dist, Vector2 & frict)
{
    if (use_velocity_pass)
    {
        calcStaticFriction(dp, n, dist, frict);
        return;
    }

    Vector2 dp_n;
    Vector2 dp_t;

    // perp projection
    const float proj = dp.dot(n);
    dp_n.add(n, proj);

    dp_t.subtractVectors(dp, dp_n);
    const float dp_t_len = dp_t.length();
    // part of velocity update pass

    if (dp_t_len < mu_s * dist)
    {
        frict.x = dp_t.x;
        frict.y = dp_t.y;
    }
    else
    {
        // 0/0 = NaN, so avoid it
        const float k = mu_k == 0 ? 0.f : fminf(mu_k * dist / dp_t_len, 1.f);
        frict.x = k * dp_t.x;
        frict.y = k * dp_t.y;
    }
}

// ------------------------------------------------

static void solveBoundaryConstraint(const bool is_stab)
{
    //var br = physicsScene.boundaryRadius - particleRadius;
    //var bc = physicsScene.boundaryCenter;

    Vector2 n;
    Vector2 pi;
    Vector2 dp;
    Vector2 dp_n;
    Vector2 dp_t;
    Vector2 path;
    Vector2 frict;
    
    for (int i = 0; i < physicsScene.numParticles; ++i)
    {
        pi.x = particles.pos[2*i    ];
        pi.y = particles.pos[2*i + 1];

        for (int j = 0; j < physicsScene.boundaries.size(); ++j)
        {
            const bool inv = physicsScene.boundaries[j].inverted;
            
            const float br = physicsScene.boundaries[j].radius - (inv ? particleRadius : -particleRadius);
            const Vector2 & bc = physicsScene.boundaries[j].pos;

            n.subtractVectors(pi, bc);
            const float d = n.length();
            n.scale(1.f / d);
            float C = d - br;
            if (inv)
            {
                n.invert();
                C = -C;
            }

            if (C < 0.f)
            {
                dp.x = 0.f;
                dp.y = 0.f;
                dp.add(n, -C);
                
                const float dist = fabsf(C);

                particles.pos[2 * i    ] += dp.x;
                particles.pos[2 * i + 1] += dp.y;

                // corrected pos minus old pos
                path.x = (particles.pos[2 * i    ] - particles.prev[2 * i    ]);
                path.y = (particles.pos[2 * i + 1] - particles.prev[2 * i + 1]);

                calcFriction(path, n, dist, frict);

                if (use_velocity_pass)
                {
                    const float dvx = particles.vel[2 * i    ] - 0.f;
                    const float dvy = particles.vel[2 * i + 1] - 0.f;
                    const float d_lambda_n = dist; //path.dot(n);
                    
                    constraints.pushBack(i, -1, dvx, dvy, n.x, n.y, d_lambda_n, mu_k, e);
                }

                particles.pos[2 * i    ] -= frict.x;
                particles.pos[2 * i + 1] -= frict.y;

                if (is_stab)
                {
                    particles.prev[2 * i    ] += dp.x - frict.x;
                    particles.prev[2 * i + 1] += dp.y - frict.y;
                }
            }
        }
    }
}

static void solveCollisionConstraints(const bool is_stab)
{
    Vector2 frict;
    Vector2 path;
    Vector2 n;
    
    for (int i = 0; i < physicsScene.numParticles; ++i)
    {
        const float px = particles.pos[2 * i    ];
        const float py = particles.pos[2 * i + 1];

        const int first = firstNeighbor[i];
        const int num = firstNeighbor[i + 1] - first;

        for (int j = 0; j < num; j++)
        {
            const int id = neighbors.vals[first + j];
            
            if (id == i)
                continue;

            n.x = px - particles.pos[2 * id    ];
            n.y = py - particles.pos[2 * id + 1];
            
            const float d = n.length();

            if (d > 0.f)
            {
                n.scale(1.f/d);
            }

            const float C = 2.f*particleRadius - d;
            
            if (C > 0.f)
            {
                // particles have same mass, so  w1/(w1+w2) = w2/(w1+w2) = 0.5

                const float dx = .5f * C * n.x;
                const float dy = .5f * C * n.y;

                particles.pos[2 * i    ] += dx;
                particles.pos[2 * i + 1] += dy;

                particles.pos[2 * id    ] -= dx;
                particles.pos[2 * id + 1] -= dy;

                // corrected pos minus old pos
                path.x =
					(particles.pos[2 * i  + 0] - particles.prev[2 * i  + 0]) -
					(particles.pos[2 * id + 0] - particles.prev[2 * id + 0]);
                path.y =
					(particles.pos[2 * i  + 1] - particles.prev[2 * i  + 1]) -
					(particles.pos[2 * id + 1] - particles.prev[2 * id + 1]);

                const float dist = C;
                
                frict.x = frict.y = 0.f;
                calcFriction(path, n, dist, frict);
                
                particles.pos[2 * i  + 0] -= .5f * frict.x;
                particles.pos[2 * i  + 1] -= .5f * frict.y;
                particles.pos[2 * id + 0] += .5f * frict.x;
                particles.pos[2 * id + 1] += .5f * frict.y;

                if (is_stab)
                {
                    particles.prev[2 * i  + 0] += +dx - .5f * frict.x;
                    particles.prev[2 * i  + 1] += +dy - .5f * frict.y;
                    particles.prev[2 * id + 0] += -dx + .5f * frict.x;
                    particles.prev[2 * id + 1] += -dy + .5f * frict.y;
                }

                if (use_velocity_pass)
                {
                    // fill constraint for velocity pass
                    const float dvx = particles.vel[2 * i    ] - particles.vel[2 * id    ];
                    const float dvy = particles.vel[2 * i + 1] - particles.vel[2 * id + 1];
                    const float d_lambda_n = .5f * path.dot(n);
                    constraints.pushBack(i, id, dvx, dvy, n.x, n.y, d_lambda_n, mu_k, e);
                }
            }
        }
    }

}

// ------------------------------------------------

const float very_small_float = 1e-6f;

static void velocityUpdate(const float h)
{
    //var mu_k;
    //var e;
    Vector2 v;
    Vector2 vprev;
    Vector2 n;
    Vector2 dv;
    Vector2 vt;
    
    for (int i = 0; i < constraints.size; i++)
    {
        const int idA = constraints.idxA(i);
        const int idB = constraints.idxB(i);

        // relative velocity
        v.x = particles.vel[2*idA + 0];
        v.y = particles.vel[2*idA + 1];
        v.x -= idB == -1 ? 0.f : particles.vel[2*idB + 0];
        v.y -= idB == -1 ? 0.f : particles.vel[2*idB + 1];

        // previous relative velocity
        vprev.x = constraints.dvx(i);
        vprev.y = constraints.dvy(i);

        n.x = constraints.nx(i);
        n.y = constraints.ny(i);
        const float d_lambda_n = constraints.d_lambda_n(i);

        const float vn = n.dot(v);
        vt.x = v.x - vn * n.x;
        vt.y = v.y - vn * n.y;

		// friction force (dynamic friction)
		const float vt_len = vt.length();
		const float fn = fabsf(d_lambda_n / (h * h));
		
		// Eq. 30
        if (vt_len > very_small_float)
        {
            dv.x = -(vt.x / vt_len) * fminf(h * mu_k * fn, vt_len);
            dv.y = -(vt.y / vt_len) * fminf(h * mu_k * fn, vt_len);
        }
        else
        {
           dv.x = dv.y = 0.f;
        }

		// restitution, Eq. 34
		const float v_prev_n = vprev.dot(n);
        // !NB: original paper has: min() but probably it is an error
		float restitution_x = n.x * (-vn + fmaxf(-e * v_prev_n, 0.f));
		float restitution_y = n.y * (-vn + fmaxf(-e * v_prev_n, 0.f));

		// now, apply delta velocity
		// NOTE: probably should first apply dv and then calculate restitution_dv and
		// apply it again?
		const float w1 = 1.f;
		const float w2 = idB == -1 ? 0.f : 1.f;

		particles.vel[2*idA + 0] += (dv.x + restitution_x) * w1 / (w1 + w2);
		particles.vel[2*idA + 1] += (dv.y + restitution_y) * w1 / (w1 + w2);
		
		if (idB != -1)
		{
			restitution_x = n.x * (-vn + fmaxf(-e * v_prev_n, 0.f));
			restitution_y = n.y * (-vn + fmaxf(-e * v_prev_n, 0.f));
			
		    particles.vel[2*idB + 0] -= (dv.x + restitution_x) * w2 / (w1 + w2);
		    particles.vel[2*idB + 1] -= (dv.y + restitution_y) * w2 / (w1 + w2);
		}
    }

}

// ------------------------------------------------

static void simulate()
{
    if (physicsScene.paused)
        return;

    const float h = physicsScene.dt / physicsScene.numSteps;
    const Vector2 g = physicsScene.gravity;
    Vector2 vi;
    Vector2 pi;
    Vector2 pprevi;

    const float delta_angle = 120.f * physicsScene.dt;
    const float smallBoundaryRadius = .25f * physicsScene.boundaryRadius;
    Vector2 bpos;
    const Mat2 rot_m = Mat2::rotation(delta_angle * float(M_PI/180.0));
    
    for(int i = 1; i < physicsScene.boundaries.size(); ++i)
    {
        bpos = physicsScene.boundaries[i].pos;
        bpos.x -= physicsScene.boundaryCenter.x;
        bpos.y -= physicsScene.boundaryCenter.y;
        bpos.rotate(rot_m);
        bpos.x += physicsScene.boundaryCenter.x;
        bpos.y += physicsScene.boundaryCenter.y;
        physicsScene.boundaries[i].pos = bpos;
    }

    constraints.clear();
    findNeighbors();

    for (int step = 0; step < physicsScene.numSteps; step++)
    {
        constraints.clear();
        
        // predict
        for (int i = 0; i < physicsScene.numParticles; i++)
        {
            // use temp var to not overwrite correct velocities
            // we need them when filling constraint structure
            
            const float vx = particles.vel[2 * i + 0] + g.x * h;
            const float vy = particles.vel[2 * i + 1] + g.y * h;
            
            particles.prev[2 * i] = particles.pos[2 * i];
            particles.prev[2 * i + 1] = particles.pos[2 * i + 1];
            
            particles.pos[2 * i] += vx * h;
            particles.pos[2 * i + 1] += vy * h;
        }

        bool is_stab = false;
        
        if (step < 5)
           is_stab = true;

        solveCollisionConstraints(is_stab);
        solveBoundaryConstraint(is_stab);

        //force = Math.abs(lambda / sdt / sdt);

        // update velocities
        for (int i = 0; i < physicsScene.numParticles; i++)
        {
            pi.x = particles.pos[2*i + 0];
            pi.y = particles.pos[2*i + 1];

            pprevi.x = particles.prev[2*i + 0];
            pprevi.y = particles.prev[2*i + 1];

            vi.subtractVectors(pi, pprevi);
            vi.scale(1.f / h);
            
		#if PHYS_TODO
            if(Number.isNaN(vi.x) || Number.isNaN(vi.y)) {
                //alert("Nan");
                vi.x = 0;
            }
		#endif

            particles.vel[2 * i + 0] = vi.x;
            particles.vel[2 * i + 1] = vi.y;
        }

        velocityUpdate(h);
    }
}

// --------------------------------------------------------

static void setupScene()
{
    const float particleDiameter = 2.f * particleRadius;

    if (physicsScene.numColumns*physicsScene.numRows > maxParticles)
    {
        logError("Too many particles, please increase maxPariticles value");
        return;
    }

    physicsScene.radius = simMinWidth * .05f;
    physicsScene.paused = true;
    
    physicsScene.boundaryCenter.x = simWidth / 2.f;
    physicsScene.boundaryCenter.y = simHeight / 2.f;
    physicsScene.boundaryRadius   = simMinWidth * .4f;

    const float offsetX = physicsScene.boundaryCenter.x - physicsScene.numColumns * particleDiameter * .5f;
    const float offsetY = physicsScene.boundaryCenter.y + physicsScene.numRows    * particleDiameter * .5f;

    //offsetX += particleRadius * 50;
    //offsetY -= particleRadius * 50;

    // init particle positions
    
    int idx = 0;
    
    for (int y = 0; y < physicsScene.numRows; y++)
    {
        for (int x = 0; x < physicsScene.numColumns; x++)
        {
            // x
            particles.pos[idx] = offsetX + x * 1.2f * particleDiameter;
            particles.pos[idx] += .01f * particleDiameter * (y % 2);
            
            // y
            particles.pos[idx+ 1] = offsetY - y * particleDiameter;

            // v.x v.y
            particles.vel[idx    ] = 0.f;
            particles.vel[idx + 1] = 0.f;
            
            idx += 2;
        }
    }
    
    physicsScene.numParticles = physicsScene.numColumns * physicsScene.numRows;

    // add some more boundaries
    physicsScene.boundaries.push_back(CircleBoundary(physicsScene.boundaryCenter, physicsScene.boundaryRadius, true));

    const int num_boundaries = 5;
    const float delta_angle = 360.f / num_boundaries;
    const float smallBoundaryRadius = .25f * physicsScene.boundaryRadius;
    
    Vector2 bpos;
    
    for (int i = 0; i < num_boundaries; ++i)
    {
        const Mat2 rot_m = Mat2::rotation(delta_angle * i * float(M_PI / 180.0));
        
        bpos.x = physicsScene.boundaryRadius;
        bpos.y = 0.f;
        bpos.rotate(rot_m);
        
        bpos.x += physicsScene.boundaryCenter.x;
        bpos.y += physicsScene.boundaryCenter.y;
        
        physicsScene.boundaries.push_back(CircleBoundary(bpos, smallBoundaryRadius, false));
    }

    initNeighborsHash();
    constraints.clear();

#if PHYS_TODO
    doc.getElementById("mu_s_slider").setAttribute("value", 100*mu_s);
    doc.getElementById("mu_k_slider").setAttribute("value", 100*mu_k);
    doc.getElementById("e_slider").setAttribute("value", 100*e);
    doc.getElementById("mu_k").innerHTML = mu_k;
    doc.getElementById("mu_s").innerHTML = mu_s;
    doc.getElementById("e").innerHTML = e;
    doc.getElementById("use_velocity_pass").checked = use_velocity_pass;
    doc.getElementById("cols").innerHTML = physicsScene.numColumns;
    doc.getElementById("rows").innerHTML = physicsScene.numRows;
#endif
}

// --------------------------------------------------------

static void run()
{
    physicsScene.paused = false;
}

static void step()
{
    physicsScene.paused = false;
    simulate();
    physicsScene.paused = true;
}

static void update()
{
	simulate();
}

#if PHYS_TODO

		document.getElementById("mu_s_slider").oninput = function() {
			mu_s = this->value / 100;
			document.getElementById("mu_s").innerHTML = mu_s;
		}
		document.getElementById("mu_k_slider").oninput = function() {
			mu_k = this->value / 100;
			document.getElementById("mu_k").innerHTML = mu_k;
		}
		document.getElementById("e_slider").oninput = function() {
			e = this->value / 100;
			document.getElementById("e").innerHTML = e;
		}
		document.getElementById("use_velocity_pass").onclick = function() {
			use_velocity_pass = this->checked;
		}
		document.getElementById("cols").onchange = function() {
			physicsScene.numColumns = this->value;
			setupScene();
		}
		document.getElementById("rows").onchange = function() {
			physicsScene.numRows = this->value;
			setupScene();
		}
#endif

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	setupGlobals();

	setupScene();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_SPACE))
			run();
			
		if (keyboard.wentDown(SDLK_s))
			step();
			
		update();
		
		framework.beginDraw(100, 100, 100, 0);
		{
			draw();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
