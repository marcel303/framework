#include "Debugging.h"
#include "GeoMesh.h"
#include "GeoPoly.h"
#include "Types.h"

namespace Geo
{

	Poly::Poly()
	{

		bFinalized = false;
		
	}

	Poly::~Poly()
	{

		if (bFinalized)
		{
			Unfinalize();
		}
		
		Clear();
		
	}

	Vertex* Poly::Add()
	{

		return AddTail();
		
	}

	Vertex* Poly::AddHead()
	{

		Assert(!bFinalized);
		
		Vertex* vertex = new Vertex;
		
		vertices.push_front(vertex);
		
		return vertex;
		
	}

	Vertex* Poly::AddTail()
	{

		Assert(!bFinalized);
		
		Vertex* vertex = new Vertex;
		
		vertices.push_back(vertex);
		
		return vertex;
		
	}

	Vertex* Poly::Link(Vertex* vertex)
	{

		return LinkTail(vertex);
		
	}

	Vertex* Poly::LinkHead(Vertex* vertex)
	{

		Assert(vertex);
		Assert(!bFinalized);
		
		vertices.push_front(vertex);
		
		return vertex;
		
	}

	Vertex* Poly::LinkTail(Vertex* vertex)
	{

		Assert(vertex);
		Assert(!bFinalized);
		
		vertices.push_back(vertex);
		
		return vertex;
		
	}

	void Poly::Clear()
	{

		Assert(!bFinalized);

		for (std::list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			delete (*i);
		}
		
		vertices.clear();
		
		for (std::list<Edge*>::iterator i = edges.begin(); i != edges.end(); ++i)
		{
			delete (*i);
		}
		
		edges.clear();
		
	}

	void Poly::RevertWinding()
	{

		Assert(!bFinalized);

		std::list<Vertex*> temp;
		
		for (std::list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			temp.push_front((*i));
		}
		
		vertices.clear();
		
		for (std::list<Vertex*>::iterator i = temp.begin(); i != temp.end(); ++i)
		{
			vertices.push_back((*i));
		}

	}

	PlaneClassification Poly::Classify(const Plane& plane) const
	{

		Assert(bFinalized);
		
		// FIXME: This can be done much faster. :)
		
		PlaneClassification classification;
		
		// Test if poly lies on plane.

		classification = pcOn;
		
		for (std::list<Vertex*>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
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
		
		// Test if poly lies in front of plane.
		
		classification = pcFront;
		
		for (std::list<Vertex*>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
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
		
		// Test of poly lies behind plane.
		
		classification = pcBack;
		
		for (std::list<Vertex*>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
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
		
		// It's none of the above, so it must be spanning.

		return pcSpan;
		
	}

	PlaneClassification Poly::Classify(const Poly& poly) const
	{

		Assert(bFinalized);

		int nOn = 0;
		int nFront = 0;
		int nBack = 0;
		
		for (std::list<Vertex*>::const_iterator i = poly.vertices.begin(); i != poly.vertices.end(); ++i)
		{
			PlaneClassification temp = plane.Classify((*i)->position);
			if (temp == pcOn)
			{
				++nOn;
			}
			if (temp == pcFront)
			{
				++nFront;
			}
			if (temp == pcBack)
			{
				++nBack;
			}
		}

		if (nFront > 0 && nBack == 0)
		{
			return pcFront;
		}
		
		if (nBack > 0 && nFront == 0)
		{
			return pcBack;
		}
		
		if (nFront > 0 && nBack > 0)
		{
			return pcSpan;
		}
		
		return pcOn;
		
	}

	void Poly::Split(const Plane& plane, Poly* polyFront, Poly* polyBack)
	{

		Assert(bFinalized);
		
		// FIXME: Check classification.
		// If front or back, copy.
		
		for (std::list<Edge*>::iterator i = edges.begin(); i != edges.end(); ++i)
		{
		
			PlaneClassification classification1 = plane.Classify((*i)->vertex[0]->position);
			PlaneClassification classification2 = plane.Classify((*i)->vertex[1]->position);
		
			if (classification1 == pcOn)
			{
				
				Vertex* vertex1 = polyFront->AddTail();
				Vertex* vertex2 = polyBack->AddTail();
				
				*vertex1 = *(*i)->vertex[0];
				*vertex2 = *(*i)->vertex[0];
				
			}
			else if (classification1 == pcFront)
			{
			
				Vertex* vertex = polyFront->AddTail();
				
				*vertex = *(*i)->vertex[0];
			
				if (classification2 == pcBack)
				{
				
					Vertex* vertex1 = polyFront->AddTail();
					Vertex* vertex2 = polyBack->AddTail();
					
					Vec3 delta = (*i)->vertex[1]->position - (*i)->vertex[0]->position;
					
					float t = - (plane * (*i)->vertex[0]->position) / (plane.normal * delta);
					
					Assert(t >= 0.0f && t <= 1.0f);
					
					*vertex1 = *(*i)->vertex[0] + (*(*i)->vertex[1] - *(*i)->vertex[0]) * t;
					*vertex2 = *(*i)->vertex[0] + (*(*i)->vertex[1] - *(*i)->vertex[0]) * t;
				
				}
				
			}
			else if (classification1 == pcBack)
			{
				
				Vertex* vertex = polyBack->AddTail();
				
				*vertex = *(*i)->vertex[0];
				
				if (classification2 == pcFront)
				{
				
					Vertex* vertex1 = polyBack->AddTail();
					Vertex* vertex2 = polyFront->AddTail();
					
					Vec3 delta = (*i)->vertex[1]->position - (*i)->vertex[0]->position;
					
					float t = - (plane * (*i)->vertex[0]->position) / (plane.normal * delta);
					
					Assert(t >= 0.0f && t <= 1.0f);
					
					*vertex1 = *(*i)->vertex[0] + (*(*i)->vertex[1] - *(*i)->vertex[0]) * t;
					*vertex2 = *(*i)->vertex[0] + (*(*i)->vertex[1] - *(*i)->vertex[0]) * t;
				
				}
				
			}
			else
			{
			
				Assert(false);
			
			}
		
		}
		
		Assert(polyFront->vertices.size() >= 3);
		Assert(polyBack->vertices.size() >= 3);
		
	}

	void Poly::ClipKeepFront(const Plane& plane, Poly* poly)
	{

		Assert(bFinalized);

		Poly* polyTemp = new Poly;
		
		Split(plane, poly, polyTemp);
		
		delete polyTemp;
		
	}

	void Poly::ClipKeepBack(const Plane& plane, Poly* poly)
	{

		Assert(bFinalized);

		Poly* polyTemp = new Poly;
		
		Split(plane, polyTemp, poly);
		
		delete polyTemp;
		
	}

	void Poly::Clip(const Poly* poly, Poly** inside, class Mesh** outside)
	{

		Assert(poly);
		Assert(inside);
		Assert(outside);
		Assert(bFinalized);
		
		*inside = new Poly;
		*outside = new Mesh;
		
		Poly* temp = new Poly;
		
		*temp = *this;
		
		for (std::list<Edge*>::iterator i = edges.begin(); i != edges.end(); ++i)
		{
		
			temp->Finalize();

			Poly* clip[2] =
			{
				new Poly,
				new Poly
			};
			
			temp->Split((*i)->planeOutward, clip[0], clip[1]);

			if (clip[0]->vertices.size() >= 3)
			{
				(*outside)->Link(clip[0]);
			}
			else
			{
				delete clip[0];
			}
			
			delete temp;

			temp = clip[1];

		}

		if (temp->vertices.size() >= 3)
		{
			*(*inside) = *temp;
		}

		delete temp;

	}

	void Poly::Triangulate(Mesh* mesh)
	{

		Assert(mesh);
		Assert(bFinalized);

		int nTriangle = (int)edges.size() - 2;
		
		Edge* edge1 = edges.front();
		Edge* edge2 = edge1->next;
		Edge* edge3 = edge2->next;
		
		for (int i = 0; i < nTriangle; ++i)
		{
		
			Poly* poly = mesh->Add();
			
			Vertex* vertex1 = poly->AddTail();
			Vertex* vertex2 = poly->AddTail();
			Vertex* vertex3 = poly->AddTail();
			
			*vertex1 = *edge1->vertex[0];
			*vertex2 = *edge2->vertex[0];
			*vertex3 = *edge3->vertex[0];
			
			edge2 = edge2->next;
			edge3 = edge3->next;
			
		}

	}

	void Poly::CalculateExtents(Vec3 & out_min, Vec3& out_max) const
	{

		bool bFirstVertex = true;
		
		for (std::list<Vertex*>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			if (bFirstVertex)
			{
				out_min = (*i)->position;
				out_max = (*i)->position;
				bFirstVertex = false;
			}
			else
			{
				for (int j = 0; j < 3; ++j)
				{
					if ((*i)->position[j] < out_min[j])
					{
						out_min[j] = (*i)->position[j];
					}
					else if ((*i)->position[j] > out_max[j])
					{
						out_max[j] = (*i)->position[j];
					}
				}
			}
		}
		
	}

	Vec3 Poly::CalculateCenter() const
	{

		Vec3 aabbMin;
		Vec3 aabbMax;
		
		CalculateExtents(aabbMin, aabbMax);
		
		const Vec3 center = (aabbMin + aabbMax) / 2.0f;
		
		return center;
		
	}

	Vec3 Poly::CalculateVertexCenter() const
	{

		Vec3 center;
		
		for (std::list<Vertex*>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			center += (*i)->position;
		}
		
		if (vertices.size() > 0)
		{
			center /= (float)vertices.size();
		}
		
		return center;

	}

	const Aabb& Poly::GetExtents() const
	{

		Assert(bFinalized);
		
		return aabb;
		
	}

	Vec3 Poly::GetCenter() const
	{

		Aabb extents = GetExtents();
		
		return (extents.min + extents.max) / 2.0f;
		
	}

	Vec3 Poly::GetVertexCenter() const
	{

		return vertexCenter;
		
	}
	
	void Poly::Finalize()
	{

		Assert(vertices.size() >= 3);
		Assert(!bFinalized);
		
		// Calculate plane.
		
		std::list<Vertex*>::iterator i = vertices.begin();
		
		Vertex* vertex1 = (*i); ++i;
		Vertex* vertex2 = (*i); ++i;
		Vertex* vertex3 = (*i); ++i;
		
		plane.Setup(vertex1->position, vertex2->position, vertex3->position);
		plane.Normalize();
		
		// Create edges.
		
		for (std::list<Edge*>::iterator i = edges.begin(); i != edges.end(); ++i)
		{
			delete (*i);
		}
		
		edges.clear();
		
		for (std::list<Vertex*>::iterator i1 = vertices.begin(); i1 != vertices.end(); ++i1)
		{
		
			std::list<Vertex*>::iterator i2 = i1; ++i2;
			
			if (i2 == vertices.end())
			{
				i2 = vertices.begin();
			}
			
			Edge* edge = new Edge;
			
			edge->vertex[0] = (*i1);
			edge->vertex[1] = (*i2);
			
			edge->Finalize();
			
			edge->planeOutward.normal = edge->planeEdge.normal % plane.normal;
			edge->planeOutward.distance = edge->planeOutward.normal * edge->vertex[0]->position;
			
			edges.push_back(edge);
			
		}
		
		for (std::list<Edge*>::iterator i1 = edges.begin(); i1 != edges.end(); ++i1)
		{
		
			std::list<Edge*>::iterator i2 = i1; ++i2;
			
			if (i2 == edges.end())
			{
				i2 = edges.begin();
			}
			
			(*i1)->next = (*i2);
			(*i2)->prev = (*i1);
		
		}
		
		// Calculate extents.
		
		CalculateExtents(aabb.min, aabb.max);
		
		// Calculate vertex center.
		
		vertexCenter.SetZero();
		
		for (std::list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			vertexCenter += (*i)->position;
		}
		
		vertexCenter /= (float)vertices.size();
		
		bFinalized = true;
		
		Assert(edges.size() == vertices.size());
		
	}

	void Poly::Unfinalize()
	{

		Assert(bFinalized);
		
		bFinalized = false;
		
	}

	Poly& Poly::operator=(const Poly& inPoly)
	{

		Clear();
		
		Poly* poly = (Poly*)&inPoly;
		
		for (std::list<Vertex*>::iterator i = poly->vertices.begin(); i != poly->vertices.end(); ++i)
		{
		
			Vertex* vertex = AddTail();
			
			*vertex = *(*i);
			
		}
		
		return (*this);
		
	}

}
