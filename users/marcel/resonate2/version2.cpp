#include "Benchmark.h"
#include "computeEditor.h"
#include "constants.h"
#include "framework.h"
#include "gpu.h"
#include "gpuSimulationContext.h"
#include "gpuSources.h"
#include "imgui-framework.h"
#include "lattice.h"
#include "nfd.h"
#include "shape.h"
#include "StringEx.h"

//#define __CL_ENABLE_EXCEPTIONS

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
	
	const Mat4x4 & worldToCubeFaceMatrix = s_worldToCubeFaceMatrices[cubeFaceIndex];
	
	const Vec3 direction_cubeFace = worldToCubeFaceMatrix.Mul4(direction);
	
	cubeFacePosition[0] = direction_cubeFace[0] / direction_cubeFace[2];
	cubeFacePosition[1] = direction_cubeFace[1] / direction_cubeFace[2];
}

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
	
	ComputeEditor computeEdgeForcesEditor(s_gpuSimulationContext->computeEdgeForcesProgram);
	ComputeEditor integrateEditor(s_gpuSimulationContext->integrateProgram);
	
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
				ImGui::SliderFloat("Simulation time step", &simulationTimeStep, 0.f, 1.f / 40.f);
				ImGui::SliderInt("Num simulation steps per draw", &numSimulationStepsPerDraw, 1, 1000);
				ImGui::SliderFloat("Velocity falloff", &velocityFalloff, 0.f, 1.f);
				if (ImGui::Button("Squash lattice"))
				{
					if (simulateUsingGpu)
						s_gpuSimulationContext->fetchVerticesFromGpu();
					
					const int numVertices = 6 * kTextureSize * kTextureSize;
					
					for (int i = 0; i < numVertices; ++i)
					{
						auto & p = lattice.vertices[i].p;
						p.x *= .99f;
						p.y *= .99f;
						p.z *= .99f;
					}
					
					if (simulateUsingGpu)
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
					setColor(63, 63, 255);
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
