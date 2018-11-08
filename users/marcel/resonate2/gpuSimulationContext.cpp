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
	Assert(integrateProgram == nullptr);
	Assert(integrateImpulseResponseProgram == nullptr);
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
		
		vertex_p_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_p, sizeof(Lattice::Vector) * numVertices, true, "vertex p data");
		vertex_p_init_Buffer = new GpuBuffer(true, &gpuContext, in_lattice.vertices_p_init, sizeof(Lattice::Vector) * numVertices, true, "vertex p_init data");
		vertex_n_Buffer = new GpuBuffer(true, &gpuContext, in_lattice.vertices_n, sizeof(Lattice::Vector) * numVertices, true, "vertex n data");
		vertex_f_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_f, sizeof(Lattice::Vector) * numVertices, true, "vertex f data");
		vertex_v_Buffer = new GpuBuffer(false, &gpuContext, in_lattice.vertices_v, sizeof(Lattice::Vector) * numVertices, true, "vertex v data");
		
		edgeBuffer = new GpuBuffer(true, &gpuContext, &in_lattice.edges[0], sizeof(Lattice::Edge) * numEdges, true, "edge data");
		
		//
		
		impulseResponseState = in_impulseResponseState;
		
		probes = in_probes;
		numProbes = in_numProbes;
		
		impulseResponseStateBuffer = new GpuBuffer();
		impulseResponseStateBuffer->initReadWrite(&gpuContext, impulseResponseState, sizeof(ImpulseResponseState), true, "impulse response state");
		
		impulseResponseProbesBuffer = new GpuBuffer();
		impulseResponseProbesBuffer->initReadWrite(&gpuContext, probes, sizeof(ImpulseResponseProbe) * numProbes, true, "impulse response probes");
		
		//
		
		computeEdgeForcesProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
		
		if (computeEdgeForcesProgram->updateSource(computeEdgeForces_source) == false)
			return false;
		
		computeEdgeForcesKernel = new cl::Kernel(*computeEdgeForcesProgram->program, "computeEdgeForces");

		//
		
		integrateProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
		
		if (integrateProgram->updateSource(integrate_source) == false)
			return false;

		integrateKernel = new cl::Kernel(*integrateProgram->program, "integrate");

		//

		integrateImpulseResponseProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);

		if (integrateImpulseResponseProgram->updateSource(integrateImpulseResponse_source) == false)
			return false;

		integrateImpulseResponseKernel = new cl::Kernel(*integrateImpulseResponseProgram->program, "integrateImpulseResponse");

		//

		advanceImpulseResponseProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);

		if (advanceImpulseResponseProgram->updateSource(advanceImpulseResponse_source) == false)
			return false;

		advanceImpulseResponseKernel = new cl::Kernel(*advanceImpulseResponseProgram->program, "advanceImpulseResponse");

		return true;
	}
}

bool GpuSimulationContext::shut()
{
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
	
	delete edgeBuffer;
	edgeBuffer = nullptr;
	
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
	return edgeBuffer->sendToGpu();
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
	
	if (kernel.setArg(0, *edgeBuffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, *vertex_p_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(2, *vertex_f_Buffer->buffer) != CL_SUCCESS ||
		kernel.setArg(3, tension) != CL_SUCCESS)
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
