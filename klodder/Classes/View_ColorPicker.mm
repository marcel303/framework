#import <QuartzCore/CALayer.h>
#import <QuartzCore/QuartzCore.h>
#import "AppDelegate.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "MenuView.h"
#import "UIColorEx.h"
#import "View_ColorPicker.h"
#import "View_ColorPicker_BasicIndicator.h"
#import "View_ColorPicker_BrightnessGauge.h"
#import "View_ColorPicker_OpacityGauge.h"
#import "View_ColorPickerMgr.h"
#import "View_MenuSelection.h"
#import "View_SwatchesMgr.h"

#define CIRCLE_MID Vec2F(160.0f, 210.0f)
#define CIRCLE_RADIUS (210.0f / 2.0f)
#define CIRCLE_BASEHUE (Calc::m2PI / 4.0f)

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]
#define IMG_OVERLAY [UIImage imageNamed:@IMG("colorpicker_preview")]
#define IMG_COLORWHEEL [UIImage imageNamed:@IMG("colorwheel")]
#define IMG_COLORWHEEL_DARK [UIImage imageNamed:@IMG("colorwheel_dark")]

void ExclusionZone::Add(RectF rect)
{
	mRectList.push_back(rect);
}
	
bool ExclusionZone::IsInside(Vec2F location)
{
	for (size_t i = 0; i < mRectList.size(); ++i)
		if (mRectList[i].IsInside(location))
			return true;
			
	return false;
}

void ExclusionZone::RenderUi()
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(ctx, [UIColor colorWithRed:1.0f green:0.0f blue:0.0f alpha:0.1f].CGColor);
	
	for (size_t i = 0; i < mRectList.size(); ++i)
	{
		RectF& rect = mRectList[i];
		
		CGContextFillRect(ctx, CGRectMake(rect.m_Position[0], rect.m_Position[1], rect.m_Size[0], rect.m_Size[1]));
	}
}

@implementation View_ColorPicker

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(UIViewController*)_controller delegate:(id<PickerDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame]))
	{
		[self setMultipleTouchEnabled:FALSE];
		
		app = _app;
		controller = _controller;
		delegate = _delegate;
		
		oldColor = delegate.colorPickerState->Color_get();
		oldColor.rgb[3] = delegate.colorPickerState->Opacity_get();
		
		colorBar = [[[View_ColorPicker_ColorBar alloc] initWithFrame:CGRectMake(30.0f, 20.0f, 260.0f, 40.0f) delegate:self] autorelease];
		[self addSubview:colorBar];
		
		darkOverlay = [[[UIImageView alloc] initWithImage:[AppDelegate loadImageResource:@"colorwheel_dark.png"]] autorelease];
		[self addSubview:darkOverlay];
		
		//
		
		indicator = [[[View_ColorPicker_BasicIndicator alloc] init] autorelease];
		[self addSubview:indicator];
		
		brightnessGauge = [[[View_ColorPicker_BrightnessGauge alloc] initWithFrame:CGRectMake(65.0f, 345.0f, 190.0f, 50.0f) height:30.0f delegate:delegate] autorelease];
		[self addSubview:brightnessGauge];
		
		opacityGauge = [[[View_ColorPicker_OpacityGauge alloc] initWithFrame:CGRectMake(65.0f, 395.0f, 190.0f, 50.0f) height:30.0f delegate:delegate] autorelease];
		[self addSubview:opacityGauge];
		
		exclusionZone.Add(RectF(Vec2F(50.0f, 340.0), Vec2F(220.0f, 110.0f)));
		
		[self updateUi];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)sampleAtPoint:(Vec2F)location
{
	Vec2F delta = location - CIRCLE_MID;
	
	float angle = Vec2F::ToAngle(delta) + CIRCLE_BASEHUE;
	
	if (angle < Calc::m2PI)
		angle += Calc::m2PI;
	if (angle > Calc::m2PI)
		angle -= Calc::m2PI;
	
	float hue = angle / Calc::m2PI;
	float saturation = Calc::Mid(delta.Length_get() / CIRCLE_RADIUS, 0.0f, 1.0f);
	
	UIColor* color = [UIColor colorWithHue:hue saturation:saturation brightness:1.0f alpha:1.0f];
	
	const CGFloat* components = CGColorGetComponents([color CGColor]);
	
	delegate.colorPickerState->BaseColor_set(components[0], components[1], components[2]);
}

-(void)handleTouch:(Vec2F)location
{
	location -= CIRCLE_MID;
	
	location /= CIRCLE_RADIUS;
	
	if (location.Length_get() > 1.0f)
		location.Normalize();
	
	location *= CIRCLE_RADIUS;
	
	location += CIRCLE_MID;
	
	[self sampleAtPoint:location];
	
	[self updateUi];
}

-(Rgba)color
{
	return delegate.colorPickerState->Color_get();
}

-(void)updateUi
{
	Rgba baseColor = delegate.colorPickerState->Color_get();
	float opacity = delegate.colorPickerState->Opacity_get();
	UIColor* color = [UIColor colorWithRed:baseColor.rgb[0] green:baseColor.rgb[1] blue:baseColor.rgb[2] alpha:opacity];
	
	[colorBar setOldColor:[UIColor colorWithRed:oldColor.rgb[0] green:oldColor.rgb[1] blue:oldColor.rgb[2] alpha:oldColor.rgb[3]]];
	[colorBar setNewColorDirect:color];

	float hue = color.hue;
	float saturation = color.saturation;
	float brightness = delegate.colorPickerState->Brightness_get();
	
	Vec2F location = CIRCLE_MID + Vec2F::FromAngle(hue * Calc::m2PI - Calc::mPI2) * CIRCLE_RADIUS * saturation;
	
	[brightnessGauge setValueDirect:brightness];
	[opacityGauge setValueDirect:opacity];
	
	[indicator.layer setPosition:CGPointMake(location[0], location[1])];

	[darkOverlay.layer setOpacity:1.0f - brightness];
	[darkOverlay setCenter:CGPointMake(0.0f, 0.0f)];
	[darkOverlay setTransform:CGAffineTransformMakeTranslation(CIRCLE_MID[0], CIRCLE_MID[1])];
}

// ColorButtonDelegate

-(void)handleColorSelect:(UIColor*)color
{
	const CGFloat* components = CGColorGetComponents([color CGColor]);
	
//	Rgba temp = Rgba_Make(components[0], components[1], components[2], delegate.colorPickerState->Opacity_get());
	Rgba temp = Rgba_Make(components[0], components[1], components[2], components[3]);
	
	delegate.colorPickerState->Color_set(temp);
	
	[self updateUi];
}

-(void)handleColorHistorySelect
{
	[delegate openingSwatches];
	
	View_SwatchesMgr* subController = [[[View_SwatchesMgr alloc] initWithApp:app] autorelease];
	[subController setModalTransitionStyle:UIModalTransitionStyleCrossDissolve];
	[controller presentModalViewController:subController animated:TRUE];
}

//

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	CGPoint point = [[[touches allObjects] objectAtIndex:0] locationInView:self];
	
	Vec2F location(point.x, point.y);
	
	if (exclusionZone.IsInside(location))
		return;
	
	if (location.Distance(CIRCLE_MID) < CIRCLE_RADIUS * 1.1f)
	{
		delegate.colorPickerState->isActive = true;
		
		[self handleTouch:location];
	}
	else
	{
		[controller dismissModalViewControllerAnimated:YES];
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
	
	delegate.colorPickerState->isActive = false;

	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	if (!delegate.colorPickerState->isActive)
		return;
	
	CGPoint point = [[[touches allObjects] objectAtIndex:0] locationInView:self];
	
	Vec2F location(point.x, point.y);
	
	[self handleTouch:location];

	HandleExceptionObjcEnd(false);
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	// draw background
	
#if 1
	[IMG_BACK drawAsPatternInRect:CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height)];
#else
#warning
	CGImageRef image = app.mApplication->LayerMgr_get()->Merged_get()->Image_get();
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextDrawImage(ctx, self.bounds, image);
	CGImageRelease(image);
#endif
	
	// draw touch exclusion zone
	
#ifdef DEBUG
	exclusionZone.RenderUi();
#endif
	
	// draw colour circle
	
	Vec2I location(CIRCLE_MID[0] - IMG_COLORWHEEL.size.width / 2.0f, CIRCLE_MID[1] - IMG_COLORWHEEL.size.height / 2.0f);
	
	[IMG_COLORWHEEL drawAtPoint:CGPointMake(location[0], location[1])];
	
//	[IMG_COLORWHEEL_DARK drawAtPoint:CGPointMake(location[0], location[1]) blendMode:kCGBlendModeNormal alpha:1.0f - delegate.colorPickerState->Brightness_get()];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPicker", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
