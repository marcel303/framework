#import <QuartzCore/CoreAnimation.h>
#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_LayerManager.h"
#import "View_LayerManagerLayer.h"
#import "View_LayersMgr.h"

#define ANIMATION_INTERVAL (1.0f / 60.0f)

#define IMG_LAYER0 [UIImage imageNamed:@IMG("layer0")]
#define IMG_LAYER1 [UIImage imageNamed:@IMG("layer1")]

@implementation View_LayerManagerLayer

@synthesize index;
@synthesize animationLocation;
@synthesize targetLocation;
@synthesize moveMode;
@synthesize previewOpacity;
@synthesize animationTimer;
@synthesize selectionOverlay;

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(View_LayersMgr*)_controller parent:(View_LayerManager*)_parent index:(int)_index layerIndex:(int)_layerIndex
{
	HandleExceptionObjcBegin();
	
	UIImage* image = IMG_LAYER1;
	
	frame.size = image.size;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setUserInteractionEnabled:TRUE];
		
		app = _app;
		controller = _controller;
		parent = _parent;
		index = _index;
		
		isFocused = false;
		isDragging = false;
		
		moveMode = LayerMoveMode_TargetInstant;
		
		//visibilityIcon = [[[UIImageView alloc] initWithImage:[AppDelegate loadImageResource:@"layer_hidden.png"]] autorelease];
		visibilityIcon = [[[UIImageView alloc] initWithImage:[[UIImage imageNamed:@IMG("layer_hidden")] retain]] autorelease];
		[self addSubview:visibilityIcon];
		[visibilityIcon.layer setPosition:CGPointMake(-visibilityIcon.bounds.size.width/2.0f, self.bounds.size.height / 2.0f)];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleFocus
{
	previewOpacity.Reset();
}

// --------------------
// visual
// --------------------

static void StretchBlitX(const MacImage* src, MacImage* dst, int sy, int dy, float spx, float ssx, float dpx, float dsx)
{
	Assert(sy >= 0 && sy < src->Sy_get());
	Assert(dy >= 0 && dy < dst->Sy_get());
	
	const MacRgba* srcLine = src->Line_get(sy);
	MacRgba* dstLine = dst->Line_get(dy);
	
	//LOG_DBG("stretch: %f", dsx);
	
	MacRgba baseColor;
#if 1
    baseColor.rgba[0] = 0;
    baseColor.rgba[1] = 0;
    baseColor.rgba[2] = 0;
    baseColor.rgba[3] = 0;
#else
	baseColor.rgba[0] = 191;
	baseColor.rgba[1] = 227;
	baseColor.rgba[2] = 255;
	baseColor.rgba[3] = 63;
#endif
    
	for (float i = 0.0f; i < dsx; i += 1.0f)
	{
		const float stx = spx + ssx / dsx * i;
		const float dtx = i + dpx;
		
		const int stxi = (int)stx;
		const int dtxi = (int)dtx;
		
		Assert(stxi >= 0 && stxi < src->Sx_get());
		Assert(dtxi >= 0 && dtxi < dst->Sx_get());
		
		const int a = 255 - srcLine[stxi].rgba[3];
		
		for (int j = 0; j < 4; ++j)
		{
			const int c = (baseColor.rgba[j] * a) >> 8;
			
			dstLine[dtxi].rgba[j] = srcLine[stxi].rgba[j] + c;
		}
	}
}

static void CreatePreview(const MacImage* _src, MacImage* dst, int _sx1, int _sx2, int _sy, float _layerOpacity)
{
	int layerOpacity = int(_layerOpacity * 255.0f);
	
	int _sx = Calc::Max(_sx1, _sx2);
	
	dst->Size_set(_sx, _sy, true);
	
#if 0
	MacImage __src;
	__src.Size_set(_sx, _sy, false);
	
	_src->Blit_Resampled(&__src, true);
	const MacImage* src = &__src;
#else
	const MacImage* src = _src;
#endif
	
	const float sx1 = _sx1;
	const float sx2 = _sx2;
	
	for (int y = 0; y < dst->Sy_get(); ++y)
	{
		const float t = y / (_sy - 1.0f);
		
		const float sx = sx1 * (1.0f - t) + sx2 * t;
		
		const float spacing = _sx - sx;
		
		const int sy = (src->Sy_get() - 1) * y / (_sy - 1);
		
		StretchBlitX(src, dst, sy, y, 0.0f, src->Sx_get(), spacing * 0.5f, sx);
		
		MacRgba* line = dst->Line_get(y);
		
		for (int x = 0; x < dst->Sx_get(); ++x)
		{
			for (int i = 0; i < 4; ++i)
			{
				line[x].rgba[i] = ((int)line[x].rgba[i] * layerOpacity) >> 8;
			}
		}
	}
}

-(void)updatePreview
{
	// update picture data
	
	MacImage* src = app.mApplication->LayerMgr_get()->DataLayer_get(index);
	
	CreatePreview(src, &preview, 160, 230, 90, previewOpacity.HasValue_get() ? previewOpacity.Value_get() : app.mApplication->LayerMgr_get()->DataLayerOpacity_get(index));
	
	[self setNeedsDisplay];
}

-(void)updateUi
{
	LOG_DBG("View_LayerManagerLayer: updateUi", 0);
	
	// update visibility option
	
	bool visibility = app.mApplication->LayerMgr_get()->DataLayerVisibility_get(index);
	
	visibilityIcon.hidden = visibility;
	
	// update border
	
	bool _isFocused = controller.focusLayerIndex == index;
	
	if (_isFocused != isFocused)
	{
		isFocused = _isFocused;
		
		[self setNeedsDisplay];
	}
	
	LOG_DBG("View_LayersMgr: visible: %d", visibility ? 1 : 0);
}

-(void)setPreviewOpacity:(Nullable<float>)opacity
{
	previewOpacity = opacity;
	
	[self updatePreview];
}

-(void)setAnimationLocation:(Vec2F)_location
{
	animationLocation = _location;
	
	[self.layer setPosition:CGPointMake(animationLocation[0], animationLocation[1])];
	
	[parent updateDepthOrder];
	
	[self animationCheck];
}

-(void)setTargetLocation:(Vec2F)__location
{
	targetLocation = __location;
	
	[self animationCheck];
}

-(void)animationCheck
{
	bool animate1 = moveMode != LayerMoveMode_Manual && ![self targetReached];
	bool animate2 = animationTimer != nil;
	
	if (animate1 == animate2)
		return;
	
	if (animate1)
		[self animationBegin];
	else
		[self animationEnd];
}

-(void)animationBegin 
{
	LOG_DBG("layer preview: animation begin", 0);
	
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:ANIMATION_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
}

-(void)animationEnd
{
	LOG_DBG("layer preview: animation end", 0);
	
	[animationTimer invalidate];
	
	self.animationTimer = nil;
}

-(bool)targetReached
{
	Vec2F delta = targetLocation - animationLocation;
	
	return delta.Length_get() <= 1.0f;
}

-(void)animationUpdate
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("layer preview: animate", 0);
	
	if (moveMode == LayerMoveMode_TargetAnim)
	{
		// update layer in the appropriate direction
		
		Vec2F location1 = animationLocation;
		Vec2F location2 = targetLocation;
			
		Vec2F delta = location2 - location1;
		
		float speed = 400.0f;
		
		Vec2F step(
				   Calc::Sign(delta[0]) * Calc::Min(Calc::Abs(delta[0]), speed * ANIMATION_INTERVAL),
				   Calc::Sign(delta[1]) * Calc::Min(Calc::Abs(delta[1]), speed * ANIMATION_INTERVAL));

		Vec2F __location = location1 + step;
		
		[self setAnimationLocation:__location];
	}
	else if (moveMode == LayerMoveMode_Manual)
	{
		// nop
	}
	else if (moveMode == LayerMoveMode_TargetInstant)
	{
		[self setAnimationLocation:targetLocation];
	}
	
	[self animationCheck];
	
	HandleExceptionObjcEnd(false);
}

// --------------------
// interaction
// --------------------

-(void)setIsFocused:(bool)_isFocused
{
	if (_isFocused == isFocused)
		return;
	
	isFocused = _isFocused;
	
	[self setNeedsDisplay];
}

-(void)setMoveMode:(LayerMoveMode)mode
{	
	moveMode = mode;
	
	[self animationCheck];
}

-(void)moveBegin
{
	[self setMoveMode:LayerMoveMode_Manual];
}

-(void)moveEnd
{
	[self setMoveMode:LayerMoveMode_TargetInstant];
	
	[parent updateLayerOrder:self];
}

-(void)moveUpdate:(float)delta
{
	LOG_DBG("move update", 0);
	
	[self setAnimationLocation:animationLocation + Vec2F(0.0f, delta)];
//	[self setTargetLocation:animationLocation];
	
	[parent alignLayers:FALSE animated:TRUE];
}

// --------------------
// drawing
// --------------------

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("rendering layer preview, %dx%d", preview.Sx_get(), preview.Sy_get());
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGImageRef cgImage = preview.Image_get();
	int x = (self.frame.size.width - preview.Sx_get()) / 2.0f;
	int y = (self.frame.size.height - preview.Sy_get()) / 2.0f;
    CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextDrawImage(ctx, CGRectMake(x, y, preview.Sx_get(), preview.Sy_get()), cgImage);	
	CGImageRelease(cgImage);
	
	if (isFocused)
		[IMG_LAYER1 drawAtPoint:CGPointMake(0.0f, 0.0f)];
	else
		[IMG_LAYER0 drawAtPoint:CGPointMake(0.0f, 0.0f)];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	UITouch* touch = [touches anyObject];
	
	[controller handleLayerFocus:self];
	
	if (touch.tapCount == 2)
	{
		bool visibility = app.mApplication->LayerMgr_get()->DataLayerVisibility_get(index);
		
		if (!visibility)
		{
			[controller handleToggleVisibility];
		}
		
		[controller handleLayerSelect:self];
	}
	else
	{
		[self moveBegin];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self touchesEnded:touches withEvent:event];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	UITouch* touch = [touches anyObject];
	
	//CGPoint location = [touch locationInView:self];
	
	[self moveEnd];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	UITouch* touch = [touches anyObject];
	
	CGPoint location1 = [touch previousLocationInView:self];
	CGPoint location2 = [touch locationInView:self];
	
	float dy = location2.y - location1.y;
	
	[self moveUpdate:dy];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_LayerManager_Layer", 0);
	
	[self.selectionOverlay removeFromSuperview];
	self.selectionOverlay = nil;
	
	[self animationEnd];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
