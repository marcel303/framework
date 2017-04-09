#include "vfxNodeBase.h"

class MediaPlayer;

struct VfxNodeVideo : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture * image;
	
	MediaPlayer * mediaPlayer;
	
	VfxNodeVideo();
	
	virtual ~VfxNodeVideo() override;
	
	virtual void tick(const float dt) override;
};
