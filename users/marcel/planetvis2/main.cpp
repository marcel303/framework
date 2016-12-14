#include "Calc.h"
#include "framework.h"

#define GFX_SX 1024
#define GFX_SY 1024

#define DO_QUADTREE 1

#if DO_QUADTREE

struct QuadNode;
struct QuadParams;
struct QuadTree;

struct QuadParams
{
	QuadParams()
		: tree(nullptr)
		, viewX(0)
		, viewY(0)
		, viewZ(0)
	{
	}

	QuadTree * tree;

	int viewX;
	int viewY;
	int viewZ;
};

struct QuadNode
{
	QuadNode * children;

	bool isResident;

	QuadNode()
		: children(nullptr)
		, isResident(false)
	{
	}

	~QuadNode()
	{
		delete[] children;
		children = nullptr;
	}

	void subdivide(const int size, const int maxSize)
	{
		Assert(size > maxSize);

		children = new QuadNode[4];

		const int childSize = size / 2;

		if (childSize > maxSize)
		{
			for (int i = 0; i < 4; ++i)
			{
				children[i].subdivide(childSize, maxSize);
			}
		}
	}

	void makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool _isResident);

	void traverse(const int level, const int x, const int y, const int size, const QuadParams & params)
	{
		const bool doTraverse = traverseImpl(x, y, size, params);

		if (doTraverse || children == nullptr)
		{
			if (!isResident)
				makeResident(level, x, y, size, params, true);
		}
		else
		{
			//if (isResident)
				makeResident(level, x, y, size, params, false);
		}

		if (children == nullptr || doTraverse == false)
		{
			const int ds = calculateDistanceSq(params.viewX, params.viewY, params.viewZ, x, y, size);
			const float d = std::sqrt(float(ds));

			setColorf(1.f, 1.f, 1.f, 1.f / (d / 200.f + 1.f));

			drawRectLine(x, y, x + size, y + size);

			drawText(x + size/2, y + size/2, 20, 0, 0, "%d", int(d));
		}
		
		if (children != nullptr && doTraverse)
		{
			const int childSize = size / 2;

			const int x1 = x;
			const int x2 = x + childSize;
			const int y1 = y;
			const int y2 = y + childSize;
			
			children[0].traverse(level + 1, x1, y1, childSize, params);
			children[1].traverse(level + 1, x2, y1, childSize, params);
			children[2].traverse(level + 1, x2, y2, childSize, params);
			children[3].traverse(level + 1, x1, y2, childSize, params);
		}
	}

	void traverseDrawResidency(const int level, const int x, const int y, const int size, const QuadParams & params)
	{
		if (isResident)
		{
			setColor(255, 0, 0, 63);
			drawRect(x, y, x + size, y + size);
		}

		if (children != nullptr)
		{
			const int childSize = size / 2;

			const int x1 = x;
			const int x2 = x + childSize;
			const int y1 = y;
			const int y2 = y + childSize;

			children[0].traverseDrawResidency(level + 1, x1, y1, childSize, params);
			children[1].traverseDrawResidency(level + 1, x2, y1, childSize, params);
			children[2].traverseDrawResidency(level + 1, x2, y2, childSize, params);
			children[3].traverseDrawResidency(level + 1, x1, y2, childSize, params);
		}
	}

	//

	static int calculateDistanceSq(const int viewX, const int viewY, const int viewZ, const int x, const int y, const int size)
	{
		const int x1 = x;
		const int x2 = x + size;
		const int y1 = y;
		const int y2 = y + size;

		const int dx = viewX < x1 ? x1 - viewX : viewX > x2 ? viewX - x2 : 0;
		const int dy = viewY < y1 ? y1 - viewY : viewY > y2 ? viewY - y2 : 0;
		const int dz = viewZ;

		const int ds = dx * dx + dy * dy + dz * dz;

		return ds;
	}

	bool traverseImpl(const int x, const int y, const int size, const QuadParams & params)
	{
		const int ds = calculateDistanceSq(params.viewX, params.viewY, params.viewZ, x, y, size);

		//if (ds * 4 < size * size)
		if (ds < size * size)
			return true;
		else
			return false;
	}
};

struct QuadTree
{
	GLuint texture;

	QuadNode root;

	int initSize;
	int pageSize;

	int numLevels;
	int lastLevel;
	int numAllocatedLevels;

	QuadTree()
		: texture(0)
		, root()
		, initSize(0)
		, pageSize(0)
		, numLevels(0)
		, lastLevel(0)
		, numAllocatedLevels(0)
	{
	}

	void init(const int size)
	{
		initSize = size;

		// create sparse texture

		glGenTextures(1, &texture);
		checkErrorGL();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		checkErrorGL();

		GLint pageSx = 0;
		GLint pageSy = 0;
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSx);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSy);
		checkErrorGL();

		//

		pageSize = std::max(pageSx, pageSy);

		lastLevel = 0;

		while (initSize / (1 << lastLevel) > pageSize)
		{
			lastLevel++;
		}

		numLevels = 1;

		while ((1 << (numLevels - 1)) < initSize)
		{
			numLevels++;
		}

		numAllocatedLevels = lastLevel + 1;

		Assert((1 << (numLevels - 1)) == initSize);
		Assert((1 << lastLevel) == (initSize / pageSize));
		Assert((1 << (numLevels - numAllocatedLevels)) == pageSize);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numAllocatedLevels - 1);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		checkErrorGL();

		glTexStorage2D(GL_TEXTURE_2D, numAllocatedLevels, GL_RGBA8, initSize, initSize);
		checkErrorGL();

		//

		root.subdivide(size, pageSize);
	}
};

void QuadNode::makeResident(const int level, const int x, const int y, const int size, const QuadParams & params, const bool _isResident)
{
	isResident = _isResident;

	const int levelSize = params.tree->pageSize << level;
	const int mipIndex = params.tree->numAllocatedLevels - level - 1;
	const int scale = params.tree->initSize / levelSize;

	const int px = x / scale;
	const int py = y / scale;
	const int sx = params.tree->pageSize;
	const int sy = params.tree->pageSize;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, params.tree->texture);
	checkErrorGL();

	glTexPageCommitmentARB(
		GL_TEXTURE_2D,
		mipIndex,
		px, py, 0,
		sx, sy, 1,
		isResident);
	checkErrorGL();

	if (isResident)
	{
		uint8_t * pixels = new uint8_t[sx * sy * 4];
		uint8_t * pixelPtr = pixels;

		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				*pixelPtr++ = x >> 0;
				*pixelPtr++ = y >> 1;
				*pixelPtr++ = (x + y) >> 2;
				*pixelPtr++ = 255;
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D,
			mipIndex,
			px, py,
			sx, sy,
			GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);
		checkErrorGL();

		delete[] pixels;
		pixels = nullptr;
	}

	if (children != nullptr && isResident == false)
	{
		const int childSize = size / 2;

		const int x1 = x;
		const int x2 = x + childSize;
		const int y1 = y;
		const int y2 = y + childSize;

		if (children[0].isResident)
			children[0].makeResident(level + 1, x1, y1, childSize, params, false);
		if (children[1].isResident)
			children[1].makeResident(level + 1, x2, y1, childSize, params, false);
		if (children[2].isResident)
			children[2].makeResident(level + 1, x2, y2, childSize, params, false);
		if (children[3].isResident)
			children[3].makeResident(level + 1, x1, y2, childSize, params, false);
	}
}

struct CubeSide
{
	QuadTree quadTree;

	int planeEquation[4];
	int transform[3][3];

	void init(const int size, const int axis[3][3])
	{
		quadTree.init(size);

		planeEquation[0] = axis[2][0];
		planeEquation[1] = axis[2][1];
		planeEquation[2] = axis[2][2];
		planeEquation[3] = - size / 2;

		for (int a = 0; a < 3; ++a)
			for (int e = 0; e < 3; ++e)
				transform[a][e] = axis[a][e];
	}

	void eval(const QuadParams & params)
	{
		static int i = 0;
		static int d = 0;
		if (i == 0 && keyboard.wentDown(SDLK_SPACE))
			d = (d + 1) % 6;

		int t[3];

		gxPushMatrix();
		if (i == d)
		{
			const int cubeSize = quadTree.initSize;
			gxTranslatef(-cubeSize/2, -cubeSize/2, -cubeSize/2);

			setColor(colorWhite);
			gxBegin(GL_LINES);
			{
				const int cubeSize = quadTree.initSize;
				int v1[3];
				int v2[3];
				doTransformTranspose(cubeSize/2 + 0, cubeSize/2 + 0, cubeSize/2 +   0, v1);
				doTransformTranspose(cubeSize/2 + 0, cubeSize/2 + 0, cubeSize/2 + 512, v2);
				gxVertex3f(v1[0], v1[1], v1[2]);
				gxVertex3f(v2[0], v2[1], v2[2]);
			}
			gxEnd();
		}
		gxPopMatrix();

		doTransform(params.viewX, params.viewY, params.viewZ, t);

		QuadParams localParams = params;
		localParams.tree = &quadTree;
		localParams.viewX = t[0];
		localParams.viewY = t[1];
		localParams.viewZ = t[2];

		gxPushMatrix();
		{
			Mat4x4 matT(true);

			for (int a = 0; a < 3; ++a)
			{
				for (int e = 0; e < 3; ++e)
				{
					matT(a, e) = transform[a][e];
				}

				matT(3, a) += planeEquation[a] * planeEquation[3];
			}

			const Mat4x4 mat = matT.Translate(-quadTree.initSize/2, -quadTree.initSize/2, 0);
			//const Mat4x4 mat = matT;

			//gxTranslatef(-2048, -2048, 0);
			gxMultMatrixf(mat.m_v);

			setColor(colorWhite);
			drawRectLine(0, 0, quadTree.initSize, quadTree.initSize);

#if 1
			if (i == d)
			{
				setColor(255, 255, 255, 127);
				gxBegin(GL_POINTS);
				{
					for (int x = 0; x < quadTree.initSize; x += 128)
					{
						for (int y = 0; y < quadTree.initSize; y += 128)
						{
							gxVertex2f(x, y);
						}
					}
				}
				gxEnd();
			}
#endif
			i = (i + 1) % 6;

			quadTree.root.traverse(0, 0, 0, quadTree.initSize, localParams);
		}
		gxPopMatrix();
	}

	void doTransform(const int x, const int y, const int z, int r[3])
	{
		const int cubeSize2 = quadTree.initSize / 2;

		const int v[3] = { x - cubeSize2, y - cubeSize2, z - cubeSize2 };

		for (int a = 0; a < 3; ++a)
		{
			r[a] =
				v[0] * transform[a][0] +
				v[1] * transform[a][1] +
				v[2] * transform[a][2] +
				cubeSize2;
		}
	}

	void doTransformTranspose(const int x, const int y, const int z, int r[3])
	{
		const int cubeSize2 = quadTree.initSize / 2;

		const int v[3] = { x - cubeSize2, y - cubeSize2, z - cubeSize2 };

		for (int a = 0; a < 3; ++a)
		{
			r[a] =
				v[0] * transform[0][a] +
				v[1] * transform[1][a] +
				v[2] * transform[2][a] +
				cubeSize2;
		}
	}

	int calcDistance(const int x, const int y, const int z)
	{
		return
			planeEquation[0] * x +
			planeEquation[1] * y +
			planeEquation[2] * z +
			planeEquation[3];
	}
};

struct Cube
{
	CubeSide sides[6];

	void init(const int size)
	{
		for (int s = 0; s < 6; ++s)
		{
			Mat4x4 m;
	
			if (s == 0) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(+1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f));
			if (s == 1) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, -1.f));
			if (s == 2) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, +1.f, 0.f), Vec3(1.f, 0.f, 0.f));
			if (s == 3) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, -1.f, 0.f), Vec3(1.f, 0.f, 0.f));
			if (s == 4) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, +1.f), Vec3(0.f, 1.f, 0.f));
			if (s == 5) m.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, -1.f), Vec3(0.f, 1.f, 0.f));

			//

			int axis[3][3];

			for (int x = 0; x < 3; ++x)
			{
				for (int y = 0; y < 3; ++y)
				{
					axis[x][y] =
						m(x, y) < -.5f ? -1 :
						m(x, y) > +.5f ? +1 :
						0;
				}
			}

			//for (int a = 0; a < 3; ++a)
			//{
			//	logDebug("axis[%d]: %+d, %+d, %+d", a, axis[a][0], axis[a][1], axis[a][2]);
			//}

			sides[s].init(size, axis);
		}
	}

	void eval(const QuadParams & params)
	{
		for (int i = 0; i < 6; ++i)
		{
			sides[i].eval(params);
		}
	}
};

#endif

int main(int argc, char * argv[])
{
	changeDirectory("data");

	framework.enableDepthBuffer = true;

	framework.enableRealTimeEditing = true;

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
	#if DO_QUADTREE
		Cube * cube = new Cube();

		cube->init(8 * 1024);
	#endif

		//

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			framework.beginDraw(0, 0, 0, 0);
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				setBlend(BLEND_OPAQUE);

			#if DO_QUADTREE
				Mat4x4 matP;
				Mat4x4 matV;

				const int cubeSize = cube->sides[0].quadTree.initSize;

				matP.MakePerspectiveLH(Calc::DegToRad(90.f), GFX_SY / float(GFX_SX), 1.f, 100000.f);

#if 1
				matV = Mat4x4(true)
					.Translate(0, 0, 6000)
					.RotateY(std::sin(framework.time * .1f) * .1f);
#else
				matV = Mat4x4(true)
					.Translate(0, 0, 6000)
					.RotateX(framework.time * .1f)
					.RotateY(framework.time * .1f)
					.RotateZ(framework.time * .1f);
#endif

				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				gxLoadMatrixf(matP.m_v);
				gxMatrixMode(GL_MODELVIEW);
				gxPushMatrix();
				gxLoadMatrixf(matV.m_v);
				{
					glDisable(GL_DEPTH_TEST);
					//setBlend(BLEND_ALPHA);
					setBlend(BLEND_ADD);

					QuadParams params;
					params.tree = nullptr;
					params.viewX = +(mouse.x / float(GFX_SX) - .5f) * 2.f * cubeSize + cubeSize/2;
					params.viewY = -(mouse.y / float(GFX_SY) - .5f) * 2.f * cubeSize + cubeSize/2;
					params.viewZ = (std::cos(framework.time * .1f) + 1.f) / 2.f * cubeSize;

					setFont("calibri.ttf");

					cube->eval(params);

					gxPushMatrix();
					{
						gxTranslatef(-cubeSize/2, -cubeSize/2, -cubeSize/2);

						glPointSize(10.f);
						gxBegin(GL_POINTS);
						{
							gxColor4f(1.f, 1.f, 1.f, 1.f);
							gxVertex3f(params.viewX, params.viewY, params.viewZ);
						}
						gxEnd();
						glPointSize(1.f);
					}
					gxPopMatrix();

					setBlend(BLEND_OPAQUE);
					glEnable(GL_DEPTH_TEST);
				}
				gxMatrixMode(GL_PROJECTION);
				gxPopMatrix();
				gxMatrixMode(GL_MODELVIEW);
				gxPopMatrix();
			#endif
			}
			framework.endDraw();
		}

	#if DO_QUADTREE
		delete cube;
		cube = nullptr;
	#endif

		framework.shutdown();
	}

	return 0;
}
