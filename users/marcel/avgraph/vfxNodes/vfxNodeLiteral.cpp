/*
Copyright (C) 2017 Marcel Smit
marcel303@gmail.com
https://www.facebook.com/marcel.smit981

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "graph.h"
#include "Parse.h"
#include "vfxNodeLiteral.h"

VFX_NODE_TYPE(bool, VfxNodeBoolLiteral)
{
	typeName = "literal.bool";
	displayName = "bool";
	
	out("value", "bool");
	outEditable("value");
}

VFX_NODE_TYPE(int, VfxNodeIntLiteral)
{
	typeName = "literal.int";
	displayName = "int";
	
	out("value", "int");
	outEditable("value");
}

VFX_NODE_TYPE(float, VfxNodeFloatLiteral)
{
	typeName = "literal.float";
	displayName = "float";
	
	out("value", "float");
	outEditable("value");
}

VFX_NODE_TYPE(string, VfxNodeStringLiteral)
{
	typeName = "literal.string";
	displayName = "string";
	
	out("value", "string");
	outEditable("value");
}

VFX_NODE_TYPE(color_literal, VfxNodeColorLiteral)
{
	typeName = "literal.color";
	displayName = "color";
	
	out("value", "color");
	outEditable("value");
}

//

VfxNodeBoolLiteral::VfxNodeBoolLiteral()
	: VfxNodeBase()
	, value(false)
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_Bool, &value);
}

void VfxNodeBoolLiteral::initSelf(const GraphNode & node)
{
	value = Parse::Bool(node.editorValue);
}

//

VfxNodeIntLiteral::VfxNodeIntLiteral()
	: VfxNodeBase()
	, value(0)
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_Int, &value);
}

void VfxNodeIntLiteral::initSelf(const GraphNode & node)
{
	value = Parse::Int32(node.editorValue);
}

//

VfxNodeFloatLiteral::VfxNodeFloatLiteral()
	: VfxNodeBase()
	, value(0)
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_Float, &value);
}

void VfxNodeFloatLiteral::initSelf(const GraphNode & node)
{
	value = Parse::Float(node.editorValue);
}

//

VfxNodeTransformLiteral::VfxNodeTransformLiteral()
	: VfxNodeBase()
	, value()
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_Transform, &value);
}

void VfxNodeTransformLiteral::initSelf(const GraphNode & node)
{
	// todo : parse node.editorValue;
}

//

VfxNodeStringLiteral::VfxNodeStringLiteral()
	: VfxNodeBase()
	, value()
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_String, &value);
}

void VfxNodeStringLiteral::initSelf(const GraphNode & node)
{
	value = node.editorValue;
}

//

VfxNodeColorLiteral::VfxNodeColorLiteral()
	: VfxNodeBase()
	, value(1.f, 1.f, 1.f, 1.f)
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_Value, kVfxPlugType_Color, &value);
}

void VfxNodeColorLiteral::initSelf(const GraphNode & node)
{
	const Color color = Color::fromHex(node.editorValue.c_str());
	
	value.setRgba(color.r, color.g, color.b, color.a);
}
