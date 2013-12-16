#import <QuartzCore/CoreAnimation.h>
#import "AppDelegate.h"
#import "Benchmark.h"
#import "Bitmap.h"
#import "Brush_Pattern.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "MacImage.h"
#import "Tool_Brush.h"
#import "View_BrushSelect.h"
#import "View_BrushSelect_Strip.h"

@implementation View_BrushSelect_Strip

-(id)initWithLocation:(Vec2F)_location itemList:(BrushItemList*)_itemList
{
	HandleExceptionObjcBegin();
	
    if ((self = [super init])) 
	{
		[self setClearsContextBeforeDrawing:TRUE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		[self setUserInteractionEnabled:TRUE];
		[self setOpaque:FALSE];
		
		location = _location;
		itemList = _itemList;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)updateUi
{
	int sy = PREVIEW_DIAMETER;
	int sx = (PREVIEW_SIZE) * itemList->size();
	
	self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, sx, sy);
	
	[self setNeedsDisplay];
}

-(void)updateTransform:(float)scrollPosition
{
//	LOG_DBG("scroll: %f", scrollPosition);
	
	[self.layer setPosition:CGPointMake(location[0] + self.frame.size.width / 2.0f + scrollPosition, self.frame.size.height / 2.0f + location[1])];
}

- (void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();

	LOG_DBG("View_BrushSelect_Strip: drawRect", 0);
	
	Benchmark bm("View_BrushSelect_Strip: drawRect");
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();

	const float scale = [AppDelegate displayScale];
	
	Filter filter;
	
	filter.Size_set(self.frame.size.width * scale, self.frame.size.height * scale);
	
	// draw brushes
	
	for (size_t i = 0; i < itemList->size(); ++i)
	{
		Brush_Pattern* pattern = (*itemList)[i].mPattern;
		
		if (pattern)
		{
			Tool_Brush brush;
			
			brush.Setup_Pattern(PREVIEW_DIAMETER * scale, &pattern->mFilter, pattern->mIsOriented);
			
			AreaI dirty;
			
			const float x = PREVIEW_SIZE * (i + 0.5f);
			const float y = self.frame.size.height / 2.0f;
			
			brush.ApplyFilter(&filter, brush.Filter_get(), x * scale, y * scale, 0.0f, 0.0f, dirty);
		}
		else
		{
			Assert(false);
			
			// brush does not exist
		}
	}
	
	// convert render to MAC image and draw
	
	MacImage image;
	image.Size_set(self.frame.size.width * scale, self.frame.size.height * scale, true);
	filter.ToMacImage(image, Rgba_Make(4 / 255.0f, 140 / 255.0f, 203 / 255.0f));
	
	CGImageRef cgImage = image.ImageWithAlpha_get();
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, image.Sx_get() / scale, image.Sy_get() / scale), cgImage);
	CGImageRelease(cgImage);
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BrushSelect_Strip", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
