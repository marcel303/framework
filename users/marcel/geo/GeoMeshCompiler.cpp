#include <math.h>
#include "GeoMeshCompiler.h"

#if 0

CGeomCompiler::CGeomCompiler()
{

}

CGeomCompiler::~CGeomCompiler()
{

}

void CGeomCompiler::compile(CMesh& mesh, CCompiledMesh& out)
{

	compile(mesh, out, out);

}

void CGeomCompiler::compile(CMesh& mesh, CCompiledMesh& base, CCompiledMesh& index)
{

	for (CPoly* poly = mesh.polyHead; poly; poly = poly->next)
	{

		CCompiledPolygon compiledPoly;
		int poly_index = (int)base.polygon.size();

		for (CEdge* edge1 = poly->edgeHead; edge1; edge1 = edge1->next)
		{

			CEdge* edge2 = edge1->next ? edge1->next : poly->edgeHead;

			CCompiledVertex vertex1;
			CCompiledVertex vertex2;

			vertex1.p = edge1->p;
			vertex2.p = edge2->p;

			int vertex1_index = findVertex(base, vertex1);

			if (vertex1_index == -1)
			{
				vertex1_index = (int)base.vertex.size();
				base.vertex.push_back(vertex1);
			}

			int vertex2_index = findVertex(base, vertex2);

			if (vertex2_index == -1)
			{
				vertex2_index = (int)base.vertex.size();
				base.vertex.push_back(vertex2);
			}

			CCompiledTexcoord texcoord;

			texcoord.t[0] = edge1->t[0][0];
			texcoord.t[1] = edge1->t[0][1];

			int texcoord_index = findTexcoord(base, texcoord);

			if (texcoord_index == -1)
			{
				texcoord_index = (int)base.texcoord.size();
				base.texcoord.push_back(texcoord);
			}

			// Don't bother adding this edge / vertex if the edge has zero length.
			// This might happen when geometry is weird or when really close vertices get merged.

			if (vertex1_index != vertex2_index || 1)
			{

				compiledPoly.vertex.push_back(vertex1_index);
				compiledPoly.texcoord.push_back(texcoord_index);

				CCompiledEdge edge;

				edge.vertex.push_back(vertex1_index);
				edge.vertex.push_back(vertex2_index);

				int edge_index = findEdge(base, edge);

				if (edge_index == -1)
				{
					edge_index = (int)base.edge.size();
					base.edge.push_back(edge);
				}

				compiledPoly.edge.push_back(edge_index);
				
				base.vertex[vertex1_index].edge.push_back(edge_index);
				base.vertex[vertex2_index].edge.push_back(edge_index);

				base.vertex[vertex1_index].polygon.push_back(poly_index);
				base.vertex[vertex2_index].polygon.push_back(poly_index);

			}

		}

		compiledPoly.data = poly->data;

		base.polygon.push_back(compiledPoly);

		int polyIndex = (int)base.polygon.size() - 1;

		if (&index != &base)
			base.polygonIndex.push_back(polyIndex);
		index.polygonIndex.push_back(polyIndex);

	}

	base.init();

	index.base = &base;

}

int CGeomCompiler::findVertex(CCompiledMesh& mesh, CCompiledVertex& vertex)
{

	for (int i = 0; i < (int)mesh.vertex.size(); ++i)
	{
		if (
			fabs(mesh.vertex[i].p[0] - vertex.p[0]) < GEOM_COMPILER_VERTEX_EPS &&
			fabs(mesh.vertex[i].p[1] - vertex.p[1]) < GEOM_COMPILER_VERTEX_EPS &&
			fabs(mesh.vertex[i].p[2] - vertex.p[2]) < GEOM_COMPILER_VERTEX_EPS)
		{
			return i;
		}
	}

	return -1;

}

int CGeomCompiler::findTexcoord(CCompiledMesh& mesh, CCompiledTexcoord& texcoord)
{

	for (int i = 0; i < (int)mesh.texcoord.size(); ++i)
	{
		if (
			fabs(mesh.texcoord[i].t[0] - texcoord.t[0]) < GEOM_COMPILER_VERTEX_EPS &&
			fabs(mesh.texcoord[i].t[1] - texcoord.t[1]) < GEOM_COMPILER_VERTEX_EPS)
		{
			return i;
		}
	}

	return -1;

}

int CGeomCompiler::findEdge(CCompiledMesh& mesh, CCompiledEdge& edge)
{

	for (int i = 0; i < (int)mesh.edge.size(); ++i)
	{

		if (mesh.edge[i].vertex.size() == 2)
		{

			if (
   				(mesh.edge[i].vertex[0] == edge.vertex[0] && mesh.edge[i].vertex[1] == edge.vertex[1]) ||
				(mesh.edge[i].vertex[1] == edge.vertex[0] && mesh.edge[i].vertex[0] == edge.vertex[1]))
			{
				return i;
			}

		}

	}

	return -1;

}

void CGeomCompiler::removeDuplicateIndices(CCompiledMesh& mesh)
{

	for (int i = 0; i < (int)mesh.vertex.size(); ++i)
	{

		#define REMOVE_DUPLICATES(_vector) \
		{ \
			std::vector<int> temp; \
			for (int j = 0; j < (int)_vector.size(); ++j) \
			{ \
				int duplicate = 0; \
				for (int k = 0; k < (int)temp.size(); ++k) \
					if (temp[k] == _vector[j]) \
						duplicate = 1; \
 				if (!duplicate) \
 					temp.push_back(_vector[j]); \
			} \
			_vector.clear(); \
			for (int j = 0; j < (int)temp.size(); ++j) \
				_vector.push_back(temp[j]); \
		}

		for (int i = 0; i < (int)mesh.vertex.size(); ++i)
		{
			REMOVE_DUPLICATES(mesh.vertex[i].edge);
			REMOVE_DUPLICATES(mesh.vertex[i].polygon);
		}

		for (int i = 0; i < (int)mesh.texcoord.size(); ++i)
		{
			REMOVE_DUPLICATES(mesh.texcoord[i].edge);
			REMOVE_DUPLICATES(mesh.texcoord[i].polygon);
		}

		#undef REMOVE_DUPLICATES

	}

}

void CCompiledMesh::clear()
{

	vertex.clear();
	texcoord.clear();
	edge.clear();
	polygon.clear();
	polygonIndex.clear();

}

void CCompiledMesh::init()
{

	// Make sure 'usage' / 'referenced by' reference arrays don't contain any duplicates.
	
	CGeomCompiler::I().removeDuplicateIndices(*this);
	
	// Add polygon indices to edges.
	
	for (int i = 0; i < (int)polygon.size(); ++i)
	{
		for (int j = 0; j < (int)polygon[i].edge.size(); ++j)
			edge[polygon[i].edge[j]].polygon.push_back(i);
	}
	
	// Calculate polygon normal.

	for (int i = 0; i < (int)polygon.size(); ++i)
	{

		if (polygon[i].vertex.size() >= 3)
		{

			CVector p1 = vertex[polygon[i].vertex[0]].p;
			CVector p2 = vertex[polygon[i].vertex[1]].p;
			CVector p3 = vertex[polygon[i].vertex[2]].p;

			CVector delta1 = p2 - p1;
			CVector delta2 = p3 - p2;

			CVector normal = delta1 % delta2;

			normal.normalize();

			polygon[i].plane.normal = normal;

			polygon[i].plane.distance = p1 * normal;

		}
		else
		{
			printf("warning: CCompiledMesh.polygon.vertex.size() < 3\n");
		}

	}

	// Calculate planes at vertices.

	for (int i = 0; i < (int)vertex.size(); ++i)
	{

		CVector normal;

		for (int j = 0; j < (int)vertex[i].polygon.size(); ++j)
		{

			normal += polygon[vertex[i].polygon[j]].plane.normal;

		}

		normal.normalize();

		vertex[i].plane.normal = normal;

		vertex[i].plane.distance = vertex[i].p * normal;

	}

	// Calculate polygon center.

	for (int i = 0; i < (int)polygon.size(); ++i)
	{

		CVector center;

		for (int j = 0; j < (int)polygon[i].vertex.size(); ++j)
		{
			center += vertex[polygon[i].vertex[j]].p;
		}

		center /= polygon[i].vertex.size();

		polygon[i].center = center;

	}

}

#endif