#pragma once

static const char * computeEdgeForces_source =
R"SHADER(
typedef struct Vector
{
	float x;
	float y;
	float z;
	float padding;
} Vector;

typedef struct Edge
{
	int vertex1;
	int vertex2;
	float weight;
	
	// physics stuff
	float initialDistance;
} Edge;

void atomicAdd_g_f(volatile __global float *addr, float val)
{
	union
	{
		unsigned int u32;
		float        f32;
	} next, expected, current;
	
	current.f32    = *addr;
	
	do
	{
		expected.f32 = current.f32;
		next.f32     = expected.f32 + val;
		current.u32  = atomic_cmpxchg(
			(volatile __global unsigned int *)addr,
			expected.u32, next.u32);
	} while( current.u32 != expected.u32 );
}

void kernel computeEdgeForces(
	global const Edge * restrict edges,
	global const Vector * restrict vertices_p,
	global Vector * restrict vertices_f,
	float tension)
{
	const float eps = 1e-12f;
	
	const int ID = get_global_id(0);
	
	const Edge edge = edges[ID];
	
	const Vector p1 = vertices_p[edge.vertex1];
	const Vector p2 = vertices_p[edge.vertex2];
	
	const float dx = p2.x - p1.x;
	const float dy = p2.y - p1.y;
	const float dz = p2.z - p1.z;
	
	const float distance = sqrt(dx * dx + dy * dy + dz * dz);
	
	if (distance < eps)
		return;
	
	const float distance_inverse = 1.f / distance;
	
	const float directionX = dx * distance_inverse;
	const float directionY = dy * distance_inverse;
	const float directionZ = dz * distance_inverse;
	
	const float force = (distance - edge.initialDistance) * edge.weight * tension;
	
	const float fx = directionX * force;
	const float fy = directionY * force;
	const float fz = directionZ * force;

	// let's kill some performance here by using atomics!
	// todo : store forces in edges and let vertices gather forces in a follow-up step
	
#if 1
	atomicAdd_g_f(&vertices_f[edge.vertex1].x, +fx);
	atomicAdd_g_f(&vertices_f[edge.vertex1].y, +fy);
	atomicAdd_g_f(&vertices_f[edge.vertex1].z, +fz);
	
	atomicAdd_g_f(&vertices_f[edge.vertex2].x, -fx);
	atomicAdd_g_f(&vertices_f[edge.vertex2].y, -fy);
	atomicAdd_g_f(&vertices_f[edge.vertex2].z, -fz);
#else
	vertices_f[edge.vertex1].x += fx;
	vertices_f[edge.vertex1].y += fy;
	vertices_f[edge.vertex1].z += fz;
	
	vertices_f[edge.vertex2].x -= fx;
	vertices_f[edge.vertex2].y -= fy;
	vertices_f[edge.vertex2].z -= fz;
#endif
}
)SHADER";

static const char * integrate_source =
R"SHADER(
typedef struct Vector
{
	float x;
	float y;
	float z;
	float padding;
} Vector;

void kernel integrate(
	global Vector * restrict vertices_p,
	global Vector * restrict vertices_f,
	global Vector * restrict vertices_v,
	float dt,
	float retain)
{
	int ID = get_global_id(0);
	
	Vector v_p = vertices_p[ID];
	Vector v_f = vertices_f[ID];
	Vector v_v = vertices_v[ID];
	
	v_v.x *= retain;
	v_v.y *= retain;
	v_v.z *= retain;
	
#if 1
	// constrain the vertex to the line emanating from (0, 0, 0) towards this vertex. this helps to stabalize
	// the simulation while at the same time making the resulting impulse-response more pleasing. it sort of
	// models a more rigid structure where vertices are held in place 'xy' on the plane they sit in through
	// the forces of the elements in the structure below them

	// todo : constrain vertices using the normal of the plane they sit in instead of the line to (0, 0, 0)

	float nx = v_p.x;
	float ny = v_p.y;
	float nz = v_p.z;
	const float ns = sqrt(nx * nx + ny * ny + nz * nz);
	nx /= ns;
	ny /= ns;
	nz /= ns;

	const float dot =
		nx * v_f.x +
		ny * v_f.y +
		nz * v_f.z;

	v_v.x += nx * dot * dt;
	v_v.y += ny * dot * dt;
	v_v.z += nz * dot * dt;
#else
	v_v.x += v_f.x * dt;
	v_v.y += v_f.y * dt;
	v_v.z += v_f.z * dt;
#endif
	
	v_p.x += v_v.x * dt;
	v_p.y += v_v.y * dt;
	v_p.z += v_v.z * dt;
	
	v_f.x = 0.0;
	v_f.y = 0.0;
	v_f.z = 0.0;
	
	vertices_p[ID] = v_p;
	vertices_f[ID] = v_f;
	vertices_v[ID] = v_v;
}
)SHADER";

static const char * integrateImpulseResponse_source =
R"SHADER(
typedef struct Vector
{
	float x;
	float y;
	float z;
	float padding;
} Vector;

#define kNumProbeFrequencies 128 // todo : must match declaration in cpp file!

typedef struct ImpulseResponseState
{
	float frequency[kNumProbeFrequencies];
	float phase[kNumProbeFrequencies];
	
	float cos_sin[kNumProbeFrequencies][2];
	
	float dt;
} ImpulseResponseState;

typedef struct ImpulseResponseProbe
{
	float response[kNumProbeFrequencies][2];
	
	int vertexIndex;
} ImpulseResponseProbe;

void kernel integrateImpulseResponse(
	global const Vector * restrict vertices_p,
	global const Vector * restrict vertices_p_init,
	constant const ImpulseResponseState * restrict state,
	global ImpulseResponseProbe * restrict probes,
	float dt)
{
	int ID = get_global_id(0);

	local float cos_sin[kNumProbeFrequencies][2];

    const int local_ID = get_local_id(0);

    const int i1 = local_ID * kNumProbeFrequencies / get_local_size(0);
    const int i2 = (local_ID + 1) * kNumProbeFrequencies / get_local_size(0);

    for (int i = i1; i < i2; ++i)
    {
        cos_sin[i][0] = state->cos_sin[i][0];
        cos_sin[i][1] = state->cos_sin[i][1];
    }

    barrier(CLK_LOCAL_MEM_FENCE);

	// measureValueAtVertex(..)
	
	global ImpulseResponseProbe * restrict probe = probes + ID;
	
	const Vector p = vertices_p[probe->vertexIndex];
	const Vector p_init = vertices_p_init[probe->vertexIndex];
	
	const float dx = p.x - p_init.x;
	const float dy = p.y - p_init.y;
	const float dz = p.z - p_init.z;

	const float value = sqrt(dx * dx + dy * dy + dz * dz);
	
	const float value_times_dt = value * dt;

	// measureValue(..)

	for (int i = 0; i < kNumProbeFrequencies; ++i)
	{
		probe->response[i][0] += cos_sin[i][0] * value_times_dt;
		probe->response[i][1] += cos_sin[i][1] * value_times_dt;
	}
}
)SHADER";

static const char * advanceImpulseResponse_source =
R"SHADER(

#define kNumProbeFrequencies 128 // todo : must match declaration in cpp file!

typedef struct ImpulseResponseState
{
	float frequency[kNumProbeFrequencies];
	float phase[kNumProbeFrequencies];
	
	float cos_sin[kNumProbeFrequencies][2];
	
	float dt;
} ImpulseResponseState;

void kernel advanceImpulseResponse(
	global ImpulseResponseState * restrict state,
	float dt)
{
	int ID = get_global_id(0);
	
	state->phase[ID] = fmod(state->phase[ID] + state->frequency[ID] * dt, 1.f);

	const float twoPi = 2.f * 3.14159265358979323846;

	state->cos_sin[ID][0] = cos(state->phase[ID] * twoPi);
	state->cos_sin[ID][1] = sin(state->phase[ID] * twoPi);
}
)SHADER";
