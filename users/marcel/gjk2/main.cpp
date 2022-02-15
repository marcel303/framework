#include "framework.h"
#include <vector>

struct Shape
{
	std::vector<Vec3> points;
	
	Vec3 translation;
};

static Vec3 makeRandomDirectionVector()
{
	for (;;)
	{
		const float x = random(-1.f, +1.f);
		const float y = random(-1.f, +1.f);
		const float z = random(-1.f, +1.f);
		
		const float r = x * x + y * y + z * z;
		
		if (r > 1.f)
			continue;
		
		return Vec3(x, y, z).CalcNormalized();
	}
	
	Assert(false);
}

static void makeShape(Vec3Arg origin, const float radius, const int numPoints, Shape & out_shape)
{
	out_shape.points.resize(numPoints * 2);
	
	for (int i = 0; i < numPoints; ++i)
	{
	// note : we could just add a bunch of random point and the GJK algorithm will still work
	// todo : verify the above observation is true
	
	#if 1
		const float x = cosf(i / float(numPoints) * float(M_PI * 2.0)) * radius;
		const float y = sinf(i / float(numPoints) * float(M_PI * 2.0)) * radius;
		const float z1 = -1.f;
		const float z2 = +1.f;
		
		out_shape.points[i * 2 + 0].Set(x, y, z1);
		out_shape.points[i * 2 + 1].Set(x, y, z2);
	#else
		const Vec3 direction = makeRandomDirectionVector();
		
		out_shape.points[i] = direction * radius;
	#endif
	}
	
	out_shape.translation = origin;
}

static void drawShape(const Shape & shape, const Color & color)
{
	// draw fill
	
	setColor(color);
	beginCubeBatch();
	{
		for (auto & point : shape.points)
			fillCube(shape.translation + point, Vec3(.1f));
	}
	endCubeBatch();
}

static Vec3 makeInitialDirectionVector(const Shape & shape1, const Shape & shape2)
{
	//return shape2.translation - shape1.translation;
	return Vec3(1.f, 0.f, 0.f);
}

static Vec3 findSupportPoint(const Shape & shape, Vec3Arg direction)
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
	
	return shape.translation + shape.points[maxIndex];
}

static Vec3 minkowskiDifferent(const Shape & shape1, const Shape & shape2, Vec3Arg direction)
{
	const Vec3 supportPoint1 = findSupportPoint(shape1,   direction);
	const Vec3 supportPoint2 = findSupportPoint(shape2, - direction);
	
	return supportPoint1 - supportPoint2;
}

static void drawMinkowskiDifferenceEstimate(
	const Shape & shape1,
	const Shape & shape2,
	const Color & color)
{
	static const int kNumPoints = 100;
	static bool isInit = false;
	static Vec3 samplingDirections[kNumPoints];
	
	if (isInit == false)
	{
		isInit = true;
		for (int i = 0; i < kNumPoints; ++i)
			samplingDirections[i] = makeRandomDirectionVector();
	}
	
	setColor(color);
	beginCubeBatch();
	{
		for (int i = 0; i < kNumPoints; ++i)
		{
			const Vec3 direction = samplingDirections[i];
			
			const Vec3 vertex = minkowskiDifferent(shape1, shape2, direction);
			
			fillCube(vertex, Vec3(.05f));
		}
	}
	endCubeBatch();
}

static void drawEdge(
	Vec3Arg vertex1,
	Vec3Arg vertex2,
	Vec3Arg normalVector,
	const float strokeSize)
{
	const Vec3 edgeVector = (vertex2 - vertex1).CalcNormalized() * strokeSize;
	const Vec3 tangentVector = normalVector.CalcNormalized() * strokeSize;
	
	const Vec3 p11 = vertex1 - edgeVector - tangentVector;
	const Vec3 p21 = vertex2 + edgeVector - tangentVector;
	const Vec3 p22 = vertex2 + edgeVector + tangentVector;
	const Vec3 p12 = vertex1 - edgeVector + tangentVector;
	
	gxBegin(GX_QUADS);
	{
		gxVertex3fv(&p11[0]);
		gxVertex3fv(&p21[0]);
		gxVertex3fv(&p22[0]);
		gxVertex3fv(&p12[0]);
	}
	gxEnd();
}

static void drawTriangle(
	Vec3Arg vertex1,
	Vec3Arg vertex2,
	Vec3Arg vertex3,
	const Color & color,
	const bool drawLine,
	const float strokeSize,
	const bool drawFill)
{
	if (drawLine)
	{
		const Vec3 edgeVector1 = vertex3 - vertex1;
		const Vec3 edgeVector2 = vertex3 - vertex2;
		const Vec3 normalVector = (edgeVector1 % edgeVector2);
		
		setColor(color);
		drawEdge(vertex1, vertex2, normalVector, strokeSize);
		drawEdge(vertex2, vertex3, normalVector, strokeSize);
		drawEdge(vertex3, vertex1, normalVector, strokeSize);
	}
	
	if (drawFill)
	{
		setColor(color.mulRGB(.2f));
		gxBegin(GX_TRIANGLES);
		gxVertex3fv(&vertex1[0]);
		gxVertex3fv(&vertex2[0]);
		gxVertex3fv(&vertex3[0]);
		gxEnd();
	}
}

static void drawTetrahedron(
	Vec3Arg vertex1,
	Vec3Arg vertex2,
	Vec3Arg vertex3,
	Vec3Arg vertex4,
	const Color & color,
	const bool drawLine,
	const float strokeSize,
	const bool drawFill)
{
	drawTriangle(
		vertex1,
		vertex2,
		vertex3,
		color,
		drawLine,
		strokeSize,
		drawFill);
	
	drawTriangle(
		vertex2,
		vertex3,
		vertex4,
		color,
		drawLine,
		strokeSize,
		drawFill);
	
	drawTriangle(
		vertex3,
		vertex4,
		vertex1,
		color,
		drawLine,
		strokeSize,
		drawFill);
		
	drawTriangle(
		vertex4,
		vertex1,
		vertex2,
		color,
		drawLine,
		strokeSize,
		drawFill);
}

static bool drawGjk(
	const Shape & shape1,
	const Shape & shape2,
	const int showAtIteration,
	const int stopAtIteration,
	int & out_numIterations)
{
	bool result = false;
	
	bool canIntersect = true;
	
	Vec3 triangleVertex1(false);
	Vec3 triangleVertex2(false);
	Vec3 triangleVertex3(false);
	
	Vec3 searchDirection(false);
	
	// find first vertex using some initial direction vector
	
	if (canIntersect)
	{
		searchDirection = makeInitialDirectionVector(shape1, shape2);
		
		triangleVertex1 = minkowskiDifferent(
			shape1,
			shape2,
			searchDirection);
	}
	
	// find the second vertex in the reverse direction of the initial direction vector
	
	if (canIntersect)
	{
		searchDirection = - triangleVertex1;
		
		triangleVertex2 = minkowskiDifferent(
			shape1,
			shape2,
			searchDirection);
		
		// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
		if (searchDirection * triangleVertex2 < 0.f)
		{
			logDebug("no k-simplex possible w/ triangleVertex2. the shapes cannot be intersecting");
			
			canIntersect = false;
		}
	}

	// find the third vertex by searching in the direction perpendicular
	// to the plane formed by our two vertices and the origin. note that
	// given how we construct the normal, we know the new search direction
	// will be in the direction of the origin
	
	if (canIntersect)
	{
		const Vec3 edgeDirection1 = /* origin */    - triangleVertex1;
		const Vec3 edgeDirection2 = triangleVertex2 - triangleVertex1;
		
		const Vec3 normalDirection =
			edgeDirection1 %
			edgeDirection2;
		
		searchDirection = normalDirection;
		
		/*
		searchDirection =
			normalDirection %
			searchDirection;
		*/
		
	#if defined(DEBUG)
		const float distanceToOrigin1a = triangleVertex1 * normalDirection;
		const float distanceToOrigin2a = triangleVertex2 * normalDirection;
		Assert(distanceToOrigin1a <= +1/1000.f);
		Assert(distanceToOrigin2a <= +1/1000.f);
		/*
		const float distanceToOrigin1b = triangleVertex1 * searchDirection;
		const float distanceToOrigin2b = triangleVertex2 * searchDirection;
		Assert(distanceToOrigin1b <= +1/1000.f);
		Assert(distanceToOrigin2b <= +1/1000.f);
		*/
	#endif
		
		triangleVertex3 = minkowskiDifferent(
			shape1,
			shape2,
			searchDirection);
		
		// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
		if (searchDirection * triangleVertex3 < 0.f)
		{
			logDebug("no k-simplex possible w/ triangleVertex3. the shapes cannot be intersecting");
			
			canIntersect = false;
		}
	}
	
	{
		// set up the next search direction for the simplex loop below
		
		const Vec3 edgeDirection1 = triangleVertex2 - triangleVertex1;
		const Vec3 edgeDirection2 = triangleVertex3 - triangleVertex1;
		
		const Vec3 normalDirection =
			edgeDirection1 %
			edgeDirection2;
			
		searchDirection =
			normalDirection * triangleVertex1 < 0.f
			?   normalDirection
			: - normalDirection;
	}
		
	for (int iteration = 0; iteration < stopAtIteration && canIntersect; ++iteration)
	{
		const Vec3 triangleVertex4 = minkowskiDifferent(
			shape1,
			shape2,
			searchDirection);
		
		/*
		if (triangleVertex4 == triangleVertex1 ||
			triangleVertex4 == triangleVertex2 ||
			triangleVertex4 == triangleVertex3)
		{
			logDebug("search exhausted (fold). the shapes cannot be intersecting");
			
			canIntersect = false;
		}
		*/
		
		if (canIntersect)
		{
			if (showAtIteration == -1 || iteration == showAtIteration)
			{
				// draw the current simplex
				
				const bool drawLine = true;
				const float strokeSize = .02f;
				const bool drawFill = iteration == showAtIteration;
				
				drawTetrahedron(
					triangleVertex1,
					triangleVertex2,
					triangleVertex3,
					triangleVertex4,
					Color::fromHSL(iteration / float(stopAtIteration), .25f, .25f),
					drawLine,
					strokeSize,
					drawFill);
			}
			
			// early out - is it still possible to form a k-simplex ? if not the shapes cannot be intersecting
		
			if (searchDirection * triangleVertex4 < 0.f)
			{
				logDebug("no k-simplex possible w/ triangleVertex4. the shapes cannot be intersecting");
				
				canIntersect = false;
			}
		}
		
		if (canIntersect)
		{
			const Vec3 edgeVector1 = triangleVertex4 - triangleVertex1;
			const Vec3 edgeVector2 = triangleVertex4 - triangleVertex2;
			const Vec3 edgeVector3 = triangleVertex4 - triangleVertex3;

			const Vec3 normalVector1 = edgeVector1 % edgeVector2;
			const Vec3 normalVector2 = edgeVector2 % edgeVector3;
			const Vec3 normalVector3 = edgeVector3 % edgeVector1;
			
			// note : we wouldn't have to negate triangleVertex4 if we just used
			//        the direction away from the origin and flipped the comparisons
			//        below. but I kept it as is for readability

			const Vec3 directionTowardsOrigin = - triangleVertex4;
			
			// note : the vertex we discard is the vertex that didn't participate
			//        in the formation of the place we found the origin lies in
			//        front of (aka the plane that gave us the new search direction)
			//        this vertex is behind the plane instead of on it
			
		// note : all of these tests and assignments could be transformed into
		//        conditional moves, effectively reducing the number of branches
		//        by three. the final case (where the origin is contained inside
		//        the simplex) could be checked last, hopefully without much
		//        of a pipeline stall
		
			if (normalVector1 * directionTowardsOrigin > 0.f)
			{
				Assert(triangleVertex3 * normalVector1 <= +1/1000.f);
				
				triangleVertex3 = triangleVertex4;
				
				searchDirection = normalVector1;
			}
			else if (normalVector2 * directionTowardsOrigin > 0.f)
			{
				Assert(triangleVertex1 * normalVector2 <= +1/1000.f);
				
				triangleVertex1 = triangleVertex4;
				
				searchDirection = normalVector2;
			}
			else if (normalVector3 * directionTowardsOrigin > 0.f)
			{
				Assert(triangleVertex2 * normalVector3 <= +1/1000.f);
				
				triangleVertex2 = triangleVertex4;
				
				searchDirection = normalVector3;
			}
			else
			{
				// intersection 
				
				logDebug("intersecting!");
				
				result = true;
				
				out_numIterations = iteration;
				
				canIntersect = false;
				
				if (showAtIteration == -1)
				{
					drawTetrahedron(
						triangleVertex1,
						triangleVertex2,
						triangleVertex3,
						triangleVertex4,
						Color::fromHSL(iteration / float(stopAtIteration), .25f, .25f),
						true,
						.02f,
						true);
				}
			}
		}
		
		if (canIntersect)
		{
			if (iteration == showAtIteration)
			{
				const Vec3 triangleCenter =
					(
						triangleVertex1 +
						triangleVertex2 +
						triangleVertex3
					) / 3.f;
					
				setColor(Color::fromHSL(.75f, .75f, .75f));
				drawEdge(
					triangleCenter,
					triangleCenter + searchDirection.CalcNormalized() * 2.f,
					triangleCenter % searchDirection,
					.04f);
			}
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
	
	framework.enableDepthBuffer = true;
	framework.enableSound = false;
	
	if (!framework.init(800, 600))
		return -1;
	
	Shape shape1;
	Shape shape2;
		
	makeShape(Vec3(0, 0, 0), 1.f, 5, shape1);
	makeShape(Vec3(1, 0, 0), 1.f, 7, shape2);
	
	Camera3d camera;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
			
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 10.f);
			pushDepthTest(true, DEPTH_LESS);
			camera.pushViewMatrix();
			
			setColor(Color::fromHSL(0.f, 0.f, 1.f));
			drawLine3d(Vec3(-1, 0, 0), Vec3(+1, 0, 0));
			drawLine3d(Vec3(0, -1, 0), Vec3(0, +1, 0));
			drawLine3d(Vec3(0, 0, -1), Vec3(0, 0, +1));
			
			static float t = 0.f;
			if (mouse.isDown(BUTTON_LEFT))
				t += framework.timeStep;
			if (mouse.isDown(BUTTON_RIGHT))
				t -= framework.timeStep;
			shape1.translation.Set(
				0.f + sinf(t / 4.56f) * 1.f,
				0.f + sinf(t / 5.67f) * 1.f,
				0.f + sinf(t / 6.78f) * 1.f);
			shape2.translation.Set(
				2.f + sinf(t / 1.23f) * 1.f,
				0.f + sinf(t / 2.34f) * 1.f,
				0.f + sinf(t / 3.45f) * 1.f);

			drawMinkowskiDifferenceEstimate(
				shape1,
				shape2,
				Color::fromHSL(.5f, .25f, .25f));
			
			drawShape(shape1, Color::fromHSL(.0f, .5f, .5f));
			drawShape(shape2, Color::fromHSL(.1f, .5f, .5f));
			
			const int maxIterations = 20;
			
			static int showAtIteration = -1;
			
			if (keyboard.wentDown(SDLK_q))
				showAtIteration = -1;
			if (keyboard.wentDown(SDLK_a))
				showAtIteration++;
			if (keyboard.wentDown(SDLK_z))
				showAtIteration--;
				
			showAtIteration = clamp<int>(showAtIteration, -1, maxIterations);
			
			int numIterations;
			const bool isIntersecting = drawGjk(shape1, shape2, showAtIteration, maxIterations, numIterations);
			
			camera.popViewMatrix();
			popDepthTest();
			
			projectScreen2d();
			
			if (isIntersecting)
			{
				setColor(colorWhite);
				drawText(
					framework.getCurrentWindow().getWidth()/2.f,
					framework.getCurrentWindow().getHeight()/2.f,
					24, 0, 0, "Intersecting @%d", numIterations);
			}
			
			if (showAtIteration != -1)
			{
				setColor(colorWhite);
				drawText(
					framework.getCurrentWindow().getWidth()/2.f,
					framework.getCurrentWindow().getHeight()/2.f + 30,
					24, 0, 0, "showAtIteration: %d", showAtIteration);
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
