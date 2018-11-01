#include "framework.h"
#include "imgui-framework.h"
#include "Parse.h"
#include "TextIO.h"

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

const int VIEW_SX = 1000;
const int VIEW_SY = 600;

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
		mat.MakeRotationX(-90 * degToRad);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: // Bottom
		mat.MakeRotationX(+90 * degToRad);
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
	struct Vertex
	{
		Vec3 p;
		
		// physics stuff
		Vec3 f;
		Vec3 v;
	};
	
	struct Edge
	{
		int vertex1;
		int vertex2;
		
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
					
					vertices[index].p = p;
				}
			}
		}
		
		// setup up edges for face interiors
		
		auto addEdge = [&](const int faceIndex, const int x1, const int y1, const int x2, const int y2)
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
			
			edges.push_back(edge);
		};
		
		for (int i = 0; i < 6; ++i)
		{
			for (int x = 0; x < kTextureSize - 1; ++x)
			{
				for (int y = 0; y < kTextureSize - 1; ++y)
				{
					addEdge(i, x + 0, y + 0, x + 1, y + 0);
					addEdge(i, x + 0, y + 0, x + 0, y + 1);
					
					addEdge(i, x + 0, y + 0, x + 1, y + 1);
					addEdge(i, x + 0, y + 1, x + 1, y + 0);
				
				}
			}
		}
		
		finalize();
	}
	
	void finalize()
	{
		for (auto & edge : edges)
		{
			const auto & p1 = vertices[edge.vertex1].p;
			const auto & p2 = vertices[edge.vertex2].p;
			
			const auto delta = p2 - p1;
			
			const float distance = delta.CalcSize();
			
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
		
		const float t = shape.intersectRay_directional(p);
		
		lattice.vertices[i].p = p * t;
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
			
			gxVertex3f(p[0], p[1], p[2]);
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
			
			gxVertex3f(p1[0], p1[1], p1[2]);
			gxVertex3f(p2[0], p2[1], p2[2]);
		}
	}
	gxEnd();
}

static void simulateLattice(Lattice & lattice, const float dt, const float tension)
{
	const float eps = 1e-4f;
	
	const int numVertices = 6 * kTextureSize * kTextureSize;
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & v = lattice.vertices[i];
		
		v.f.SetZero();
	}
	
	for (auto & edge : lattice.edges)
	{
		auto & v1 = lattice.vertices[edge.vertex1];
		auto & v2 = lattice.vertices[edge.vertex2];
		
		const Vec3 delta = v2.p - v1.p;
		
		const float distance = delta.CalcSize();
		
		if (distance < eps)
			continue;
		
		const Vec3 direction = delta / distance;
		
		const float pull = distance - edge.initialDistance;
		
		const Vec3 f = direction * pull * tension;
		
		v1.f += f;
		v2.f -= f;
	}
	
	for (int i = 0; i < numVertices; ++i)
	{
		auto & v = lattice.vertices[i];
		
		v.v += v.f * dt;
		v.p += v.v * dt;
	}
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
	guiContext.init();
	
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
	float latticeTension = 1.f;
	
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
			ImGui::Begin("Interaction");
			{
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
					lattice.init();
				if (ImGui::Button("Project lattice onto shape"))
					projectLatticeOntoShape(lattice, shapeDefinition);
				ImGui::Checkbox("Show lattice vertices", &showLatticeVertices);
				ImGui::Checkbox("Show lattice edges", &showLatticeEdges);
				ImGui::Checkbox("Simulate lattice", &simulateLattice);
				ImGui::SliderFloat("Lattice tension", &latticeTension, .1f, 100.f);
				if (ImGui::Button("Squash lattice"))
				{
					const int numVertices = 6 * kTextureSize * kTextureSize;
					for (int i = 0; i < numVertices; ++i)
						lattice.vertices[i].p *= .95f;
				}
				
				ImGui::Separator();
				ImGui::Text("Statistics");
				ImGui::LabelText("Cube size (kbyte)", "%lu", sizeof(Cube) / 1024);
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
			::simulateLattice(lattice, dt, latticeTension);
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
					setColor(colorGreen);
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

	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
