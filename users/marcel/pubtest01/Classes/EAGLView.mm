#import "EAGLView.h"
#import "ES1Renderer.h"
#import "iPhoneAPI.h"
#import "Quaternion.h"
#import "ResIO.h"
#import "ShapeBuilder.h"
#import "TexturePVR.h"
#import "TextureRGBA.h"

#define DEG2RAD 0.017453292519943f
#define RAD2DEG 57.295779513082325f

#if 0
static void* Allocate(size_t size)
{
	void* p = malloc(size);
	
	memset(p, 0xAB, size);
	
	return p;
}

static void Free(void* p)
{
	free(p);
}

void* operator new(size_t size)
{
	return Allocate(size);
}

void* operator new[](size_t size)
{
	return Allocate(size);
}

void operator delete(void* p)
{
	Free(p);
}

void operator delete[](void* p)
{
	Free(p);
}
#endif

static float gTime = 0.0f;

static void SetLight(int index, float r, float g, float b, float x, float y, float z, float w)
{
	const GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const GLfloat diffuse[] = { r, g, b, 1.0f };
	const GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const GLfloat position[] = { x, y, z, w };
	
	glLightfv(GL_LIGHT0 + index, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0 + index, GL_POSITION, position);
	GLERROR();
	
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
//	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10.0f);
	GLERROR();
}

static void RenderMesh(Mesh* mesh)
{
	glShadeModel(GL_SMOOTH);
	glDisable(GL_NORMALIZE);
	
	//
	
	if (mesh->GetVertexBuffer().GetFVF() & VertexBuffer::FVF_XYZ)
	{
	    glVertexPointer(3, GL_FLOAT, 0, mesh->GetVertexBuffer().position3);
		GLERROR();
		glEnableClientState(GL_VERTEX_ARRAY);
		GLERROR();
	}
	
	if (mesh->GetVertexBuffer().GetFVF() & VertexBuffer::FVF_NORMAL)
	{
		glNormalPointer(GL_FLOAT, 0, mesh->GetVertexBuffer().normal);
		GLERROR();
	    glEnableClientState(GL_NORMAL_ARRAY);
		GLERROR();
	}
	
	if (mesh->GetVertexBuffer().GetFVF() & VertexBuffer::FVF_COLOR)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, mesh->GetVertexBuffer().color);
		GLERROR();
	    glEnableClientState(GL_COLOR_ARRAY);
		GLERROR();
	}
	
	for (size_t i = 0; i < mesh->GetVertexBuffer().GetTexcoordCount(); ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		GLERROR();
		glTexCoordPointer(2, GL_FLOAT, sizeof(VB_Texcoord4), mesh->GetVertexBuffer().tex[0]);
		GLERROR();
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		GLERROR();
	}
	
	if (mesh->IsIndexed())
	{
		glDrawElements(GL_TRIANGLES, mesh->GetIndexBuffer().GetIndexCount(), GL_UNSIGNED_SHORT, mesh->GetIndexBuffer().index);
		GLERROR();
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, mesh->GetPrimitiveCount() * 3);
	}
	
	for (size_t i = 0; i < mesh->GetVertexBuffer().GetTexcoordCount(); ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		GLERROR();
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		GLERROR();
	}
	
    glDisableClientState(GL_NORMAL_ARRAY);
	GLERROR();
    glDisableClientState(GL_VERTEX_ARRAY);
	GLERROR();
}

static void RenderRect(Vec2F pos, Vec2F size)
{
	float p[] =
	{
		pos[0], pos[1], 0.0f,
		pos[0] + size[0], pos[1], 0.0f,
		pos[0] + size[0], pos[1] + size[1], 0.0f,
		pos[0], pos[1] + size[1], 0.0f
	};
	
	float uv[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	
	GLushort i[] =
	{
		0, 1, 2,
		0, 2, 3
	};
	
	glVertexPointer(3, GL_FLOAT, 0, p);
	GLERROR();
	glTexCoordPointer(2, GL_FLOAT, 0, uv);
	GLERROR();
	
	glEnableClientState(GL_VERTEX_ARRAY);
	GLERROR();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GLERROR();
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, i);
	GLERROR();
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	GLERROR();
	glDisableClientState(GL_VERTEX_ARRAY);
	GLERROR();
}

static GLuint CreatePvr(TexturePVR& pvr)
{
	GLuint texture;
	
	glGenTextures(1, &texture);
	GLERROR();
	glBindTexture(GL_TEXTURE_2D, texture);
	GLERROR();
	
	for (size_t i = 0; i < pvr.Levels_get().size(); ++i)
	{
		const TexturePVRLevel* level = pvr.Levels_get()[i];
		glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, level->m_Sx, level->m_Sy, 0, level->m_DataSize, level->m_Data);
		GLERROR();
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLERROR();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLERROR();
	
	return texture;
}

static GLuint CreatePvr(const char* fileName)
{
	TexturePVR pvr;
	
	pvr.Load(fileName);
	
	return CreatePvr(pvr);
}

static GLuint CreateRgba(TextureRGBA* tex)
{
	GLuint id = 0;
	
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	GLERROR();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->Sx_get(), tex->Sy_get(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->Bytes_get());
	GLERROR();
	
	glGenerateMipmapOES(GL_TEXTURE_2D);
	GLERROR();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLERROR();
	
	return id;
}

static GLuint CreateRgba(const char* fileName)
{
	TextureRGBA* tex = (TextureRGBA*)ResIO::LoadTexture_RGBA_PNG(fileName);
	
	GLuint id = CreateRgba(tex);
	
	delete tex;
	
	return id;
}

typedef void (*ForEachCb)(void* obj, VB_Position3* pos, VB_Color4* color);

class PrevGrid
{
public:
	typedef struct
	{
		float xy[2];
		float uv[2];
		GLubyte rgba[4];
	} Vertex;
	
	PrevGrid()
	{
		mSx = 0;
		mSy = 0;
	}
	
	~PrevGrid()
	{
		Setup(0, 0);
	}
	
	void Setup(int sx, int sy)
	{
		mSx = 0;
		mSy = 0;
		
		if (sx * sy == 0)
			return;
		
		ShapeBuilder::I().PushScaling(Vector(320.0f, 480.0f, 1.0f));
		ShapeBuilder::I().PushScaling(Vector(0.5f, 0.5f, 1.0f));
		ShapeBuilder::I().PushTranslation(Vector(1.0f, 1.0f, 0.0f));
			ShapeBuilder::I().CreateGrid(sx, sy, ShapeBuilder::AXIS_Z, mMesh, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_COLOR | VertexBuffer::FVF_TEXn(1));
		ShapeBuilder::I().Pop();
		ShapeBuilder::I().Pop();
		ShapeBuilder::I().Pop();
		
		mSx = sx;
		mSy = sy;
	}
	
	void Render()
	{
		RenderMesh(&mMesh);
	}
	
	void ForEach(ForEachCb cb, void* obj)
	{
		VB_Position3* position = mMesh.GetVertexBuffer().position3;
		VB_Color4* color = mMesh.GetVertexBuffer().color;
		
		for (int x = 0; x < mSx; ++x)
		{
			for (int y = 0; y < mSy; ++y)
			{
				const int index = x + y * mSx;
				
				cb(obj, position + index, color + index);
			}
		}
	}
	
	Mesh mMesh;
	int mSx;
	int mSy;
};

static float Random(float min, float max)
{
	float v = (rand() & 4095) / 4095.0f;
	
	return min + (max - min) * v;
}

class Blob
{
public:
	Blob()
	{
		mPos = Vec2F(Random(0.0f, 320.0f), Random(0.0f, 480.0f));
		mSpeed = Vec2F(Random(-100.0f, +100.0f), Random(-100.0f, +100.0f));
	}
	
	void Update(float dt)
	{
		Vec2F next = mPos + mSpeed * dt;
		
		if (next[0] < 0.0f)
		{
			next[0] = 0.0f;
			mSpeed[0] *= -1.0f;
		}
		if (next[1] < 0.0f)
		{
			next[1] = 0.0f;
			mSpeed[1] *= -1.0f;
		}
		
		if (next[0] > 320.0f)
		{
			next[0] = 320.0f;
			mSpeed[0] *= -1.0f;
		}
		if (next[1] > 480.0f)
		{
			next[1] = 480.0f;
			mSpeed[1] *= -1.0f;
		}
		
		mPos = next;
	}
	
	float Eval(float x, float y)
	{
		float dx = mPos[0] - x;
		float dy = mPos[1] - y;
		
		float distanceSq = dx * dx + dy * dy;
//		float distance = sqrtf(distanceSq);
		
		return 1.0f / (distanceSq + 0.01f);
//		return 1.0f / (distance + 0.01f);
	}
	
	Vec2F mPos;
	Vec2F mSpeed;
};

static inline uint8_t ToUint8(float v)
{
	if (v < 0.0f)
		v = 0.0f;
	else if (v > 1.0f)
		v = 1.0f;
	
	return (uint8_t)(v * 255.0f);
}

#define BLOB_COUNT 3

class QuatDemo
{
public:
	QuatDemo()
	{
		mTime = 0.0f;
		mZoom = 1.0f;
		mOverride = false;
		mAltColors = false;
		mTouchCount = 0;
		mOverlay1 = 0;
		mOverlay2 = 0;
		mPreview1 = 0;
		
		Setup();
	}
	
	static void Eval(void* obj, VB_Position3* position, VB_Color4* color)
	{
		QuatDemo* self = (QuatDemo*)obj;
		
		float t = 0.0f;
		
		for (int i = 0; i < BLOB_COUNT; ++i)
		{
			t += self->mBlobArray[i].Eval(position->x, position->y) * 50.0f * 50.0f;
//			t += self->mBlobArray[i].Eval(position->x, position->y) * 40.0f;
		}
		
		color->r = color->g = color->b = 255;
		
		int a = ToUint8(t) * self->mTouchCount / 3;
		
		if (a > 255)
			a = 255;
		
		color->a = a;
	}
	
	void Update(float dt)
	{
		mTime += dt;
		
		if (mOverride == false)
		{
			float zDomination = (sinf(mTime / 2.0f) + 1.0f) / 2.0f;
			
			Vector axis(
				sinf(mTime / 1.0f) * (1.0f - zDomination),
				sinf(mTime / 1.3f) * (1.0f - zDomination),
				sinf(mTime / 1.7f));
			Quaternion rotator;
			rotator.FromAxisAngle(axis, F_PI * 2.0f * dt * 0.5f);
			mRotation *= rotator;
			mRotation.Normalize();
		}
		else
		{
		}
		
		for (int i = 0; i < BLOB_COUNT; ++i)
			mBlobArray[i].Update(dt);
		
		mPrevGrid.ForEach(Eval, this);
	}
	
	void Drag(Vec2F delta)
	{
		Vector v1 = Vector(0.0f, 0.0f, -1.0f);
		Vector v2 = Vector(delta[0], delta[1], 0.0f);
		Vector axis = v1 % v2;
		axis.Normalize();
		float angle = delta.Length_get() / 100.0f;
		Quaternion q;
		q.FromAxisAngle(axis, angle);
		q.Normalize();
		mRotation *= q;
		mRotation.Normalize();
	}
	
	void Render()
	{
		Vector color = Vector(0.1f, 0.1f, 0.1f) * 2.0f;
		
		float mod = (sinf(gTime * 3.0f) + 1.0f) * 0.5f;
		
		if (mTouchCount > 0)
			mod = 0.0f;
		
		color *= mod;
		
		glClearColor(color[0], color[1], color[2], 1.0f);
		GLERROR();
		glClearDepthf(1.0f);
		GLERROR();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glClear(GL_DEPTH_BUFFER_BIT);
		GLERROR();
		
		if (mTouchCount <= 2)
		{
		Mesh* meshes[2] = { &mMesh1, &mMesh2 };
		
		for (int i = 0; i < 2; ++i)
		{
			Matrix matProj;
			Matrix matView;
			Matrix matWorld;

			matProj.MakePerspectiveFovLH(F_PI / 2.0f / mZoom, 480.0f / 320.0f, 0.1f, 10.0f);
			matView.MakeLookat(Vector(0.0f, 0.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f), Vector(0.0f, 1.0f, 0.0f));
			
			{
				Matrix matWorldPos;
				Matrix matWorldRot;
				Matrix matWorldZoom;
				matWorldPos.MakeTranslation(Vector(0.0f, 0.0f, 2.0f));
				matWorldRot.MakeRotationEuler(Vector(0.0f, mTime, mTime));
//				if (i == 0)
					matWorldRot = mRotation.ToMatrix();
//				if (i == 1)
//					matWorldRot = mRotation.Inverse().ToMatrix();
				
				matWorld = matWorldPos * matWorldRot;
			}
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMultMatrixf(matProj.m_values);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		
#if 1
			glEnable(GL_LIGHTING);
			GLERROR();
			if (mAltColors == false)
			{
				float r1 = (sinf(mTime * 1.00f) + 1.0f) * 0.5f;
				float g1 = (sinf(mTime * 1.11f) + 1.0f) * 0.5f;
				float b1 = (sinf(mTime * 1.34f) + 1.0f) * 0.5f;
				
				float r2 = (sinf(mTime * 1.23f) + 1.0f) * 0.5f * 0.5f;
				float g2 = (sinf(mTime * 1.41f) + 1.0f) * 0.5f * 0.5f;
				float b2 = (sinf(mTime * 1.17f) + 1.0f) * 0.5f * 0.5f;
				
				SetLight(0, r1, g1, b1, +1.0f, +1.0f, +1.0f, 0.0f);
				SetLight(1, r2, g2, b2, -1.0f, -1.0f, -1.0f, 0.0f);
//				SetLight(0, 1.0f, 0.2f, 0.5f, +1.0f, +1.0f, +1.0f, 0.0f);
//				SetLight(1, 0.0f, 0.5f, 1.0f, -1.0f, -1.0f, -1.0f, 0.0f);
			}
			else
			{
				SetLight(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
				SetLight(1, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f);
			}
			glEnable(GL_LIGHT0);
			glEnable(GL_LIGHT1);
			GLERROR();
#endif
			
			glMultMatrixf(matView.m_values);
			
			glMultMatrixf(matWorld.m_values);

			glEnable(GL_CULL_FACE);
			
			if (i == 0)
				glCullFace(GL_FRONT);
			if (i == 1)
				glCullFace(GL_BACK);
			
			RenderMesh(meshes[i]);
			
			glDisable(GL_CULL_FACE);
		}
		}
		
		glDisable(GL_LIGHTING);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		Matrix ortho;
		ortho.MakeOrthoOffCenterLH(0.0f, 320.0f, 0.0f, 480.0f, 0.1f, 10.0f);
		glMultMatrixf(ortho.m_values);
//		glOrthof(0.0f, 320.0f, 480.0f, 0.0f, 0.1f, 10.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, 5.0f);
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		
		glEnable(GL_TEXTURE_2D);
		GLERROR();
		
		if (mTouchCount == 0)
		{
			glBindTexture(GL_TEXTURE_2D, mOverlay1);
			GLERROR();
			RenderRect(Vec2F(0.0f, 0.0f), Vec2F(128.0f, 256.0f));

			glBindTexture(GL_TEXTURE_2D, mOverlay2);
			GLERROR();
			RenderRect(Vec2F(320.0f - 64.0f, 480.0f - 128.0f), Vec2F(64.0f, 128.0f));
		}
		
		if (mTouchCount > 0)
		{
			glBindTexture(GL_TEXTURE_2D, mPreview1);
			GLERROR();
			mPrevGrid.Render();
		}
		
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		GLERROR();
		
		float interval = 0.6f;
		
		if (fmodf(mTime, interval) < interval * 0.5f)
		{
			glPushMatrix();
			glTranslatef(32.0f, 480.0f - 32.0f, 0.0f);
			RenderMesh(&mIndicatorB);
			RenderMesh(&mIndicatorF);
			glPopMatrix();
		}
	}
	
	void Setup()
	{
//		const int n = 25;
		const int n = 15;
//		const int n = 13;
//		const int n = 7;
//		const int n = 5;
//		const int n = 3;
		const float nInv = 1.0f / n;
		const float nInv1 = 1.0f / (n - 1);
		
		std::vector<Mesh*> meshes1;
		std::vector<Mesh*> meshes2;
		
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				const float u = i * nInv1;
				const float v = j * nInv1;

				const float x = (u - 0.5f) * 2.0f;
				const float y = (v - 0.5f) * 2.0f;

//				float scale = sinf(sqrtf(x * x + y * y) * F_PI * 2.0f * 2.0f);
				float scale = cosf(sqrtf(x * x + y * y) * F_PI * 2.0f * 1.3f);

				if (fabsf(scale) < 0.01f)
					continue;
				
				scale = -fabsf(scale);
				
				scale *= nInv;
				
				Mesh* mesh = new Mesh();
				
				ShapeBuilder::I().PushTranslation(Vector(x, y, 0.0f));
				{
					ShapeBuilder::I().PushRotation(Vector(x, y, x + y));
					{
//						ShapeBuilder::I().PushScaling(Vector(scale, scale, scale));
						ShapeBuilder::I().PushScaling(Vector(scale * 2.0f, scale, scale));
//						ShapeBuilder::I().PushScaling(Vector(scale * 4.0f, scale, scale));
						{
							ShapeBuilder::I().CreateCube(*mesh, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_NORMAL);
//							ShapeBuilder::I().CreateGridCube(3, 3, *mesh, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_NORMAL);
//							ShapeBuilder::I().CreateQuad(ShapeBuilder::AXIS_Z, *mesh, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_NORMAL);
//							ShapeBuilder::I().CreateDonut(13, 13, 1.0f, 0.3f, *mesh, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_NORMAL);
						}
						ShapeBuilder::I().Pop();
					}
					ShapeBuilder::I().Pop();
				}
				ShapeBuilder::I().Pop();
				
#if 0
				for (int i = 0; i < mesh->GetVertexBuffer().GetVertexCount(); ++i)
				{
					VB_Position3& position = mesh->GetVertexBuffer().position3[i];
					
					float t = (position.x + position.y + position.z) * 3.0f;
					
					position.x += cosf(t * 1.0f) * 0.05f;
					position.y += cosf(t * 1.1f) * 0.05f;
					position.z += cosf(t * 1.3f) * 0.05f;
				}
#endif
				
				if (scale >= 0.0f)
					meshes1.push_back(mesh);
				else
					meshes2.push_back(mesh);
			}
		}
			
		mOverlay1 = CreatePvr("ovl1.pvr4");
		mOverlay2 = CreatePvr("ovl2.pvr4");
		mPreview1 = CreateRgba("ss01.png");
		
//		mPrevGrid.Setup(32, 48);
		mPrevGrid.Setup(16, 24);
		
		float size = 14.0f;
		float border = 4.0f;
		int resolution = 20;
		
		ShapeBuilder::I().PushScaling(Vector(size, size, 1.0f));
		{
			ShapeBuilder::I().CreateCircle(resolution, mIndicatorF, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_COLOR);
			if (iPhoneAPI::ApplicationIsHacked())
				ShapeBuilder::I().Colorize(mIndicatorF, 255, 0, 0, 255);
			else
				ShapeBuilder::I().Colorize(mIndicatorF, 200, 255, 0, 255);
		}
		ShapeBuilder::I().Pop();
		
		ShapeBuilder::I().PushScaling(Vector(size + border, size + border, 1.0f));
		{
			ShapeBuilder::I().CreateCircle(resolution, mIndicatorB, VertexBuffer::FVF_XYZ | VertexBuffer::FVF_COLOR);
			ShapeBuilder::I().Colorize(mIndicatorB, 0, 0, 0, 255);
		}
		ShapeBuilder::I().Pop();
		
#if 1
		Mesh temp1;
		Mesh temp2;

		ShapeBuilder::I().Merge(meshes1, temp1);
		ShapeBuilder::I().Merge(meshes2, temp2);
		
		ShapeBuilder::I().ConvertToIndexed(temp1, mMesh1);
		ShapeBuilder::I().ConvertToIndexed(temp2, mMesh2);
#else
		ShapeBuilder::I().Merge(meshes1, mMesh1);
		ShapeBuilder::I().Merge(meshes2, mMesh2);
#endif
		
		ShapeBuilder::I().CalculateNormals(mMesh1);
		ShapeBuilder::I().CalculateNormals(mMesh2);
	}
	
	float mTime;
	Quaternion mRotation;
	std::vector<Mesh*> mMeshes;
	Mesh mMesh1;
	Mesh mMesh2;
	Mesh mMeshOvl;
	float mZoom;
	bool mOverride;
	bool mAltColors;
	int mTouchCount;
	GLuint mOverlay1;
	GLuint mOverlay2;
	GLuint mPreview1;
	Mesh mIndicatorF;
	Mesh mIndicatorB;
	PrevGrid mPrevGrid;
	Blob mBlobArray[BLOB_COUNT];
};

static inline void glRotateRadf(float angle, float x, float y, float z)
{
	glRotatef(angle * RAD2DEG, x, y, z);
}

static QuatDemo* gQuatDemo;

AppState* gAppState;

AppState::AppState()
{
	TouchListener listener;
	listener.Setup(this, TouchBegin, TouchEnd, TouchMove);
	mTouchDlg.Register(0, listener);
	mTouchDlg.Enable(0);
	
	mTouchState = TouchState_None;
	mZoom = 1.0f;
	
	gQuatDemo = new QuatDemo();
}

void AppState::Update(float dt)
{
	gTime += dt;
	
	gQuatDemo->mZoom = mZoom;
	
	gQuatDemo->Update(dt);
}

void AppState::Render()
{
	gQuatDemo->Render();
}

void AppState::DecideTouchState(int touchCount)
{
	if (touchCount == 0)
		mTouchState = TouchState_None;
	else if (touchCount == 1)
		mTouchState = TouchState_Rotate;
	else if (touchCount == 2)
		mTouchState = TouchState_Zoom;
	else if (touchCount == 3)
		mTouchState = TouchState_ColorCycle;
	
	gQuatDemo->mOverride = mTouchState != TouchState_None;
	gQuatDemo->mTouchCount = touchCount;
}

bool AppState::TouchBegin(void* obj, const TouchInfo& ti)
{
	AppState* self = (AppState*)obj;
	
//	NSLog(@"begin: %d (finger: %d)", ti.m_TouchCount, ti.m_FingerIndex);
	
	self->mTouchPosition[ti.m_FingerIndex] = ti.m_Location;
	
	self->DecideTouchState(ti.m_TouchCount);
	
	if (self->mTouchState == TouchState_Zoom)
	{
		self->mZoomPosition[0] = self->mTouchPosition[0];
		self->mZoomPosition[1] = self->mTouchPosition[1];
	}
	
	if (self->mTouchState == TouchState_ColorCycle)
	{
		gQuatDemo->mAltColors = !gQuatDemo->mAltColors;
	}
	
	return true;
}

bool AppState::TouchEnd(void* obj, const TouchInfo& ti)
{
	AppState* self = (AppState*)obj;
	
	self->DecideTouchState(ti.m_TouchCount - 1);
	
	return true;
}

bool AppState::TouchMove(void* obj, const TouchInfo& ti)
{
	AppState* self = (AppState*)obj;
	
	self->mTouchPosition[ti.m_FingerIndex] = ti.m_Location;
	
	Vec2F delta = ti.m_LocationDelta;
//	float distance = delta.Length_get();
	
//	NSLog(@"distance: %f", distance);
//	NSLog(@"state: %d", (int)self->mTouchState);
	
	switch (self->mTouchState)
	{
		case TouchState_None:
		{
			break;
		}
		
		case TouchState_Rotate:
		{
			gQuatDemo->Drag(delta);
			
			break;
		}
			
		case TouchState_Zoom:
		{
			float distance1 = (self->mZoomPosition[1] - self->mZoomPosition[0]).Length_get();
			self->mZoomPosition[ti.m_FingerIndex] = ti.m_Location;
			float distance2 = (self->mZoomPosition[1] - self->mZoomPosition[0]).Length_get();
			
			self->mZoom *= 1.0f + (distance2 - distance1) / 300.0f;
			
			break;
		}
			
		case TouchState_ColorCycle:
		{
			// todo: cycle colors
			break;
		}
	}
	
	return true;
}

void AppState::HandleTouchBegin(void* obj, void* arg)
{
	AppState* self = (AppState*)obj;
	
	self->mTouchDlg.HandleTouchBegin(&self->mTouchDlg, arg);
}

void AppState::HandleTouchEnd(void* obj, void* arg)
{
	AppState* self = (AppState*)obj;
	
	self->mTouchDlg.HandleTouchEnd(&self->mTouchDlg, arg);
}

void AppState::HandleTouchMove(void* obj, void* arg)
{
	AppState* self = (AppState*)obj;
	
	self->mTouchDlg.HandleTouchMove(&self->mTouchDlg, arg);
}

static void RenderBegin(EAGLContext* context, GLuint bufferId, int viewSx, int viewSy)
{
    [EAGLContext setCurrentContext:context];
    GLERROR();
	
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, bufferId);
	GLERROR();
    glViewport(0, 0, viewSx, viewSy);
    GLERROR();
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	GLERROR();
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLERROR();
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	GLERROR();
	
	Matrix matrix;
	
	matrix.MakePerspectiveFovLH(F_PI / 2.0f, 480.0f / 320.0f, 0.01f, 10.0f);
	
	glMultMatrixf(matrix.m_values);
	GLERROR();
}


static void RenderEnd(EAGLContext* context, GLuint colorRenderbuffer)
{
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
	GLERROR();
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
	GLERROR();
}

@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id) initWithCoder:(NSCoder*)coder
{    
    if ((self = [super initWithCoder:coder]))
	{
		[self setMultipleTouchEnabled:TRUE];
		
        // Get the layer
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
/*        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
	*/
	eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
		kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat, nil];
		
		renderer = new Renderer(eaglLayer);
		
		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = 1;
		displayLink = nil;
		animationTimer = nil;
		
		// A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
		// class is used as fallback when it isn't available.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			displayLinkSupported = TRUE;
		
		gAppState = new AppState();
		
		mTouchMgr.OnTouchBegin = CallBack(gAppState, AppState::HandleTouchBegin);
		mTouchMgr.OnTouchEnd = CallBack(gAppState, AppState::HandleTouchEnd);
		mTouchMgr.OnTouchMove = CallBack(gAppState, AppState::HandleTouchMove);
    }
	
    return self;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			CGPoint location = [touch locationInView:self];
			
			Vec2F locationVec = Vec2F(location.x, location.y);
			
			mTouchMgr.TouchBegin(touch, locationVec, locationVec, locationVec);
		}
	}
	catch (Exception& e)
	{
//		[self showException:e];
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
	
			CGPoint location = [touch locationInView:self];
			
			Vec2F locationVec = Vec2F(location.x, location.y);
			
			mTouchMgr.TouchMoved(touch, locationVec, locationVec, locationVec);
		}
	}
	catch (Exception& e)
	{
//		[self showException:e];
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			mTouchMgr.TouchEnd(touch);
		}
	}
	catch (Exception& e)
	{
//		[self showException:e];
	}
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			mTouchMgr.TouchEnd(touch);
		}
	}
	catch (Exception& e)
	{
//		[self showException:e];
	}
}

- (void) drawView:(id)sender
{
	RenderBegin(renderer->context, renderer->defaultFramebuffer, renderer->backingWidth, renderer->backingHeight);
	
   	renderer->Render();
	
	gAppState->Update(1.0f / 60.0f);
	gAppState->Render();
	
	RenderEnd(renderer->context, renderer->colorRenderbuffer);
}

- (void) layoutSubviews
{
	renderer->ResizeFromLayer((CAEAGLLayer*)self.layer);
	
    [self drawView:nil];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.

			displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[displayLink setFrameInterval:animationFrameInterval];
			[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			animationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 100.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[displayLink invalidate];
			displayLink = nil;
		}
		else
		{
			[animationTimer invalidate];
			animationTimer = nil;
		}
		
		animating = FALSE;
	}
}

- (void) dealloc
{
    delete renderer;
	renderer = 0;
	
    [super dealloc];
}

@end
