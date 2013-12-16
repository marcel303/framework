#import "MainView.h"

//#define DT (1.0f / 60.0f)
#define DT (1.0f / 20.0f)

@implementation MainView

- (id)initWithFrame:(CGRect)frame
{
	if (self = [super initWithFrame:frame])
	{
		NSLog(@"initWithFrame");
	}
	
	return self;
}

- (void)layoutSubviews
{
	[self setOpaque:TRUE];
	[self setBackgroundColor:[UIColor whiteColor]];
	[self setMultipleTouchEnabled:TRUE];
	
	m_ActiveController = &m_Controller3;
	
	m_Pos = Vec2(0.0f, 0.0f);
	
	[NSTimer scheduledTimerWithTimeInterval:DT target:self selector:@selector(update) userInfo:nil repeats:YES];
}

static CGPoint ToPoint(Vec2 v)
{
	return CGPointMake(v[0], v[1]);
}

- (void)drawRect:(CGRect)rect
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextSetAllowsAntialiasing(ctx, NO);
//	CGContextSetFlatness(ctx, 1.0f);
	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGContextSetRenderingIntent(ctx, kCGRenderingIntentDefault);
	CGContextSetShouldAntialias(ctx, NO);
	
	// background
	
//	CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
//	CGContextFillRect(ctx, self.frame);

	CGContextSaveGState(ctx);
	CGContextTranslateCTM(ctx, self.frame.size.width / 2.0f, self.frame.size.height / 2.0f);
	CGContextTranslateCTM(ctx, -m_Pos[0] , -m_Pos[1]);
	
	// backdrop
	
	CGContextSaveGState(ctx);
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f, 0.1f);
	const int n = 5;
	for (int rx = -n; rx <= +n; ++rx)
		for (int ry = -n; ry <= +n; ++ry)
			CGContextStrokeRect(ctx, CGRectMake(rx * 40.0f, ry * 40.0f, 40.0f, 40.0f));
	CGContextRestoreGState(ctx);
	
	// orientation & movement
	
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 1.0f, 0.1f);
	CGContextFillEllipseInRect(ctx, CGRectMake(m_Pos[0] - DIS_MOVE_AND_ORIENT, m_Pos[1] - DIS_MOVE_AND_ORIENT, DIS_MOVE_AND_ORIENT * 2.0f, DIS_MOVE_AND_ORIENT * 2.0f));
	
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 1.0f, 0.1f);
	CGContextFillEllipseInRect(ctx, CGRectMake(m_Pos[0] - DIS_ORIENT, m_Pos[1] - DIS_ORIENT, DIS_ORIENT * 2.0f, DIS_ORIENT * 2.0f));
	
	CGContextBeginPath(ctx);
	float fov = M_PI / 4.0f;
	float angle0 = m_ActiveController->Orientation_get().ToAngle();
	float angle1 = angle0 - fov / 2.0f;
	float angle2 = angle0 + fov / 2.0f;
	Vec2 p1 = m_Pos;
	Vec2 p2 = p1.Add(Vec2::FromAngle(angle1).Scale(100.0f));
	Vec2 p3 = p1.Add(Vec2::FromAngle(angle2).Scale(100.0f));
	CGPoint point1 = ToPoint(p1);
	CGPoint point2 = ToPoint(p2);
	CGPoint point3 = ToPoint(p3);
	CGPoint lines[] = { point1, point2, point2, point3, point3, point1 };
	CGContextAddLines(ctx, lines, 6);
	CGContextClosePath(ctx);
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 1.0f, 0.1f);
	CGContextSetRGBStrokeColor(ctx, 0.0f, 0.0f, 0.0f, 0.5f);
	CGContextDrawPath(ctx, kCGPathFillStroke);
	
	//
	
	CGContextRestoreGState(ctx);
	
	// controllers
	
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 1.0f, 0.1f);
	Analog* a1 = &m_Controller2.m_Analog1;
	Analog* a2 = &m_Controller2.m_Analog2;
	CGContextFillEllipseInRect(ctx, CGRectMake(a1->m_Pos[0] - a1->m_Radius, a1->m_Pos[1] - a1->m_Radius, a1->m_Radius * 2.0f, a1->m_Radius * 2.0f));
	CGContextFillEllipseInRect(ctx, CGRectMake(a2->m_Pos[0] - a2->m_Radius, a2->m_Pos[1] - a2->m_Radius, a2->m_Radius * 2.0f, a2->m_Radius * 2.0f));
	
	CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
	[[NSString stringWithCString:m_ActiveController->Name_get()] drawAtPoint:CGPointMake(0.0f, 0.0f) withFont:[UIFont fontWithName:@"arial" size:12.0f]];
}

- (void)update
{
	float speed = m_ActiveController->MovementSpeed_get();
	
	m_Pos = m_Pos.Add(m_ActiveController->MovementDirection_get().Scale(speed * DT));
	
	[self setNeedsDisplay];
}

- (void)toggle
{
	IController* active;
	
	if (m_ActiveController == &m_Controller1)
		active = &m_Controller2;
	else if (m_ActiveController == &m_Controller2)
		active = &m_Controller3;
	else if (m_ActiveController == &m_Controller3)
		active = &m_Controller1;
	
	[self select:active];
}

- (void)select:(IController*)controller
{
	m_ActiveController = controller;
}

- (void)benchmark
{
	const int count = 1000000;
	const int reps = 10;
	
	short* bytes = new short[count];
	
	float t1;
	float t2;
	
	double t0 = CFAbsoluteTimeGetCurrent();
	
	t1 = CFAbsoluteTimeGetCurrent() - t0;
	for (volatile int j = 0; j < reps; ++j)
	for (volatile int i = 0; i < count; ++i)
		bytes[i] = i;
	t2 = CFAbsoluteTimeGetCurrent() - t0;
	
	float d1 = t2 - t1;
	
	t1 = CFAbsoluteTimeGetCurrent() - t0;
	volatile short x;
	for (volatile int j = 0; j < reps; ++j)
	for (volatile int i = 0; i < count; ++i)
		x += bytes[i];
	t2 = CFAbsoluteTimeGetCurrent() - t0;
	
	float d2 = t2 - t1;
	
	delete bytes;
	
	UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"benchmark" message:[NSString stringWithFormat:@"d1: %fMS, d2: %fMS",  d1 * 1000.0f, d2 * 1000.0f] delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
	[alert show];
	[alert release];
}

static Vec2 ToVec2(CGPoint point)
{
	return Vec2(point.x, point.y);
}

static UITouch* g_Touches[5] = { 0, 0, 0, 0, 0 };

static int AllocFinger(UITouch* touch)
{
	for (int i = 0; i < 5; ++i)
	{
		if (g_Touches[i])
			continue;
		
		g_Touches[i] = touch;
		return i;
	}
	
	return -1;
}

static void FreeFinger(int finger)
{
	g_Touches[finger] = 0;
}

static int GetFinger(UITouch* touch)
{
	for (int i = 0; i < 5; ++i)
		if (touch == g_Touches[i])
			return i;
	
	return -1;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	for (int i = 0; i < [touches count]; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		if ([touch tapCount] == 2)
		{
			[self toggle];
		}
		
		if ([touch tapCount] == 3)
		{
			[self benchmark];
		}
		
		int finger = GetFinger(touch);
		
		if (finger < 0)
			finger = AllocFinger(touch);
		
		Vec2 pos = ToVec2([touch locationInView:self]);
	
		m_ActiveController->TouchBegin(finger, pos);
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	for (int i = 0; i < [touches count]; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		int finger = GetFinger(touch);
		
		Vec2 pos = ToVec2([touch locationInView:self]);
		
		m_ActiveController->TouchMove(finger, pos);
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	for (int i = 0; i < [touches count]; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		int finger = GetFinger(touch);
		
		m_ActiveController->TouchEnd(finger);
		
		FreeFinger(finger);
	}
}

- (void)dealloc
{
	[super dealloc];
}

@end
