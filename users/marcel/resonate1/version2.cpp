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

static Mat4x4 s_cubeFaceMatrices[6];

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
	
	float intersectRay(Vec3Arg rayDirection) const
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
	guiContext.init();
	
	for (int i = 0; i < 6; ++i)
	{
		s_cubeFaceMatrices[i] = createCubeFaceMatrix(s_cubeFaceNamesGL[i]);
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
	
	int numPlanesForRandomization = ShapeDefinition::kMaxPlanes;
	
	bool showCube = false;
	bool showIntersectionPoints = true;
	bool showAxis = true;
	bool showCubePoints = false;
	float cubePointScale = 1.f;
	bool projectCubePoints = false;
	bool colorizeCubePoints = false;
	
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
				
				ImGui::Separator();
				ImGui::Text("Shape generation");
				ImGui::SliderInt("Shape planes", &numPlanesForRandomization, 0, ShapeDefinition::kMaxPlanes);
				
				if (ImGui::Button("Randomize shape"))
				{
					shapeDefinition.makeRandomShape(numPlanesForRandomization);
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
						gxMultMatrixf(s_cubeFaceMatrices[i].m_v);
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
						const Mat4x4 & matrix = s_cubeFaceMatrices[i];
						
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
									const float t = shapeDefinition.intersectRay(p);
									
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
					setColor(colorWhite);
					gxBegin(GL_POINTS);
					for (int i = 0; i < 10000; ++i)
					{
						const float dx = random(-1.f, +1.f);
						const float dy = random(-1.f, +1.f);
						const float dz = random(-1.f, +1.f);
						
						const Vec3 rayDirection(dx, dy, dz);
						
						const float t = shapeDefinition.intersectRay(rayDirection);
						
						const Vec3 pointOfIntersection = rayDirection * t;
						
						gxVertex3f(
							pointOfIntersection[0],
							pointOfIntersection[1],
							pointOfIntersection[2]);
					}
					gxEnd();
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
	
	glDeleteTextures(6, textureGL);
	
	delete cube;
	cube = nullptr;

	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
