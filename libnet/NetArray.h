#pragma once

#include <stdint.h>
#include <vector>
#include "libnet_forward.h"
#include "NetSerializable.h"

template <typename T>
class NetArray
{
	const static size_t kMaxSize = (1 << 16);

	enum ActionType
	{
		ActionType_SetElement,
		ActionType_PushBack,
		ActionType_Clear,
		ActionType_Erase,
		ActionType_Resize,
		ActionType_Reserve,
		ActionType_Undefined
	};

	struct Action
	{
		Action()
			: type(ActionType_Undefined)
			, index(-1)
			, value()
		{
		}

		union
		{
			uint8_t type;
			ActionType type_ : 8;
		};
		uint16_t index;
		uint16_t size;
		T value;
	};

	std::vector<Action> m_actions;
	uint32_t m_actionsSize;
	bool m_fullSync;

	std::vector<T> m_data;

public:
	NetArray()
		: m_actions()
		, m_actionsSize(0)
		, m_fullSync(false)
		, m_data()
	{
	}

	void clear()
	{
		if (!m_data.empty())
		{
			m_actions.clear();
			m_actionsSize = 0;
			m_fullSync = false;

			Action action;
			action.type = ActionType_Clear;
			action.size = m_data.capacity();
			m_actions.push_back(action);
			m_actionsSize += sizeof(action.type) + sizeof(action.size);

			m_data.clear();
		}
	}

	void resize(size_t size)
	{
		NetAssert(size <= kMaxSize);

		if (size == 0)
		{
			clear();
		}
		else if (size != m_data.size())
		{
			if (!m_fullSync)
			{
				Action action;
				action.type = ActionType_Resize;
				action.size = size;
				m_actions.push_back(action);
				m_actionsSize += sizeof(action.type) + sizeof(action.size);
			}

			m_data.resize(size);
		}
	}

	size_t size() const
	{
		return m_data.size();
	}

	bool empty() const
	{
		return m_data.empty();
	}

	void reserve(size_t size)
	{
		NetAssert(size <= kMaxSize);

		if (size != m_data.capacity())
		{
			if (!m_fullSync)
			{
				Action action;
				action.type = ActionType_Reserve;
				action.size = size;
				m_actions.push_back(action);
				m_actionsSize += sizeof(action.type) + sizeof(action.size);
			}

			m_data.reserve(size);
		}
	}

	size_t capacity() const
	{
		return m_data.capacity();
	}

	void push_back(const T & value)
	{
		NetAssert(m_data.size() + 1 <= kMaxSize);

		if (!m_fullSync)
		{
			Action action;
			action.type = ActionType_PushBack;
			action.value = value;
			m_actions.push_back(action);
			m_actionsSize += sizeof(action.type) + sizeof(action.value);
		}

		m_data.push_back(value);
	}

	void erase(size_t index)
	{
		NetAssert(index < m_data.size());

		if (m_data.size() == 1)
		{
			clear();
		}
		else
		{
			if (!m_fullSync)
			{
				Action action;
				action.type = ActionType_Erase;
				action.index = index;
				m_actions.push_back(action);
				m_actionsSize += sizeof(action.type) + sizeof(action.index);
			}

			m_data.erase(m_data.begin() + index);
		}
	}

	void set(size_t index, const T & value)
	{
		NetAssert(index < m_data.size());

		if (m_data[index] != value)
		{
			if (!m_fullSync)
			{
				Action action;
				action.type = ActionType_SetElement;
				action.index = index;
				action.value = value;
				m_actions.push_back(action);
				m_actionsSize += sizeof(action.type) + sizeof(action.index) + sizeof(action.value);
			}

			m_data[index] = value;
		}
	}

	const T & get(size_t index) const
	{
		return m_data[index];
	}

	const T & operator[](size_t index) const
	{
		return get(index);
	}

	void Serialize(NetSerializationContext & context)
	{
		if (context.IsSend())
		{
			bool fullSync = context.IsInit() || m_fullSync;

			bool isDirty = fullSync || !m_actions.empty();

			context.Serialize(isDirty);

			if (isDirty)
			{
				const size_t actionsSize = sizeof(uint16_t) + m_actionsSize;
				const size_t fullSize = sizeof(uint16_t) + sizeof(uint16_t) + (m_data.size() * sizeof(T));

				if (!fullSync)
				{
					// check if it's more efficient to do a full sync

					if (actionsSize >= fullSize)
						fullSync = true;
				}

				// serialize!

				context.Serialize(fullSync);

			#ifdef DEBUG
				const size_t expectedSize = context.GetBitStream().GetDataSize() + (fullSync ? fullSize : actionsSize) * 8;
				//printf("%u vs %u\n", fullSize, actionsSize);
			#endif

				if (fullSync)
				{
					NetAssert(m_data.size() < (1 << 16));
					NetAssert(m_data.capacity() < (1 << 16));

					uint16_t size = m_data.size();
					uint16_t capacity = m_data.capacity();

					context.Serialize(size);
					context.Serialize(capacity);

					for (size_t i = 0; i < m_data.size(); ++i)
					{
						context.Serialize(m_data[i]);
					}
				}
				else
				{
					NetAssert(m_actions.size() < (1 << 16));
					uint16_t numActions = m_actions.size();
					context.Serialize(numActions);

					for (size_t i = 0; i < m_actions.size(); ++i)
					{
						Action & action = m_actions[i];
						context.Serialize(action.type);

						switch (action.type)
						{
						case ActionType_SetElement:
							context.Serialize(action.index);
							context.Serialize(action.value);
							break;
						case ActionType_PushBack:
							context.Serialize(action.value);
							break;
						case ActionType_Clear:
							context.Serialize(action.size);
							break;
						case ActionType_Erase:
							context.Serialize(action.index);
							break;
						case ActionType_Resize:
							context.Serialize(action.size);
							break;
						case ActionType_Reserve:
							context.Serialize(action.size);
							break;

						default:
							NetAssert(false);
						}
					}

				#ifdef DEBUG
					NetAssert(context.GetBitStream().GetDataSize() == expectedSize);
				#endif
				}
			}
		}
		else
		{
			NetAssert(m_actions.empty());

			bool isDirty;

			context.Serialize(isDirty);

			NetAssert(!context.IsInit() || isDirty);

			if (isDirty)
			{
				bool fullSync;

				context.Serialize(fullSync);

				NetAssert(!context.IsInit() || fullSync);

				if (fullSync)
				{
					uint16_t size;
					uint16_t capacity;

					context.Serialize(size);
					context.Serialize(capacity);

					m_data.reserve(capacity);
					m_data.resize(size);

					for (size_t i = 0; i < size; ++i)
					{
						context.Serialize(m_data[i]);
					}
				}
				else
				{
					uint16_t numActions;
					context.Serialize(numActions);

					for (size_t i = 0; i < numActions; ++i)
					{
						Action action;
						context.Serialize(action.type);

						switch (action.type)
						{
						case ActionType_SetElement:
							context.Serialize(action.index);
							context.Serialize(action.value);
							m_data[action.index] = action.value;
							break;
						case ActionType_PushBack:
							context.Serialize(action.value);
							m_data.push_back(action.value);
							break;
						case ActionType_Clear:
							context.Serialize(action.size);
							m_data.clear();
							m_data.reserve(action.size);
							break;
						case ActionType_Erase:
							context.Serialize(action.index);
							m_data.erase(m_data.begin() + action.index);
							break;
						case ActionType_Resize:
							context.Serialize(action.size);
							m_data.resize(action.size);
							break;
						case ActionType_Reserve:
							context.Serialize(action.size);
							m_data.reserve(action.size);
							break;

						default:
							NetAssert(false);
						}
					}
				}
			}
		}
		
		if (!context.IsInit())
		{
			m_actions.clear();
			m_actionsSize = 0;
			m_fullSync = false;
		}
	}
};
