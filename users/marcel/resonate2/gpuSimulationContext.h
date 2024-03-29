#pragma once

struct GpuBuffer;
struct GpuContext;
struct GpuProgram;
struct ImpulseResponseProbe;
struct ImpulseResponseState;
struct Lattice;
struct LatticeVector;

namespace cl
{
	class Buffer;
	class CommandQueue;
	class Context;
	class Device;
	class Kernel;
	class Program;
}

struct GpuSimulationContext
{
	GpuContext & gpuContext;
	
	Lattice * lattice = nullptr;
	int numVertices = 0;
	int numEdges = 0;
	
	ImpulseResponseState * impulseResponseState = nullptr;
	ImpulseResponseProbe * probes = nullptr;
	int numProbes = 0;

	LatticeVector * edgeForces = nullptr;
	
	GpuBuffer * vertex_p_Buffer = nullptr;
	GpuBuffer * vertex_p_init_Buffer = nullptr;
	GpuBuffer * vertex_n_Buffer = nullptr;
	GpuBuffer * vertex_f_Buffer = nullptr;
	GpuBuffer * vertex_v_Buffer = nullptr;
	GpuBuffer * edge_vertices_Buffer = nullptr;
	GpuBuffer * edgeBuffer = nullptr;
	GpuBuffer * edge_f_Buffer = nullptr;
	GpuBuffer * edge_f_gather_Buffer = nullptr;
	GpuBuffer * impulseResponseStateBuffer = nullptr;
	GpuBuffer * impulseResponseProbesBuffer = nullptr;
	
	GpuProgram * computeEdgeForcesProgram = nullptr;
	GpuProgram * gatherEdgeForcesProgram = nullptr;
	GpuProgram * integrateProgram = nullptr;
	GpuProgram * integrateImpulseResponseProgram = nullptr;
	GpuProgram * advanceImpulseResponseProgram = nullptr;

	cl::Kernel * computeEdgeForcesKernel = nullptr;
	cl::Kernel * gatherEdgeForcesKernel = nullptr;
	cl::Kernel * integrateKernel = nullptr;
	cl::Kernel * integrateImpulseResponseKernel = nullptr;
	cl::Kernel * advanceImpulseResponseKernel = nullptr;
	
	GpuSimulationContext(GpuContext & in_gpuContext);
	~GpuSimulationContext();
	
	bool init(Lattice & in_lattice, ImpulseResponseState * impulseResponseState, ImpulseResponseProbe * probes, const int numProbes);
	bool shut();
	
	bool sendVerticesToGpu();
	bool fetchVerticesFromGpu();

	bool sendEdgesToGpu();
	
	bool sendImpulseResponseStateToGpu();
	bool fetchImpulseResponseStateFromGpu();
	bool sendImpulseResponseProbesToGpu();
	bool fetchImpulseResponseProbesFromGpu();
	
	bool computeEdgeForces(const float tension);
	bool gatherEdgeForces();
	bool integrate(Lattice & lattice, const float dt, const float falloff);
	bool integrateImpulseResponse(const float dt);
	bool advanceImpulseState(const float dt);
};
