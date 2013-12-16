#import "MacImage.h"
#import "Mandelbrot.h"
#import "MandelbrotView.h"

@implementation MandelbrotView

-(id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
		[self setUserInteractionEnabled:YES];
		
		zoom = 1.0f;
    }
	
    return self;
}

-(RectF)displayRect
{
	float scale = 1.0f / 100.0f * zoom;
	Vec2F size = Vec2F(self.bounds.size) * scale;
	return RectF(pan - size/2.0f, size);
}

-(void)drawRect:(CGRect)_rect 
{
	float scale = self.contentScaleFactor;
	
	MacImage image;
	
	image.Size_set(self.bounds.size.width * scale, self.bounds.size.height * scale, true);
	
	RectF rect = [self displayRect];
	
	MbRender(image, rect);
	
	CGImageRef cgImage = image.Image_get();
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextDrawImage(ctx, self.bounds, cgImage);
	CGImageRelease(cgImage);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	RectF rect = [self displayRect];
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:self];
	location.x /= self.bounds.size.width;
	location.y /= self.bounds.size.height;
	Vec2F mid;
	mid[0] = rect.m_Position[0] + rect.m_Size[0] * location.x;
	mid[1] = rect.m_Position[1] + rect.m_Size[1] * location.y;
	pan = mid;
	zoom /= 2.0f;
	
	[self setNeedsDisplay];
}

-(void)dealloc 
{
    [super dealloc];
}

@end
