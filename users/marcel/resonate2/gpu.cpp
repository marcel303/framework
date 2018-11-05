#include "Debugging.h"
#include "gpu.h"
#include "Log.h"

GpuProgram::GpuProgram(cl::Device & in_device, cl::Context & in_context)
	: device(in_device)
	, context(in_context)
{
}

GpuProgram::~GpuProgram()
{
	Assert(program == nullptr);
	
	shut();
}

void GpuProgram::shut()
{
	delete program;
	program = nullptr;
}

bool GpuProgram::updateSource(const char * in_source)
{
	cl::Program::Sources sources;
	
	sources.push_back({ in_source, strlen(in_source) });

	cl::Program newProgram(context, sources);
	
	if (newProgram.build({ device },
		""
		"-cl-single-precision-constant "
		"-cl-denorms-are-zero "
		"-cl-strict-aliasing "
		"-cl-mad-enable "
		"-cl-no-signed-zeros "
		"-cl-unsafe-math-optimizations "
		"-cl-finite-math-only "
		) != CL_SUCCESS)
	{
		LOG_ERR("failed to build OpenCL program", 0);
		LOG_ERR("%s", newProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str());
		
		return false;
	}
	
	delete program;
	program = nullptr;
	
	program = new cl::Program(newProgram);
	
	source = in_source;
	
	return true;
}

//

GpuContext::~GpuContext()
{
	Assert(device == nullptr);
	Assert(context == nullptr);
	Assert(commandQueue == nullptr);
	
	shut();
}

bool GpuContext::init()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	
	if (platforms.empty())
	{
		LOG_INF("no OpenCL platform(s) found", 0);
		return false;
	}
	
	for (auto & platform : platforms)
	{
		LOG_INF("available OpenCL platform: %s", platform.getInfo<CL_PLATFORM_NAME>().c_str());
	}
	
	const cl::Platform & defaultPlatform = platforms[0];
	
	std::vector<cl::Device> devices;
	
	defaultPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	
	if (devices.empty())
	{
		LOG_ERR("no OpenGL GPU device(s) found", 0);
		return false;
	}
	
	for (auto & device : devices)
	{
		LOG_INF("available GPU device: %s", device.getInfo<CL_DEVICE_NAME>().c_str());
	}
	
	device = new cl::Device(devices[0]);
	
	LOG_INF("using GPU device: %s", device->getInfo<CL_DEVICE_NAME>().c_str());
	
	context = new cl::Context(*device);
	
	// create a command queue
	
	commandQueue = new cl::CommandQueue(*context, *device, CL_QUEUE_PROFILING_ENABLE * 0);
	
	return true;
}

void GpuContext::shut()
{
	delete commandQueue;
	commandQueue = nullptr;
	
	delete context;
	context = nullptr;
	
	delete device;
	device = nullptr;
}

bool GpuContext::isValid() const
{
	return
		device != nullptr &&
		context != nullptr &&
		commandQueue != nullptr;
}
