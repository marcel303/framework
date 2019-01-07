#include "framework.h"
#include "Mat4x4.h"
#include <vector>

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
	
    std::vector<float> CalcHomographyMatrix() const
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

        return { h11, h12, h13, h21, h22, h23, h31, h32, 1.f };
    }

    std::vector<float> CalcInverseMatrix(std::vector<float> & mat) const
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

        return { o11, o12, o13, o21, o22, o23, o31, o32, o33 };
    }
	
    void tickEditor(const float dt)
    {
    	Vertex * vertices[4] = { &v00, &v01, &v10, &v11 };
		
		Vec2 mousePos(mouse.x, mouse.y);
		
		hoveredVertex = nullptr;
		
		for (auto & v : vertices)
		{
			auto delta = v->viewPosition - mousePos;
			
			if (delta.CalcSize() < 10.f)
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
		{
			draggedVertex->viewPosition[0] += mouse.dx;
			draggedVertex->viewPosition[1] += mouse.dy;
		}
		
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
	
	void drawEditor()
	{
		Vertex * vertices[4] = { &v00, &v10, &v11, &v01 };
		
		setColor(255, 255, 255, 140);
		hqBegin(HQ_LINES);
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
			hqBegin(HQ_FILLED_CIRCLES);
			hqFillCircle(v->viewPosition[0], v->viewPosition[1], radius);
			hqEnd();
			
			if (v == draggedVertex)
				setColorf(1.f, 1.f, 1.f, v->dragAnim);
			else if (v == hoveredVertex)
				setColorf(1.f, 1.f, 0.f, v->hoverAnim);
			else
				setColor(100, 100, 100);
			
			hqBegin(HQ_STROKED_CIRCLES);
			hqStrokeCircle(v->viewPosition[0], v->viewPosition[1], radius, 1.f);
			hqEnd();
		}
	}
};

int main(int argc, char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	Homography h;
	
	h.v00.viewPosition.Set(100, 100);
	h.v10.viewPosition.Set(300, 100);
	h.v01.viewPosition.Set(100, 300);
	h.v11.viewPosition.Set(300, 300);
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		h.tickEditor(dt);
		
		auto m = h.CalcHomographyMatrix();
		
		Mat4x4 m4(true);
		for (int x = 0; x < 3; ++x)
			for (int y = 0; y < 3; ++y)
				m4(x, y) = m[x + y * 3];
		
		framework.beginDraw(0, 0, 0, 0);
		{
			for (int x = 0; x < 10; ++x)
			{
				for (int y = 0; y < 10; ++y)
				{
					const float x1 = (x + 0) / 10.f;
					const float y1 = (y + 0) / 10.f;
					const float x2 = (x + 1) / 10.f;
					const float y2 = (y + 1) / 10.f;
					
					Vec3 p1 = m4.Mul3(Vec3(x1, y1, 1.f));
					Vec3 p2 = m4.Mul3(Vec3(x2, y2, 1.f));
					
					if (p1[2] > 0.f && p2[2] > 0.f)
					{
						p1 /= p1[2];
						p2 /= p2[2];
						
						setColorf(x1, y1, x2);
						drawRect(p1[0], p1[1], p2[0], p2[1]);
					}
				}
			}
			
			setColor(255, 255, 255, 127);
			gxBegin(GL_LINE_STRIP);
			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
			checkErrorGL();
			for (int i = 0; i < 1000; ++i)
			{
				const float angle = i / 1000.f * 2.f * float(M_PI);
				const float x = .5f + cosf(angle) / 3.f;
				const float y = .5f + sinf(angle) / 3.f;
				
				Vec3 p = m4.Mul3(Vec3(x, y, 1.f));
				
				p /= p[2];
				
				gxVertex2f(p[0], p[1]);
			}
			gxEnd();
			
			h.drawEditor();
		}
		framework.endDraw();
	}
	
	return 0;
}
