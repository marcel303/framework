#include "gltf.h"

namespace gltf
{
	Buffer::~Buffer()
	{
		free(data);
		data = nullptr;
	}
	
	//
	
	bool resolveBufferView(
		const Scene & scene,
		const int index,
		const gltf::Accessor *& out_accessor,
		const gltf::BufferView *& out_bufferView,
		const gltf::Buffer *& out_buffer)
	{
		if (index < 0 || index >= scene.accessors.size())
			return false;

		out_accessor = &scene.accessors[index];

		if (out_accessor->bufferView < 0 || out_accessor->bufferView >= scene.bufferViews.size())
			return false;

		out_bufferView = &scene.bufferViews[out_accessor->bufferView];

		if (out_bufferView->buffer < 0 || out_bufferView->buffer >= scene.buffers.size())
			return false;

		out_buffer = &scene.buffers[out_bufferView->buffer];
		
		return true;
	}
}
