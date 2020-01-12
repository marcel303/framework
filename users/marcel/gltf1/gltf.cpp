#include "gltf.h"

namespace gltf
{
	Buffer::~Buffer()
	{
		free(data);
		data = nullptr;
	}
	
	//
	
	bool resolveBufferView(const Scene & scene, const int index, const gltf::Accessor *& accessor, const gltf::BufferView *& bufferView, const gltf::Buffer *& buffer)
	{
		if (index < 0 || index >= scene.accessors.size())
			return false;

		accessor = &scene.accessors[index];

		if (accessor->bufferView < 0 || accessor->bufferView >= scene.bufferViews.size())
			return false;

		bufferView = &scene.bufferViews[accessor->bufferView];

		if (bufferView->buffer < 0 || bufferView->buffer >= scene.buffers.size())
			return false;

		buffer = &scene.buffers[bufferView->buffer];
		
		return true;
	}
}
