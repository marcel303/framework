#pragma once

class Fader
{
public:
	Fader();
	void Initialize();
	
	void Setup(float downSpeed, float upSpeed, float reference, float value);
	
	void Update(float dt);
	
	void Reference_set(float reference);
	float Value_get() const;
	
private:
	float m_DownSpeed;
	float m_UpSpeed;
	float m_Reference;
	float m_Value;
};
