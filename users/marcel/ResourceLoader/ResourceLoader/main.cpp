#include <algorithm>
#include <assert.h>

typedef bool (*ResourceDataHandler)(void * obj, int state, void *& dst, size_t & dstSize);

class ResourceLoader
{
public:
	const static int kStateDone = -1;
	
	void Begin(ResourceDataHandler handler, void * obj)
	{
		m_handler = handler;
		m_obj = obj;
		m_state = -1;
		m_dst = 0;
		m_todo = 0;
		
		NextState();
	}
	
	void HandleData(void * data, size_t byteCount)
	{
		while (!IsDone() && (byteCount != 0))
		{
			size_t copySize = std::min(byteCount, m_todo);
			
			memcpy(m_dst, data, copySize);
			
			m_dst += copySize;
			m_todo -= copySize;
			byteCount -= copySize;
			
			if (m_todo == 0)
			{
				NextState();
			}
		}
	}
	
	bool IsDone() const
	{
		return m_state == kStateDone;
	}
	
private:
	void NextState()
	{
		m_state++;
		
		void * dst;
		size_t size;
		
		if (m_handler(m_obj, m_state, dst, size))
		{
			assert((size == 0) || (dst != 0));
			
			m_dst = (unsigned char*)dst;
			m_todo = size;
		}
		else
		{
			m_state = kStateDone;
		}
	}

	ResourceDataHandler m_handler;
	void * m_obj;
	int m_state;
	unsigned char * m_dst;
	size_t m_todo;
};

class Mesh
{
public:
	enum LoadState
	{
		kLoadHeader,
		kLoadVertexBuffer,
		kLoadIndexBuffer
	};
	
	class Header
	{
	public:
		unsigned short vertexBufferSize;
		unsigned short indexBufferSize;
	};
	
	Mesh()
		: m_vertexBuffer(0)
		, m_indexBuffer(0)
	{
	}
	
	~Mesh()
	{
		free(m_vertexBuffer);
		m_vertexBuffer = 0;
		
		free(m_indexBuffer);
		m_indexBuffer = 0;
	}
	
	Header m_header;
	void * m_vertexBuffer;
	void * m_indexBuffer;
	
	static bool HandleResourceData(void * obj, int state, void *& dst, size_t & dstSize)
	{
		Mesh * self = (Mesh*)obj;
		
		switch (state)
		{
			case kLoadHeader:
				dst = &self->m_header;
				dstSize = sizeof(self->m_header);
				return true;
				
			case kLoadVertexBuffer:
				self->m_vertexBuffer = malloc(self->m_header.vertexBufferSize);
				dst = self->m_vertexBuffer;
				dstSize = self->m_header.vertexBufferSize;
				return true;
				
			case kLoadIndexBuffer:
				self->m_indexBuffer = malloc(self->m_header.indexBufferSize);
				dst = self->m_indexBuffer;
				dstSize = self->m_header.indexBufferSize;
				return true;
				
			default:
				return false;
		}
	}
};

//

extern void TestAllocationSystem();

int main (int argc, const char * argv[])
{
	TestAllocationSystem();
	
	//
	
	unsigned char * data = new unsigned char[4096];
	unsigned int value = 0;
	
	for (unsigned int i = 0; i < 1000*1000; ++i)
	{
		Mesh mesh;
		
		ResourceLoader loader;
		
		loader.Begin(Mesh::HandleResourceData, &mesh);
		
		while (!loader.IsDone())
		{
			unsigned int sizes[] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 5, 7, 11, 13, 17, 23, 1024 };
			
			unsigned int size = sizes[rand() % sizeof(sizes) / sizeof(unsigned int)];
			
			for (unsigned int j = 0; j < size; ++j)
			{
				value = (value + 1) * 13;
				
				data[j] = value;
			}
			
			loader.HandleData(data, size);
		}
		
		//if ((i % 1000) == 0)
		{
			printf("loaded %u\n", i);
		}
	}
	
	delete [] data;
	data = 0;
}





