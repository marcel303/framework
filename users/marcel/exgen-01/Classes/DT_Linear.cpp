#include "DT_Linear.h"

namespace DT_Linear
{
	class Data
	{
	public:
		double mLinearFactor;
	};
	
	void Setup(Particle* p, double linearFactor)
	{
		p->Density_get = Density_get;
		
		Data& data = p->DensityData_get<Data>();
		
		data.mLinearFactor = linearFactor;
	}
	
	double Density_get(Particle* p, double x, double y)
	{
		Data& data = p->DensityData_get<Data>();
		
		double distance = p->Distance_get(x, y);
		
		return 1.0 - distance * data.mLinearFactor;
	}
}
