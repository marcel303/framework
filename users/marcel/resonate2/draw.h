#pragma once

class Color;
struct ImpulseResponseProbe;
struct ImpulseResponseState;
struct Lattice;

enum VertexColorMode
{
	kVertexColorMode_Velocity,
	kVertexColorMode_VelocityDotN,
	kVertexColorMode_N,
	kVertexColorMode_COUNT
};

extern const char * s_vertexColorModeNames[kVertexColorMode_COUNT];

extern VertexColorMode s_vertexColorMode;

void colorizeLatticeVertices(const Lattice & lattice, Color * colors);
void drawLatticeVertices(const Lattice & lattice);
void drawLatticeEdges(const Lattice & lattice);
void drawLatticeFaces(const Lattice & lattice);

void drawImpulseResponseProbes(const ImpulseResponseProbe * probes, const int numProbes, const Lattice & lattice);
void drawImpulseResponseGraph(const ImpulseResponseState & state, const float responses[kNumProbeFrequencies], const bool drawFrequencyTable, const float in_maxResponse = -1.f, const float saturation = .5f);
void drawImpulseResponseGraphs(const ImpulseResponseState & state, const float * responses, const int numGraphs, const bool drawFrequencyTable, const float maxResponse = -1.f, const int highlightIndex = -1);
