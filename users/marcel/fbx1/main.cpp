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
	
	{
		int x = 0;
		x -= getTimeMS();
		std::vector<unsigned char> bytes;
		
		if (!LoaderFbxBinary::readFile(filename, bytes))
		{
			log(logIndent, "failed to open %s\n", filename);
			return -1;
		}
		x += getTimeMS();
		printf("load took %d ms\n", x);
	}
	
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
		//if (SDL_SetVideoMode(640, 480, 32, SDL_OPENGL) < 0)
		if (SDL_SetVideoMode(1200, 900, 32, SDL_OPENGL) < 0)
		{
			log(0, "failed to intialize SDL");
			exit(-1);
		}
		
		bool wireframe = false;
		bool lightEnabled = false;
		bool drawBlendWeights = false;
		bool drawBlendIndices = false;
		bool drawTexcoords = false;
		bool drawNormalColors = true;
		
		int drawFlags = DrawMesh;
		
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
					else if (e.key.keysym.sym == SDLK_l)
						lightEnabled = !lightEnabled;						
					else if (e.key.keysym.sym == SDLK_F1)
						drawBlendWeights = !drawBlendWeights;
					else if (e.key.keysym.sym == SDLK_F2)
						drawBlendIndices = !drawBlendIndices;
					else if (e.key.keysym.sym == SDLK_F3)
						drawTexcoords = !drawTexcoords;
					else if (e.key.keysym.sym == SDLK_F4)
						drawNormalColors = !drawNormalColors;
				}
			}
			
			// process animation
			
			static float time = 0.f;
			time += 1.f / 60.f;
			
			// draw
			
			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClearDepth(1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glColor3ub(255, 255, 255);
			
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			
			static float r = 130.f;
			r += 1.f / 10.f;
			
			Vec4 light(1.f, 1.f, 1.f, 0.f);
			light.Normalize();
			glLightfv(GL_LIGHT0, GL_POSITION, &light[0]);
			glEnable(GL_LIGHT0);
			if (lightEnabled)
				glEnable(GL_LIGHTING);
			else
				glDisable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);
			
			glPushMatrix();
			
			glTranslatef(0.f, -.5f, 0.f);
			
			glRotatef(r, 0.f, 1.f, 0.f);
			
		#if 1
			glRotatef(-90.f, 1.f, 0.f, 0.f);
		#else
			glTranslatef(0.f, .5f, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
		#endif
			const float scale = 0.005f;
			glScalef(scale, scale, scale);
			
			model->process(1.f / 60.f);
			model->draw(drawFlags);
			
//			glTranslatef(150.f, 0.f, 0.f);
			
		#if 0
			for (std::list<MeshBuilder>::iterator m = meshes.begin(); m != meshes.end(); ++m)
			{
				const MeshBuilder & mesh = *m;
				
				int vertexCount = 0;
				
				for (size_t i = 0; i < mesh.m_indices.size(); ++i)
//				for (size_t i = 0; i < mesh.m_indices.size(); i = ((i + 1) % 3) == 0 ? i + 27 + 1 : i + 1)
//				for (size_t i = 0; i < mesh.m_indices.size(); i = ((i + 1) % 3) == 0 ? i + 6 + 1 : i + 1)
				{
					bool begin = vertexCount == 0;
					
					if (begin)
					{
						glBegin(GL_POLYGON);
					}
					
					int index = mesh.m_indices[i];
					
					bool end = false;
					
					if (index < 0)
					{
						// negative indices mark the end of a polygon
						
						index = ~index;
						
						end = true;
					}
					
					const MeshBuilder::Vertex & v = mesh.m_vertices[index];
					
					// -- software vertex blend (soft skinned) --
					Vec3 p(0.f, 0.f, 0.f);
					Vec3 n(0.f, 0.f, 0.f);
					for (int j = 0; j < 4; ++j)
					{
						if (v.boneWeights[j] == 0)
							continue;
						const int boneIndex = v.boneIndices[j];
						const float boneWeight = v.boneWeights[j] / 255.f;
						const Mat4x4 & objectToBone = objectToBoneMatrices[boneIndex];
						const Mat4x4 & boneToObject = boneToObjectMatrices[boneIndex];
						p += (boneToObject * objectToBone).Mul4(Vec3(v.px, v.py, v.pz)) * boneWeight;
						n += (boneToObject * objectToBone).Mul (Vec3(v.nx, v.ny, v.nz)) * boneWeight;
					}
					// -- software vertex blend (soft skinned) --
					
					float r = 1.f;
					float g = 1.f;
					float b = 1.f;
					
					if (drawNormalColors)
					{
						r *= (n[0] + 1.f) / 2.f;
						g *= (n[1] + 1.f) / 2.f;
						b *= (n[2] + 1.f) / 2.f;
					}
					if (drawBlendIndices)
					{
						r *= v.boneIndices[0] / float(modelNameToBoneIndex.size());
						g *= v.boneIndices[1] / float(modelNameToBoneIndex.size());
						b *= v.boneIndices[2] / float(modelNameToBoneIndex.size());
					}
					if (drawBlendWeights)
					{
						r *= (1.f + v.boneWeights[0] / 255.f) / 2.f;
						g *= (1.f + v.boneWeights[1] / 255.f) / 2.f;
						b *= (1.f + v.boneWeights[2] / 255.f) / 2.f;
					}
					if (drawTexcoords)
					{
						r *= v.tx;
						g *= v.ty;
					}
					
					glColor3f(r, g, b);
					glNormal3f(n[0], n[1], n[2]);
					glVertex3f(p[0], p[1], p[2]);
					
					vertexCount++;
					
					if (end)
					{
						glEnd();
						
						vertexCount = 0;
					}
				}
				
				if (vertexCount != 0)
				{
					assert(false);
					
					glEnd();
				}
			}
		#endif
			
			glPopMatrix();
			
			SDL_GL_SwapBuffers();
		}

		
		
		SDL_Quit();
	}
	
	return 0;
}
