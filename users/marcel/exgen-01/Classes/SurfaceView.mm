#import "Calc.h"
#import "Surface2Image.h"
#import "SurfaceView.h"

#import "PT_LinearMovement.h"
#import "DT_Linear.h"
#import "CR_Fire.h"

@implementation SurfaceView

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
		[self setClearsContextBeforeDrawing:TRUE];
		[self setBackgroundColor:[UIColor blackColor]];
		
		mParticleMgr.Setup(100);
		
		for (int i = 0; i < mParticleMgr.ParticleCount_get(); ++i)
		{
			double vx, vy;
			Calc::RandomA(4.0, 10.0, vx, vy);
			
			mParticleMgr[i].Setup(128.0, 128.0, vx, vy, 10.0);
			DT_Linear::Setup(&mParticleMgr[i], 0.1);
			
			AC_Vortex* vortex = new AC_Vortex();
			vortex->Setup(128.0, 128.0, 1.0);
//			mParticleMgr[i].ModifierList_get().Add(vortex);
		}
		
		mSurface.Setup(256, 256);
		
		[NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(HandleTimer) userInfo:NULL repeats:YES];
		
		[self setNeedsDisplay];
    }
    return self;
}

- (void)drawRect:(CGRect)rect {
	
	int n = 10;
	
	for (int i = 0; i < n; ++i)
		mParticleMgr.Update(0.1 / n);
	mParticleMgr.Render(&mSurface, CR_Fire::ColorRamp);
	
	//
	
	CGImageRef image = Surface2Image(&mSurface);
	
	UIImage* uiImage = [[UIImage alloc] initWithCGImage:image];
	[uiImage drawAtPoint:CGPointMake(0.0f, 0.0f)];
	[uiImage release];
	
	CGImageRelease(image);
}

- (void)HandleTimer
{
	[self setNeedsDisplay];
}

- (void)dealloc {
    [super dealloc];
}

@end
