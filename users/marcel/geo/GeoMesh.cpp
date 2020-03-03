#include "Debugging.h"
#include "GeoMesh.h"
#include "Types.h"

namespace Geo
{

	Mesh::Mesh()
	{

		bFinalized = false;
		
	}

	Mesh::Mesh(const Mesh& mesh)
	{

		bFinalized = false;
		
		(*this) = mesh;
		
	}

	Mesh::~Mesh()
	{

		if (bFinalized)
		{
			Unfinalize();
		}
		
		Clear();
		
	}

	Poly* Mesh::Add()
	{

		Assert(!bFinalized);

		Poly* poly = new Poly;
		
		polys.push_back(poly);
		
		return poly;
		
	}

	void Mesh::Remove(Poly* poly)
	{

		Assert(poly);
		Assert(!bFinalized);

		for (std::list<Poly*>::iterator i = polys.begin(); i != polys.end(); ++i)
		{
			if ((*i) == poly)
			{
				delete (*i);
				polys.erase(i);
				return;
			}
		}
		
	}

	Poly* Mesh::Link(Poly* poly)
	{

		Assert(poly);
		Assert(!bFinalized);
		
		polys.push_back(poly);
		
		return poly;
		
	}

	Poly* Mesh::Unlink(Poly* poly)
	{

		Assert(poly);
		Assert(!bFinalized);
		
		for (std::list<Poly*>::iterator i = polys.begin(); i != polys.end(); ++i)
		{
			if ((*i) == poly)
			{
				polys.erase(i);
				return poly;
			}
		}
		
		return poly;
		
	}

	void Mesh::Clear()
	{

		Assert(!bFinalized);

		while (polys.empty() == false)
		{
			Remove(polys.front());
		}
		
	}

	void Mesh::Move(Mesh& mesh)
	{

		Assert(!bFinalized);
		
		while (polys.size() > 0)
		{
			Poly* poly = polys.front();
			Unlink(poly);
			mesh.Link(poly);
		}
		
	}

	void Mesh::RevertPolyWinding()
	{

		Assert(!bFinalized);

		for (std::list<Poly*>::iterator i = polys.begin(); i != polys.end(); ++i)
		{
			(*i)->RevertWinding();
		}
		
	}

	Aabb Mesh::CalculateExtents() const
	{

		Aabb aabb;
		
		bool bFirstPoly = true;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
		
			Aabb polyAabb;
			
			if ((*i)->bFinalized)
			{
				polyAabb = (*i)->GetExtents();
			}
			else
			{
				(*i)->CalculateExtents(polyAabb.min, polyAabb.max);
			}

			if (bFirstPoly)
			{
				aabb = polyAabb;
				bFirstPoly = false;
			}
			else
			{
				aabb += polyAabb;
			}
			
		}
		
		return aabb;
		
	}

	Vector Mesh::CalculateCenter() const
	{

		const Aabb aabb = CalculateExtents();
		
		return (aabb.min + aabb.max) / 2.0f;
		
	}

	Vector Mesh::CalculateVertexCenter() const
	{

		Vector center;
		int nVertices = 0;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			if ((*i)->bFinalized)
			{
				center += (*i)->GetVertexCenter() * (float)(*i)->cVertex.size();
				nVertices += (int)(*i)->cVertex.size();
			}
			else
			{
				for (std::list<Vertex*>::iterator j = (*i)->cVertex.begin(); j != (*i)->cVertex.end(); ++j)
				{
					center += (*j)->position;
					++nVertices;
				}
			}
		}
		
		if (nVertices != 0)
		{
			center /= (float)nVertices;
		}
		
		return center;

	}

	Vector Mesh::CalculatePolyCenter() const
	{

		Vector center;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			if ((*i)->bFinalized)
			{
				center += (*i)->GetVertexCenter();
			}
			else
			{
				center += (*i)->CalculateVertexCenter();
			}
		}
		
		if (polys.empty() == false)
		{
			center /= (float)polys.size();
		}
		
		return center;
		
	}

	const Aabb& Mesh::GetExtents() const
	{

		Assert(bFinalized);
		
		return aabb;

	}

	const Vector& Mesh::GetCenter() const
	{

		Assert(bFinalized);

		return center;
		
	}

	const Vector& Mesh::GetPolyCenter() const
	{

		Assert(bFinalized);

		return polyCenter;
		
	}

	const Vector& Mesh::GetVertexCenter() const
	{

		Assert(bFinalized);

		return vertexCenter;

	}

	bool Mesh::IsConvex() const
	{

		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			for (std::list<Poly*>::const_iterator j = polys.begin(); j != polys.end(); ++j)
			{
				if ((*i) != (*j))
				{
					PlaneClassification temp = (*i)->Classify(*(*j));
					if (temp != pcFront && temp != pcOn)
					{
						return false;
					}
				}
			}
		}

		return true;
		
	}

	PlaneClassification Mesh::Classify(const Plane& plane) const
	{

		PlaneClassification classification;
		
		// Test if mesh lies on plane.
		
		classification = pcOn;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			PlaneClassification temp = (*i)->Classify(plane);
			if (temp != pcOn)
			{
				classification = pcUnknown;
			}
		}
		
		if (classification != pcUnknown)
		{
			return classification;
		}
		
		// Test if mesh lies in front of plane.
		
		classification = pcFront;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			PlaneClassification temp = (*i)->Classify(plane);
			if (temp != pcFront && temp != pcOn)
			{
				classification = pcUnknown;
			}
		}
		
		if (classification != pcUnknown)
		{
			return classification;
		}
		
		// Test of mesh lies behind plane.
		
		classification = pcBack;
		
		for (std::list<Poly*>::const_iterator i = polys.begin(); i != polys.end(); ++i)
		{
			PlaneClassification temp = (*i)->Classify(plane);
			if (temp != pcBack && temp != pcOn)
			{
				classification = pcUnknown;
			}
		}
		
		if (classification != pcUnknown)
		{
			return classification;
		}
		
		// Must be spanning.
		
		return pcSpan;
		
	}

	void Mesh::Finalize()
	{

		Assert(!bFinalized);
		
		for (std::list<Poly*>::iterator i = polys.begin(); i != polys.end(); ++i)
		{
			(*i)->Finalize();
		}
		
		aabb = CalculateExtents();
		center = (aabb.min + aabb.max) / 2.0f;
		polyCenter = CalculatePolyCenter();
		vertexCenter = CalculateVertexCenter();
		
		bFinalized = true;
		
	}

	void Mesh::Unfinalize()
	{

		Assert(bFinalized);

		for (std::list<Poly*>::iterator i = polys.begin(); i != polys.end(); ++i)
		{
			(*i)->Unfinalize();
		}
		
		bFinalized = false;
		
	}

	Mesh& Mesh::operator=(const Mesh& inMesh)
	{

		Clear();
		
		Mesh* mesh = (Mesh*)&inMesh;
		
		for (std::list<Poly*>::iterator i = mesh->polys.begin(); i != mesh->polys.end(); ++i)
		{
		
			Poly* poly = Add();
			
			*poly = *(*i);
			
		}
		
		return (*this);
		
	}

}
