#pragma once

#include "libgg_forward.h"
#include "libklodder_forward.h"
#include "Types.h"

#define MAX_LAYERS_V1 3

enum CommandType
{
	CommandType_Undefined = -1,
	CommandType_ColorSelect = 0,
	CommandType_ImageSize = 30,
	CommandType_LayerBlit = 47,
	CommandType_LayerClear = 40,
	CommandType_LayerMerge = 41,
	CommandType_LayerSelect = 42,
	CommandType_LayerOrder = 44,
	CommandType_LayerVisibility = 45,
	CommandType_LayerOpacity = 46,
	CommandType_ToolSelect_SoftBrush = 50,
	CommandType_ToolSelect_PatternBrush = 51,
	CommandType_ToolSelect_SoftBrushDirect = 57,
	CommandType_ToolSelect_PatternBrushDirect = 58,
	CommandType_ToolSelect_SoftSmudge = 52,
	CommandType_ToolSelect_PatternSmudge = 53,
	CommandType_ToolSelect_SoftEraser = 56,
	CommandType_ToolSelect_PatternEraser = 55,
	CommandType_Stroke_Begin = 100,
	CommandType_Stroke_End = 101,
	CommandType_Stroke_Move = 102
};

enum CommandStrokeFlag
{
	CommandStrokeFlag_Smooth = 0x01,
	CommandStrokeFlag_MirrorX = 0x02
};

class CommandPacket
{
public:
	inline CommandPacket()
	{
		mCommandType = CommandType_Undefined;
	}
	
	void Write(StreamWriter & writer) const;
	void Read(StreamReader & reader);
	
	int8_t mCommandType;
	int8_t mCommandVersion;
	
	union
	{
		struct
		{
			float rgba[4];
		} color;
		struct
		{
			uint32_t diameter;
			float hardness;
			float spacing;
		} brush_soft;
		struct
		{
			uint32_t pattern_id;
			uint32_t diameter;
			float spacing;
		} brush_pattern;
		struct
		{
			uint32_t diameter;
			float hardness;
			float spacing;
		} brush_soft_direct;
		struct
		{
			uint32_t pattern_id;
			uint32_t diameter;
			float spacing;
		} brush_pattern_direct;
		struct
		{
			uint32_t diameter;
			float hardness;
			float spacing;
		} eraser_soft;
		struct
		{
			uint32_t pattern_id;
			uint32_t diameter;
			float spacing;
		} eraser_pattern;
		struct
		{
			uint32_t layerCount;
			uint32_t sx;
			uint32_t sy;
		} image_size;
		struct
		{
			uint32_t index;
			struct
			{
				float anchorX;
				float anchorY;
				float angle;
				float scale;
				float x;
				float y;
			} transform;
		} layer_blit;
		struct
		{
			uint32_t index;
			float rgba[4];
		} layer_clear;
		struct
		{
			uint32_t index1;
			uint32_t index2;
		} layer_merge;
		struct
		{
			uint32_t index;
			float opacity;
		} layer_opacity;
		struct
		{
			uint32_t count;
			uint32_t order[MAX_LAYERS_V1];
		} layer_order;
		struct
		{
			uint32_t index;
		} layer_select;
		struct
		{
			uint32_t index;
			uint32_t visibility;
		} layer_visibility;
		struct
		{
			uint32_t diameter;
			float hardness;
			float spacing;
			float strength;
		} smudge_soft;
		struct
		{
			uint32_t pattern_id;
			uint32_t diameter;
			float spacing;
			float strength;
		} smudge_pattern;
		struct
		{
			uint8_t index;
			uint8_t flags;
			float x;
			float y;
		} stroke_begin;
		struct
		{
		} stroke_end;
		struct
		{
			float x;
			float y;
		} stroke_move;
	};
	
	static CommandPacket Make_ColorSelect(const float r, const float g, const float b, const float a);
	static CommandPacket Make_ImageSize(const int layerCount, const int sx, const int sy);
	static CommandPacket Make_DataLayerBlit(const int index, const BlitTransform & transform);
	static CommandPacket Make_DataLayerClear(const int index, const float r, const float g, const float b, const float a);
	static CommandPacket Make_DataLayerMerge(const int index1, const int index2);
	static CommandPacket Make_DataLayerOpacity(const int index, const float opacity);
	static CommandPacket Make_LayerOrder(const int * layerOrder, const int layerCount);
	static CommandPacket Make_DataLayerSelect(const int index);
//	static CommandPacket Make_LayerSwap(const int layer1, const int layer2);
	static CommandPacket Make_DataLayerVisibility(const int index, const bool visibility);
	static CommandPacket Make_StrokeBegin(const int index, const bool smooth, const bool mirrorX, const float x, const float y);
	static CommandPacket Make_StrokeEnd();
	static CommandPacket Make_StrokeMove(const float x, const float y);
	static CommandPacket Make_ToolSelect_SoftBrush(const int diameter, const float hardness, const float spacing);
	static CommandPacket Make_ToolSelect_PatternBrush(const int patternId, const int diameter, const float spacing);
	static CommandPacket Make_ToolSelect_SoftBrushDirect(const int diameter, const float hardness, const float spacing);
	static CommandPacket Make_ToolSelect_PatternBrushDirect(const int patternId, const int diameter, const float spacing);
	static CommandPacket Make_ToolSelect_SoftSmudge(const int diameter, const float hardness, const float spacing, const float strength);
	static CommandPacket Make_ToolSelect_PatternSmudge(const int patternId, const int diameter, const float spacing, const float strength);
	static CommandPacket Make_ToolSelect_SoftEraser(const int diameter, const float hardness, const float spacing);
	static CommandPacket Make_ToolSelect_PatternEraser(const int patternId, const int diameter, const float spacing);
};
