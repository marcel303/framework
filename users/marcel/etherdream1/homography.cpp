#include "framework.h"
#include "homography.h"

void Homography::calcHomographyMatrix(float * out_mat) const
{
    auto p00 = v00.viewPosition;
    auto p01 = v01.viewPosition;
    auto p10 = v10.viewPosition;
    auto p11 = v11.viewPosition;

    auto x00 = p00[0];
    auto y00 = p00[1];
    auto x01 = p01[0];
    auto y01 = p01[1];
    auto x10 = p10[0];
    auto y10 = p10[1];
    auto x11 = p11[0];
    auto y11 = p11[1];

    auto a = x10 - x11;
    auto b = x01 - x11;
    auto c = x00 - x01 - x10 + x11;
    auto d = y10 - y11;
    auto e = y01 - y11;
    auto f = y00 - y01 - y10 + y11;

    auto h13 = x00;
    auto h23 = y00;
    auto h32 = (c * d - a * f) / (b * d - a * e);
    auto h31 = (c * e - b * f) / (a * e - b * d);
    auto h11 = x10 - x00 + h31 * x10;
    auto h12 = x01 - x00 + h32 * x01;
    auto h21 = y10 - y00 + h31 * y10;
    auto h22 = y01 - y00 + h32 * y01;

	out_mat[0] = h11;
	out_mat[1] = h12;
	out_mat[2] = h13;
	out_mat[3] = h21;
	out_mat[4] = h22;
	out_mat[5] = h23;
	out_mat[6] = h31;
	out_mat[7] = h32;
	out_mat[8] = 1.f;
}

void Homography::calcInverseMatrix(const float * mat, float * out_mat)
{
    auto i11 = mat[0];
    auto i12 = mat[1];
    auto i13 = mat[2];
    auto i21 = mat[3];
    auto i22 = mat[4];
    auto i23 = mat[5];
    auto i31 = mat[6];
    auto i32 = mat[7];
    auto i33 = 1.f;
    auto a = 1.f / (
        + (i11 * i22 * i33)
        + (i12 * i23 * i31)
        + (i13 * i21 * i32)
        - (i13 * i22 * i31)
        - (i12 * i21 * i33)
        - (i11 * i23 * i32)
    );

    auto o11 = ( i22 * i33 - i23 * i32) / a;
    auto o12 = (-i12 * i33 + i13 * i32) / a;
    auto o13 = ( i12 * i23 - i13 * i22) / a;
    auto o21 = (-i21 * i33 + i23 * i31) / a;
    auto o22 = ( i11 * i33 - i13 * i31) / a;
    auto o23 = (-i11 * i23 + i13 * i21) / a;
    auto o31 = ( i21 * i32 - i22 * i31) / a;
    auto o32 = (-i11 * i32 + i12 * i31) / a;
    auto o33 = ( i11 * i22 - i12 * i21) / a;

	out_mat[0] = o11;
	out_mat[1] = o12;
	out_mat[2] = o13;
	out_mat[3] = o21;
	out_mat[4] = o22;
	out_mat[5] = o23;
	out_mat[6] = o31;
	out_mat[7] = o32;
	out_mat[8] = o33;
}

void Homography::tickEditor(Vec2Arg mousePos, const float sensitivity, const float dt, bool & inputIscaptured)
{
	Vertex * vertices[4] = { &v00, &v01, &v10, &v11 };
	
	const float radius = .1f;
	const Vec2 mouseDelta = mousePos - previousMousePos;
	
	if (inputIscaptured)
	{
		cancelEditing();
	}
	else
	{
		hoveredVertex = nullptr;
		
		for (auto & v : vertices)
		{
			auto delta = v->viewPosition - mousePos;
			
			if (delta.CalcSize() < radius)
				hoveredVertex = v;
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (hoveredVertex != nullptr)
			{
				draggedVertex = hoveredVertex;
			}
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			draggedVertex = nullptr;
		}
		
		if (draggedVertex != nullptr)
			inputIscaptured = true;
	}
	
	if (draggedVertex != nullptr)
	{
		draggedVertex->viewPosition += mouseDelta * sensitivity;
	}
	
	previousMousePos = mousePos;
	
	for (auto & v : vertices)
	{
		if (v == draggedVertex)
			v->dragAnim = fminf(v->dragAnim + dt / .2f, 1.f);
		else
			v->dragAnim = fmaxf(v->dragAnim - dt / .3f, 0.f);
		
		if (v == hoveredVertex)
			v->hoverAnim = fminf(v->hoverAnim + dt / .2f, 1.f);
		else
			v->hoverAnim = fmaxf(v->hoverAnim - dt / .3f, 0.f);
	}
}

void Homography::drawEditor() const
{
	const Vertex * vertices[4] = { &v00, &v10, &v11, &v01 };
	
	setColor(255, 255, 255, 140);
	hqBegin(HQ_LINES, true);
	for (int i = 0; i < 4; ++i)
	{
		auto & v1 = vertices[(i + 0) % 4]->viewPosition;
		auto & v2 = vertices[(i + 1) % 4]->viewPosition;
		hqLine(v1[0], v1[1], 1.f, v2[0], v2[1], 1.f);
	}
	hqEnd();
	
	for (auto & v : vertices)
	{
		float opacity = .2f;
		opacity = lerp(opacity, .5f, v->hoverAnim);
		opacity = lerp(opacity, .9f, v->dragAnim);
		
		float radius = 8.f;
		radius = lerp(radius, 12.f, v->hoverAnim);
		radius = lerp(radius, 10.f, v->dragAnim);
		
		setColorf(1.f, 1.f, 1.f, opacity);
		hqBegin(HQ_FILLED_CIRCLES, true);
		hqFillCircle(v->viewPosition[0], v->viewPosition[1], radius);
		hqEnd();
		
		if (v == draggedVertex)
			setColorf(1.f, 1.f, 1.f, v->dragAnim);
		else if (v == hoveredVertex)
			setColorf(1.f, 1.f, 0.f, v->hoverAnim);
		else
			setColor(100, 100, 100);
		
		hqBegin(HQ_STROKED_CIRCLES, true);
		hqStrokeCircle(v->viewPosition[0], v->viewPosition[1], radius, 1.f);
		hqEnd();
	}
}

void Homography::cancelEditing()
{
	hoveredVertex = nullptr;
	draggedVertex = nullptr;
}
