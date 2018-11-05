#include "Debugging.h"
#include "lattice.h"
#include "Mat4x4.h"

extern Mat4x4 s_cubeFaceToWorldMatrices[6];

void Lattice::init()
{
	for (int i = 0; i < 6; ++i)
	{
		const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
		
		const Vec3 zAxis = matrix.GetAxis(2);
		
		Vector n;
		n.set(zAxis[0], zAxis[1], zAxis[2]);
		
		for (int x = 0; x < kTextureSize; ++x)
		{
			for (int y = 0; y < kTextureSize; ++y)
			{
				const int index =
					i * kTextureSize * kTextureSize +
					y * kTextureSize +
					x;
				
				const float xf = ((x + .5f) / float(kTextureSize) - .5f) * 2.f;
				const float yf = ((y + .5f) / float(kTextureSize) - .5f) * 2.f;
	
				Vec3 p = matrix.Mul4(Vec3(xf, yf, 1.f));
				
				vertices[index].p.set(p[0], p[1], p[2]);
				
				vertices[index].f.setZero();
				vertices[index].v.setZero();
				vertices[index].n = n;
			}
		}
	}
	
	// setup up edges for face interiors
	
	auto addEdge = [&](const int faceIndex, const int x1, const int y1, const int x2, const int y2, const float weight)
	{
		const int index1 =
			faceIndex * kTextureSize * kTextureSize +
			y1 * kTextureSize +
			x1;
		
		const int index2 =
			faceIndex * kTextureSize * kTextureSize +
			y2 * kTextureSize +
			x2;
		
		Edge edge;
		edge.vertex1 = index1;
		edge.vertex2 = index2;
		edge.weight = weight;
		
		edges.push_back(edge);
	};
	
	for (int i = 0; i < 6; ++i)
	{
		for (int x = 0; x < kTextureSize; ++x)
		{
			for (int y = 0; y < kTextureSize; ++y)
			{
				if (x + 1 < kTextureSize)
					addEdge(i, x + 0, y + 0, x + 1, y + 0, 1.f);
				if (y + 1 < kTextureSize)
					addEdge(i, x + 0, y + 0, x + 0, y + 1, 1.f);
				
				if (x + 1 < kTextureSize && y + 1 < kTextureSize)
				{
					addEdge(i, x + 0, y + 0, x + 1, y + 1, 1.f / sqrtf(2.f));
					addEdge(i, x + 0, y + 1, x + 1, y + 0, 1.f / sqrtf(2.f));
				}
			}
		}
	}
	
	// setup edges between faces
	
	auto addEdge2 = [&](const int faceIndex1, const int x1, const int y1, const int faceIndex2, const int x2, const int y2, const float weight)
	{
		const int index1 =
			faceIndex1 * kTextureSize * kTextureSize +
			y1 * kTextureSize +
			x1;
		
		const int index2 =
			faceIndex2 * kTextureSize * kTextureSize +
			y2 * kTextureSize +
			x2;
		
		Edge edge;
		edge.vertex1 = index1;
		edge.vertex2 = index2;
		edge.weight = weight;
		
		edges.push_back(edge);
	};
	
	auto processSeam = [&](
		const int faceIndex1,
		const int face1_x1,
		const int face1_y1,
		const int face1_x2,
		const int face1_y2,
		const int faceIndex2,
		const int face2_x1,
		const int face2_y1,
		const int face2_x2,
		const int face2_y2)
	{
		const int face1_stepx = face1_x2 - face1_x1;
		const int face1_stepy = face1_y2 - face1_y1;
		int face1_x = face1_x1 * (kTextureSize - 1);
		int face1_y = face1_y1 * (kTextureSize - 1);
		
		const int face2_stepx = face2_x2 - face2_x1;
		const int face2_stepy = face2_y2 - face2_y1;
		int face2_x = face2_x1 * (kTextureSize - 1);
		int face2_y = face2_y1 * (kTextureSize - 1);
		
		for (int i = 0; i < kTextureSize; ++i)
		{
			Assert(face1_x >= 0 && face1_x < kTextureSize);
			Assert(face1_y >= 0 && face1_y < kTextureSize);
			Assert(face2_x >= 0 && face2_x < kTextureSize);
			Assert(face2_y >= 0 && face2_y < kTextureSize);
			
			addEdge2(faceIndex1, face1_x + face1_stepx * 0, face1_y + face1_stepy * 0, faceIndex2, face2_x + face2_stepx * 0, face2_y + face2_stepy * 0, 1.f);
			
			if (i + 1 < kTextureSize)
			{
				addEdge2(faceIndex1, face1_x + face1_stepx * 0, face1_y + face1_stepy * 0, faceIndex2, face2_x + face2_stepx * 1, face2_y + face2_stepy * 1, 1.f / sqrtf(2.f));
			
				addEdge2(faceIndex1, face1_x + face1_stepx * 1, face1_y + face1_stepy * 1, faceIndex2, face2_x + face2_stepx * 0, face2_y + face2_stepy * 0, 1.f / sqrtf(2.f));
			}
		
			face1_x += face1_stepx;
			face1_y += face1_stepy;
			face2_x += face2_stepx;
			face2_y += face2_stepy;
		}
	};
	
	// signed vertex coordinates for a face in cube face space
	const int face_sx[4] = { -1, +1, +1, -1 };
	const int face_sy[4] = { -1, -1, +1, +1 };
	
	// unsigned vertex coordinates for a face in cube face space
	const int face_ux[4] = { 0, 1, 1, 0 };
	const int face_uy[4] = { 0, 0, 1, 1 };
	
	// transform signed face vertex positions into world space, so we can come vertices and edges of our faces in world space
	struct CubeFace
	{
		Vec3 transformedPosition[4];
	};
	
	CubeFace cubeFaces[6];
	
	for (int i = 0; i < 6; ++i)
	{
		const Mat4x4 & matrix = s_cubeFaceToWorldMatrices[i];
		
		for (int v = 0; v < 4; ++v)
		{
			cubeFaces[i].transformedPosition[v] = matrix.Mul4(Vec3(face_sx[v], face_sy[v], 1.f));
			
		#if 0
			printf("transformed position: (%.2f, %.2f, %.2f)\n",
				cubeFaces[i].transformedPosition[v][0],
				cubeFaces[i].transformedPosition[v][1],
				cubeFaces[i].transformedPosition[v][2]);
		#endif
		}
	}
	
	// loop over all of the faces and all of their edges and try to find a second face which shared the same edge (meaning : sharing two vertices with equal world positions)
	
	int numEdges = 0;
	
	for (int f1 = 0; f1 < 6; ++f1)
	{
		for (int f1_v1 = 0; f1_v1 < 4; ++f1_v1)
		{
			const int f1_v2 = (f1_v1 + 1) % 4;
			
			const Vec3 & f1_p1 = cubeFaces[f1].transformedPosition[f1_v1];
			const Vec3 & f1_p2 = cubeFaces[f1].transformedPosition[f1_v2];
			
			for (int f2 = f1 + 1; f2 < 6; ++f2)
			{
				for (int f2_v1 = 0; f2_v1 < 4; ++f2_v1)
				{
					const int f2_v2 = (f2_v1 + 1) % 4;
					
					const Vec3 & f2_p1 = cubeFaces[f2].transformedPosition[f2_v1];
					const Vec3 & f2_p2 = cubeFaces[f2].transformedPosition[f2_v2];
					
					const bool pass1 = ((f2_p1 - f1_p1).CalcSize() <= .05f && (f2_p2 - f1_p2).CalcSize() <= .05f);
					const bool pass2 = ((f2_p1 - f1_p2).CalcSize() <= .05f && (f2_p2 - f1_p1).CalcSize() <= .05f);
					
					// the cases for pass1 and pass2 are different only in the respect the face space x/y's for the second face are swapped
					
					if (pass1)
					{
						numEdges++;
						
						processSeam(
							f1, face_ux[f1_v1], face_uy[f1_v1], face_ux[f1_v2], face_uy[f1_v2],
							f2, face_ux[f2_v1], face_uy[f2_v1], face_ux[f2_v2], face_uy[f2_v2]);
					}
					else if (pass2)
					{
						numEdges++;
						
						processSeam(
							f1, face_ux[f1_v1], face_uy[f1_v1], face_ux[f1_v2], face_uy[f1_v2],
							f2, face_ux[f2_v2], face_uy[f2_v2], face_ux[f2_v1], face_uy[f2_v1]);
					}
				}
			}
		}
	}
	
	//printf("found %d edges, %d valid edges!\n", numEdges, numValidEdges);
	
	finalize();
}

void Lattice::finalize()
{
	for (auto & vertex : vertices)
	{
		vertex.p_init = vertex.p;
	}
	
	for (auto & edge : edges)
	{
		const auto & p1 = vertices[edge.vertex1].p;
		const auto & p2 = vertices[edge.vertex2].p;
		
		const float dx = p2.x - p1.x;
		const float dy = p2.y - p1.y;
		const float dz = p2.z - p1.z;
		
		const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
		
		edge.initialDistance = distance;
	}
}
