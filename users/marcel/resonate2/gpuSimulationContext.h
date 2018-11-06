#pragma once

struct GpuContext;
struct GpuProgram;
struct ImpulseResponseProbe;
struct ImpulseResponseState;
struct Lattice;

namespace cl
{
	class Buffer;
	class CommandQueue;
	class Context;
	class Device;
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
	
	cl::Buffer * vertexBuffer = nullptr;
	cl::Buffer * edgeBuffer = nullptr;
	cl::Buffer * cosSinBuffer = nullptr;
	cl::Buffer * impulseResponseProbesBuffer = nullptr;
	
	GpuProgram * computeEdgeForcesProgram = nullptr;
	GpuProgram * integrateProgram = nullptr;
	GpuProgram * integrateImpulseResponseProgram = nullptr;
	
	GpuSimulationContext(GpuContext & in_gpuContext);
	~GpuSimulationContext();
	
	bool init(Lattice & in_lattice, ImpulseResponseState * impulseResponseState, ImpulseResponseProbe * probes, const int numProbes);
	bool shut();
	
	bool sendVerticesToGpu();
	bool fetchVerticesFromGpu();

	bool sendEdgesToGpu();
	
	bool sendImpulseResponseStateToGpu();
	bool sendImpulseResponseProbesToGpu();
	bool fetchImpulseResponseProbesFromGpu();
	
	bool computeEdgeForces(const float tension);
	bool integrate(Lattice & lattice, const float dt, const float falloff);
	bool integrateImpulseResponse(const float dt);
};
