#pragma once

#include "vfxNodeBase.h"

struct VfxNodeBoolLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	bool value;
	
	VfxNodeBoolLiteral()
		: VfxNodeBase()
		, value(false)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Bool, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Bool(node.editorValue);
	}
};

struct VfxNodeIntLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	int value;
	
	VfxNodeIntLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Int, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Int32(node.editorValue);
	}
};

struct VfxNodeFloatLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float value;
	
	VfxNodeFloatLiteral()
		: VfxNodeBase()
		, value(0)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Float, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Parse::Float(node.editorValue);
	}
};

struct VfxNodeTransformLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	VfxTransform value;
	
	VfxNodeTransformLiteral()
		: VfxNodeBase()
		, value()
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Transform, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		// todo : parse node.editorValue;
	}
};

struct VfxNodeStringLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	std::string value;
	
	VfxNodeStringLiteral()
		: VfxNodeBase()
		, value()
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_String, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = node.editorValue;
	}
};

struct VfxNodeColorLiteral : VfxNodeBase
{
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	Color value;
	
	VfxNodeColorLiteral()
		: VfxNodeBase()
		, value(1.f, 1.f, 1.f, 1.f)
	{
		resizeSockets(0, kOutput_COUNT);
		addOutput(kOutput_Value, kVfxPlugType_Color, &value);
	}
	
	virtual void initSelf(const GraphNode & node) override
	{
		value = Color::fromHex(node.editorValue.c_str());
	}
};
