#include "cl.hpp"

struct GpuContext;
struct GpuProgram;
struct Lattice;

struct GpuSimulationContext
{
	GpuContext & gpuContext;
	
	Lattice * lattice = nullptr;
	int numVertices = 0;
	int numEdges = 0;
	
	cl::Buffer * vertexBuffer = nullptr;
	cl::Buffer * edgeBuffer = nullptr;
	
	GpuProgram * computeEdgeForcesProgram = nullptr;
	GpuProgram * integrateProgram = nullptr;
	
	GpuSimulationContext(GpuContext & in_gpuContext);
	
	~GpuSimulationContext();
	
	bool init(Lattice & in_lattice);
	bool shut();
	
	bool sendVerticesToGpu();
	bool fetchVerticesFromGpu();

	bool sendEdgesToGpu();
	
	bool computeEdgeForces(const float tension);
	bool integrate(Lattice & lattice, const float dt, const float falloff);
};
