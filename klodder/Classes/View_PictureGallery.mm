#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "KlodderSystem.h"
#import "Log.h"
#import "View_EditingMgr.h"
#import "View_PictureGallery.h"
#import "View_PictureGalleryImage.h"
#import "View_PictureGalleryMgr.h"

@implementation View_PictureGallery

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(View_PictureGalleryMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:TRUE];
		[self setBackgroundColor:[UIColor colorWithRed:1.0f green:1.0f blue:0.9f alpha:1.0f]];
//		[self setBackgroundColor:[UIColor colorWithRed:0.2f green:0.3f blue:0.5f alpha:1.0f]];
//		[self setContentMode:UIViewContentModeRedraw];
//		[self setNeedsLayout];
//		[self setAutoresizingMask:UIViewAutoresizingFlexibleHeight];
//		[self setAutoresizesSubviews:YES];
//		[self setContentMode:UIViewContentModeScaleAspectFit];
		
		app = _app;
		controller = _controller;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)scan
{
	// remove old scroll view (if any)
	
	[scroll removeFromSuperview];
	
	// create new scroll view
	
//	const float spaceY1 = 50.0f;
//	const float spaceY2 = 50.0f;
//	const float spaceY1 = 0.0f;
//	const float spaceY2 = 0.0f;
	
//	scroll = [[[UIScrollView alloc] initWithFrame:CGRectMake(0.0f, spaceY1, self.frame.size.width, self.frame.size.height - spaceY1 - spaceY2)] autorelease];
	scroll = [[[UIScrollView alloc] initWithFrame:self.bounds] autorelease];
	[scroll setOpaque:FALSE];
	[scroll setBackgroundColor:[UIColor clearColor]];
	[self addSubview:scroll];
	[self sendSubviewToBack:scroll];
	[scroll setAutoresizingMask:UIViewAutoresizingFlexibleHeight];
	
	// scan for drawings
	
	NSArray* fileNameList = [AppDelegate scanForPictures:gSystem.GetDocumentPath(".").c_str()];
	
	// add image view for each drawing
	
	for (NSUInteger i = 0; i < [fileNameList count]; ++i)
	{
		NSString* fileName = [fileNameList objectAtIndex:i];
		
		try
		{
			ImageId imageId([fileName cStringUsingEncoding:NSASCIIStringEncoding]);
			
			[self addImage:imageId];
		}
		catch (std::exception& e)
		{
			LOG_ERR(e.what(), 0);
		}
	}
	
	[scroll setClipsToBounds:FALSE];
	
	[self setNeedsLayout];
}

-(void)addImage:(ImageId)imageId
{
//	float yOffset = 50.0f;
	float yOffset = 0.0f;
	
	CGRect screenRect = [UIScreen mainScreen].bounds;
	
	//int sx = 90;
	//int sy = 90 * 480 / 320;
	int sx = screenRect.size.width / 4;
	int sy = screenRect.size.height / 4;
	
	int nx = 3;
	
	int dx = 10;
	int dy = 10;
	int ox = 15;
	int oy = 15;
	
	int i = (int)[scroll subviews].count;
	
	int ix = i % nx;
	int iy = i / nx;
	
	int x = ox + ix * (sx + dx);
	int y1 = oy + iy * (sy + dy) + yOffset;
	int y2 = oy + (iy + 1) * (sy + dy) + yOffset;
	
	View_PictureGalleryImage* imageView = [[[View_PictureGalleryImage alloc] initWithApp:app imageId:&imageId target:controller action:@selector(pictureSelected:)] autorelease];
	
	imageView.frame = CGRectMake(x, y1, sx, sy);
	
	[scroll addSubview:imageView];
	
	if (y2 > scroll.contentSize.height)
		scroll.contentSize = CGSizeMake(scroll.frame.size.width, y2);
}

-(void)scrollIntoView:(ImageId)imageId
{
	Assert(imageId.IsSet_get());
	
	if (!imageId.IsSet_get())
	{
		[self scrollToBottom];
		return;
	}
	
	for (NSUInteger i = 0; i < scroll.subviews.count; ++i)
	{
		NSObject* obj = [scroll.subviews objectAtIndex:i];
		
		if (![obj isMemberOfClass:[View_PictureGalleryImage class]])
			continue;
		
		View_PictureGalleryImage* image = (View_PictureGalleryImage*)obj;
		
		if (*image.imageId == imageId)
		{
			[scroll scrollRectToVisible:image.frame animated:FALSE];
		}
	}
}

-(void)scrollToBottom
{
	CGPoint contentOffset = CGPointMake(0, [scroll contentSize].height - scroll.bounds.size.height - self.bounds.size.height);
	
	if (contentOffset.y < 0.0f)
		contentOffset.y = 0.0f;

	[scroll setContentOffset:contentOffset];
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_PictureGallery", 0);
	
	[[NSNotificationCenter defaultCenter] removeObserver:self name:@"LocalhostAdressesResolved" object:nil];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
