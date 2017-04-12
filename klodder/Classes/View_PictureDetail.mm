#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_PictureDetail.h"
#import "View_PictureDetailMgr.h"

@implementation View_PictureDetail

- (id)initWithFrame:(CGRect)frame controller:(View_PictureDetailMgr*)_controller imageId:(ImageId)_imageId
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:TRUE];
		[self setClearsContextBeforeDrawing:NO];
		
		controller = _controller;
		imageId = _imageId;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	const float scale = [AppDelegate displayScale];
	CGContextRef ctx = UIGraphicsGetCurrentContext();
    
	// load image preview
	
	MacImage image;
	
	Application::LoadImagePreview(imageId, image);
	
	// render image
	
	CGImageRef cgImage = image.Image_get();
	
    if (!cgImage)
    {
        LOG_WRN("no CG image", 0);
        return;
    }
    
    CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, image.Sx_get() / scale, image.Sy_get() / scale), cgImage);
	
	CGImageRelease(cgImage);
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[controller handleEdit];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_PictureDetail", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
