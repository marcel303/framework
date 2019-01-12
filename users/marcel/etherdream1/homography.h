#pragma once

#include "Vec2.h"

struct Homography
{
	struct Vertex
	{
		Vec2 viewPosition;
		
		float hoverAnim = 0.f;
		float dragAnim = 0.f;
	};
	
	Vertex v00;
	Vertex v01;
	Vertex v10;
	Vertex v11;
	
	Vertex * hoveredVertex = nullptr;
	Vertex * draggedVertex = nullptr;
	
	Vec2 previousMousePos;
	
    void calcHomographyMatrix(float * out_mat) const;
    static void calcInverseMatrix(const float * mat, float * out_mat);
	
	void tickEditor(Vec2Arg mousePos, const float sensitivity, const float dt, bool & inputIscaptured);
	void drawEditor() const;
	
	void cancelEditing();
};
