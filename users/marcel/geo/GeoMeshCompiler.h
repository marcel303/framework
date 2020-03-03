#ifndef __GeoCompilerMesh_h__
#define __GeoCompilerMesh_h__

#if 0

// FIXME: Fix / implement texcoord stuff.

#include "Plane.h"
#include "Poly.h"

//---------------------------------------------------------------------------

#define GEOM_COMPILER_VERTEX_EPS 0.001   ///< Epsilon value for CGeomCompiler to merge vertices.
#define GEOM_COMPILER_TEXCOORD_EPS 0.001 ///< Epsilom value for CGeomCompiler to merge texture coordinates.

/// Geometry: Compiled / shared vertex.
/**
 * Used by the CCompiledMesh class.
 * @see CGeomCompiler
 */
class CCompiledVertex
{

	public:
	
	CVector p; ///< XYZ position of the vertex.
	
	std::vector<int> edge;    ///< Edges the vertex is used by.
	std::vector<int> polygon; ///< Polygons the vertex is used by.
	
	public:
	
	CPlane plane; ///< The normal of the plane is the average normal of all polygons that use this vertex. distance is calculated normal * p.

};

/// Geometry: Compiled / shared texture coordinate.
/**
 * Used by the CCompiledMesh class.
 * @see CGeomCompiler
 */
class CCompiledTexcoord
{

	public:
	
	float t[2]; ///< Texture coordinate.
	
	std::vector<int> edge;    ///< Edges the texture coordinate is used by.
	std::vector<int> polygon; ///< Polygons the texture coordinate is used by.
	
};

/// Geometry: Compiled / shared edge.
/**
 * Used by the CCompiledMesh class.
 * @see CGeomCompiler
 */
class CCompiledEdge
{

	public:
	
	std::vector<int> vertex;   ///< Vertices that make up the edge.
	std::vector<int> texCoord; ///< Texture coordinates that belong to the vertices.
	std::vector<int> polygon;

};

/// Geometry: Compiled polygon.
/**
 * Used by the CCompiledMesh class.
 * @see CGeomCompiler
 */
class CCompiledPolygon
{

	public:
	
	std::vector<int> vertex; ///< Vertices that make up this polygon. In drawing order.
	std::vector<int> texcoord; ///< Texture coordinates associated with the vertices.
	std::vector<int> edge;   ///< Edges that make up this polygon.
	
	public:
	
	CPlane plane;
	CVector center;
	
	public:
	
	void* data; ///< Custom data field.
	
};

/// Geometry: Compiled mesh.
/**
 * The compiled mesh class holds all vertex, texCoord, edge and polygon information produced by the geometry compiler.
 * Polygons, vertices and edges index into the vertex, texCoord and edge arrays.
 * Without the CCompiledMesh class the other compiled types are useless.
 * @see CGeomCompiler
 */
class CCompiledMesh
{

	public:

 	CCompiledMesh()
 	{
 		base = this;
 	}
 	
	public:
	
	std::vector<CCompiledVertex> vertex;     ///< Vertices that make up the mesh's geometry.
	std::vector<CCompiledTexcoord> texcoord; ///< Texture coordinates used by the mesh's geometry.
	std::vector<CCompiledEdge> edge;         ///< Edges used by the mesh's polygons.
	std::vector<CCompiledPolygon> polygon;   ///< Polygons that constitute the mesh.
	
	public:
	
	CCompiledMesh* base; ///< Mesh where shared data is stored.
	std::vector<int> polygonIndex; ///< Index into polygon array of shared data mesh.
	
	public:

	void clear();
	void init(); ///< Initialize the mesh after all polygon / edge / vertex data has been created. This will calculate polygon & vertex normals, etc, etc.

};

/// Geometry: Geometry compiler.
/**
 * Use this class to compile meshes into CCompiledMesh structures.
 * The compiling process does the following things:
 * - Remove duplicate vertices by merging them.
 * - Create polygons that index into the CCompiledMesh's vertex array and index into the edge array.
 * Full shared edge, shared vertex, etc information is available through the CCompiledEdge, CCompiledVertex structures.
 * The compiler also calculate average normals at vertices for smooth shading / lighting or texture coordinate generation (eg sphere mapping).
 * See the other CCompiled* classes for more information.
\code
// Example:

CCompiledMesh scene;

void create_scene()
{

	CMesh mesh;
	
	CGeomBuilder::I().sphere(mesh, 10, 20);
	
	CGeomBuilder::I().cone(mesh, 20);
	
	CGeomCompiler::I().compile(mesh, scene);

}
\endcode
 */
class CGeomCompiler
{

	private:
	
	CGeomCompiler();
	
	public:
	
	~CGeomCompiler();
	
	public:
	
	/**
     * Use this function to get a reference to the geometry compiler singleton object.
     */
	static CGeomCompiler& I()
	{
		static CGeomCompiler geomCompiler;
		return geomCompiler;
	}
	
	public:
	
	/**
     * Compile the mesh into an CCompiledMesh object. The compiler will
     * merge identical vertices, create shared edges, etc, etc...
     */
	void compile(CMesh& mesh, CCompiledMesh& out);
	void compile(CMesh& mesh, CCompiledMesh& base, CCompiledMesh& index);
	
	private:
	
	int findVertex(CCompiledMesh& mesh, CCompiledVertex& vertex);
	int findTexcoord(CCompiledMesh& mesh, CCompiledTexcoord& texCoord);
	int findEdge(CCompiledMesh& mesh, CCompiledEdge& edge);
	
	public:
	
	void removeDuplicateIndices(CCompiledMesh& mesh);

};

#endif

#endif
