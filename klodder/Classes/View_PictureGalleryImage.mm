#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "View_PictureGalleryImage.h"

@implementation View_PictureGalleryImage

- (id)initWithApp:(AppDelegate*)_app imageId:(ImageId*)_imageId target:(id)_target action:(SEL)_action
{
	HandleExceptionObjcBegin();
	
	Assert(_imageId);
	
    if ((self = [super init]))
	{
		[self setOpaque:TRUE];
		[self setUserInteractionEnabled:TRUE];
		[self setClearsContextBeforeDrawing:FALSE];
		
		app = _app;
		imageId = *_imageId;
		target = _target;
		action = _action;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(ImageId*)imageId
{
	return &imageId;
}

-(void)setImageId:(ImageId *)_imageId
{
	Assert(_imageId);
	
	imageId = *_imageId;
}

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
//	CGRect fillRect = CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height);
	
	try
	{
		MacImage image;
		
		Application::LoadImageThumbnail(imageId, image, false);
		
		CGImageRef cgImage = image.Image_get();
		int x = (self.bounds.size.width - image.Sx_get()) / 2.0f;
		int y = (self.bounds.size.height - image.Sy_get()) / 2.0f;
//		CGContextDrawImage(ctx, fillRect, cgImage);
		CGContextDrawImage(ctx, CGRectMake(x, y, image.Sx_get(), image.Sy_get()), cgImage);
		CGImageRelease(cgImage);
		
		image.Size_set(0, 0, false);
		
		CGContextSetLineWidth(ctx, 2.0f);
		CGContextSetStrokeColorWithColor(ctx, [UIColor blackColor].CGColor);
		CGContextStrokeRect(ctx, self.bounds);
	}
	catch (std::exception& e)
	{
		CGContextFillRect(ctx, self.bounds);

		ExceptionLogger::Log(e);
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[target performSelector:action withObject:self];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_PictureGalleryImage", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
