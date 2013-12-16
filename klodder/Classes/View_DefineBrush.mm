#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "ExceptionLogger.h"
#import "LayerMgr.h"
#import "View_ActiveLabel.h"
#import "View_DefineBrush.h"
#import "View_EditingMgr.h"
#import "View_ToolSelectMgr.h"

@implementation View_DefineBrush

@synthesize size;
@synthesize location;

static int diameter[3] = { 15, 31, 71 };

static UIView* MakeButton(id self, SEL action, float x, float y, float sx, float sy, NSString* text, UITextAlignment alignment)
{
	/*
	UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[button setTitle:text forState:UIControlStateNormal];
	[button setFrame:CGRectMake(x, y, sx, sy)];
	[button setTitleColor:[UIColor greenColor] forState:UIControlStateNormal];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;*/
	return [[View_ActiveLabel alloc] initWithFrame:CGRectMake(x, y, sx, sy) andText:text andFont:@"Helvetica" ofSize:20.0f andAlignment:alignment andColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:0.55f] andDelegate:self andClicked:action];
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app
{
    if ((self = [super initWithFrame:frame]))
	{
		[self setMultipleTouchEnabled:FALSE];
		
		mAppDelegate = app;
		
		mCrosshair[0] = [UIImage imageNamed:@IMG("crosshair_small")];
		mCrosshair[1] = [UIImage imageNamed:@IMG("crosshair_medium")];
		mCrosshair[2] = [UIImage imageNamed:@IMG("crosshair_large")];
		
		mImage = 0;
		
		size = 0;
		location = Vec2I(160, 240); // fixme: dont assume
		
		[self addSubview:MakeButton(self, @selector(sizeSmall), 10.0f, 0.0f, 100.0f, 40.0f, @"Small", UITextAlignmentLeft)];
		[self addSubview:MakeButton(self, @selector(sizeMedium), 110.0f, 0.0f, 100.0f, 40.0f, @"Medium", UITextAlignmentCenter)];
		[self addSubview:MakeButton(self, @selector(sizeLarge), 210.0f, 0.0f, 100.0f, 40.0f, @"Large", UITextAlignmentRight)];
		
		[self addSubview:MakeButton(self, @selector(save), 10.0f, self.frame.size.height - 40.0f, 100.0f, 40.0f, @"Save", UITextAlignmentLeft)];
		[self addSubview:MakeButton(self, @selector(cancel), 210.0f, self.frame.size.height - 40.0f, 100.0f, 40.0f, @"Cancel", UITextAlignmentRight)];
    }
	
    return self;
}

-(void)handleFocus
{
	Assert(mImage == 0);
	
	mBack = [AppDelegate loadImageResource:@"back_dark_pattern.png"];
	
	Bitmap* bmp = mAppDelegate.mApplication->LayerMgr_get()->EditingBuffer_get();
	
	MacImage image;
	
//	image.Size_set(bmp->Sx_get(), bmp->Sy_get());
	
	bmp->ToMacImage_GrayAlpha(image);
//	bmp->ToMacImage(image);
	
	mImage = image.ImageWithAlpha_get();
	
	mOffset = Vec2I(self.frame.size.width - image.Sx_get(), self.frame.size.height - image.Sy_get()) / 2;
	
	LOG_DBG("offset: %d, %d", mOffset[0], mOffset[1]);
	
	[self setNeedsDisplay];
}

-(void)handleFocusLost
{
	CGImageRelease(mImage);
	mImage = 0;
	[mBack release];
}

-(void)sizeSmall
{
	[self setSize:0];
}

-(void)sizeMedium
{
	[self setSize:1];
}

-(void)sizeLarge
{
	[self setSize:2];
}

-(void)save
{
	// todo: save brush
	
	// todo: show 'brush saved' message
	
//	[mAppDelegate show:mAppDelegate.vcToolSelect];
}

-(void)cancel
{
//	[mAppDelegate transitionCurl:mAppDelegate.mViewEditing];
}

-(void)setSize:(int)_size
{
	size = _size;
	
	[self setNeedsDisplay];
}

-(void)setLocation:(Vec2I)_location
{
	location = _location;
	
	[self clampLocation];
	
	[self setNeedsDisplay];
}

-(void)clampLocation
{
	RectI rect = [self calcRect];
	
	int dx = 0;
	int dy = 0;
	
	if (rect.m_Position[0] < 0)
		dx = -rect.m_Position[0];
	if (rect.m_Position[1] < 0)
		dy = -rect.m_Position[1];
	if (rect.m_Position[0] + rect.m_Size[0] - 1 > 320 - 1) // fixme: dont assume
		dx = 320 - 1 - (rect.m_Position[0] + rect.m_Size[0] - 1);
	if (rect.m_Position[1] + rect.m_Size[1] - 1 > 400 - 1) // fixme: dont assume
		dy = 400 - 1 - (rect.m_Position[1] + rect.m_Size[1] - 1);
	
	location = location + Vec2I(dx, dy);
}

-(RectI)calcRect
{
	int radius = (diameter[size] - 1) / 2;
	
	int x1 = (int)floorf(location.x) - radius;
	int y1 = (int)floorf(location.y) - radius;
	
	return RectI(Vec2I(x1, y1), Vec2I(diameter[size], diameter[size]));
}

-(void)drawRect:(CGRect)rect 
{
	try
	{
		Assert(mImage);
		
		int radius = (diameter[size] - 1) / 2;
		
		CGContextRef ctx = UIGraphicsGetCurrentContext();
		
		// todo: draw background
		
		//[mBack drawAtPoint:CGPointMake(0.0f, 0.0f)];
		[mBack drawAsPatternInRect:self.frame];
		
		// todo: draw grayscale image w/ transparency
		
#if 0
		CGContextSaveGState(ctx);
		CGContextSetBlendMode(ctx, kCGBlendModeSourceAtop);
		int sx = CGImageGetWidth(mImage);
		int sy = CGImageGetHeight(mImage);
		CGContextDrawImage(ctx, CGRectMake(mOffset[0], mOffset[1], sx, sy), mImage);
		CGContextRestoreGState(ctx);
#endif
		
		// draw crosshair lines
		
		CGContextSetStrokeColorWithColor(ctx, [[UIColor colorWithRed:77/255.0f green:105/255.0f blue:140/255.0f alpha:0.5f] CGColor]);
		int radius2 = radius + 2;
		{
			CGPoint points[] = { CGPointMake(0.0f, mOffset[1] + location.y), CGPointMake(mOffset[0] + location.x - radius2, mOffset[1] + location.y) };
			CGContextStrokeLineSegments(ctx, points, 1);
		}
		{
			CGPoint points[] = { CGPointMake(mOffset[0] + location.x + radius2, mOffset[1] + location.y), CGPointMake(self.frame.size.width, mOffset[1] + location.y) };
			CGContextStrokeLineSegments(ctx, points, 1);
		}
		{
			CGPoint points[] = { CGPointMake(mOffset[0] + location.x, 0.0f), CGPointMake(mOffset[0] + location.x, mOffset[1] + location.y - radius2) };
			CGContextStrokeLineSegments(ctx, points, 1);
		}
		{
			CGPoint points[] = { CGPointMake(mOffset[0] + location.x, mOffset[1] + location.y + radius2), CGPointMake(mOffset[0] + location.x, self.frame.size.height) };
			CGContextStrokeLineSegments(ctx, points, 1);
		}
		
		// todo: draw crosshair
		
		UIImage* crosshair = mCrosshair[size];
		
		[crosshair drawAtPoint:CGPointMake(mOffset[0] + location.x - crosshair.size.width / 2.0f, mOffset[1] + location.y - crosshair.size.height / 2.0f)];
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
	}
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	try
	{
		UITouch* touch = [touches anyObject];
		
		CGPoint prev = [touch previousLocationInView:self];
		CGPoint curr = [touch locationInView:self];
		
		int dx = (int)Calc::RoundNearest(curr.x - prev.x);
		int dy = (int)Calc::RoundNearest(curr.y - prev.y);
		
		[self setLocation:location + Vec2I(dx, dy)];
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
	}
}

-(void)dealloc
{
	LOG_DBG("dealloc: View_DefineBrush", 0);
	
    [super dealloc];
}


@end
