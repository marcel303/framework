#include <GL/glew.h>
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include "Quat.h"

#include "../framework/image.h"
#include "../framework/model.h"
#include "../framework/model_ogre.h"

using namespace Model;

AnimModel * loadModel(const char * meshFileName, const char * skeletonFileName)
{
	LoaderOgreXML loader;
	
	BoneSet * boneSet = loader.loadBoneSet(skeletonFileName);
	MeshSet * meshSet = loader.loadMeshSet(meshFileName, boneSet);
	AnimSet * animSet = loader.loadAnimSet(skeletonFileName, boneSet);
	
	AnimModel * model = new AnimModel(meshSet, boneSet, animSet);
	
	return model;
}

int main(int argc, char * argv[])
{
	(void)argc;
	(void)argv;
	
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
	
	float angle = 0.f;
	bool wireframe = false;
	bool rotate = false;
	bool drawBones = false;
	bool drawNormals = false;
	bool loop = false;
	bool autoPlay = false;
	bool light = true;
	
	const int modelId = 0;
	
	ImageData * imageData = loadImage("ninja.png");
	if (imageData)
	{
		GLuint texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, imageData->sx);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageData->sx, imageData->sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData->imageData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEnable(GL_TEXTURE_2D);
		delete imageData;
	}
	
	for (int i = 0; i < 1; ++i)
	{
		AnimModel * model = 0;
		
		if (modelId == 0)
			model = loadModel("mesh.xml", "skeleton.xml");
		else
			model = loadModel("men_alrike_mesh.xml", "men_alrike_skeleton.xml");
		
		bool stop = false;
		
		while (!stop)
		{
			bool startRandomAnimation = false;
			
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_w)
					{
						wireframe = !wireframe;
					}
					else if (e.key.keysym.sym == SDLK_r)
					{
						rotate = !rotate;
					}
					else if (e.key.keysym.sym == SDLK_b)
					{
						drawBones = !drawBones;
					}
					else if (e.key.keysym.sym == SDLK_n)
					{
						drawNormals = !drawNormals;
					}
					else if (e.key.keysym.sym == SDLK_l)
					{
						loop = !loop;
					}
					else if (e.key.keysym.sym == SDLK_p)
					{
						autoPlay = !autoPlay;
					}
					else if (e.key.keysym.sym == SDLK_i)
					{
						light = !light;
					}
					else if (e.key.keysym.sym == SDLK_SPACE)
					{
						startRandomAnimation = true;
					}
					else if (e.key.keysym.sym == SDLK_UP)
					{
						model->animSpeed *= 2.f;
					}
					else if (e.key.keysym.sym == SDLK_DOWN)
					{
						model->animSpeed /= 2.f;
					}
					else if (e.key.keysym.sym == SDLK_ESCAPE)
					{
						stop = true;
					}
				}
			}
			
			if (autoPlay && model->animIsDone)
			{
				startRandomAnimation = true;
			}
			
			if (startRandomAnimation)
			{
				std::vector<std::string> names;
				
				for (std::map<std::string, Anim*>::iterator i = model->m_animations->m_animations.begin(); i != model->m_animations->m_animations.end(); ++i)
					names.push_back(i->first);
				
				const size_t index = rand() % names.size();
				const char * name = names[index].c_str();
				
				printf("anim: %s\n", name);					
				model->startAnim(name, loop ? -1 : 1);
				
				if (model->currentAnim)
				{
					int currentKey = 0;
					
					for (int i = 0; i < model->m_bones->m_numBones; ++i)
					{
						const int numKeys = model->currentAnim->m_numKeys[i];
						
						printf("bone %3d: numKeys: %5d endTime:%6.2f\n",
							i, numKeys, numKeys ? model->currentAnim->m_keys[currentKey + numKeys - 1].time : 0.f);
						
						currentKey += numKeys;
					}
				}
			}
			
			const float timeStep = 1.f / 60.f / 1.f;
			
			if (rotate)
			{
				angle += 45.f * timeStep;
			}
			
			model->process(timeStep);
			
			glClearColor(0, 0, 0, 0);
			glClearDepth(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			float scale = 1.f;
			if (modelId == 0)
				scale = 1.f / 150.f;
			else
				scale = 1.f;
			glScalef(scale, scale, scale);
			
			glEnable(GL_LIGHT0);
			GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glEnable(GL_NORMALIZE);
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			
			float ty = 0.f;
			if (modelId == 0)
				ty = -100.f;
			else
				ty = -1.f;
			glTranslatef(0.f, ty, 0.f);
			glRotatef(angle, 0.f, 1.f, 0.f);
			glRotatef(90, 0.f, 1.f, 0.f);
			
			const int num = 0;
			
			for (int x = -num; x <= +num; ++x)
			{
				for (int y = -num; y <= +num; ++y)
				{
					Mat4x4 matrix;
					matrix.MakeTranslation(x / scale, 0.f, y / scale);
					
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_GREATER);
					if (light)
						glEnable(GL_LIGHTING);
					
					glColor3ub(255, 255, 255);
					model->drawEx(matrix, DrawMesh | (drawNormals ? DrawNormals : 0));
					
					if (light)
						glDisable(GL_LIGHTING);
					glDisable(GL_DEPTH_TEST);
					
					if (drawBones)
					{
						glColor3ub(0, 255, 0);
						model->drawEx(matrix, DrawBones);
					}
				}
			}
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
			SDL_GL_SwapBuffers();
		}
		
		delete model;
		model = 0;
	}
	
	SDL_Quit();
	
	return 0;
}
