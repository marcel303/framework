#include "Benchmark.h"
#include "framework.h"
#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "nfd.h"
#include "Parse.h"
#include "StringEx.h"
#include "TextIO.h"

#define __CL_ENABLE_EXCEPTIONS

#include "cl.hpp"

/*

From: https://learnopengl.com/Advanced-OpenGL/Cubemaps

	GL_TEXTURE_CUBE_MAP_POSITIVE_X	Right
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X	Left
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y	Top
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	Bottom
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z	Back
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	Front
 
	See cubemaps_skybox.png in the documentation folder for reference.

*/

const int VIEW_SX = 1200;
const int VIEW_SY = 700;

const int kTextureSize = 64;
const int kTextureArraySize = 128;

extern void splitString(const std::string & str, std::vector<std::string> & result);

struct Texture
{
	float value[kTextureSize][kTextureSize];
};

struct CubeFace
{
	Texture textureArray[kTextureArraySize];
};

struct Cube
{
	CubeFace faces[6];
};

static void randomizeTexture(Texture & texture, const float min, const float max)
{
	for (int x = 0; x < kTextureSize; ++x)
	{
		for (int y = 0; y < kTextureSize; ++y)
		{
			texture.value[y][x] = random<float>(min, max);
		}
	}
}

static void randomizeCubeFace(CubeFace & cubeFace, const int firstTexture, const int numTextures, const float min, const float max)
{
	for (int i = 0; i < numTextures; ++i)
	{
		randomizeTexture(cubeFace.textureArray[firstTexture + i], min, max);
	}
}

static GLuint textureToGL(const Texture & texture)
{
	return createTextureFromR32F(texture.value, kTextureSize, kTextureSize, false, true);
}

static const GLenum s_cubeFaceNamesGL[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

static Mat4x4 createCubeFaceMatrix(const GLenum cubeFace)
{
	Mat4x4 mat;
	
	const float degToRad = M_PI / 180.f;
	
	switch (cubeFace)
	{
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: // Right
		mat.MakeRotationY(-90 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: // Left
		mat.MakeRotationY(+90 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: // Top
		mat.MakeRotationX(+90 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: // Bottom
		mat.MakeRotationX(-90 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: // Back
		mat.MakeRotationY(180 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: // Front
		mat.MakeIdentity();
		break;
		
	default:
		Assert(false);
		mat.MakeIdentity();
		break;
	}
	
	return mat;
}

static Mat4x4 s_cubeFaceToWorldMatrices[6];
static Mat4x4 s_worldToCubeFaceMatrices[6];

struct ShapeDefinition
{
	static const int kMaxPlanes = 64;
	
	struct Plane
	{
		Vec3 normal;
		float offset;
	};
	
	Plane planes[kMaxPlanes];
	int numPlanes;
	
	void loadFromFile(const char * filename)
	{
		numPlanes = 0;
		
		//
		
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
		
		if (TextIO::load(filename, lines, lineEndings) == false)
		{
			// todo : error
		}
		else
		{
			for (auto & line : lines)
			{
				std::vector<std::string> parts;
				
				splitString(line, parts);
				
				if (parts.empty())
					continue;
				
				if (parts[0] == "plane")
				{
					// plane definition
					
					if (numPlanes == kMaxPlanes)
					{
						// todo : error
					}
					else
					{
						auto & plane = planes[numPlanes++];
						
						if (parts.size() - 1 >= 4)
						{
							plane.normal[0] = Parse::Float(parts[1]);
							plane.normal[1] = Parse::Float(parts[2]);
							plane.normal[2] = Parse::Float(parts[3]);
							
							plane.offset = Parse::Float(parts[4]);
						}
					}
				}
			}
		}
	}
	
	void makeRandomShape(const int in_numPlanes)
	{
		Assert(in_numPlanes <= kMaxPlanes);
		numPlanes = in_numPlanes;
		
		for (int i = 0; i < numPlanes; ++i)
		{
			const float dx = random(-1.f, +1.f);
			const float dy = random(-1.f, +1.f);
			const float dz = random(-1.f, +1.f);
			
			const Vec3 normal = Vec3(dx, dy, dz).CalcNormalized();
			
			const float offset = .5f;
			
			planes[i].normal = normal;
			planes[i].offset = offset;
		}
	}
	
	float intersectRay_directional(Vec3Arg rayDirection) const
	{
		float t = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < numPlanes; ++i)
		{
			const float dd = planes[i].normal * rayDirection;
			
			if (dd > 0.f)
			{
				const float d1 = planes[i].offset;
				const float pt = d1 / dd;
				
				Assert(pt >= 0.f);
				
				if (pt < t)
					t = pt;
			}
		}
		
		return t;
	}
	
	float intersectRay(Vec3Arg rayOrigin, Vec3Arg rayDirection) const
	{
		float t = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < numPlanes; ++i)
		{
			const float dd = planes[i].normal * rayDirection;
			
			if (dd > 0.f)
			{
				const float d1 = planes[i].offset - planes[i].normal * rayOrigin;
				const float pt = d1 / dd;
				
				if (pt >= 0.f && pt < t)
					t = pt;
			}
		}
		
		return t;
	}
};

static void projectDirectionToCubeFace(Vec3Arg direction, int & cubeFaceIndex, Vec2 & cubeFacePosition)
{
	const float absDirection[3] =
	{
		fabsf(direction[0]),
		fabsf(direction[1]),
		fabsf(direction[2])
	};
	
	int majorAxis = 0;
	
	if (absDirection[1] > absDirection[majorAxis])
		majorAxis = 1;
	if (absDirection[2] > absDirection[majorAxis])
		majorAxis = 2;
	
	/*
	work out the cube face index given this list
	
	GL_TEXTURE_CUBE_MAP_POSITIVE_X
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	*/
	
	cubeFaceIndex = majorAxis * 2 + (direction[majorAxis] < 0 ? 1 : 0);
	
	const Mat4x4 & worldToCubeFaceMatrix = s_worldToCubeFaceMatrices[cubeFaceIndex];
	
	const Vec3 direction_cubeFace = worldToCubeFaceMatrix.Mul4(direction);
	
	cubeFacePosition[0] = direction_cubeFace[0] / direction_cubeFace[2];
	cubeFacePosition[1] = direction_cubeFace[1] / direction_cubeFace[2];
}

struct Lattice
{
	struct Vector
	{
		float x;
		float y;
		float z;
		
		void set(const float in_x, const float in_y, const float in_z)
		{
			x = in_x;
			y = in_y;
			z = in_z;
		}
		
		void setZero()
		{
			x = y = z = 0.f;
		}
	};
	
	struct Vertex
	{
		Vector p;
		
		// physics stuff
		Vector f;
		Vector v;
	};
	
	struct Edge
	{
		int vertex1;
		int vertex2;
		float weight;
		
		// physics stuff
		float initialDistance;
	};
	
	Vertex vertices[6 * kTextureSize * kTextureSize];
	
	std::vector<Edge> edges;
	
	void init()
	{
		for (int i = 0; i < 6; ++i)
		{
			const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
			
			for (int x = 0; x < kTextureSize; ++x)
			{
				for (int y = 0; y < kTextureSize; ++y)
				{
					const int index =
						i * kTextureSize * kTextureSize +
						y * kTextureSize +
						x;
					
					const float xf = ((x + .5f) / float(kTextureSize) - .5f) * 2.f;
					const float yf = ((y + .5f) / float(kTextureSize) - .5f) * 2.f;
		
					Vec3 p = matrix.Mul4(Vec3(xf, yf, 1.f));
					
					vertices[index].p.set(p[0], p[1], p[2]);
					
					vertices[index].f.setZero();
					vertices[index].v.setZero();
				}
			}
		}
		
		// setup up edges for face interiors
		
		auto addEdge = [&](const int faceIndex, const int x1, const int y1, const int x2, const int y2, const float weight)
		{
			const int index1 =
				faceIndex * kTextureSize * kTextureSize +
				y1 * kTextureSize +
				x1;
			
			const int index2 =
				faceIndex * kTextureSize * kTextureSize +
				y2 * kTextureSize +
				x2;
			
			Edge edge;
			edge.vertex1 = index1;
			edge.vertex2 = index2;
			edge.weight = weight;
			
			edges.push_back(edge);
		};
		
		for (int i = 0; i < 6; ++i)
		{
			for (int x = 0; x < kTextureSize; ++x)
			{
				for (int y = 0; y < kTextureSize; ++y)
				{
					if (x + 1 < kTextureSize)
						addEdge(i, x + 0, y + 0, x + 1, y + 0, 1.f);
					if (y + 1 < kTextureSize)
						addEdge(i, x + 0, y + 0, x + 0, y + 1, 1.f);
					
					if (x + 1 < kTextureSize && y + 1 < kTextureSize)
					{
						addEdge(i, x + 0, y + 0, x + 1, y + 1, 1.f / sqrtf(2.f));
						addEdge(i, x + 0, y + 1, x + 1, y + 0, 1.f / sqrtf(2.f));
					}
				}
			}
		}
		
		// setup edges between faces
		
		auto addEdge2 = [&](const int faceIndex1, const int x1, const int y1, const int faceIndex2, const int x2, const int y2, const float weight)
		{
			const int index1 =
				faceIndex1 * kTextureSize * kTextureSize +
				y1 * kTextureSize +
				x1;
			
			const int index2 =
				faceIndex2 * kTextureSize * kTextureSize +
				y2 * kTextureSize +
				x2;
			
			Edge edge;
			edge.vertex1 = index1;
			edge.vertex2 = index2;
			edge.weight = weight;
			
			edges.push_back(edge);
		};
		
		auto processSeam = [&](
			const int faceIndex1,
			const int face1_x1,
			const int face1_y1,
			const int face1_x2,
			const int face1_y2,
			const int faceIndex2,
			const int face2_x1,
			const int face2_y1,
			const int face2_x2,
			const int face2_y2)
		{
			const int face1_stepx = face1_x2 - face1_x1;
			const int face1_stepy = face1_y2 - face1_y1;
			int face1_x = face1_x1 * (kTextureSize - 1);
			int face1_y = face1_y1 * (kTextureSize - 1);
			
			const int face2_stepx = face2_x2 - face2_x1;
			const int face2_stepy = face2_y2 - face2_y1;
			int face2_x = face2_x1 * (kTextureSize - 1);
			int face2_y = face2_y1 * (kTextureSize - 1);
			
			for (int i = 0; i < kTextureSize; ++i)
			{
				Assert(face1_x >= 0 && face1_x < kTextureSize);
				Assert(face1_y >= 0 && face1_y < kTextureSize);
				Assert(face2_x >= 0 && face2_x < kTextureSize);
				Assert(face2_y >= 0 && face2_y < kTextureSize);
				
				addEdge2(faceIndex1, face1_x + face1_stepx * 0, face1_y + face1_stepy * 0, faceIndex2, face2_x + face2_stepx * 0, face2_y + face2_stepy * 0, 1.f);
				
				if (i + 1 < kTextureSize)
				{
					addEdge2(faceIndex1, face1_x + face1_stepx * 0, face1_y + face1_stepy * 0, faceIndex2, face2_x + face2_stepx * 1, face2_y + face2_stepy * 1, 1.f / sqrtf(2.f));
				
					addEdge2(faceIndex1, face1_x + face1_stepx * 1, face1_y + face1_stepy * 1, faceIndex2, face2_x + face2_stepx * 0, face2_y + face2_stepy * 0, 1.f / sqrtf(2.f));
				}
			
				face1_x += face1_stepx;
				face1_y += face1_stepy;
				face2_x += face2_stepx;
				face2_y += face2_stepy;
			}
		};
		
		// signed vertex coordinates for a face in cube face space
		const int face_sx[4] = { -1, +1, +1, -1 };
		const int face_sy[4] = { -1, -1, +1, +1 };
		
		// unsigned vertex coordinates for a face in cube face space
		const int face_ux[4] = { 0, 1, 1, 0 };
		const int face_uy[4] = { 0, 0, 1, 1 };
		
		// transform signed face vertex positions into world space, so we can come vertices and edges of our faces in world space
		struct CubeFace
		{
			Vec3 transformedPosition[4];
		};
		
		CubeFace cubeFaces[6];
		
		for (int i = 0; i < 6; ++i)
		{
			const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
			
			for (int v = 0; v < 4; ++v)
			{
				cubeFaces[i].transformedPosition[v] = matrix.Mul4(Vec3(face_sx[v], face_sy[v], 1.f));
				
			#if 0
				printf("transformed position: (%.2f, %.2f, %.2f)\n",
					cubeFaces[i].transformedPosition[v][0],
					cubeFaces[i].transformedPosition[v][1],
					cubeFaces[i].transformedPosition[v][2]);
			#endif
			}
		}
		
		// loop over all of the faces and all of their edges and try to find a second face which shared the same edge (meaning : sharing two vertices with equal world positions)
		
		int numEdges = 0;
		
		for (int f1 = 0; f1 < 6; ++f1)
		{
			for (int f1_v1 = 0; f1_v1 < 4; ++f1_v1)
			{
				const int f1_v2 = (f1_v1 + 1) % 4;
				
				const Vec3 & f1_p1 = cubeFaces[f1].transformedPosition[f1_v1];
				const Vec3 & f1_p2 = cubeFaces[f1].transformedPosition[f1_v2];
				
				for (int f2 = f1 + 1; f2 < 6; ++f2)
				{
					for (int f2_v1 = 0; f2_v1 < 4; ++f2_v1)
					{
						const int f2_v2 = (f2_v1 + 1) % 4;
						
						const Vec3 & f2_p1 = cubeFaces[f2].transformedPosition[f2_v1];
						const Vec3 & f2_p2 = cubeFaces[f2].transformedPosition[f2_v2];
						
						const bool pass1 = ((f2_p1 - f1_p1).CalcSize() <= .05f && (f2_p2 - f1_p2).CalcSize() <= .05f);
						const bool pass2 = ((f2_p1 - f1_p2).CalcSize() <= .05f && (f2_p2 - f1_p1).CalcSize() <= .05f);
						
						// the cases for pass1 and pass2 are different only in the respect the face space x/y's for the second face are swapped
						
						if (pass1)
						{
							numEdges++;
							
							processSeam(
								f1, face_ux[f1_v1], face_uy[f1_v1], face_ux[f1_v2], face_uy[f1_v2],
								f2, face_ux[f2_v1], face_uy[f2_v1], face_ux[f2_v2], face_uy[f2_v2]);
						}
						else if (pass2)
						{
							numEdges++;
							
							processSeam(
								f1, face_ux[f1_v1], face_uy[f1_v1], face_ux[f1_v2], face_uy[f1_v2],
								f2, face_ux[f2_v2], face_uy[f2_v2], face_ux[f2_v1], face_uy[f2_v1]);
						}
					}
				}
			}
		}
		
		//printf("found %d edges, %d valid edges!\n", numEdges, numValidEdges);
		
		finalize();
	}
	
	void finalize()
	{
		for (auto & edge : edges)
		{
			const auto & p1 = vertices[edge.vertex1].p;
			const auto & p2 = vertices[edge.vertex2].p;
			
			const float dx = p2.x - p1.x;
			const float dy = p2.y - p1.y;
			const float dz = p2.z - p1.z;
			
			const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
			
			edge.initialDistance = distance;
		}
	}
};

static void projectLatticeOntoShape(Lattice & lattice, const ShapeDefinition & shape)
{
	const int numVertices = 6 * kTextureSize * kTextureSize;

	for (int i = 0; i < numVertices; ++i)
	{
		auto & p = lattice.vertices[i].p;
		
		const float t = shape.intersectRay_directional(Vec3(p.x, p.y, p.z));
		
		lattice.vertices[i].p.set(
			p.x * t,
			p.y * t,
			p.z * t);
		
		lattice.vertices[i].f.setZero();
		lattice.vertices[i].v.setZero();
	}
	
	lattice.finalize();
}

static void drawLatticeVertices(const Lattice & lattice)
{
	gxBegin(GL_POINTS);
	{
		const int numVertices = 6 * kTextureSize * kTextureSize;
		
		for (int i = 0; i < numVertices; ++i)
		{
			auto & p = lattice.vertices[i].p;
			
			gxVertex3f(p.x, p.y, p.z);
		}
	}
	gxEnd();
}

static void drawLatticeEdges(const Lattice & lattice)
{
	gxBegin(GL_LINES);
	{
		for (auto & edge : lattice.edges)
		{
			const auto & p1 = lattice.vertices[edge.vertex1].p;
			const auto & p2 = lattice.vertices[edge.vertex2].p;
			
			gxVertex3f(p1.x, p1.y, p1.z);
			gxVertex3f(p2.x, p2.y, p2.z);
		}
	}
	gxEnd();
}

//

struct GpuProgram
{
	cl::Device & device;
	
	cl::Context & context;
	
	cl::Program * program = nullptr;
	
	GpuProgram(cl::Device & in_device, cl::Context & in_context)
		: device(in_device)
		, context(in_context)
	{
	}
	
	~GpuProgram()
	{
		Assert(program == nullptr);
		
		shut();
	}
	
	void shut()
	{
		delete program;
		program = nullptr;
	}
	
	bool updateSource(const char * source)
	{
		cl::Program::Sources sources;
		
		sources.push_back({ source, strlen(source) });
	
		cl::Program newProgram(context, sources);
		
		if (newProgram.build({ device },
			"-cl-single-precision-constant "
			"-cl-denorms-are-zero "
			"-cl-strict-aliasing "
			"-cl-mad-enable "
			"-cl-no-signed-zeros "
			"-cl-unsafe-math-optimizations "
			"-cl-finite-math-only ") != CL_SUCCESS)
		{
			logError("failed to build OpenCL program");
			logError("%s", newProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str());
			
			return false;
		}
		
		delete program;
		program = nullptr;
		
		program = new cl::Program(newProgram);
		
		return true;
	}
};

struct GpuContext
{
	cl::Device * device = nullptr;
	
	cl::Context * context = nullptr;
	
	cl::CommandQueue * commandQueue = nullptr;
	
	~GpuContext()
	{
		Assert(device == nullptr);
		Assert(context == nullptr);
		Assert(commandQueue == nullptr);
		
		shut();
	}
	
	bool init()
	{
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		
		if (platforms.empty())
		{
			logInfo("no OpenCL platform(s) found");
			return false;
		}
		
		for (auto & platform : platforms)
		{
			logInfo("available OpenCL platform: %s", platform.getInfo<CL_PLATFORM_NAME>().c_str());
		}
		
		const cl::Platform & defaultPlatform = platforms[0];
		
		std::vector<cl::Device> devices;
		
		defaultPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
		
		if (devices.empty())
		{
			logError("no OpenGL GPU device(s) found");
			return false;
		}
		
		for (auto & device : devices)
		{
			logInfo("available GPU device: %s", device.getInfo<CL_DEVICE_NAME>().c_str());
		}
		
		device = new cl::Device(devices[0]);
		
		logInfo("using GPU device: %s", device->getInfo<CL_DEVICE_NAME>().c_str());
		
		context = new cl::Context(*device);
		
		// create a command queue
		
		commandQueue = new cl::CommandQueue(*context, *device, CL_QUEUE_PROFILING_ENABLE * 0);
		
		return true;
	}
	
	void shut()
	{
		delete commandQueue;
		commandQueue = nullptr;
		
		delete context;
		context = nullptr;
		
		delete device;
		device = nullptr;
	}
	
	bool isValid() const
	{
		return
			device != nullptr &&
			context != nullptr &&
			commandQueue != nullptr;
	}
};

struct GpuSimulationContext
{
	GpuContext & gpuContext;
	
	Lattice * lattice = nullptr;
	int numVertices = 0;
	int numEdges = 0;
	
	cl::Buffer * vertexBuffer = nullptr;
	cl::Buffer * edgeBuffer = nullptr;
	
	GpuProgram * computeEdgeForcesProgram = nullptr;
	GpuProgram * integrateProgram = nullptr;
	
	GpuSimulationContext(GpuContext & in_gpuContext)
		: gpuContext(in_gpuContext)
	{
	}
	
	~GpuSimulationContext()
	{
		Assert(vertexBuffer == nullptr);
		Assert(integrateProgram == nullptr);
	}
	
	bool init(Lattice & in_lattice)
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
			
			const char  *computeEdgeForces_source =
				R"SHADER(
			
				typedef struct Vector
				{
					float x;
					float y;
					float z;
				} Vector;
				
				typedef struct Vertex
				{
					Vector p;
					
					// physics stuff
					Vector f;
					Vector v;
				} Vertex;
			
				typedef struct Edge
				{
					int vertex1;
					int vertex2;
					float weight;
					
					// physics stuff
					float initialDistance;
				} Edge;
			
				void atomicAdd_g_f(volatile __global float *addr, float val)
				{
					union
					{
						unsigned int u32;
						float        f32;
					} next, expected, current;
					
					current.f32    = *addr;
					
					do
					{
						expected.f32 = current.f32;
						next.f32     = expected.f32 + val;
						current.u32  = atomic_cmpxchg(
							(volatile __global unsigned int *)addr,
							expected.u32, next.u32);
					} while( current.u32 != expected.u32 );
				}
			
				void kernel computeEdgeForces(
					global Edge * restrict edges,
					global Vertex * restrict vertices,
					float tension)
				{
					const float eps = 1e-4f;
					
					const int ID = get_global_id(0);
					
					const Edge edge = edges[ID];
					
					const Vector p1 = vertices[edge.vertex1].p;
					const Vector p2 = vertices[edge.vertex2].p;
					
					const float dx = p2.x - p1.x;
					const float dy = p2.y - p1.y;
					const float dz = p2.z - p1.z;
					
					const float distance = sqrt(dx * dx + dy * dy + dz * dz);
					
					if (distance < eps)
						return;
					
					const float distance_inverse = 1.f / distance;
					
					const float directionX = dx * distance_inverse;
					const float directionY = dy * distance_inverse;
					const float directionZ = dz * distance_inverse;
					
					const float force = (distance - edge.initialDistance) * edge.weight * tension;
					
					const float fx = directionX * force;
					const float fy = directionY * force;
					const float fz = directionZ * force;
				
					// let's kill some performance here by using atomics!
					// todo : store forces in edges and let vertices gather forces in a follow-up step
					
				#if 1
					atomicAdd_g_f(&vertices[edge.vertex1].f.x, +fx);
					atomicAdd_g_f(&vertices[edge.vertex1].f.y, +fy);
					atomicAdd_g_f(&vertices[edge.vertex1].f.z, +fz);
					
					atomicAdd_g_f(&vertices[edge.vertex2].f.x, -fx);
					atomicAdd_g_f(&vertices[edge.vertex2].f.y, -fy);
					atomicAdd_g_f(&vertices[edge.vertex2].f.z, -fz);
				#endif
				}
			
				)SHADER";
			
			computeEdgeForcesProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
			
			if (computeEdgeForcesProgram->updateSource(computeEdgeForces_source) == false)
				return false;
			
			//
			
			const char * integrate_source =
				R"SHADER(
			
				typedef struct Vector
				{
					float x;
					float y;
					float z;
				} Vector;
			
				typedef struct Vertex
				{
					Vector p;
					
					// physics stuff
					Vector f;
					Vector v;
				} Vertex;
			
				void kernel integrate(
					global Vertex * restrict vertices,
					float dt,
					float retain)
				{
					int ID = get_global_id(0);
					
					Vertex v = vertices[ID];
					
					v.v.x *= retain;
					v.v.y *= retain;
					v.v.z *= retain;
					
					v.v.x += v.f.x * dt;
					v.v.y += v.f.y * dt;
					v.v.z += v.f.z * dt;
					
					v.p.x += v.v.x * dt;
					v.p.y += v.v.y * dt;
					v.p.z += v.v.z * dt;
					
					v.f.x = 0.0;
					v.f.y = 0.0;
					v.f.z = 0.0;
					
					vertices[ID] = v;
				}
			
				)SHADER";
			
			integrateProgram = new GpuProgram(*gpuContext.device, *gpuContext.context);
			
			if (integrateProgram->updateSource(integrate_source) == false)
				return false;
			
			sendEdgesToGpu();
			sendVerticesToGpu();
			
			return true;
		}
	}
	
	bool shut()
	{
		delete integrateProgram;
		integrateProgram = nullptr;
		
		delete vertexBuffer;
		vertexBuffer = nullptr;
		
		return true;
	}
	
	bool sendVerticesToGpu()
	{
		// send the vertex data to the GPU
		
		if (gpuContext.commandQueue->enqueueWriteBuffer(
			*vertexBuffer,
			CL_TRUE,
			0, sizeof(Lattice::Vertex) * numVertices,
			lattice->vertices) != CL_SUCCESS)
		{
			logError("failed to send vertex data to the GPU");
			return false;
		}
		
		return true;
	}
	
	bool fetchVerticesFromGpu()
	{
		// fetch the vertex data from the GPU
		
		if (gpuContext.commandQueue->enqueueReadBuffer(
			*vertexBuffer,
			CL_TRUE,
			0, sizeof(Lattice::Vertex) * numVertices,
			lattice->vertices) != CL_SUCCESS)
		{
			logError("failed to fetch vertices from the GPU");
			return false;
		}
		
		return true;
	}
	
	bool sendEdgesToGpu()
	{
		// send the edge data to the GPU
		
		if (gpuContext.commandQueue->enqueueWriteBuffer(
			*edgeBuffer,
			CL_TRUE,
			0, sizeof(Lattice::Edge) * numEdges,
			&lattice->edges[0]) != CL_SUCCESS)
		{
			logError("failed to send edge data to the GPU");
			return false;
		}
		
		return true;
	}
	
	bool computeEdgeForces(const float tension)
	{
		//Benchmark bm("computeEdgeForces_GPU");
		
		// run the integration program on the GPU
		
		cl::Kernel kernel(*computeEdgeForcesProgram->program, "computeEdgeForces");
		
		if (kernel.setArg(0, *edgeBuffer) != CL_SUCCESS ||
			kernel.setArg(1, *vertexBuffer) != CL_SUCCESS ||
			kernel.setArg(2, tension) != CL_SUCCESS)
		{
			logError("failed to set buffer arguments for kernel");
			return false;
		}
		
		if (gpuContext.commandQueue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(numEdges & ~63)) != CL_SUCCESS)
		{
			logError("failed to enqueue kernel");
			return false;
		}
		
		gpuContext.commandQueue->enqueueBarrierWithWaitList();
		
		return true;
	}
	
	bool integrate(Lattice & lattice, const float dt, const float falloff)
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
			logError("failed to set buffer arguments for kernel");
			return false;
		}
		
		if (gpuContext.commandQueue->enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(numVertices), cl::NDRange(64)) != CL_SUCCESS)
		{
			logError("failed to enqueue kernel");
			return false;
		}
		
		gpuContext.commandQueue->enqueueBarrierWithWaitList();
		
		return true;
	}
};

static GpuContext * s_gpuContext = nullptr;

static GpuSimulationContext * s_gpuSimulationContext = nullptr;

static bool gpuInit(Lattice & lattice)
{
	s_gpuContext = new GpuContext();
	
	if (!s_gpuContext->init())
		return false;
	
	s_gpuSimulationContext = new GpuSimulationContext(*s_gpuContext);
	
	if (!s_gpuSimulationContext->init(lattice))
		return false;

	return true;
}

bool gpuShut()
{
	if (s_gpuContext != nullptr)
	{
		s_gpuContext->shut();
		
		delete s_gpuContext;
		s_gpuContext = nullptr;
	}
	
	return true;
}

void simulateLattice_computeEdgeForces(Lattice & lattice, const float tension)
{
	//Benchmark bm("computeEdgeForces");
	
	const float eps = 1e-4f;
	
	for (auto & edge : lattice.edges)
	{
		auto & v1 = lattice.vertices[edge.vertex1];
		auto & v2 = lattice.vertices[edge.vertex2];
		
		const float dx = v2.p.x - v1.p.x;
		const float dy = v2.p.y - v1.p.y;
		const float dz = v2.p.z - v1.p.z;
		
		const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
		
		if (distance < eps)
			continue;
		
		const float distance_inverse = 1.f / distance;
		
		const float directionX = dx * distance_inverse;
		const float directionY = dy * distance_inverse;
		const float directionZ = dz * distance_inverse;
		
		const float force = (distance - edge.initialDistance) * edge.weight * tension;
		
		const float fx = directionX * force;
		const float fy = directionY * force;
		const float fz = directionZ * force;
		
		v1.f.x += fx;
		v1.f.y += fy;
		v1.f.z += fz;
		
		v2.f.x -= fx;
		v2.f.y -= fy;
		v2.f.z -= fz;
	}
}

static void simulateLattice_integrate(Lattice & lattice, const float dt, const float falloff)
{
	//Benchmark bm("simulateLattice_Integrate");
	
	const int numVertices = 6 * kTextureSize * kTextureSize;
	
	const float retain = powf(1.f - falloff, dt);
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & v = lattice.vertices[i];
		
		v.v.x *= retain;
		v.v.y *= retain;
		v.v.z *= retain;
		
		v.v.x += v.f.x * dt;
		v.v.y += v.f.y * dt;
		v.v.z += v.f.z * dt;
		
		v.p.x += v.v.x * dt;
		v.p.y += v.v.y * dt;
		v.p.z += v.v.z * dt;
		
		v.f.setZero();
	}
}

static void simulateLattice(Lattice & lattice, const float dt, const float tension, const float falloff)
{
	simulateLattice_computeEdgeForces(lattice, tension);
	
	simulateLattice_integrate(lattice, dt, falloff);
}

static void simulateLattice_gpu(Lattice & lattice, const float dt, const float tension, const float falloff)
{
	s_gpuSimulationContext->computeEdgeForces(tension);
	
	s_gpuSimulationContext->integrate(lattice, dt, falloff);
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.enableDepthBuffer = true;

	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	FrameworkImGuiContext guiContext;
	guiContext.init(true);
	
	TextEditor textEditor;
	
	for (int i = 0; i < 6; ++i)
	{
		s_cubeFaceToWorldMatrices[i] = createCubeFaceMatrix(s_cubeFaceNamesGL[i]);
		
		s_worldToCubeFaceMatrices[i] = s_cubeFaceToWorldMatrices[i].CalcInv();
	}
	
	Cube * cube = new Cube();
	memset(cube, 0, sizeof(Cube));
	
	for (int i = 0; i < 6; ++i)
	{
		randomizeCubeFace(cube->faces[i], 0, 1, 0.f, 1.f);
	}
	
	GLuint textureGL[6];
	
	for (int i = 0; i < 6; ++i)
	{
		textureGL[i] = textureToGL(cube->faces[i].textureArray[0]);
	}
	
	ShapeDefinition shapeDefinition;
	shapeDefinition.makeRandomShape(ShapeDefinition::kMaxPlanes);
	
	shapeDefinition.loadFromFile("shape1.txt");
	
	Lattice lattice;
	lattice.init();
	
	gpuInit(lattice);
	
	Assert(sizeof(Lattice::Vertex) == 3*3*4);
	Assert(sizeof(Lattice::Edge) == 16);
	
	int numPlanesForRandomization = ShapeDefinition::kMaxPlanes;
	
	bool showCube = false;
	bool showIntersectionPoints = false;
	bool showAxis = true;
	bool showCubePoints = true;
	float cubePointScale = 1.f;
	bool projectCubePoints = false;
	bool colorizeCubePoints = false;
	bool raycastCubePointsUsingMouse = true;
	bool showLatticeVertices = false;
	bool showLatticeEdges = false;
	bool simulateLattice = false;
	bool simulateUsingGpu = false;
	float latticeTension = 1.f;
	float simulationTimeStep = 1.f / 1000.f;
	int numSimulationStepsPerDraw = 10;
	float velocityFalloff = .2f;
	
	bool doCameraControl = false;
	
	Camera3d camera;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;

		bool inputIsCaptured = doCameraControl;
		
		guiContext.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			if (ImGui::Begin("Interaction", nullptr,
				ImGuiWindowFlags_MenuBar))
			{
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("File"))
					{
						if (ImGui::MenuItem("Load shape.."))
						{
							nfdchar_t * path = nullptr;
							const nfdresult_t result = NFD_OpenDialog("txt", "", &path);

							if (result == NFD_OKAY)
							{
								shapeDefinition.loadFromFile(path);
							}
							
							delete path;
								path = nullptr;
						}
						
						ImGui::MenuItem("Load simulated data..");
						ImGui::MenuItem("Save simulated data..");
						ImGui::Separator();
						if (ImGui::MenuItem("Quit"))
							framework.quitRequested = true;
						
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}
				
				ImGui::PushItemWidth(140);
				
				ImGui::Text("Visibility");
				ImGui::Checkbox("Show cube", &showCube);
				ImGui::Checkbox("Show intersection points", &showIntersectionPoints);
				ImGui::Checkbox("Show axis", &showAxis);
				
				ImGui::Separator();
				ImGui::Text("Cube points");
				ImGui::Checkbox("Show cube points", &showCubePoints);
				ImGui::SliderFloat("Cube points scale", &cubePointScale, 0.f, 2.f);
				ImGui::Checkbox("Project cube points", &projectCubePoints);
				ImGui::Checkbox("Colorize cube points", &colorizeCubePoints);
				ImGui::Checkbox("Raycast cube points with mouse", &raycastCubePointsUsingMouse);
				
				ImGui::Separator();
				ImGui::Text("Shape generation");
				ImGui::SliderInt("Shape planes", &numPlanesForRandomization, 0, ShapeDefinition::kMaxPlanes);
				if (ImGui::Button("Randomize shape"))
					shapeDefinition.makeRandomShape(numPlanesForRandomization);
				
				ImGui::Separator();
				ImGui::Text("Lattice");
				if (ImGui::Button("Initialize lattice"))
				{
					lattice.init();
					s_gpuSimulationContext->sendVerticesToGpu();
					s_gpuSimulationContext->sendEdgesToGpu();
				}
				if (ImGui::Button("Project lattice onto shape"))
				{
					projectLatticeOntoShape(lattice, shapeDefinition);
					s_gpuSimulationContext->sendVerticesToGpu();
					s_gpuSimulationContext->sendEdgesToGpu();
				}
				ImGui::Checkbox("Show lattice vertices", &showLatticeVertices);
				ImGui::Checkbox("Show lattice edges", &showLatticeEdges);
				ImGui::Checkbox("Simulate lattice", &simulateLattice);
				if (ImGui::Checkbox("Use GPU", &simulateUsingGpu))
				{
					// make sure the vertices are synced between cpu and gpu at this point
					if (simulateUsingGpu)
						s_gpuSimulationContext->sendVerticesToGpu();
					else
						s_gpuSimulationContext->fetchVerticesFromGpu();
				}
				ImGui::SliderFloat("Lattice tension", &latticeTension, .1f, 100000.f, "%.2f", 4.f);
				ImGui::SliderFloat("Simulation time step", &simulationTimeStep, 0.f, 1.f / 10.f);
				ImGui::SliderInt("Num simulation steps per draw", &numSimulationStepsPerDraw, 1, 1000);
				ImGui::SliderFloat("Velocity falloff", &velocityFalloff, 0.f, 1.f);
				if (ImGui::Button("Squash lattice"))
				{
					s_gpuSimulationContext->fetchVerticesFromGpu();
					
					const int numVertices = 6 * kTextureSize * kTextureSize;
					
					for (int i = 0; i < numVertices; ++i)
					{
						auto & p = lattice.vertices[i].p;
						p.x *= .99f;
						p.y *= .99f;
						p.z *= .99f;
					}
					
					s_gpuSimulationContext->sendVerticesToGpu();
				}
				
				ImGui::Separator();
				ImGui::Text("Statistics");
				ImGui::LabelText("Cube size (kbyte)", "%lu", sizeof(Cube) / 1024);
				
				ImGui::PopItemWidth();
			}
			ImGui::End();
			
			if (ImGui::Begin("Inspect", nullptr,
				ImGuiWindowFlags_MenuBar))
			{
				const char * items[ShapeDefinition::kMaxPlanes];
				
				char lines[ShapeDefinition::kMaxPlanes][128];
				for (int i = 0; i < shapeDefinition.numPlanes; ++i)
				{
					auto & p = shapeDefinition.planes[i];
					
					sprintf_s(lines[i], sizeof(lines[i]), "%+.2f, %+.2f, %+.2f @ %.2f\n",
						p.normal[0],
						p.normal[1],
						p.normal[2],
						p.offset);
					
					items[i] = lines[i];
				}
				int selectedItem = - 1;
				ImGui::ListBox("Shape planes", &selectedItem, items, shapeDefinition.numPlanes);
				
				for (int i = 0; i < shapeDefinition.numPlanes; ++i)
				{
					ImGui::PushID(i);
					ImGui::PushItemWidth(200.f);
					ImGui::InputFloat3("Plane", &shapeDefinition.planes[i].normal[0], -1.f, +1.f);
					ImGui::PopItemWidth();
					
					ImGui::SameLine();
					ImGui::PushItemWidth(100.f);
					ImGui::SliderFloat("Offset", &shapeDefinition.planes[i].offset, 0.f, +2.f);
					ImGui::PopItemWidth();
					ImGui::PopID();
				}
			}
			ImGui::End();
			
			ImGui::SetNextWindowSize(ImVec2(300, 400));
			
			if (ImGui::Begin("Compute", nullptr,
				ImGuiWindowFlags_MenuBar))
			{
				textEditor.Render("");
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		const float dt = framework.timeStep;
		
		if (doCameraControl)
		{
			if (mouse.wentDown(BUTTON_LEFT))
				doCameraControl = false;
		}
		else
		{
			if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
				doCameraControl = true;
		}
		
		camera.tick(dt, doCameraControl);
		
		if (simulateLattice)
		{
			if (simulateUsingGpu)
			{
				Benchmark bm("simulate_gpu");
				
				for (int i = 0; i < numSimulationStepsPerDraw; ++i)
					::simulateLattice_gpu(lattice, simulationTimeStep, latticeTension, velocityFalloff);
				
				s_gpuSimulationContext->fetchVerticesFromGpu();
			}
			else
			{
				Benchmark bm("simulate");
				
				for (int i = 0; i < numSimulationStepsPerDraw; ++i)
					::simulateLattice(lattice, simulationTimeStep, latticeTension, velocityFalloff);
			}
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(90.f, .01f, 100.f);
			camera.pushViewMatrix();
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				checkErrorGL();
				
				if (showAxis)
				{
					gxBegin(GL_LINES);
					setColor(colorRed);
					gxVertex3f(0, 0, 0);
					gxVertex3f(1, 0, 0);
					
					setColor(colorGreen);
					gxVertex3f(0, 0, 0);
					gxVertex3f(0, 1, 0);
					
					setColor(colorBlue);
					gxVertex3f(0, 0, 0);
					gxVertex3f(0, 0, 1);
					gxEnd();
					
					const float fontSize = .2f;
					
					setColor(colorRed);
					gxPushMatrix();
					gxTranslatef(1, 0, 0);
					gxScalef(1, -1, 1);
					drawText(0, 0, fontSize, +1, +1, "+x");
					gxPopMatrix();
					
					setColor(colorGreen);
					gxPushMatrix();
					gxTranslatef(0, 1, 0);
					gxScalef(1, -1, 1);
					drawText(0, 0, fontSize, +1, +1, "+y");
					gxPopMatrix();
					
					setColor(colorBlue);
					gxPushMatrix();
					gxTranslatef(0, 0, 1);
					gxScalef(1, -1, 1);
					drawText(0, 0, fontSize, +1, +1, "+z");
					gxPopMatrix();
				}
			
				if (showCube)
				{
					for (int i = 0; i < 6; ++i)
					{
						gxSetTexture(textureGL[i]);
						gxPushMatrix();
						gxMultMatrixf(s_cubeFaceToWorldMatrices[i].m_v);
						gxTranslatef(0, 0, 1);
						setColor(colorWhite);
						drawRect(-1, -1, +1, +1);
						gxPopMatrix();
						gxSetTexture(0);
					}
				}
				
				if (showCubePoints)
				{
					setColor(colorBlue);
					for (int i = 0; i < 6; ++i)
					{
						const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
						
						gxBegin(GL_POINTS);
						for (int x = 0; x < kTextureSize; ++x)
						{
							for (int y = 0; y < kTextureSize; ++y)
							{
								const float xf = ((x + .5f) / float(kTextureSize) - .5f) * 2.f * cubePointScale;
								const float yf = ((y + .5f) / float(kTextureSize) - .5f) * 2.f * cubePointScale;
								
								Vec3 p = matrix.Mul4(Vec3(xf, yf, 1.f));
								
								if (projectCubePoints)
								{
									const float t = shapeDefinition.intersectRay_directional(p);
									
									p = p * t;
								}
								
								if (colorizeCubePoints)
								{
									setColorf(
										(p[0] + 1.f) * .5f,
										(p[1] + 1.f) * .5f,
										(p[2] + 1.f) * .5f);
								}
								
								gxVertex3f(p[0], p[1], p[2]);
							}
						}
						gxEnd();
					}
				}
				
				if (raycastCubePointsUsingMouse)
				{
					const Vec3 rayOrigin = camera.position;
					const Vec3 rayDirection = camera.getWorldMatrix().GetAxis(2);
					
					const float t = shapeDefinition.intersectRay(rayOrigin, rayDirection);
					
					const Vec3 p = rayOrigin + rayDirection * t;
					
					const float pointSize = clamp<float>(4.f / t, .01f, 80.f);
					
					setColor(colorYellow);
					glPointSize(pointSize);
					gxBegin(GL_POINTS);
					gxVertex3f(p[0], p[1], p[2]);
					gxEnd();
					glPointSize(1);
					
					// show the location in cube face space
					
					int cubeFaceIndex;
					Vec2 cubeFacePosition;
		
					projectDirectionToCubeFace(p, cubeFaceIndex, cubeFacePosition);
		
					const float fontSize = t * .05f;
					
					gxPushMatrix();
					gxTranslatef(p[0], p[1], p[2]);
					gxRotatef(-camera.yaw, 0, 1, 0);
					gxScalef(1, -1, 1);
					setColor(colorWhite);
					drawText(0, 0, fontSize, +1, +1, "cube face: %d, cube position: %.2f, %.2f",
						cubeFaceIndex,
						cubeFacePosition[0],
						cubeFacePosition[1]);
					const int texturePosition[2] =
					{
						(int)roundf((cubeFacePosition[0] / 2.f + .5f) * kTextureSize - .5f),
						(int)roundf((cubeFacePosition[1] / 2.f + .5f) * kTextureSize - .5f)
					};
					drawText(0, fontSize, fontSize, +1, +1, "texture position: %d, %d",
						texturePosition[0],
						texturePosition[1]);
					gxPopMatrix();
				}
			
				if (showIntersectionPoints)
				{
					setColor(colorWhite);
					gxBegin(GL_POINTS);
					for (int i = 0; i < 10000; ++i)
					{
						const float dx = random(-1.f, +1.f);
						const float dy = random(-1.f, +1.f);
						const float dz = random(-1.f, +1.f);
						
						const Vec3 rayDirection(dx, dy, dz);
						
						const float t = shapeDefinition.intersectRay_directional(rayDirection);
						
						const Vec3 pointOfIntersection = rayDirection * t;
						
						gxVertex3f(
							pointOfIntersection[0],
							pointOfIntersection[1],
							pointOfIntersection[2]);
					}
					gxEnd();
				}
				
				if (showLatticeVertices)
				{
					setColor(colorWhite);
					drawLatticeVertices(lattice);
				}
				
				if (showLatticeEdges)
				{
					setColor(0, 127, 0);
					drawLatticeEdges(lattice);
				}
				
				glDisable(GL_DEPTH_TEST);
				checkErrorGL();
			}
			camera.popViewMatrix();
			projectScreen2d();
			
			guiContext.draw();
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	glDeleteTextures(6, textureGL);
	
	delete cube;
	cube = nullptr;
	
	gpuShut();
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
