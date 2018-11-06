#include "Benchmark.h"
#include "computeEditor.h"
#include "constants.h"
#include "draw.h"
#include "fmt_cird.h"
#include "framework.h"
#include "gpu.h"
#include "gpuSimulationContext.h"
#include "imgui-framework.h"
#include "impulseResponse.h"
#include "lattice.h"
#include "nfd.h"
#include "shape.h"
#include "StringEx.h"
#include <math.h>

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStream.h"

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

struct Texture
{
	float value[kProbeGridSize][kProbeGridSize];
};

struct CubeFace
{
	Texture textures[kNumProbeFrequencies];
};

struct Cube
{
	CubeFace faces[6];
};

static void randomizeTexture(Texture & texture, const float min, const float max)
{
	for (int x = 0; x < kProbeGridSize; ++x)
	{
		for (int y = 0; y < kProbeGridSize; ++y)
		{
			texture.value[y][x] = random<float>(min, max);
		}
	}
}

static void fillCube(const Lattice & lattice, const ImpulseResponseProbe * probes, const int frequencyIndex, Cube & cube)
{
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
	{
		CubeFace & face = cube.faces[faceIndex];
		
		Texture & texture = face.textures[frequencyIndex];
		
		for (int x = 0; x < kProbeGridSize; ++x)
		{
			for (int y = 0; y < kProbeGridSize; ++y)
			{
				const int probeIndex = calcProbeIndex(faceIndex, x, y);
				
				auto & probe = probes[probeIndex];
				
				const float value = probe.calcResponseMagnitudeForFrequencyIndex(frequencyIndex) * 4000.f;
				
				texture.value[y][x] = value;
			}
		}
	}
}

static void randomizeCubeFace(CubeFace & cubeFace, const int firstTexture, const int numTextures, const float min, const float max)
{
	for (int i = 0; i < numTextures; ++i)
	{
		randomizeTexture(cubeFace.textures[firstTexture + i], min, max);
	}
}

static GLuint textureToGL(const Texture & texture)
{
	return createTextureFromR32F(texture.value, kProbeGridSize, kProbeGridSize, false, true);
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
		mat.MakeIdentity();
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: // Front
		mat.MakeRotationY(180 * degToRad);
		break;
		
	default:
		Assert(false);
		mat.MakeIdentity();
		break;
	}
	
	return mat;
}

Mat4x4 s_cubeFaceToWorldMatrices[6];
Mat4x4 s_worldToCubeFaceMatrices[6];

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
	
	Assert(cubeFaceIndex >= 0 && cubeFaceIndex < 6);
	
	const Mat4x4 & worldToCubeFaceMatrix = s_worldToCubeFaceMatrices[cubeFaceIndex];
	
	const Vec3 direction_cubeFace = worldToCubeFaceMatrix.Mul4(direction);
	
	cubeFacePosition[0] = direction_cubeFace[0] / direction_cubeFace[2];
	cubeFacePosition[1] = direction_cubeFace[1] / direction_cubeFace[2];
	
	Assert(cubeFacePosition[0] >= -1.f && cubeFacePosition[0] <= +1.f);
	Assert(cubeFacePosition[1] >= -1.f && cubeFacePosition[1] <= +1.f);
}

static void projectLatticeOntoShape(Lattice & lattice, const ShapeDefinition & shape)
{
	const int numVertices = kNumVertices;
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & v = lattice.vertices[i];
		
		int planeIndex;
		
		const float t = shape.intersectRay_directional(Vec3(v.p.x, v.p.y, v.p.z), planeIndex);
		
		v.p.set(
			v.p.x * t,
			v.p.y * t,
			v.p.z * t);
		
		const Vec3 & n = shape.planes[planeIndex].normal;
		
		v.n.set(n[0], n[1], n[2]);
	}

	lattice.finalize();
}

static void projectLatticeOntoShere(Lattice & lattice)
{
	const int numVertices = kNumVertices;
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & p = lattice.vertices[i].p;
		
		const float t = 1.f / Vec3(p.x, p.y, p.z).CalcSize();
		
		lattice.vertices[i].p.set(
			p.x * t,
			p.y * t,
			p.z * t);
		
		lattice.vertices[i].f.setZero();
		lattice.vertices[i].v.setZero();
		
		lattice.vertices[i].n = lattice.vertices[i].p;
	}

	lattice.finalize();
}

//

static GpuContext * s_gpuContext = nullptr;

static GpuSimulationContext * s_gpuSimulationContext = nullptr;

static bool gpuInit(Lattice & lattice, ImpulseResponseState * impulseResponseState, ImpulseResponseProbe * probes, const int numProbes)
{
	s_gpuContext = new GpuContext();
	
	if (!s_gpuContext->init())
		return false;
	
	s_gpuSimulationContext = new GpuSimulationContext(*s_gpuContext);
	
	if (!s_gpuSimulationContext->init(lattice, impulseResponseState, probes, numProbes))
		return false;

	return true;
}

bool gpuShut()
{
	if (s_gpuSimulationContext != nullptr)
	{
		s_gpuSimulationContext->shut();
		
		delete s_gpuSimulationContext;
		s_gpuSimulationContext = nullptr;
	}
	
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
	
	const float eps = 1e-12f;
	
	for (auto & edge : lattice.edges)
	{
		auto & v1 = lattice.vertices[edge.vertex1];
		auto & v2 = lattice.vertices[edge.vertex2];
		
	// todo : remove ?
		//edge.weight = 1.f / edge.initialDistance * 1.f / edge.initialDistance * 1.f / edge.initialDistance / 10000.f;
		
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
	
	const int numVertices = kNumVertices;
	
	const float retain = powf(1.f - falloff, dt / 1000.f);
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & v = lattice.vertices[i];
		
		v.v.x *= retain;
		v.v.y *= retain;
		v.v.z *= retain;
		
	#if 1
		// constrain the vertex to the line emanating from (0, 0, 0) towards this vertex. this helps to stabalize
		// the simulation while at the same time making the resulting impulse-response more pleasing. it sort of
		// models a more rigid structure where vertices are held in place 'xy' on the plane they sit in through
		// the forces of the elements in the structure below them
		
		// todo : constrain vertices using the normal of the plane they sit in instead of the line to (0, 0, 0)
		
		float nx = v.p.x;
		float ny = v.p.y;
		float nz = v.p.z;
		const float ns = sqrtf(nx * nx + ny * ny + nz * nz);
		nx /= ns;
		ny /= ns;
		nz /= ns;
		
		const float dot =
			nx * v.f.x +
			ny * v.f.y +
			nz * v.f.z;
		
		v.v.x += nx * dot * dt;
		v.v.y += ny * dot * dt;
		v.v.z += nz * dot * dt;
	#else
		v.v.x += v.f.x * dt;
		v.v.y += v.f.y * dt;
		v.v.z += v.f.z * dt;
	#endif
		
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

static void integrateImpulseResponses(const Lattice & lattice, ImpulseResponseState & state, ImpulseResponseProbe * probes, const int numProbes, const float dt)
{
	state.processBegin(dt);

	for (int i = 0; i < numProbes; ++i)
	{
		auto & probe = probes[i];
		
		probe.measureAtVertex(state, lattice);
	}

	state.processEnd();
}

static void integrateImpulseResponses_gpu(const Lattice & lattice, ImpulseResponseState & state, ImpulseResponseProbe * probes, const int numProbes, const float dt)
{
	state.processBegin(dt);

	// send impulse response state to the gpu

	s_gpuSimulationContext->sendImpulseResponseStateToGpu();

	// execute impulse-response integration kernel

	s_gpuSimulationContext->integrateImpulseResponse(dt);

	state.processEnd();
}

//

static void squashLattice(Lattice & lattice)
{
	const int numVertices = kNumVertices;

	for (int i = 0; i < numVertices; ++i)
	{
		auto & p = lattice.vertices[i].p;
		p.x *= .99f;
		p.y *= .99f;
		p.z *= .99f;
	}
}

static bool cirdToImpulseResponseData(const CIRD & cird, ImpulseResponseState & state, ImpulseResponseProbe * probes, const int probeGridSize)
{
	// check if we can handle the CIRD data. that is, whether it isn't too big to fit our arrays
	
	if (cird.header.numFrequencies > kNumProbeFrequencies ||
		cird.header.cubeMapSx != probeGridSize ||
		cird.header.cubeMapSy != probeGridSize)
	{
		return false;
	}
	
	// initialize the impulse response state with the frequency table from the CIRD
	
	state = ImpulseResponseState();
	
	state.init(cird.frequencyTable.frequencies, cird.header.numFrequencies);
	
	// copy the information from the CIRD to the impulse response probes
	
	const int numProbes = probeGridSize * probeGridSize;

	for (int i = 0; i < numProbes; ++i)
		probes[i].init(probes[i].vertexIndex);
	
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
	{
		const CIRD::Face & face = cird.cube.faces[faceIndex];
		
		for (int imageIndex = 0; imageIndex < cird.header.numFrequencies; ++imageIndex)
		{
			const int frequencyIndex = imageIndex;
			
			const CIRD::Image & image = face.images[imageIndex];
			
			for (int y = 0; y < cird.header.cubeMapSy; ++y)
			{
				for (int x = 0; x < cird.header.cubeMapSx; ++x)
				{
					const int probeIndex =
						faceIndex * probeGridSize * probeGridSize +
						y * probeGridSize +
						x;
					
					ImpulseResponseProbe & probe = probes[probeIndex];
					
					const int imageValueIndex = cird.calcImageValueIndex(x, y);
					
					probe.response[frequencyIndex][0] = image.values[imageValueIndex].real;
					probe.response[frequencyIndex][1] = image.values[imageValueIndex].imag;
				}
			}
		}
	}
	
	return true;
}

static bool impulseResponseDataToCird(const ImpulseResponseState & state, const ImpulseResponseProbe * probes, const int probeGridSize, CIRD & cird)
{
	// initialize the CIRD with our frequency table
	
	float frequencies[kNumProbeFrequencies];
	for (int i = 0; i < kNumProbeFrequencies; ++i)
		frequencies[i] = state.frequency[i];

	if (cird.initialize(kNumProbeFrequencies, probeGridSize, probeGridSize, frequencies) == false)
	{
		logError("failed to initialize the CIRD");
		return false;
	}
	else
	{
		// save the impulse response data to the CIRD image arrays
		
		for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
		{
			CIRD::Face & face = cird.cube.faces[faceIndex];
			
			for (int imageIndex = 0; imageIndex < cird.header.numFrequencies; ++imageIndex)
			{
				const int frequencyIndex = imageIndex;
				
				CIRD::Image & image = face.images[imageIndex];
				
				for (int y = 0; y < cird.header.cubeMapSy; ++y)
				{
					for (int x = 0; x < cird.header.cubeMapSx; ++x)
					{
						const int probeIndex =
							faceIndex * probeGridSize * probeGridSize +
							y * probeGridSize +
							x;
						
						const ImpulseResponseProbe & probe = probes[probeIndex];
						
						const int imageValueIndex = cird.calcImageValueIndex(x, y);
						
						image.values[imageValueIndex].real = probe.response[frequencyIndex][0];
						image.values[imageValueIndex].imag = probe.response[frequencyIndex][1];
					}
				}
			}
		}
		
		return true;
	}
}

struct Sonify : AudioStream
{
	const ImpulseResponseState * state = nullptr;
	
	float oldMagnitudes[kNumProbeFrequencies];
	float newMagnitudes[kNumProbeFrequencies];
	
	float phase[kNumProbeFrequencies];
	
	SDL_mutex * mutex = nullptr;
	
	AudioOutput_PortAudio * audioOutput = nullptr;
	
	virtual ~Sonify() override
	{
		Assert(audioOutput == nullptr);
		
		shut();
	}
	
	void init(const ImpulseResponseState * in_state)
	{
		shut();
		
		//
		
		state = in_state;
		
		memset(oldMagnitudes, 0, sizeof(oldMagnitudes));
		memset(newMagnitudes, 0, sizeof(newMagnitudes));
		
		memset(phase, 0, sizeof(phase));
		
		mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);
		
		audioOutput = new AudioOutput_PortAudio();
		audioOutput->Initialize(2, 44100, 256);
		audioOutput->Play(this);
	}
	
	void shut()
	{
		if (audioOutput != nullptr)
		{
			audioOutput->Shutdown();
			
			delete audioOutput;
			audioOutput = nullptr;
		}
		
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
		
		state = nullptr;
	}
	
	void update(const ImpulseResponseProbe & probe)
	{
		float magnitudes[kNumProbeFrequencies];
		probe.calcResponseMagnitude(magnitudes);
		
		Verify(SDL_LockMutex(mutex) == 0);
		{
			memcpy(newMagnitudes, magnitudes, sizeof(newMagnitudes));
		}
		Verify(SDL_UnlockMutex(mutex) == 0);
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		// create a copy of the new magnitudes so we can work with the latest values
		// do it first to avoid locking the mutex around the entire synthesis section
		
		float newMagnitudesCopy[kNumProbeFrequencies];
		
		Verify(SDL_LockMutex(mutex) == 0);
		{
			memcpy(newMagnitudesCopy, newMagnitudes, sizeof(newMagnitudesCopy));
		}
		Verify(SDL_UnlockMutex(mutex) == 0);
		
		// create ramps to blend between the old and the new magnitude values
		
		float newRamp[numSamples];
		float oldRamp[numSamples];
		
		for (int s = 0; s < numSamples; ++s)
		{
			newRamp[s] = s / float(numSamples);
			oldRamp[s] = 1.f - newRamp[s];
		}
		
		// synthesize signal
		
		const float twoPi = 2.f * M_PI;
		
		const float dt_times_twoPi = twoPi / 44100.f;
		
		for (int s = 0; s < numSamples; ++s)
		{
			// accumulate per-frequency oscillators
			
			float value = 0.f;
			
			for (int i = 0; i < kNumProbeFrequencies; ++i)
			{
				const float phaseValue = sinf(phase[i]);
				
				value +=
					phaseValue * newMagnitudesCopy[i] * newRamp[s] +
					phaseValue * oldMagnitudes[i]     * oldRamp[s];
			}
			
			// apply gain
			
			value *= 100.f;
			
			// clipping
			
			if (value < -1.f)
				value = -1.f;
			else if (value > +1.f)
				value = +1.f;
			
			// write to buffer
			
			const int valueAsInt = value * ((1 << 15) - 1);
			
			buffer[s].channel[0] = valueAsInt;
			buffer[s].channel[1] = valueAsInt;
			
			// increment the phase for each frequency
			
			for (int i = 0; i < kNumProbeFrequencies; ++i)
			{
				phase[i] = phase[i] + state->frequency[i] * dt_times_twoPi;
				
				while (phase[i] >= twoPi)
					phase[i] -= twoPi;
			}
		}
		
		memcpy(oldMagnitudes, newMagnitudesCopy, sizeof(oldMagnitudes));
		
		return numSamples;
	}
};

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
	
	// initialize the cube map and associated data
	
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
	
	//
	
	GLuint textureGL[6];
	
	for (int i = 0; i < 6; ++i)
	{
		textureGL[i] = textureToGL(cube->faces[i].textures[0]);
	}
	
	// initialize the shape
	
	ShapeDefinition shapeDefinition;
	shapeDefinition.makeRandomShape(ShapeDefinition::kMaxPlanes);
	
	shapeDefinition.loadFromFile("shape1.txt");
	
	// initialize the lattice
	
	Lattice * latticePtr = new Lattice();
	
	Lattice & lattice = *latticePtr;
	lattice.init();
	
	// initialize the impulse response probes and associated state
	
	ImpulseResponseState impulseResponseState;
	impulseResponseState.init();
	
	ImpulseResponseProbe * impulseResponseProbes = new ImpulseResponseProbe[kNumProbes];
	
	for (int cubeFaceIndex = 0; cubeFaceIndex < 6; ++cubeFaceIndex)
	{
		for (int y = 0; y < kProbeGridSize; ++y)
		{
			for (int x = 0; x < kProbeGridSize; ++x)
			{
				const int probeIndex = calcProbeIndex(cubeFaceIndex, x, y);
				
				const int faceX = x * kGridSize / kProbeGridSize;
				const int faceY = y * kGridSize / kProbeGridSize;
				
				auto & probe = impulseResponseProbes[probeIndex];
				
				probe.init(calcVertexIndex(cubeFaceIndex, faceX, faceY));
			}
		}
	}
	
	// initialize the GPU simulation context
	
	gpuInit(lattice, &impulseResponseState, impulseResponseProbes, kNumProbes);
	
	Assert(sizeof(Lattice::Vertex) == 5*3*4);
	Assert(sizeof(Lattice::Edge) == 16);
	
	ComputeEditor computeEdgeForcesEditor(s_gpuSimulationContext->computeEdgeForcesProgram);
	ComputeEditor integrateEditor(s_gpuSimulationContext->integrateProgram);
	ComputeEditor integrateImpulseResponseEditor(s_gpuSimulationContext->integrateImpulseResponseProgram);
	
	// sonification
	
	Sonify * sonify = new Sonify();
	
	sonify->init(&impulseResponseState);
	
	// editable state through ImGui
	
	int numPlanesForRandomization = ShapeDefinition::kMaxPlanes;
	
	bool showGui = true;
	bool showCube = false;
	bool showIntersectionPoints = false;
	bool showAxis = true;
	bool showImpulseResponseGraph = true;
	bool showImpulseResponseProbeLocations = true;
	bool showCubePoints = false;
	float cubePointScale = 1.f;
	bool projectCubePoints = false;
	bool colorizeCubePoints = false;
	bool raycastCubePointsUsingMouse = true;
	bool showLatticeVertices = false;
	bool showLatticeEdges = false;
	bool showLatticeFaces = false;
	bool simulateLattice = false;
	bool simulateUsingGpu = false;
	float latticeTension = 1.f;
	float simulationTimeStep_ms = .1f;
	int numSimulationStepsPerDraw = 10;
	float velocityFalloff = .6f;
	int fillCubeFrequencyIndex = kNumProbeFrequencies / 2;
	
	// camera control and ray cast
	
	bool doCameraControl = false;
	
	Camera3d camera;
	
	bool hasMouseCubeFace = false;
	int mouseCubeFaceIndex = -1;
	int mouseCubeFacePosition[2] = { -1, -1 };
	
	// simulation state
	
	float simulationTime_ms = 0.f;
	
	while (!framework.quitRequested)
	{
		// when camera control is active, hide the mouse cursor and set it to relative mode
		
		if (doCameraControl)
		{
			mouse.showCursor(false);
			mouse.setRelative(true);
		}
		else
		{
			mouse.showCursor(true);
			mouse.setRelative(false);
		}
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		// toggle UI visibility when the tab key is pressed
		
		if (keyboard.wentDown(SDLK_TAB))
		{
			showGui = !showGui;
			
			if (showGui == false)
			{
				doCameraControl = true;
				
				mouse.dx = 0;
				mouse.dy = 0;
			}
			else
				doCameraControl = false;
		}
		
		// show the UI
		
		bool inputIsCaptured = doCameraControl;
		
		guiContext.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			if (showGui)
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
							
							if (ImGui::MenuItem("Load simulated data.."))
							{
								nfdchar_t * path = nullptr;
								const nfdresult_t result = NFD_OpenDialog("cird", "", &path);

								if (result == NFD_OKAY)
								{
									framework.process(); // to make sure the NFD dialog disappears
									
									CIRD cird;
									
									if (cird.loadFromFile(path))
									{
										cirdToImpulseResponseData(
											cird,
											impulseResponseState,
											impulseResponseProbes,
											kProbeGridSize);
									}
								}
								
								delete path;
								path = nullptr;
								
								simulateLattice = false;
							}
							if (ImGui::MenuItem("Save simulated data.."))
							{
								nfdchar_t * path = nullptr;
								const nfdresult_t result = NFD_SaveDialog("cird", "", &path);

								if (result == NFD_OKAY)
								{
									CIRD cird;
									
									if (impulseResponseDataToCird(
										impulseResponseState,
										impulseResponseProbes,
										kProbeGridSize,
										cird))
									{
										framework.process(); // to make sure the NFD dialog disappears
										
										cird.saveToFile(path);
									}
								}
								
								delete path;
								path = nullptr;
							}
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
					ImGui::Checkbox("Show impulse response graph", &showImpulseResponseGraph);
					ImGui::Checkbox("Show impulse response probes", &showImpulseResponseProbeLocations);
					
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
						simulationTime_ms = 0.f;
					}
					if (ImGui::Button("Project lattice onto shape"))
					{
						projectLatticeOntoShape(lattice, shapeDefinition);
						s_gpuSimulationContext->sendVerticesToGpu();
						s_gpuSimulationContext->sendEdgesToGpu();
						simulationTime_ms = 0.f;
					}
					if (ImGui::Button("Project lattice onto sphere"))
					{
						projectLatticeOntoShere(lattice);
						s_gpuSimulationContext->sendVerticesToGpu();
						s_gpuSimulationContext->sendEdgesToGpu();
						simulationTime_ms = 0.f;
					}
					ImGui::Checkbox("Show lattice vertices", &showLatticeVertices);
					ImGui::Checkbox("Show lattice edges", &showLatticeEdges);
					ImGui::Checkbox("Show lattice faces", &showLatticeFaces);
					int colorMode = s_vertexColorMode;
					ImGui::Combo("Color mode", &colorMode, s_vertexColorModeNames, kVertexColorMode_COUNT);
					s_vertexColorMode = (VertexColorMode)colorMode;
					if (ImGui::Button("Squash lattice"))
					{
						if (simulateUsingGpu)
							s_gpuSimulationContext->fetchVerticesFromGpu();
						squashLattice(lattice);
						if (simulateUsingGpu)
							s_gpuSimulationContext->sendVerticesToGpu();
					}
					
					ImGui::Separator();
					ImGui::Text("Simulation");
					ImGui::Checkbox("Simulate lattice", &simulateLattice);
					if (ImGui::Checkbox("Use GPU", &simulateUsingGpu))
					{
						// make sure the vertices are synced between cpu and gpu at this point
						if (simulateUsingGpu)
							s_gpuSimulationContext->sendVerticesToGpu();
						else
							s_gpuSimulationContext->fetchVerticesFromGpu();
					}
					ImGui::SliderFloat("Lattice tension", &latticeTension, .1f, 100.f, "%.3f", 4.f);
					ImGui::SliderFloat("Simulation time step (ms)", &simulationTimeStep_ms, 1.f / 1000.f, .1f, "%.3f", 2.f);
					ImGui::SliderInt("Num simulation steps per draw", &numSimulationStepsPerDraw, 1, 100);
					ImGui::SliderFloat("Velocity falloff", &velocityFalloff, 0.f, 1.f);
					if (ImGui::Button("(Re)start simulation"))
					{
						lattice.init();
						projectLatticeOntoShape(lattice, shapeDefinition);
						squashLattice(lattice);
						s_gpuSimulationContext->sendVerticesToGpu();
						s_gpuSimulationContext->sendEdgesToGpu();
						for (int i = 0; i < kNumProbes; ++i)
							impulseResponseProbes[i].init(impulseResponseProbes[i].vertexIndex);
						simulateLattice = true;
						simulationTime_ms = 0.f;
					}
					
					ImGui::Separator();
					ImGui::Text("Cube map");
					if (ImGui::Button("Fill cube map"))
					{
						fillCube(lattice, impulseResponseProbes, fillCubeFrequencyIndex, *cube);
						glDeleteTextures(6, textureGL);
						for (int i = 0; i < 6; ++i)
							textureGL[i] = textureToGL(cube->faces[i].textures[fillCubeFrequencyIndex]);
					}
					if (ImGui::SliderInt("Frequency index", &fillCubeFrequencyIndex, 0, kNumProbeFrequencies - 1))
					{
						fillCube(lattice, impulseResponseProbes, fillCubeFrequencyIndex, *cube);
						glDeleteTextures(6, textureGL);
						for (int i = 0; i < 6; ++i)
							textureGL[i] = textureToGL(cube->faces[i].textures[fillCubeFrequencyIndex]);
						showCube = true;
					}
					
					ImGui::Separator();
					ImGui::Text("Statistics");
					ImGui::LabelText("Cube size (kbyte)", "%lu", sizeof(Cube) / 1024);
					ImGui::LabelText("Probes size (kbyte)", "%lu", sizeof(ImpulseResponseProbe) * kNumProbes / 1024);
					
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
				
				ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Compute #1", nullptr,
					ImGuiWindowFlags_MenuBar))
				{
					computeEdgeForcesEditor.Render();
				}
				ImGui::End();
				
				ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Compute #2", nullptr,
					ImGuiWindowFlags_MenuBar))
				{
					integrateEditor.Render();
				}
				ImGui::End();
				
				ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Compute #2", nullptr,
					ImGuiWindowFlags_MenuBar))
				{
					integrateImpulseResponseEditor.Render();
				}
				ImGui::End();
			}
		}
		guiContext.processEnd();
		
		const float dt = framework.timeStep;
		
		// toggle camera control when the left mouse button is pressed (and ImGui doesn't already capture input)
		
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
			const float scaledLatticeTension = latticeTension * kGridSize * kGridSize / 1000.f;
			
			if (simulateUsingGpu)
			{
				Benchmark bm("simulate_gpu");
				
				// send impulse response values to the gpu
			// todo : only upload impulse-response values once, similar to how we cache vertices and edges on the gpu
			
				s_gpuSimulationContext->sendImpulseResponseProbesToGpu();
				
				for (int i = 0; i < numSimulationStepsPerDraw; ++i)
				{
					::simulateLattice_gpu(lattice, simulationTimeStep_ms, scaledLatticeTension, velocityFalloff);
					
					simulationTime_ms += simulationTimeStep_ms;
					
					//
					
					integrateImpulseResponses_gpu(
						lattice,
						impulseResponseState,
						impulseResponseProbes,
						kNumProbes,
						simulationTimeStep_ms / 1000.f);
				}
				
				// refresh the vertices and impulse responses with the data computed on the GPU. we will need it for visualization purposes
				
				s_gpuSimulationContext->fetchVerticesFromGpu();
				
				s_gpuSimulationContext->fetchImpulseResponseProbesFromGpu();
			}
			else
			{
				Benchmark bm("simulate");
				
				for (int i = 0; i < numSimulationStepsPerDraw; ++i)
				{
					::simulateLattice(lattice, simulationTimeStep_ms, scaledLatticeTension, velocityFalloff);
					
					simulationTime_ms += simulationTimeStep_ms;
					
					//
					
					integrateImpulseResponses(
						lattice,
						impulseResponseState,
						impulseResponseProbes,
						kNumProbes,
						simulationTimeStep_ms / 1000.f);
				}
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
					// show XYZ axis
					
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
					// todo : update cube map with data from the impulse response probes
					
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
					// draw a point for each texel of the cube map. optionally project these points onto the shape
					
					for (int i = 0; i < 6; ++i)
					{
						const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
						
						setColor(63, 63, 255);
						gxBegin(GL_POINTS);
						for (int x = 0; x < kGridSize; ++x)
						{
							for (int y = 0; y < kGridSize; ++y)
							{
								const float xf = ((x + .5f) / float(kGridSize) - .5f) * 2.f * cubePointScale;
								const float yf = ((y + .5f) / float(kGridSize) - .5f) * 2.f * cubePointScale;
								
								Vec3 p = matrix.Mul4(Vec3(xf, yf, 1.f));
								
								if (projectCubePoints)
								{
									int planeIndex;
									
									const float t = shapeDefinition.intersectRay_directional(p, planeIndex);
									
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
				
				if (showIntersectionPoints)
				{
					// show a random distribution of points projected onto the shape
					
					setColor(colorWhite);
					gxBegin(GL_POINTS);
					for (int i = 0; i < 20000; ++i)
					{
						const float dx = random(-1.f, +1.f);
						const float dy = random(-1.f, +1.f);
						const float dz = random(-1.f, +1.f);
						
						const Vec3 rayDirection(dx, dy, dz);
						
						int planeIndex;
						
						const float t = shapeDefinition.intersectRay_directional(rayDirection, planeIndex);
						
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
				
				if (showLatticeFaces)
				{
					setColor(127, 0, 0);
					drawLatticeFaces(lattice);
				}
				
				if (raycastCubePointsUsingMouse && hasMouseCubeFace)
				{
					// show the impulse response probe locations at the mouse cursor
					
					const int cubeFaceIndex = mouseCubeFaceIndex;
					const int x = mouseCubeFacePosition[0] * kProbeGridSize / kGridSize;
					const int y = mouseCubeFacePosition[1] * kProbeGridSize / kGridSize;
					
					Assert(x >= 0 && x < kProbeGridSize);
					Assert(y >= 0 && y < kProbeGridSize);
					
					const int probeIndex = calcProbeIndex(cubeFaceIndex, 0, y);
					
					// draw the impulse response probe directly underneath the mouse cursor
					setColor(255, 200, 200);
					drawImpulseResponseProbes(impulseResponseProbes + probeIndex + x, 1, lattice);
					
					// draw the line of impulse response probes along the a-xis underneath the mouse cursor
					setColor(255, 63, 63);
					drawImpulseResponseProbes(impulseResponseProbes + probeIndex, kProbeGridSize, lattice);
				}
				
				if (showImpulseResponseProbeLocations)
				{
					// show all of the impulse response probe locations
					
					setColor(127, 0, 0);
					drawImpulseResponseProbes(impulseResponseProbes, kNumProbes, lattice);
				}
				
				if (raycastCubePointsUsingMouse)
				{
					// intersect the shape using a raycast from the camera into the scene and show some
					// useful information about the location we hit
					
					const Vec3 rayOrigin = camera.position;
					const Vec3 rayDirection = camera.getWorldMatrix().GetAxis(2);
					
					const float t = shapeDefinition.intersectRay(rayOrigin, rayDirection);
					
					const Vec3 p = rayOrigin + rayDirection * t;
					
					const float pointSize = clamp<float>(4.f / t, .01f, 80.f);
					
					// show a yellow rectangle at the point of intersection
					
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
					glDisable(GL_DEPTH_TEST);
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
						(int)roundf((cubeFacePosition[0] / 2.f + .5f) * kGridSize - .5f),
						(int)roundf((cubeFacePosition[1] / 2.f + .5f) * kGridSize - .5f)
					};
					drawText(0, fontSize, fontSize, +1, +1, "texture position: %d, %d",
						texturePosition[0],
						texturePosition[1]);
					glEnable(GL_DEPTH_TEST);
					gxPopMatrix();
					
					// cache the raycast information so it can be used elsewhere
					
					hasMouseCubeFace = true;
					mouseCubeFaceIndex = cubeFaceIndex;
					mouseCubeFacePosition[0] = texturePosition[0];
					mouseCubeFacePosition[1] = texturePosition[1];
				}
				else
				{
					hasMouseCubeFace = false;
				}
				
				glDisable(GL_DEPTH_TEST);
				checkErrorGL();
			}
			camera.popViewMatrix();
			projectScreen2d();
			
			if (showImpulseResponseGraph && hasMouseCubeFace)
			{
				// show the impulse response graphs for the location of the raycast
				
				gxPushMatrix();
				{
					gxTranslatef(VIEW_SX - 740, 10, 0);
					
					const int cubeFaceIndex = mouseCubeFaceIndex;
					const int x = mouseCubeFacePosition[0] * kProbeGridSize / kGridSize;
					const int y = mouseCubeFacePosition[1] * kProbeGridSize / kGridSize;
			
					Assert(x >= 0 && x < kProbeGridSize);
					Assert(y >= 0 && y < kProbeGridSize);
			
					const int probeIndex = calcProbeIndex(cubeFaceIndex, 0, y);
					
					const auto probes = impulseResponseProbes + probeIndex;
					
					float responses[kGridSize * kNumProbeFrequencies];
					
					for (int i = 0; i < kGridSize; ++i)
					{
						auto & probe = probes[i];
						
						probe.calcResponseMagnitude(responses + i * kNumProbeFrequencies);
					}
					
					drawImpulseResponseGraphs(impulseResponseState, responses, kGridSize, true, -1.f, x);
				}
				gxPopMatrix();
			}
			
			if (true && hasMouseCubeFace)
			{
				const int cubeFaceIndex = mouseCubeFaceIndex;
				const int x = mouseCubeFacePosition[0] * kProbeGridSize / kGridSize;
				const int y = mouseCubeFacePosition[1] * kProbeGridSize / kGridSize;
		
				Assert(x >= 0 && x < kProbeGridSize);
				Assert(y >= 0 && y < kProbeGridSize);
		
				const int probeIndex = calcProbeIndex(cubeFaceIndex, x, y);
			
				const auto & probe = impulseResponseProbes[probeIndex];
				
				sonify->update(probe);
			}
			
			setColor(colorWhite);
			drawText(8, VIEW_SY - 20, 14, +1, +1, "time %.2fms", simulationTime_ms);
			
			guiContext.draw();
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	sonify->shut();
	delete sonify;
	sonify = nullptr;
	
	delete latticePtr;
	latticePtr = nullptr;
	
	glDeleteTextures(6, textureGL);
	
	delete cube;
	cube = nullptr;
	
	delete [] impulseResponseProbes;
	impulseResponseProbes = nullptr;
	
	gpuShut();
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
