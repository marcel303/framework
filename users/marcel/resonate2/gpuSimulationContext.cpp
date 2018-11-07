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
	Assert(vertexBuffer == nullptr);
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
		
		vertexBuffer = new GpuBuffer();
		vertexBuffer->initReadWrite(&gpuContext, in_lattice.vertices, sizeof(Lattice::Vertex) * numVertices, true, "vertex data");
		
		edgeBuffer = new GpuBuffer();
		edgeBuffer->initReadOnly(&gpuContext, &in_lattice.edges[0], sizeof(Lattice::Edge) * numEdges, true, "edge data");
		
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
	
	delete vertexBuffer;
	vertexBuffer = nullptr;
	
	return true;
}

bool GpuSimulationContext::sendVerticesToGpu()
{
	return vertexBuffer->sendToGpu();
}

bool GpuSimulationContext::fetchVerticesFromGpu()
{
	return vertexBuffer->fetchFromGpu();
}

bool GpuSimulationContext::sendEdgesToGpu()
{
	return edgeBuffer->sendToGpu();
}

bool GpuSimulationContext::sendImpulseResponseStateToGpu()
{
	return edgeBuffer->fetchFromGpu();
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
		kernel.setArg(1, *vertexBuffer->buffer) != CL_SUCCESS ||
		kernel.setArg(2, tension) != CL_SUCCESS)
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
	
	if (kernel.setArg(0, *vertexBuffer->buffer) != CL_SUCCESS ||
		kernel.setArg(1, dt) != CL_SUCCESS ||
		kernel.setArg(2, retain) != CL_SUCCESS)
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
	
	if (integrateKernel.setArg(0, *vertexBuffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(1, *impulseResponseStateBuffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(2, *impulseResponseProbesBuffer->buffer) != CL_SUCCESS ||
		integrateKernel.setArg(3, dt) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(numProbes, 8), cl::NDRange(8, 8)) != CL_SUCCESS)
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
