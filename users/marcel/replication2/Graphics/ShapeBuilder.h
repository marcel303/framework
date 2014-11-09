#ifndef SHAPEBUILDER_H
#define SHAPEBUILDER_H
#pragma once

#include <algorithm>
//#include <boost/shared_ptr.hpp>
#include <stdint.h>
//#include <stack>
#include "MatrixStack.h"
#include "MemAllocators2.h"
#include "Mesh.h"
#include "ProcTexcoord.h"
#include "Types.h"

// FIXME, to h/cpp
#include <map>
#include "MxPlane.h"

class Edge
{
public:
	Edge()
	{
		m_vertex1 = 0;
		m_vertex2 = 0;
	}

	void Initialize(uint32_t vertex1, uint32_t vertex2, Vec3Arg v1, Vec3Arg v2, Vec3Arg v3)
	{
		if (vertex1 > vertex2)
		{
			uint32_t temp = vertex1;
			vertex1 = vertex2;
			vertex2 = temp;
		}

		m_vertex1 = vertex1;
		m_vertex2 = vertex2;

		const Vec3 delta1 = v2 - v1;
		const Vec3 delta2 = v3 - v2;

		Vec3 n1 = delta1;
		Vec3 n2 = n1 % delta2;
		Vec3 n3 = n1 % n2;

		n1.Normalize();
		n2.Normalize();
		n3.Normalize();

		const float d1 = n1 * v1;
		const float d2 = n2 * v1;
		const float d3 = n3 * v1;

		m_plane1.Setup(n1, d1);
		m_plane2.Setup(n2, d2);
		m_plane3.Setup(n3, d3);
	}

	uint32_t m_vertex1;
	uint32_t m_vertex2;

	Mx::Plane m_plane1; // d = 0..1
	Mx::Plane m_plane2; // d = 0
	Mx::Plane m_plane3; // d = 0

	uint32_t m_poly1;
	uint32_t m_poly2;

	bool operator<(const Edge& edge) const
	{
		if (m_vertex1 < edge.m_vertex1)
			return true;
		if (m_vertex1 > edge.m_vertex1)
			return false;

		if (m_vertex2 < edge.m_vertex2)
			return true;
		if (m_vertex2 > edge.m_vertex2)
			return false;

		return false;
	}
};

typedef std::map<Edge, int> EdgeTable;

class Patch
{
public:
	inline static Vec3 Eval(const Vec3* v, float t)
	{
		const float t1 = 1.0f - t;
		const float t2 = t;

		return
			v[0] * (t1 * t1 * t1) +
			v[1] * (3 * t1 * t1 * t2) +
			v[2] * (3 * t1 * t2 * t2) +
			v[3] * (t2 * t2 * t2);
	}

	inline static void EvalU(const Patch& v, float t, Vec3* out_u)
	{
		for (uint32_t i = 0; i < 4; ++i)
			out_u[i] = Eval(v.v[i], t);
	}

	inline static Vec3 EvalV(const Vec3* u, float t)
	{
		return Eval(u, t);
	}

	Vec3 v[4][4];
};

class PatchMeshLine
{
public:
	void SetSize(uint32_t s)
	{
		m_patches.resize(s);
	}

	std::vector<Patch> m_patches;
};

class PatchMesh
{
public:
	Patch& GetPatch(uint32_t x, uint32_t y)
	{
		return m_lines[y].m_patches[x];
	}

	void SetSize(uint32_t sx, uint32_t sy)
	{
		m_sx = sx;
		m_sy = sy;

		m_lines.resize(m_sy);

		for (uint32_t i = 0; i < m_sy; ++i)
			m_lines[i].SetSize(m_sx);
	}

	uint32_t m_sx;
	uint32_t m_sy;
	std::vector<PatchMeshLine> m_lines;
};

static MemAllocatorGeneric s_sbAllocs(1);

class ShapeBuilder
{
private:
	IMemAllocator * m_allocator;
	IMemAllocator * m_scratchAllocator;

public:
	enum AXIS
	{
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2
	};

	ShapeBuilder()
		: m_allocator(&s_sbAllocs)
		, m_scratchAllocator(&s_sbAllocs)
	{
	}

	ShapeBuilder(IMemAllocator * allocator, IMemAllocator * scratchAllocator)
		: m_allocator(allocator)
		, m_scratchAllocator(scratchAllocator)
	{
	}

	inline void Push(const Mat4x4 & transform)
	{
		m_transformStack.Push(transform);
	}

	void PushTranslation(Vec3Arg translation)
	{
		Mat4x4 transform;
		transform.MakeTranslation(translation);
		Push(transform);
	}

	void PushRotation(Vec3Arg rotation)
	{
		Mat4x4 transform;
		Mat4x4 transform1;
		Mat4x4 transform2;
		Mat4x4 transform3;
		transform1.MakeRotationX(rotation[0]);
		transform2.MakeRotationY(rotation[1]);
		transform3.MakeRotationZ(rotation[2]);
		transform = transform3 * transform2 * transform1;
		//transform.MakeRotationEuler(rotation);
		Push(transform);
	}

	void PushScaling(Vec3Arg scale)
	{
		Mat4x4 transform;
		transform.MakeScaling(scale);
		Push(transform);
	}

	inline void Pop()
	{
		m_transformStack.Pop();
	}

	inline const Mat4x4 & GetTransform()
	{
		return m_transformStack.Top();
	}

	inline uint32_t GetTransformStackDepth()
	{
		return m_transformStack.GetDepth();
	}

	bool CreateCircle(IMemAllocator * allocator, uint32_t resolution, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateLine(IMemAllocator * allocator, Vec3Arg p1, Vec3Arg p2, float thickness, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateQuad(IMemAllocator * allocator, AXIS upAxis, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateCube(IMemAllocator * allocator, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateGrid(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, AXIS upAxis, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateGridCube(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateDonut(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, float outerRadius, float innerRadius, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreateCilinder(IMemAllocator * allocator, uint32_t resolution, bool closed, AXIS upAxis, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);
	bool CreatePatch(IMemAllocator * allocator, PatchMesh& mesh, uint32_t resolutionU, uint32_t resolutionV, Mesh & out_mesh, uint32_t fvf = FVF_XYZ);

	bool Copy(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh);

	bool ConvertToFVF(IMemAllocator * allocator, Mesh & mesh, uint32_t fvf, Mesh& out_mesh);
	bool ConvertToIndexed(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh);
	bool ConvertToNonIndexed(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh);

	bool GetEdges(IMemAllocator * allocator, Mesh & mesh, EdgeTable & out_edges);
	bool RemoveTJunctions(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh);

	bool Merge(IMemAllocator * allocator, Mesh & mesh1, Mesh & mesh2, Mesh & out_mesh);
	bool Merge(IMemAllocator * allocator, Mesh ** meshes, uint32_t meshCount, Mesh & out_mesh);
	bool Merge(IMemAllocator * allocator, std::vector<Mesh *> & meshes, Mesh & out_mesh);

	bool CalculateTexcoords(Mesh & out_mesh, uint32_t sampler, ProcTexcoord * proc);
	bool CalculateNormals(Mesh & out_mesh);
	bool CalculateCoordinateFrameFromUV(Mesh & out_mesh, uint32_t sampler, uint32_t out_sampler);

	bool Colorize(Mesh & out_mesh, uint32_t r, uint32_t g, uint32_t b, uint32_t a = 255);
	bool UpdatePatch(PatchMesh & out_mesh);

private:
	inline Vec3 Mul(Vec3Arg vector) const
	{
		return m_transformStack.Top() * vector;
	}

	inline static void CopyVertex(Mesh & srcMesh, uint32_t srcIndex, Mesh & dstMesh, uint32_t dstIndex)
	{
		const uint32_t srcFvf = srcMesh.GetVB()->GetFVF();
		const uint32_t dstFvf = dstMesh.GetVB()->GetFVF();

		// Copy non-indexed components.

		if ((srcFvf & FVF_XYZ) && (dstFvf & FVF_XYZ))
			dstMesh.GetVB()->SetPosition(
				dstIndex,
				srcMesh.GetVB()->position[srcIndex][0],
				srcMesh.GetVB()->position[srcIndex][1],
				srcMesh.GetVB()->position[srcIndex][2]);

		/*
		if ((srcFvf & FVF_COLOR) && (dstFvf & FVF_COLOR))
			dstMesh.GetVB()->SetColor(
				dstIndex,
				srcMesh.GetVB()->color[srcIndex].r,
				srcMesh.GetVB()->color[srcIndex].g,
				srcMesh.GetVB()->color[srcIndex].b,
				srcMesh.GetVB()->color[srcIndex].a);
		*/

		if ((srcFvf & FVF_NORMAL) && (dstFvf & FVF_NORMAL))
			dstMesh.GetVB()->SetNormal(
				dstIndex,
				srcMesh.GetVB()->normal[srcIndex][0],
				srcMesh.GetVB()->normal[srcIndex][1],
				srcMesh.GetVB()->normal[srcIndex][2]);

		/*
		if ((srcFvf & FVF_TANGENT) && (dstFvf & FVF_TANGENT))
			dstMesh.GetVB()->SetTangent(
				dstIndex,
				srcMesh.GetVB()->tangent[srcIndex].x,
				srcMesh.GetVB()->tangent[srcIndex].y,
				srcMesh.GetVB()->tangent[srcIndex].z);

		if ((srcFvf & FVF_BINORMAL) && (dstFvf & FVF_BINORMAL))
			dstMesh.GetVB()->SetBiNormal(
				dstIndex,
				srcMesh.GetVB()->biNormal[srcIndex].x,
				srcMesh.GetVB()->biNormal[srcIndex].y,
				srcMesh.GetVB()->biNormal[srcIndex].z);
		*/

		// Copy indexed texture coordinate components.

		const uint32_t srcTexCnt = srcMesh.GetVB()->GetTexCnt();
		const uint32_t dstTexCnt = dstMesh.GetVB()->GetTexCnt();
		const uint32_t minTexCnt = std::min<uint32_t>(srcTexCnt, dstTexCnt);

		for (uint32_t i = 0; i < minTexCnt; ++i)
			dstMesh.GetVB()->SetTex(
				dstIndex,
				i,
				srcMesh.GetVB()->tex[i][srcIndex][0],
				srcMesh.GetVB()->tex[i][srcIndex][1]/*,
				srcMesh.GetVB()->tex[i][srcIndex][2],
				srcMesh.GetVB()->tex[i][srcIndex][3]*/);

		// Copy indexed blend weight components.

		/*
		uint32_t srcBlendWeightCnt = srcMesh.GetVB()->GetBlendWeightCnt();
		uint32_t dstBlendWeightCnt = dstMesh.GetVB()->GetBlendWeightCnt();
		uint32_t minBlendWeightCnt = std::min<int>(srcBlendWeightCnt, dstBlendWeightCnt);

		for (uint32_t i = 0; i < minBlendWeightCnt; ++i)
			dstMesh.GetVB()->blendn[i][dstIndex] = srcMesh.GetVB()->blendn[i][srcIndex];
		*/
	}

	class VertexIterator
	{
	public:
		inline VertexIterator(Mesh & mesh)
			: m_mesh(mesh)
		{
			Rewind();
		}

		inline bool Next()
		{
			switch (m_mesh.GetPT())
			{
			case PT_TRIANGLE_FAN:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCnt++;
					m_primitiveIndex = 0;
				}
				if (m_primitiveIndex == 0)
					m_vertexIndex = 0;
				else
					m_vertexIndex = m_primitiveCnt + m_primitiveIndex;

				break;

			case PT_TRIANGLE_LIST:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCnt++;
					m_primitiveIndex = 0;
				}
				m_vertexIndex = m_primitiveCnt * 3 + m_primitiveIndex;

				break;

			case PT_TRIANGLE_STRIP:

				m_primitiveIndex++;
				if (m_primitiveIndex == 3)
				{
					m_primitiveCnt++;
					m_primitiveIndex = 0;
				}
				m_vertexIndex = m_primitiveCnt + m_primitiveIndex;

				break;
			}

			if (m_vertexIndex >= GetRealVertexCnt())
				return false;
			else
				return true;
		}

		inline uint32_t GetRealVertexCnt() const
		{
			if (m_mesh.IsIndexed())
				return m_mesh.GetIB()->GetIndexCnt();
			else
				return m_mesh.GetVB()->GetVertexCnt();
		}

		inline void Rewind()
		{
			m_primitiveCnt = 0;
			m_primitiveIndex = -1;
		}

		inline uint32_t GetVertexIndex() const
		{
			if (m_mesh.IsIndexed())
				return m_mesh.GetIB()->index[m_vertexIndex];
			else
				return m_vertexIndex;
		}

	private:
		Mesh & m_mesh;
		uint32_t m_vertexIndex;
		uint32_t m_primitiveCnt;
		uint32_t m_primitiveIndex; // Possibly -1.
	};

	MatrixStack m_transformStack;
};

#endif
