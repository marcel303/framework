#include "vfxNodeBase.h"

struct MediaPlayer;

struct VfxNodeVideo : VfxNodeBase
{
	enum Input
	{
		kInput_Source,
		kInput_Loop,
		kInput_Speed,
		kInput_OutputMode,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_ImageCpuRGBA,
		kOutput_ImageCpuY,
		kOutput_ImageCpuU,
		kOutput_ImageCpuV,
		kOutput_COUNT
	};
	
	VfxImage_Texture imageOutput;
	VfxImageCpu imageCpuOutputRGBA;
	VfxImageCpu imageCpuOutputY;
	VfxImageCpu imageCpuOutputU;
	VfxImageCpu imageCpuOutputV;
	
	MediaPlayer * mediaPlayer;
	
	GLuint textureBlack;
	
	VfxNodeVideo();
	virtual ~VfxNodeVideo() override;
	
	virtual void tick(const float dt) override;
	virtual void init(const GraphNode & node) override;
};
