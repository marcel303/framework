#import "DrawingView.h"

#define SX 320
#define SY 480
//#define BLUR_SPEED 1
#define BLUR_SPEED 2
//#define BLUR_SPEED 3
#define FINGER_SIZE 131
#define MAX_VALUE (1024 + 200)

class Buffer
{
public:
	Buffer(int sx, int sy)
	{
		m_Sx = 0;
		m_Sy = 0;
		m_Data = 0;
		
		Initialize(sx, sy);
		
		Clear(0);
	}
	
	~Buffer()
	{
		Shutdown();
	}
	
	void Initialize(int sx, int sy)
	{
		if (m_Data)
		{
			delete[] m_Data;
			m_Data = 0;
			m_Sx = 0;
			m_Sy = 0;
		}
		
		int area = sx * sy;
		
		if (area == 0)
			return;
		
		m_Data = new short[area];
		m_Sx = sx;
		m_Sy = sy;
	}
			
	void Shutdown()
	{
		Initialize(0, 0);
	}
	
	void Clear(int value)
	{
		int area = m_Sx * m_Sy;
		
		for (int i = 0; i < area; ++i)
			m_Data[i] = value;
	}
	
	void SetValue(int x, int y, int value)
	{
		if (x < 0 || y < 0 || x >= m_Sx || y >= m_Sy)
			return;
		
		if (value > MAX_VALUE)
			value = MAX_VALUE;
		
		int index = x + y * m_Sx;
		
		m_Data[index] = value;
	}
	
	int GetValue(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= m_Sx || y >= m_Sy)
			return 0;
		
		int index = x + y * m_Sx;
		
		return m_Data[index];
	}

	void Box()
	{
		for (int x = 0; x < m_Sx; ++x)
		{
			SetValue(x, 0, 0);
			SetValue(x, m_Sy - 1, 0);
		}
		
		for (int y = 0; y < m_Sy; ++y)
		{
			SetValue(0, y, 0);
			SetValue(m_Sx - 1, y, 0);
		}
	}
	
	void Fire()
	{
		for (int i = 0; i < 5; ++i)
		{
			int fireX = rand() % m_Sx;
			
			for (int x = 1; x < m_Sx - 1; ++x)
			{
				int index = x + (m_Sy - 2) * m_Sx;
				
				int distance = x - fireX;
				
				if (distance < 0)
					distance = -distance;
				
				int v = 255 - distance * 25;
				
				if (v < 0)
					v = 0;
				
//				v <<= 3;
				v <<= 4;

				m_Data[index] += v;
			}
		}
	}
	
	void LinearFade()
	{
		int area = m_Sx * m_Sy;
		
		for (int i = 0; i < area; ++i)
		{
			int c = m_Data[i] - BLUR_SPEED;
			
			if (c < 0)
				c = 0;
			if (c > 1023)
				c = 1023;
			
			m_Data[i] = c;
		}
	}
	
	void FireFade()
	{
		for (int y = 1; y < m_Sy - 1; ++y)
		{
			int index0 = 1 + (y - 1) * m_Sx;
			int index1 = 1 + (y + 0) * m_Sx;
			int index2 = 1 + (y + 1) * m_Sx;
			
			int i = m_Sx - 2;
			
#define FIRE_BLUR \
	{ \
		/*short v = ((m_Data[index2] << 1) + m_Data[index1 - 1] + m_Data[index1 + 1]) >> 2;*/ \
		short v = (m_Data[index0] + m_Data[index2] + m_Data[index1 - 1] + m_Data[index1 + 1]) >> 2; \
		if (v >= BLUR_SPEED) \
			v -= BLUR_SPEED; \
		m_Data[index1] = v; \
		++index0; \
		++index1; \
		++index2; \
	}
			
#if 1
			for (; i >= 4; i -= 4)
			{
				FIRE_BLUR;
				FIRE_BLUR;
				FIRE_BLUR;
				FIRE_BLUR;
			}
			
			for (; i >= 1; i -= 1)
			{
				FIRE_BLUR;
			}
#else
			for (int i = 0; i < m_Sx; ++i)
			{
				FIRE_BLUR;
			}
#endif
		}
	}
	
	int m_Sx;
	int m_Sy;
	short* m_Data;
};

@implementation DrawingView

//static Buffer g_Buffer(320, 480);
static Buffer g_Buffer(SX, SY);
static int32_t g_Colors[1024];

static int g_Frame = 0;
static int g_FpsFrame = 0;
static int g_Fps = 0;

static int32_t* g_BitmapData;
static CGImageRef g_BitmapImage;

static short g_Finger[FINGER_SIZE][FINGER_SIZE];

class TouchInfo
{
public:
	TouchInfo()
	{
		down = false;
	}
	
	bool down;
	int x;
	int y;
};

static TouchInfo g_TouchDown[5];

- (id)initWithFrame:(CGRect)frame
{
	if (self = [super initWithFrame:frame])
	{
		[self setUserInteractionEnabled:TRUE];
		[self setMultipleTouchEnabled:TRUE];
		[self setOpaque:TRUE];
		[self setContentMode:UIViewContentModeRedraw];
		
		g_Buffer.Clear(0);
		g_BitmapData = new int32_t[g_Buffer.m_Sx * g_Buffer.m_Sy];
		
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		g_BitmapImage = CGImageCreate(
			g_Buffer.m_Sx,
			g_Buffer.m_Sy, 8, 32, g_Buffer.m_Sx * 4,
			colorSpace,
			0,
			CGDataProviderCreateWithData(0, g_BitmapData, g_Buffer.m_Sx * g_Buffer.m_Sy * 4, 0),
			0, FALSE, kCGRenderingIntentDefault);
		CGColorSpaceRelease(colorSpace);
		
		for (int i = 0; i < 1024; ++i)
		{
#define MAKECOL(r, g, b) ((int)(r) | ((int)(g) << 8) | ((int)(b) << 16) | (255 << 24))
//			g_Colors[i] = MAKECOL(i, i >> 2, i >> 3);
			float t = i / 1023.0f;
			g_Colors[i] = MAKECOL(
 				(-cos(2.0f * M_PI * t * 0.5f) + 1.0f) / 2.0f * 255.0f,
 				(-cos(2.0f * M_PI * t * 0.9f) + 1.0f) / 2.0f * 255.0f,
				(-cos(2.0f * M_PI * t * 1.05f) + 1.0f) / 2.0f * 255.0f);
		}
		
		for (int x = 0; x < FINGER_SIZE; ++x)
		{
			for (int y = 0; y < FINGER_SIZE; ++y)
			{
				int dx = x - (FINGER_SIZE - 1) / 2;
				int dy = y - (FINGER_SIZE - 1) / 2;
				
				float length = sqrtf(dx * dx + dy * dy) / ((FINGER_SIZE - 1) / 2);
				
				length = 1.0f - length;
				
				if (length < 0.0f)
					length = 0.0f;
				
				int value = length * 100;
				
				g_Finger[x][y] = value;
			}
		}
		
		// Initialization code
		[NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(HandleTimer) userInfo:NULL repeats:YES];
//		[NSTimer scheduledTimerWithTimeInterval:0.002f target:self selector:@selector(HandleTimer) userInfo:NULL repeats:YES];
		[NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(HandleTimerFPS) userInfo:NULL repeats:YES];
		
		[self setNeedsDisplay];
	}

	return self;
}

- (void)touchesBegan:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	
	for (int i = 0; i < touches.count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		CGPoint location = [touch locationInView:self];
		
		if (touch.phase == UITouchPhaseEnded || touch.phase == UITouchPhaseCancelled)
			continue;
		
		g_TouchDown[i].down = true;
		g_TouchDown[i].x = location.x;
		g_TouchDown[i].y = location.y;
	}
}

- (void)applyFinger:(int)lx y:(int)ly
{
	int offset = - (FINGER_SIZE - 1) / 2;

	for (int x = 0; x < FINGER_SIZE; ++x)
	{
		for (int y = 0; y < FINGER_SIZE; ++y)
		{
			int value = g_Finger[x][y];
			
			int fx = x + offset + lx;
			int fy = y + offset + ly;
			
			g_Buffer.SetValue(fx, fy, g_Buffer.GetValue(fx, fy) + value);
		}
	}
}

- (void)touchesMoved:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	
	int touchCount = [touches count];
	
	for (int i = 0; i < touchCount; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		CGPoint location = [touch locationInView:self];
		
		g_TouchDown[i].x = location.x;
		g_TouchDown[i].y = location.y;
		
#if 0
		int size = 40;
		
		for (int x = -size; x <= +size; ++x)
			for (int y = -size; y <= +size; ++y)
			{
				int bx = location.x + x;
				int by = location.y + y;
				
				float length = sqrtf(x * x + y * y);
				
				length /= size;
				
				length = 1.0f - length;

				int value = length * 150;
				
#if 1
				if (value < 0)
					value = 0;
#endif
				
//				int value = size * 2 - abs(x) - abs(y);
				
//				value *= 5;
//				value *= 2;
				
//				g_Buffer.SetValue(location.x + x, location.y + y, 2000);
				g_Buffer.SetValue(bx, by, g_Buffer.GetValue(bx, by) + value);
			}
#else

#endif
	}
}

- (void)touchesEnded:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	
	for (int i = 0; i < touches.count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];

		if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
			continue;
		
		g_TouchDown[i].down = false;
	}
}

- (void)touchesCancelled:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	
	for (int i = 0; i < touches.count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled)
			continue;
		
		g_TouchDown[i].down = false;
	}
}

- (void)HandleTimer
{
	for (int i = 0; i < 5; ++i)
	{
		if (!g_TouchDown[i].down)
			continue;
		
		[self applyFinger:g_TouchDown[i].x y:g_TouchDown[i].y];
	}
	
	[self setNeedsDisplay];
	
	g_FpsFrame++;
}

- (void)HandleTimerFPS
{
	g_Fps = g_FpsFrame;
	g_FpsFrame = 0;
}

- (void)drawRect:(CGRect)rect {

	const int iterations = 1;
	
	for (int i = 0; i < iterations; ++i)
	{
//		g_Buffer.Fire();
		g_Buffer.LinearFade();
//		g_Buffer.FireFade();
	}

	g_Buffer.Box();
	
#if 1
	const short* srcPtr = g_Buffer.m_Data;
	int32_t* dstPtr = g_BitmapData;
	const int count = g_Buffer.m_Sx * g_Buffer.m_Sy;
	
	for (int i = 0; i < count; ++i)
	{
		int index = srcPtr[i];
		
		if (index > 1023)
			index = 1023;
		
		//index &= 1023;
		
		dstPtr[i] = g_Colors[index];
	}
	
	//
	
	UIImage* uiImage = [[UIImage alloc] initWithCGImage:g_BitmapImage];
	[uiImage drawAtPoint:CGPointMake(0.0f, 0.0f)];
//	[uiImage drawInRect:CGRectMake(0.0f, 0.0f, 320, 480)];
	[uiImage release];
	
#endif
	
#if 0
	NSString* string = [NSString stringWithCString:"Hello World"];
	
	[[UIColor whiteColor] set];
	
	UIFont* font = [UIFont systemFontOfSize:16.0f];
	
	[string drawAtPoint:CGPointMake(0.0f, 00.0f) withFont:font];
#endif
	
#if 0
	CGContextRef context = UIGraphicsGetCurrentContext();
	
	CGContextSetRGBFillColor(context, 255, 0, 0, 0.1);
	CGContextFillRect(context, CGRectMake(0, 0, g_Buffer.m_Sx / 3, g_Buffer.m_Sy));
	CGContextSetRGBFillColor(context, 0, 255, 0, 0.1);
	CGContextFillRect(context, CGRectMake(g_Buffer.m_Sx * 2 / 3, 0, g_Buffer.m_Sx / 3, g_Buffer.m_Sy));
	CGContextFillEllipseInRect(context, CGRectMake(100, 100, 25, 25));
	CGContextSetRGBStrokeColor(context, 255, 255, 0, 1);
	 CGContextStrokeRect(context, CGRectMake(195, 195, 60, 60));
	CGContextSetRGBStrokeColor(context, 0, 0, 255, 1);
	CGContextStrokeEllipseInRect(context, CGRectMake(200, 200, 50, 50));
	
	CGContextSetRGBStrokeColor(context, 255, 0, 255, 1);
	CGPoint points[6] =
	{
		CGPointMake(100, 200), CGPointMake(150, 250),
		CGPointMake(150, 250), CGPointMake(50, 250),
		CGPointMake(50, 250), CGPointMake(100, 200)
	};
	CGContextStrokeLineSegments(context, points, 6);
	
	CGContextSetRGBFillColor(context, 0, 0, 100, 0.2);
	CGContextFillEllipseInRect(context, CGRectMake(0, 100, (g_Frame * 1) % 320, 50));
#endif
	
#if 1
	[[UIColor whiteColor] set];
	
	[[NSString stringWithFormat:@"%d FPS", g_Fps]
		drawInRect:CGRectMake(0, 100, 320, 50)
		withFont:[UIFont fontWithName:@"Marker Felt" size:14]
		lineBreakMode:UILineBreakModeMiddleTruncation
		alignment:UITextAlignmentCenter];
#endif
	
	++g_Frame;
}


- (void)dealloc
{
	CGImageRelease(g_BitmapImage);
	delete[] g_BitmapData;
	
	[super dealloc];
}


@end
