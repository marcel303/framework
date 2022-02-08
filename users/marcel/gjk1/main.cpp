#include "framework.h"
#include <vector>

struct Shape
{
	std::vector<Vec2> points;
};

static void makeShape(Vec2Arg origin, const float radius, const int numPoints, Shape & out_shape)
{
	out_shape.points.resize(numPoints);
	
	for (int i = 0; i < numPoints; ++i)
	{
		const float angle = i / float(numPoints) * float(M_PI * 2.0);
		
		const float x = origin[0] + cosf(angle) * radius;
		const float y = origin[1] + sinf(angle) * radius;
		
		out_shape.points[i].Set(x, y);
	}
}

static void drawShape(const Shape & shape, const Color & color)
{
	// draw fill
	
	setColor(color.mulA(.5));
	gxBegin(GX_TRIANGLE_FAN);
	{
		for (auto & point : shape.points)
			gxVertex2f(point[0], point[1]);
	}
	gxEnd();
	
	// draw line
	
	setColor(color);
	gxBegin(GX_LINE_LOOP);
	{
		for (auto & point : shape.points)
			gxVertex2f(point[0], point[1]);
	}
	gxEnd();
}

static Vec2 makeDirectionVectorFromAngle(const float angle)
{
	return Vec2(cosf(angle), sinf(angle));
}

static Vec2 makeInitialDirectionVector()
{
	//const float angle = random<float>(0.f, float(M_PI * 2.0));
	const float angle = 0.f;
	
	return makeDirectionVectorFromAngle(angle);
}

static const Vec2 & findSupportPoint(const Shape & shape, Vec2Arg direction)
{
	float maxDistanceSq = shape.points[0] * direction;
	size_t maxIndex = 0;
	
	for (size_t i = 1; i < shape.points.size(); ++i)
	{
		const float distanceSq = shape.points[i] * direction;
		
		if (distanceSq > maxDistanceSq)
		{
			maxDistanceSq = distanceSq;
			maxIndex = i;
		}
	}
	
	return shape.points[maxIndex];
}

static Vec2 minkowskiDifferent(const Shape & shape1, const Shape & shape2, Vec2Arg direction)
{
	const Vec2 & supportPoint1 = findSupportPoint(shape1,   direction);
	const Vec2 & supportPoint2 = findSupportPoint(shape2, - direction);
	
	return supportPoint1 - supportPoint2;
}

static void drawMinkowskiDifferenceEstimate(
	const Shape & shape1,
	const Shape & shape2,
	const Color & color)
{
	Vec2 previousVertex;
	
	setColor(color);
	gxBegin(GX_LINE_LOOP);
	{
		for (int i = 0; i < 1000; ++i)
		{
			const float angle = i / 1000.f * float(M_PI * 2.0);
			
			const Vec2 direction = makeDirectionVectorFromAngle(angle);
			
			const Vec2 vertex = minkowskiDifferent(shape1, shape2, direction);
			
			const bool hasPreviousVertex = (i != 0);
			
			if (hasPreviousVertex == false ||
				vertex != previousVertex)
			{
				previousVertex = vertex;
				
				gxVertex2f(vertex[0], vertex[1]);
			}
		}
	}
	gxEnd();
}

static void drawTriangle(
	Vec2Arg vertex1,
	Vec2Arg vertex2,
	Vec2Arg vertex3,
	const Color & color)
{
	setColor(color);
	gxBegin(GX_LINE_LOOP);
	{
		gxVertex2f(vertex1[0], vertex1[1]);
		gxVertex2f(vertex2[0], vertex2[1]);
		gxVertex2f(vertex3[0], vertex3[1]);
	}
	gxEnd();
}

static Vec2 makePerpendicularVector(Vec2Arg vector)
{
	return Vec2(
		+ vector[1],
		- vector[0]);
}

static bool isPointInTriangle(
	Vec2Arg triangleVertex1,
	Vec2Arg triangleVertex2,
	Vec2Arg triangleVertex3,
	Vec2Arg point)
{
	auto sign = [](Vec2Arg p1, Vec2Arg p2, Vec2Arg p3) -> float
	{
		return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
	};

    const float d1 = sign(point, triangleVertex1, triangleVertex2);
    const float d2 = sign(point, triangleVertex2, triangleVertex3);
    const float d3 = sign(point, triangleVertex3, triangleVertex1);

    const bool has_neg = (d1 < 0.f) || (d2 < 0.f) || (d3 < 0.f);
    const bool has_pos = (d1 > 0.f) || (d2 > 0.f) || (d3 > 0.f);

    return !(has_neg && has_pos);
}

static bool drawGjk(const Shape & shape1, const Shape & shape2, const int stopAtIteration)
{
	bool result = false;
	
	bool canIntersect = true;
	
	Vec2 triangleVertex1;
	Vec2 triangleVertex2;
	
	if (canIntersect)
	{
		const Vec2 initialDirection = makeInitialDirectionVector();
		
		triangleVertex1 = minkowskiDifferent(shape1, shape2, initialDirection);
	}
	
	if (canIntersect)
	{
		const Vec2 directionTowardsOrigin = - triangleVertex1;
		
		triangleVertex2 = minkowskiDifferent(shape1, shape2, directionTowardsOrigin);
		
		// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
		if (directionTowardsOrigin * triangleVertex2 < 0.f)
		{
			logDebug("no k-simplex possible w/ triangleVertex2. the shapes cannot be intersecting");
			
			canIntersect = false;
		}
	}
	
	for (int iteration = 0; iteration < stopAtIteration && canIntersect; ++iteration)
	{
		const Vec2 edgeDirection = triangleVertex2 - triangleVertex1;
		
		const Vec2 edgeNormal = makePerpendicularVector(edgeDirection);
		
		const float edgeNormalParity =
			triangleVertex1 *
			edgeNormal;
			
		const Vec2 directionTowardsOrigin =
			edgeNormalParity < 0.f
			?   edgeNormal
			: - edgeNormal;
		
		const Vec2 triangleVertex3 = minkowskiDifferent(shape1, shape2, directionTowardsOrigin);
		
		if (triangleVertex3 == triangleVertex1 ||
			triangleVertex3 == triangleVertex2)
		{
			logDebug("search exhausted (fold). the shapes cannot be intersecting");
			
			canIntersect = false;
		}
		
		if (canIntersect)
		{
			// draw the current triangle
			
			drawTriangle(
				triangleVertex1,
				triangleVertex2,
				triangleVertex3,
				Color::fromHSL(iteration / float(stopAtIteration), .25f, .75f));
			
			// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
			if (directionTowardsOrigin * triangleVertex3 < 0.f)
			{
				logDebug("no k-simplex possible w/ triangleVertex3. the shapes cannot be intersecting");
				
				canIntersect = false;
			}
		}
		
	#if 0
		if (canIntersect)
		{
			if (isPointInTriangle(
				triangleVertex1,
				triangleVertex2,
				triangleVertex3,
				Vec2()))
			{
				logDebug("intersecting!");
				
				result = true;
				
				canIntersect = false;
			}
		}
	#endif
		
		if (canIntersect)
		{
		#if 1
			const Vec2 edgeVector1 = triangleVertex1 - triangleVertex3;
			const Vec2 edgeVector2 = triangleVertex2 - triangleVertex3;

			Vec2 perpendicularVector1 = makePerpendicularVector(edgeVector1);
			Vec2 perpendicularVector2 = makePerpendicularVector(edgeVector2);
			
			if (perpendicularVector1 * edgeVector2 > 0.f) perpendicularVector1 = -perpendicularVector1;
			if (perpendicularVector2 * edgeVector1 > 0.f) perpendicularVector2 = -perpendicularVector2;

			const Vec2 directionTowardsOrigin = - triangleVertex3;
			
			if (perpendicularVector1 * directionTowardsOrigin > 0.f)
			{
				triangleVertex2 = triangleVertex3;
			}
			else if (perpendicularVector2 * directionTowardsOrigin > 0.f)
			{
				triangleVertex1 = triangleVertex3;
			}
			else
			{
				// intersection 
				
				logDebug("intersecting!");
				
				result = true;
				
				canIntersect = false;
			}
		#else
			const Vec2 perpendicularVector1 = makePerpendicularVector(triangleVertex3 - triangleVertex1);
			const Vec2 perpendicularVector2 = makePerpendicularVector(triangleVertex3 - triangleVertex2);
			
			const Vec2 edgeNormal1 = perpendicularVector1.CalcNormalized();
			const Vec2 edgeNormal2 = perpendicularVector2.CalcNormalized();
			
			const float edgeDistanceToNormal1 = fabsf(edgeNormal1 * triangleVertex3);
			const float edgeDistanceToNormal2 = fabsf(edgeNormal2 * triangleVertex3);
			
			if (edgeDistanceToNormal1 <
				edgeDistanceToNormal2)
			{
				triangleVertex2 = triangleVertex3;
			}
			else
			{
				triangleVertex1 = triangleVertex3;
			}
		#endif
		}
	}
	
	if (canIntersect)
	{
		logDebug("we have a possible intersection!");
	}
	
	return result;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxTranslatef(
				framework.getCurrentWindow().getWidth()  / 2.f,
				framework.getCurrentWindow().getHeight() / 2.f,
				0);
			gxPushMatrix();
			gxScalef(100, 100, 1);
			
			setColor(Color::fromHSL(0.f, 0.f, .25f));
			drawLine(-1, 0, +1, 0);
			drawLine(0, -1, 0, +1);
			
			Shape shape1;
			Shape shape2;
			
			Mat4x4 worldToView;
			gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
			const Mat4x4 viewToWorld = worldToView.CalcInv();
			
			const Vec2 mousePosition_view(mouse.x, mouse.y);
			const Vec2 mousePosition_world = viewToWorld.Mul4(mousePosition_view);
			
			makeShape(Vec2(mousePosition_world[0], mousePosition_world[1]), 1.f, 5, shape1);
			makeShape(Vec2(                    +1,                      0), 1.f, 7, shape2);
			
			drawMinkowskiDifferenceEstimate(
				shape1,
				shape2,
				Color::fromHSL(.5f, .25f, .25f));
			
			drawShape(shape1, Color::fromHSL(.0f, .5f, .5f));
			drawShape(shape2, Color::fromHSL(.1f, .5f, .5f));
			
			const bool isIntersecting = drawGjk(shape1, shape2, 10);
			
			gxPopMatrix();
			
			if (isIntersecting)
			{
				drawText(0, 100, 24, 0, 0, "Intersecting");
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
