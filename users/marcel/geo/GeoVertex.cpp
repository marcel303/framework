#include "GeoVertex.h"

namespace Geo
{

	Vertex::Vertex()
	{

	}

	Vertex::Vertex(const Vertex& vertex)
	{

		*this = vertex;
		
	}

	Vertex::~Vertex()
	{

	}

	PlaneClassification Vertex::Classify(const Plane& plane) const
	{

		return plane.Classify(position);
		
	}

	Vertex& Vertex::operator=(const Vertex& vertex)
	{

		this->position = vertex.position;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			varying[i] = vertex.varying[i];
		}
		
		return (*this);
		
	}

	Vertex Vertex::operator+(const Vertex& vertex) const
	{

		Vertex temp;
		
		temp.position = position + vertex.position;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			temp.varying[i] = varying[i] + vertex.varying[i];
		}
		
		return temp;
		
	}

	Vertex Vertex::operator-(const Vertex& vertex) const
	{

		Vertex temp;
		
		temp.position = position - vertex.position;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			temp.varying[i] = varying[i] - vertex.varying[i];
		}
		
		return temp;
		
	}

	Vertex Vertex::operator*(const float v) const
	{

		Vertex temp;
		
		temp.position = position * v;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			temp.varying[i] = varying[i] * v;
		}
		
		return temp;
		
	}

	Vertex& Vertex::operator+=(const Vertex& vertex)
	{

		position += vertex.position;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			varying[i] += vertex.varying[i];
		}

		return (*this);
		
	}

	Vertex& Vertex::operator-=(const Vertex& vertex)
	{

		position -= vertex.position;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			varying[i] -= vertex.varying[i];
		}
		
		return (*this);
		
	}

	Vertex& Vertex::operator*=(const float v)
	{

		position *= v;
		
		for (int i = 0; i < kMaxVaryings; ++i)
		{
			varying[i] *= v;
		}
		
		return (*this);

	}

}
