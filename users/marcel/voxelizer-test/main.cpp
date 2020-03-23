#include "framework.h"
#include "gx_mesh.h"

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer/voxelizer.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;

	// create a mesh
	
	const Vec3 position[3] =
	{
		Vec3(-1, -1, 0),
		Vec3(+1, -1, 0),
		Vec3(+1, +1, 0)
	};
	
	const int numInstances = 64;
	
	vx_mesh_t * mesh = vx_mesh_alloc(3 * numInstances, 3 * numInstances);
	
	for (int i = 0; i < numInstances; ++i)
	{
		const float x = random<float>(-16.f, +16.f);
		const float y = random<float>(-1.f,  +1.f);
		const float z = random<float>(-4.f,  +4.f);
		
		for (int j = 0; j < 3; ++j)
		{
			const int index = i * 3 + j;
			
			const int elem_offset = i % 3;
			
			mesh->vertices[index].x = x + position[j][(0 + elem_offset) % 3];
			mesh->vertices[index].y = y + position[j][(1 + elem_offset) % 3];
			mesh->vertices[index].z = z + position[j][(2 + elem_offset) % 3];
			
			mesh->indices[index] = index;
		}
	}
	
	// todo : voxelizer the mesh
	
	float precision = .0f; // precision factor to reduce "holes" artifact

	vx_mesh_t * result = vx_voxelize(mesh, .05f, .05f, .05f, precision);

	vx_mesh_free(mesh);
	mesh = nullptr;
	
	GxMesh drawMesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	
	gxCaptureMeshBegin(drawMesh, vb, ib);
	{
		gxBegin(GX_TRIANGLES);
		{
			for (size_t i = 0; i < result->nindices; ++i)
			{
				auto normal_index = result->normalindices[i];
				float r = (result->normals[normal_index].r + 1.f) * .3f;
				float g = (result->normals[normal_index].g + 1.f) * .3f;
				float b = (result->normals[normal_index].b + 1.f) * .3f;
				gxColor3f(r, g, b);
				
				auto index = result->indices[i];
				//gxColor3fv(result->colors[index].v);
				gxVertex3fv(result->vertices[index].v);
			}
		}
		gxEnd();
	}
	gxCaptureMeshEnd();
	
	vx_mesh_free(result);
	result = nullptr;
	
	Camera3d camera;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(255, 255, 255, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				// draw voxelized mesh
				
				drawMesh.draw();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}

	framework.shutdown();
	
	return 0;
}
