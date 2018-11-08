#include "constants.h"
#include "draw.h"
#include "framework.h"
#include "impulseResponse.h"
#include "lattice.h"

const char * s_vertexColorModeNames[kVertexColorMode_COUNT] =
{
	"velocity",
	"dot(velocity, surface normal)",
	"surface normal"
};

VertexColorMode s_vertexColorMode = kVertexColorMode_VelocityDotN;

//

void colorizeLatticeVertices(const Lattice & lattice, Color * colors)
{
	const int numVertices = kNumVertices;
	
	if (s_vertexColorMode == kVertexColorMode_Velocity)
	{
		for (int i = 0; i < numVertices; ++i)
		{
			const float vx = lattice.vertices_v[i].x;
			const float vy = lattice.vertices_v[i].y;
			const float vz = lattice.vertices_v[i].z;
			
			const float scale = 400.f;
			
			const float r = vx * scale + .5f;
			const float g = vy * scale + .5f;
			const float b = vz * scale + .5f;
			
			colors[i].set(r, g, b, 1.f);
		}
	}
	else if (s_vertexColorMode == kVertexColorMode_VelocityDotN)
	{
		for (int i = 0; i < numVertices; ++i)
		{
			const float vx = lattice.vertices_v[i].x;
			const float vy = lattice.vertices_v[i].y;
			const float vz = lattice.vertices_v[i].z;
			
			const float dot = lattice.vertices_n[i].dot(vx, vy, vz);
			
			const float scale = 400.f;
			
			const float v = dot * scale + .5f;
			
			colors[i].set(v, v, v, 1.f);
		}
	}
	else if (s_vertexColorMode == kVertexColorMode_N)
	{
		for (int i = 0; i < numVertices; ++i)
		{
			const float scale = .5f;
			
			const float r = lattice.vertices_n[i].x * scale + .5f;
			const float g = lattice.vertices_n[i].y * scale + .5f;
			const float b = lattice.vertices_n[i].z * scale + .5f;
			
			colors[i].set(r, g, b, 1.f);
		}
	}
}

void drawLatticeVertices(const Lattice & lattice)
{
	gxBegin(GL_POINTS);
	{
		const int numVertices = kNumVertices;
		
		for (int i = 0; i < numVertices; ++i)
		{
			auto & p = lattice.vertices_p[i];
			
			gxVertex3f(p.x, p.y, p.z);
		}
	}
	gxEnd();
}

static Color colors[kNumVertices];

void drawLatticeEdges(const Lattice & lattice)
{
	const int numVertices = kNumVertices;
	
	colorizeLatticeVertices(lattice, colors);
	
	gxBegin(GL_LINES);
	{
		for (auto & edge : lattice.edgeVertices)
		{
			const int vertex1 = edge.vertex1;
			const int vertex2 = edge.vertex2;

			const auto & p1 = lattice.vertices_p[vertex1];
			const auto & p2 = lattice.vertices_p[vertex2];
			
			setColor(colors[vertex1]);
			gxVertex3f(p1.x, p1.y, p1.z);
			
			setColor(colors[vertex2]);
			gxVertex3f(p2.x, p2.y, p2.z);
		}
	}
	gxEnd();
}

void drawLatticeFaces(const Lattice & lattice, const int faceIndex)
{
	const int numVertices = kNumVertices;
	
	colorizeLatticeVertices(lattice, colors);
	
	gxBegin(GL_TRIANGLES);
	{
		for (int i = 0; i < 6; ++i)
		{
			if (faceIndex != -1 && i != faceIndex)
				continue;
			
			for (int y = 0; y < kGridSize - 1; ++y)
			{
				const int index1 = calcVertexIndex(i, 0, y + 0);
				const int index2 = calcVertexIndex(i, 0, y + 1);
				
				for (int x = 0; x < kGridSize - 1; ++x)
				{
					const int index00 = index1 + x + 0;
					const int index10 = index1 + x + 1;
					const int index01 = index2 + x + 0;
					const int index11 = index2 + x + 1;
					
					const auto & p00 = lattice.vertices_p[index00];
					const auto & p10 = lattice.vertices_p[index10];
					const auto & p01 = lattice.vertices_p[index01];
					const auto & p11 = lattice.vertices_p[index11];
					
					setColor(colors[index00]); gxVertex3f(p00.x, p00.y, p00.z);
					setColor(colors[index10]); gxVertex3f(p10.x, p10.y, p10.z);
					setColor(colors[index11]); gxVertex3f(p11.x, p11.y, p11.z);
					
					setColor(colors[index00]); gxVertex3f(p00.x, p00.y, p00.z);
					setColor(colors[index11]); gxVertex3f(p11.x, p11.y, p11.z);
					setColor(colors[index01]); gxVertex3f(p01.x, p01.y, p01.z);
				}
			}
		}
	}
	gxEnd();
}

void drawImpulseResponseProbes(const ImpulseResponseProbe * probes, const int numProbes, const Lattice & lattice)
{
	glPointSize(10.f);
	gxBegin(GL_POINTS);
	for (int i = 0; i < numProbes; ++i)
	{
		const auto & probe = probes[i];
		
		const auto & vertex_p = lattice.vertices_p[probe.vertexIndex];
		const auto & vertex_n = lattice.vertices_n[probe.vertexIndex];
		
		const float offset = -.01f;
		
		gxVertex3f(
			vertex_p.x + vertex_n.x * offset,
			vertex_p.y + vertex_n.y * offset,
			vertex_p.z + vertex_n.z * offset);
	}
	gxEnd();
	glPointSize(1.f);
}

void drawImpulseResponseGraph(const ImpulseResponseState & state, const float responses[kNumProbeFrequencies], const bool drawFrequencyTable, const float in_maxResponse, const float saturation)
{
	float maxResponse;
	
	if (in_maxResponse < 0.f)
	{
		maxResponse = 0.f;
		for (int i = 0; i < kNumProbeFrequencies; ++i)
			maxResponse = fmax(maxResponse, responses[i]);
	}
	else
	{
		maxResponse = in_maxResponse;
	}
	
	maxResponse = fmax(maxResponse, 1e-6f);

	const float graphSx = 700.f;
	const float graphSy = 120.f;
	
	hqBegin(HQ_LINES);
	{
		// draw graph lines
		
		setColor(Color::fromHSL(.5f, saturation, .5f));
		
		for (int i = 0; i < kNumProbeFrequencies - 1; ++i)
		{
			const float response1 = responses[i + 0];
			const float response2 = responses[i + 1];
			const float strokeSize = 2.f;
			
			hqLine(
				(i + 0) * graphSx / kNumProbeFrequencies, graphSy - response1 * graphSy / maxResponse, strokeSize,
				(i + 1) * graphSx / kNumProbeFrequencies, graphSy - response2 * graphSy / maxResponse, strokeSize);
		}
	}
	hqEnd();
	
	if (drawFrequencyTable)
	{
		for (int i = 0; i < kNumProbeFrequencies; i += 2)
		{
			gxPushMatrix();
			gxTranslatef((i + .5f) * graphSx / kNumProbeFrequencies, graphSy + 4, 0);
			gxRotatef(90, 0, 0, 1);
			setColor(colorWhite);
			drawText(0, 0, 13, +1, +1, "%dHz", int(state.frequency[i]));
			gxPopMatrix();
		}
	}
}

void drawImpulseResponseGraphs(const ImpulseResponseState & state, const float * responses, const int numGraphs, const bool drawFrequencyTable, const float maxResponse, const int highlightIndex)
{
	gxPushMatrix();
	{
		for (int i = 0; i < numGraphs; ++i)
		{
			const float saturation = (i == highlightIndex) ? 1.f : ((i + .5f) / numGraphs);
			
			drawImpulseResponseGraph(
				state,
				responses + i * kNumProbeFrequencies,
				drawFrequencyTable && (i == numGraphs - 1),
				maxResponse,
				saturation);
			
			gxTranslatef(1, 3, 0);
		}
	}
	gxPopMatrix();
}
