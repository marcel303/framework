#include "framework.h"

#include <time.h>
static int getTimeUS() { return clock() * 1000000 / CLOCKS_PER_SEC; }

#define VIEW_SX (1600/2)
#define VIEW_SY (1600/2)

#if 0
	#define X1 0
	#define X2 0
	#define Y1 0
	#define Y2 0
	#define Z1 0
	#define Z2 0
#else
	#define X1 -2
	#define X2 +2
	#define Y1 -1
	#define Y2 +1
	#define Z1 0
	#define Z2 0
#endif

class CoordKey
{
public:
	int values[3];
	
	CoordKey(int x, int y, int z)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
	}
	
	bool operator<(const CoordKey & other) const
	{
		for (int i = 0; i < 3; ++i)
			if (values[i] != other.values[i])
				return values[i] < other.values[i];
		return false;
	}
};

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, argv, VIEW_SX, VIEW_SY))
		return -1;
	
	std::map<CoordKey, Model*> models;
	
	for (int x = X1; x <= X2; ++x)
		for (int y = Y1; y <= Y2; ++y)
			for (int z = Z1; z <= Z2; ++z)
				models[CoordKey(x, y, z)] = new Model("model.txt");
	
	const std::vector<std::string> animList = models[CoordKey(0, 0, 0)]->getAnimList();
	
	for (size_t i = 0; i < animList.size(); ++i)
		log("anim: %s", animList[i].c_str());
	
	int drawFlags = DrawMesh | DrawColorNormals;
	bool wireframe = false;
	bool rotate = false;
	bool loop = false;
	bool autoPlay = false;
	float animSpeed = 1.f;
	
	float angle = 0.f;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		bool startRandomAnim = false;
		
		if (keyboard.wentDown(SDLK_w))
			wireframe = !wireframe;
		if (keyboard.wentDown(SDLK_b))
			drawFlags ^= DrawBones;
		if (keyboard.wentDown(SDLK_m))
			drawFlags = drawFlags ^ DrawPoseMatrices;
		if (keyboard.wentDown(SDLK_n))
			drawFlags = drawFlags ^ DrawNormals;
		if (keyboard.wentDown(SDLK_F1))
			drawFlags ^= DrawColorBlendWeights;
		if (keyboard.wentDown(SDLK_F2))
			drawFlags ^= DrawColorBlendIndices;
		if (keyboard.wentDown(SDLK_F3))
			drawFlags ^= DrawColorTexCoords;
		if (keyboard.wentDown(SDLK_F4))
			drawFlags ^= DrawColorNormals;
		if (keyboard.wentDown(SDLK_SPACE))
			startRandomAnim = true;
		if (keyboard.wentDown(SDLK_a))
			rotate = !rotate;
		if (keyboard.wentDown(SDLK_l))
			loop = !loop;
		if (keyboard.wentDown(SDLK_p))
			autoPlay = !autoPlay;
		if (keyboard.wentDown(SDLK_UP))
			animSpeed *= 2.f;
		if (keyboard.wentDown(SDLK_DOWN))
			animSpeed /= 2.f;
		
		for (int x = X1; x <= X2; ++x)
		{
			for (int y = Y1; y <= Y2; ++y)
			{
				for (int z = Z1; z <= Z2; ++z)
				{
					bool startRandomAnimForModel = startRandomAnim;
					
					Model * model = models[CoordKey(x, y, z)];
					
					if (autoPlay &&  !model->animIsActive)
						startRandomAnimForModel = true;
					
					if (startRandomAnimForModel)
					{
						for (int x = X1; x <= X2; ++x)
						{
							for (int y = Y1; y <= Y2; ++y)
							{
								for (int z = Z1; z <= Z2; ++z)
								{
									const std::string & name = animList[rand() % animList.size()];
									
									model->startAnim(name.c_str(), loop ? -1 : 1);
									model->animSpeed = animSpeed;
								}
							}
						}
					}
				}
			}
		}
		
		if (rotate)
		{
			angle += framework.timeStep * 10.f;
		}
		
		// set 3D transform
		
		const float fov = 90.f * M_PI / 180.f;
		const float aspect = 1.f;
		
		Mat4x4 transform3d;
		transform3d.MakePerspectiveGL(fov, aspect, .1f, +2000.f);
		setTransform3d(transform3d);
		
		framework.beginDraw(31, 0, 0, 0);
		{
			glClearDepth(1.f);
			glClear(GL_DEPTH_BUFFER_BIT);
			
			// switch to 3D drawing mode
			
			setTransform(TRANSFORM_3D);
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			
			glPushMatrix();
			{
				glTranslatef(0.f, -100.f, 600.f);
				glRotatef(angle, 0.f, 1.f, 0.f);
				
				setBlend(BLEND_OPAQUE);
				setColor(255, 255, 255);
				
				int time = 0;
				time -= getTimeUS();
				
				for (int x = X1; x <= X2; ++x)
				{
					for (int y = Y1; y <= Y2; ++y)
					{
						for (int z = Z1; z <= Z2; ++z)
						{
							Model * model = models[CoordKey(x, y, z)];
							model->x = x * 200.f;
							model->y = y * 200.f;
							model->z = 0.f;
							model->draw(drawFlags);
						}
					}
				}
				
				time += getTimeUS();
				log("draw took %.2fms", time / 1000.f);
				
				Font font("calibri.ttf");
				setFont(font);
				setColor(255, 255, 0);
				const Vec2 s = transformToScreen(Vec3(0.f, 0.f, 0.f));
				debugDrawText(s[0], s[1], 18, 0, 0, "root");
				setColor(255, 255, 255);
			}
			glPopMatrix();
			glDisable(GL_DEPTH_TEST);
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
