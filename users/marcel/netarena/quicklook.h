#pragma once

class GameSim;

class QuickLook
{
	bool m_isActive;
	float m_animProgress;

public:
	QuickLook();
	~QuickLook();

	void open(bool animated);
	void close(bool animated);

	bool isActive() const;

	void tick(float dt);
	void drawWorld(GameSim & gameSim);
	void drawHud(GameSim & gameSim);
};
