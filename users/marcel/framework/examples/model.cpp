#include "framework.h"

#define VIEW_SX (1000/2)
#define VIEW_SY (1000/2)

int main(int argc, char * argv[])
{
	changeDirectory("data");
	
	framework.fullscreen = false;
	if (!framework.init(argc, argv, VIEW_SX, VIEW_SY))
		return -1;
	
	Model model("model.txt");
	
	const std::vector<std::string> animList = model.getAnimList();
	for (size_t i = 0; i < animList.size(); ++i)
		log("anim: %s", animList[i].c_str());
	
	int drawFlags = DrawMesh | DrawColorNormals;
	bool wireframe = false;
	bool rotate = false;
	bool loop = false;
	bool autoPlay = false;
	
	float angle = 180.f;
	
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
			model.animSpeed *= 2.f;
		if (keyboard.wentDown(SDLK_DOWN))
			model.animSpeed /= 2.f;
		
		if (autoPlay && !model.animIsActive)
			startRandomAnim = true;
		
		if (startRandomAnim)
		{
			const std::string & name = animList[rand() % animList.size()];
			
			model.startAnim(name.c_str(), loop ? -1 : 1);
		}
		
		if (rotate)
			angle += framework.timeStep * 10.f;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			glClearDepth(1.f);
			glClear(GL_DEPTH_BUFFER_BIT);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			glColor3ub(255, 255, 255);
			
			glPushMatrix();
			{
				glTranslatef(0.f, -.5f, 0.f);
				glRotatef(angle, 0.f, 1.f, 0.f);
				
				glRotatef(-90.f, 1.f, 0.f, 0.f); // fix up vector
				const float scale = 0.005f;
				glScalef(scale, scale, scale);
				
				model.draw(drawFlags);
			}
			glPopMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
