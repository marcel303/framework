#import <QuartzCore/CALayer.h>
#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "Events.h"
#import "ExceptionLoggerObjC.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_BrushSettings.h"
#import "View_Editing.h"
#import "View_EditingMgr.h"
#import "View_EditingImage.h"
#import "View_Eyedropper.h"
#import "View_ZoomInfo.h"

#define DOUBLETAP_INTERVAL 0.3f
#define EYEDROPPER_WAIT 0.63f
#define TRANSFORM_ANIMATION_INTERVAL (1.0f / 60.0f)
#define MIN_ZOOM 0.7f
#define MAX_ZOOM 24.0f

@implementation View_EditingImage

@synthesize pan;
@synthesize targetPan;
@synthesize zoom;
@synthesize targetZoom;
@synthesize mirrorX;

static void handleTouchZoomStateChange(void* obj, void* arg);
static void handleZoomChange(void* obj, void* arg);

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_EditingMgr*)_controller parent:(View_Editing*)_parent;
{
	HandleExceptionObjcBegin();
	
	const float scale = [AppDelegate displayScale];
	Vec2I size = app.mApplication->LayerMgr_get()->Size_get() / scale;
	
	Assert(size[0] > 0);
	Assert(size[1] > 0);
	
	frame.size.width = size[0];
	frame.size.height = size[1];
	
	LOG_DBG("editing size: %dx%d", size[0], size[1]);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setUserInteractionEnabled:FALSE];
		[self setOpaque:TRUE];
//		[self setMultipleTouchEnabled:TRUE];
		[self setClearsContextBeforeDrawing:NO];
//		[self setAutoresizingMask:UIViewAutoresizingNone];
//		[self setAutoresizesSubviews:FALSE];
		
		mAppDelegate = app;
		controller = _controller;
		parent = _parent;
		
		[self setPan:Vec2F(0.0f, 0.0f) animated:FALSE];
		[self setZoom:1.0f animated:FALSE];
		mirrorX = false;
		
		doubletapTimer = nil;
		doubletapState = 0;
		eyedropperTimer = nil;
		transformTimer = nil;
		
		undoArea.Setup(Vec2F(0.0f, frame.size.height - 40.0f), Vec2F(40.0f, 40.0f));
		redoArea.Setup(Vec2F(frame.size.width - 40.0f, frame.size.height - 40.0f), Vec2F(40.0f, 40.0f));
		sizeArea.Setup(Vec2F(frame.size.width - 40.0f, 0.0f), Vec2F(40.0f, 40.0f));
		
		// register touch listener
		
		mTouchMgr.Setup(self);
		
		mTouchZoom.OnStateChange = CallBack(self, handleTouchZoomStateChange);
		mTouchZoom.OnZoomChange = CallBack(self, handleZoomChange);
		
		[[NSNotificationCenter defaultCenter]
			addObserver:self
			selector:@selector(handleLayersChanged:)
			name:EVT_LAYERS_CHANGED
			object:nil];
		
		[self setNeedsDisplay];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleFocus
{
	LOG_DBG("handleFocus", 0);
	
	[self updatePaint];
	
	[parent updateImageBorder];
}

-(void)setPan_Direct:(Vec2F)_pan
{
	pan = _pan;
	
	//
	
	[self updateTransform];
	
	[parent updateImageBorder];
}

-(void)setPan:(Vec2F)pan
{
	throw ExceptionNA();
}

-(void)setPan:(Vec2F)_pan animated:(BOOL)animated
{
	if (animated)
	{
		targetPan = _pan;
		[self transformAnimationBegin];
	}
	else
	{
		[self transformAnimationEnd];
		[self setPan_Direct:_pan];
	}
}

-(void)setZoom_Direct:(float)_zoom
{
	zoom = _zoom;
	
	//
	
	[self updateTransform];
	
	if (zoom >= 1.1f && zoom < 2.5f)
		[self.layer setMagnificationFilter:kCAFilterLinear];
	else
		[self.layer setMagnificationFilter:kCAFilterNearest];
	
	parent.zoomIndicator.zoom = [self snapZoom:zoom];
	
	[parent updateImageBorder];

}

-(void)setZoom:(float)zoom
{
	throw ExceptionNA();
}

-(void)setZoom:(float)_zoom animated:(BOOL)animated
{
	_zoom = Calc::Mid(_zoom, MIN_ZOOM, MAX_ZOOM);
	
	if (animated)
	{
		targetZoom = _zoom;
		[self transformAnimationBegin];
	}
	else
	{
		[self transformAnimationEnd];
		[self setZoom_Direct:_zoom];
	}
}

-(void)transformToCenter:(bool)animated
{
	[self setPan:Vec2F(0.0f, 0.0f) animated:animated];
	[self setZoom:1.0f animated:TRUE];
}

-(void)touchBegin:(TouchInfoV2*)ti
{
	mTouchZoom.Begin(ti->finger, ti->location, ti->location2);
}

-(void)touchEnd:(TouchInfoV2*)ti
{
	mTouchZoom.End(ti->finger);
}

// --------------------
// Double-tap behaviour
// --------------------

-(void)doubletapTimerBegin
{
	doubletapTimer = [[NSTimer scheduledTimerWithTimeInterval:DOUBLETAP_INTERVAL target:self selector:@selector(doubletapTimerElapsed) userInfo:nil repeats:FALSE] retain];
}

-(void)doubletapTimerEnd
{
	if (doubletapTimer == nil)
		return;
	
	[doubletapTimer invalidate];
	[doubletapTimer release];
	doubletapTimer = nil;
}

-(void)doubletapTimerElapsed
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("double-tap timer elapsed", 0);
	
	[self doubletapFlush:doubletapState == 2];
	[self doubletapTimerEnd];
	doubletapState = 0;
	
	HandleExceptionObjcEnd(false);
}

-(void)doubletapFlush:(bool)includeEnd
{
	Assert(doubletapState != 0);
	
	[self handleTouchBegin:doubletapLocationScreen1t location1:doubletapLocationScreen1a location2:doubletapLocationScreen1b];
	
	if (includeEnd)
	{
		[self handleTouchEnd:doubletapLocationScreen1t];
	}
}

// --------------------
// Eyedropper behaviour
// --------------------

-(void)updateEyedropperLocationAndColor:(Vec2F)touchLocation
{
	CGPoint location = [self viewToScreen:touchLocation].ToCgPoint();
	
	Vec2F imageLocation = [self viewToImage:touchLocation];
	
	Rgba color = mAppDelegate.mApplication->GetColorAtLocation(imageLocation);
	
	UIColor* uiColor = [UIColor colorWithRed:color.rgb[0] green:color.rgb[1] blue:color.rgb[2] alpha:color.rgb[3]];
	
	[controller setEyedropperLocation:location color:uiColor];
}

-(void)eyedropperTimerBegin
{
	eyedropperTimer = [[NSTimer scheduledTimerWithTimeInterval:EYEDROPPER_WAIT target:self selector:@selector(eyedropperTimerElapsed) userInfo:nil repeats:FALSE] retain];
}

-(void)eyedropperTimerEnd
{
	if (eyedropperTimer == nil)
		return;
	
	[eyedropperTimer invalidate];
	[eyedropperTimer release];
	eyedropperTimer = nil;
}

-(void)eyedropperTimerElapsed
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("eyedropper timer elapsed", 0);
	
	if (mTouchZoom.State_get() == TouchZoomState_Draw)
	{
		Vec2F location1 = mTouchZoom.StartLocation_get(0);
//		Vec2F location2 = mTouchZoom.Location_get(0);
		
		mTouchZoom.State_set(TouchZoomState_EyeDropper, location1, location1);
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)touchMove:(TouchInfoV2*)ti
{
	Application* app = mAppDelegate.mApplication;

	mTouchZoom.Move(ti->finger, ti->location, ti->location2);
	
	switch (mTouchZoom.State_get())
	{
		case TouchZoomState_Idle:
			break;
		case TouchZoomState_Draw:
		{
			// call traveller update
			
			Vec2F location = [self viewToImage:ti->location];
			
			app->StrokeMove(location[0], location[1]);
			
			[parent brushRetinaUpdate:[self viewToScreen:ti->location] zoom:zoom];
			
			break;
		}
		case TouchZoomState_SwipeAndZoom:
		{
			Assert(ti->touchCount >= 2);
			
			//Vec2F delta = ti->location2Delta / 2.0f / zoom;
			
			//[self swipe:delta];
			
			break;
		}
		case TouchZoomState_EyeDropper:
		{
			[self updateEyedropperLocationAndColor:ti->location];
			break;
		}
		default:
			break;
	}

	[self updatePaint];
}

-(void)handleTouchZoomStateChange:(TouchZoomEvent)e
{
	Application* app = mAppDelegate.mApplication;
	
	switch (e.oldState)
	{
		case TouchZoomState_Idle:
		{
			[controller setMagicMenuEnabled:FALSE];
			break;
		}
		case TouchZoomState_Draw:
		{
			if (e.newState == TouchZoomState_Idle)
				app->StrokeEnd();
			else
				app->StrokeCancel();
			
			[parent brushRetinaHide];
			break;
		}
		case TouchZoomState_EyeDropper:
		{
			[controller setEyedropperEnabled:FALSE];
			[self eyedropperTimerEnd];
			UIColor* uiColor = [controller getView].eyeDropper.color;
			CGColorRef cgColor = uiColor.CGColor;
			const CGFloat* components = CGColorGetComponents(cgColor);
			Rgba color = Rgba_Make(components[0], components[1], components[2], components[3]);
			if (color.rgb[3] != 0.0f)
			{
				// demultiply alpha
				color.rgb[0] /= color.rgb[3];
				color.rgb[1] /= color.rgb[3];
				color.rgb[2] /= color.rgb[3];
			}
			mAppDelegate.mApplication->ColorSelect(color.rgb[0], color.rgb[1], color.rgb[2], color.rgb[3]);
			break;
		}
			
		default:
			break;
	}
	
	bool zoomVisible = false;
	
	switch (e.newState)
	{
		case TouchZoomState_Idle:
		{
			[controller setMagicMenuEnabled:TRUE];
			break;
		}
		case TouchZoomState_Draw:
		{
			Vec2F location = [self viewToImage:e.location];
			
			// todo: smooth = option?
			
			app->StrokeBegin(app->LayerMgr_get()->EditingDataLayer_get(), true, mirrorX, location[0], location[1]);
			
			[controller showMenu:FALSE];
			
			[parent brushRetinaShow:[self viewToScreen:e.location] zoom:zoom];

			break;
		}
		case TouchZoomState_SwipeAndZoom:
		{
			zoomVisible = true;
			
			mZoomStart = zoom;
			mPanStart = pan;
			
			break;
		}
		case TouchZoomState_ToggleMenu:
		{
			[controller showMenu:TRUE];
			break;
		}
		case TouchZoomState_WaitGesture:
		{
			break;
		}
		case TouchZoomState_EyeDropper:
		{
			[controller setEyedropperEnabled:TRUE];
			[self updateEyedropperLocationAndColor:e.location];
			
			break;
		}
		default:
			break;
	}
	
	if (zoomVisible)
		[parent zoomIndicatorShow];
	else
		[parent zoomIndicatorHide:TRUE];
	
	[self updatePaint];
}

-(void)handleZoomChange:(TouchZoomEvent)e
{
	LOG_DBG("initialZoomDistance: %f, zoomDistance: %f", e.initialZoomDistance, e.zoomDistance);
	
#if 0
	// screen coordinates at start
	Vec2F s1a = mTouchZoom.StartLocation2_get(0);
	Vec2F s2a = mTouchZoom.StartLocation2_get(1);
	
	// screen coordinates at present
	Vec2F s1b = mTouchZoom.Location2_get(0);
	Vec2F s2b = mTouchZoom.Location2_get(1);
	
	float zoom2 = s1b.Distance(s2b) / s1a.Distance(s2a) * mZoomStart;
	
	// image coordinates at start
	Vec2F p1 = mTouchZoom.StartLocation_get(0);
	Vec2F p2 = mTouchZoom.StartLocation_get(1);
	
	Vec2F pMid = (p1 + p2) / 2.0f;
	Vec2F sMid = (s1b + s2b) / 2.0f;
	
	LOG_DBG("s1a: %f, %f", s1a[0], s1a[1]);
	LOG_DBG("s2a: %f, %f", s2a[0], s2a[1]);
	LOG_DBG("s1b: %f, %f", s1b[0], s1b[1]);
	LOG_DBG("s2b: %f, %f", s2b[0], s2b[1]);
	LOG_DBG("p1: %f, %f", p1[0], p1[1]);
	LOG_DBG("p2: %f, %f", p2[0], p2[1]);
	
	Vec2I si = mAppDelegate.mApplication->LayerMgr_get()->Size_get();
	Vec2F s(si[0], si[1]);
	
	Vec2F pan2 = (sMid - s/2.0f - pMid * zoom2) / zoom2 + s/2.0f;
	
	[self setZoom:zoom2 animated:NO];
	[self setPan:pan2 animated:NO];
#endif
	
#if 1
	// screen coordinates at start
	Vec2F s1a = mTouchZoom.StartLocation2_get(0);
	Vec2F s2a = mTouchZoom.StartLocation2_get(1);
	
	// screen coordinates at present
	Vec2F s1b = mTouchZoom.Location2_get(0);
	Vec2F s2b = mTouchZoom.Location2_get(1);
	
	float zoom2 = s1b.Distance(s2b) / s1a.Distance(s2a) * mZoomStart;
	
	// image coordinates at start
	Vec2F p1 = mTouchZoom.StartLocation_get(0);
	Vec2F p2 = mTouchZoom.StartLocation_get(1);
	
	Vec2F pMid = (p1 + p2) / 2.0f;
	Vec2F sMid = (s1b + s2b) / 2.0f;
	
	LOG_DBG("s1a: %f, %f", s1a[0], s1a[1]);
	LOG_DBG("s2a: %f, %f", s2a[0], s2a[1]);
	LOG_DBG("s1b: %f, %f", s1b[0], s1b[1]);
	LOG_DBG("s2b: %f, %f", s2b[0], s2b[1]);
	LOG_DBG("p1: %f, %f", p1[0], p1[1]);
	LOG_DBG("p2: %f, %f", p2[0], p2[1]);
	
	Vec2I si = mAppDelegate.mApplication->LayerMgr_get()->Size_get();
	Vec2F s(si[0], si[1]);
	s /= [AppDelegate displayScale];
	
	Vec2F pan2 = (sMid - s/2.0f - pMid * zoom2) / zoom2 + s/2.0f;
	
	[self setZoom:zoom2 animated:NO];
	[self setPan:pan2 animated:NO];
#endif
}

-(RectF)transformedBounds
{
	CGPoint point1 = CGPointMake(0.0f, 0.0f);
	CGPoint point2 = CGPointMake(self.bounds.size.width, self.bounds.size.height);
	
//	CGPoint point1 = CGPointMake(-self.bounds.size.width/2.0f, -self.bounds.size.height/2.0f);
//	CGPoint point2 = CGPointMake(+self.bounds.size.width/2.0f, +self.bounds.size.height/2.0f);
	
//	LOG_DBG("in2: %f, %f", point2.x, point2.y);
	
	CGAffineTransform transform = self.transform;
	
	point1 = CGPointApplyAffineTransform(point1, transform);
	point2 = CGPointApplyAffineTransform(point2, transform);
	
	Vec2F p1(point1);
	Vec2F p2(point2);
	
	p1 += Vec2F(self.center);
	p2 += Vec2F(self.center);
	
//	LOG_DBG("transformedBounds: p1: %f, %f", p1[0], p1[1]);
//	LOG_DBG("transformedBounds: p2: %f, %f", p2[0], p2[1]);
	
	Vec2F pd = p2 - p1;
	
	return RectF(p1, pd);
}

-(void)handleDoubleTap:(Vec2F)location
{
	LOG_DBG("double tap!", 0);
	
	if (sizeArea.IsInside(location))
	{
		LOG_DBG("double tap: size", 0);
		
		[controller handleBrushSize];
	}
	else if (undoArea.IsInside(location))
	{
		LOG_DBG("double tap: undo", 0);
		
		[controller handleUndo];
	}
	else if (redoArea.IsInside(location))
	{
		LOG_DBG("double tap: redo", 0);
		
		[controller handleRedo];
	}
	else
	{
		LOG_DBG("double tap: zoom", 0);
		
		Vec2F location2 = [self screenToView:location];
		
		Vec2F pan2 = Vec2F(self.bounds.size) / 2.0f - location2;
		
		[controller handleZoomToggle:pan2];
	}
}

-(void)swipe:(Vec2F)delta
{
	[self setPan:pan + delta animated:FALSE];
}

-(Vec2F)touchToView:(UITouch*)touch
{
	CGPoint location = [touch locationInView:self];
	
	return Vec2F(location);
}

-(Vec2F)touchToScreen:(UITouch*)touch
{
	CGPoint location = [touch locationInView:touch.window];
	
	return Vec2F(location.x, location.y);
}

-(Vec2F)viewToImage:(Vec2F)location
{
	const float scale = [AppDelegate displayScale];
	return location * scale;
}

-(Vec2F)imageToView:(Vec2F)location
{
	const float scale = [AppDelegate displayScale];
	return location / scale;
}

-(Vec2F)viewToScreen:(Vec2F)_location
{
	CGPoint location = _location.ToCgPoint();

	location = CGPointApplyAffineTransform(location, self.transform);
	location.x += self.center.x;
	location.y += self.center.y;
	
	return Vec2F(location);
}
		 
-(Vec2F)screenToView:(Vec2F)_location
{
	CGPoint location = _location.ToCgPoint();
	
	location.x -= self.center.x;
	location.y -= self.center.y;
	CGAffineTransform transform = CGAffineTransformInvert(self.transform);
	location = CGPointApplyAffineTransform(location, transform);
	
	return Vec2F(location);
}

-(void)updatePaint
{
	AreaI area = mAppDelegate.mApplication->LayerMgr_get()->Validate();
	
	if (area.IsSet_get())
	{
		const float scale = [AppDelegate displayScale];
		
		float x = area.m_Min[0];
		float y = area.m_Min[1];
		float sx = area.m_Max[0] - area.m_Min[0] + 1;
		float sy = area.m_Max[1] - area.m_Min[1] + 1;
		
		[self setNeedsDisplayInRect:CGRectMake(x / scale, y / scale, sx / scale, sy / scale)];
	}
}

-(void)updateTransform
{
	LOG_DBG("updateTransform: %3.2f, %3.2f @ %1.2f (size: %f, %f)", pan[0], pan[1], zoom, self.bounds.size.width, self.bounds.size.height);
	
	[self.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
//	[self setCenter:CGPointMake(0.0f, 0.0f)];
//	[self setCenter:CGPointMake(self.bounds.size.width / 2.0f, self.bounds.size.height / 2.0f)];
	
	Vec2F snappedPan = [self snapPan:pan];
	float snappedZoom = [self snapZoom:zoom];
	
	CGAffineTransform transform = CGAffineTransformIdentity;
	
	transform = CGAffineTransformScale(transform, snappedZoom, snappedZoom);
	transform = CGAffineTransformTranslate(transform, snappedPan[0], snappedPan[1]);
	transform = CGAffineTransformTranslate(transform, -self.bounds.size.width / 2.0f, -self.bounds.size.height / 2.0f);
	
	[self setTransform:transform];
}

static void handleTouchZoomStateChange(void* obj, void* arg)
{
	View_EditingImage* self = (View_EditingImage*)obj;
	TouchZoomEvent e = *(TouchZoomEvent*)arg;
	
	[self handleTouchZoomStateChange:e];
}

static void handleZoomChange(void* obj, void* arg)
{
	View_EditingImage* self = (View_EditingImage*)obj;
	TouchZoomEvent e = *(TouchZoomEvent*)arg;
	
	[self handleZoomChange:e];
}

-(Vec2F)snapPan:(Vec2F)_pan
{
	if (transformTimer != nil)
		return _pan;
	if (zoom > 1.05f || [self snapZoom:zoom] < 1.0f)
		return _pan;

	if (fabs(_pan[0]) < 15.0f)
		_pan[0] = 0.0f;
	if (fabs(_pan[1]) < 15.0f)
		_pan[1] = 0.0f;
	
	_pan[0] = floorf(_pan[0]);
	_pan[1] = floorf(_pan[1]);
	
	return _pan;
}

-(float)snapZoom:(float)_zoom
{
	if (transformTimer != nil)
		return _zoom;
//	if (_zoom <= 0.85f || _zoom > 1.0f)
	if (_zoom <= 0.80f || _zoom > 1.2f)
		return _zoom;
	else
		return 1.0f;
}

// -------------------------
// Canvas pan/zoom animation
// -------------------------

-(void)transformAnimationBegin
{
	if (transformTimer != nil)
		return;
	
	//[self transformAnimationEnd];
	
	transformTimer = [[NSTimer scheduledTimerWithTimeInterval:TRANSFORM_ANIMATION_INTERVAL target:self selector:@selector(transformAnimationUpdate) userInfo:nil repeats:TRUE] retain];
}

-(void)transformAnimationEnd
{
	if (transformTimer == nil)
		return;
	
	[transformTimer invalidate];
	[transformTimer release];
	transformTimer = nil;
	
	[controller handleImageAnimationEnd];
}

-(void)transformAnimationUpdate
{
	if ([self transformAnimationCheck])
	{
		[self transformAnimationEnd];
		return;
	}
	
#if 0
	const float zoomSpeed = 10.0f;
	const float maxZoomDelta = zoomSpeed * TRANSFORM_ANIMATION_INTERVAL;
	const float panSpeed = 400.0f;
	const float maxPanDelta = panSpeed * TRANSFORM_ANIMATION_INTERVAL;
	
	// update pan/zoom
	
	float zoomDelta = targetZoom - zoom;
	
	if (fabs(zoomDelta) <= maxZoomDelta)
		[self setZoom_Direct:targetZoom animated:FALSE];
	else
		[self setZoom_Direct:zoom + maxZoomDelta * Calc::Sign(zoomDelta) animated:FALSE];
	
	Vec2F newPan;
	
	for (int i = 0; i < 2; ++i)
	{
		float panDelta = (targetPan[i] - pan[i]);
		
		if (fabsf(panDelta) <= maxPanDelta)
			newPan[i] = targetPan[i];
		else
			newPan[i] = pan[i] + maxPanDelta * Calc::Sign(panDelta);
	}
	
	[self setPan_Direct:newPan animated:FALSE];
#endif
	
#if 1
	float zoomEps = 0.005f;
	float panEps = 0.01f;
	
	float zoomFalloff = 0.27f;
	float panFalloff = 0.3f;
	
	// update pan/zoom
	
	float newZoom = zoom + (targetZoom - zoom) * zoomFalloff;
	float zoomDelta = newZoom - zoom;
	
	if (fabs(zoomDelta) <= zoomEps)
		[self setZoom_Direct:targetZoom];
	else
		[self setZoom_Direct:newZoom];
	
	Vec2F newPan;
	
	for (int i = 0; i < 2; ++i)
	{
		newPan[i] = pan[i] + (targetPan[i] - pan[i]) * panFalloff;
		float panDelta = newPan[i] - pan[i];
		
		if (fabsf(panDelta) <= panEps)
			newPan[i] = targetPan[i];
	}
	
	[self setPan_Direct:newPan];
#endif
	
//	[self updateTransform];
}

-(bool)transformAnimationCheck
{
	if (targetZoom != zoom)
		return false;
	if (targetPan[0] != pan[0])
		return false;
	if (targetPan[1] != pan[1])
		return false;
	
	return true;
}

//

-(void)drawRect:(CGRect)rect 
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	[self drawRect:rect withContext:ctx];
}

-(void)drawRect:(CGRect)rect withContext:(CGContextRef)ctx
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("drawRect (%f, %f) - (%f, %f)", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
	
	const float scale = [AppDelegate displayScale];
	
	MacImage* image = mAppDelegate.mApplication->LayerMgr_get()->Merged_get();
	
	// draw image
	
	if (!image)
	{
		LOG_WRN("no image", 0);
		return;
	}
	
	if (image->Sx_get() * image->Sy_get() == 0)
	{
		CGContextSetFillColorWithColor(ctx, [UIColor whiteColor].CGColor);
		CGContextFillRect(ctx, self.bounds);
		return;
	}
	
	CGImageRef cgImage = image->Image_get();
	
	if (!cgImage)
	{
		LOG_WRN("no CG image", 0);
		return;
	}
	
	// draw shadow
	
	const float sx = CGImageGetWidth(cgImage);
	const float sy = CGImageGetHeight(cgImage);
	
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGRect r = CGRectMake(0.0f, 0.0f, sx / scale, sy / scale);
	CGContextDrawImage(ctx, r, cgImage);
	
	CGImageRelease(cgImage);
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayersChanged:(NSNotification*)notification
{
	NSLog(@"View_EditingImage: handleLayersChanged");
	
	[self updatePaint];
	
	[parent updateImageBorder];
}

-(void)handleTouchBegin:(UITouch*)touch location1:(Vec2F)location1 location2:(Vec2F)location2
{
	mTouchMgr.TouchBegin(touch, location1, location2);
	
	if (mTouchZoom.State_get() == TouchZoomState_Draw)
	{
		[self eyedropperTimerBegin];
	}
}

-(void)handleTouchEnd:(UITouch*)touch
{
	mTouchMgr.TouchEnd(touch);
	
	[self eyedropperTimerEnd];
}

-(void)touchesBegan:(NSSet*)_touches withEvent:(UIEvent*)event
{
	HandleExceptionObjcBegin();
	
	NSSet* touches = _touches;
	
	if (touches.count != 1)
	{
		if (doubletapState)
			[self doubletapFlush:true];
		[self doubletapTimerEnd];
		doubletapState = 0;
	}
	
	if (doubletapState == 2)
	{
		UITouch* touch = [touches anyObject];
		
		Vec2F location = [self touchToView:touch];
		
		doubletapLocationScreen2 = [self viewToScreen:location];

		if (doubletapLocationScreen1.Distance(doubletapLocationScreen2) > 20.0f)
		{
			[self doubletapFlush:true];
			[self doubletapTimerEnd];
			doubletapState = 0;
		}
	}
	
	if (doubletapState == 2)
	{
		[self doubletapTimerEnd];
		doubletapState = 0;
		
		[self handleDoubleTap:doubletapLocationScreen1];
	}
	else
	{
		if (doubletapState == 0 && [touches count] == 1)
		{
			UITouch* touch = [touches anyObject];
			
			Vec2F location = [self touchToView:touch];
			Vec2F location2 = [self touchToScreen:touch];
			
			// delay touch
			
			doubletapLocationScreen1 = [self viewToScreen:location];
			doubletapLocationScreen1a = location;
			doubletapLocationScreen1b = location2;
			doubletapLocationScreen1t = touch;
			[self doubletapTimerEnd];
			[self doubletapTimerBegin];
			
			doubletapState = 1;
		}
		else
		{
			for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
			{
				UITouch* touch = [[touches allObjects] objectAtIndex:i];

				Vec2F location = [self touchToView:touch];
				Vec2F location2 = [self touchToScreen:touch];

				[self handleTouchBegin:touch location1:location location2:location2];
			}
		}
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet*)_touches withEvent:(UIEvent*)event
{
	HandleExceptionObjcBegin();
	
	if (doubletapState == 1)
	{
		[self doubletapFlush:false];
		[self doubletapTimerEnd];
		doubletapState = 0;
	}
	
	Assert(doubletapState == 0);
	
	NSSet* touches = _touches;
	
	for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		Vec2F location = [self touchToView:touch];
		Vec2F location2 = [self touchToScreen:touch];
		
		mTouchMgr.TouchMoved(touch, location, location2);
	}
	
	[self eyedropperTimerEnd];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesEnded:(NSSet*)_touches withEvent:(UIEvent*)event
{
	HandleExceptionObjcBegin();
	
	if (doubletapState == 1)
	{
		doubletapState = 2;
	}
	else
	{
		NSSet* touches = _touches;
		
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			[self handleTouchEnd:touch];
		}
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet*)_touches withEvent:(UIEvent*)event
{
	HandleExceptionObjcBegin();
	
	NSLog(@"TouchesCancelled");
	
	if (doubletapState == 1)
	{
		doubletapState = 2;
	}
	else
	{
		NSSet* touches = _touches;
		
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			[self handleTouchEnd:touch];
		}
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayersChanged
{
	[self setNeedsDisplay];
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_EditingImage", 0);
	
	[[NSNotificationCenter defaultCenter]
		removeObserver:self
		name:EVT_LAYERS_CHANGED
		object:nil];
	
	[self doubletapTimerEnd];
	[self eyedropperTimerEnd];
	[self transformAnimationEnd];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
