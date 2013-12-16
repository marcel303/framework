#import <UIKit/UIKit.h>
#import "TiltReader.h"

#define kAccelerometerFrequency		25.0f // Hz
#define kFilteringFactor			0.4f

@interface TiltReaderCC : NSObject <UIAccelerometerDelegate>
{
	@public
	
	Vec3 accel;
}

@property (assign) Vec3 accel;

-(id)init;

@end

@implementation TiltReaderCC

@synthesize accel;

-(id)init
{
	if ((self = [super init]))
	{
		[[UIAccelerometer sharedAccelerometer] setUpdateInterval:(1.0 / kAccelerometerFrequency)];
		[[UIAccelerometer sharedAccelerometer] setDelegate:self];
	}
	
	return self;
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
	// use a basic low pass filter to smooth out results
	
	accel[0] = (float)acceleration.x * kFilteringFactor + accel[0] * (1.0f - kFilteringFactor);
	accel[1] = (float)acceleration.y * kFilteringFactor + accel[1] * (1.0f - kFilteringFactor);
	accel[2] = (float)acceleration.z * kFilteringFactor + accel[2] * (1.0f - kFilteringFactor);
	
#if 0
#warning
	accel[0] = (float)acceleration.x;
	accel[1] = (float)acceleration.y;
	accel[2] = (float)acceleration.z;
#endif
	
//	LOG_INF("accel: %1.03f, %1.03f, %1.03f", accel[0], accel[1], accel[2]);
}

@end

TiltReader::TiltReader()
{
	TiltReaderCC* reader = [[TiltReaderCC alloc] init];
	
	mReader = reader;
}

Vec3 TiltReader::GravityVector_get() const
{
	TiltReaderCC* reader = (TiltReaderCC*)mReader;
	
	return reader.accel;
//	return Vec3(reader.accel[0], reader.accel[1], reader.accel[2]);
}

Vec2F TiltReader::DirectionXY_get() const
{
	TiltReaderCC* reader = (TiltReaderCC*)mReader;
	
	Vec3 accel = reader.accel;
	
	return Vec2F(accel[0], accel[1]);
}
