#include "cl.hpp"

struct GpuContext;
struct GpuProgram;
struct ImpulseResponseProbe;
struct Lattice;

struct GpuSimulationContext
{
	GpuContext & gpuContext;
	
	Lattice * lattice = nullptr;
	int numVertices = 0;
	int numEdges = 0;
	
	ImpulseResponseProbe * probes = nullptr;
	int numProbes = 0;
	
	cl::Buffer * vertexBuffer = nullptr;
	cl::Buffer * edgeBuffer = nullptr;
	cl::Buffer * cosSinBuffer = nullptr;
	cl::Buffer * impulseResponseProbesBuffer = nullptr;
	
	GpuProgram * computeEdgeForcesProgram = nullptr;
	GpuProgram * integrateProgram = nullptr;
	GpuProgram * integrateImpulseResponseProgram = nullptr;
	
	GpuSimulationContext(GpuContext & in_gpuContext);
	~GpuSimulationContext();
	
	bool init(Lattice & in_lattice, ImpulseResponseProbe * probes, const int numProbes);
	bool shut();
	
	bool sendVerticesToGpu();
	bool fetchVerticesFromGpu();

	bool sendEdgesToGpu();
	
	bool sendImpulseReponsesToGpu();
	bool fetchImpulseResponsesFromGpu();
	
	bool computeEdgeForces(const float tension);
	bool integrate(Lattice & lattice, const float dt, const float falloff);
	bool integrateImpulseResponse(const float dt);
};
