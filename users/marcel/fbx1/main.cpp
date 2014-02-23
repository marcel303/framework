#include <assert.h>
#include <list>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include "fbx.h"

#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "Mat4x4.h"

#include "../framework/model.h"
#include "../framework/model_fbx.h"

using namespace Model;

// INTEGRATED
static bool logEnabled = true;

// INTEGRATED
static void log(int logIndent, const char * fmt, ...)
{
	if (logEnabled)
	{
		va_list va;
		va_start(va, fmt);
		
		char tabs[128];
		for (int i = 0; i < logIndent; ++i)
			tabs[i] = '\t';
		tabs[logIndent] = 0;
		
		char temp[1024];
		vsprintf(temp, fmt, va);
		va_end(va);
		
		printf("%s%s", tabs, temp);
	}
}

static int getTimeMS()
{
	return clock() * 1000 / CLOCKS_PER_SEC;
}

class FbxFileLogger
{
	int m_logIndent;
	
	const FbxReader * m_reader;
	
public:
	FbxFileLogger(const FbxReader & reader)
	{
		m_logIndent = 0;
		
		m_reader = &reader;
	}
	
	enum DumpMode
	{
		DumpNodes      = 0x1,
		DumpProperties = 0x2,
		DumpFirstFewProperties = 0x4 // only dump the first few properties.. not every vertex, index value, etc
	};
	
	// dumpFileContents iterates over the entire file contents and dumps every record and property
	
	void dumpFileContents(bool firstFewProperties)
	{
		dump(DumpNodes | DumpProperties | (firstFewProperties ? DumpFirstFewProperties : 0));
	}
	
	// dumpFileContents iterates over the entire file contents and dumps every record
	
	void dumpHierarchy()
	{
		dump(DumpNodes);
	}
	
	void dump(int dumpMode)
	{
		for (FbxRecord record = m_reader->firstRecord(); record.isValid(); record = record.nextSibling())
		{
			dumpRecord(record, dumpMode);
		}
	}
	
	void dumpRecord(const FbxRecord & record, int dumpMode)
	{
		if (dumpMode & DumpNodes)
		{
			log(m_logIndent, "node: endOffset=%d, numProperties=%d, name=%s\n",
				(int)record.getEndOffset(),
				(int)record.getNumProperties(),
				record.name.c_str());
		}
		
		m_logIndent++;
		
		std::vector<FbxValue> properties = record.captureProperties<FbxValue>();
		
		if (dumpMode & DumpProperties)
		{
			size_t numProperties = properties.size();
			
			if (dumpMode & DumpFirstFewProperties)
				if (numProperties > 4)
					numProperties = 4;
			
			for (size_t i = 0; i < numProperties; ++i)
			{
				dumpProperty(properties[i]);
			}
		}
		
		for (FbxRecord childRecord = record.firstChild(); childRecord.isValid(); childRecord = childRecord.nextSibling())
		{
			dumpRecord(childRecord, dumpMode);
		}
		
		m_logIndent--;
	}
	
	void dumpProperty(const FbxValue & value)
	{
		switch (value.type)
		{
			case FbxValue::TYPE_BOOL:
				log(m_logIndent, "bool: %d\n", get<bool>(value));
				break;
			case FbxValue::TYPE_INT:
				log(m_logIndent, "int: %lld\n", get<int64_t>(value));
				break;
			case FbxValue::TYPE_REAL:
				log(m_logIndent, "float: %f\n", get<float>(value));
				break;
			case FbxValue::TYPE_STRING:
				log(m_logIndent, "string: %s\n", value.getString());
				break;
			
			case FbxValue::TYPE_INVALID:
				log(m_logIndent, "(invalid)\n");
				break;
		}
	}
};

int main(int argc, char * argv[])
{
	int logIndent = 0;
	
	// parse command line
	
	bool dumpMeshes = false;
	bool drawMeshes = false;
	bool dumpAll = false;
	bool dumpAllButDoItSilently = false;
	bool dumpHierarchy = false;
	const char * filename = "test.fbx";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-v"))
			dumpAll = true;
		else if (!strcmp(argv[i], "-vs"))
			dumpAllButDoItSilently = true;
		else if (!strcmp(argv[i], "-h"))
			dumpHierarchy = true;
		else if (!strcmp(argv[i], "-m"))
			dumpMeshes = true;
		else if (!strcmp(argv[i], "-d"))
			drawMeshes = true;
		else
			filename = argv[i];
	}
	
	if (dumpAllButDoItSilently)
		dumpAll = true;
	
	if (dumpAll || dumpHierarchy)
	{
		// read file contents
		
		std::vector<unsigned char> bytes;
		
		if (!LoaderFbxBinary::readFile(filename, bytes))
		{
			log(logIndent, "failed to open %s\n", filename);
			return -1;
		}
		
		// open FBX file
		
		FbxReader reader;
		
		reader.openFromMemory(&bytes[0], bytes.size());
		
		// dump FBX contents
		
		FbxFileLogger logger(reader);
		
		if (dumpAll)
		{
			logger.dumpFileContents(dumpAllButDoItSilently);
		}
		
		if (dumpHierarchy)
		{
			logger.dumpHierarchy();
		}
	}
	
	// load model
	
	log(logIndent, "-- loading BoneSet + MeshSet + AnimSet --\n");
	
	int time = 0;
	
	time -= getTimeMS();
	
	LoaderFbxBinary loader;
	
	BoneSet * boneSet = loader.loadBoneSet(filename);
	MeshSet * meshSet = loader.loadMeshSet(filename, boneSet);
	AnimSet * animSet = loader.loadAnimSet(filename, boneSet);
	
	AnimModel * model = new AnimModel(meshSet, boneSet, animSet);
		
	time += getTimeMS();
	
	log(logIndent, "LoaderFbxBinary took %d ms\n", time);
	
	// dump mesh data
	
	if (dumpMeshes)
	{
		log(logIndent, "-- dumping mesh data --\n");
		
		for (int i = 0; i < meshSet->m_numMeshes; ++i)
		{
			const Mesh & mesh = *meshSet->m_meshes[i];
			
			log(logIndent, "mesh: numVertices=%d, numIndices=%d\n", mesh.m_numVertices, mesh.m_numIndices);
			
			logIndent++;
			
			for (int j = 0; j < mesh.m_numIndices; ++j)
			{
				int index = mesh.m_indices[j];
				
				if ((j % 3) == 0)
				{
					log(logIndent, "triangle [%d]:\n", j / 3);
				}
				
				logIndent++;
				
				log(logIndent, "[%05d] position = (%+7.2f %+7.2f %+7.2f), normal = (%+5.2f %+5.2f %+5.2f), color = (%4.2f %4.2f %4.2f %4.2f), uv = (%+5.2f %+5.2f)\n",
					index,
					mesh.m_vertices[index].px,
					mesh.m_vertices[index].py,
					mesh.m_vertices[index].pz,
					mesh.m_vertices[index].nx,
					mesh.m_vertices[index].ny,
					mesh.m_vertices[index].nz,
					mesh.m_vertices[index].cx,
					mesh.m_vertices[index].cy,
					mesh.m_vertices[index].cz,
					mesh.m_vertices[index].cw,
					mesh.m_vertices[index].tx,
					mesh.m_vertices[index].ty);
							
				logIndent--;
			}
			
			logIndent--;
		}
	}
	
	model->startAnim("Take 001", -1);
	
	if (drawMeshes)
	{
		// initialize SDL
		
		SDL_Init(SDL_INIT_EVERYTHING);
		
		if (SDL_SetVideoMode(1200, 900, 32, SDL_OPENGL) < 0)
		{
			log(logIndent, "failed to intialize SDL");
			exit(-1);
		}
		
		bool wireframe = false;
		int drawFlags = DrawMesh | DrawColorNormals;
		
		float time = 0.f;
		
		bool stop = false;
		
		while (!stop)
		{
			// process input
			
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE)
						stop = true;
					else if (e.key.keysym.sym == SDLK_w)
						wireframe = !wireframe;
					else if (e.key.keysym.sym == SDLK_b)
						drawFlags ^= DrawBones;
					else if (e.key.keysym.sym == SDLK_p)
						drawFlags = drawFlags ^ DrawPoseMatrices;
					else if (e.key.keysym.sym == SDLK_n)
						drawFlags = drawFlags ^ DrawNormals;
					else if (e.key.keysym.sym == SDLK_F1)
						drawFlags ^= DrawColorBlendWeights;
					else if (e.key.keysym.sym == SDLK_F2)
						drawFlags ^= DrawColorBlendIndices;
					else if (e.key.keysym.sym == SDLK_F3)
						drawFlags ^= DrawColorTexCoords;
					else if (e.key.keysym.sym == SDLK_F4)
						drawFlags ^= DrawColorNormals;
				}
			}
			
			// process animation
			
			const float timeStep = 1.f / 60.f;
			
			time += timeStep;
			
			model->process(timeStep);
			
			// draw
			
			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClearDepth(1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
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
				glRotatef(time * 30.f, 0.f, 1.f, 0.f);
				
				glRotatef(-90.f, 1.f, 0.f, 0.f); // fix up vector
				const float scale = 0.005f;
				glScalef(scale, scale, scale);
				
				model->draw(drawFlags);
			}
			glPopMatrix();
			
			SDL_GL_SwapBuffers();
		}
		
		SDL_Quit();
	}
	
	return 0;
}
