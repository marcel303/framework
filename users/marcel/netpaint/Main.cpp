#include <algorithm>
#include <allegro.h>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "AntiAliased.h"
#include "Brush.h"
#include "Canvas.h"
#include "ChannelManager.h"
#include "Command.h"
#include "Discover.h"
#include "DrawState.h"
#include "EventGen.h"
#include "EventMgr.h"
#include "GkContext.h"
#include "GkWidget.h"
#include "Image.h"
#include "NetProtocols.h"
#include "NetSocket.h"
#include "Packet.h"
#include "PacketDispatcher.h"
#include "Paint.h"
#include "PolledTimer.h"
#include "RectSet.h"
#include "Smudge.h"
#include "Timer.h"

#ifdef WIN32
	#include <conio.h>
#endif

#define COMPILE_SERVER 1
#define COMPILE_CLIENT 1

#define SV_PORT 6050

#define PROTOCOL_DRAW (PROTOCOL_CUSTOM + 0)

class AnimTimer
{
public:
	void Set(double t)
	{
		this->t = t;
		polledTimer.Initialize(&g_TimerRT);
		polledTimer.SetInterval(t);
		polledTimer.Start();
	}

	bool Trig()
	{
		if (polledTimer.ReadTick())
		{
			polledTimer.Stop();
			return true;
		}

		return false;
	}

	double T()
	{
		double result = polledTimer.TimeS_get() / t;

		if (result > 1.0)
			result = 1.0;

		return result;
	}

	double t;
	PolledTimer polledTimer;
};

class PopupWidget : public GkWidget
{
public:
	PopupWidget(const std::string& filename)
	{
		SetSize(40, 40);

		bmp = load_bitmap_il(filename.c_str());

		state = ST_INACTIVE;
		timer.Set(0.0);

		ap = 40.0f;
		av = 0.0f;
	}

	void DoUpdate(float dt)
	{
		if (timer.Trig())
		{
			HandleTimer();
		}

		av -= 1000.0f * dt;
		ap += av * dt;
		av *= pow(0.9f, dt);
		if (ap < 0.0f)
		{
			ap *= -.5f;
			av *= -.5f;
		}

		SetPos(pos.p[0], ap);

		Invalidate();
	}

	void HandleTimer()
	{
		switch (state)
		{
		case ST_INACTIVE:
			state = ST_FADEIN;
			timer.Set(1.0);
			break;
		case ST_FADEIN:
			state = ST_FADEOUT;
			timer.Set(1.0);
			break;
		case ST_FADEOUT:
			state = ST_DEAD;
			Dispose();
			break;
		}
	}

	virtual void DoPaint()
	{
		// FIXME: Use global coords.

		Render(gk_buf, pos.p[0], pos.p[1]);
	}

	void Render(BITMAP* dst, int x, int y)
	{
		float t = timer.T();

		int w = size.p[0];
		int h = size.p[1];

		switch (state)
		{
		case ST_FADEIN:
			rectfill(dst, x, y, x + w - 1, y + h - 1, makecol(0, t * 255.0, 0));
			break;
		case ST_FADEOUT:
			rectfill(dst, x, y, x + w - 1, y + h - 1, makecol(0, 255 - t * 255.0, 0));
			break;
		}

		int s = 3;

		rectfill(dst, x + s, y + s, x + w - 1 - s, y + h - 1 - s, makecol(255, 255, 255));
		blit(bmp, dst, 0, 0, x + (w - bmp->w) / 2, y + (h - bmp->h) / 2, bmp->w, bmp->h);
	}

	BITMAP* bmp;

	AnimTimer timer;

	enum STATE
	{
		ST_INACTIVE,
		ST_FADEIN,
		ST_FADEOUT,
		ST_DEAD
	};

	STATE state;

	float ap;
	float av;
};

class PopupsWidget : public GkWidget
{
public:
	virtual void DoUpdate(float dt)
	{
		int x = 0;

		//for (size_t i = 0; i < children.size(); ++i)
		for (int i = children.size() - 1; i >= 0; --i)
		{
			PopupWidget* p = (PopupWidget*)children[i];

			p->SetPos(x, 0);

			//printf("y: %d.\n", y);

			x += p->size.p[0] + 1;
		}
	}
};

//static CL_Popups cl_popups;
static GkContext* gk_ctx;

class CL_DrawStateACT
{
public:
	CL_DrawStateACT()
	{
		brush = new Brush;
		//smudge = new Smudge;
	}

	bool UpdateBrush(brush_desc_t& desc)
	{
		// TODO: Compare & update.
		if (desc != brush->desc)
		{
			printf("Recreating brush.\n");

			CreateBrush(brush, desc);

			return true;
		}

		return false;
	}

	bool UpdateSmudge(smudge_desc_t& desc)
	{
		/*
		// TODO: Compare & update.
		if (desc != smudge->desc)
		{
			printf("Recreating smudge.\n");

			CreateSmudge(smudge, desc);

			return true;
		}
		*/

		return false;
	}

	void CreateBrush(Brush* brush, brush_desc_t& desc)
	{
		brush->desc = desc;

		brush->Create(desc.size);

		const float half_size = brush->desc.size / 2.0f;
		const float half_size_inv = 1.0f / half_size;

		for (int iy = 0; iy < brush->desc.size; ++iy)
		{
			for (int ix = 0; ix < brush->desc.size; ++ix)
			{
				float x = ix + 0.5f;
				float y = iy + 0.5f;

				float delta[2];

				delta[0] = (x - half_size) * half_size_inv;
				delta[1] = (y - half_size) * half_size_inv;

				const float distanceSq =
					delta[0] * delta[0] +
					delta[1] * delta[1];

				const float distance = sqrt(distanceSq);

				float v;

				if (distance < desc.hardness)
					v = 1.0f;
				else
					v = 1.0f - (distance - desc.hardness) / (1.0f - desc.hardness);

				//float v = 1.0f - distance;

				v = MAX(0.0f, v);

				v *= cos(delta[0] * brush->desc.cos_freq[0]);
				v *= cos(delta[1] * brush->desc.cos_freq[1]);

				v = MAX(0.0f, v);

				sample_t* sample = brush->GetPix(ix, iy);

				*sample = v;
			}
		}
	}

	void CreateSmudge(Smudge* smudge, smudge_desc_t& desc)
	{
		smudge->Create(/*desc.size, */desc.strength);
	}

	Brush* brush;
	Smudge* smudge;

	BITMAP* brush_preview;
};

class CL_State
{
public:
	enum CONSTRAINT
	{
		CONSTRAIN_X,
		CONSTRAIN_Y,
		CONSTRAIN_RECT
	};

	CL_State(ChannelManager* cm)
	{
		this->cm = cm;

		channel = 0;

		bool isServer = cm->m_listenChannel != NULL;
		bool isClient = cm->m_listenChannel == NULL;
		disc = new Discover(isServer, isClient);

		buffer = 0;
		brush_preview = create_bitmap(100, 100);

		canvas = new Canvas;

		command_type = CMD_BRUSH;

		OnBrushChange = 0;

		travel = 0.0f;
		travel_step = 8.0f;

		color_hue = 0.0f;
		color_saturation = 1.0f;
		color_value = 1.0f;

		UpdateColor();

		// Set default brush.
		draw_state.brush_desc.size = 40;

		// Set default smudge.
		// TODO.

		// Set interaction state.
		drawing = false;
		mouse1 = false;
		mouse2 = false;

		UpdateACT();
	}

	void Connect(const std::string& hostname)
	{
		if (channel != 0)
		{
			channel->Disconnect();
			cm->DestroyChannel(channel);
			channel = 0;
		}

		printf("Connect: %s\n", hostname.c_str());

		hostent* host = gethostbyname(hostname.c_str());

		// fixme, move before Connect().
		channel = cm->CreateChannel(ChannelPool_Client);

		if (host == 0)
		{
			printf("Connect: Could not resolve host.\n");
			return;
		}

		in_addr* addr = (in_addr*)host->h_addr;
		NetAddress address(ntohl(addr->s_addr), SV_PORT);

		channel->Connect(address);
	}

	void Load(const std::string& filename)
	{
		canvas->Load(filename.c_str(), false);
	}

	void Save(const std::string& filename_bmp, const std::string filename_rpl)
	{
		printf("Saving %s (%s).\n", filename_bmp.c_str(), filename_rpl.c_str());

		if (save_bitmap(filename_bmp.c_str(), canvas->bmp, 0) < 0)
		{
			printf("Error: Could not save %s.\n", filename_bmp.c_str());
		}

		BitStream bitStream;

		size_t startIndex = 0;
		command_t::SerializeVector(bitStream, hist_commands, startIndex, -1);
		
#if 0
		File file;

		if (file.Open(filename_rpl.c_str()))
		{
			// todo : write bitstream contents to file
		}
		else
		{
			printf("Error: Could not save %s.\n", filename_rpl.c_str());
		}
#endif
	}

	void Save()
	{
		char filename_bmp[256];
		char filename_rpl[256];

		bool stop = false;

		for (int i = 0; !stop; ++i)
		{
			sprintf(filename_bmp, "save%04d.bmp", i);
			sprintf(filename_rpl, "save%04d.rpl", i);

			FILE* f = fopen(filename_bmp, "rb");

			if (!f)
				stop = true;
			else
				fclose(f);
		}

		Save(filename_bmp, filename_rpl);
	}

	void UpdateColor()
	{
		int r, g, b;

		hsv_to_rgb(color_hue, color_saturation, color_value, &r, &g, &b);

		draw_state.color[0] = ITOF(r);
		draw_state.color[1] = ITOF(g);
		draw_state.color[2] = ITOF(b);

		UpdateBrushPreview(draw_state_act.brush, draw_state.color, draw_state.opacity, brush_preview);
	}

	void UpdateACT(bool opacityChanged = false)
	{
		if (draw_state_act.UpdateBrush(draw_state.brush_desc) || opacityChanged)
		{
			UpdateBrushPreview(draw_state_act.brush, draw_state.color, draw_state.opacity, brush_preview);

			// FIXME: Make travel step settable?

			travel_step = MAX(1, Round(draw_state.brush_desc.size / 8.0f));
		}

		draw_state_act.UpdateSmudge(draw_state.smudge_desc);
	}

	void UpdateBrushPreview(Brush* brush, float* color, float opacity, BITMAP* bmp)
	{
		// TODO: Replace with brush draw method.
		// TODO: Add Validate with 1 channel to Canvas.

	#if 0
		clear_to_color(bmp, makecol(0, 0, 0));
	#endif

		Canvas temp;

		temp.Create(bmp->w, bmp->h, 3);
	#if 0
		sample_t bgcolor[3] = { 1.0f, 1.0f, 1.0f };
		temp.Clear(bgcolor);
	#else
		const sample_t bgcolor[2][3] =
		{
			{ .5f, .5f, .5f },
			{ .3f, .3f, .3f }
		};
		for (int y = 0; y < bmp->h; ++y)
			for (int x = 0; x < bmp->w; ++x)
				for (int i = 0; i < 3; ++i)
					temp.GetPix(x, y)[i] = bgcolor[((x >> 4) + (y >> 4)) & 1][i];
	#endif

		coord_t coord;

		coord.p[0] = bmp->w / 2;
		coord.p[1] = bmp->h / 2;

		for (float x = 0.f; x < bmp->w; x += travel_step)
		{
			coord.p[0] = x;

			PaintBrush(&temp, &temp, &coord, brush, color, opacity);
		}

		temp.Validate();

		blit(temp.bmp, bmp, 0, 0, 0, 0, temp.bmp->w, temp.bmp->h);

		if (OnBrushChange != 0)
			OnBrushChange(this);
	}

	/*
	static void handle_canvas_update(void* up, Canvas* canvas, const Rect& rect)
	{
		CL_State* state = (CL_State*)up;

		const int w = rect.x2 - rect.x1 + 1;
		const int h = rect.y2 - rect.y1 + 1;

		blit(canvas->bmp, state->buffer, rect.x1, rect.y1, rect.x1, rect.y1, w, h);

		// TODO: Delegate? Set invalidated flag?
		blit(state->buffer, screen, rect.x1, rect.y1, rect.x1, rect.y1, w, h);
	}
	*/

	// FIXME: Move down.
	bool drawing;
	bool mouse1;
	bool mouse2;

	static void HandleEvent(void* up, const Event& e)
	{
		CL_State* me = (CL_State*)up;

		PopupsWidget* popups = (PopupsWidget*)gk_ctx->Find("popups");

		switch (e.type)
		{
		case EV_KEYDOWN:
			switch (e.a1)
			{
			case KEY_Q:
				me->command_type = CMD_BRUSH;
				popups->Add(new PopupWidget("tool_brush.bmp"));
				printf("tool: brush\n");
				break;
			case KEY_W:
				me->command_type = CMD_BLUR;
				popups->Add(new PopupWidget("tool_blur.bmp"));
				printf("tool: blur\n");
				break;
			case KEY_E:
				me->command_type = CMD_SMUDGE;
				popups->Add(new PopupWidget("tool_smudge.bmp"));
				printf("tool: smudge\n");
				break;

			case KEY_S:
				me->Save();
				printf("save\n");
				break;

			case KEY_1:
				me->draw_state.blend_mode = BM_REPLACE;
				popups->Add(new PopupWidget("blend_replace.bmp"));
				printf("blend: replace\n");
				break;
			case KEY_2:
				me->draw_state.blend_mode = BM_ADD;
				popups->Add(new PopupWidget("blend_add.bmp"));
				printf("blend: add\n");
				break;
			case KEY_3:
				me->draw_state.blend_mode = BM_SUB;
				popups->Add(new PopupWidget("blend_subtract.bmp"));
				printf("blend: subtract\n");
				break;
			case KEY_4:
				me->draw_state.blend_mode = BM_BURN;
				popups->Add(new PopupWidget("blend_burn.bmp"));
				printf("blend: burn\n");
				break;
			case KEY_5:
				me->draw_state.blend_mode = BM_MULTIPLY;
				popups->Add(new PopupWidget("blend_multiply.bmp"));
				printf("blend: multiply\n");
				break;
			case KEY_6:
				me->draw_state.blend_mode = BM_DIFFERENCE;
				popups->Add(new PopupWidget("blend_difference.bmp"));
				printf("blend: difference\n");
				break;
			case KEY_7:
				me->draw_state.blend_mode = BM_SCREEN;
				popups->Add(new PopupWidget("blend_screen.bmp"));
				printf("blend: screen\n");
				break;
			case KEY_8:
				me->draw_state.blend_mode = BM_OVERLAY;
				popups->Add(new PopupWidget("blend_overlay.bmp"));
				printf("blend: overlay\n");
				break;
			case KEY_9:
				me->draw_state.blend_mode = BM_COLORIZE;
				popups->Add(new PopupWidget("blend_colorize.bmp"));
				printf("blend: colorize\n");
				break;

			case KEY_T:
				me->Console();
				break;

			case KEY_PLUS_PAD:
				{
					me->draw_state.brush_desc.size = MIN(100, me->draw_state.brush_desc.size + 1);
					me->UpdateACT();
					printf("brush size: %d\n", me->draw_state.brush_desc.size);
				}
				break;
			case KEY_MINUS_PAD:
				{
					me->draw_state.brush_desc.size = MAX(1, me->draw_state.brush_desc.size - 1);
					me->UpdateACT();
					printf("brush size: %d\n", me->draw_state.brush_desc.size);
				}
				break;
			}
			break;
		case EV_KEYUP:
			break;

		case EV_MOUSEDOWN:
			if (e.a1 == 0)
			{
				me->drawing = true;

				me->draw_state.coord.p[0] = e.a2;
				me->draw_state.coord.p[1] = e.a3;
			}
			if (e.a1 == 1)
				me->mouse1 = true;
			if (e.a1 == 2)
				me->mouse2 = true;
			break;
		case EV_MOUSEUP:
			if (e.a1 == 0)
			{
				me->drawing = false;
			}
			if (e.a1 == 1)
				me->mouse1 = false;
			if (e.a1 == 2)
				me->mouse2 = false;
			break;
		case EV_MOUSEMOVE:
			me->HandleMouseMove(e.a1, e.a2, e.a3, e.a4);
			break;
		}
	}

	void HandleMouseMove(int x, int y, int dx, int dy)
	{
		if (drawing)
		{
			coord_t temp;

			temp.p[0] = x;
			temp.p[1] = y;

			coord_t delta;

			delta.p[0] = temp.p[0] - draw_state.coord.p[0];
			delta.p[1] = temp.p[1] - draw_state.coord.p[1];

			if (delta.p[0] != 0.0f || delta.p[1] != 0.0f)
			{
				float distanceSq =
					delta.p[0] * delta.p[0] +
					delta.p[1] * delta.p[1];

				float distance = sqrt(distanceSq);

				travel = distance;

				delta.p[0] /= distance;
				delta.p[1] /= distance;
			}

			std::vector<command_t> commands;

			while (travel >= travel_step)
			//if (travel >= travel_step)
			{
				coord_t move;

				move.p[0] = delta.p[0] * travel_step;
				move.p[1] = delta.p[1] * travel_step;

				draw_state.coord.p[0] += move.p[0];
				draw_state.coord.p[1] += move.p[1];

				draw_state.direction.p[0] = move.p[0];
				draw_state.direction.p[1] = move.p[1];

				travel -= travel_step;

				command_t command;

				command.client_id = channel->m_id;

				if (command_type == CMD_BRUSH)
					command.CreateBrush(draw_state);
				if (command_type == CMD_BLUR)
					command.CreateBlur(draw_state);
				if (command_type == CMD_SMUDGE)
					command.CreateSmudge(draw_state);

				commands.push_back(command);
			}

			if (!commands.empty())
			{
			#ifdef DEBUG
				size_t sendSize = 0;
			#endif

				size_t startIndex = 0;

				while (startIndex != commands.size())
				{
					BitStream bitStream;

					const uint32_t count = command_t::SerializeVector(bitStream, commands, startIndex, 1024*8);

					const uint8_t protocolID = PROTOCOL_DRAW;
					const uint16_t size = bitStream.GetDataSize();

					PacketBuilder<2048> packetBuilder;
					packetBuilder.Write8(&protocolID);
					packetBuilder.Write16(&size);
					packetBuilder.Write16(&count);
					packetBuilder.Write(bitStream.GetData(), (bitStream.GetDataSize() + 7) >> 3);

					const Packet packet = packetBuilder.ToPacket();

					channel->Send(packet);

				#ifdef DEBUG
					sendSize += packet.GetSize();
				#endif
				}

			#ifdef DEBUG
				printf("send size: %u bytes, %u commands, %g bytes/command\n", (uint32_t)sendSize, (uint32_t)commands.size(), (float)sendSize / commands.size());
			#endif
			}
		}

		if (mouse1)
		{
			if (key[KEY_LSHIFT] || key[KEY_RSHIFT])
			{
				draw_state.brush_desc.cos_freq[0] += dx * 0.01f;
				draw_state.brush_desc.cos_freq[1] += dy * 0.01f;
				UpdateACT();
			}
			else
			{
				draw_state.opacity = MID(0.f, draw_state.opacity + dy / 200.f, 1.f);
				draw_state.brush_desc.size = MID(1, draw_state.brush_desc.size + dx, 100);
				UpdateACT(true);
			}
		}

		if (mouse2)
		{
			if (key[KEY_LSHIFT] || key[KEY_RSHIFT])
			{
				draw_state.brush_desc.hardness += dy * 0.01f;
				draw_state.brush_desc.hardness = Saturate(draw_state.brush_desc.hardness);
				UpdateACT();
			}
			else if (key[KEY_ALT])
			{
				color_value += dx * 0.01f;
				color_value = Saturate(color_value);

				UpdateColor();
			}
			else
			{
				color_hue += dy;
				color_saturation += dx * 0.01f;
				color_saturation = Saturate(color_saturation);

				UpdateColor();
			}
		}
	}

	void Console()
	{
		//printf("> ");

		std::vector<DiscoverServer> servers = disc->GetServers();

		for (size_t i = 0; i < servers.size(); ++i)
		{
			std::cout << "[" << i << "] server: " <<
				servers[i].m_address.ToString(true) <<
				" (" << servers[i].m_time << ")" <<
				std::endl;
		}

		//std::string str;

		//std::cin >> str;
		size_t i;

		std::cout << "server: ";
		std::cin >> i;

		if (i < 0 || i >= servers.size())
		{
			std::cout << "cancel" << std::endl;
			return;
		}

		std::string str = servers[i].m_address.ToString(false);

		Connect(str);
	}

	typedef void (*CbOnBrushChange)(CL_State* state);

	ChannelManager* cm;
	Channel* channel;
	Discover* disc;

	BITMAP* buffer;
	BITMAP* brush_preview;

	Canvas* canvas;

	COMMAND_TYPE command_type;

	float color_hue;
	float color_saturation;
	float color_value;

	float travel;
	float travel_step;

	CL_DrawState draw_state;
	CL_DrawStateACT draw_state_act;

	CbOnBrushChange OnBrushChange;

	// Command history.
	std::vector<command_t> hist_commands;
};

#if COMPILE_CLIENT

static CL_State* cl_state = 0;

#endif

class CL_Client : public Validatable
{
public:
	CL_Client(int client_id)
	{
		m_client_id = client_id;
	}

	int m_client_id;

	CL_DrawState m_draw_state;
	CL_DrawStateACT m_draw_state_act;

	// Validation stuff.
	bool invalidated;
	Rect invalidated_rect;
	//RectSet invalidated_rect_set;

	/*
	void Validate()
	{
		ValCommit();
	}
	*/

	virtual void Invalidate(const Rect& rect)
	{
		if (StartNewRect(rect))
			Commit();

		if (!invalidated)
		{
			invalidated_rect = rect;
			invalidated = true;
		}
		else
			invalidated_rect.Merge(rect);
	}

	bool StartNewRect(Rect rect)
	{
		if (!invalidated)
			return false;

		return false;

		Rect rect0 = rect;
		Rect rect1 = invalidated_rect;
		Rect rect2 = invalidated_rect;

		rect2.Merge(rect);

		int area0 = rect0.Area();
		int area1 = rect1.Area();
		int area2 = rect2.Area();

#if 1
		if (area2 - area1 > area0)
			return true;
		else
			return false;
#else
		if (area2 - area1 > 20 * 20)
			return true;
		else
			return false;
#endif

		// TODO: Compare distance to current rect.

		//return true;
		//return false;
	}

	void Commit()
	{
		if (!invalidated)
			return;

		cl_state->canvas->Invalidate(invalidated_rect);

		invalidated = false;
	}
};

std::vector<CL_Client*> cl_clients;

static CL_Client* CL_FindClient(int client_id)
{
	for (size_t i = 0; i < cl_clients.size(); ++i)
		if (cl_clients[i]->m_client_id == client_id)
			return cl_clients[i];

	return 0;
}

std::vector<Channel*> sv_clients;

class MyChannelHandler : public ChannelHandler
{
public:
	MyChannelHandler() : ChannelHandler()
	{
	}

	virtual void SV_OnChannelConnect(Channel* channel)
	{
		printf("channel connect.\n");

		sv_clients.push_back(channel);
	}

	virtual void SV_OnChannelDisconnect(Channel* channel)
	{
		printf("channel disconnect.\n");

		sv_clients.erase(std::find(sv_clients.begin(), sv_clients.end(), channel));
	}

	virtual void CL_OnChannelConnect(Channel * channel)
	{
	}

	virtual void CL_OnChannelDisconnect(Channel * channel)
	{
	}
};

std::vector<command_t> recv_commands;

class MyPacketListener : public PacketListener
{
public:
	virtual void OnReceive(Packet& packet, Channel* channel)
	{
		if (channel->m_channelPool == ChannelPool_Client)
		{
			//printf("client command.\n");

			uint16_t size;
			uint16_t count;
			packet.Read16(&size);
			packet.Read16(&count);

			Packet commandPacket;
			packet.Extract(commandPacket, (size + 7) >> 3);
			packet.Skip((size + 7) >> 3);

			BitStream bitStream(commandPacket.GetData(), size);

			std::vector<command_t> commands;

			command_t::DeserializeVector(bitStream, commands, count);

			recv_commands.insert(recv_commands.end(), commands.begin(), commands.end());
		}
		else
		{
			//printf("server command.\n");

			std::vector<command_t> commands;

			{
				uint16_t size;
				uint16_t count;
				packet.Read16(&size);
				packet.Read16(&count);

				Packet commandPacket;
				packet.Extract(commandPacket, (size + 7) >> 3);
				packet.Skip((size + 7) >> 3);

				BitStream bitStream(commandPacket.GetData(), size);

				command_t::DeserializeVector(bitStream, commands, count);

				for (size_t i = 0; i < commands.size(); ++i)
					commands[i].client_id = channel->m_id;
			}

			// broadcast packet to all clients

			size_t startIndex = 0;

			while (startIndex != commands.size())
			{
				BitStream bitStream;

				const uint16_t count = command_t::SerializeVector(bitStream, commands, startIndex, 1024*8);

				const uint8_t protocolID = PROTOCOL_DRAW;
				const uint16_t size = bitStream.GetDataSize();

				PacketBuilder<2048> packetBuilder;
				packetBuilder.Write8(&protocolID);
				packetBuilder.Write16(&size);
				packetBuilder.Write16(&count);
				packetBuilder.Write(bitStream.GetData(), (bitStream.GetDataSize() + 7) >> 3);

				const Packet broadcastPacket = packetBuilder.ToPacket();

				for (size_t i = 0; i < sv_clients.size(); ++i)
				{
					sv_clients[i]->Send(broadcastPacket);
				}
			}
		}
	}
};

static void ProcessCommand(CL_Client* client, command_t& command)
{
	client->m_draw_state = command.draw_state;

	// FIXME: UpdateACT..
	client->m_draw_state_act.UpdateBrush(client->m_draw_state.brush_desc);
	client->m_draw_state_act.UpdateSmudge(client->m_draw_state.smudge_desc);

	cl_state->canvas->blend_mode = (BLEND_MODE)client->m_draw_state.blend_mode;

	//cl_state->canvas->SetUpdateCB(CL_State::handle_canvas_update, cl_state);

	if (command.type == CMD_BRUSH)
		PaintBrush(client, cl_state->canvas, &client->m_draw_state.coord, client->m_draw_state_act.brush, client->m_draw_state.color, client->m_draw_state.opacity);
	if (command.type == CMD_BLUR)
		PaintBlur(client, cl_state->canvas, &client->m_draw_state.coord, client->m_draw_state_act.brush, 1); // TODO: Make blur size settable.
	if (command.type == CMD_SMUDGE)
		PaintSmudge(client, cl_state->canvas, &client->m_draw_state.coord, client->m_draw_state_act.brush, &client->m_draw_state.direction, client->m_draw_state.smudge_desc.strength);

	//cl_state->canvas->SetUpdateCB(0, 0);

	cl_state->hist_commands.push_back(command);
}

static void ProcessCommands()
{
	// Process incoming commands.
	{
		for (size_t i = 0; i < recv_commands.size(); ++i)
		{
			command_t& command = recv_commands[i];

			CL_Client* client = CL_FindClient(command.client_id);

			if (client == 0)
			{
				client = new CL_Client(command.client_id);
				cl_clients.push_back(client);
			}

			ProcessCommand(client, command);
		}

		recv_commands.clear();
	}

	for (size_t i = 0; i < cl_clients.size(); ++i)
	{
		cl_clients[i]->Commit();
	}
}


void PerformHistory()
{
	std::vector<command_t> commands = cl_state->hist_commands;

	for (size_t i = 0; i < commands.size(); ++i)
	{
		command_t& command = commands[i];

		CL_Client* client = CL_FindClient(command.client_id);

		if (client == 0)
		{
			client = new CL_Client(command.client_id);
			cl_clients.push_back(client);
		}

		ProcessCommand(client, command);
	}
}

class CanvasWidget : public GkWidget
{
public:
	CanvasWidget(CL_State* state)
	{
		this->state = state;

		this->state->canvas->SetUpdateCB(HandleUpdate, this);
	}

	virtual void DoPaint()
	{
		blit(state->canvas->bmp, gk_buf, 0, 0, pos.p[0], pos.p[1], size.p[0], size.p[1]);
	}

	virtual void HandleEvent(const Event& e)
	{
		//printf("event!!\n");

		state->HandleEvent(state, e);
	}

	static void HandleUpdate(void* up, Canvas* canvas, const Rect& rect)
	{
		CanvasWidget* me = (CanvasWidget*)up;

		me->Invalidate(rect);
	}

	CL_State* state;
};

class BrushPreviewWidget : public GkWidget
{
public:
	// TODO: BrushPreviewWidget should be owner of brush preview?
	BrushPreviewWidget(CL_State* state)
	{
		m_state = state;
	}

	virtual void DoPaint()
	{
		blit(m_state->brush_preview, gk_buf, 0, 0, pos.p[0], pos.p[1], size.p[0], size.p[1]);
	}

	void HandleChange()
	{
		Invalidate();
	}

private:
	CL_State* m_state;
};

static void HandleBrushChange(CL_State* state)
{
	BrushPreviewWidget* preview = (BrushPreviewWidget*)gk_ctx->Find("preview");

	preview->HandleChange();
}

int main(int argc, char* argv[])
{
	// Server or client?

	bool run_server = false;

	printf("S: run server (with built-in client)\n");
	printf("press any other key to run as client\n");

	char c;

#ifdef WIN32
	c = _getche();
#else
	std::cin >> c;
#endif

	if (c == 's')
		run_server = true;

	// Shared init.

	allegro_init();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 480, 0, 0);
	set_display_switch_mode(SWITCH_BACKGROUND);
	set_window_title("NetPaint");
	install_keyboard();
	install_mouse();
	show_os_cursor(1);

#if 0
	ilInit();
#endif

	// Shared init.

	Timer timer;

	PacketDispatcher packetDispatcher;

	MyChannelHandler* ch = new MyChannelHandler();
	ChannelManager* cm = new ChannelManager();
	MyPacketListener* pl = new MyPacketListener();
	packetDispatcher.RegisterProtocol(PROTOCOL_CHANNEL, cm);
	packetDispatcher.RegisterProtocol(PROTOCOL_DRAW, pl);

	cm->Initialize(&packetDispatcher, ch, run_server ? SV_PORT : 0, run_server);

	// Server init.

	Channel* sv_channel = NULL;
	
	if (run_server)
	{
		sv_channel = cm->CreateListenChannel(false);
	}

	// Shared init.

	EventMgr em;

	// Client init.

#if COMPILE_CLIENT

	cl_state = new CL_State(cm);

	//em.handlers.push_back(EventHandler(cl_state->HandleEvent, cl_state));

	cl_state->Connect("127.0.0.1");

	printf("L mouse button: Draw\n");
	printf("R mouse button: Resize brush\n");
	printf("M mouse button: Change color\n");
	printf("             Q: Select brush tool\n");
	printf("             W: Select blur tool\n");
	printf("             E: Select smudge tool\n");
	printf("          1..9: Select blend mode\n");

	std::string filename;

	if (argc >= 2)
		filename = argv[1];
	else
		filename = "back.bmp";
	
	cl_state->Load(filename);
	//set_gfx_mode(GFX_AUTODETECT_WINDOWED, cl_state->canvas->w, cl_state->canvas->h, 0, 0); // FIXME

	cl_state->buffer = create_bitmap(cl_state->canvas->w, cl_state->canvas->h);

	cl_state->draw_state.coord.p[0] = mouse_x;
	cl_state->draw_state.coord.p[1] = mouse_y;

	cl_state->UpdateACT();

#endif

#if COMPILE_CLIENT

	gk_ctx = new GkContext;

	em.handlers.push_back(EventHandler(gk_ctx->HandleEventST, gk_ctx));

	{
		CanvasWidget* canvas = new CanvasWidget(cl_state);

		gk_ctx->root->Add(canvas);

		canvas->SetName("canvas");

		canvas->SetSize(cl_state->canvas->w, cl_state->canvas->h);
		//canvas->SetPos(10, 10);
	}

	{
		PopupsWidget* popups = new PopupsWidget();

		gk_ctx->root->Add(popups);

		popups->SetName("popups");
	}

	{
		BrushPreviewWidget* preview = new BrushPreviewWidget(cl_state);

		gk_ctx->root->Add(preview);

		preview->SetName("preview");

		preview->SetSize(100, 100);
		preview->SetPos(SCREEN_W - 100, 0);

		cl_state->OnBrushChange = HandleBrushChange;
	}

#endif

	static bool mouse[3];

	while (!key[KEY_ESC])
	{
		cm->Update(timer.TimeMS_get());

		em.Poll();

		em.Update();

		cm->Update(timer.TimeMS_get());

		cl_state->disc->Update(); // fixme

#if COMPILE_CLIENT

		ProcessCommands();

#endif

#if COMPILE_CLIENT

		if (key[KEY_H])
		{
			PerformHistory();
		}

#endif

#if COMPILE_CLIENT

		// FIXME: Create Paint method. Call on Validate and BrushPreview change.

		// Validate canvas.
		acquire_bitmap(screen);

		cl_state->canvas->Validate();

		// FIXME: Update brush preview on change..
		//blit(cl_state->brush_preview, screen, 0, 0, SCREEN_W - 1 - cl_state->brush_preview->w, 0, cl_state->brush_preview->w, cl_state->brush_preview->h);

		gk_ctx->Update(0.01f);

		gk_ctx->Paint();

		release_bitmap(screen);

	#if 1
		Sleep(2);
	#else
		yield_timeslice();
	#endif

#endif

#if 0
		{
			int x1 = rand() % SCREEN_W;
			int y1 = rand() % SCREEN_H;
			int x2 = rand() % SCREEN_W;
			int y2 = rand() % SCREEN_H;

			aa_line(screen, x1 << 16, y1 << 16, x2 << 16, y2 << 16, makecol(255, 127, 63));
		}
#endif

	}

#if COMPILE_CLIENT

	delete gk_ctx;

	cl_state->channel->Disconnect();

	delete cl_state;

#endif

	cm->Shutdown(true);

	return 0;
}
END_OF_MAIN()
