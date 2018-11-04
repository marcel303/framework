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
	global Edge * restrict edges,
	global Vertex * restrict vertices,
	float tension)
{
	const float eps = 1e-4f;
	
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
	
	v.v.x += v.f.x * dt;
	v.v.y += v.f.y * dt;
	v.v.z += v.f.z * dt;
	
	v.p.x += v.v.x * dt;
	v.p.y += v.v.y * dt;
	v.p.z += v.v.z * dt;
	
	v.f.x = 0.0;
	v.f.y = 0.0;
	v.f.z = 0.0;
	
	vertices[ID] = v;
}
)SHADER";
