#pragma once

#include "klodder_forward.h"
#include "libgg_forward.h"
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
	
	void Write(StreamWriter& writer) const;
	void Read(StreamReader& reader);
	
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
	
	static CommandPacket Make_ColorSelect(float r, float g, float b, float a);
	static CommandPacket Make_ImageSize(int layerCount, int sx, int sy);
	static CommandPacket Make_DataLayerBlit(int index, const BlitTransform& transform);
	static CommandPacket Make_DataLayerClear(int index, float r, float g, float b, float a);
	static CommandPacket Make_DataLayerMerge(int index1, int index2);
	static CommandPacket Make_DataLayerOpacity(int index, float opacity);
	static CommandPacket Make_LayerOrder(int* layerOrder, int layerCount);
	static CommandPacket Make_DataLayerSelect(int index);
//	static CommandPacket Make_LayerSwap(int layer1, int layer2);
	static CommandPacket Make_DataLayerVisibility(int index, bool visibility);
	static CommandPacket Make_StrokeBegin(int index, bool smooth, bool mirrorX, float x, float y);
	static CommandPacket Make_StrokeEnd();
	static CommandPacket Make_StrokeMove(float x, float y);
	static CommandPacket Make_ToolSelect_SoftBrush(int diameter, float hardness, float spacing);
	static CommandPacket Make_ToolSelect_PatternBrush(int patternId, int diameter, float spacing);
	static CommandPacket Make_ToolSelect_SoftBrushDirect(int diameter, float hardness, float spacing);
	static CommandPacket Make_ToolSelect_PatternBrushDirect(int patternId, int diameter, float spacing);
	static CommandPacket Make_ToolSelect_SoftSmudge(int diameter, float hardness, float spacing, float strength);
	static CommandPacket Make_ToolSelect_PatternSmudge(int patternId, int diameter, float spacing, float strength);
	static CommandPacket Make_ToolSelect_SoftEraser(int diameter, float hardness, float spacing);
	static CommandPacket Make_ToolSelect_PatternEraser(int patternId, int diameter, float spacing);
};
