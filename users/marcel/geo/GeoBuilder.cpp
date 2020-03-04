#include "GeoBuilder.h"

// todo : add quad shape

// todo : add grid shape

namespace Geo
{

	Builder& Builder::push()
	{
	
		matrixStack.Push();
		
		return *this;
		
	}
	
	Builder& Builder::pop()
	{
	
		matrixStack.Pop();
		
		return *this;
		
	}
	
	Builder& Builder::scale(float s)
	{
	
		matrixStack.ApplyScaling(Vec3(s, s, s));
		
		return *this;
		
	}
	
	Builder& Builder::scale(float sx, float sy, float sz)
	{
	
		matrixStack.ApplyScaling(Vec3(sx, sy, sz));
		
		return *this;
		
	}
	
	Builder& Builder::rotate(float angle, float axisX, float axisY, float axisZ)
	{
	
		matrixStack.ApplyRotationAngleAxis(angle, Vec3(axisX, axisY, axisZ));
		
		return *this;
		
	}
	
	Builder& Builder::translate(float x, float y, float z)
	{
	
		matrixStack.ApplyTranslation(Vec3(x, y, z));
		
		return *this;
	
	}

	Builder& Builder::transform(Poly* poly)
	{

		for (std::list<Vertex*>::iterator i = poly->vertices.begin(); i != poly->vertices.end(); ++i)
		{

			(*i)->position = matrixStack.GetMatrix() * (*i)->position;

		}
		
		return *this;

	}
	
	//
	
	Builder& Builder::grid(Mesh& mesh, int axis, int resolution1, int resolution2, Vec3Arg origin)
	{
		
		for (int i1 = 0; i1 < resolution1; ++i1)
		{
		
			for (int i2 = 0; i2 < resolution2; ++i2)
			{
			
				float v11 = (i1 + 0) / (resolution1 / 2.0f) - 1.0f;
				float v12 = (i2 + 0) / (resolution2 / 2.0f) - 1.0f;

				float v21 = (i1 + 1) / (resolution1 / 2.0f) - 1.0f;
				float v22 = (i2 + 0) / (resolution2 / 2.0f) - 1.0f;
				
				float v31 = (i1 + 1) / (resolution1 / 2.0f) - 1.0f;
				float v32 = (i2 + 1) / (resolution2 / 2.0f) - 1.0f;
				
				float v41 = (i1 + 0) / (resolution1 / 2.0f) - 1.0f;
				float v42 = (i2 + 1) / (resolution2 / 2.0f) - 1.0f;
				
				Geo::Poly* poly = mesh.Add();
				
				Geo::Vertex* vertex1 = poly->Add();
				Geo::Vertex* vertex2 = poly->Add();
				Geo::Vertex* vertex3 = poly->Add();
				Geo::Vertex* vertex4 = poly->Add();
				
				vertex1->position[(axis + 1) % 3] = v11;
				vertex1->position[(axis + 2) % 3] = v12;
				
				vertex2->position[(axis + 1) % 3] = v21;
				vertex2->position[(axis + 2) % 3] = v22;

				vertex3->position[(axis + 1) % 3] = v31;
				vertex3->position[(axis + 2) % 3] = v32;

				vertex4->position[(axis + 1) % 3] = v41;
				vertex4->position[(axis + 2) % 3] = v42;
				
				vertex1->position += origin;
				vertex2->position += origin;
				vertex3->position += origin;
				vertex4->position += origin;
			
			}
			
		}
		
		return *this;

	}

}
