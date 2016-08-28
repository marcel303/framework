#include "BlitTransform.h"
#include "Calc.h"
#include "CommandPacket.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#define EX_VERSION ExceptionVA("unknown command version: %d. command type=%d", (int)mCommandVersion, (int)mCommandType)
#define EX_TYPE ExceptionVA("unknown command type: %d", (int)mCommandType)

static inline float FloatToPercentage(const float value, const bool clamp)
{
	if (clamp)
		return Calc::Mid(value, 0.0f, 1.0f);
		//return (uint8_t)Calc::Mid(value * 100.0f, 0.0f, 100.0f);
	else
		return value;
		//return (uint8_t)Calc::Mid(value * 100.0f, 0.0f, 255.0f);
}

static inline float FloatToColor(const float value)
{
	//return (uint8_t)Calc::Mid(value * 255.0f, 0.0f, 255.0f);
	return Calc::Mid(value, 0.0f, 1.0f);
}

void CommandPacket::Write(StreamWriter & writer) const
{
	writer.WriteInt8(mCommandType);
	
	switch (mCommandType)
	{
		case CommandType_ColorSelect:
			writer.WriteInt8(1); // version
			writer.WriteFloat(color.rgba[0]);
			writer.WriteFloat(color.rgba[1]);
			writer.WriteFloat(color.rgba[2]);
			writer.WriteFloat(color.rgba[3]);
			break;
		case CommandType_ImageSize:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(image_size.layerCount);
			writer.WriteUInt32(image_size.sx);
			writer.WriteUInt32(image_size.sy);
			break;
		case CommandType_LayerBlit:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_blit.index);
			writer.WriteFloat(layer_blit.transform.anchorX);
			writer.WriteFloat(layer_blit.transform.anchorY);
			writer.WriteFloat(layer_blit.transform.angle);
			writer.WriteFloat(layer_blit.transform.scale);
			writer.WriteFloat(layer_blit.transform.x);
			writer.WriteFloat(layer_blit.transform.y);
			break;
		case CommandType_LayerClear:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_clear.index);
			writer.WriteFloat(layer_clear.rgba[0]);
			writer.WriteFloat(layer_clear.rgba[1]);
			writer.WriteFloat(layer_clear.rgba[2]);
			writer.WriteFloat(layer_clear.rgba[3]);
			break;
		case CommandType_LayerMerge:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_merge.index1);
			writer.WriteUInt32(layer_merge.index2);
			break;
		case CommandType_LayerOpacity:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_opacity.index);
			writer.WriteFloat(layer_opacity.opacity);
			break;
		case CommandType_LayerOrder:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_order.count);
			for (uint32_t i = 0; i < layer_order.count; ++i)
				writer.WriteUInt32(layer_order.order[i]);
			break;
		case CommandType_LayerSelect:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_select.index);
			break;
		case CommandType_LayerVisibility:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(layer_visibility.index);
			writer.WriteUInt32(layer_visibility.visibility);
			break;
		case CommandType_ToolSelect_SoftBrush:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(brush_soft.diameter);
			writer.WriteFloat(brush_soft.hardness);
			writer.WriteFloat(brush_soft.spacing);
			break;
		case CommandType_ToolSelect_PatternBrush:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(brush_pattern.pattern_id);
			writer.WriteUInt32(brush_pattern.diameter);
			writer.WriteFloat(brush_pattern.spacing);
			break;
		case CommandType_ToolSelect_SoftBrushDirect:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(brush_soft_direct.diameter);
			writer.WriteFloat(brush_soft_direct.hardness);
			writer.WriteFloat(brush_soft_direct.spacing);
			break;
		case CommandType_ToolSelect_PatternBrushDirect:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(brush_pattern_direct.pattern_id);
			writer.WriteUInt32(brush_pattern_direct.diameter);
			writer.WriteFloat(brush_pattern_direct.spacing);
			break;
		case CommandType_ToolSelect_SoftSmudge:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(smudge_soft.diameter);
			writer.WriteFloat(smudge_soft.hardness);
			writer.WriteFloat(smudge_soft.spacing);
			writer.WriteFloat(smudge_soft.strength);
			break;
		case CommandType_ToolSelect_PatternSmudge:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(smudge_pattern.pattern_id);
			writer.WriteUInt32(smudge_pattern.diameter);
			writer.WriteFloat(smudge_pattern.spacing);
			writer.WriteFloat(smudge_pattern.strength);
			break;
		case CommandType_ToolSelect_SoftEraser:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(eraser_soft.diameter);
			writer.WriteFloat(eraser_soft.hardness);
			writer.WriteFloat(eraser_soft.spacing);
			break;
		case CommandType_ToolSelect_PatternEraser:
			writer.WriteInt8(1); // version
			writer.WriteUInt32(eraser_pattern.pattern_id);
			writer.WriteUInt32(eraser_pattern.diameter);
			writer.WriteFloat(eraser_pattern.spacing);
			break;
		case CommandType_Stroke_Begin:
			writer.WriteInt8(1); // version
			writer.WriteUInt8(stroke_begin.index);
			writer.WriteUInt8(stroke_begin.flags);
			writer.WriteFloat(stroke_begin.x);
			writer.WriteFloat(stroke_begin.y);
			break;
		case CommandType_Stroke_End:
			writer.WriteInt8(1); // version
			break;
		case CommandType_Stroke_Move:
			writer.WriteInt8(1); // version
			writer.WriteFloat(stroke_move.x);
			writer.WriteFloat(stroke_move.y);
			break;

		default:
			throw EX_TYPE;
	}
}

void CommandPacket::Read(StreamReader & reader)
{
	mCommandType = (CommandType)reader.ReadInt8();
	mCommandVersion = reader.ReadInt8();

	switch (mCommandType)
	{
		case CommandType_ColorSelect:
			if (mCommandVersion == 1)
			{
				color.rgba[0] = reader.ReadFloat();
				color.rgba[1] = reader.ReadFloat();
				color.rgba[2] = reader.ReadFloat();
				color.rgba[3] = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ImageSize:
			if (mCommandVersion == 1)
			{
				image_size.layerCount = reader.ReadUInt32();
				image_size.sx = reader.ReadUInt32();
				image_size.sy = reader.ReadUInt32();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerBlit:
			if (mCommandVersion == 1)
			{
				layer_blit.index = reader.ReadUInt32();
				layer_blit.transform.anchorX = reader.ReadFloat();
				layer_blit.transform.anchorY = reader.ReadFloat();
				layer_blit.transform.angle = reader.ReadFloat();
				layer_blit.transform.scale = reader.ReadFloat();
				layer_blit.transform.x = reader.ReadFloat();
				layer_blit.transform.y = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerClear:
			if (mCommandVersion == 1)
			{
				layer_clear.index = reader.ReadUInt32();
				layer_clear.rgba[0] = reader.ReadFloat();
				layer_clear.rgba[1] = reader.ReadFloat();
				layer_clear.rgba[2] = reader.ReadFloat();
				layer_clear.rgba[3] = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerMerge:
			if (mCommandVersion == 1)
			{
				layer_merge.index1 = reader.ReadUInt32();
				layer_merge.index2 = reader.ReadUInt32();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerOpacity:
			if (mCommandVersion == 1)
			{
				layer_opacity.index = reader.ReadUInt32();
				layer_opacity.opacity = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerOrder:
			if (mCommandVersion == 1)
			{
				layer_order.count = reader.ReadUInt32();
				
				if (layer_order.count > MAX_LAYERS_V1)
					throw ExceptionVA("layer_order.count > max number of layers (%d)", (int)layer_order.count);
				
				for (uint32_t i = 0; i < layer_order.count; ++i)
					layer_order.order[i] = reader.ReadUInt32();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerSelect:
			if (mCommandVersion == 1)
			{
				layer_select.index = reader.ReadUInt32();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_LayerVisibility:
			if (mCommandVersion == 1)
			{
				layer_visibility.index = reader.ReadUInt32();
				layer_visibility.visibility = reader.ReadUInt32();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_SoftBrush:
			if (mCommandVersion == 1)
			{
				brush_soft.diameter = reader.ReadUInt32();
				brush_soft.hardness = reader.ReadFloat();
				brush_soft.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_PatternBrush:
			if (mCommandVersion == 1)
			{
				brush_pattern.pattern_id = reader.ReadUInt32();
				brush_pattern.diameter = reader.ReadUInt32();
				brush_pattern.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_SoftBrushDirect:
			if (mCommandVersion == 1)
			{
				brush_soft_direct.diameter = reader.ReadUInt32();
				brush_soft_direct.hardness = reader.ReadFloat();
				brush_soft_direct.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_PatternBrushDirect:
			if (mCommandVersion == 1)
			{
				brush_pattern_direct.pattern_id = reader.ReadUInt32();
				brush_pattern_direct.diameter = reader.ReadUInt32();
				brush_pattern_direct.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_SoftSmudge:
			if (mCommandVersion == 1)
			{
				smudge_soft.diameter = reader.ReadUInt32();
				smudge_soft.hardness = reader.ReadFloat();
				smudge_soft.spacing = reader.ReadFloat();
				smudge_soft.strength = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_PatternSmudge:
			if (mCommandVersion == 1)
			{
				smudge_pattern.pattern_id = reader.ReadUInt32();
				smudge_pattern.diameter = reader.ReadUInt32();
				smudge_pattern.spacing = reader.ReadFloat();
				smudge_pattern.strength = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_SoftEraser:
			if (mCommandVersion == 1)
			{
				eraser_soft.diameter = reader.ReadUInt32();
				eraser_soft.hardness = reader.ReadFloat();
				eraser_soft.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_ToolSelect_PatternEraser:
			if (mCommandVersion == 1)
			{
				eraser_pattern.pattern_id = reader.ReadUInt32();
				eraser_pattern.diameter = reader.ReadUInt32();
				eraser_pattern.spacing = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_Stroke_Begin:
			if (mCommandVersion == 1)
			{
				stroke_begin.index = reader.ReadUInt8();
				stroke_begin.flags = reader.ReadUInt8();
				stroke_begin.x = reader.ReadFloat();
				stroke_begin.y = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_Stroke_End:
			if (mCommandVersion == 1)
			{
			}
			else
			{
				throw EX_VERSION;
			}
			break;
		case CommandType_Stroke_Move:
			if (mCommandVersion == 1)
			{
				stroke_move.x = reader.ReadFloat();
				stroke_move.y = reader.ReadFloat();
			}
			else
			{
				throw EX_VERSION;
			}
			break;

		default:
			throw EX_TYPE;
	}
}

CommandPacket CommandPacket::Make_ColorSelect(const float r, const float g, const float b, const float a)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ColorSelect;
	result.mCommandVersion = 1;
	result.color.rgba[0] = FloatToColor(r);
	result.color.rgba[1] = FloatToColor(g);
	result.color.rgba[2] = FloatToColor(b);
	result.color.rgba[3] = FloatToColor(a);
	
	return result;
}

CommandPacket CommandPacket::Make_ImageSize(const int layerCount, const int sx, const int sy)
{
	CommandPacket result;

	result.mCommandType = CommandType_ImageSize;
	result.mCommandVersion = 1;
	result.image_size.layerCount = layerCount;
	result.image_size.sx = sx;
	result.image_size.sy = sy;

	return result;
}

CommandPacket CommandPacket::Make_DataLayerBlit(const int index, const BlitTransform & transform)
{
	CommandPacket result;

	result.mCommandType = CommandType_LayerBlit;
	result.mCommandVersion = 1;
	result.layer_blit.index = index;
	result.layer_blit.transform.anchorX = transform.anchorX;
	result.layer_blit.transform.anchorY = transform.anchorY;
	result.layer_blit.transform.angle = transform.angle;
	result.layer_blit.transform.scale = transform.scale;
	result.layer_blit.transform.x = transform.x;
	result.layer_blit.transform.y = transform.y;

	return result;
}

CommandPacket CommandPacket::Make_DataLayerClear(const int index, const float r, const float g, const float b, const float a)
{
	CommandPacket result;

	result.mCommandType = CommandType_LayerClear;
	result.mCommandVersion = 1;
	result.layer_clear.index = index;
	result.layer_clear.rgba[0] = FloatToColor(r);
	result.layer_clear.rgba[1] = FloatToColor(g);
	result.layer_clear.rgba[2] = FloatToColor(b);
	result.layer_clear.rgba[3] = FloatToColor(a);

	return result;
}

CommandPacket CommandPacket::Make_DataLayerMerge(const int index1, const int index2)
{
	CommandPacket result;

	result.mCommandType = CommandType_LayerMerge;
	result.mCommandVersion = 1;
	result.layer_merge.index1 = index1;
	result.layer_merge.index2 = index2;

	return result;
}

CommandPacket CommandPacket::Make_DataLayerOpacity(const int index, const float opacity)
{
	Assert(index < MAX_LAYERS_V1);
	
	CommandPacket result;
	
	result.mCommandType = CommandType_LayerOpacity;
	result.mCommandVersion = 1;
	result.layer_opacity.index = index;
	result.layer_opacity.opacity = FloatToPercentage(opacity, true);
	
	return result;
}

CommandPacket CommandPacket::Make_LayerOrder(const int * layerOrder, const int layerCount)
{
	Assert(layerCount <= MAX_LAYERS_V1);
	
	CommandPacket result;
	
	result.mCommandType = CommandType_LayerOrder;
	result.mCommandVersion = 1;
	result.layer_order.count = layerCount;
	for (int i = 0; i < layerCount; ++i)
		result.layer_order.order[i] = layerOrder[i];
	
	return result;
}

CommandPacket CommandPacket::Make_DataLayerSelect(const int index)
{
	CommandPacket result;

	result.mCommandType = CommandType_LayerSelect;
	result.mCommandVersion = 1;
	result.layer_select.index = index;

	return result;
}

CommandPacket CommandPacket::Make_DataLayerVisibility(const int index, const bool visibility)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_LayerVisibility;
	result.mCommandVersion = 1;
	result.layer_visibility.index = index;
	result.layer_visibility.visibility = visibility;
	
	return result;
}

CommandPacket CommandPacket::Make_StrokeBegin(const int index, const bool smooth, const bool mirrorX, const float x, const float y)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_Stroke_Begin;
	result.mCommandVersion = 1;
	result.stroke_begin.index = index;
	result.stroke_begin.flags = 0;
	if (smooth)
		result.stroke_begin.flags |= CommandStrokeFlag_Smooth;
	if (mirrorX)
		result.stroke_begin.flags |= CommandStrokeFlag_MirrorX;
	result.stroke_begin.x = x;
	result.stroke_begin.y = y;
	
	return result;
}

CommandPacket CommandPacket::Make_StrokeEnd()
{
	CommandPacket result;
	
	result.mCommandType = CommandType_Stroke_End;
	result.mCommandVersion = 1;
	
	return result;
}

CommandPacket CommandPacket::Make_StrokeMove(const float x, const float y)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_Stroke_Move;
	result.mCommandVersion = 1;
	result.stroke_move.x = x;
	result.stroke_move.y = y;
	
	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_SoftBrush(const int diameter, const float hardness, const float spacing)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_SoftBrush;
	result.mCommandVersion = 1;
	result.brush_soft.diameter = diameter;
	result.brush_soft.hardness = FloatToPercentage(hardness, true);
	result.brush_soft.spacing = FloatToPercentage(spacing, false);

	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_PatternBrush(const int patternId, const int diameter, const float spacing)
{
	CommandPacket result;

	result.mCommandType = CommandType_ToolSelect_PatternBrush;
	result.mCommandVersion = 1;
	result.brush_pattern.pattern_id = patternId;
	result.brush_pattern.diameter = diameter;
	result.brush_pattern.spacing = FloatToPercentage(spacing, false);

	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_SoftBrushDirect(const int diameter, const float hardness, const float spacing)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_SoftBrushDirect;
	result.mCommandVersion = 1;
	result.brush_soft_direct.diameter = diameter;
	result.brush_soft_direct.hardness = FloatToPercentage(hardness, true);
	result.brush_soft_direct.spacing = FloatToPercentage(spacing, false);

	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_PatternBrushDirect(const int patternId, const int diameter, const float spacing)
{
	CommandPacket result;

	result.mCommandType = CommandType_ToolSelect_PatternBrushDirect;
	result.mCommandVersion = 1;
	result.brush_pattern_direct.pattern_id = patternId;
	result.brush_pattern_direct.diameter = diameter;
	result.brush_pattern_direct.spacing = FloatToPercentage(spacing, false);

	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_SoftSmudge(const int diameter, const float hardness, const float spacing, const float strength)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_SoftSmudge;
	result.mCommandVersion = 1;
	result.smudge_soft.diameter = diameter;
	result.smudge_soft.hardness = FloatToPercentage(hardness, true);
	result.smudge_soft.spacing = FloatToPercentage(spacing, false);
	result.smudge_soft.strength = FloatToPercentage(strength, true);
	
	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_PatternSmudge(const int patternId, const int diameter, const float spacing, const float strength)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_PatternSmudge;
	result.mCommandVersion = 1;
	result.smudge_pattern.pattern_id = patternId;
	result.smudge_pattern.diameter = diameter;
	result.smudge_pattern.spacing = FloatToPercentage(spacing, false);
	result.smudge_pattern.strength = FloatToPercentage(strength, true);
	
	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_SoftEraser(const int diameter, const float hardness, const float spacing)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_SoftEraser;
	result.mCommandVersion = 1;
	result.eraser_soft.diameter = diameter;
	result.eraser_soft.hardness = FloatToPercentage(hardness, true);
	result.eraser_soft.spacing = FloatToPercentage(spacing, false);

	return result;
}

CommandPacket CommandPacket::Make_ToolSelect_PatternEraser(const int patternId, const int diameter, const float spacing)
{
	CommandPacket result;
	
	result.mCommandType = CommandType_ToolSelect_PatternEraser;
	result.mCommandVersion = 1;
	result.eraser_pattern.pattern_id = patternId;
	result.eraser_pattern.diameter = diameter;
	result.eraser_pattern.spacing = FloatToPercentage(spacing, false);
	
	return result;
}
