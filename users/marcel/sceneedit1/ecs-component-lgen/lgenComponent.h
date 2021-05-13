#pragma once

#include "component.h"

#include <vector>

class GxIndexBuffer;
class GxMesh;
class GxVertexBuffer;

struct LgenComponent : Component<LgenComponent>
{
	enum GeneratorType
	{
		kGeneratorType_OffsetSquare,
		kGeneratorType_DiamondSquare
	};
	
	enum FilterType
	{
		kFilterType_Highpass,
		kFilterType_Maximum,
		kFilterType_Mean,
		kFilterType_Median,
		kFilterType_Minimum,
		kFilterType_Quantize
	};
	
	struct Filter
	{
		bool enabled = true;
		FilterType type = kFilterType_Mean;
		int radius = 1;
		float numLevels = 1.f;
	};
	
	GeneratorType type = kGeneratorType_OffsetSquare;
	int resolution = 5;
	int seed = 0;
	float scale = 1.f;
	float height = 1.f;
	std::vector<Filter> filters;
	bool castShadows = false;
	
	GxVertexBuffer * vb = nullptr;
	GxIndexBuffer * ib = nullptr;
	GxMesh * mesh = nullptr;
	
	~LgenComponent() override final;
	
	virtual bool init() override final;
	
	virtual void propertyChanged(void * address) override final;
	
	void generate();
};

struct LgenComponentMgr : ComponentMgr<LgenComponent>
{
	void drawOpaque() const;
	void drawOpaque_ForwardShaded() const;
};

extern LgenComponentMgr g_lgenComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct LgenComponentType : ComponentType<LgenComponent>
{
	LgenComponentType()
		: ComponentType("LgenComponent", &g_lgenComponentMgr)
	{
		add("type", &LgenComponent::type);
		add("resolution", &LgenComponent::resolution)
			.limits(0, 10);
		add("seed", &LgenComponent::seed);
		add("scale", &LgenComponent::scale);
		add("height", &LgenComponent::height);
		add("filters", &LgenComponent::filters);
		add("castShadows", &LgenComponent::castShadows);
	}
	
	virtual void reflect(TypeDB & typeDB) override final
	{
		typeDB.addEnum<LgenComponent::GeneratorType>("LgenComponent::GeneratorType")
			.add("offsetSquare", LgenComponent::kGeneratorType_OffsetSquare)
			.add("diamondSquare", LgenComponent::kGeneratorType_DiamondSquare);
		
		typeDB.addEnum<LgenComponent::FilterType>("LgenComponent::FilterType")
			.add("highpass", LgenComponent::kFilterType_Highpass)
			.add("maximum", LgenComponent::kFilterType_Maximum)
			.add("mean", LgenComponent::kFilterType_Mean)
			.add("median", LgenComponent::kFilterType_Median)
			.add("minimum", LgenComponent::kFilterType_Minimum)
			.add("quantize", LgenComponent::kFilterType_Quantize);
		
		typeDB.addStructured<LgenComponent::Filter>("LgenComponent::Filter")
			.add("enabled", &LgenComponent::Filter::enabled)
			.add("type", &LgenComponent::Filter::type)
			.add("radius", &LgenComponent::Filter::radius)
				.addFlag(new ComponentMemberFlag_IntLimits(1, 8))
			.add("numLevels", &LgenComponent::Filter::numLevels);
	}
};

#endif
