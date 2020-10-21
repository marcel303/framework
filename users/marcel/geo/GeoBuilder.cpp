#include "GeoBuilder.h"

namespace Geo
{

	Builder::Builder()
	{
	
		mesh = nullptr;
		
	}
	
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
	
	Builder& Builder::pushScale(float s)
	{
	
		push();
		
		scale(s);
		
		return *this;
		
	}
	
	Builder& Builder::pushScale(float sx, float sy, float sz)
	{
	
		push();
		
		scale(sx, sy, sz);
		
		return *this;
		
	}
	
	Builder& Builder::pushRotate(float angle, float axisX, float axisY, float axisZ)
	{
	
		push();
		
		rotate(angle, axisX, axisY, axisZ);
		
		return *this;
		
	}
	
	Builder& Builder::pushTranslate(float x, float y, float z)
	{
	
		push();
		
		translate(x, y, z);
		
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
	
	Builder& Builder::begin(Mesh& mesh)
	{
	
		push();
		
		this->mesh = &mesh;
		
		return *this;
	
	}
	
	Builder& Builder::end()
	{
	
		pop();
		
		mesh->Finalize();
		
		mesh = nullptr;
		
		return *this;
		
	}

}
