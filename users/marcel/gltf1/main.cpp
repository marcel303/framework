#include "framework.h"
#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "gx_mesh.h"
#include "Quat.h"

#include "data/engine/ShaderCommon.txt"

#define VIEW_SX 1000
#define VIEW_SY 600

#define USE_BUFFER_CACHE 1 // create static vertex and index buffers from gltf resources

#define ANIMATED_CAMERA 1 // todo : remove option and use hybrid

#define LOW_LATENCY_HACK_TEST 0 // todo : remove

#if LOW_LATENCY_HACK_TEST
	#include <unistd.h>
#endif

#if ANIMATED_CAMERA

struct AnimatedCamera3d
{
	Vec3 position;
	Vec3 lookatTarget;
	
	Vec3 desiredPosition;
	Vec3 desiredLookatTarget;
	bool animate = false;
	float animationSpeed = 1.f;
	
	void tick(const float dt, const bool inputIsCaptured)
	{
		if (inputIsCaptured == false)
		{
		
		}
		
		if (animate)
		{
			const float retain = powf(1.f - animationSpeed, dt);
			const float attain = 1.f - retain;
			
			position = lerp(position, desiredPosition, attain);
			lookatTarget = lerp(lookatTarget, desiredLookatTarget, attain);
		}
	}
	
	Mat4x4 getWorldMatrix() const
	{
		return getViewMatrix().CalcInv();
	}

	Mat4x4 getViewMatrix() const
	{
		Mat4x4 m;
		m.MakeLookat(position, lookatTarget, Vec3(0, 1, 0));
		return m;
	}

	void pushViewMatrix() const
	{
		const Mat4x4 matrix = getViewMatrix();
		
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxMultMatrixf(matrix.m_v);
		}
		gxMatrixMode(restoreMatrixMode);
	}

	void popViewMatrix() const
	{
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
		}
		gxMatrixMode(restoreMatrixMode);
	}
};

#endif

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	//const char * path = "van_gogh_room/scene.gltf";
	//const char * path = "littlest_tokyo/scene.gltf";
	const char * path = "ftm/scene.gltf";
	//const char * path = "nara_the_desert_dancer_free_download/scene.gltf";
	//const char * path = "halloween_little_witch/scene.gltf";

	gltf::Scene scene;
	
	if (!gltf::loadScene(path, scene))
	{
		logError("failed to load GLTF file");
	}

#if USE_BUFFER_CACHE
	std::map<int, GxVertexBuffer*> vertexBuffers;
	std::map<int, GxIndexBuffer*> indexBuffers;
	std::map<const gltf::Mesh*, GxMesh*> meshes;
	
	// create vertex buffers from buffer objects
	
	int bufferIndex = 0;
	
	for (auto & buffer : scene.buffers)
	{
		GxVertexBuffer * vertexBuffer = new GxVertexBuffer();
		vertexBuffer->alloc(&buffer.data.front(), buffer.byteLength);
		
		Assert(vertexBuffers[bufferIndex] == nullptr);
		vertexBuffers[bufferIndex++] = vertexBuffer;
	}
	
	// create index buffers and meshes for mesh primitives
	
	for (auto & mesh : scene.meshes)
	{
		for (auto & primitive : mesh.primitives)
		{
			GxIndexBuffer * indexBuffer = nullptr;
			
			//
			
			auto indexBuffer_itr = indexBuffers.find(primitive.indices);
			
			if (indexBuffer_itr != indexBuffers.end())
			{
				indexBuffer = indexBuffer_itr->second;
			}
			else
			{
				const gltf::Accessor * accessor;
				const gltf::BufferView * bufferView;
				const gltf::Buffer * buffer;
				
				if (!gltf::resolveBufferView(scene, primitive.indices, accessor, bufferView, buffer))
				{
					logWarning("failed to resolve buffer view");
				}
				else
				{
					if (accessor->componentType != gltf::kElementType_U16 &&
						accessor->componentType != gltf::kElementType_U32)
					{
						logWarning("index element type not supported");
						continue;
					}
					
					indexBuffer = new GxIndexBuffer();
					const uint8_t * index_mem = &buffer->data.front() + bufferView->byteOffset + accessor->byteOffset;
					
					const GX_INDEX_FORMAT format =
						accessor->componentType == gltf::kElementType_U16
						? GX_INDEX_16
						: GX_INDEX_32;
					
					indexBuffer->alloc(index_mem, accessor->count, format);
					
					Assert(indexBuffers[primitive.indices] == nullptr);
					indexBuffers[primitive.indices] = indexBuffer;
				}
			}
			
			// create mapping between vertex buffer and vertex shader
			
			std::vector<GxVertexInput> vertexInputs;
			
			int vertexBufferIndex = -1;
			
			for (auto & attribute : primitive.attributes)
			{
				const gltf::Accessor * accessor;
				const gltf::BufferView * bufferView;
				const gltf::Buffer * buffer;
	
				const std::string & attributeName = attribute.first;
				const int accessorIndex = attribute.second;
	
				if (!gltf::resolveBufferView(scene, accessorIndex, accessor, bufferView, buffer))
				{
					logWarning("failed to resolve buffer view");
					continue;
				}
				
				if (vertexBufferIndex == -1)
					vertexBufferIndex = bufferView->buffer;
				else if (bufferView->buffer != vertexBufferIndex)
					vertexBufferIndex = -2;
	
				/*
				POSITION,
				NORMAL,
				TANGENT,
				TEXCOORD_0,
				TEXCOORD_1,
				COLOR_0,
				JOINS_0, (bone indices)
				WEIGHTS_0
				
				note : bitangent = cross(normal, tangent.xyz) * tangent.w
				*/
				
				const int id =
					attributeName == "POSITION" ? VS_POSITION :
					attributeName == "NORMAL" ? VS_NORMAL :
					attributeName == "COLOR_0" ? VS_COLOR :
					attributeName == "TEXCOORD_0" ? VS_TEXCOORD0 :
					attributeName == "TEXCOORD_1" ? VS_TEXCOORD1 :
					attributeName == "JOINTS_0" ? VS_BLEND_INDICES :
					attributeName == "WEIGHTS_0" ? VS_BLEND_WEIGHTS :
					-1;
				
				if (id == -1)
				{
					//logDebug("unknown attribute: %s", attributeName.c_str());
					continue;
				}
				
				const int numComponents =
					accessor->type == "SCALAR" ? 1 :
					accessor->type == "VEC2" ? 2 :
					accessor->type == "VEC3" ? 3 :
					accessor->type == "VEC4" ? 4 :
					-1;
				
				if (numComponents == -1)
				{
					logWarning("number of components not supported");
					continue;
				}
				
				const GX_ELEMENT_TYPE type =
					accessor->type == "SCALAR" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC2" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC3" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC4" ? GX_ELEMENT_FLOAT32 :
					(GX_ELEMENT_TYPE)-1;
				
				if (type == (GX_ELEMENT_TYPE)-1)
				{
					logWarning("element type not supported");
					continue;
				}
				
				GxVertexInput v;
				v.id = id;
				v.numComponents = numComponents;
				v.type = type;
				v.normalize = accessor->normalized;
				v.offset = bufferView->byteOffset + accessor->byteOffset;
				v.stride = bufferView->byteStride;
				
				vertexInputs.push_back(v);
			}
			
			if (vertexBufferIndex < 0)
			{
				logWarning("invalid vertex buffer index");
				continue;
			}
			
			// create mesh
			
			Assert(vertexBuffers[vertexBufferIndex] != nullptr);
			
			GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
			
			GxMesh * gxMesh = new GxMesh();
			gxMesh->setVertexBuffer(vertexBuffer, &vertexInputs.front(), vertexInputs.size(), 0);
			gxMesh->setIndexBuffer(indexBuffer);
			
			Assert(meshes[&mesh] == nullptr);
			meshes[&mesh] = gxMesh;
		}
	}
#endif
	
#if ANIMATED_CAMERA
	AnimatedCamera3d camera;
#else
	Camera3d camera;
#endif
	
	camera.position = Vec3(0, 0, -2);
	
	bool centimeters = true;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_t))
			centimeters = !centimeters;
		
	#if ANIMATED_CAMERA
		if (keyboard.wentDown(SDLK_p))
		{
			gltf::BoundingBox boundingBox;
			
			if (scene.activeScene >= 0 && scene.activeScene < scene.sceneRoots.size())
			{
				auto & sceneRoot = scene.sceneRoots[scene.activeScene];
			
				for (auto & node_index : sceneRoot.nodes)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						auto & node = scene.nodes[node_index];
						
						calculateNodeMinMaxTraverse(scene, node, boundingBox);
					}
				}
			}
			
			const float distance = (boundingBox.max - boundingBox.min).CalcSize() / 2.f * .9f;
			const Vec3 target = (boundingBox.min + boundingBox.max) / 2.f;
			
			const float angle = random<float>(-M_PI, +M_PI);
			camera.desiredPosition = target + Mat4x4(true).RotateY(angle).GetAxis(2) * 10.f;
			camera.desiredLookatTarget = target;
			camera.animate = true;
			camera.animationSpeed = .9f;
		}
	#endif
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .1f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			pushBlend(BLEND_OPAQUE);
			camera.pushViewMatrix();
			{
				if (centimeters)
					gxScalef(-.01f, .01f, .01f);
				else
					gxScalef(-1, 1, 1);
				
				for (int i = 0; i < 2; ++i)
				{
					const bool isOpaquePass = (i == 0);
					
					if (scene.activeScene < 0 || scene.activeScene >= scene.sceneRoots.size())
					{
						logWarning("invalid scene index");
						continue;
					}
					
					pushDepthWrite(keyboard.isDown(SDLK_z) ? true : isOpaquePass ? true : false);
					{
						auto & sceneRoot = scene.sceneRoots[scene.activeScene];
						
						for (auto & node_index : sceneRoot.nodes)
						{
							if (node_index >= 0 && node_index < scene.nodes.size())
							{
								auto & node = scene.nodes[node_index];
								
								drawNodeTraverse(scene, node, isOpaquePass);
							}
						}
					}
					popDepthWrite();
				}
				
				if (keyboard.isDown(SDLK_b))
				{
					pushBlend(BLEND_ADD);
					pushDepthWrite(false);
					{
						if (scene.activeScene >= 0 && scene.activeScene < scene.sceneRoots.size())
						{
							auto & sceneRoot = scene.sceneRoots[scene.activeScene];
							
							for (auto & node_index : sceneRoot.nodes)
							{
								if (node_index >= 0 && node_index < scene.nodes.size())
								{
									auto & node = scene.nodes[node_index];
									
									drawNodeMinMaxTraverse(scene, node);
								}
							}
						}
					}
					popDepthWrite();
					popBlend();
				}
			}
			camera.popViewMatrix();
			popBlend();
			popDepthTest();
			
			projectScreen2d();
			
			setColor(0, 0, 0, 127);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(4, 4, VIEW_SX - 4, 90, 10.f);
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			setLumi(170);
			
			drawText(10, 30, 16, +1, -1, "(Extra) Author: %s", scene.asset.extras.author.c_str());
			drawText(10, 50, 16, +1, -1, "(Extra) License: %s", scene.asset.extras.license.c_str());
			drawText(10, 70, 16, +1, -1, "(Extra) Title: %s", scene.asset.extras.title.c_str());
		}
		framework.endDraw();
		
	#if LOW_LATENCY_HACK_TEST
		static uint64_t t1 = 0;
		static uint64_t t2 = 0;
		
		static int x = 0;
		x++;
		printf("frame: %d\n", x);
		
		t2 = SDL_GetTicks();
		
		const uint64_t time_ms = t2 - t1 + 1;
		
		if (time_ms < 16 && !keyboard.isDown(SDLK_RSHIFT))
		{
			const uint64_t delay_ms = 16 - time_ms;
			
			usleep(delay_ms * 1000);
		}
		
		t1 = SDL_GetTicks();
	#endif
	}
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
