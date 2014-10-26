#include <exception>
#include <map>
#include <SDL/SDL.h>
#include "BitStream.h"
#include "BinaryDiff.h"
#include "ChannelHandler.h"
#include "ChannelManager.h"
#include "Log.h"
#include "NetArray.h"
#include "NetDiag.h"
#include "NetProtocols.h"
#include "NetSerializable.h"
#include "NetStats.h"
#include "Packet.h"
#include "PacketDispatcher.h"
#include "PolledTimer.h"
#include "RpcManager.h"
#include "SDL_Bitmap.h"
#include "Timer.h"

static void TestBitStream();
static void TestRpc();
static void TestSerializableObject();
static void TestGameUpdate();
static void TestNetArray();

enum TestProtocol
{
	TestProtocol_RT = PROTOCOL_CUSTOM,
	TestProtocol_ClientState
};

class StateVector
{
public:
	StateVector()
		: m_allocIdx(0)
	{
		memset(m_bytes, 0, sizeof(m_bytes));
	}

	template <typename T>
	T & Alloc()
	{
		NetAssert(m_allocIdx + sizeof(T) <= kSize);
		T & v = reinterpret_cast<T &>(m_bytes[m_allocIdx]);
		m_allocIdx += sizeof(T);
		return v;
	}

	const static uint32_t kSize = 2048;

	uint32_t m_allocIdx;
	uint8_t m_bytes[kSize];
};

template <typename T>
class svValue
{
	T & m_storage;

public:
	svValue(StateVector & v, const T & init = T())
		: m_storage(v.Alloc<T>())
	{
		m_storage = init;
	}

	operator T()
	{
		return m_storage;
	}

	operator T() const
	{
		return m_storage;
	}

	T & operator=(const T & other)
	{
		m_storage = other;
		
		return m_storage;
	}

	void operator+=(const T & other)
	{
		m_storage += other;
	}

	void operator-=(const T & other)
	{
		m_storage -= other;
	}

	void operator*=(const T & other)
	{
		m_storage *= other;
	}

	void operator/=(const T & other)
	{
		m_storage /= other;
	}

	void operator&=(const T & other)
	{
		m_storage &= other;
	}

	void operator^=(const T & other)
	{
		m_storage ^= other;
	}
};

typedef svValue<int8_t>   svInt8;
typedef svValue<uint8_t>  svUint8;
typedef svValue<int16_t>  svInt16;
typedef svValue<uint16_t> svUint16;
typedef svValue<int32_t>  svInt32;
typedef svValue<uint32_t> svUint32;

const static int kWorldSize = 256;

class Player
{
public:
	Player(StateVector & stateVector)
		: m_posX(stateVector)
		, m_posY(stateVector)
		, m_velX(stateVector)
		, m_velY(stateVector)
	{
		m_posX = 0;
		m_posY = 0;
		m_velX = 0;
		m_velY = 0;
	}

	void Update(float dt)
	{
		m_posX += m_velX;
		m_posY += m_velY;

		if (m_posX < 0)
			m_posX = 0;
		if (m_posY < 0)
			m_posY = 0;
		if (m_posX + kSize > kWorldSize)
			m_posX = kWorldSize - kSize;
		if (m_posY + kSize > kWorldSize)
			m_posY = kWorldSize - kSize;
	}

	void Draw(SDL_Surface * surface)
	{
		SDL_Bitmap bitmap(
			surface,
			m_posX,
			m_posY,
			kSize,
			kSize);

		const uint32_t color = bitmap.Color(0.f, 0.f, 1.f);

		bitmap.Clear(color);
	}

	const static int kSize = 16;

	svInt16 m_posX;
	svInt16 m_posY;
	svInt16 m_velX;
	svInt16 m_velY;
};

class Block
{
public:
	Block(StateVector & stateVector)
		: m_posX(stateVector)
		, m_posY(stateVector)
		, m_extX(stateVector)
		, m_extY(stateVector)
		, m_type(stateVector)
	{
		Randomize();
	}

	void Randomize()
	{
		m_posX = rand() % (kWorldSize - kSize);
		m_posY = rand() % (kWorldSize - kSize);
		m_extX = kSize;
		m_extY = kSize;
		m_type = rand() % 256;
	}

	const static int kSize = 16;

	svUint16 m_posX;
	svUint16 m_posY;
	svUint16 m_extX;
	svUint16 m_extY;
	svUint8  m_type;
};

class Door
{
public:
	Door(StateVector & stateVector)
		: m_posX(stateVector)
		, m_posY(stateVector)
		, m_state(stateVector, State_Closed)
		, m_oldState(State_Closed)
		, m_animProgress(1.f)
	{
		m_posX = rand() % kWorldSize;
		m_posY = rand() % kWorldSize;
	}

	void Update(float dt)
	{
		if (m_state != m_oldState)
		{
			m_oldState = m_state;
			m_animProgress = 0.f;
		}
		else
		{
			m_animProgress += dt;

			if (m_animProgress > 1.f)
				m_animProgress = 1.f;
		}
	}

	void Draw(SDL_Surface * surface)
	{
		SDL_Bitmap bitmap(
			surface,
			m_posX,
			m_posY,
			kSize,
			kSize);

		const float colorOpen[3] = { 0.f, 1.f, 0.f };
		const float colorClosed[3] = { 1.f, 0.f, 0.f };

		const float * color1;
		const float * color2;

		if (m_state == State_Open)
		{
			color1 = colorClosed;
			color2 = colorOpen;
		}
		else
		{
			color1 = colorOpen;
			color2 = colorClosed;
		}

		const float r = color1[0] * (1.f - m_animProgress) + color2[0] * m_animProgress;
		const float g = color1[1] * (1.f - m_animProgress) + color2[1] * m_animProgress;
		const float b = color1[2] * (1.f - m_animProgress) + color2[2] * m_animProgress;

		const uint32_t color = bitmap.Color(r, g, b);

		bitmap.RectFill(
			0,
			0,
			Door::kSize,
			Door::kSize,
			color);
	}

	void Interact()
	{
		if (m_animProgress == 1.f)
		{
			m_animProgress = 0.f;

			if (m_state == State_Open)
			{
				m_state = State_Closed;
			}
			else
			{
				m_state = State_Open;
			}
		}
	}

	enum State
	{
		State_Closed,
		State_Open
	};

	const static int kSize = 16;

	svUint16       m_posX;
	svUint16       m_posY;
	svValue<State> m_state;

	State m_oldState;
	float m_animProgress;
};

class Map
{
public:
	Map(StateVector & stateVector)
	{
		for (uint32_t i = 0; i < kMaxBlocks; ++i)
			m_blocks[i] = new Block(stateVector);

		for (uint32_t i = 0; i < kMaxDoors; ++i)
			m_doors[i] = new Door(stateVector);
	}

	~Map()
	{
		for (uint32_t i = 0; i < kMaxBlocks; ++i)
		{
			delete m_blocks[i];
			m_blocks[i] = 0;
		}

		for (uint32_t i = 0; i < kMaxDoors; ++i)
		{
			delete m_doors[i];
			m_doors[i] = 0;
		}
	}

	void Draw(SDL_Surface * surface)
	{
		SDL_Bitmap bitmap(
			surface,
			0,
			0,
			kWorldSize,
			kWorldSize);

		for (int i = 0; i < kMaxBlocks; ++i)
		{
			Block * block = m_blocks[i];
			
			const uint32_t color = bitmap.Color(1.f, 1.f, block->m_type / 255.f);

			bitmap.RectFill(
				block->m_posX,
				block->m_posY,
				block->m_extX,
				block->m_extY,
				color);
		}

		for (int i = 0; i < kMaxDoors; ++i)
		{
			Door * door = m_doors[i];

			door->Draw(surface);
		}
	}

	const static uint32_t kMaxBlocks = 64;
	const static uint32_t kMaxDoors = 8;

	Block * m_blocks[kMaxBlocks];
	Door * m_doors[kMaxDoors];
};

class GameState
{
public:
	GameState(StateVector & stateVector)
	{
		m_map = new Map(stateVector);

		for (int i = 0; i < kMaxPlayers; ++i)
			m_players[i] = new Player(stateVector);
	}

	~GameState()
	{
		for (int i = 0; i < kMaxPlayers; ++i)
		{
			delete m_players[i];
			m_players[i] = 0;
		}

		delete m_map;
		m_map = 0;
	}

	const static int kMaxPlayers = 4;

	Map * m_map;
	Player * m_players[kMaxPlayers];
};

static void UpdatePlayerCollision(Map * map, Player * player, bool isAuthorative, bool isHost)
{
	for (int i = 0; i < Map::kMaxBlocks; ++i)
	{
		Block * block = map->m_blocks[i];

		bool collision =
			(player->m_posX + Player::kSize) > block->m_posX &&
			(player->m_posY + Player::kSize) > block->m_posY &&
			player->m_posX < (block->m_posX + block->m_extX) &&
			player->m_posY < (block->m_posY + block->m_extY);

		if (collision)
		{
			if (block->m_type < 128)
			{
				if (isHost)
				{
					block->Randomize();
				}
			}
			else if (isAuthorative)
			{
				const int dx1 = block->m_posX - (player->m_posX + Player::kSize);
				const int dy1 = block->m_posY - (player->m_posY + Player::kSize);
				const int dx2 = (block->m_posX + block->m_extX) - player->m_posX;
				const int dy2 = (block->m_posY + block->m_extY) - player->m_posY;
				const int dx = abs(dx1) < abs(dx2) ? dx1 : dx2;
				const int dy = abs(dy1) < abs(dy2) ? dy1 : dy2;

				if (abs(dx) < abs(dy))
					player->m_posX += dx;
				else
					player->m_posY += dy;
			}
		}
	}	
}

class GameStateData
{
public:
	GameStateData()
	{
		m_gameState = new GameState(m_stateVector);
	}

	~GameStateData()
	{
		delete m_gameState;
		m_gameState = 0;
	}

	BinaryDiffResult GetDiff(const StateVector & other, uint32_t skipTreshold)
	{
		return BinaryDiff(
			m_stateVector.m_bytes,
			other.m_bytes,
			StateVector::kSize,
			skipTreshold);
	}

	GameState * m_gameState;
	StateVector m_stateVector;
};

//

class ClientState
{
public:
	ClientState(Channel * channel, uint32_t clientIdx)
		: m_clientIdx(clientIdx)
		, m_channel(channel)
		, m_value(0)
	{
	}

	void Draw(SDL_Surface * surface)
	{
		uint32_t n = 16;
		uint32_t x1 = m_clientIdx % n;
		uint32_t y1 = m_clientIdx / n;
		uint32_t s = surface->w / n;

		SDL_Bitmap bitmap(
			surface,
			x1 * s,
			y1 * s,
			s,
			s);

		uint32_t backColor = bitmap.Color(0.5f, 0.0f, 0.0f);
		bitmap.Clear(backColor);

		uint32_t foreColor = bitmap.Color(0.8f, 0.8f, 0.8f);
		uint32_t v = m_value;
		uint32_t x = v % s;
		uint32_t y = (v / s) % s;
		bitmap.RectFill(x, y, 1, 1, foreColor);
	}

	uint32_t m_clientIdx;
	Channel * m_channel;
	uint32_t m_value;
};

class MyChannelHandler : public ChannelHandler, public PacketListener
{
public:
	typedef std::map<uint32_t, Channel*> ChannelMap;

	MyChannelHandler()
		: m_clientIdx(0)
	{
		PacketDispatcher::RegisterProtocol(TestProtocol_RT, this);
	}

	virtual ~MyChannelHandler()
	{
	}

	void CreateClientState(Channel * channel)
	{
		ClientState * clientState = new ClientState(channel, m_clientIdx);
		m_clientStates[channel->m_id] = clientState;
		m_clientIdx++;
	}

	void DestroyClientState(Channel * channel)
	{
		std::map<uint32_t, ClientState *>::iterator j = m_clientStates.find(channel->m_id);

		if (j != m_clientStates.end())
		{
			ClientState * clientState = j->second;

			delete clientState;
			clientState = 0;

			m_clientStates.erase(j);
		}

		if (m_clientStates.empty())
		{
			m_clientIdx = 0;
		}
	}

	virtual void OnReceive(Packet & packet, Channel * channel)
	{
		//LOG_DBG("received packet", 0);

		uint16_t value;

		if (packet.Read16(&value))
		{
			//LOG_DBG("value: %05u", value);

			std::map<uint32_t, ClientState *>::iterator i = m_clientStates.find(channel->m_id);

			if (i != m_clientStates.end())
			{
				ClientState * clientState = i->second;

				clientState->m_value = value;
			}
			else
			{
				LOG_ERR("client state not found", 0);
			}
		}
		else
		{
			LOG_ERR("read error", value);
		}
	}

	virtual void SV_OnChannelConnect(Channel * channel)
	{
		LOG_INF("SV channel connect: %u", channel->m_id);

		NetAssert(m_svChannels.find(channel->m_id) == m_svChannels.end());

		m_svChannels[channel->m_id] = channel;

		//

		CreateClientState(channel);
	}
	
	virtual void SV_OnChannelDisconnect(Channel * channel)
	{
		LOG_INF("SV channel disconnect: %u", channel->m_id);

		ChannelMap::iterator i = m_svChannels.find(channel->m_id);

		if (i != m_svChannels.end())
		{
			m_svChannels.erase(i);
		}
		else
		{
			LOG_WRN("channel disconnect: channel does not exist: %u", channel->m_id);
		}

		//

		DestroyClientState(channel);
	}

	virtual void CL_OnChannelConnect(Channel * channel)
	{
		LOG_INF("CL channel connect: %u", channel->m_id);

		NetAssert(m_clChannels.find(channel->m_id) == m_clChannels.end());

		m_clChannels[channel->m_id] = channel;

		//

		CreateClientState(channel);
	}
	
	virtual void CL_OnChannelDisconnect(Channel * channel)
	{
		LOG_INF("CL channel disconnect: %u", channel->m_id);

		ChannelMap::iterator i = m_clChannels.find(channel->m_id);

		if (i != m_clChannels.end())
		{
			m_clChannels.erase(i);
		}
		else
		{
			LOG_WRN("channel disconnect: channel does not exist: %u", channel->m_id);
		}

		//

		DestroyClientState(channel);
	}

	Channel * PickRandomChannel(ChannelMap & channels)
	{
		if (channels.size() == 0)
			return 0;

		uint32_t index = rand() % channels.size();

		for (ChannelMap::iterator i = channels.begin(); /* true */; ++i, --index)
		{
			if (index == 0)
				return i->second;
		}
	}

	void Disconnect(ChannelManager * channelMgr, Channel * channel)
	{
		if (channel == 0)
		{
			LOG_INF("no channels left to remove", 0);
		}
		else
		{
			channel->Disconnect();
			channelMgr->DestroyChannel(channel);
		}
	}

	void DisconnectAll(ChannelManager * channelMgr, ChannelMap & channels)
	{
		if (channels.size() == 0)
		{
			LOG_INF("no channels left to remove", 0);
		}
		else
		{
			while (channels.size() != 0)
			{
				Channel * channel = channels.begin()->second;
				Disconnect(channelMgr, channel);
			}
		}
	}

	void CL_DisconnectRandom(ChannelManager * channelMgr)
	{
		Channel * channel = PickRandomChannel(m_clChannels);
		Disconnect(channelMgr, channel);
	}

	void CL_DisconnectAll(ChannelManager * channelMgr)
	{
		DisconnectAll(channelMgr, m_clChannels);
	}

	void SV_DisconnectRandom(ChannelManager * channelMgr)
	{
		Channel * channel = PickRandomChannel(m_svChannels);
		Disconnect(channelMgr, channel);
	}

	void SV_DisconnectAll(ChannelManager * channelMgr)
	{
		DisconnectAll(channelMgr, m_svChannels);
	}

	void ShowChannelList(ChannelMap & channels, const char * name)
	{
		LOG_INF("%s:", name);
		if (channels.empty())
		{
			LOG_INF("(empty list)", 0);
		}
		else
		{
			for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				Channel * channel = i->second;
				LOG_INF("[%09u] => [%09u]",
					channel->m_id,
					channel->m_destinationId);
			}
		}
	}

	void Show()
	{
		ShowChannelList(m_svChannels, "server channels");
		ShowChannelList(m_clChannels, "client channels");
		NetStats::Show();
	}

	void Send(ChannelMap & channels)
	{
		for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			Channel * channel = i->second;

			PacketBuilder<3> packetBuilder;

			const uint8_t protocolId = TestProtocol_RT;
			const uint16_t value = rand();

			packetBuilder.Write8(&protocolId);
			packetBuilder.Write16(&value);

			const Packet packet = packetBuilder.ToPacket();

			channel->Send(packet);

			//

			std::map<uint32_t, ClientState *>::iterator j = m_clientStates.find(channel->m_id);

			if (j != m_clientStates.end())
			{
				ClientState * clientState = j->second;

				clientState->m_value = value;
			}
		}
	}

	void CL_Send()
	{
		Send(m_clChannels);
	}

	void SV_Send()
	{
		Send(m_svChannels);
	}

	void SendRT(ChannelMap & channels)
	{
		for (ChannelMap::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			Channel * channel = i->second;

			PacketBuilder<3> packetBuilder;

			const uint8_t protocolId = TestProtocol_RT;
			const uint16_t value = rand();

			packetBuilder.Write8(&protocolId);
			packetBuilder.Write16(&value);

			const Packet packet = packetBuilder.ToPacket();

			channel->SendReliable(packet);

			//

			std::map<uint32_t, ClientState *>::iterator j = m_clientStates.find(channel->m_id);

			if (j != m_clientStates.end())
			{
				ClientState * clientState = j->second;

				clientState->m_value = value;
			}
		}
	}

	void CL_SendRT()
	{
		SendRT(m_clChannels);
	}

	void SV_SendRT()
	{
		SendRT(m_svChannels);
	}

	ChannelMap m_svChannels;
	ChannelMap m_clChannels;
	uint32_t m_clientIdx;
	std::map<uint32_t, ClientState *> m_clientStates;
};

static void TestGameUpdate(SDL_Surface * surface)
{
	printf("LEFT/RIGHT/UP/DOWN: move player\n");
	printf("               ESC: end test\n");

	GameStateData gameDataSV;  // server side game state
	StateVector clientState;   // server side view of the client state

	GameStateData gameDataCL;  // client side game state
	StateVector serverState;   // client side view of the server state

	BinaryDiffPackage server2clientPackage;
	BinaryDiffPackage client2serverPackage;
	
	// initial 'send' of the game state vector to the client

	clientState = gameDataSV.m_stateVector;

	gameDataCL.m_stateVector = gameDataSV.m_stateVector;
	serverState = gameDataSV.m_stateVector;

	//

	int moveX1 = 0;
	int moveY1 = 0;
	int moveX2 = 0;
	int moveY2 = 0;
	bool interact = false;
	bool stop = false;
	while (stop == false)
	{
		SDL_Delay(10);

		// apply incoming deltas from client (server should be in sync with the client, since we're running in lock-step)

		ApplyBinaryDiffPackage(clientState.m_bytes, StateVector::kSize, client2serverPackage);
		NetAssert(!memcmp(clientState.m_bytes, gameDataCL.m_stateVector.m_bytes, StateVector::kSize));

		ApplyBinaryDiffPackage(gameDataSV.m_stateVector.m_bytes, StateVector::kSize, client2serverPackage);
		NetAssert(!memcmp(gameDataSV.m_stateVector.m_bytes, gameDataCL.m_stateVector.m_bytes, StateVector::kSize));

		// update server

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (SDL_EVENTMASK(e.type) & SDL_KEYEVENTMASK)
			{
				if (e.key.keysym.sym == SDLK_LEFT)
					moveX1 = e.key.state ? -1 : 0;
				if (e.key.keysym.sym == SDLK_RIGHT)
					moveX2 = e.key.state ? +1 : 0;
				if (e.key.keysym.sym == SDLK_UP)
					moveY1 = e.key.state ? -1 : 0;
				if (e.key.keysym.sym == SDLK_DOWN)
					moveY2 = e.key.state ? +1 : 0;
				if (e.key.keysym.sym == SDLK_SPACE)
					interact = e.key.state ? true : false;
				if (e.key.keysym.sym == SDLK_ESCAPE && e.key.state)
					stop = true;
			}
		}

		const float dt = 1.f / 60.f;

		{
			Player * player = gameDataSV.m_gameState->m_players[0];
			player->m_velX = moveX1 + moveX2;
			player->m_velY = moveY1 + moveY2;
			player->Update(dt);
		}

		for (int p = 0; p < GameState::kMaxPlayers; ++p)
		{
			Player * player = gameDataSV.m_gameState->m_players[p];

			UpdatePlayerCollision(gameDataSV.m_gameState->m_map, player, (p == 0), true);
		}

		for (int i = 0; i < Map::kMaxDoors; ++i)
		{
			Door * door = gameDataSV.m_gameState->m_map->m_doors[i];

			door->Update(dt);
		}

		for (int p = 0; p < GameState::kMaxPlayers; ++p)
		{
			Player * player = gameDataSV.m_gameState->m_players[p];

			for (int i = 0; i < Map::kMaxDoors; ++i)
			{
				Door * door = gameDataSV.m_gameState->m_map->m_doors[i];

				bool collision =
					(player->m_posX + Player::kSize) > door->m_posX &&
					(player->m_posY + Player::kSize) > door->m_posY &&
					player->m_posX < (door->m_posX + Door::kSize) &&
					player->m_posY < (door->m_posY + Door::kSize);

				if (collision && interact)
				{
					door->Interact();
				}
			}
		}

		// calculate diff to send to client

		{
			const BinaryDiffResult server2client = gameDataSV.GetDiff(clientState, 4);
			server2clientPackage = MakeBinaryDiffPackage(gameDataSV.m_stateVector.m_bytes, StateVector::kSize, server2client);

			if (server2client.m_diffBytes != 0)
				LOG_INF("SV diff bytes: %u bytes", server2client.m_diffBytes);
		}

		ApplyBinaryDiffPackage(clientState.m_bytes, StateVector::kSize, server2clientPackage);
		NetAssert(!memcmp(clientState.m_bytes, gameDataSV.m_stateVector.m_bytes, StateVector::kSize));

		// apply incoming deltas from server (client should be in sync with the server, since we're running in lock-step)

		ApplyBinaryDiffPackage(serverState.m_bytes, StateVector::kSize, server2clientPackage);
		NetAssert(!memcmp(serverState.m_bytes, gameDataSV.m_stateVector.m_bytes, StateVector::kSize));

		ApplyBinaryDiffPackage(gameDataCL.m_stateVector.m_bytes, StateVector::kSize, server2clientPackage);
		NetAssert(!memcmp(gameDataCL.m_stateVector.m_bytes, gameDataSV.m_stateVector.m_bytes, StateVector::kSize));

		// update client

		{
			Player * player = gameDataCL.m_gameState->m_players[1];

			player->m_velX = - moveX1 - moveX2;
			player->m_velY = - moveY1 - moveY2;
			player->Update(dt);
		}

		{
			Player * player = gameDataCL.m_gameState->m_players[2];

			player->m_velX = + moveX1 + moveX2;
			player->m_velY = - moveY1 - moveY2;
			player->Update(dt);
		}

		{
			Player * player = gameDataCL.m_gameState->m_players[3];

			player->m_velX = - moveX1 - moveX2;
			player->m_velY = + moveY1 + moveY2;
			player->Update(dt);
		}

		for (int p = 1; p < GameState::kMaxPlayers; ++p)
		{
			Player * player = gameDataCL.m_gameState->m_players[p];

			UpdatePlayerCollision(gameDataCL.m_gameState->m_map, player, true, false);
		}

		for (int i = 0; i < Map::kMaxDoors; ++i)
		{
			Door * door = gameDataCL.m_gameState->m_map->m_doors[i];

			door->Update(dt);
		}

		// calculate diff to send to server

		{
			const BinaryDiffResult client2server = gameDataCL.GetDiff(serverState, 4);
			client2serverPackage = MakeBinaryDiffPackage(gameDataCL.m_stateVector.m_bytes, StateVector::kSize, client2server);

			if (client2server.m_diffBytes != 0)
				LOG_INF("CL diff bytes: %u bytes", client2server.m_diffBytes);
		}

		ApplyBinaryDiffPackage(serverState.m_bytes, StateVector::kSize, client2serverPackage);
		NetAssert(!memcmp(serverState.m_bytes, gameDataCL.m_stateVector.m_bytes, StateVector::kSize));

	#if 0
		for (uint32_t i = 0; i < 8; ++i)
		{
			const BinaryDiffResult result = gameData.GetDiff(i);
			const uint32_t overhead = 4;
			const uint32_t byteCount = result.m_diffCount * overhead + result.m_diffBytes;
			LOG_INF("byteCount @ treshold %u: %u", i, byteCount);
			for (const BinaryDiffEntry * entry = result.m_diffs.get(); entry; entry = entry->m_next)
				LOG_INF("entry: offset: %09u, size: %09u", entry->m_offset, entry->m_size);
			if (!BinaryDiffValidate(
				gameData.m_stateVectors[0].m_bytes,
				gameData.m_stateVectors[1].m_bytes,
				StateVector::kSize,
				result.m_diffs.get()))
			{
				LOG_ERR("binary diff validation failed", 0);
				NetAssert(false);
			}
		}
	#endif

		SDL_Bitmap bitmap(
			surface,
			0,
			0,
			surface->w,
			surface->h);

		bitmap.Clear(0);

		gameDataCL.m_gameState->m_map->Draw(surface);
		for (int p = 0; p < GameState::kMaxPlayers; ++p)
			gameDataCL.m_gameState->m_players[p]->Draw(surface);

		SDL_Flip(surface);
	}
}

static void TestBitStream()
{
	BitStream bs1;

	uint8_t v1 = 0x11;
	uint16_t v2 = 0x2233;
	uint32_t v3 = 0x44556677;

	bs1.Write(v1);
	bs1.Write(v2);
	bs1.Write(v3);

	bs1.WriteBit(true);
	bs1.WriteBit(false);
	bs1.WriteBit(true);

	uint32_t v4 = 0x8899aabb;
	uint8_t v5 = 0xcc;
	bs1.Write(v4);
	bs1.WriteAlign();
	bs1.Write(v5);

	bs1.WriteString("Hello World!");

	uint8_t a1[4] = { 0x44, 0x33, 0x22, 0x11 };
	bs1.WriteAlignedBytes(a1, 4);

	uint8_t a2[4] = { 0x44, 0xff, 0x22, 0xff };
	BinaryDiffResult diff = BinaryDiff(a1, a2, sizeof(a1), 0);

	WriteDiff(bs1, diff, a1);

	{
		BitStream bs2(bs1.GetData(), bs1.GetDataSize());

		uint8_t r1;
		uint16_t r2;
		uint32_t r3;

		bs2.Read(r1);
		bs2.Read(r2);
		bs2.Read(r3);

		Assert(r1 == 0x11);
		Assert(r2 == 0x2233);
		Assert(r3 == 0x44556677);

		bool b1 = bs2.ReadBit();
		bool b2 = bs2.ReadBit();
		bool b3 = bs2.ReadBit();

		Assert(b1 == true);
		Assert(b2 == false);
		Assert(b3 == true);

		uint32_t r4;
		uint8_t r5;

		bs2.Read(r4);
		bs2.ReadAlign();
		bs2.Read(r5);

		Assert(r4 == 0x8899aabb);
		Assert(r5 == 0xcc);

		std::string s = bs2.ReadString();

		Assert(s == "Hello World!");

		uint8_t r[4];
		bs2.ReadAlignedBytes(r, 4);

		ReadDiff(bs2, a2);

		NetAssert(!memcmp(a1, a2, sizeof(a1)));
	}
}

static void TestRpcCall(BitStream & bs)
{
	printf("TestRpcCall!\n");
}

static void TestRpc()
{
	Timer timer;
	ChannelManager channelMgr;
	RpcManager rpcMgr(&channelMgr);

	uint32_t testRpcCall = rpcMgr.Register("TestRpcCall", TestRpcCall);

	channelMgr.Initialize(nullptr, 12345, true);

	PacketDispatcher::RegisterProtocol(PROTOCOL_CHANNEL, &channelMgr);
	PacketDispatcher::RegisterProtocol(PROTOCOL_RPC, &rpcMgr);

	NetAddress loopback;
	loopback.Set(127, 0, 0, 1, 12345);

	printf("  C: do RPC call\n");
	printf("  A: add client channel\n");
	printf("ESC: end test\n");

	bool stop = false;

	while (stop == false)
	{
		SDL_Delay(100);

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_c)
				{
					BitStream bs;

					rpcMgr.Call(testRpcCall, bs, ChannelPool_Client, nullptr, true, true);
				}
				if (e.key.keysym.sym == SDLK_a)
				{
					Channel * channel = channelMgr.CreateChannel(ChannelPool_Client);
					channel->Connect(loopback);
				}
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					stop = true;
				}
			}
		}

		const uint32_t time = static_cast<uint32_t>(timer.TimeMS_get());

		channelMgr.Update(time);
	}

	rpcMgr.Unregister(testRpcCall, TestRpcCall);

	PacketDispatcher::UnregisterProtocol(PROTOCOL_CHANNEL, &channelMgr);
	PacketDispatcher::UnregisterProtocol(PROTOCOL_RPC, &rpcMgr);

	channelMgr.Shutdown(false);
}

//

class MySerializable : public NetSerializable
{
public:
	MySerializable(NetSerializableObject * owner)
		: NetSerializable(owner)
	{
	}

	virtual void SerializeStruct()
	{
		bool b = false;
		int v = 0;
		std::string s;

		if (IsSend())
		{
			b = true;
			v = 0x11223344;
			s = "Hello World!";
		}

		Serialize(b);
		Align();
		Serialize(v);
		Serialize(s);

		NetAssert(b == true);
		NetAssert(v == 0x11223344);
		NetAssert(s == "Hello World!");
	}
};

static void TestSerializableObject()
{
	NetSerializableObject object;
	MySerializable serializable1(&object);
	MySerializable serializable2(&object);
	MySerializable serializable3(&object);

	{
		BitStream b1;
		if (object.Serialize(false, true, b1))
		{
			BitStream b2(b1.GetData(), b1.GetDataSize());
			object.Serialize(false, false, b2);
		}
	}

	{
		serializable2.SetDirty();

		BitStream b1;
		if (object.Serialize(false, true, b1))
		{
			BitStream b2(b1.GetData(), b1.GetDataSize());
			object.Serialize(false, false, b2);
		}

		NetAssert(!object.Serialize(false, true, b1));
	}
}

//

template <typename T>
static bool IsEqual(const NetArray<T> & a1, const NetArray<T> & a2)
{
	if (a1.size() != a2.size())
		return false;
	if (a1.capacity() != a2.capacity())
		return false;
	for (size_t i = 0; i < a1.size(); ++i)
	{
		if (a1[i] != a2[i])
			return false;
	}
	return true;
}

static void TestNetArray()
{
	NetArray<int> a1;

	a1.push_back(0);
	a1.push_back(1);
	a1.push_back(2);

	a1.clear();
	a1.reserve(100);
	a1.resize(50);

	a1.push_back(0);
	a1.push_back(1);
	a1.push_back(2);
	a1.erase(1);
	a1.set(2, -2);

	{
		BitStream b1;
		NetSerializationContext context;
		context.Set(false, true, b1);
		a1.Serialize(context);

		//

		NetArray<int> a2;
		BitStream b2(b1.GetData(), b1.GetDataSize());
		context.Set(false, false, b2);
		a2.Serialize(context);

		NetAssert(IsEqual(a1, a2));

		//

		BitStream b3;
		context.Set(true, true, b3);
		a1.Serialize(context);

		NetArray<int> a3;
		BitStream b4(b3.GetData(), b3.GetDataSize());
		context.Set(true, false, b4);
		a3.Serialize(context);

		NetAssert(IsEqual(a1, a3));
	}

	a1.clear();
	
	for (size_t i = 0; i < 100; ++i)
		a1.push_back(i);

	{
		BitStream b1;
		NetSerializationContext context;
		context.Set(false, true, b1);
		a1.Serialize(context);

		NetArray<int> a2;
		BitStream b2(b1.GetData(), b1.GetDataSize());
		context.Set(false, false, b2);
		a2.Serialize(context);

		NetAssert(IsEqual(a1, a2));
	}

	for (size_t i = 0; i < 100; ++i)
	{
		NetArray<int> a1;
		NetArray<int> a2;

		struct
		{
			char c;
			size_t chance;
		} actions[] =
		{
			{ 'c', 1  }, // clear
			{ 'e', 5  }, // erase
			{ 'p', 20 }, // push_back
			{ 'r', 2  }, // resize
			{ 's', 10 }  // set element
		};

		size_t totalChance = 0;

		for (size_t j = 0; j < sizeof(actions) / sizeof(actions[0]); ++j)
		{
			totalChance += actions[j].chance;
			actions[j].chance = totalChance;
		}

		for (size_t j = 0; j < 100; ++j)
		{
			size_t dice = rand() % totalChance;

			for (size_t k = 0, c = 0; k < sizeof(actions) / sizeof(actions[0]); ++k)
			{
				c += actions[k].chance;
				
				if (dice < actions[k].chance)
				{
					switch (actions[k].c)
					{
					case 'c':
						a1.clear();
						break;
					case 'e':
						if (!a1.empty())
						{
							size_t index = rand() % a1.size();
							a1.erase(index);
						}
						break;
					case 'p':
						a1.push_back(rand());
						break;
					case 'r':
						if (a1.empty())
							a1.resize(rand() % 10);
						else
							a1.resize(rand() % (a1.size() * 2));
						break;
					case 's':
						if (!a1.empty())
						{
							size_t index = rand() % a1.size();
							a1.set(index, rand());
						}
						break;
					}
					break;
				}
			}

			if ((rand() % 10) == 0)
			{
				BitStream b1;
				NetSerializationContext context;
				context.Set(false, true, b1);
				a1.Serialize(context);

				BitStream b2(b1.GetData(), b1.GetDataSize());
				context.Set(false, false, b2);
				a2.Serialize(context);

				NetAssert(IsEqual(a1, a2));
			}
		}
	}
}

int main(int argc, char * argv[])
{
	try
	{
		const uint32_t displaySx = 256;
		const uint32_t displaySy = 256;

		if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
			throw std::exception();//"failed to initialize SDL");

		SDL_Surface * surface = SDL_SetVideoMode(displaySx, displaySy, 32, 0);

		//TestBitStream();
		//TestRpc();
		TestSerializableObject();
		TestNetArray();
		//TestGameUpdate(surface);
		printf("skipping libnet test!\n");
		return -1;

		printf("select mode:\n");
		printf("S = server\n");
		printf("C = client\n");
		printf("B = both server & client\n");

		char mode = 'U';

		while (mode == 'U')
		{
			SDL_Event e;
			if (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_s)
						mode = 'S';
					if (e.key.keysym.sym == SDLK_c)
						mode = 'C';
					if (e.key.keysym.sym == SDLK_b)
						mode = 'B';
				}
			}
		}

		printf("mode = %c\n", mode);

		printf("       ESC: quit\n");
		printf("         A: add client channel\n");
		printf("LSHIFT + A: add 100 client channels\n");
		printf("         D: remove client channel\n");
		printf("LSHIFT + D: remove all client channels\n");
		printf("         L: list channels\n");
		printf("         R: randomize data, send reliably\n");
		printf("         T: randomize data, send unreliably\n");

		bool isServer = mode == 'S' || mode == 'B';
		bool isClient = mode == 'C' || mode == 'B';

		uint16_t serverPort = 3300;
		uint16_t listenPort = isServer ? serverPort : serverPort + 1;

		//

		Timer timer;
		PolledTimer polledTimer;
		polledTimer.Initialize(&timer);
		polledTimer.Start();
		
		MyChannelHandler channelHandler;
		
		ChannelManager channelMgr;
		channelMgr.Initialize(&channelHandler, listenPort, isServer);

		PacketDispatcher::RegisterProtocol(PROTOCOL_CHANNEL, &channelMgr);

		bool stop = false;
		bool randomize1 = false;
		bool randomize2 = false;

		while (stop == false)
		{
			SDL_Delay(10);
			//SDL_Delay(1);
			//SDL_Delay(0);

			SDL_Event e;

			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE)
						stop = true;
					if (isClient && e.key.keysym.sym == SDLK_a)
					{
						uint32_t count = (e.key.keysym.mod & KMOD_SHIFT) ? 100 : 1;

						for (uint32_t i = 0; i < count; ++i)
						{
							Channel * clientChannel = channelMgr.CreateChannel(ChannelPool_Client);
							NetAssert(clientChannel != 0);
							LOG_INF("created client channel %09u", clientChannel->m_id);
							
							//NetAddress address(74, 125, 79, 104, 4850);
							NetAddress address(127, 0, 0, 1, serverPort);
							
							clientChannel->Connect(address);

							//channelHandler.CL_AddChannel(clientChannel);
						}
					}
					if (e.key.keysym.sym == SDLK_d)
					{
						bool all = (e.key.keysym.mod & KMOD_SHIFT) != 0;

						if (isClient)
						{
							if (all)
								channelHandler.CL_DisconnectAll(&channelMgr);
							else
								channelHandler.CL_DisconnectRandom(&channelMgr);
						}
						else if (isServer)
						{
							if (all)
								channelHandler.SV_DisconnectAll(&channelMgr);
							else
								channelHandler.SV_DisconnectRandom(&channelMgr);
						}
					}
					if (e.key.keysym.sym == SDLK_l)
					{
						channelHandler.Show();
					}
					if (e.key.keysym.sym == SDLK_t)
					{
						randomize1 = true;
					}
					if (e.key.keysym.sym == SDLK_r)
					{
						randomize2 = true;
					}
				}
				if (e.type == SDL_KEYUP)
				{
					if (e.key.keysym.sym == SDLK_t)
					{
						randomize1 = false;
					}
					if (e.key.keysym.sym == SDLK_r)
					{
						randomize2 = false;
					}
				}
			}

			if (randomize1)
			{
				if (isClient)
					channelHandler.CL_Send();
				else if (isServer)
					channelHandler.SV_Send();
			}

			if (randomize2)
			{
				if (isClient)
					channelHandler.CL_SendRT();
				else if (isServer)
					channelHandler.SV_SendRT();
			}

			uint32_t time = polledTimer.TimeMS_get();
			
			channelMgr.Update(time);

			if (SDL_LockSurface(surface) < 0)
				throw std::exception();//"failed to lock surface");

			SDL_Bitmap bitmap(surface, 0, 0, displaySx, displaySy);

			uint32_t c = bitmap.Color(isClient ? 1.0f : 0.0f, isServer ? 1.0f : 0.0f, 0.0f);

			bitmap.Clear(c);

#if 1
			for (std::map<uint32_t, ClientState *>::iterator i = channelHandler.m_clientStates.begin(); i != channelHandler.m_clientStates.end(); ++i)
			{
				ClientState * clientState = i->second;

				clientState->Draw(surface);
			}
#endif

			SDL_UnlockSurface(surface);

			SDL_Flip(surface);
		}
		
		channelMgr.Shutdown(true);

		SDL_FreeSurface(surface);
		
		return 0;
	}
	catch (std::exception & e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
}
