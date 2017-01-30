#import <QuartzCore/CALayer.h>
#import "AppDelegate.h"
#import "BrushSettingsLibrary.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_BrushRetina.h"
#import "View_BrushSettings.h"
#import "View_Editing.h"
#import "View_EditingImage.h"
#import "View_EditingImageBorder.h"
#import "View_EditingMgr.h"
#import "View_Eyedropper.h"
#import "View_MagicButton.h"
#import "View_MiniBrushSizeContainer.h"
#import "View_ZoomInfo.h"

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_Editing

@synthesize image;
@synthesize eyeDropper;
@synthesize zoomIndicator;

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)_app controller:(View_EditingMgr*)_controller
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithFrame:frame])
	{
        frame.origin.x = 0;
        frame.origin.y = 0;
        
        //[self setAutoresizesSubviews:FALSE];
        
        [self setUserInteractionEnabled:TRUE];
		[self setMultipleTouchEnabled:TRUE];
		
#if 1
//		[self setAlpha:0.0f];
		[self setOpaque:TRUE];
//		[self setBackgroundColor:[UIColor blackColor]];
		[self setClipsToBounds:FALSE];
//		[self setHidden:TRUE];
#endif
		
#if DEBUG && 0
		[self setBackgroundColor:[UIColor greenColor]];
#else
		[self setClearsContextBeforeDrawing:FALSE];
#endif
		
		app = _app;
		controller = _controller;
		
		imageBorder = [[[View_EditingImageBorder alloc] initWithFrame:frame] autorelease];
		[self addSubview:imageBorder];
        
		image = [[[View_EditingImage alloc] initWithFrame:frame andApp:app controller:controller parent:self] autorelease];
		[self addSubview:image];
		
		int sx = 40;
		int sy = 40;
#if 0
		// add menu activation button
		UIButton* menuButton = [UIButton buttonWithType:UIButtonTypeInfoLight];
		[menuButton setFrame:CGRectMake((self.frame.size.width - sx) / 2, self.frame.size.height - 10 - sy, sx, sy)];
		[menuButton addTarget:self action:@selector(handleMenuShow) forControlEvents:UIControlEventTouchUpInside];
		[self addSubview:menuButton];
#endif
		
		magicButton = [[[View_MagicButton alloc] initWithFrame:CGRectMake(floorf((self.frame.size.width - sx) / 2.0f), self.frame.size.height - 10.0f - sy, sx, sy) delegate:controller] autorelease];
		[self addSubview:magicButton];
		
		eyeDropper = [[View_Eyedropper alloc] init];
		eyeDropperVisible = FALSE;
		
		zoomIndicator = [[View_ZoomInfo alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 150.0f, 35.0f)];
		[self.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
		[zoomIndicator setTransform:CGAffineTransformMakeTranslation(floorf(self.frame.size.width/2.0f-zoomIndicator.bounds.size.width/2.0f), 20.0f)];
		
		brushRetinaVisible = false;
		
		activityIndicatorBack = [[[UIView alloc] initWithFrame:self.bounds] autorelease];
		[activityIndicatorBack setOpaque:TRUE];
		[activityIndicatorBack setBackgroundColor:[UIColor colorWithRed:0.0f green:0.0f blue:0.1f alpha:1.0f]];
		[activityIndicatorBack setAlpha:0.5f];
		[activityIndicatorBack setUserInteractionEnabled:NO];
		[activityIndicatorBack setHidden:TRUE];
		[self addSubview:activityIndicatorBack];
		activityIndicator = [[[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge] autorelease];
		activityIndicator.center = self.center;
		[self addSubview:activityIndicator];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleFocus
{
	LOG_DBG("handleFocus", 0);
	
	[image handleFocus];
}

-(void)handleFocusLost
{
	[image handleFocusLost];
	
	[self zoomIndicatorHide:TRUE];
}

-(void)setEyeDropperVisible:(BOOL)visible
{
	if (eyeDropperVisible == visible)
		return;
	
	eyeDropperVisible = visible;
	
//	[eyeDropper setHidden:!visible];
	
	if (visible)
		[self addSubview:eyeDropper];
	else
		[eyeDropper removeFromSuperview];
}

-(void)setEyedropperLocation:(CGPoint)location color:(UIColor*)color
{
	[eyeDropper.layer setPosition:location];
	
	eyeDropper.color = color;
}

-(void)setMagicMenuVisible:(BOOL)visible
{
	//magicButton.hidden = !visible;
	[magicButton setVisible:visible];
}

-(void)updateImageBorder
{
	[imageBorder updateLayers:[image transformedBounds]];
}

-(void)showMiniBrushSize
{
	[miniBrushSize removeFromSuperview];
	
	miniBrushSize = [[[View_MiniBrushSizeContainer alloc] initWithFrame:self.bounds app:app] autorelease];
	[self addSubview:miniBrushSize];
	
/*	miniBrushSize = [[[View_MiniBrushSize alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 250.0f, 140.0f) app:app] autorelease];
	[miniBrushSize setHidden:TRUE];
	[miniBrushSize.layer setPosition:CGPointMake(floorf(self.frame.size.width / 2.0f), floorf(self.frame.size.height / 2.0f))];
	[self addSubview:miniBrushSize];*/
	
	[miniBrushSize show];
}

-(void)zoomIndicatorShow
{
	[zoomIndicator removeFromSuperview];
	
	[self addSubview:zoomIndicator];
}

-(void)zoomIndicatorHide:(BOOL)animated
{
	[zoomIndicator removeFromSuperview];
}

-(void)brushRetinaShow:(Vec2F)location zoom:(float)zoom
{
	Assert(!brushRetinaVisible);
	
	brushRetinaVisible = true;
	brushRetinaRadius = -1.0f;
	
	[self brushRetinaUpdate:location zoom:zoom];
}

-(void)brushRetinaHide
{
	Assert(brushRetinaVisible);
	
	[brushRetina removeFromSuperview];
	brushRetina = nil;
	
	brushRetinaVisible = false;
}

-(void)brushRetinaUpdate:(Vec2F)location zoom:(float)zoom
{
	Assert(brushRetinaVisible);
	
	const float scale = [AppDelegate displayScale];
	const float radius = (app.brushSettings->diameter - 1.0f) / 2.0f / scale * zoom;
	
	if (radius != brushRetinaRadius)
	{
		// update picture
		
		[brushRetina removeFromSuperview];
		brushRetina = nil;
		
		brushRetina = [[[View_BrushRetina alloc] initWithRadius:radius] autorelease];
		[self addSubview:brushRetina];
		
		brushRetinaRadius = radius;
	}
	
	// update location
	
	[brushRetina setLocation:location];
}

-(void)activityIndicatorShow
{
	[activityIndicator startAnimating];
	[activityIndicatorBack setHidden:FALSE];
}

-(void)activityIndicatorHide
{
	[activityIndicator stopAnimating];
	[activityIndicatorBack setHidden:TRUE];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[miniBrushSize hide];
	
	[image touchesBegan:touches withEvent:event];

	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self touchesEnded:touches withEvent:event];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[image touchesEnded:touches withEvent:event];

	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[image touchesMoved:touches withEvent:event];
	
	HandleExceptionObjcEnd(false);
}

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	[IMG_BACK drawAsPatternInRect:self.bounds];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_Editing", 0);
	
	[brushRetina release];
	brushRetina = nil;
	
	[zoomIndicator release];
	zoomIndicator = nil;
	
	[eyeDropper release];
	eyeDropper = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
