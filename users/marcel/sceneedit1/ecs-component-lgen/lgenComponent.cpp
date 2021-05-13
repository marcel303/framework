#include "lgenComponent.h"

#define DEFINE_COMPONENT_TYPES
#include "sceneNodeComponent.h"

#include "gx_mesh.h"

#include "Lgen.h"

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
	
// fixme : let lgen have its own RNG with local scope
	srand(seed);
	
	switch (type)
	{
	case kGeneratorType_OffsetSquare:
		{
			lgen::Generator_OffsetSquare g;
			
			g.generate(h.get());
		}
		break;
		
	case kGeneratorType_DiamondSquare:
		{
			lgen::Generator_DiamondSquare g;
			
			g.generate(h.get());
		}
		break;
	}
	
	h.rerange(-.5f, +.5f);
	
	for (auto & filter : filters)
	{
		if (filter.enabled == false)
			continue;
			
		const int size = filter.radius * 2 + 1;
	
	// todo : we don't want filters to use wrap-around sampling
	
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
		
		gxBegin(GX_QUADS);
		{
			auto & hs = h.get();
			
			for (int tx = 0; tx + 1 < hs.w; ++tx)
			{
				for (int tz = 0; tz + 1 < hs.h; ++tz)
				{
					const float h00 = hs.getHeight(tx + 0, tz + 0);
					const float h10 = hs.getHeight(tx + 1, tz + 0);
					const float h11 = hs.getHeight(tx + 1, tz + 1);
					const float h01 = hs.getHeight(tx + 0, tz + 1);
					
				// todo : compute smooth normals
					const float dx = h10 - h00;
					const float dz = h01 - h00;
					//const Vec3 n = (Vec3(1, dx, 0) % Vec3(0, dz, 1)).CalcNormalized();
					const Vec3 n = -(Vec3(scale, dx * height, 0) % Vec3(0, dz * height, scale)).CalcNormalized();
					gxNormal3fv(&n[0]);
					gxColor3fv(&n[0]);
					
					gxVertex3f(tx + 0, h00, tz + 0);
					gxVertex3f(tx + 1, h10, tz + 0);
					gxVertex3f(tx + 1, h11, tz + 1);
					gxVertex3f(tx + 0, h01, tz + 1);
				}
			}
		}
		gxEnd();
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
