#include "ISystem.h"
#include "SpriteGfx.h"
#include "TiltReader.h"

class System_iPhone : public ISystem
{
public:
	System_iPhone();
	~System_iPhone();
	
	virtual void Vibrate();
	virtual void CheckNetworkConnectivity();
	virtual bool HasNetworkConnectivity_get();
	virtual void HasNetworkConnectivity_set(bool value);
	virtual bool IsHacked();
	//virtual std::string GetDeviceId();
	virtual std::string GetResourcePath(const char* fileName);
	virtual std::string GetDocumentPath(const char* fileName);
	virtual std::string GetCountryCode();
	virtual void SaveToAlbum(class Screenshot* ss, const char* name);
	virtual Vec3 GetTiltVector();
	virtual Vec2F GetTiltDirectionXY();
	virtual bool IsIpad();
	
	SpriteColor Color_FromHSB(float h, float s, float b);
	int GetScreenRotation();
	
private:
	TiltReader* mTiltReader;
};
