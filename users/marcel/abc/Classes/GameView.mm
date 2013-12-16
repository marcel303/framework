#import "Game.h"
#import "GameView.h"

#define kAccelerometerFrequency		100.0 // Hz

@implementation GameView

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (id)initWithCoder:(NSCoder*)coder
{    
    if ((self = [super initWithCoder:coder]))
	{
		[self setMultipleTouchEnabled:TRUE];
		
		gGame = new Game();
		gGame->Setup();
		
		mTouchMgr.OnTouchBegin = CallBack(&gGame->mTouchDelegator, TouchDelegator::HandleTouchBegin);
		mTouchMgr.OnTouchEnd = CallBack(&gGame->mTouchDelegator, TouchDelegator::HandleTouchEnd);
		mTouchMgr.OnTouchMove = CallBack(&gGame->mTouchDelegator, TouchDelegator::HandleTouchMove);
		
		[[UIAccelerometer sharedAccelerometer] setUpdateInterval:(1.0 / kAccelerometerFrequency)];
		[[UIAccelerometer sharedAccelerometer] setDelegate:self];
		
		[NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0 / 100.0) target:self selector:@selector(handleTimer) userInfo:nil repeats:TRUE];
	}
	return self;
}

- (void)drawRect:(CGRect)rect {
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextSetAllowsAntialiasing(ctx, FALSE);
	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGContextSetShouldAntialias(ctx, FALSE);
	CGContextSetShouldSmoothFonts(ctx, FALSE);
	
//	gGame->Update(1.0f / 60.0f);
	gGame->Update(1.0f / 30.0f);
	gGame->Render();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			CGPoint location = [touch locationInView:self];
			
			Vec2F locationVec = Vec2F(location.x, location.y);
			
			mTouchMgr.TouchBegin(touch, locationVec, locationVec, locationVec);
		}
	}
	catch (Exception& e)
	{
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			mTouchMgr.TouchEnd(touch);
		}
	}
	catch (Exception& e)
	{
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			mTouchMgr.TouchEnd(touch);
		}
	}
	catch (Exception& e)
	{
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	try
	{
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
	
			CGPoint location = [touch locationInView:self];
			
			Vec2F locationVec = Vec2F(location.x, location.y);
			
			mTouchMgr.TouchMoved(touch, locationVec, locationVec, locationVec);
		}
	}
	catch (Exception& e)
	{
	}
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
	mAcceleration[0] = acceleration.x;
	mAcceleration[1] = acceleration.y;
	mAcceleration[2] = acceleration.z;
	
	gGame->mPlayer.HandleAcceleration(mAcceleration[0], mAcceleration[1], mAcceleration[2]);
}

- (void)handleTimer {
	[self setNeedsDisplay];
}


- (void)dealloc {
    [super dealloc];
}

@end
