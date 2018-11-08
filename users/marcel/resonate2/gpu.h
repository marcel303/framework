#pragma

#include <string>

namespace cl
{
	class Buffer;
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
	std::string buildOptions;
	
	GpuProgram(cl::Device & in_device, cl::Context & in_context, const char * buildOptions);
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

struct GpuBuffer
{
	GpuContext * context = nullptr;
	
	cl::Buffer * buffer = nullptr;
	int size = 0;
	void * data = nullptr;
	bool readOnly = true;
	std::string desc;
	
	GpuBuffer() { }
	GpuBuffer(const bool readOnly, GpuContext * context, void * data, const int size, const bool sendToGpu, const char * desc);
	~GpuBuffer();
	
	void initReadWrite(GpuContext * context, void * data, const int size, const bool sendToGpu, const char * desc);
	void initReadOnly(GpuContext * context, void * data, const int size, const bool sendToGpu, const char * desc);
	
	bool sendToGpu();
	bool fetchFromGpu();
};
