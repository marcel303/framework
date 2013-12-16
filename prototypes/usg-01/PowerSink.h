#pragma once

#include <vector>
#include "Types.h"

/*

PowerSink class.

A power sink dynamically stores and redistributes the available energy to other
sources amd/or consumers.

 */
class PowerSink;

class PowerRequest
{
public:
	PowerSink* m_PowerSink;
	float m_Amount;
};

class PowerSink
{
public:
	PowerSink();
	void Initialize();

	void DistributionRequest(PowerSink* sink, float powerPerSecond);
	void DistributionCommit();

	void Capacity_set(float amount)
	{
		m_Capacity = amount;
	}

	void Store(float amount);
	BOOL TryConsume(float amount);
	float ConsumeAll();
	void Update();

private:
	float m_Power;
	float m_Capacity;

	std::vector<PowerRequest> m_Requests;
};
