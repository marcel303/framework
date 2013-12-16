#include "PowerSink.h"

PowerSink::PowerSink()
{
	Initialize();
}

void PowerSink::Initialize()
{
	m_Power = 0.0f;
	m_Capacity = 0.0f;
}

void PowerSink::DistributionRequest(PowerSink* sink, float powerPerSecond)
{
	PowerRequest request;

	request.m_PowerSink = sink;
	request.m_Amount = powerPerSecond;

	m_Requests.push_back(request);
}

void PowerSink::DistributionCommit()
{
	if (m_Requests.size() == 0)
		return;

	float total = 0.0f;

	for (int i = 0; i < m_Requests.size(); ++i)
		total += m_Requests[i].m_Amount;

	if (total == 0.0f)
		return;

	float scale;

	if (total > m_Power)
		scale = m_Power / total;
	else
		scale = 1.0f;

	for (int i = 0; i < m_Requests.size(); ++i)
	{
		m_Requests[i].m_PowerSink->Store(m_Requests[i].m_Amount * scale);
	}

	// todo: assume power is lost after commit?
	//       or super charge?
	//           -> both above methods require request/distribute be sequenced.
	//       or keep storing energy? -> would create huge reserve, impact of power loss would not be noticable until it dries up.
	
	m_Power -= total * scale;
	//m_Power = 0.0f;

	m_Requests.clear();
}

void PowerSink::Store(float amount)
{
	m_Power += amount;
}

BOOL PowerSink::TryConsume(float amount)
{
	if (amount > m_Power)
		return FALSE;

	m_Power -= amount;

	return TRUE;
}

float PowerSink::ConsumeAll()
{
	float result = m_Power;

	m_Power = 0.0f;

	return result;
}

void PowerSink::Update()
{
	if (m_Power > m_Capacity)
		m_Power = 0.0f;
}
