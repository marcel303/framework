#pragma once

#define GG_ENABLE_TIMERS 1

#if GG_ENABLE_TIMERS

#include <stdint.h>

class StatTimer;
class StatTimerManager;
class StatTimerView;

// StatTimerManager

class StatTimerManager
{
	uint64_t m_nextUpdateTime;

public:
	StatTimerManager();

	void Register(StatTimer * timer);
	void Register(StatTimerView * view);

	bool Update();

	StatTimer * GetFirstTimer() const;
	StatTimerView * GetFirstView() const;
};

extern StatTimerManager g_statTimerManager;

// StatTimer

class StatTimer
{
public:
	enum UpdateMode
	{
		PerFrame,
		PerSecond
	};

private:
	friend class StatTimerManager;
	friend class StatTimerManualCommit;

	static const int kHistorySize = 60;

	struct HistoryRecord
	{
		HistoryRecord()
			: time(0)
			, count(0)
		{
		}

		uint32_t time;
		uint32_t count;
	};

	StatTimer * m_next;
	UpdateMode m_updateMode;
	bool m_isAutoCommit;
	const char * m_path;

	HistoryRecord m_history[kHistorySize];
	uint8_t m_historySize;
	uint8_t m_nextHistoryIndex;

	uint64_t m_timeValue;
	uint32_t m_countValue;

	uint32_t GetLastIndex() const;

public:
	StatTimer(UpdateMode updateMode, const char * path);

	void Commit(bool isAutoCommit);

	StatTimer * GetNext()
	{
		return m_next;
	}

	UpdateMode GetUpdateMode() const
	{
		return m_updateMode;
	}

	const char * GetPath() const
	{
		return m_path;
	}

	void Start();
	void Stop();

	void AddCount(uint32_t count)
	{
		m_countValue += count;
	}

	uint32_t GetLastTime() const;
	uint32_t GetLastCount() const;
	uint32_t GetAverageTime() const;
	uint32_t GetAverageCount() const;
};

class StatTimerManualCommit
{
public:
	StatTimerManualCommit(StatTimer * timer)
	{
		timer->m_isAutoCommit = false;
	}
};

class StatTimerScope
{
	StatTimer * m_timer;

public:
	StatTimerScope(StatTimer * timer)
		: m_timer(timer)
	{
		m_timer->Start();
	}

	~StatTimerScope()
	{
		m_timer->Stop();
	}
};

// StatTimerView

class StatTimerView
{
public:
	enum DisplayMode
	{
		Time = 1 << 0,
		Count = 1 << 1
	};

private:
	friend class StatTimerManager;

	struct Item
	{
		Item(Item *& head, Item *& tail, StatTimer * timer, uint8_t displayMode);

		StatTimer * m_timer;
		uint8_t m_displayMode;

		Item * m_next;
	};

	const char * m_path;
	StatTimerView * m_next;

	Item * m_itemHead;
	Item * m_itemTail;

public:
	StatTimerView(const char * path);
	~StatTimerView();

	void Add(StatTimer * timer, uint8_t displayMode);
};

class StatTimerViewAdd
{
public:
	StatTimerViewAdd(StatTimerView * view, StatTimer * timer, uint8_t displayMode)
	{
		view->Add(timer, displayMode);
	}
};

// timers

#define TIMER_EXTERN(name) extern StatTimer name
#define TIMER_DEFINE(name, updateMode, path) StatTimer name(StatTimer::updateMode, path)
#define TIMER_MANUAL(name) static StatTimerManualCommit name ## _manualCommit(&name)
#define TIMER_COMMIT(name) name.Commit(false)
#define TIMER_ADD(name, count) name.AddCount(count)
#define TIMER_INC(name) TIMER_ADD(name, 1)
#define TIMER_START(name) name.Start()
#define TIMER_STOP(name) name.Stop()
#define TIMER_SCOPE(name) StatTimerScope name ## _scope(&name)

// timer views

#define TIMER_VIEW_BEGIN(name, path) \
	StatTimerView name(path); \
	class name ## _Begin \
	{ \
		StatTimerView * m_view; \
	public: \
		enum DisplayMode \
		{ \
			Time = StatTimerView::Time, \
			Count = StatTimerView::Count \
		}; \
		name ## _Begin() \
			: m_view(&name) \
		{ \
			do { } while (false)

#define TIMER_VIEW_END(name) \
		} \
	} name ## _begin

#define TIMER_VIEW_ADD(name, displayMode) static StatTimerViewAdd name ## _add(m_view, &name, displayMode)

#else

	// timers

	#define TIMER_EXTERN(name)
	#define TIMER_DEFINE(name, updateMode, path)
	#define TIMER_MANUAL(name)
	#define TIMER_COMMIT(name)     do { } while (false)
	#define TIMER_ADD(name, count) do { } while (false)
	#define TIMER_INC(name)        do { } while (false)
	#define TIMER_START(name)      do { } while (false)
	#define TIMER_STOP(name)       do { } while (false)
	#define TIMER_SCOPE(name)      do { } while (false)

	// timer views

	#define TIMER_VIEW_BEGIN(name, path)
	#define TIMER_VIEW_END(name)
	#define TIMER_VIEW_ADD(name, displayMode)

#endif
