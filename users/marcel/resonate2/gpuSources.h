#pragma once

static const char * computeEdgeForces_source =
R"SHADER(
typedef struct Vector
{
	float x;
	float y;
	float z;
} Vector;

typedef struct Vertex
{
	Vector p;
	Vector p_init;
	Vector n;
	
	// physics stuff
	Vector f;
	Vector v;
} Vertex;

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
	global Vertex * restrict vertices,
	float tension)
{
	const float eps = 1e-12f;
	
	const int ID = get_global_id(0);
	
	const Edge edge = edges[ID];
	
	const Vector p1 = vertices[edge.vertex1].p;
	const Vector p2 = vertices[edge.vertex2].p;
	
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
	atomicAdd_g_f(&vertices[edge.vertex1].f.x, +fx);
	atomicAdd_g_f(&vertices[edge.vertex1].f.y, +fy);
	atomicAdd_g_f(&vertices[edge.vertex1].f.z, +fz);
	
	atomicAdd_g_f(&vertices[edge.vertex2].f.x, -fx);
	atomicAdd_g_f(&vertices[edge.vertex2].f.y, -fy);
	atomicAdd_g_f(&vertices[edge.vertex2].f.z, -fz);
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
} Vector;

typedef struct Vertex
{
	Vector p;
	Vector p_init;
	Vector n;
	
	// physics stuff
	Vector f;
	Vector v;
} Vertex;

void kernel integrate(
	global Vertex * restrict vertices,
	float dt,
	float retain)
{
	int ID = get_global_id(0);
	
	Vertex v = vertices[ID];
	
	v.v.x *= retain;
	v.v.y *= retain;
	v.v.z *= retain;
	
#if 1
	// constrain the vertex to the line emanating from (0, 0, 0) towards this vertex. this helps to stabalize
	// the simulation while at the same time making the resulting impulse-response more pleasing. it sort of
	// models a more rigid structure where vertices are held in place 'xy' on the plane they sit in through
	// the forces of the elements in the structure below them

	// todo : constrain vertices using the normal of the plane they sit in instead of the line to (0, 0, 0)

	float nx = v.p.x;
	float ny = v.p.y;
	float nz = v.p.z;
	const float ns = sqrt(nx * nx + ny * ny + nz * nz);
	nx /= ns;
	ny /= ns;
	nz /= ns;

	const float dot =
		nx * v.f.x +
		ny * v.f.y +
		nz * v.f.z;

	v.v.x += nx * dot * dt;
	v.v.y += ny * dot * dt;
	v.v.z += nz * dot * dt;
#else
	v.v.x += v.f.x * dt;
	v.v.y += v.f.y * dt;
	v.v.z += v.f.z * dt;
#endif
	
	v.p.x += v.v.x * dt;
	v.p.y += v.v.y * dt;
	v.p.z += v.v.z * dt;
	
	v.f.x = 0.0;
	v.f.y = 0.0;
	v.f.z = 0.0;
	
	vertices[ID] = v;
}
)SHADER";

static const char * integrateImpulseResponse_source =
R"SHADER(
typedef struct Vector
{
	float x;
	float y;
	float z;
} Vector;

typedef struct Vertex
{
	Vector p;
	Vector p_init;
	Vector n;
	
	// physics stuff
	Vector f;
	Vector v;
} Vertex;

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
	global const Vertex * restrict vertices,
	global const ImpulseResponseState * restrict state,
	global ImpulseResponseProbe * restrict probes,
	float dt)
{
	int ID = get_global_id(0);
	
	// measureValueAtVertex(..)
	
	global ImpulseResponseProbe * restrict probe = probes + ID;
	
	const Vertex vertex = vertices[probe->vertexIndex];
	
	const float dx = vertex.p.x - vertex.p_init.x;
	const float dy = vertex.p.y - vertex.p_init.y;
	const float dz = vertex.p.z - vertex.p_init.z;

	const float value = sqrt(dx * dx + dy * dy + dz * dz);
	
	const float value_times_dt = value * dt;

	// measureValue(..)

	for (int i = 0; i < kNumProbeFrequencies; ++i)
	{
		probe->response[i][0] += state->cos_sin[i][0] * value_times_dt;
		probe->response[i][1] += state->cos_sin[i][1] * value_times_dt;
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
	
	state->phase[ID] = fmod(state->phase[ID] + state->frequency[ID] * dt, 1.0);

	const float twoPi = 2.f * 3.14159265358979323846;

	state->cos_sin[ID][0] = cos(state->phase[ID] * twoPi);
	state->cos_sin[ID][1] = sin(state->phase[ID] * twoPi);
}
)SHADER";
