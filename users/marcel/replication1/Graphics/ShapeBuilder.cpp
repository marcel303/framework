#include <map>
#include <math.h>
#include "Calc.h"
#include "ShapeBuilder.h"

static inline Vec3 UVToXYZ(float u, float v, ShapeBuilder::AXIS upAxis, float w = 0.0f)
{
	Vec3 xyz;

	uint32_t a1 = upAxis + 1;
	uint32_t a2 = upAxis + 2;
	uint32_t a3 = upAxis;

	if (a1 >= 3)
		a1 -= 3;
	if (a2 >= 3)
		a2 -= 3;

	xyz[a1] = u;
	xyz[a2] = v;
	xyz[a3] = w;

	return xyz;
}

bool ShapeBuilder::CreateCircle(IMemAllocator * allocator, uint32_t resolution, Mesh & out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, resolution + 1, fvf, resolution * 3);

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	for (uint32_t i = 0; i < resolution; ++i)
	{
		float angle = Calc::mPI * 2.0f / resolution * i;

		float x = cosf(angle);
		float y = sinf(angle);

		Vec3 xyz = Mul(Vec3(x, y, 0.0f));
		vb.SetPosition(i, xyz[0], xyz[1], xyz[2]);
	}

	Vec3 xyz = Mul(Vec3(0.0f, 0.0f, 0.0f));
	vb.SetPosition(resolution, xyz[0], xyz[1], xyz[2]);

	for (uint32_t i = 0; i < resolution; ++i)
	{
		uint32_t index1r = i;
		uint32_t index2r = i + 1;
		if (index2r >= resolution)
			index2r -= resolution;

		uint16_t index1 = static_cast<uint16_t>(index1r);
		uint16_t index2 = static_cast<uint16_t>(index2r);
		uint16_t center = static_cast<uint16_t>(resolution);

		ib.index[i * 3 + 0] = index1;
		ib.index[i * 3 + 1] = index2;
		ib.index[i * 3 + 2] = center;
	}

	return true;
}

bool ShapeBuilder::CreateLine(IMemAllocator * allocator, Vec3Arg p1, Vec3Arg p2, float thickness, Mesh & out_mesh, uint32_t fvf)
{
	Vec3 d = p2 - p1;
	float length = d.CalcSize();

	Mat4x4 lookat;
	lookat.MakeLookat(p1, p2, Vec3(0.0f, 0.0f, 1.0f));
	lookat = lookat.CalcInv();

	Push(lookat);
		PushScaling(Vec3(thickness * 0.5f, thickness * 0.5f, length * 0.5f));
			PushTranslation(Vec3(0.0f, 0.0f, 1.0f));
				CreateCube(allocator, out_mesh, fvf);
			Pop();
		Pop();
	Pop();
	
	return true;
}

bool ShapeBuilder::CreateQuad(IMemAllocator * allocator, AXIS upAxis, Mesh & out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, 4, fvf, 2 * 3);

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	const Vec3 position[4] =
	{
		Mul(UVToXYZ(-1.0f, -1.0f, upAxis)),
		Mul(UVToXYZ(+1.0f, -1.0f, upAxis)),
		Mul(UVToXYZ(+1.0f, +1.0f, upAxis)),
		Mul(UVToXYZ(-1.0f, +1.0f, upAxis))
	};

	vb.SetPosition(0, position[0][0], position[0][1], position[0][2]);
	vb.SetPosition(1, position[1][0], position[1][1], position[1][2]);
	vb.SetPosition(2, position[2][0], position[2][1], position[2][2]);
	vb.SetPosition(3, position[3][0], position[3][1], position[3][2]);

	ib.index[0] = 0;
	ib.index[1] = 1;
	ib.index[2] = 2;

	ib.index[3] = 0;
	ib.index[4] = 2;
	ib.index[5] = 3;

	return true;
}

bool ShapeBuilder::CreateCube(IMemAllocator * allocator, Mesh & out_mesh, uint32_t fvf)
{
	IMemAllocator * pScratchAllocator = m_scratchAllocator;
	//IMemAllocator * pScratchAllocator = m_allocator;

	Mesh * sides[6] = { 0 };

	PushTranslation(Vec3(+1.0f, 0.0f, 0.0f));
		sides[0] = new (pScratchAllocator) Mesh();
		CreateQuad(pScratchAllocator, AXIS_X, *sides[0], fvf);
	Pop();
	PushTranslation(Vec3(-1.0f, 0.0f, 0.0f));
		PushRotation(Vec3(0.0f, Calc::mPI, 0.0f));
			sides[1] = new (pScratchAllocator) Mesh();
			CreateQuad(pScratchAllocator, AXIS_X, *sides[1], fvf);
		Pop();
	Pop();

	PushTranslation(Vec3(0.0f, +1.0f, 0.0f));
		sides[2] = new (pScratchAllocator) Mesh();
		CreateQuad(pScratchAllocator, AXIS_Y, *sides[2], fvf);
	Pop();
	PushTranslation(Vec3(0.0f, -1.0f, 0.0f));
		PushRotation(Vec3(0.0f, 0.0f, Calc::mPI));
			sides[3] = new (pScratchAllocator) Mesh();
			CreateQuad(pScratchAllocator, AXIS_Y, *sides[3], fvf);
		Pop();
	Pop();

	PushTranslation(Vec3(0.0f, 0.0f, +1.0f));
		sides[4] = new (pScratchAllocator) Mesh();
		CreateQuad(pScratchAllocator, AXIS_Z, *sides[4], fvf);
	Pop();
	PushTranslation(Vec3(0.0f, 0.0f, -1.0f));
		PushRotation(Vec3(Calc::mPI, 0.0f, 0.0f));
			sides[5] = new (pScratchAllocator) Mesh();
			CreateQuad(pScratchAllocator, AXIS_Z, *sides[5], fvf);
		Pop();
	Pop();

	Merge(allocator, sides, 6, out_mesh);

	for (uint32_t i = 6; i != 0; --i)
	{
		pScratchAllocator->SafeDelete(sides[i - 1]);
	}

	return true;
}

bool ShapeBuilder::CreateGrid(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, AXIS upAxis, Mesh & out_mesh, uint32_t fvf)
{
	const uint32_t vertexCount = resolutionU * resolutionV;
	const uint32_t cellCount = (resolutionU - 1) * (resolutionV - 1);

	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, vertexCount, fvf, cellCount * 2 * 3);

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	const uint16_t resolutionU16 = static_cast<uint16_t>(resolutionU);
	const uint16_t resolutionV16 = static_cast<uint16_t>(resolutionV);

	for (uint16_t i = 0; i < resolutionU16; ++i)
	{
		for (uint16_t j = 0; j < resolutionV16; ++j)
		{
			uint16_t vertexIndex = i + j * resolutionU16;

			float u = i / float(resolutionU16 - 1);
			float v = j / float(resolutionV16 - 1);

			float x = (u - 0.5f) * 2.0f;
			float y = (v - 0.5f) * 2.0f;

			Vec3 xyz = Mul(UVToXYZ(x, y, upAxis));
			vb.SetPosition(vertexIndex, xyz[0], xyz[1], xyz[2]);
		}
	}

	for (uint16_t i = 0; i < resolutionU16 - 1; ++i)
	{
		for (uint16_t j = 0; j < resolutionV16 - 1; ++j)
		{
			uint16_t baseIndex = (i + j * (resolutionU16 - 1)) * 2 * 3;
			uint16_t topLeftVertex = i + j * resolutionU16;

			ib.index[baseIndex + 0] = topLeftVertex + 0;
			ib.index[baseIndex + 1] = topLeftVertex + 1;
			ib.index[baseIndex + 2] = topLeftVertex + resolutionU16 + 1;

			ib.index[baseIndex + 3] = topLeftVertex + 0;
			ib.index[baseIndex + 4] = topLeftVertex + resolutionU16 + 1;
			ib.index[baseIndex + 5] = topLeftVertex + resolutionU16 + 0;
		}
	}

	return true;
}

bool ShapeBuilder::CreateGridCube(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, Mesh & out_mesh, uint32_t fvf)
{
	IMemAllocator * pScratchAllocator = m_scratchAllocator;

	Mesh * sides[6];

	PushTranslation(Vec3(+1.0f, 0.0f, 0.0f));
		sides[0] = new(pScratchAllocator) Mesh();
		CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_X, *sides[0], fvf);
	Pop();
	PushTranslation(Vec3(-1.0f, 0.0f, 0.0f));
		PushRotation(Vec3(0.0f, Calc::mPI, 0.0f));
			sides[1] = new(pScratchAllocator) Mesh();
			CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_X, *sides[1], fvf);
		Pop();
	Pop();

	PushTranslation(Vec3(0.0f, +1.0f, 0.0f));
		sides[2] = new(pScratchAllocator) Mesh();
		CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_Y, *sides[2], fvf);
	Pop();
	PushTranslation(Vec3(0.0f, -1.0f, 0.0f));
		PushRotation(Vec3(0.0f, 0.0f, Calc::mPI));
			sides[3] = new(pScratchAllocator) Mesh();
			CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_Y, *sides[3], fvf);
		Pop();
	Pop();

	PushTranslation(Vec3(0.0f, 0.0f, +1.0f));
		sides[4] = new(pScratchAllocator) Mesh();
		CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_Z, *sides[4], fvf);
	Pop();
	PushTranslation(Vec3(0.0f, 0.0f, -1.0f));
		PushRotation(Vec3(Calc::mPI, 0.0f, 0.0f));
			sides[5] = new(pScratchAllocator) Mesh();
			CreateGrid(pScratchAllocator, resolutionU, resolutionV, AXIS_Z, *sides[5], fvf);
		Pop();
	Pop();

	Merge(allocator, sides, 6, out_mesh);

	for (uint32_t i = 0; i < 6; ++i)
		pScratchAllocator->SafeDelete(sides[5 - i]);

	return true;
}

bool ShapeBuilder::CreateDonut(IMemAllocator * allocator, uint32_t resolutionU, uint32_t resolutionV, float outerRadius, float innerRadius, Mesh & out_mesh, uint32_t fvf)
{
	const uint32_t vertexCount = resolutionU * resolutionV;
	const uint32_t triangleCount = resolutionU * resolutionV * 2;
	const uint32_t indexCount = triangleCount * 3;

	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, vertexCount, fvf, indexCount);

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	const uint16_t resolutionU16 = static_cast<uint16_t>(resolutionU);
	const uint16_t resolutionV16 = static_cast<uint16_t>(resolutionV);

	for (uint16_t i = 0; i < resolutionU16; ++i)
	{
		const float angle1 = i / float(resolutionU16) * 2.0f * Calc::mPI;
		PushRotation(Vec3(-angle1, 0.0f, 0.0f));
			PushTranslation(Vec3(0.0f, outerRadius, 0.0f));
				for (uint16_t j = 0; j < resolutionV16; ++j)
				{
					const float angle2 = j / float(resolutionV16) * 2.0f * Calc::mPI;

					PushRotation(Vec3(0.0f, 0.0f, angle2));
						const Vec3 position = Mul(Vec3(innerRadius, 0.0f, 0.0f));
						vb.position[i + j * resolutionU][0] = position[0];
						vb.position[i + j * resolutionU][1] = position[1];
						vb.position[i + j * resolutionU][2] = position[2];
					Pop();
				}
			Pop();
		Pop();
	}

	for (uint16_t i = 0; i < resolutionU16; ++i)
	{
		for (uint16_t j = 0; j < resolutionV16; ++j)
		{
			const uint16_t baseIndex = (i + j * resolutionU16) * 2 * 3;

			const uint16_t index1 = ((i + 0) % resolutionU16) + ((j + 0) % resolutionV16) * resolutionU16;
			const uint16_t index2 = ((i + 1) % resolutionU16) + ((j + 0) % resolutionV16) * resolutionU16;
			const uint16_t index3 = ((i + 1) % resolutionU16) + ((j + 1) % resolutionV16) * resolutionU16;

			const uint16_t index4 = ((i + 0) % resolutionU16) + ((j + 0) % resolutionV16) * resolutionU16;
			const uint16_t index5 = ((i + 1) % resolutionU16) + ((j + 1) % resolutionV16) * resolutionU16;
			const uint16_t index6 = ((i + 0) % resolutionU16) + ((j + 1) % resolutionV16) * resolutionU16;

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

bool ShapeBuilder::CreateCilinder(IMemAllocator * allocator, uint32_t resolution, bool closed, AXIS upAxis, Mesh & out_mesh, uint32_t fvf)
{
	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false,
		resolution * 2 + (closed ? resolution * 2 : 0), fvf,
		resolution * 2 * 3 + (closed ? (resolution - 2) * 3 * 2 : 0));

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	const uint16_t resolution16 = static_cast<uint16_t>(resolution);

	for (uint16_t i = 0; i < resolution16; ++i)
	{
		float angle = 2.0f * Calc::mPI / resolution * i;
		Vec3 position;

		position = Mul(UVToXYZ(cosf(angle), sinf(angle), upAxis, -1.0f));
		vb.position[i * 2 + 0][0] = position[0];
		vb.position[i * 2 + 0][1] = position[1];
		vb.position[i * 2 + 0][2] = position[2];
		if (closed)
		{
			vb.position[(resolution16 + i) * 2 + 0][0] = position[0];
			vb.position[(resolution16 + i) * 2 + 0][1] = position[1];
			vb.position[(resolution16 + i) * 2 + 0][2] = position[2];
		}

		position = Mul(UVToXYZ(cosf(angle), sinf(angle), upAxis, +1.0f));
		vb.position[i * 2 + 1][0] = position[0];
		vb.position[i * 2 + 1][1] = position[1];
		vb.position[i * 2 + 1][2] = position[2];
		if (closed)
		{
			vb.position[(resolution16 + i) * 2 + 1][0] = position[0];
			vb.position[(resolution16 + i) * 2 + 1][1] = position[1];
			vb.position[(resolution16 + i) * 2 + 1][2] = position[2];
		}
	}

	for (uint16_t i = 0; i < resolution16; ++i)
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
		uint16_t index = resolution16 * 2 * 3;

		for (uint16_t i = 0; i < resolution16 - 2; ++i)
		{
			ib.index[index + 0] = (resolution16 + (i + 2) % resolution16) * 2 + 0;
			ib.index[index + 1] = (resolution16 + (i + 1) % resolution16) * 2 + 0;
			ib.index[index + 2] = resolution16 * 2 + 0;

			index += 3;
		}

		for (uint16_t i = 0; i < resolution16 - 2; ++i)
		{
			ib.index[index + 0] = resolution16 * 2 + 1;
			ib.index[index + 1] = (resolution16 + (i + 1) % resolution16) * 2 + 1;
			ib.index[index + 2] = (resolution16 + (i + 2) % resolution16) * 2 + 1;

			index += 3;
		}
	}

	return true;
}

bool ShapeBuilder::CreatePatch(IMemAllocator * allocator, PatchMesh & mesh, uint32_t resolutionU, uint32_t resolutionV, Mesh & out_mesh, uint32_t fvf)
{
	uint32_t quadCountX = mesh.m_sx;
	uint32_t quadCountY = mesh.m_sy;
	uint32_t quadCount = quadCountX * quadCountY;

	uint32_t vertexCountQX = resolutionU;
	uint32_t vertexCountQY = resolutionV;
	uint32_t vertexCountQ = vertexCountQX * vertexCountQY;

	uint32_t vertexCountX = vertexCountQX * quadCountX;
	uint32_t vertexCountY = vertexCountQY * quadCountY;
	uint32_t vertexCount = vertexCountX * vertexCountY;

	uint32_t triangleCount = quadCount * (vertexCountQX - 1) * (vertexCountQY - 1) * 2;
	uint32_t indexCount = triangleCount * 3;

	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, vertexCount, fvf, indexCount);

	ResVB & vb = *out_mesh.GetVB();
	ResIB & ib = *out_mesh.GetIB();

	// Fill vertex buffer.

	#define INDEX(x, y, u, v) static_cast<uint16_t>(((((x) + (y) * quadCountX) * vertexCountQ) + ((u) + (v) * resolutionU)))

	Vec3 temp[4];

	const float invResU = 1.0f / (resolutionU - 1.0f);
	const float invResV = 1.0f / (resolutionV - 1.0f);

	for (uint32_t px = 0; px < mesh.m_sx; ++px)
	{
		for (uint32_t py = 0; py < mesh.m_sy; ++py)
		{
			const Patch & patch = mesh.GetPatch(px, py);

			for (uint32_t u = 0; u < resolutionU; ++u)
			{
				const float tu = u * invResU;

				Patch::EvalU(patch, tu, temp);

				for (uint32_t v = 0; v < resolutionV; ++v)
				{
					const float tv = v * invResV;

					vb.position[INDEX(px, py, u, v)] = Patch::EvalV(temp, tv);
				}
			}
		}
	}

	// Fill index buffer.
	uint32_t index = 0;
	for (uint32_t px = 0; px < mesh.m_sx; ++px)
	{
		for (uint32_t py = 0; py < mesh.m_sy; ++py)
		{
			for (uint32_t u = 0; u < resolutionU - 1; ++u)
			{
				for (uint32_t v = 0; v < resolutionV - 1; ++v)
				{
					uint16_t index1 = INDEX(px, py, u + 0, v + 0);
					uint16_t index2 = INDEX(px, py, u + 1, v + 0);
					uint16_t index3 = INDEX(px, py, u + 1, v + 1);
					uint16_t index4 = INDEX(px, py, u + 0, v + 1);

					ib.index[index++] = index1;
					ib.index[index++] = index2;
					ib.index[index++] = index3;
					ib.index[index++] = index1;
					ib.index[index++] = index3;
					ib.index[index++] = index4;
				}
			}
		}
	}

	return true;
}

bool ShapeBuilder::Copy(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh)
{
	ResVB & vb = *mesh.GetVB();
	ResIB & ib = *mesh.GetIB();

	out_mesh.Initialize(
		allocator,
		mesh.GetPT(),
		false,
		vb.GetVertexCnt(),
		vb.GetFVF(),
		ib.GetIndexCnt()/*,
		ib.format*/);

	for (uint32_t i = 0; i < vb.GetVertexCnt(); ++i)
		CopyVertex(mesh, i, out_mesh, i);

	for (uint32_t i = 0; i < ib.GetIndexCnt(); ++i)
		out_mesh.GetIB()->index[i] = ib.index[i];

	return true;
}

#define CONV_BEGIN() \
	Mesh out_temp2; \
	bool useTemp = &out_mesh == &mesh; \
	Mesh & out = useTemp ? out_temp2 : out_mesh;

#define CONV_END() \
	if (&out_mesh == &mesh) \
		Copy(allocator, out, out_mesh);

bool ShapeBuilder::ConvertToFVF(IMemAllocator * allocator, Mesh & mesh, uint32_t fvf, Mesh & out_mesh)
{
	CONV_BEGIN();

	ResVB & vb = *mesh.GetVB();
	ResIB & ib = *mesh.GetIB();

	out.Initialize(
		useTemp ? m_scratchAllocator : allocator,
		mesh.GetPT(),
		false,
		vb.GetVertexCnt(),
		fvf,
		ib.GetIndexCnt()/*,
		ib.format*/);

	for (uint32_t i = 0; i < vb.GetVertexCnt(); ++i)
		CopyVertex(mesh, i, out, i);

	for (uint32_t i = 0; i < ib.GetIndexCnt(); ++i)
		out.GetIB()->index[i] = ib.index[i];

	CONV_END();

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

	bool operator<(const MeshVertex & other) const
	{
		const uint32_t fvf = other.m_mesh->GetVB()->GetFVF();

		if (other.m_mesh->GetVB()->GetFVF() != fvf)
			FASSERT(0);

		ResVB & vb1 = *m_mesh->GetVB();
		ResVB & vb2 = *other.m_mesh->GetVB();

		const float eps = 0.01f;

		if (fvf & FVF_XYZ)
		{
			if (vb1.position[m_srcIndex][0] < vb2.position[other.m_srcIndex][0] - eps)
				return true;
			if (vb1.position[m_srcIndex][0] > vb2.position[other.m_srcIndex][0] + eps)
				return false;

			if (vb1.position[m_srcIndex][1] < vb2.position[other.m_srcIndex][1] - eps)
				return true;
			if (vb1.position[m_srcIndex][1] > vb2.position[other.m_srcIndex][1] + eps)
				return false;

			if (vb1.position[m_srcIndex][2] < vb2.position[other.m_srcIndex][2] - eps)
				return true;
			if (vb1.position[m_srcIndex][2] > vb2.position[other.m_srcIndex][2] + eps)
				return false;
		}

		/*if (fvf & FVF_COLOR)
		{
			if (vb1.color[m_srcIndex].c < vb2.color[other.m_srcIndex].c)
				return true;
			if (vb1.color[m_srcIndex].c > vb2.color[other.m_srcIndex].c)
				return false;
		}*/

		uint32_t textureCount = vb1.GetTexCnt();

		for (uint32_t i = 0; i < textureCount; ++i)
		{
			if (vb1.tex[i][m_srcIndex][0] < vb2.tex[i][other.m_srcIndex][0] - eps)
				return true;
			if (vb1.tex[i][m_srcIndex][0] > vb2.tex[i][other.m_srcIndex][0] + eps)
				return false;

			if (vb1.tex[i][m_srcIndex][1] < vb2.tex[i][other.m_srcIndex][1] - eps)
				return true;
			if (vb1.tex[i][m_srcIndex][1] > vb2.tex[i][other.m_srcIndex][1] + eps)
				return false;

			/*
			if (vb1.tex[i][m_srcIndex].s < vb2.tex[i][other.m_srcIndex].s)
				return true;
			if (vb1.tex[i][m_srcIndex].s > vb2.tex[i][other.m_srcIndex].s)
				return false;

			if (vb1.tex[i][m_srcIndex].t < vb2.tex[i][other.m_srcIndex].t)
				return true;
			if (vb1.tex[i][m_srcIndex].t > vb2.tex[i][other.m_srcIndex].t)
				return false;*/
		}

		/*
		uint32_t blendWeightCount = vb1.GetBlendWeightCount();

		for (uint32_t i = 0; i < blendWeightCount; ++i)
		{
			if (vb1.blendn[i][m_srcIndex] < vb2.blendn[i][other.m_srcIndex])
				return true;
			if (vb1.blendn[i][m_srcIndex] > vb2.blendn[i][other.m_srcIndex])
				return false;
		}*/

		return false;
	}

};

bool ShapeBuilder::ConvertToIndexed(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh)
{
	CONV_BEGIN();

	typedef std::map<
		MeshVertex,
		int,
		std::less<MeshVertex>/*,
		boost::fast_pool_allocator< std::pair<MeshVertex, int> > fixme */
		> VertexMap;

	// todo: allocate scratch mem
	VertexMap vertices;
	std::vector<int> indices;

	ResVB & vb = *mesh.GetVB();
	ResIB & ib = *mesh.GetIB();

	#undef max
	indices.reserve(std::max(vb.GetVertexCnt(), ib.GetIndexCnt()));

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

	//printf("New mesh: %d vertices, %d indices.\n", vertices.size(), indices.size());

	PRIMITIVE_TYPE primitiveType = PT_TRIANGLE_LIST;
	uint32_t vertexCount = vertices.size();
	uint32_t vertexFVF   = vb.GetFVF();
	uint32_t indexCount  = indices.size();
	/*
	IndexBuffer::INDEX_FORMAT indexFormat;
	if (indexCount <= 65535)
		indexFormat = IndexBuffer::IF_INDEX16;
	else
		indexFormat = IndexBuffer::IF_INDEX32;*/

	out.Initialize(allocator, primitiveType, false, vertexCount, vertexFVF, indexCount/*, indexFormat*/);

	// Copy vertices.
	for (VertexMap::iterator i = vertices.begin(); i != vertices.end(); ++i)
		CopyVertex(mesh, i->first.m_srcIndex, out, i->first.m_dstIndex);

	// Copy indices.
	for (size_t i = 0; i < indices.size(); ++i)
		out.GetIB()->index[i] = indices[i];

	CONV_END();

	return true;
}

bool ShapeBuilder::ConvertToNonIndexed(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh)
{
	CONV_BEGIN();

	VertexIterator i(mesh);
	uint32_t index = 0;

	out.Initialize(
		useTemp ? m_scratchAllocator : allocator,
		mesh.GetPT(),
		false,
		i.GetRealVertexCnt(),
		mesh.GetVB()->GetFVF(), 0);
	
	while (i.Next())
	{
		CopyVertex(mesh, i.GetVertexIndex(), out, index);

		index++;
	}

	CONV_END();

	return true;
}

bool ShapeBuilder::GetEdges(IMemAllocator * allocator, Mesh & mesh, EdgeTable & out_edges)
{
	ResVB & vb = *mesh.GetVB();

	VertexIterator i(mesh);

	while (i.Next())
	{
		uint32_t vertex1 = i.GetVertexIndex(); i.Next();
		uint32_t vertex2 = i.GetVertexIndex(); i.Next();
		uint32_t vertex3 = i.GetVertexIndex();

		Vec3 v1 = vb.position[vertex1];
		Vec3 v2 = vb.position[vertex2];
		Vec3 v3 = vb.position[vertex3];

		Edge e1;
		Edge e2;
		Edge e3;

		e1.Initialize(vertex1, vertex2, v1, v2, v3);
		e2.Initialize(vertex2, vertex3, v2, v3, v1);
		e3.Initialize(vertex3, vertex1, v3, v1, v2);

		out_edges[e1] = 1;
		out_edges[e2] = 1;
		out_edges[e3] = 1;
	}

	return true;
}

// FIXME.
typedef struct
{
	uint32_t vertex1;
	uint32_t vertex2;
	float t;
} SplitOp;

bool ShapeBuilder::RemoveTJunctions(IMemAllocator * allocator, Mesh & mesh, Mesh & out_mesh)
{
	CONV_BEGIN();

	ResVB & vb = *mesh.GetVB();

	EdgeTable edges;

	if (!GetEdges(m_scratchAllocator, mesh, edges))
		return false;

	std::vector<SplitOp> ops;

	// For each edge, check if there are points on it. If so, split edge.
	for (EdgeTable::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		const Edge & e = i->first;

		VertexIterator j(mesh);

		while (j.Next())
		{
			uint32_t vertex = j.GetVertexIndex();

			if (vertex != e.m_vertex1 && vertex != e.m_vertex2)
			{
				Vec3 p = vb.position[vertex];

				float d1 = e.m_plane1 * p;
				float d2 = e.m_plane2 * p;
				float d3 = e.m_plane3 * p;

				if (d1 >= 0.0f && d1 <= 1.0f && d2 == 0.0f && d3 == 0.0f)
				{
					printf("Detected T-Junction (%d -> %d vs %d).\n", e.m_vertex1, e.m_vertex2, vertex);

					SplitOp op;
					op.vertex1 = e.m_vertex1;
					op.vertex2 = e.m_vertex2;
					op.t = d1;

					ops.push_back(op);
					
					printf("Added split op @ t = %f.\n", d1);
				}
			}
		}
	}

	printf("T-Junction removal requires %d splits, generating %d new vertices.\n", ops.size(), ops.size());

	CONV_END();

	return true;
}

bool ShapeBuilder::Merge(IMemAllocator * allocator, Mesh & mesh1, Mesh & mesh2, Mesh & out_mesh)
{
	Mesh * meshes[2] = { &mesh1, &mesh2 };

	return Merge(allocator, meshes, 2, out_mesh);
}

bool ShapeBuilder::Merge(IMemAllocator * allocator, Mesh ** meshes, uint32_t meshCount, Mesh & out_mesh)
{
	if (meshCount == 0)
		return true;

	uint32_t fvf = meshes[0]->GetVB()->GetFVF();
	//PRIMITIVE_TYPE primitiveType = meshes[0]->GetPT();
	//bool isIndexed = meshes[0]->IsIndexed();

	// The meshes share the same properties. We can convert.

	uint32_t totalPrimitiveCount = 0;

	for (uint32_t i = 0; i < meshCount; ++i)
		totalPrimitiveCount += meshes[i]->GetPrimitiveCount();

	out_mesh.Initialize(allocator, PT_TRIANGLE_LIST, false, totalPrimitiveCount * 3, fvf, 0);

	uint32_t vertexIndex = 0;

	for (uint32_t i = 0; i < meshCount; ++i)
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

bool ShapeBuilder::Merge(IMemAllocator * allocator, std::vector<Mesh *> & meshes, Mesh & out_mesh)
{
	return Merge(allocator, &meshes[0], meshes.size(), out_mesh);
}

bool ShapeBuilder::CalculateTexcoords(Mesh & mesh, uint32_t sampler, ProcTexcoord * proc)
{
	ResVB & vb = *mesh.GetVB();
	uint32_t vertexCount = vb.GetVertexCnt();

	bool hasXYZ = vb.GetFVF() & FVF_XYZ ? true : false;
	bool hasNormal = vb.GetFVF() & FVF_NORMAL ? true : false;

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		Vec3 position;
		if (hasXYZ)
		{
			position[0] = vb.position[i][0];
			position[1] = vb.position[i][1];
			position[2] = vb.position[i][2];
		}

		Vec3 normal;
		if (hasNormal)
		{
			normal[0] = vb.normal[i][0];
			normal[1] = vb.normal[i][1];
			normal[2] = vb.normal[i][2];
		}

		Vec3 texcoords = proc->Generate(position, normal);

		vb.SetTex(i, sampler,
			texcoords[0],
			texcoords[1]/*,
			texcoords[2],
			texcoords[3]*/);
	}

	return true;
}

bool ShapeBuilder::CalculateNormals(Mesh & mesh)
{
	ResVB & vb = *mesh.GetVB();

	uint32_t vertexCount = vb.GetVertexCnt();

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		vb.normal[i][0] = 0.0f;
		vb.normal[i][1] = 0.0f;
		vb.normal[i][2] = 0.0f;
	}

	VertexIterator j(mesh);

	while (j.Next())
	{
		uint32_t index[3];
		index[0] = j.GetVertexIndex(); j.Next();
		index[1] = j.GetVertexIndex(); j.Next();
		index[2] = j.GetVertexIndex();

		Vec3 position[3];

		for (uint32_t i = 0; i < 3; ++i)
			position[i] = vb.position[index[i]];

		Vec3 delta[2];

		delta[0] = position[1] - position[0];
		delta[1] = position[2] - position[1];

		Vec3 normal = delta[0] % delta[1];
		normal.Normalize();

		for (uint32_t i = 0; i < 3; ++i)
			vb.normal[index[i]] += normal;
	}

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		Vec3 normal = vb.normal[i];

		normal.Normalize();

		vb.normal[i] = normal;
	}

	return true;
}

static Vec3 CalcUV(Vec3 p1, Vec3 p2, Vec3 p3, Vec4 c1, Vec4 c2, Vec4 c3, uint32_t coord)
{
	// Make points array.
	Vec3 p[3] = { p1, p2, p3 };
	Vec4 c[3] = { c1, c2, c3 };

#if 0
	//     P_middle
	//        * .
	//       /|   .
	//      / |     .       result = delta[coord]/delta[axis] between P_middle & P_interp.
	//     /  |       .
	//    *---*---------*
	//     P_interp

	Vec3 r;

	for (uint32_t axis = 0; axis < 3; ++axis)
	{
		// Calculate middle vertex.
		uint32_t min = 0;
		uint32_t max = 0;

		if (p[1][axis] < p[min][axis])
			min = 1;
		if (p[2][axis] < p[min][axis])
			min = 2;

		if (p[1][axis] > p[max][axis])
			max = 1;
		if (p[2][axis] > p[max][axis])
			max = 2;

		if (min == max)
			r[axis] = 0.0f;
		else
		{
			uint32_t mid = 2 - min - max;

			// Found middle vertex.
			Vec3 m_p = p[mid];
			Vec4 m_c = c[mid];

			// Calculate interpolated vertex.
			Vec3 n_p = p[max] - p[min];
			Vec4 n_c = c[max] - c[min];

			n_p.Normalize();
			n_c.Normalize();

			float distance = m_p * n_p;
			float d1 = p[min] * n_p - distance;
			float d2 = p[max] * n_p - distance;
			float dd = d2 - d1;

			float t = - d1 / dd;

			// Found interpolated vertex.
			Vec3 i_p = p[min] + n_p * t;
			Vec4 i_c = c[min] + n_c * t;

			// Calculate delta[coord]/delta[axis] between P_middle & P_interp.
			Vec3 delta_p = i_p - m_p;
			Vec4 delta_c = i_c - m_c;

			float d_p = delta_p[axis];
			float d_c = delta_c[axis];

			if (fabs(d_p) <= 0.0001f)
				r[axis] = 0.0f;
			else
				r[axis] = d_c / d_p;
		}
	}

	return r;
#else
	const uint32_t ci1 = coord;
	const uint32_t ci2 = 1 - coord;

	const Vec3 v1  = (p[1]      -      p[0]) * (c[2][ci2] - c[0][ci2]) - (p[2]      -      p[0]) * (c[1][ci2] - c[0][ci2]);
	float v2       = (c[1][ci1] - c[0][ci1]) * (c[2][ci2] - c[0][ci2]) - (c[2][ci1] - c[0][ci1]) * (c[1][ci2] - c[0][ci2]);

#if 0
	v2 = -v2;
#endif
	//if (fabs(v2) <= 0.0001f)
		//return Vec3(1.0f, 0.0f, 0.0f);

	return v1 / v2;
#endif
}

bool ShapeBuilder::CalculateCoordinateFrameFromUV(Mesh & out_mesh, uint32_t sampler, uint32_t out_sampler)
{
	ResVB & vb = *out_mesh.GetVB();

	uint32_t vertexCount = vb.GetVertexCnt();

	// Calculate normal, tangent & binormal.

	if (!CalculateNormals(out_mesh))
		return false;

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		Vec4 normal = Vec4(vb.normal[i], 0.0f);

		vb.tex[out_sampler + 2][i] = normal;
	}

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		for (uint32_t j = 0; j < 2; ++j)
		{
			vb.tex[out_sampler + j][i][0] = 0.0f;
			vb.tex[out_sampler + j][i][1] = 0.0f;
			vb.tex[out_sampler + j][i][2] = 0.0f;
			vb.tex[out_sampler + j][i][3] = 0.0f;
		}
	}

	VertexIterator j(out_mesh);

	while (j.Next())
	{
		uint32_t index[3];
		index[0] = j.GetVertexIndex(); j.Next();
		index[1] = j.GetVertexIndex(); j.Next();
		index[2] = j.GetVertexIndex();

		Vec3 position[3];
		Vec4 texcoord[3];

		for (uint32_t i = 0; i < 3; ++i)
		{
			position[i] = vb.position[index[i]];
			texcoord[i] = vb.tex[sampler][index[i]];
		}

		Vec3 UV[2];

		for (uint32_t i = 0; i < 2; ++i)
		{
			UV[i] = CalcUV(
				position[0],
				position[1],
				position[2],
				texcoord[0],
				texcoord[1],
				texcoord[2], i);
		}
		
		for (uint32_t i = 0; i < 3; ++i)
		{
			vb.tex[out_sampler + 0][index[i]] += Vec4(UV[0], 0.0f);
			vb.tex[out_sampler + 1][index[i]] += Vec4(UV[1], 0.0f);
		}
	}

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
#if 1
		Mat4x4 m;

		m.MakeIdentity();

		for (uint32_t j = 0; j < 3; ++j)
		{
			vb.tex[out_sampler + j][i].Normalize();

			m(0, j) = vb.tex[out_sampler + j][i][0];
			m(1, j) = vb.tex[out_sampler + j][i][1];
			m(2, j) = vb.tex[out_sampler + j][i][2];
		}

		m = m.CalcInv();

		for (uint32_t j = 0; j < 3; ++j)
		{
			vb.tex[out_sampler + j][i][0] = m(0, j);
			vb.tex[out_sampler + j][i][1] = m(1, j);
			vb.tex[out_sampler + j][i][2] = m(2, j);
		}
#endif

		for (uint32_t j = 0; j < 3; ++j)
			vb.tex[out_sampler + j][i].Normalize();

		/*
		printf("Normal:   %+02.3f %+02.3f %+02.3f.\n", vb.tex[out_sampler + 0][i][0], vb.tex[out_sampler + 0][i][1], vb.tex[out_sampler + 0][i][2]);
		printf("Tangent:  %+02.3f %+02.3f %+02.3f.\n", vb.tex[out_sampler + 1][i][0], vb.tex[out_sampler + 1][i][1], vb.tex[out_sampler + 1][i][2]);
		printf("Binormal: %+02.3f %+02.3f %+02.3f.\n", vb.tex[out_sampler + 2][i][0], vb.tex[out_sampler + 2][i][1], vb.tex[out_sampler + 2][i][2]);
		*/
	}

	return true;
}

bool ShapeBuilder::Colorize(Mesh & out_mesh, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	ResVB & vb = *out_mesh.GetVB();
	uint32_t vertexCount = vb.GetVertexCnt();

	const float rf = r / 255.0f;
	const float gf = g / 255.0f;
	const float bf = b / 255.0f;
	const float af = a / 255.0f;

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		vb.color[i][0] = rf;
		vb.color[i][1] = gf;
		vb.color[i][2] = bf;
		vb.color[i][3] = af;
	}

	return true;
}

bool ShapeBuilder::UpdatePatch(PatchMesh & out_mesh)
{
	return true;
	
	for (uint32_t px = 0; px < out_mesh.m_sx; ++px)
	{
		for (uint32_t py = 0; py < out_mesh.m_sy; ++py)
		{
			Patch & dst = out_mesh.GetPatch(px, py);

			// Right.
			if (px != out_mesh.m_sx - 1)
			{
				Patch & src = out_mesh.GetPatch(px + 1, py);

				for (uint32_t i = 0; i < 3; ++i)
					dst.v[3][i] = src.v[0][i];
			}
			// Bottom.
			if (py != out_mesh.m_sy - 1)
			{
				Patch & src = out_mesh.GetPatch(px, py + 1);

				for (uint32_t i = 0; i < 3; ++i)
					dst.v[i][3] = src.v[i][3];
			}
			// Corner.
			if (px != out_mesh.m_sx - 1 && py != out_mesh.m_sy - 1)
			{
				Patch & src = out_mesh.GetPatch(px + 1, py + 1);

				dst.v[3][3] = src.v[3][3];
			}
		}
	}

	return true;
}
