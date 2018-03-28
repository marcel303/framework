#pragma once

struct Thermalizer
{
	Mat4x4 thermalToView;
	
	int size = 0;
	
	double * heat = nullptr;
	double * bang = nullptr;
	
	Thermalizer();
	~Thermalizer();
	
	void init(const int _size);
	void shut();
	
	void applyHeat(const int index, const double _heat, const double dt);
	double sampleHeat(const int index) const;
	void diffuseHeat(const double dt);
	void applyHeatAtViewPos(const float x, const float y, const float dt);
	
	void tick(const float dt);
	
	void draw2d() const;
};
