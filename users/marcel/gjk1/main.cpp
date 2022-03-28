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
	Vec2 previousVertex(false);
	
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
// todo : make sure it points toward the origin, so we can get rid of the parity stuff below
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

static bool drawGjk(
	const Shape & shape1,
	const Shape & shape2,
	const int stopAtIteration,
	Vec2 & out_triangleVertex1,
	Vec2 & out_triangleVertex2,
	Vec2 & out_triangleVertex3)
{
	bool result = false;
	
	bool canIntersect = true;
	
	Vec2 triangleVertex1(false);
	Vec2 triangleVertex2(false);
	
	Vec2 searchDirection(false);
	
	if (canIntersect)
	{
		searchDirection = makeInitialDirectionVector();
		
		triangleVertex1 = minkowskiDifferent(shape1, shape2, searchDirection);
	}
	
	if (canIntersect)
	{
		searchDirection = - triangleVertex1;
		
		triangleVertex2 = minkowskiDifferent(shape1, shape2, searchDirection);
		
		// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
		if (searchDirection * triangleVertex2 < 0.f)
		{
			logDebug("no k-simplex possible w/ triangleVertex2. the shapes cannot be intersecting");
			
			canIntersect = false;
		}
	}
	
	{
		// set up the next search direction for the simplex loop below
		
		const Vec2 edgeDirection = triangleVertex2 - triangleVertex1;
		
		const Vec2 normalDirection = makePerpendicularVector(edgeDirection);
			
		searchDirection =
			normalDirection * triangleVertex1 < 0.f
			?   normalDirection
			: - normalDirection;
	}
	
	for (int iteration = 0; iteration < stopAtIteration && canIntersect; ++iteration)
	{
		const Vec2 triangleVertex3 = minkowskiDifferent(shape1, shape2, searchDirection);
		
		/*
		if (triangleVertex3 == triangleVertex1 ||
			triangleVertex3 == triangleVertex2)
		{
			logDebug("search exhausted (fold). the shapes cannot be intersecting");
			
			canIntersect = false;
		}
		*/
		
		if (canIntersect)
		{
			// draw the current triangle
			
			drawTriangle(
				triangleVertex1,
				triangleVertex2,
				triangleVertex3,
				Color::fromHSL(iteration / float(stopAtIteration), .25f, .75f));
			
			// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
			if (searchDirection * triangleVertex3 < 0.f)
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
			const Vec2 edgeVector1 = triangleVertex3 - triangleVertex1;
			const Vec2 edgeVector2 = triangleVertex3 - triangleVertex2;

			Vec2 perpendicularVector1 = makePerpendicularVector(edgeVector1);
			Vec2 perpendicularVector2 = makePerpendicularVector(edgeVector2);
			
			if (perpendicularVector1 * edgeVector2 < 0.f) perpendicularVector1 = -perpendicularVector1;
			if (perpendicularVector2 * edgeVector1 < 0.f) perpendicularVector2 = -perpendicularVector2;

			// note : we wouldn't have to negate triangleVertex3 if we just used
			//        the direction away from the origin and flipped the comparisons
			//        below. but I kept it as is for readability
			
			const Vec2 directionTowardsOrigin = - triangleVertex3;
			
			if (perpendicularVector1 * directionTowardsOrigin > 0.f)
			{
				Assert(triangleVertex2 * perpendicularVector1 <= +1/1000.f);
				
				triangleVertex2 = triangleVertex3;
				
				searchDirection = perpendicularVector1;
			}
			else if (perpendicularVector2 * directionTowardsOrigin > 0.f)
			{
				Assert(triangleVertex1 * perpendicularVector2 <= +1/1000.f);
				
				triangleVertex1 = triangleVertex3;
				
				searchDirection = perpendicularVector2;
			}
			else
			{
				// intersection 
				
				logDebug("intersecting!");
				
				result = true;
				
				canIntersect = false;
				
				out_triangleVertex1 = triangleVertex1;
				out_triangleVertex2 = triangleVertex2;
				out_triangleVertex3 = triangleVertex3;
			}
		}
	}
	
	if (canIntersect)
	{
		logDebug("we have a possible intersection!");
	}
	
	return result;
}

#include "Vec2.h"
#include <queue>

struct Plane
{
	Vec2 v1;
	Vec2 v2;
	
	Vec2 normal;
	float distance;
	
	Plane()
		: v1(false)
		, v2(false)
		, normal(false)
	{
	}
	
	bool operator<(const Plane & other) const
	{
		return distance > other.distance;
	}
};

static Plane makePlane(Vec2Arg v1, Vec2Arg v2)
{
	Plane result;
	
	result.v1 = v1;
	result.v2 = v2;
	
	result.normal = makePerpendicularVector(v2 - v1).CalcNormalized();
	result.distance = result.normal * v1;
	
	Assert(result.distance >= -1.f / 1000.f);
	
	//logDebug("make plane with distance %.03f", result.distance);
	
	return result;
}

static Plane makePlaneWithWinding(Vec2Arg v1, Vec2Arg v2, const float windingParity)
{
	Plane result;
	
	result.v1 = v1;
	result.v2 = v2;
	
	result.normal = makePerpendicularVector(v2 - v1).CalcNormalized();
	result.distance = result.normal * v1;
	
	if (windingParity < 0.f)
	{
		std::swap(result.v1, result.v2);
		
		result.normal = -result.normal;
		result.distance = -result.distance;
	}

	Assert(result.distance >= -1.f / 1000.f);
	
	//logDebug("make plane with distance %.03f", result.distance);
	
	return result;
}

static float computeWindingParity(
	const Vec2 & triangleVertex1,
	const Vec2 & triangleVertex2,
	const Vec2 & triangleVertex3)
{
	const Vec2 edgeVector1 = triangleVertex2 - triangleVertex1;
	const Vec2 edgeVector2 = triangleVertex3 - triangleVertex2;
	
	const float windingParity =
		(
			Vec3(edgeVector1[0], edgeVector1[1], 0.f) %
			Vec3(edgeVector2[0], edgeVector2[1], 0.f)
		)[2];
	
	return windingParity;
}

static void epa(
	const Shape & shape1,
	const Shape & shape2,
	const Vec2 & triangleVertex1,
	const Vec2 & triangleVertex2,
	const Vec2 & triangleVertex3,
	Vec2 & out_normal,
	float & out_distance)
{
	std::priority_queue<Plane> planes;
	
	const float windingParity =
		computeWindingParity(
			triangleVertex1,
			triangleVertex2,
			triangleVertex3);
	
// todo : check if winding parity is 0. if it is this means two or more vertices are equal to each other and the simplex has collapsed in on itself

	planes.push(makePlaneWithWinding(triangleVertex1, triangleVertex2, windingParity));
	planes.push(makePlaneWithWinding(triangleVertex2, triangleVertex3, windingParity));
	planes.push(makePlaneWithWinding(triangleVertex3, triangleVertex1, windingParity));
	
	for (;;)
	{
		auto & plane = planes.top();
		
		logDebug("eval with plane distance: %.03f", plane.distance);
		
		const Vec2 vertex = minkowskiDifferent(shape1, shape2, plane.normal);
		
		const float distance = vertex * plane.normal;
		
		Assert(distance >= -1.f / 1000.f);
		
		if (fabsf(distance - plane.distance) <= 1.f / 1000.f)
		{
			// found it!
			
			out_normal = plane.normal;
			out_distance = distance;
			
			break;
		}
		else
		{
			// construct new planes based on the found point and the (extreme) vertices used to construct the old the plane
			
			auto plane_v1 = plane.v1;
			auto plane_v2 = plane.v2;
			
			planes.pop();
			
			planes.push(makePlane(plane_v1, vertex));
			planes.push(makePlane(vertex, plane_v2));
		}
	}
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableSound = false;
	
	if (!framework.init(800, 600))
		return -1;
	
	Vec2 offset;
	
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
			//{
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
				
				makeShape(mousePosition_world,  1.f, 5, shape1);
				makeShape(Vec2(+1, 0) - offset, 1.f, 7, shape2);
				
				drawMinkowskiDifferenceEstimate(
					shape1,
					shape2,
					Color::fromHSL(.5f, .25f, .25f));
				
				drawShape(shape1, Color::fromHSL(.0f, .5f, .5f));
				drawShape(shape2, Color::fromHSL(.1f, .5f, .5f));
				
				Vec2 triangleVertex1(false);
				Vec2 triangleVertex2(false);
				Vec2 triangleVertex3(false);
				
				const bool isIntersecting = drawGjk(
					shape1,
					shape2,
					10,
					triangleVertex1,
					triangleVertex2,
					triangleVertex3);
				
				if (isIntersecting)
				{
					Vec2 collisionNormal(false);
					float collisionDistance;
				
					epa(
						shape1,
						shape2,
						triangleVertex1,
						triangleVertex2,
						triangleVertex3,
						collisionNormal,
						collisionDistance);
					
					setColor(colorRed);
					drawLine(
						0.f,
						0.f,
						collisionNormal[0] * collisionDistance,
						collisionNormal[1] * collisionDistance);
						
					offset -= collisionNormal * collisionDistance;
				}
			//}
			gxPopMatrix();
			
			if (isIntersecting)
			{
				setColor(colorWhite);
				drawText(0, 100, 24, 0, 0, "Intersecting");
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
