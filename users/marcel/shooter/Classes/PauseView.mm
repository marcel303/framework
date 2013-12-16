#import <QuartzCore/QuartzCore.h>
#import "AppDelegate.h"
#import "AppView.h"
#import "Calc.h"
#import "game.h"
#import "PauseView.h"
#import "render_iphone.h"

static void Position(UIView* view, float x, float y)
{
	CGAffineTransform transform = CGAffineTransformIdentity;
	transform = CGAffineTransformTranslate(transform, x, y);
	transform = CGAffineTransformRotate(transform, Calc::mPI2);
	[view setCenter:CGPointMake(0.0f, 0.0f)];
	[view.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
	[view setTransform:transform];
}

@implementation PauseView

-(id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) 
	{
//		[self setOpaque:NO];
		[self setBackgroundColor:[UIColor blackColor]];
		
		// todo: add resume button
		
		resumeButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[resumeButton setFrame:CGRectMake(0.0f, 0.0f, 140.0f, 44.0f)];
		Position(resumeButton, 140.0f, 15.0f);
		[resumeButton addTarget:self action:@selector(handleResume:) forControlEvents:UIControlEventTouchUpInside];
		[resumeButton setTitle:@"Resume" forState:UIControlStateNormal];
		[self addSubview:resumeButton];
		
		// todo: add calibrate button
		
		calibrateButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[calibrateButton setFrame:CGRectMake(0.0f, 0.0f, 140.0f, 44.0f)];
		Position(calibrateButton, 140.0f, 170.0f);
		[calibrateButton addTarget:self action:@selector(handleCalibrate:) forControlEvents:UIControlEventTouchUpInside];
		[calibrateButton setTitle:@"Calibrate" forState:UIControlStateNormal];
		[self addSubview:calibrateButton];
		
		// todo: add restart button
		
		restartButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[restartButton setFrame:CGRectMake(0.0f, 0.0f, 140.0f, 44.0f)];
		Position(restartButton, 140.0f, 325.0f);
		[restartButton addTarget:self action:@selector(handleRestart:) forControlEvents:UIControlEventTouchUpInside];
		[restartButton setTitle:@"Restart" forState:UIControlStateNormal];
		[self addSubview:restartButton];
		
		// add tilt sensitivity slider
		
		sensitivitySlider = [[[UISlider alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 140.0f, 44.0f)] autorelease];
		Position(sensitivitySlider, 200.0f, 170.0f);
		[sensitivitySlider setMinimumValue:0.5f];
		[sensitivitySlider setMaximumValue:1.5f];
		[sensitivitySlider addTarget:self action:@selector(handleTiltSensitivityChanged:) forControlEvents:UIControlEventValueChanged];
		[self addSubview:sensitivitySlider];
		
		// create restart alert
		
		restartAlert = [[UIAlertView alloc] initWithTitle:@"Restart?" message:@"Are you sure?" delegate:self cancelButtonTitle:@"No" otherButtonTitles:@"Yes", nil];
    }
	
    return self;
}

-(void)updateUi
{
	[sensitivitySlider setValue:gTiltSensitivity];
}

-(void)handleResume:(id)sender
{
	[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].appView];
}

-(void)handleCalibrate:(id)sender
{
	RenderIphone* render = (RenderIphone*)gRender;
	
	render->CalibrateTilt();
	
	[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].appView];
}

-(void)handleRestart:(id)sender
{
	[restartAlert show];
}

-(void)handleTiltSensitivityChanged:(id)sender
{
	gTiltSensitivity = [sensitivitySlider value];
	
	SetInt(@"tilt_sensitivity", gTiltSensitivity * 100.0f);
}

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (alertView == restartAlert)
	{
		if (buttonIndex == 0)
		{
		}
		else if (buttonIndex == 1)
		{
			if (gGame->IsPlaying_get())
			{
				gGame->End();
			}
			
			gGame->Begin();
			
			[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].appView];
		}
	}
}

- (void)dealloc 
{
	[restartAlert release];
	
    [super dealloc];
}


@end
