#import <QuartzCore/CALayer.h>
#import "AppDelegate.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "View_MagicButton.h"

#define BUTTON_DISTANCE 40.0f
#define FADE_INTERVAL (1.0f / 30.0f)

static MagicButtonInfo Create(UIImage* image, Vec2F location, MagicAction action)
{
	location[0] = floorf(location[0]);
	location[1] = floorf(location[1]);
	UIImageView* view = [[[UIImageView alloc] initWithImage:image] autorelease];
	[view setUserInteractionEnabled:FALSE];
	[view.layer setPosition:CGPointMake(location.x, location.y)];
	MagicButtonInfo info;
	info.view = view;
	info.action = action;
	info.location = location;
	return info;
}

@implementation View_MagicButton

@synthesize animationTimer;

-(id)initWithFrame:(CGRect)frame delegate:(id<MagicDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setMultipleTouchEnabled:FALSE];
		[self setUserInteractionEnabled:TRUE];
		
		delegate = _delegate;
		
		// add buttons
		
		//Vec2F center = Vec2F(frame.origin) + Vec2F(frame.size) / 2.0f;
		Vec2F center = Vec2F(frame.size) / 2.0f;
		
		buttons.push_back(Create([delegate provideMagicImage:MagicAction_Left], center + Vec2F(-BUTTON_DISTANCE, 0.0f), MagicAction_Left));
		buttons.push_back(Create([delegate provideMagicImage:MagicAction_Right], center + Vec2F(+BUTTON_DISTANCE, 0.0f), MagicAction_Right));
		buttons.push_back(Create([delegate provideMagicImage:MagicAction_Top], center + Vec2F(0.0f, -BUTTON_DISTANCE), MagicAction_Top));
		buttons.push_back(Create([delegate provideMagicImage:MagicAction_Middle], center, MagicAction_Middle));
		
		for (size_t i = 0; i < buttons.size(); ++i)
			[self addSubview:buttons[i].view];
		
		[self updateButtonVisibility:FALSE];
		
		// add glow
		
		glow = [[[UIImageView alloc] initWithImage:[AppDelegate loadImageResource:@"magic_glow.png"]] autorelease];
		[self addSubview:glow];
		[self updateGlow:MagicAction_None];
		
		fade = 1.0f;
		fadeTarget = 1.0f;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(MagicAction)decideAction
{
	const Vec2F delta = touchLocation2 - touchLocation1;
	
	const int major = fabs(delta[0]) > fabs(delta[1]) ? 0 : 1;
	
	const float spacing = delta[major];
	const float length = fabs(spacing);
	
	MagicAction action = MagicAction_Middle;
	
	if (length > 40.0f)
	{
		if (major == 0 && spacing < 0.0f)
			action = MagicAction_Left;
		if (major == 0 && spacing > 0.0f)
			action = MagicAction_Right;
		if (major == 1 && spacing < 0.0f)
			action = MagicAction_Top;
	}

	return action;
}

-(void)updateGlow:(MagicAction)action
{
	if (action == MagicAction_None)
	{
		[glow setHidden:TRUE];
	}
	else
	{
		[glow setHidden:FALSE];
		
		// move glow overlay to appropriate button
	
		for (size_t i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].action == action)
				[glow.layer setPosition:buttons[i].location.ToCgPoint()];
		}
	}
}

-(void)updateButtonVisibility:(BOOL)visible
{
	for (size_t i = 0; i < buttons.size(); ++i)
	{
		if (buttons[i].action == MagicAction_Middle)
			[buttons[i].view setHidden:FALSE];
		else
			[buttons[i].view setHidden:!visible];
	}
}

-(void)updateFade:(float)_fade
{
	for (size_t i = 0; i < buttons.size(); ++i)
	{
		buttons[i].view.layer.opacity = _fade;
	}
}

-(void)setVisible:(BOOL)visible
{
	fadeTarget = visible ? 1.0f : 0.0f;
	
	[self animationCheck];
}

-(void)animationBegin
{
//	LOG_DBG("magic button: animation begin", 0);
	
	[self animationEnd];
	
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:FADE_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
}

-(void)animationEnd
{
//	LOG_DBG("magic button: animation end", 0);
	
	[animationTimer invalidate];
	self.animationTimer = nil;
}

-(void)animationUpdate
{
	HandleExceptionObjcBegin();
	
//	LOG_DBG("magic button: animation update", 0);
	
	float speed = fadeTarget < fade ? 1.0f / 0.1f : 1.0f / 0.2f;
	
	float delta = fadeTarget - fade;
	float step = Calc::Sign(delta) * speed * FADE_INTERVAL;
	if (fabsf(step) > fabsf(delta))
		fade = fadeTarget;
	else
		fade += step;
	
	[self updateFade:fade];
		
	[self animationCheck];
	
	HandleExceptionObjcEnd(false);
}

-(void)animationCheck
{
	if (fade == fadeTarget)
	{
		[self animationEnd];
	}
	else
	{
		if (animationTimer == nil)
		{
			[self animationBegin];
		}
	}
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	touchLocation1 = Vec2F([[touches anyObject] locationInView:self]);
	touchLocation2 = touchLocation1;
	
	// display magic buttons
	
	[self updateButtonVisibility:TRUE];
	
	// update magic glow
	
	MagicAction action = [self decideAction];
	
	[self updateGlow:action];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	touchLocation2 = Vec2F([[touches anyObject] locationInView:self]);
	
	// update magic glow
	
	MagicAction action = [self decideAction];
	
	[self updateGlow:action];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[self touchesEnded:touches withEvent:event];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();

	touchLocation2 = Vec2F([[touches anyObject] locationInView:self]);
	
	// hide magic buttons
	
	[self updateButtonVisibility:FALSE];
	
	// decide which gesture was made
	
	MagicAction action = [self decideAction];
	
	[delegate handleMagicAction:action];
	
	[self updateGlow:MagicAction_None];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	[self animationEnd];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
