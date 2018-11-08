#include "constants.h"
#include "Debugging.h"
#include "gpu.h"
#include "gpuSimulationContext.h"
#include "gpuSources.h"
#include "impulseResponse.h"
#include "lattice.h"
#include "Log.h"
#include <math.h>

#include "cl.hpp"

struct LatticeVector : Lattice::Vector
{
};

GpuSimulationContext::GpuSimulationContext(GpuContext & in_gpuContext)
	: gpuContext(in_gpuContext)
{
}

GpuSimulationContext::~GpuSimulationContext()
{
	Assert(vertex_p_Buffer == nullptr);
	Assert(vertex_p_init_Buffer == nullptr);
	Assert(vertex_n_Buffer == nullptr);
	Assert(vertex_f_Buffer == nullptr);
	Assert(vertex_v_Buffer == nullptr);
	Assert(edge_vertices_Buffer == nullptr);
	Assert(edgeBuffer == nullptr);
	Assert(edge_f_Buffer == nullptr);
	
	Assert(edgeForces == nullptr);

	Assert(computeEdgeForcesProgram == nullptr);
	Assert(integrateProgram == nullptr);
	Assert(integrateImpulseResponseProgram == nullptr);
	Assert(advanceImpulseResponseProgram == nullptr);
}

bool GpuSimulationContext::init(Lattice & in_lattice, ImpulseResponseState * in_impulseResponseState, ImpulseResponseProbe * in_probes, const int in_numProbes)
{
	if (gpuContext.isValid() == false)
	{
		return false;
	}
	else
	{
		lattice = &in_lattice;
		numVertices = kNumVertices;
		numEdges = in_lattice.edges.size();

		edgeForces = new LatticeVector[numEdges];
		memset(edgeForces, 0, sizeof(LatticeVector) * numEdges);

		typedef int32_t GatherType;
		GatherType * gatherMap = new GatherType[numVertices * 8];
		memset(gatherMap, -1, sizeof(GatherType) * numVertices * 8);
		for (int e = 0; e < numEdges; ++e)
		{
			GatherType * vertex1 = gatherMap + in_lattice.edgeVertices[e].vertex1 * 8;
			GatherType * vertex2 = gatherMap + in_lattice.edgeVertices[e].vertex2 * 8;

			for (int i = 0; i < 8; ++i)
			{
				if (vertex1[i] == -1)
				{
					vertex1[i] = e;
					break;
				}

				Assert(i != 7);
			}

			for (int i = 0; i < 8; ++i)
			{
				if (vertex2[i] == -1)
				{
					vertex2[i] = -2 - e;
					break;
				}

				Assert(i != 7);
			}
		}
		
#if 0
		for (int i = 0; i < numVertices * 8; ++i)
		{
			printf("gather: %04d : %04d\n", i / 8, gatherMap[i]);
		}
#endif

		vertex_p_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_p, sizeof(Lattice::Vector) * numVertices, true, "vertex p data");
		vertex_p_init_Buffer = new GpuBuffer(true, &gpuContext, in_lattice.vertices_p_init, sizeof(Lattice::Vector) * numVertices, true, "vertex p_init data");
		vertex_n_Buffer = new GpuBuffer(true, &gpuContext, in_lattice.vertices_n, sizeof(Lattice::Vector) * numVertices, true, "vertex n data");
		vertex_f_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_f, sizeof(Lattice::Vector) * numVertices, true, "vertex f data");
		vertex_v_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_v, sizeof(Lattice::Vector) * numVertices, true, "vertex v data");
		
		edge_vertices_Buffer = new GpuBuffer(true, &gpuContext, &in_lattice.edgeVertices[0], sizeof(Lattice::EdgeVertices) * numEdges, true, "edge vertices");
		edgeBuffer = new GpuBuffer(true, &gpuContext, &in_lattice.edges[0], sizeof(Lattice::Edge) * numEdges, true, "edge data");
		edge_f_Buffer = new GpuBuffer(false, &gpuContext, &edgeForces[0], sizeof(LatticeVector) * numEdges, true, "edge forces");
		edge_f_gather_Buffer = new GpuBuffer(true, &gpuContext, gatherMap, sizeof(GatherType) * numVertices * 8, true, "edge forces gather map");

		delete [] gatherMap;
		gatherMap = nullptr;

		//
		
		impulseResponseState = in_impulseResponseState;
		
		probes = in_probes;
		numProbes = in_numProbes;
		
		impulseResponseStateBuffer = new GpuBuffer();
		impulseResponseStateBuffer->initReadWrite(&gpuContext, impulseResponseState, sizeof(ImpulseResponseState), true, "impulse response state");
		
		impulseResponseProbesBuffer = new GpuBuffer();
		impulseResponseProbesBuffer->initReadWrite(&gpuContext, probes, sizeof(ImpulseResponseProbe) * numProbes, true, "impulse response probes");
		
		//
		
		char buildOptions[1024];
		sprintf(buildOptions, "-DkNumProbeFrequencies=%d", kNumProbeFrequencies);
		
		//
		
		computeEdgeForcesProgram = new GpuProgram(*gpuContext.device, *gpuContext.context, buildOptions);
		
		if (computeEdgeForcesProgram->updateSource(computeEdgeForces_source) == false)
			return false;
		
		computeEdgeForcesKernel = new cl::Kernel(*computeEdgeForcesProgram->program, "computeEdgeForces");

		//
		
		gatherEdgeForcesProgram = new GpuProgram(*gpuContext.device, *gpuContext.context, buildOptions);

		if (gatherEdgeForcesProgram->updateSource(gatherEdgeForces_source) == false)
			return false;

		gatherEdgeForcesKernel = new cl::Kernel(*gatherEdgeForcesProgram->program, "gatherEdgeForces");

		//

		integrateProgram = new GpuProgram(*gpuContext.device, *gpuContext.context, buildOptions);
		
		if (integrateProgram->updateSource(integrate_source) == false)
			return false;

		integrateKernel = new cl::Kernel(*integrateProgram->program, "integrate");

		//

		integrateImpulseResponseProgram = new GpuProgram(*gpuContext.device, *gpuContext.context, buildOptions);

		if (integrateImpulseResponseProgram->updateSource(integrateImpulseResponse_source) == false)
			return false;

		integrateImpulseResponseKernel = new cl::Kernel(*integrateImpulseResponseProgram->program, "integrateImpulseResponse");

		//

		advanceImpulseResponseProgram = new GpuProgram(*gpuContext.device, *gpuContext.context, buildOptions);

		if (advanceImpulseResponseProgram->updateSource(advanceImpulseResponse_source) == false)
			return false;

		advanceImpulseResponseKernel = new cl::Kernel(*advanceImpulseResponseProgram->program, "advanceImpulseResponse");

		return true;
	}
}

bool GpuSimulationContext::shut()
{
	delete advanceImpulseResponseKernel;
	advanceImpulseResponseKernel = nullptr;

	if (advanceImpulseResponseProgram != nullptr)
	{
		advanceImpulseResponseProgram->shut();

		delete advanceImpulseResponseProgram;
		advanceImpulseResponseProgram = nullptr;
	}

	delete integrateImpulseResponseKernel;
	integrateImpulseResponseKernel = nullptr;

	if (integrateImpulseResponseProgram != nullptr)
	{
		integrateImpulseResponseProgram->shut();
		
		delete integrateImpulseResponseProgram;
		integrateImpulseResponseProgram = nullptr;
	}
	
	delete integrateKernel;
	integrateKernel = nullptr;

	if (integrateProgram != nullptr)
	{
		integrateProgram->shut();
		
		delete integrateProgram;
		integrateProgram = nullptr;
	}
	
	delete gatherEdgeForcesKernel;
	gatherEdgeForcesKernel = nullptr;

	if (gatherEdgeForcesProgram != nullptr)
	{
		gatherEdgeForcesProgram->shut();

		delete gatherEdgeForcesProgram;
		gatherEdgeForcesProgram = nullptr;
	}

	delete computeEdgeForcesKernel;
	computeEdgeForcesKernel = nullptr;

	if (computeEdgeForcesProgram != nullptr)
	{
		computeEdgeForcesProgram->shut();
		
		delete computeEdgeForcesProgram;
		computeEdgeForcesProgram = nullptr;
	}
	
	delete impulseResponseProbesBuffer;
	impulseResponseProbesBuffer = nullptr;
	
	delete impulseResponseStateBuffer;
	impulseResponseStateBuffer = nullptr;
	
	delete edge_f_Buffer;
	edge_f_Buffer = nullptr;

	delete edgeBuffer;
	edgeBuffer = nullptr;

	delete edge_vertices_Buffer;
	edge_vertices_Buffer = nullptr;
	
	delete vertex_v_Buffer;
	vertex_v_Buffer = nullptr;
	
	delete vertex_f_Buffer;
	vertex_f_Buffer = nullptr;
	
	delete vertex_n_Buffer;
	vertex_n_Buffer = nullptr;
	
	delete vertex_p_init_Buffer;
	vertex_p_init_Buffer = nullptr;
	
	delete vertex_p_Buffer;
	vertex_p_Buffer = nullptr;

	delete [] edgeForces;
	edgeForces = nullptr;
	
	return true;
}

bool GpuSimulationContext::sendVerticesToGpu()
{
	return
		vertex_p_Buffer->sendToGpu() &&
		vertex_p_init_Buffer->sendToGpu() &&
		vertex_n_Buffer->sendToGpu() &&
		vertex_f_Buffer->sendToGpu() &&
		vertex_v_Buffer->sendToGpu();
}

bool GpuSimulationContext::fetchVerticesFromGpu()
{
	return
		vertex_p_Buffer->fetchFromGpu() &&
		vertex_p_init_Buffer->fetchFromGpu() &&
		vertex_n_Buffer->fetchFromGpu() &&
		vertex_f_Buffer->fetchFromGpu() &&
		vertex_v_Buffer->fetchFromGpu();
}

bool GpuSimulationContext::sendEdgesToGpu()
{
	return
		edge_vertices_Buffer->sendToGpu() &&
		edgeBuffer->sendToGpu();
}

bool GpuSimulationContext::sendImpulseResponseStateToGpu()
{
	return impulseResponseStateBuffer->sendToGpu();
}

bool GpuSimulationContext::fetchImpulseResponseStateFromGpu()
{
	return impulseResponseStateBuffer->fetchFromGpu();
}

bool GpuSimulationContext::sendImpulseResponseProbesToGpu()
{
	return impulseResponseProbesBuffer->sendToGpu();
}

bool GpuSimulationContext::fetchImpulseResponseProbesFromGpu()
{
	return impulseResponseProbesBuffer->fetchFromGpu();
}

bool GpuSimulationContext::computeEdgeForces(const float tension)
{
	//Benchmark bm("computeEdgeForces_GPU");
	
	// run the integration program on the GPU
	
	cl::Kernel & kernel = *computeEdgeForcesKernel;
	
	if (kernel.setArg(0, *edge_vertices_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, *edgeBuffer->buffer) != CL_SUCCESS ||
		kernel.setArg(2, *vertex_p_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(3, *vertex_f_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(4, tension) != CL_SUCCESS ||
		kernel.setArg(5, *edge_f_Buffer->buffer) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(numEdges/* & ~63*/)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->flush();
	
	//gpuContext.commandQueue->enqueueBarrierWithWaitList();
	
	return true;
}

bool GpuSimulationContext::gatherEdgeForces()
{
	//Benchmark bm("gatherEdgeForces_GPU");

	cl::Kernel & kernel = *gatherEdgeForcesKernel;

	if (kernel.setArg(0, *edge_f_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, *edge_f_gather_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(2, *vertex_f_Buffer->buffer) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}

	if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(numVertices)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}

	gpuContext.commandQueue->flush();

	//gpuContext.commandQueue->enqueueBarrierWithWaitList();

	return true;
}

bool GpuSimulationContext::integrate(Lattice & lattice, const float dt, const float falloff)
{
	//Benchmark bm("simulateLattice_Integrate_GPU");
	
	const int numVertices = kNumVertices;
	
	const float retain = powf(1.f - falloff, dt / 1000.f);
	
	// run the integration program on the GPU
	
	cl::Kernel & kernel = *integrateKernel;
	
	if (kernel.setArg(0, *vertex_p_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, *vertex_f_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(2, *vertex_v_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(3, dt) != CL_SUCCESS ||
		kernel.setArg(4, retain) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(numVertices)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->flush();
	
	//gpuContext.commandQueue->enqueueBarrierWithWaitList();
	
	return true;
}

bool GpuSimulationContext::integrateImpulseResponse(const float dt)
{
	cl::Kernel & integrateKernel = *integrateImpulseResponseKernel;
	
	if (integrateKernel.setArg(0, *vertex_p_Buffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(1, *vertex_p_init_Buffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(2, *impulseResponseStateBuffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(3, *impulseResponseProbesBuffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(4, dt) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(numProbes), cl::NDRange(64)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->flush();
	
	//gpuContext.commandQueue->enqueueBarrierWithWaitList();
	
	return true;
}

bool GpuSimulationContext::advanceImpulseState(const float dt)
{
	cl::Kernel & kernel = *advanceImpulseResponseKernel;

	if (kernel.setArg(0, *impulseResponseStateBuffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, dt) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}

	if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(kNumProbeFrequencies)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->flush();

	return true;
}
