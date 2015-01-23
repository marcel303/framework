#include "StatTimers.h"

#if GG_ENABLE_TIMERS

#include "Debugging.h"
#include "Timer.h"

// StatTimerManager

static StatTimer * s_firstTimer = 0;
static StatTimerView * s_firstTimerView = 0;

StatTimerManager::StatTimerManager()
	: m_nextUpdateTime(0)
{
}

void StatTimerManager::Register(StatTimer * timer)
{
	timer->m_next = s_firstTimer;

	s_firstTimer = timer;
}

void StatTimerManager::Register(StatTimerView * view)
{
	view->m_next = s_firstTimerView;

	s_firstTimerView = view;
}

bool StatTimerManager::Update()
{
	uint64_t time = g_TimerRT.TimeUS_get();

	bool oneSecondElapsed = (m_nextUpdateTime == 0) || (time >= m_nextUpdateTime);

	if (oneSecondElapsed)
	{
		m_nextUpdateTime = time + 1000*1000;
	}

	for (StatTimer * timer = s_firstTimer; timer != 0; timer = timer->GetNext())
	{
		if (timer->m_isAutoCommit)
		{
			if (timer->GetUpdateMode() == StatTimer::PerFrame ||
				timer->GetUpdateMode() == StatTimer::PerSecond && oneSecondElapsed)
			{
				timer->Commit(true);
			}
		}
	}

	return oneSecondElapsed;
}

StatTimer * StatTimerManager::GetFirstTimer() const
{
	return s_firstTimer;
}

StatTimerView * StatTimerManager::GetFirstView() const
{
	return s_firstTimerView;
}


StatTimerManager g_statTimerManager;

// StatTimer

uint32_t StatTimer::GetLastIndex() const
{
	return (m_nextHistoryIndex == 0)
		? (kHistorySize - 1)
		: (m_nextHistoryIndex - 1);
}

StatTimer::StatTimer(UpdateMode updateMode, const char * path)
	: m_updateMode(updateMode)
	, m_isAutoCommit(true)
	, m_path(path)
	, m_timeValue(0)
	, m_countValue(0)
	, m_historySize(0)
	, m_nextHistoryIndex(0)
	, m_next(0)
{
	g_statTimerManager.Register(this);
}

void StatTimer::Commit(bool isAutoCommit)
{
	Assert(m_isAutoCommit == isAutoCommit);

	HistoryRecord & record = m_history[m_nextHistoryIndex];
	record.time = (uint32_t)m_timeValue;
	record.count = m_countValue;

	m_timeValue = 0;
	m_countValue = 0;

	m_nextHistoryIndex++;

	if (m_nextHistoryIndex > m_historySize)
		m_historySize = m_nextHistoryIndex;

	if (GetUpdateMode() == StatTimer::PerSecond)
	{
		m_nextHistoryIndex = 0;
	}
	else
	{
		if (m_nextHistoryIndex == kHistorySize)
			m_nextHistoryIndex = 0;
	}
}

void StatTimer::Start()
{
	m_timeValue -= g_TimerRT.TimeUS_get();
}

void StatTimer::Stop()
{
	m_timeValue += g_TimerRT.TimeUS_get();
}

uint32_t StatTimer::GetLastTime() const
{
	if (m_historySize == 0)
		return 0;
	return m_history[GetLastIndex()].time;
}

uint32_t StatTimer::GetLastCount() const
{
	if (m_historySize == 0)
		return 0;
	return m_history[GetLastIndex()].count;
}

uint32_t StatTimer::GetAverageTime() const
{
	if (m_historySize == 0)
		return 0;

	uint32_t total = 0;

	for (uint32_t i = 0; i < m_historySize; ++i)
		total += m_history[i].time;

	return total / m_historySize;
}

uint32_t StatTimer::GetAverageCount() const
{
	if (m_historySize == 0)
		return 0;

	uint32_t total = 0;

	for (uint32_t i = 0; i < m_historySize; ++i)
		total += m_history[i].count;

	return total / m_historySize;
}

// StatTimerView

StatTimerView::Item::Item(Item *& head, Item *& tail, StatTimer * timer, uint8_t displayMode)
	: m_timer(timer)
	, m_displayMode(displayMode)
{
	if (head == 0)
		head = this;
	else
		tail->m_next = this;
	tail = this;
	m_next = 0;
}

StatTimerView::StatTimerView(const char * path)
	: m_path(path)
	, m_next(0)
	, m_itemHead(0)
	, m_itemTail(0)
{
	g_statTimerManager.Register(this);
}

StatTimerView::~StatTimerView()
{
	Item * item = m_itemHead;

	while (item != 0)
	{
		Item * nextItem = item->m_next;
		delete item;
		item = nextItem;
	}

	m_itemHead = 0;
	m_itemTail = 0;
}

void StatTimerView::Add(StatTimer * timer, uint8_t displayMode)
{
	Item * item = new Item(m_itemHead, m_itemTail, timer, displayMode);
}

#endif
