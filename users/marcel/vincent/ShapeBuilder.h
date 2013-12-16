#ifndef __SHAPEBUILDER_H__
#define __SHAPEBUILDER_H__

#include <algorithm>
//#include <boost/shared_ptr.hpp>
#include <stack>
#include <vector>
//#include "MatrixStack.h"
#include "Matrix.h"
#include "Mesh.h"
#include "Types2.h"
//#include "ProcInfluence.h"
//#include "ProcTexcoord.h"

class ShapeBuilder
{
public:
	enum AXIS
	{
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2
	};

	static ShapeBuilder& I();
	
	void Push(const Matrix& transform);
	void PushTranslation(const Vector& translation);
	void PushRotation(const Vector& rotation);
	void PushScaling(const Vector& scale);
	void Pop();
	Matrix GetTransform();
	size_t GetTransformStackDepth();

	bool CreateCircle(size_t resolution, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateQuad(AXIS upAxis, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateCube(Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateGrid(size_t resolutionU, size_t resolutionV, AXIS upAxis, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateGridCube(size_t resolutionU, size_t resolutionV, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateDonut(size_t resolutionU, size_t resolutionV, float outerRadius, float innerRadius, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);
	bool CreateCilinder(size_t resolution, bool closed, AXIS upAxis, Mesh& out_mesh, uint32_t fvf = VertexBuffer::FVF_XYZ);

	bool ConvertToFVF(Mesh& mesh, uint32_t fvf, Mesh& out_mesh);
	bool ConvertToIndexed(Mesh& mesh, Mesh& out_mesh);
	bool ConvertToNonIndexed(Mesh& mesh, Mesh& out_mesh);

	bool Merge(Mesh& mesh1, Mesh& mesh2, Mesh& out_mesh);
	bool Merge(std::vector<Mesh*>& meshes, Mesh& out_mesh);

//	QRESULT CalculateInfluences(Mesh& out_mesh, size_t blendIndex, ProcInfluenceList& influences);
//	QRESULT NormalizeInfluences(Mesh& out_mesh, size_t blendWeightCount);
//	bool CalculateTexcoords(Mesh& out_mesh, size_t sampler, ProcTexcoord* proc);
	bool CalculateNormals(Mesh& out_mesh);

	bool Colorize(Mesh& out_mesh, int r, int g, int b, int a = 255);

private:
	ShapeBuilder()
	{
		Matrix transform;
		transform.MakeIdentity();
		m_transformStack.push(transform);
	}

	Vector Mul(const Vector& vector)
	{
		return m_transformStack.top() * vector;
	}

	inline static void CopyVertex(Mesh& srcMesh, uint32_t srcIndex, Mesh& dstMesh, uint32_t dstIndex)
	{
		const size_t srcFvf = srcMesh.GetVertexBuffer().GetFVF();
		const size_t dstFvf = dstMesh.GetVertexBuffer().GetFVF();

		// Copy non-indexed components.

		if ((srcFvf & VertexBuffer::FVF_XYZ) && (dstFvf & VertexBuffer::FVF_XYZ))
			dstMesh.GetVertexBuffer().SetPosition3(
				dstIndex,
				srcMesh.GetVertexBuffer().position3[srcIndex].x,
				srcMesh.GetVertexBuffer().position3[srcIndex].y,
				srcMesh.GetVertexBuffer().position3[srcIndex].z);

		if ((srcFvf & VertexBuffer::FVF_COLOR) && (dstFvf & VertexBuffer::FVF_COLOR))
			dstMesh.GetVertexBuffer().SetColor(
				dstIndex,
				srcMesh.GetVertexBuffer().color[srcIndex].r,
				srcMesh.GetVertexBuffer().color[srcIndex].g,
				srcMesh.GetVertexBuffer().color[srcIndex].b,
				srcMesh.GetVertexBuffer().color[srcIndex].a);

		if ((srcFvf & VertexBuffer::FVF_NORMAL) && (dstFvf & VertexBuffer::FVF_NORMAL))
			dstMesh.GetVertexBuffer().SetNormal(
				dstIndex,
				srcMesh.GetVertexBuffer().normal[srcIndex].x,
				srcMesh.GetVertexBuffer().normal[srcIndex].y,
				srcMesh.GetVertexBuffer().normal[srcIndex].z);

		if ((srcFvf & VertexBuffer::FVF_TANGENT) && (dstFvf & VertexBuffer::FVF_TANGENT))
			dstMesh.GetVertexBuffer().SetTangent(
				dstIndex,
				srcMesh.GetVertexBuffer().tangent[srcIndex].x,
				srcMesh.GetVertexBuffer().tangent[srcIndex].y,
				srcMesh.GetVertexBuffer().tangent[srcIndex].z);

		if ((srcFvf & VertexBuffer::FVF_BINORMAL) && (dstFvf & VertexBuffer::FVF_BINORMAL))
			dstMesh.GetVertexBuffer().SetBiNormal(
				dstIndex,
				srcMesh.GetVertexBuffer().biNormal[srcIndex].x,
				srcMesh.GetVertexBuffer().biNormal[srcIndex].y,
				srcMesh.GetVertexBuffer().biNormal[srcIndex].z);

		// Copy indexed texture coordinate components.

		size_t srcTexCount = srcMesh.GetVertexBuffer().GetTexcoordCount();
		size_t dstTexCount = dstMesh.GetVertexBuffer().GetTexcoordCount();
		size_t minTexCount = std::min<size_t>(srcTexCount, dstTexCount);

		for (size_t i = 0; i < minTexCount; ++i)
			dstMesh.GetVertexBuffer().SetTexcoord(
				dstIndex,
				i,
				srcMesh.GetVertexBuffer().tex[i][srcIndex].u,
				srcMesh.GetVertexBuffer().tex[i][srcIndex].v,
				srcMesh.GetVertexBuffer().tex[i][srcIndex].s,
				srcMesh.GetVertexBuffer().tex[i][srcIndex].t);

		// Copy indexed blend weight components.

		size_t srcBlendWeightCount = srcMesh.GetVertexBuffer().GetBlendWeightCount();
		size_t dstBlendWeightCount = dstMesh.GetVertexBuffer().GetBlendWeightCount();
		size_t minBlendWeightCount = std::min<size_t>(srcBlendWeightCount, dstBlendWeightCount);

		for (size_t i = 0; i < minBlendWeightCount; ++i)
			dstMesh.GetVertexBuffer().blendn[i][dstIndex] = srcMesh.GetVertexBuffer().blendn[i][srcIndex];
	}

	class VertexIterator
	{
	public:
		VertexIterator(Mesh& mesh) : m_mesh(mesh)
		{
			Rewind();
		}
		bool Next()
		{
			switch (m_mesh.GetPrimitiveType())
			{
			case Mesh::PT_TRIANGLE_FAN:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCount++;
					m_primitiveIndex = 0;
				}
				if (m_primitiveIndex == 0)
					m_vertexIndex = 0;
				else
					m_vertexIndex = m_primitiveCount + m_primitiveIndex;

				break;

			case Mesh::PT_TRIANGLE_LIST:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCount++;
					m_primitiveIndex = 0;
				}
				m_vertexIndex = m_primitiveCount * 3 + m_primitiveIndex;

				break;

			case Mesh::PT_TRIANGLE_STRIP:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCount++;
					m_primitiveIndex = 0;
				}
				m_vertexIndex = m_primitiveCount + m_primitiveIndex;

				break;
					
				case Mesh::PT_NONE:
					break;
			}

			if (m_vertexIndex >= GetRealVertexCount())
				return false;
			else
				return true;
		}
		size_t GetRealVertexCount()
		{
			if (m_mesh.IsIndexed())
				return m_mesh.GetIndexBuffer().GetIndexCount();
			else
				return m_mesh.GetVertexBuffer().GetVertexCount();
		}
		void Rewind()
		{
			m_primitiveCount = 0;
			m_primitiveIndex = -1;
		}
		uint32_t GetVertexIndex()
		{
			if (m_mesh.IsIndexed())
				return m_mesh.GetIndexBuffer().index[m_vertexIndex];
			else
				return m_vertexIndex;
		}
	private:
		Mesh& m_mesh;
		uint32_t m_vertexIndex;
		int m_primitiveCount; // Integer, because elsewise it couldn't be set to -1, which, at the moment, is a requirement.
		uint32_t m_primitiveIndex;
	};

	std::stack<Matrix> m_transformStack;
};

#endif
