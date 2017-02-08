#import "AppDelegate.h"
#import "Application.h"
#import "BlitTransform.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_ImagePlacement.h"
#import "View_ImagePlacementMgr.h"

//#define ANGLE_STEP (Calc::m2PI / 8.0f)
#define ANGLE_STEP (Calc::m2PI / 20.0f)

@implementation View_ImagePlacementMgr

@synthesize image;
@synthesize dataLayer;
@synthesize location;

-(id)initWithImage:(UIImage*)_image size:(Vec2I)_size dataLayer:(int)_dataLayer app:(AppDelegate*)_app delegate:(id<ImagePlacementDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithApp:_app]))
	{
		self.title = @"Image placement";
		self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(handleDone)] autorelease];
		
		UIBarButtonItem* item_RotateLeft = [[[UIBarButtonItem alloc] initWithTitle:@"L" style:UIBarButtonItemStylePlain target:self action:@selector(handleRotateLeft)] autorelease];
		slider = [[[UISlider alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 30.0f)] autorelease];
		[slider setMinimumValue:0.0f];
		[slider setMaximumValue:1.0f];
		[slider setValue:0.5f];
		[slider addTarget:self action:@selector(handleZoomChanged) forControlEvents:UIControlEventTouchUpInside|UIControlEventValueChanged];
		[slider setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		UIBarButtonItem* item_Zoom = [[[UIBarButtonItem alloc] initWithCustomView:slider] autorelease];
		UIBarButtonItem* item_RotateRight = [[[UIBarButtonItem alloc] initWithTitle:@"R" style:UIBarButtonItemStylePlain target:self action:@selector(handleRotateRight)] autorelease];
		UIBarButtonItem* item_Space = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil] autorelease];
		[self setToolbarItems:[NSArray arrayWithObjects:item_RotateLeft, item_Space, item_Zoom, item_Space, item_RotateRight, nil]];
		
		self.image = _image;
		size = _size;
		location = size / 2;
		dataLayer = _dataLayer;
		delegate = _delegate;
		angle = 0.0f;
		zoom = 1.0f;
		
		if ([self scale] < 1.0f)
		{
			// analyze image and rotate if needed
		
			float aspect1 = image.size.width / image.size.height;
			float aspect2 = size[0] / (float)size[1];
		
			if (aspect1 > aspect2)
			{
				angle = +Calc::mPI2;
			}
		}
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleRotateLeft
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ImagePlacementMgr: rotate left", 0);
	
	angle -= ANGLE_STEP;
	
	View_ImagePlacement* vw = (View_ImagePlacement*)self.view;
	[vw updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleRotateRight
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ImagePlacementMgr: rotate right", 0);
	
	angle += ANGLE_STEP;
	
	View_ImagePlacement* vw = (View_ImagePlacement*)self.view;
	[vw updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleZoomChanged
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ImagePlacementMgr: zoom changed", 0);
	
	zoom = [slider value] * 2.0f;
	
	View_ImagePlacement* vw = (View_ImagePlacement*)self.view;
	[vw updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDone
{
	LOG_DBG("View_ImagePlacementMgr: done", 0);
	
	UIImage* temp = image;
	
	BlitTransform transform;
	[self blitTransform:transform];
	
	[delegate placementFinished:temp dataLayer:dataLayer transform:&transform controller:self];
}

-(void)handleAdjustmentBegin
{
	// hide menu
	
	[self.navigationController setToolbarHidden:TRUE animated:TRUE];
	[self.navigationController setNavigationBarHidden:TRUE animated:TRUE];
}

-(void)handleAdjustmentEnd
{
	// show menu
	
	[self.navigationController setToolbarHidden:FALSE animated:TRUE];
	[self.navigationController setNavigationBarHidden:FALSE animated:TRUE];
}

-(void)setLocation:(Vec2I)_location
{
	LOG_DBG("View_ImagePlacementMgr: set location", 0);
	
	location = _location;
	
	View_ImagePlacement* vw = (View_ImagePlacement*)self.view;
	[vw updateUi];
}

-(float)angle
{
	return angle;
}

static float GetScale(Vec2F size, Vec2F maxSize)
{
	const float scaleX = Calc::Min(maxSize[0] / (float)size[0], 1.0f);
	const float scaleY = Calc::Min(maxSize[1] / (float)size[1], 1.0f);
	
	return Calc::Max(scaleX, scaleY);
}

-(float)scale
{
	const Vec2I maxSize1 = app.mApplication->LayerMgr_get()->Size_get();
	const Vec2F maxSize2(maxSize1[0], maxSize1[1]);
	const Vec2F size1(image.size.width, image.size.height);
	const Vec2F size2(image.size.height, image.size.width);
	const float scale1 = GetScale(size1, maxSize2);
	const float scale2 = GetScale(size2, maxSize2);
	const float scale = Calc::Min(scale1, scale2);

	return scale * zoom;
}

-(Vec2I)snapPan
{
	Vec2I result = location - size / 2;
	
	if (abs(result[0]) < 5)
		result[0] = 0;
	if (abs(result[1]) < 5)
		result[1] = 0;
	
	return result + size / 2;
}

-(CGAffineTransform)imageTransform
{
	BlitTransform blitTransform;
	[self blitTransform:blitTransform];
	
	CGAffineTransform transform = CGAffineTransformIdentity;
	
	transform = CGAffineTransformTranslate(transform, blitTransform.x, blitTransform.y);
	transform = CGAffineTransformRotate(transform, blitTransform.angle);
	transform = CGAffineTransformScale(transform, blitTransform.scale, blitTransform.scale);
	
	return transform;
}

-(CGAffineTransform)pivotTransform
{
	BlitTransform blitTransform;
	[self blitTransform:blitTransform];
	
	CGAffineTransform transform = CGAffineTransformIdentity;
	
	transform = CGAffineTransformTranslate(transform, blitTransform.x, blitTransform.y);
	transform = CGAffineTransformRotate(transform, blitTransform.angle);
	
	return transform;
}

-(void)blitTransform:(BlitTransform&)out_Transform
{
	Vec2I pan = [self snapPan];
	
	out_Transform.anchorX = image.size.width / 2.0f;
	out_Transform.anchorY = image.size.height / 2.0f;
	out_Transform.angle = [self angle];
	out_Transform.scale = [self scale];
	out_Transform.x = pan[0];
	out_Transform.y = pan[1];
}

#include "ImageResampling.h"

-(UIImage*)DBG_renderFinalImage
{
	BlitTransform transform;
	[self blitTransform:transform];
	MacImage* srcImage = [AppDelegate uiImageToMacImage:image];;
	MacImage dstImage;
	dstImage.Size_set(size[0], size[1], true);
	ImageResampling::Blit_Transformed(*srcImage, dstImage, transform);
	delete srcImage;
	
	return [AppDelegate macImageToUiImage:&dstImage];
}

-(void)loadView 
{
	HandleExceptionObjcBegin();
	
	self.view = [[[View_ImagePlacement alloc] initWithFrame:[UIScreen mainScreen].bounds controller:self] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self setMenuTransparent];
	
	View_ImagePlacement* vw = (View_ImagePlacement*)self.view;
	
	[vw updateUi];
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	self.image = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
