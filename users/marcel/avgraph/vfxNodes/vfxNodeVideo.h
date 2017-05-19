#include "vfxNodeBase.h"

struct MediaPlayer;

struct VfxNodeVideo : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_Transform,
		kInput_Loop,
		kInput_Speed,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	VfxImage_Texture * image;
	
	MediaPlayer * mediaPlayer;
	
	GLuint textureBlack;
	
	VfxNodeVideo();
	
	virtual ~VfxNodeVideo() override;
	
	virtual void tick(const float dt) override;
	virtual void init(const GraphNode & node) override;
};
