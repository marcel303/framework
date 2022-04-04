#include "lgenComponent.h"

#define DEFINE_COMPONENT_TYPES
#include "sceneNodeComponent.h"

#include "gx_mesh.h"

#include "Lgen.h"

#include "Noise.h"

LgenComponentMgr g_lgenComponentMgr;

LgenComponent::~LgenComponent()
{
	delete mesh;
	mesh = nullptr;
	
	vb->free();
	delete vb;
	vb = nullptr;
	
	ib->free();
	delete ib;
	ib = nullptr;
}

bool LgenComponent::init()
{
	vb = new GxVertexBuffer();
	ib = new GxIndexBuffer();
	mesh = new GxMesh();
	
	generate();
	
	return true;
}

void LgenComponent::propertyChanged(void * address)
{
	if (address != &scale &&
		address != &height &&
		address != &castShadows)
	{
		generate();
	}
}

void LgenComponent::generate()
{
	lgen::DoubleBufferedHeightfield h;
	
	h.setSize(
		1 << resolution,
		1 << resolution);
	
	switch (type)
	{
	case kGeneratorType_OffsetSquare:
		{
			lgen::Generator_OffsetSquare g;
			
			g.generate(h.get(), seed);
		}
		break;
		
	case kGeneratorType_DiamondSquare:
		{
			lgen::Generator_DiamondSquare g;
			
			g.generate(h.get(), seed);
		}
		break;
		
	case kGeneratorType_SimplexNoise:
		{
			auto & hs = h.get();
			
			for (int i = 0; i < hs.w; ++i)
			{
				for (int j = 0; j < hs.h; ++j)
				{
					hs.height[i][j] = scaled_octave_noise_3d(4, .5f, 1.f / hs.w, -height / 2.f, +height / 2.f, i, j, seed * 321.f);
				}
			}
		}
		break;
	}
	
	h.rerange(-.5f, +.5f);
	
	for (auto & filter : filters)
	{
		if (filter.enabled == false)
			continue;
			
		const int size = filter.radius * 2 + 1;
		
		// note : the default border mode for filters is bmClamp, which is what we want
		
		switch (filter.type)
		{
		case kFilterType_Highpass:
			{
				lgen::FilterHighpass f;
				h.applyFilter(f);
			}
			break;
			
		case kFilterType_Maximum:
			{
				lgen::FilterMaximum f;
				f.setMatrixSize(size, size);
				h.applyFilter(f);
			}
			break;
			
		case kFilterType_Mean:
			{
			// fixme : checks like these .. if (w & 0x1 && w >= 3 && h & 0x1 && h >= 3).. in setMatrixSize
				lgen::FilterMean f;
				f.setMatrixSize(size, size);
				h.applyFilter(f);
			}
			break;
			
		case kFilterType_Median:
			{
				lgen::FilterMedian f;
				f.setMatrixSize(size, size);
				h.applyFilter(f);
			}
			break;
			
		case kFilterType_Minimum:
			{
				lgen::FilterMinimum f;
				f.setMatrixSize(size, size);
				h.applyFilter(f);
			}
			break;
			
		case kFilterType_Quantize:
			{
				lgen::FilterQuantize f;
				f.setNumLevels(filter.numLevels);
				h.applyFilter(f);
			}
			break;
		}
	}
	
	gxCaptureMeshBegin(*mesh, *vb, *ib);
	{
		setColor(colorWhite);
		
		if (blocky)
		{
			gxBegin(GX_QUADS);
			{
				auto & hs = h.get();

				// emit quads
				
				for (int tx = 0; tx < hs.w; ++tx)
				{
					for (int tz = 0; tz < hs.h; ++tz)
					{
						// x-edge
						
						if (tz + 1 < hs.h)
						{
							const float h0 = hs.getHeight(tx, tz + 0);
							const float h1 = hs.getHeight(tx, tz + 1);
							
							setColor(colorRed);
							gxNormal3f(0, 0, h0 < h1 ? -1 : +1);
							gxVertex3f(tx + 0, h0, tz + 1);
							gxVertex3f(tx + 1, h0, tz + 1);
							gxVertex3f(tx + 1, h1, tz + 1);
							gxVertex3f(tx + 0, h1, tz + 1);
						}
						
						// z-edge
						
						if (tx + 1 < hs.w)
						{
							const float h0 = hs.getHeight(tx + 0, tz);
							const float h1 = hs.getHeight(tx + 1, tz);
							
							setColor(colorBlue);
							gxNormal3f(h0 < h1 ? -1 : +1, 0, 0);
							gxVertex3f(tx + 1, h1, tz + 0);
							gxVertex3f(tx + 1, h1, tz + 1);
							gxVertex3f(tx + 1, h0, tz + 1);
							gxVertex3f(tx + 1, h0, tz + 0);
						}
						
						// top
						
						{
							const float h = hs.getHeight(tx, tz);
							
							setColor(colorGreen);
							gxNormal3f(0, 1, 0);
							gxVertex3f(tx + 0, h, tz + 0);
							gxVertex3f(tx + 1, h, tz + 0);
							gxVertex3f(tx + 1, h, tz + 1);
							gxVertex3f(tx + 0, h, tz + 1);
						}
					}
				}
			}
			gxEnd();
		}
		else
		{
			gxBegin(GX_QUADS);
			{
				auto & hs = h.get();
				
				// calculate normals
				
				Vec3 * normals = new Vec3[hs.w * hs.h];
				
			// todo : remove scale/height. shader should handle this
				const Vec3 s(scale, height, scale);
						
				for (int tx = 0; tx < hs.w; ++tx)
				{
					for (int tz = 0; tz < hs.h; ++tz)
					{
						const int x1 = tx - 1 >= 0 ? tx - 1 : 0;
						const int z1 = tz - 1 >= 0 ? tz - 1 : 0;
						
						const int x2 = tx + 1 <= hs.w - 1 ? tx + 1 : hs.w - 1;
						const int z2 = tz + 1 <= hs.h - 1 ? tz + 1 : hs.h - 1;
						
						const Vec3 px1(x1, hs.getHeight(x1, tz), tz);
						const Vec3 px2(x2, hs.getHeight(x2, tz), tz);
						
						const Vec3 pz1(tx, hs.getHeight(tx, z1), z1);
						const Vec3 pz2(tx, hs.getHeight(tx, z2), z2);
						
						const Vec3 dx = (px2 - px1) / 2.f;
						const Vec3 dz = (pz2 - pz1) / 2.f;
						
						const Vec3 n = -(dx.Mul(s) % dz.Mul(s)).CalcNormalized();
						
						normals[tx + tz * hs.w] = n;
					}
				}
				
				// emit quads
				
			#define n(x, z) &normals[(tx + x) + (tz + z) * hs.w][0]
			
				for (int tx = 0; tx + 1 < hs.w; ++tx)
				{
					for (int tz = 0; tz + 1 < hs.h; ++tz)
					{
						gxColor3fv(n(0, 0)); gxNormal3fv(n(0, 0)); gxVertex3f(tx + 0, hs.getHeight(tx + 0, tz + 0), tz + 0);
						gxColor3fv(n(1, 0)); gxNormal3fv(n(1, 0)); gxVertex3f(tx + 1, hs.getHeight(tx + 1, tz + 0), tz + 0);
						gxColor3fv(n(1, 1)); gxNormal3fv(n(1, 1)); gxVertex3f(tx + 1, hs.getHeight(tx + 1, tz + 1), tz + 1);
						gxColor3fv(n(0, 1)); gxNormal3fv(n(0, 1)); gxVertex3f(tx + 0, hs.getHeight(tx + 0, tz + 1), tz + 1);
					}
				}
				
			#undef n
				
				delete [] normals;
				normals = nullptr;
			}
			gxEnd();
		}
	}
	gxCaptureMeshEnd();
}

void LgenComponentMgr::drawOpaque() const
{
	for (auto * i = head; i != nullptr; i = i->next)
	{
		if (i->enabled == false)
			continue;
			
		auto * sceneNodeComp = g_sceneNodeComponentMgr.getComponent(i->componentSet->id);
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr)
		{
			gxPushMatrix();
			{
				gxMultMatrixf(sceneNodeComp->objectToWorld.m_v);
				
				const int size = 1 << i->resolution;
				
				gxScalef(i->scale, i->height, i->scale);
				gxTranslatef(-size / 2.f, 0, -size / 2.f);
				
				i->mesh->draw();
			}
			gxPopMatrix();
		}
	}
}

void LgenComponentMgr::drawOpaque_ForwardShaded() const
{
	// todo : draw opaque as needed
}
