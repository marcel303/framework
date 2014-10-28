#pragma once

#include "BitStream.h"
#include "DrawState.h"
#include "Packet.h"
#include "NetSerializable.h"

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

	void Serialize(NetSerializationContext& context, command_t& lastCommand)
	{
		uint8_t type = this->type;

		bool serializeHeader = (client_id != lastCommand.client_id) || (type != lastCommand.type);
		context.Serialize(serializeHeader);
		if (serializeHeader)
		{
			context.Serialize(client_id);
			context.Serialize(type);

			this->type = (COMMAND_TYPE)type;
		}

		int ser = 0;

		if (this->type == CMD_BRUSH)
			ser = ser_brush;
		if (this->type == CMD_BLUR)
			ser = ser_blur;
		if (this->type == CMD_SMUDGE)
			ser = ser_smudge;

		if (ser & SER_COORD)
		{
			for (int i = 0; i < 2; ++i)
			{
				uint16_t coord = draw_state.coord.p[i];
				uint16_t lastCoord = lastCommand.draw_state.coord.p[i];

				bool bits4 = (coord & 0xfff0) == (lastCoord & 0xfff0);
				context.Serialize(bits4);
				if (bits4)
				{
					uint16_t bits = coord & 0x000f;
					context.SerializeBits(bits, 4);
					coord = (coord & 0xfff0) | bits;
				}
				else
				{
					bool bits8 = (coord & 0xff00) == (lastCoord & 0xff00);
					context.Serialize(bits8);
					if (bits8)
					{
						uint16_t bits = coord & 0x00ff;
						context.SerializeBits(bits, 8);
						coord = (coord & 0xff00) | bits;
					}
					else
					{
						context.Serialize(coord);
					}
				}

				if (context.IsRecv())
				{
					draw_state.coord.p[i] = (int16_t)coord;
				}
			}
		}

		if (ser & SER_BRUSH)
		{
			bool serializeBrush = (draw_state.brush_desc != lastCommand.draw_state.brush_desc);
			context.Serialize(serializeBrush);
			if (serializeBrush)
			{
				context.Serialize(draw_state.brush_desc.size);
				context.Serialize(draw_state.brush_desc.hardness);
				context.Serialize(draw_state.brush_desc.cos_freq[0]);
				context.Serialize(draw_state.brush_desc.cos_freq[1]);
			}
		}

		if (ser & SER_COLOR)
		{
			bool serializeColor =
				(draw_state.color[0] != lastCommand.draw_state.color[0]) ||
				(draw_state.color[1] != lastCommand.draw_state.color[1]) ||
				(draw_state.color[2] != lastCommand.draw_state.color[2]) ||
				(draw_state.opacity != lastCommand.draw_state.opacity);
			context.Serialize(serializeColor);
			if (serializeColor)
			{
				uint8_t color[4];

				color[0] = FTOI(draw_state.color[0]);
				color[1] = FTOI(draw_state.color[1]);
				color[2] = FTOI(draw_state.color[2]);
				color[3] = FTOI(draw_state.opacity);

				context.Serialize(color[0]);
				context.Serialize(color[1]);
				context.Serialize(color[2]);
				context.Serialize(color[3]);

				if (context.IsRecv())
				{
					draw_state.color[0] = ITOF(color[0]);
					draw_state.color[1] = ITOF(color[1]);
					draw_state.color[2] = ITOF(color[2]);
					draw_state.opacity = ITOF(color[3]);
				}
			}
		}

		if (ser & SER_BLEND)
		{
			bool serializeBlendMode = (draw_state.blend_mode != lastCommand.draw_state.blend_mode);
			context.Serialize(serializeBlendMode);
			if (serializeBlendMode)
			{
				uint8_t blend_mode = draw_state.blend_mode;

				context.Serialize(blend_mode);

				draw_state.blend_mode = blend_mode;
			}
		}

		if (ser & SER_BLUR)
		{
			bool serializeBlur = (draw_state.blur_desc != lastCommand.draw_state.blur_desc);
			context.Serialize(serializeBlur);
			if (serializeBlur)
			{
				context.Serialize(draw_state.blur_desc.size);
			}
		}

		if (ser & SER_SMUDGE)
		{
			bool serializeSmudge = (draw_state.smudge_desc != lastCommand.draw_state.smudge_desc);
			context.Serialize(serializeSmudge);
			if (serializeSmudge)
			{
				//bitStream..Write(draw_state.smudge_desc.size);
				context.Serialize(draw_state.smudge_desc.strength);
			}
			bool serializeDirection =
				(draw_state.direction.p[0] != lastCommand.draw_state.direction.p[0]) ||
				(draw_state.direction.p[1] != lastCommand.draw_state.direction.p[1]);
			context.Serialize(serializeDirection);
			if (serializeDirection)
			{
				context.Serialize(draw_state.direction.p[0]);
				context.Serialize(draw_state.direction.p[1]);
			}
		}
	}

	static uint32_t SerializeVector(BitStream& bitStream, std::vector<command_t>& commands, uint32_t& startIndex, uint32_t stopSize)
	{
		uint32_t count = 0;

		command_t lastCommand;
		memset(&lastCommand, 0, sizeof(lastCommand));

		NetSerializationContext context;
		context.Set(true, true, bitStream);

#if 0
		commands[startIndex++].Serialize(context, lastCommand);

		++count;
#else
		for (size_t& i(startIndex); i < commands.size() && bitStream.GetDataSize() < stopSize; ++i, ++count)
		{
			commands[i].Serialize(context, lastCommand);

			lastCommand = commands[i];
		}
#endif

		return count;
	}

	static void DeserializeVector(BitStream& bitStream, std::vector<command_t>& commands, uint32_t count)
	{
		command_t command;
		memset(&command, 0, sizeof(command));

		NetSerializationContext context;
		context.Set(true, false, bitStream);
		
		for (size_t i = 0; i < count; ++i)
		{
			command.Serialize(context, command);

			commands.push_back(command);
		}
	}

	int32_t client_id;
	COMMAND_TYPE type;

	CL_DrawState draw_state;
};
