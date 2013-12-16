//#include <boost/pool/pool_alloc.hpp>
#include <map>
#include "ShapeBuilder.h"

static inline Vector UVToXYZ(float u, float v, ShapeBuilder::AXIS upAxis, float w = 0.0f)
{
	Vector xyz;

	xyz[(upAxis + 1) % 3] = u;
	xyz[(upAxis + 2) % 3] = v;
	xyz[(upAxis + 3) % 3] = w;

	return xyz;
}

//

ShapeBuilder& ShapeBuilder::I()
{
	static ShapeBuilder shapeBuilder;
	return shapeBuilder;
}

void ShapeBuilder::Push(const Matrix& transform)
{
	m_transformStack.push(m_transformStack.top() * transform);
}

void ShapeBuilder::PushTranslation(const Vector& translation)
{
	Matrix transform;
	transform.MakeTranslation(translation);
	Push(transform);
}

void ShapeBuilder::PushRotation(const Vector& rotation)
{
	Matrix transform;
	transform.MakeRotationEuler(rotation);
	Push(transform);
}

void ShapeBuilder::PushScaling(const Vector& scale)
{
	Matrix transform;
	transform.MakeScaling(scale);
	Push(transform);
}

void ShapeBuilder::Pop()
{
	m_transformStack.pop();
	assert(m_transformStack.size() >= 1);
}

Matrix ShapeBuilder::GetTransform()
{
	return m_transformStack.top();
}

size_t ShapeBuilder::GetTransformStackDepth()
{
	return m_transformStack.size();
}

//

bool ShapeBuilder::CreateCircle(size_t resolution, Mesh& out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST, resolution + 1, fvf, resolution * 3);

	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	IndexBuffer& ib = out_mesh.GetIndexBuffer();

	for (size_t i = 0; i < resolution; ++i)
	{
		float angle = F_PI * 2.0f / resolution * i;

		float x = cosf(angle);
		float y = sinf(angle);

		Vector xyz = Mul(Vector(x, y, 0.0f));
		vb.SetPosition3(i, xyz[0], xyz[1], xyz[2]);
	}

	Vector xyz = Mul(Vector(0.0f, 0.0f, 0.0f));
	vb.SetPosition3(resolution, xyz[0], xyz[1], xyz[2]);

	for (size_t i = 0; i < resolution; ++i)
	{
		uint32_t index1 = static_cast<uint32_t>((i + 0) % resolution);
		uint32_t index2 = static_cast<uint32_t>((i + 1) % resolution);
		uint32_t center = static_cast<uint32_t>(resolution);

		ib.index[i * 3 + 0] = index1;
		ib.index[i * 3 + 1] = index2;
		ib.index[i * 3 + 2] = center;
	}

	return true;
}

bool ShapeBuilder::CreateQuad(AXIS upAxis, Mesh& out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST, size_t(4), fvf, 2 * 3);

	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	IndexBuffer& ib = out_mesh.GetIndexBuffer();

	const Vector position[4] =
	{
		Mul(UVToXYZ(-1.0f, -1.0f, upAxis)),
		Mul(UVToXYZ(+1.0f, -1.0f, upAxis)),
		Mul(UVToXYZ(+1.0f, +1.0f, upAxis)),
		Mul(UVToXYZ(-1.0f, +1.0f, upAxis))
	};

	vb.SetPosition3(0, position[0][0], position[0][1], position[0][2]);
	vb.SetPosition3(1, position[1][0], position[1][1], position[1][2]);
	vb.SetPosition3(2, position[2][0], position[2][1], position[2][2]);
	vb.SetPosition3(3, position[3][0], position[3][1], position[3][2]);

	vb.SetColor(0, 255, 255, 255);
	vb.SetColor(1, 255, 191, 191);
	vb.SetColor(2, 191, 255, 191);
	vb.SetColor(3, 191, 191, 255);

	vb.SetTexcoord(0, 0, 0.0f, 0.0f);
	vb.SetTexcoord(1, 0, 1.0f, 0.0f);
	vb.SetTexcoord(2, 0, 1.0f, 1.0f);
	vb.SetTexcoord(3, 0, 0.0f, 1.0f);

	ib.index[0] = 0;
	ib.index[1] = 1;
	ib.index[2] = 2;

	ib.index[3] = 0;
	ib.index[4] = 2;
	ib.index[5] = 3;

	return true;
}

bool ShapeBuilder::CreateCube(Mesh& out_mesh, uint32_t fvf)
{
	Mesh sides[6];

	PushTranslation(Vector(+1.0f, 0.0f, 0.0f));
		CreateQuad(AXIS_X, sides[0], fvf);
	Pop();
	PushTranslation(Vector(-1.0f, 0.0f, 0.0f));
		PushRotation(Vector(0.0f, F_PI, 0.0f));
			CreateQuad(AXIS_X, sides[1], fvf);
		Pop();
	Pop();

	PushTranslation(Vector(0.0f, +1.0f, 0.0f));
		CreateQuad(AXIS_Y, sides[2], fvf);
	Pop();
	PushTranslation(Vector(0.0f, -1.0f, 0.0f));
		PushRotation(Vector(0.0f, 0.0f, F_PI));
			CreateQuad(AXIS_Y, sides[3], fvf);
		Pop();
	Pop();

	PushTranslation(Vector(0.0f, 0.0f, +1.0f));
		CreateQuad(AXIS_Z, sides[4], fvf);
	Pop();
	PushTranslation(Vector(0.0f, 0.0f, -1.0f));
		PushRotation(Vector(F_PI, 0.0f, 0.0f));
			CreateQuad(AXIS_Z, sides[5], fvf);
		Pop();
	Pop();

	std::vector<Mesh*> meshes;
	meshes.reserve(6);

	for (size_t i = 0; i < 6; ++i)
		meshes.push_back(&sides[i]);

	Merge(meshes, out_mesh);

	return true;
}

bool ShapeBuilder::CreateGrid(size_t resolutionU, size_t resolutionV, AXIS upAxis, Mesh& out_mesh, uint32_t fvf)
{
	const size_t vertexCount = resolutionU * resolutionV;
	const size_t cellCount = (resolutionU - 1) * (resolutionV - 1);

	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST, vertexCount, fvf, cellCount * 2 * 3);

	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	IndexBuffer& ib = out_mesh.GetIndexBuffer();

	for (size_t i = 0; i < resolutionU; ++i)
	{
		for (size_t j = 0; j < resolutionV; ++j)
		{
			size_t vertexIndex = i + j * resolutionU;

			float u = i / float(resolutionU - 1);
			float v = j / float(resolutionV - 1);

			float x = (u - 0.5f) * 2.0f;
			float y = (v - 0.5f) * 2.0f;

			Vector xyz = Mul(UVToXYZ(x, y, upAxis));
			vb.SetPosition3(vertexIndex, xyz[0], xyz[1], xyz[2]);
			vb.SetTexcoord(vertexIndex, 0, u, v);
		}
	}

	for (size_t i = 0; i < resolutionU - 1; ++i)
	{
		for (size_t j = 0; j < resolutionV - 1; ++j)
		{
			uint32_t baseIndex = static_cast<uint32_t>((i + j * (resolutionU - 1)) * 2 * 3);
			uint32_t topLeftVertex = static_cast<uint32_t>(i + j * resolutionU);

			ib.index[baseIndex + 0] = topLeftVertex + 0;
			ib.index[baseIndex + 1] = topLeftVertex + 1;
			ib.index[baseIndex + 2] = topLeftVertex + resolutionU + 1;

			ib.index[baseIndex + 3] = topLeftVertex + 0;
			ib.index[baseIndex + 4] = topLeftVertex + resolutionU + 1;
			ib.index[baseIndex + 5] = topLeftVertex + resolutionU + 0;
		}
	}

	return true;
}

bool ShapeBuilder::CreateGridCube(size_t resolutionU, size_t resolutionV, Mesh& out_mesh, uint32_t fvf)
{
	Mesh sides[6];

	PushTranslation(Vector(+1.0f, 0.0f, 0.0f));
		CreateGrid(resolutionU, resolutionV, AXIS_X, sides[0], fvf);
	Pop();
	PushTranslation(Vector(-1.0f, 0.0f, 0.0f));
		PushRotation(Vector(0.0f, F_PI, 0.0f));
			CreateGrid(resolutionU, resolutionV, AXIS_X, sides[1], fvf);
		Pop();
	Pop();

	PushTranslation(Vector(0.0f, +1.0f, 0.0f));
		CreateGrid(resolutionU, resolutionV, AXIS_Y, sides[2], fvf);
	Pop();
	PushTranslation(Vector(0.0f, -1.0f, 0.0f));
		PushRotation(Vector(0.0f, 0.0f, F_PI));
			CreateGrid(resolutionU, resolutionV, AXIS_Y, sides[3], fvf);
		Pop();
	Pop();

	PushTranslation(Vector(0.0f, 0.0f, +1.0f));
		CreateGrid(resolutionU, resolutionV, AXIS_Z, sides[4], fvf);
	Pop();
	PushTranslation(Vector(0.0f, 0.0f, -1.0f));
		PushRotation(Vector(F_PI, 0.0f, 0.0f));
			CreateGrid(resolutionU, resolutionV, AXIS_Z, sides[5], fvf);
		Pop();
	Pop();

	std::vector<Mesh*> meshes;

	for (size_t i = 0; i < 6; ++i)
		meshes.push_back(&sides[i]);

	Merge(meshes, out_mesh);

	return true;
}

bool ShapeBuilder::CreateDonut(size_t resolutionU, size_t resolutionV, float outerRadius, float innerRadius, Mesh& out_mesh, uint32_t fvf)
{
	const size_t vertexCount = resolutionU * resolutionV;
	const size_t triangleCount = resolutionU * resolutionV * 2;
	const size_t indexCount = triangleCount * 3;

	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST, vertexCount, fvf, indexCount);

	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	IndexBuffer& ib = out_mesh.GetIndexBuffer();

	for (size_t i = 0; i < resolutionU; ++i)
	{
		const float angle1 = i / float(resolutionU) * 2.0f * F_PI;
		PushRotation(Vector(-angle1, 0.0f, 0.0f));
			PushTranslation(Vector(0.0f, outerRadius, 0.0f));
				for (size_t j = 0; j < resolutionV; ++j)
				{
					const float angle2 = j / float(resolutionV) * 2.0f * F_PI;

					PushRotation(Vector(0.0f, 0.0f, angle2));
						const Vector position = Mul(Vector(innerRadius, 0.0f, 0.0f));
						vb.position3[i + j * resolutionU].x = position[0];
						vb.position3[i + j * resolutionU].y = position[1];
						vb.position3[i + j * resolutionU].z = position[2];
					Pop();
				}
			Pop();
		Pop();
	}

	for (size_t i = 0; i < resolutionU; ++i)
	{
		for (size_t j = 0; j < resolutionV; ++j)
		{
			const size_t baseIndex = (i + j * resolutionU) * 2 * 3;

			const uint32_t index1 = static_cast<uint32_t>(((i + 0) % resolutionU) + ((j + 0) % resolutionV) * resolutionU);
			const uint32_t index2 = static_cast<uint32_t>(((i + 1) % resolutionU) + ((j + 0) % resolutionV) * resolutionU);
			const uint32_t index3 = static_cast<uint32_t>(((i + 1) % resolutionU) + ((j + 1) % resolutionV) * resolutionU);

			const uint32_t index4 = static_cast<uint32_t>(((i + 0) % resolutionU) + ((j + 0) % resolutionV) * resolutionU);
			const uint32_t index5 = static_cast<uint32_t>(((i + 1) % resolutionU) + ((j + 1) % resolutionV) * resolutionU);
			const uint32_t index6 = static_cast<uint32_t>(((i + 0) % resolutionU) + ((j + 1) % resolutionV) * resolutionU);

			ib.index[baseIndex + 0] = index1;
			ib.index[baseIndex + 1] = index2;
			ib.index[baseIndex + 2] = index3;

			ib.index[baseIndex + 3] = index4;
			ib.index[baseIndex + 4] = index5;
			ib.index[baseIndex + 5] = index6;
		}
	}

	return true;
}

bool ShapeBuilder::CreateCilinder(size_t resolution, bool closed, AXIS upAxis, Mesh& out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST,
		resolution * 2 + (closed ? resolution * 2 : 0), fvf,
		resolution * 2 * 3 + (closed ? (resolution - 2) * 3 * 2 : 0));

	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	IndexBuffer& ib = out_mesh.GetIndexBuffer();

	for (uint32_t i = 0; i < resolution; ++i)
	{
		float angle = 2.0f * F_PI / resolution * i;
		Vector position;

		position = Mul(UVToXYZ(cosf(angle), sinf(angle), upAxis, -1.0f));
		vb.position3[i * 2 + 0].x = position[0];
		vb.position3[i * 2 + 0].y = position[1];
		vb.position3[i * 2 + 0].z = position[2];
		if (closed)
		{
			vb.position3[(resolution + i) * 2 + 0].x = position[0];
			vb.position3[(resolution + i) * 2 + 0].y = position[1];
			vb.position3[(resolution + i) * 2 + 0].z = position[2];
		}

		position = Mul(UVToXYZ(cosf(angle), sinf(angle), upAxis, +1.0f));
		vb.position3[i * 2 + 1].x = position[0];
		vb.position3[i * 2 + 1].y = position[1];
		vb.position3[i * 2 + 1].z = position[2];
		if (closed)
		{
			vb.position3[(resolution + i) * 2 + 1].x = position[0];
			vb.position3[(resolution + i) * 2 + 1].y = position[1];
			vb.position3[(resolution + i) * 2 + 1].z = position[2];
		}
	}

	for (uint32_t i = 0; i < resolution; ++i)
	{
		ib.index[i * 2 * 3 + 0] = ((i + 0) % resolution) * 2 + 0;
		ib.index[i * 2 * 3 + 1] = ((i + 1) % resolution) * 2 + 0;
		ib.index[i * 2 * 3 + 2] = ((i + 1) % resolution) * 2 + 1;

		ib.index[i * 2 * 3 + 3] = ((i + 0) % resolution) * 2 + 0;
		ib.index[i * 2 * 3 + 4] = ((i + 1) % resolution) * 2 + 1;
		ib.index[i * 2 * 3 + 5] = ((i + 0) % resolution) * 2 + 1;
	}

	if (closed)
	{
		uint32_t index = static_cast<uint32_t>(resolution * 2 * 3);

		for (size_t i = 0; i < resolution - 2; ++i)
		{
			ib.index[index + 0] = static_cast<uint32_t>((resolution + (i + 2) % resolution) * 2 + 0);
			ib.index[index + 1] = static_cast<uint32_t>((resolution + (i + 1) % resolution) * 2 + 0);
			ib.index[index + 2] = static_cast<uint32_t>(resolution * 2 + 0);

			index += 3;
		}

		for (size_t i = 0; i < resolution - 2; ++i)
		{
			ib.index[index + 0] = static_cast<uint32_t>(resolution * 2 + 1);
			ib.index[index + 1] = static_cast<uint32_t>((resolution + (i + 1) % resolution) * 2 + 1);
			ib.index[index + 2] = static_cast<uint32_t>((resolution + (i + 2) % resolution) * 2 + 1);

			index += 3;
		}
	}

	return true;
}

bool ShapeBuilder::ConvertToFVF(Mesh& mesh, uint32_t fvf, Mesh& out_mesh)
{
	VertexBuffer& vb = mesh.GetVertexBuffer();
	IndexBuffer& ib = mesh.GetIndexBuffer();

	out_mesh.Initialize(
		mesh.GetPrimitiveType(),
		vb.GetVertexCount(),
		fvf,
		ib.GetIndexCount(),
		ib.GetIndexFormat());

	for (uint32_t i = 0; i < vb.GetVertexCount(); ++i)
		CopyVertex(mesh, i, out_mesh, i);

	for (size_t i = 0; i < ib.GetIndexCount(); ++i)
		out_mesh.GetIndexBuffer().index[i] = ib.index[i];

	return true;
}

class MeshVertex
{
public:
	MeshVertex(Mesh* mesh, uint32_t index)
	{
		m_mesh = mesh;
		m_srcIndex = index;
		m_dstIndex = 0;
	}

	Mesh* m_mesh;
	uint32_t m_srcIndex;
	uint32_t m_dstIndex;

	bool operator<(const MeshVertex& other) const
	{
		const size_t fvf = other.m_mesh->GetVertexBuffer().GetFVF();

		assert(other.m_mesh->GetVertexBuffer().GetFVF() == fvf);

		VertexBuffer& vb1 = m_mesh->GetVertexBuffer();
		VertexBuffer& vb2 = other.m_mesh->GetVertexBuffer();

		if (fvf & VertexBuffer::FVF_XYZ)
		{
			if (vb1.position3[m_srcIndex].x < vb2.position3[other.m_srcIndex].x)
				return true;
			if (vb1.position3[m_srcIndex].x > vb2.position3[other.m_srcIndex].x)
				return false;

			if (vb1.position3[m_srcIndex].y < vb2.position3[other.m_srcIndex].y)
				return true;
			if (vb1.position3[m_srcIndex].y > vb2.position3[other.m_srcIndex].y)
				return false;

			if (vb1.position3[m_srcIndex].z < vb2.position3[other.m_srcIndex].z)
				return true;
			if (vb1.position3[m_srcIndex].z > vb2.position3[other.m_srcIndex].z)
				return false;
		}

		if (fvf & VertexBuffer::FVF_COLOR)
		{
			if (vb1.color[m_srcIndex].c < vb2.color[other.m_srcIndex].c)
				return true;
			if (vb1.color[m_srcIndex].c > vb2.color[other.m_srcIndex].c)
				return false;
		}
		
		if (fvf & VertexBuffer::FVF_NORMAL)
		{
			if (vb1.normal[m_srcIndex].x < vb2.normal[other.m_srcIndex].x)
				return true;
			if (vb1.normal[m_srcIndex].x > vb2.normal[other.m_srcIndex].x)
				return false;

			if (vb1.normal[m_srcIndex].y < vb2.normal[other.m_srcIndex].y)
				return true;
			if (vb1.normal[m_srcIndex].y > vb2.normal[other.m_srcIndex].y)
				return false;

			if (vb1.normal[m_srcIndex].z < vb2.normal[other.m_srcIndex].z)
				return true;
			if (vb1.normal[m_srcIndex].z > vb2.normal[other.m_srcIndex].z)
				return false;
		}

		size_t textureCount = vb1.GetTexcoordCount();

		for (size_t i = 0; i < textureCount; ++i)
		{
			if (vb1.tex[i][m_srcIndex].u < vb2.tex[i][other.m_srcIndex].u)
				return true;
			if (vb1.tex[i][m_srcIndex].u > vb2.tex[i][other.m_srcIndex].u)
				return false;

			if (vb1.tex[i][m_srcIndex].v < vb2.tex[i][other.m_srcIndex].v)
				return true;
			if (vb1.tex[i][m_srcIndex].v > vb2.tex[i][other.m_srcIndex].v)
				return false;

			if (vb1.tex[i][m_srcIndex].s < vb2.tex[i][other.m_srcIndex].s)
				return true;
			if (vb1.tex[i][m_srcIndex].s > vb2.tex[i][other.m_srcIndex].s)
				return false;

			if (vb1.tex[i][m_srcIndex].t < vb2.tex[i][other.m_srcIndex].t)
				return true;
			if (vb1.tex[i][m_srcIndex].t > vb2.tex[i][other.m_srcIndex].t)
				return false;
		}

		size_t blendWeightCount = vb1.GetBlendWeightCount();

		for (size_t i = 0; i < blendWeightCount; ++i)
		{
			if (vb1.blendn[i][m_srcIndex] < vb2.blendn[i][other.m_srcIndex])
				return true;
			if (vb1.blendn[i][m_srcIndex] > vb2.blendn[i][other.m_srcIndex])
				return false;
		}

		return false;
	}

};

bool ShapeBuilder::ConvertToIndexed(Mesh& mesh, Mesh& out_mesh)
{
	typedef std::map<
		MeshVertex,
		int,
		std::less<MeshVertex>/*,
		boost::fast_pool_allocator< std::pair<MeshVertex, int> >*/
		> VertexMap;

	VertexMap vertices;
	std::vector<uint32_t> indices;

	VertexBuffer& vb = mesh.GetVertexBuffer();
	IndexBuffer& ib = mesh.GetIndexBuffer();

	#undef max
	indices.reserve(std::max(vb.GetVertexCount(), ib.GetIndexCount()));

	{
		VertexIterator i(mesh);
		uint32_t index = 0;

		while (i.Next())
		{
			MeshVertex vertex(&mesh, i.GetVertexIndex());
			if (vertices.find(vertex) == vertices.end())
			{
				// New vertex.
				vertex.m_dstIndex = index;
				vertices[vertex] = 1;
				indices.push_back(vertex.m_dstIndex);
				++index;
			}
			else
				indices.push_back(vertices.find(vertex)->first.m_dstIndex);
		}
	}

//	printf("New mesh: %d vertices, %d indices.\n", vertices.size(), indices.size());

	Mesh::PT primitiveType = Mesh::PT_TRIANGLE_LIST;
	size_t vertexCount = vertices.size();
	size_t vertexFVF   = vb.GetFVF();
	size_t indexCount  = indices.size();
	
	out_mesh.Initialize(primitiveType, vertexCount, vertexFVF, indexCount);

	// Copy vertices.
	for (VertexMap::iterator i = vertices.begin(); i != vertices.end(); ++i)
		CopyVertex(mesh, i->first.m_srcIndex, out_mesh, i->first.m_dstIndex);

	// Copy indices.
	for (size_t i = 0; i < indices.size(); ++i)
		out_mesh.GetIndexBuffer().index[i] = indices[i];

	return true;
}

bool ShapeBuilder::ConvertToNonIndexed(Mesh& mesh, Mesh& out_mesh)
{
	VertexIterator i(mesh);
	uint32_t index = 0;

	out_mesh.Initialize(
		mesh.GetPrimitiveType(),
		i.GetRealVertexCount(),
		mesh.GetVertexBuffer().GetFVF());
	
	while (i.Next())
	{
		CopyVertex(mesh, i.GetVertexIndex(), out_mesh, index);

		index++;
	}

	return true;
}

bool ShapeBuilder::Merge(Mesh& mesh1, Mesh& mesh2, Mesh& out_mesh)
{
	std::vector<Mesh*> meshes;

	meshes.push_back(&mesh1);
	meshes.push_back(&mesh2);

	return Merge(meshes, out_mesh);
}

bool ShapeBuilder::Merge(std::vector<Mesh*>& meshes, Mesh& out_mesh)
{
	if (meshes.size() == 0)
		return true;

	size_t fvf = meshes[0]->GetVertexBuffer().GetFVF();
//	Mesh::PT primitiveType = meshes[0]->GetPrimitiveType();
//	bool isIndexed = meshes[0]->IsIndexed();

	// The meshes share the same properties. We can convert.

	size_t totalPrimitiveCount = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
		totalPrimitiveCount += meshes[i]->GetPrimitiveCount();

	out_mesh.Initialize(Mesh::PT_TRIANGLE_LIST, totalPrimitiveCount * 3, fvf);

	uint32_t vertexIndex = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		VertexIterator j(*meshes[i]);
		while (j.Next())
		{
			uint32_t index = j.GetVertexIndex();
			CopyVertex(*meshes[i], index, out_mesh, vertexIndex);
			vertexIndex++;
		}
	}

	return true;
}

/*
bool ShapeBuilder::CalculateInfluences(Mesh& mesh, size_t blendIndex, ProcInfluenceList& influences)
{
	VertexBuffer& vb = mesh.GetVertexBuffer();
	size_t vertexCount = vb.GetVertexCount();

	for (size_t i = 0; i < vertexCount; ++i)
	{
		const Vector position(
			vb.position3[i].x,
			vb.position3[i].y,
			vb.position3[i].z);
		float totalInfluence = 0.0f;
		for (size_t j = 0; j < influences.size(); ++j)
			totalInfluence += influences[j]->CalculateInfluence(position);
		QASSERT(totalInfluence >= 0.0f);
		vb.blendn[blendIndex][i] = totalInfluence;
	}

	return qOK;
}
*/

/*
bool ShapeBuilder::NormalizeInfluences(Mesh& mesh, size_t blendWeightCount)
{
	VertexBuffer& vb = mesh.GetVertexBuffer();
	size_t vertexCount = vb.GetVertexCount();

	// NOTE: Perhaps 1 malloc to make iteration faster?
	// for (i, 0..blendweight)
	//    for (j, 0..vertexCount)
	//       total[j] += ...

	for (size_t i = 0; i < vertexCount; ++i)
	{
		float totalWeight = 0.0f;
		for (size_t j = 0; j < blendWeightCount; ++j)
			totalWeight += vb.blendn[j][i] * vb.blendn[j][i];

		const float invTotalWeight = 1.0f / sqrt(totalWeight);
		for (size_t j = 0; j < blendWeightCount; ++j)
			vb.blendn[j][i] *= invTotalWeight;
	}

	return qOK;
}
*/

/*bool ShapeBuilder::CalculateTexcoords(Mesh& mesh, size_t sampler, ProcTexcoord* proc)
{
	VertexBuffer& vb = mesh.GetVertexBuffer();
	size_t vertexCount = vb.GetVertexCount();

	bool hasXYZ = vb.GetFVF() & VertexBuffer::FVF_XYZ ? true : false;
	bool hasNormal = vb.GetFVF() & VertexBuffer::FVF_NORMAL ? true : false;

	for (size_t i = 0; i < vertexCount; ++i)
	{
		Vector position;
		if (hasXYZ)
		{
			position[0] = vb.position3[i].x;
			position[1] = vb.position3[i].y;
			position[2] = vb.position3[i].z;
		}

		Vector normal;
		if (hasNormal)
		{
			normal[0] = vb.normal[i].x;
			normal[1] = vb.normal[i].y;
			normal[2] = vb.normal[i].z;
		}

		Vector texcoords = proc->Generate(position, normal);

		vb.SetTexcoord(i, sampler,
			texcoords[0],
			texcoords[1],
			texcoords[2],
			texcoords[3]);
	}

	return qOK;
}*/

bool ShapeBuilder::CalculateNormals(Mesh& mesh)
{
	VertexBuffer& vb = mesh.GetVertexBuffer();

	size_t vertexCount = vb.GetVertexCount();

	for (size_t i = 0; i < vertexCount; ++i)
	{
		vb.normal[i].x = 0.0f;
		vb.normal[i].y = 0.0f;
		vb.normal[i].z = 0.0f;
	}

	VertexIterator j(mesh);

	while (j.Next())
	{
		size_t index[3];
		index[0] = j.GetVertexIndex(); j.Next();
		index[1] = j.GetVertexIndex(); j.Next();
		index[2] = j.GetVertexIndex();

		Vector position[3];

		for (int i = 0; i < 3; ++i)
			position[i] = Vector(
				vb.position3[index[i]].x,
				vb.position3[index[i]].y,
				vb.position3[index[i]].z);

		Vector delta[2];

		delta[0] = position[1] - position[0];
		delta[1] = position[2] - position[1];

		Vector normal = delta[0] % delta[1];
		normal.Normalize();

		for (int i = 0; i < 3; ++i)
		{
			vb.normal[index[i]].x += normal[0];
			vb.normal[index[i]].y += normal[1];
			vb.normal[index[i]].z += normal[2];
		}
	}

	for (size_t i = 0; i < vertexCount; ++i)
	{
		Vector normal(
			vb.normal[i].x,
			vb.normal[i].y,
			vb.normal[i].z);
		normal.Normalize();
		vb.normal[i].x = normal[0];
		vb.normal[i].y = normal[1];
		vb.normal[i].z = normal[2];
	}

	return true;
}

bool ShapeBuilder::Colorize(Mesh& out_mesh, int r, int g, int b, int a)
{
	VertexBuffer& vb = out_mesh.GetVertexBuffer();
	size_t vertexCount = vb.GetVertexCount();

	for (size_t i = 0; i < vertexCount; ++i)
	{
		vb.color[i].r = r;
		vb.color[i].g = g;
		vb.color[i].b = b;
		vb.color[i].a = a;
	}

	return true;
}
