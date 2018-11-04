#include "Debugging.h"
#include "gpu.h"
#include "gpuSimulationContext.h"
#include "gpuSources.h"
#include "lattice.h"
#include "Log.h"
#include <math.h>

GpuSimulationContext::GpuSimulationContext(GpuContext & in_gpuContext)
	: gpuContext(in_gpuContext)
{
}

GpuSimulationContext::~GpuSimulationContext()
{
	Assert(vertexBuffer == nullptr);
	Assert(integrateProgram == nullptr);
}

bool GpuSimulationContext::init(Lattice & in_lattice)
{
	if (gpuContext.isValid() == false)
	{
		return false;
	}
	else
	{
		lattice = &in_lattice;
		numVertices = 6 * kTextureSize * kTextureSize;
		numEdges = in_lattice.edges.size();
		
		vertexBuffer = new cl::Buffer(*gpuContext.context, CL_MEM_READ_WRITE, sizeof(Lattice::Vertex) * numVertices);
		
		edgeBuffer = new cl::Buffer(*gpuContext.context, CL_MEM_READ_ONLY, sizeof(Lattice::Edge) * numEdges);
		
		//
		
		computeEdgeForcesProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
		
		if (computeEdgeForcesProgram->updateSource(computeEdgeForces_source) == false)
			return false;
		
		//
		
		integrateProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
		
		if (integrateProgram->updateSource(integrate_source) == false)
			return false;
		
		sendEdgesToGpu();
		sendVerticesToGpu();
		
		return true;
	}
}

bool GpuSimulationContext::shut()
{
	delete integrateProgram;
	integrateProgram = nullptr;
	
	delete vertexBuffer;
	vertexBuffer = nullptr;
	
	return true;
}

bool GpuSimulationContext::sendVerticesToGpu()
{
	// send the vertex data to the GPU
	
	if (gpuContext.commandQueue->enqueueWriteBuffer(
		*vertexBuffer,
		CL_TRUE,
		0, sizeof(Lattice::Vertex) * numVertices,
		lattice->vertices) != CL_SUCCESS)
	{
		LOG_ERR("failed to send vertex data to the GPU", 0);
		return false;
	}
	
	return true;
}

bool GpuSimulationContext::fetchVerticesFromGpu()
{
	// fetch the vertex data from the GPU
	
	if (gpuContext.commandQueue->enqueueReadBuffer(
		*vertexBuffer,
		CL_TRUE,
		0, sizeof(Lattice::Vertex) * numVertices,
		lattice->vertices) != CL_SUCCESS)
	{
		LOG_ERR("failed to fetch vertices from the GPU", 0);
		return false;
	}
	
	return true;
}

bool GpuSimulationContext::sendEdgesToGpu()
{
	// send the edge data to the GPU
	
	if (gpuContext.commandQueue->enqueueWriteBuffer(
		*edgeBuffer,
		CL_TRUE,
		0, sizeof(Lattice::Edge) * numEdges,
		&lattice->edges[0]) != CL_SUCCESS)
	{
		LOG_ERR("failed to send edge data to the GPU", 0);
		return false;
	}
	
	return true;
}

bool GpuSimulationContext::computeEdgeForces(const float tension)
{
	//Benchmark bm("computeEdgeForces_GPU");
	
	// run the integration program on the GPU
	
	cl::Kernel kernel(*computeEdgeForcesProgram->program, "computeEdgeForces");
	
	if (kernel.setArg(0, *edgeBuffer) != CL_SUCCESS ||
		kernel.setArg(1, *vertexBuffer) != CL_SUCCESS ||
		kernel.setArg(2, tension) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(numEdges & ~63)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->enqueueBarrierWithWaitList();
	
	return true;
}

bool GpuSimulationContext::integrate(Lattice & lattice, const float dt, const float falloff)
{
	//Benchmark bm("simulateLattice_Integrate_GPU");
	
	const int numVertices = 6 * kTextureSize * kTextureSize;
	
	const float retain = powf(1.f - falloff, dt);
	
	// run the integration program on the GPU
	
	cl::Kernel integrateKernel(*integrateProgram->program, "integrate");
	
	if (integrateKernel.setArg(0, *vertexBuffer) != CL_SUCCESS ||
		integrateKernel.setArg(1, dt) != CL_SUCCESS ||
		integrateKernel.setArg(2, retain) != CL_SUCCESS)
	{
		LOG_ERR("failed to set buffer arguments for kernel", 0);
		return false;
	}
	
	if (gpuContext.commandQueue->enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(numVertices), cl::NDRange(64)) != CL_SUCCESS)
	{
		LOG_ERR("failed to enqueue kernel", 0);
		return false;
	}
	
	gpuContext.commandQueue->enqueueBarrierWithWaitList();
	
	return true;
}
