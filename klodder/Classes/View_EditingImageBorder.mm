#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "StringEx.h"
#import "View_EditingImageBorder.h"

#define SHADOW_SIZE 21.0f

@implementation View_EditingImageBorder

static UIImage* mShadow(int x, int y);

-(id)initWithFrame:(CGRect)frame 
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setUserInteractionEnabled:FALSE];
		
		for (int x = 0; x < 3; ++x)
		{
			for (int y = 0; y < 3; ++y)
			{
				UIImage* image = mShadow(x, y);
				
				if (image == nil)
					images[x][y] = nil;
				else
				{
					images[x][y] = [[[UIImageView alloc] initWithImage:image] autorelease];
					images[x][y].frame = CGRectMake(0.0f, 0.0f, image.size.width, image.size.height);
					[image release];
				}
				
				[self addSubview:images[x][y]];
			}
		}
		
//		[self updateLayers:self.frame];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

static void TransformToRect(UIImageView* view, CGRect rect)
{
	Vec2F viewSize(view.image.size);
	Vec2F position = Vec2F(rect.origin) + Vec2F(rect.size) * 0.5f;
	Vec2F scale = Vec2F(rect.size) ^ Vec2F(1.0f / viewSize[0], 1.0f / viewSize[1]);

//	LOG_DBG("transform: %f, %f @ %f, %f", position[0], position[1], scale[0], scale[1]);
	
	CGAffineTransform transform = CGAffineTransformIdentity;
	
	transform = CGAffineTransformTranslate(transform, -viewSize[0] * 0.5f, -viewSize[1] * 0.5f);
	transform = CGAffineTransformTranslate(transform, position[0], position[1]);
	transform = CGAffineTransformScale(transform, scale[0], scale[1]);
	
//	[view setCenter:(viewSize * 0.5f).ToCgPoint()];
	[view setTransform:transform];
}

static void TransformToRect(UIImageView* view, CGPoint position)
{
	CGRect rect = CGRectMake(position.x, position.y, view.frame.size.width, view.frame.size.height);
	
	TransformToRect(view, rect);
}

-(void)updateLayers:(RectF)rect
{
	float x = rect.m_Position[0];
	float y = rect.m_Position[1];
	float sx = rect.m_Size[0];
	float sy = rect.m_Size[1];
	
	TransformToRect(images[0][0], CGPointMake(x-SHADOW_SIZE, y-SHADOW_SIZE));
	TransformToRect(images[2][0], CGPointMake(x + sx, y-SHADOW_SIZE));
	TransformToRect(images[0][2], CGPointMake(x-SHADOW_SIZE, y+sy));
	TransformToRect(images[2][2], CGPointMake(x+sx, y+sy));
	TransformToRect(images[1][0], CGRectMake(x, y-SHADOW_SIZE, sx, SHADOW_SIZE));
	TransformToRect(images[0][1], CGRectMake(x-SHADOW_SIZE, y, SHADOW_SIZE, sy));
	TransformToRect(images[2][1], CGRectMake(x+sx, y, SHADOW_SIZE, sy));
	TransformToRect(images[1][2], CGRectMake(x, y+sy, sx, SHADOW_SIZE));
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

static UIImage* mShadow(int x, int y)
{
	if (x == 1 && y == 1)
		return nil;
	
	return [AppDelegate loadImageResource:[NSString stringWithFormat:@"shadow_%d%d.png", x, y]];
}

@end
