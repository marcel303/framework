#pragma

#include <string>

namespace cl
{
	class CommandQueue;
	class Context;
	class Device;
	class Program;
}

struct GpuProgram
{
	cl::Device & device;
	
	cl::Context & context;
	
	cl::Program * program = nullptr;
	
	std::string source;
	
	GpuProgram(cl::Device & in_device, cl::Context & in_context);
	~GpuProgram();
	
	void shut();
	
	bool updateSource(const char * in_source);
};

struct GpuContext
{
	cl::Device * device = nullptr;
	
	cl::Context * context = nullptr;
	
	cl::CommandQueue * commandQueue = nullptr;
	
	~GpuContext();
	
	bool init();
	void shut();
	
	bool isValid() const;
};
