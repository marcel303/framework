#pragma once

#include <stack>
#include "input.h"
#include "render_gl.h"
#include "BeamTree.h"

#define VIEW_SX 640
#define VIEW_SY 480

static void DrawPlane(const PlaneF& plane, const Vec2F& origin, const Color& color);
static void DrawClipEdge(
	const ClipEdge* clipEdge, 
	const Color& lineColor = Color(1.0f, 1.0f, 1.0f),
	const Color& edgePlaneNormalColor = Color(0.0f, 1.0f, 0.0f),
	const Color& clipPlaneNormalColor = Color(0.0f, 0.0f, 1.0f));
static void DrawClipShape(
	const ClipShape* clipShape,
	const Color& lineColor = Color(1.0f, 1.0f, 1.0f),
	const Color& edgePlaneNormalColor = Color(0.0f, 1.0f, 0.0f),
	const Color& clipPlaneNormalColor = Color(0.0f, 0.0f, 1.0f));
static void DrawBeamTree(
	const BeamTree* beamTree,
	const Color& lineColor = Color(1.0f, 1.0f, 1.0f),
	const Color& edgePlaneNormalColor = Color(0.0f, 1.0f, 0.0f),
	const Color& clipPlaneNormalColor = Color(0.0f, 0.0f, 1.0f));

static void TestClipEdge();
static void TestClipShape();
static void TestBeamTree();

int main(int argc, char* argv[])
{
	gRender = new RenderGL();

	gRender->Init(VIEW_SX, VIEW_SY);

	//

	TestBeamTree();
	//TestClipShape();
	//TestClipEdge();

	return 0;
}

static void DrawPlane(const PlaneF& plane, const Vec2F& origin, const Color& color)
{
	Vec2F point0 = origin;
	Vec2F point1 = origin + plane.m_Normal * 10.0f;

	gRender->Line(
		point0[0],
		point0[1],
		point1[0],
		point1[1],
		color);
}

static void DrawClipEdge(
	const ClipEdge* clipEdge, 
	const Color& lineColor,
	const Color& edgePlaneNormalColor,
	const Color& clipPlaneNormalColor)

{
	if (clipEdge->IsEmpty_get())
		return;

	gRender->Line(
		clipEdge->PointList[0][0],
		clipEdge->PointList[0][1],
		clipEdge->PointList[1][0],
		clipEdge->PointList[1][1],
		lineColor);

	DrawPlane(
		clipEdge->ClipPlane,
		(clipEdge->PointList[0] + clipEdge->PointList[1]) * 0.5f,
		clipPlaneNormalColor);

	DrawPlane(
		clipEdge->EdgePlane,
		clipEdge->PointList[0],
		edgePlaneNormalColor);
}

static void DrawClipShape(
	const ClipShape* clipShape,
	const Color& lineColor,
	const Color& edgePlaneNormalColor,
	const Color& clipPlaneNormalColor)
{
	for (size_t i = 0; i < clipShape->EdgeList.size(); ++i)
	{
		ClipEdge edge = clipShape->EdgeList[i];

		DrawClipEdge(&edge, lineColor, edgePlaneNormalColor, clipPlaneNormalColor);
	}
}

static void DrawBeamTree(
	const BeamTree* beamTree,
	const Color& lineColor,
	const Color& edgePlaneNormalColor,
	const Color& clipPlaneNormalColor)
{
	std::stack<const BeamNode*> stack;
	stack.push(beamTree->Root);

	while (stack.size() > 0)
	{
		const BeamNode* node = stack.top();
		stack.pop();

		DrawClipEdge(
			&node->ClipEdge,
			lineColor,
			edgePlaneNormalColor,
			clipPlaneNormalColor);

		if (true)
		{
			Vec2F p0 = node->ClipEdge.PointList[0] - node->ClipEdge.EdgePlane.m_Normal * 1000.0f;
			Vec2F p1 = node->ClipEdge.PointList[0] + node->ClipEdge.EdgePlane.m_Normal * 1000.0f;
			Vec2F midPoint = (node->ClipEdge.PointList[0] + node->ClipEdge.PointList[1]) * 0.5f;

			ClipEdge temp;

			temp.SetupFromPointList(p0, p1);

			BeamNode* parent = node->Parent;

#if 1
			for (BeamNode* parent = node->Parent; parent && !temp.IsEmpty_get(); parent = parent->Parent)
			{
				ClipEdge front;
				ClipEdge back;
				
				temp.ClipByPlane(parent->ClipEdge.ClipPlane, front, back);

				const float d = parent->ClipEdge.ClipPlane.Distance(midPoint);

				if (d >= 0.0f)
					temp = front;
				else
					temp = back;
			}
#endif

			DrawClipEdge(&temp);
		}

		if (node->ChildFront)
			stack.push(node->ChildFront);
		if (node->ChildBack)
			stack.push(node->ChildBack);
	}
}

static void TestClipEdge()
{
	float clipPlaneDistance = -10.0f;

	while (!gKeyboard.Get(KeyCode_Escape))
	{
		gRender->MakeCurrent();

		gRender->Clear();

		ClipEdge edge;
		edge.SetupFromPointList(Vec2F(0.0f, 0.0f), Vec2F(100.0f, 100.0f));

		DrawClipEdge(&edge);

		ClipEdge edgeFront;
		ClipEdge edgeBack;

		PlaneF clipPlane;
		clipPlane.Set(Vec2F(1.0f, 0.0f), clipPlaneDistance);

		edge.ClipByPlane(clipPlane, edgeFront, edgeBack);

		DrawClipEdge(&edgeFront);
		DrawClipEdge(&edgeBack);

		clipPlaneDistance += 0.1f;

		gRender->Present();
	}

	while (gKeyboard.Get(KeyCode_Escape))
		gRender->Present();
}

static void TestClipShape()
{
	ClipShape shape;

	int edgeCount = 5;

	Vec2F origin(VIEW_SX/2.0f, VIEW_SY/2.0f);
	float radius = 50.0f;

	for (int i = 0; i < edgeCount; ++i)
	{
		const float a0 = (float)M_PI * 2.0f / edgeCount * (i + 0);
		const float a1 = (float)M_PI * 2.0f / edgeCount * (i + 1);
		const Vec2F p0 = origin + Vec2F::FromAngle(a0) * radius;
		const Vec2F p1 = origin + Vec2F::FromAngle(a1) * radius;
		ClipEdge edge;
		edge.SetupFromPointList(p0, p1);
		shape.EdgeList.push_back(edge);
	}

	float clipPlaneDistance = 180.0f;

	while (!gKeyboard.Get(KeyCode_Escape))
	{
		gRender->MakeCurrent();

		gRender->Clear();

		ClipShape shapeFront;
		ClipShape shapeBack;

		Vec2F clipPlaneNormal(0.0f, 1.0f);
		PlaneF clipPlane;
		clipPlane.Set(clipPlaneNormal, clipPlaneDistance);

		shape.ClipByPlane(clipPlane, shapeFront, shapeBack);

#if DEBUG
		printf("clip result: fc=%lu, bc=%lu\n", 
			shapeFront.EdgeList.size(),
			shapeBack.EdgeList.size());
#endif

		DrawClipShape(&shapeFront, Color(1.0f, 1.0f, 1.0f), Color(0.0f, 1.0f, 0.0f), Color(0.0f, 0.0f, 1.0f));
		DrawClipShape(&shapeBack, Color(0.5f, 0.5f, 0.5f), Color(0.0f, 0.5f, 0.0f), Color(0.0f, 0.0f, 0.5f));

		clipPlaneDistance += 0.1f;

		gRender->Present();
	}

	while (gKeyboard.Get(KeyCode_Escape))
		gRender->Present();
}

static ClipShape CreateClipShape_Circle(int edgeIdBegin, Vec2F origin, float radius, int edgeCount, float baseAngle = 0.0f)
{
	ClipShape shape;

	for (int i = 0; i < edgeCount; ++i)
	{
		const float a0 = baseAngle + (float)M_PI * 2.0f / edgeCount * (i + 0);
		const float a1 = baseAngle + (float)M_PI * 2.0f / edgeCount * (i + 1);
		const Vec2F p0 = origin + Vec2F::FromAngle(a0) * radius;
		const Vec2F p1 = origin + Vec2F::FromAngle(a1) * radius;
		ClipEdge edge;
		edge.SetupFromPointList(p0, p1);
		shape.EdgeList.push_back(edge);
	}

	return shape;
}

static float Random(float min, float max)
{
	float t = (rand() % 1000) / 999.0f;

	return min + (max - min) * t;
}

class Bouncer
{
public:
	void Setup(Vec2F min, Vec2F max, float speed)
	{
		Min = min;
		Max = max;

		Position[0] = Random(Min[0], Max[0]);
		Position[1] = Random(Min[1], Max[1]);

		Speed = Vec2F::FromAngle(Random(0.0f, (float)M_PI * 2.0f)) * speed;
	}

	void Update(float dt)
	{
		Position += Speed * dt;

		for (int i = 0; i < 2; ++i)
		{
			if (Position[i] < Min[i])
			{
				Speed[i] = +fabs(Speed[i]);
				Position[i] = Min[i];
			}

			if (Position[i] > Max[i])
			{
				Speed[i] = -fabs(Speed[i]);
				Position[i] = Max[i];
			}
		}
	}

	Vec2F Position;
	Vec2F Speed;
	Vec2F Min;
	Vec2F Max;
};

static void TestBeamTree()
{
	float t = 0.0f;

	//const int bouncerCount = 1;
	//const int bouncerCount = 2;
	//const int bouncerCount = 5;
	const int bouncerCount = 10;
	//const int bouncerCount = 100;

	Bouncer bouncer[bouncerCount];

	for (int i = 0; i < bouncerCount; ++i)
		bouncer[i].Setup(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 10.0f);

	//bouncer[0].Speed.Set(100.0f, 100.0f);
	bouncer[0].Speed.Set(0.0f, 0.0f);

	const int entityCount = 10;

	Bouncer entityList[entityCount];

	for (int i = 0; i < entityCount; ++i)
		entityList[i].Setup(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 100.0f);

	while (!gKeyboard.Get(KeyCode_Escape))
	{
		//const float dt = 0.05f;
		const float dt = 0.005f;

		gRender->MakeCurrent();

		gRender->Clear();

		BeamTree tree;

		float borderSize = 30.0f;
		tree.Init(borderSize, borderSize, VIEW_SX - borderSize * 2.0f, VIEW_SY - borderSize * 2.0f);

		ClipShape shape1 = CreateClipShape_Circle(1000, Vec2F(160.0f, 120.0f), 50.0f, 5);
		ClipShape shape2 = CreateClipShape_Circle(2000, Vec2F(100.0f + t * 10.0f, 200.0f - t * 10.0f), 50.0f, 5);
		ClipShape shape3 = CreateClipShape_Circle(3000, Vec2F(130.0f, 200.0f), 50.0f, 5);

		if (gKeyboard.Get(KeyCode_Right))
		{
			tree.AddClipped(shape1);
		}
		if (gKeyboard.Get(KeyCode_Left))
		{
			tree.AddClipped(shape2);
			tree.AddClipped(shape3);
		}

		bouncer[0].Position[0] = gMouse.X;
		bouncer[0].Position[1] = gMouse.Y;

		for (int i = 0; i < bouncerCount; ++i)
		{
			bouncer[i].Update(dt);
			ClipShape shape = CreateClipShape_Circle(i * 10000, bouncer[i].Position, 50.0f + (bouncerCount - 1 - i) * 10.0f, 5, (bouncer[i].Position[0] + bouncer[i].Position[1]) / 100.0f);
			//ClipShape shape = CreateClipShape_Circle(i * 10000, bouncer[i].Position, 50.0f, 3, (bouncer[i].Position[0] + bouncer[i].Position[1]) / 10.0f);
			tree.AddClipped(shape);
		}

		//tree.Postprocess();

		DrawBeamTree(&tree);

		const float step = 7.0f;

		for (float x = 0.0f; x < VIEW_SX; x += step)
		{
			for (float y = 0.0f; y < VIEW_SY; y += step)
			{
				const Vec2F p(x, y);

				const bool isVisible = tree.IsVisible_Point(p);

				if (isVisible)
					gRender->Point(p[0], p[1], Color(0.0f, 1.0f, 0.0f));
				else
					gRender->Point(p[0], p[1], Color(1.0f, 0.0f, 0.0f));
			}
		}

		for (int i = 0; i < entityCount; ++i)
		{
			entityList[i].Update(0.01f);

			float radius = 5.0f;

			bool isVisible = tree.IsVisible_Circle(entityList[i].Position, radius);

			Color color = isVisible ? Color(0.0f, 1.0f, 1.0f) : Color(0.0f, 0.5f, 0.5f);

			gRender->Circle(
				entityList[i].Position[0],
				entityList[i].Position[1],
				radius,
				color);
		}

		gRender->Present();

		t += dt;
	}

	while (gKeyboard.Get(KeyCode_Escape))
		gRender->Present();
}
