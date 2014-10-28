#pragma once

#include "BitStream.h"
#include "DrawState.h"
#include "Packet.h"

enum COMMAND_TYPE
{
	CMD_BRUSH,
	CMD_LINE,
	CMD_BLUR,
	CMD_SMUDGE
};

class command_t
{
public:
	void CreateBrush(CL_DrawState& draw_state)
	{
		this->type = CMD_BRUSH;
		this->draw_state = draw_state;
	}

	void CreateBlur(CL_DrawState& draw_state)
	{
		this->type = CMD_BLUR;
		this->draw_state = draw_state;
	}

	void CreateSmudge(CL_DrawState& draw_state)
	{
		this->type = CMD_SMUDGE;
		this->draw_state = draw_state;
	}

#define SER_COORD 0x01
#define SER_BRUSH 0x02
#define SER_COLOR 0x04
#define SER_BLEND 0x08
#define SER_BLUR 0x10
#define SER_SMUDGE 0x20

	const static int ser_brush = SER_COORD | SER_BRUSH | SER_COLOR | SER_BLEND;
	const static int ser_blur = SER_COORD | SER_BRUSH | SER_BLUR;
	const static int ser_smudge = SER_COORD | SER_BRUSH | SER_SMUDGE;

	void Serialize(BitStream& bitStream)
	{
		uint8_t type = this->type;

		bitStream.Write(client_id);
		bitStream.Write(type);

		int ser = 0;

		if (this->type == CMD_BRUSH)
			ser = ser_brush;
		if (this->type == CMD_BLUR)
			ser = ser_blur;
		if (this->type == CMD_SMUDGE)
			ser = ser_smudge;

		if (ser & SER_COORD)
		{
			short coord[2] = { draw_state.coord.p[0], draw_state.coord.p[1] };

			bitStream.Write(coord[0]);
			bitStream.Write(coord[1]);
		}

		if (ser & SER_BRUSH)
		{
			bitStream.Write(draw_state.brush_desc.size);
			bitStream.Write(draw_state.brush_desc.hardness);
			bitStream.Write(draw_state.brush_desc.cos_freq[0]);
			bitStream.Write(draw_state.brush_desc.cos_freq[1]);
		}

		if (ser & SER_COLOR)
		{
			uint8_t color[4];

			color[0] = FTOI(draw_state.color[0]);
			color[1] = FTOI(draw_state.color[1]);
			color[2] = FTOI(draw_state.color[2]);
			color[3] = FTOI(draw_state.opacity);

			bitStream.Write(color[0]);
			bitStream.Write(color[1]);
			bitStream.Write(color[2]);
			bitStream.Write(color[3]);
		}

		if (ser & SER_BLEND)
		{
			uint8_t blend_mode = draw_state.blend_mode;

			bitStream.Write(blend_mode);
		}

		if (ser & SER_BLUR)
		{
			bitStream.Write(draw_state.blur_desc.size);
		}

		if (ser & SER_SMUDGE)
		{
			//bitStream..Write(draw_state.smudge_desc.size);
			bitStream.Write(draw_state.smudge_desc.strength);

			bitStream.Write(draw_state.direction.p[0]);
			bitStream.Write(draw_state.direction.p[1]);
		}
	}

	void DeSerialize(BitStream& bitStream)
	{
		uint8_t type;

		bitStream.Read(client_id);
		bitStream.Read(type);

		this->type = (COMMAND_TYPE)type;

		int ser;

		if (this->type == CMD_BRUSH)
			ser = ser_brush;
		if (this->type == CMD_BLUR)
			ser = ser_blur;
		if (this->type == CMD_SMUDGE)
			ser = ser_smudge;

		if (ser & SER_COORD)
		{
			short coord[2];

			bitStream.Read(coord[0]);
			bitStream.Read(coord[1]);

			draw_state.coord.p[0] = coord[0];
			draw_state.coord.p[1] = coord[1];
		}

		if (ser & SER_BRUSH)
		{
			bitStream.Read(draw_state.brush_desc.size);
			bitStream.Read(draw_state.brush_desc.hardness);
			bitStream.Read(draw_state.brush_desc.cos_freq[0]);
			bitStream.Read(draw_state.brush_desc.cos_freq[1]);
		}

		if (ser & SER_COLOR)
		{
			uint8_t color[4];

			bitStream.Read(color[0]);
			bitStream.Read(color[1]);
			bitStream.Read(color[2]);
			bitStream.Read(color[3]);

			draw_state.color[0] = ITOF(color[0]);
			draw_state.color[1] = ITOF(color[1]);
			draw_state.color[2] = ITOF(color[2]);
			draw_state.opacity = ITOF(color[3]);
		}

		if (ser & SER_BLEND)
		{
			uint8_t blend_mode;

			bitStream.Read(blend_mode);

			draw_state.blend_mode = blend_mode;
		}

		if (ser & SER_BLUR)
		{
			bitStream.Read(draw_state.blur_desc.size);
		}

		if (ser & SER_SMUDGE)
		{
			//bitStream.Read(draw_state.smudge_desc.size);
			bitStream.Read(draw_state.smudge_desc.strength);

			bitStream.Read(draw_state.direction.p[0]);
			bitStream.Read(draw_state.direction.p[1]);
		}
	}

	int32_t client_id;
	COMMAND_TYPE type;

	CL_DrawState draw_state;
};
