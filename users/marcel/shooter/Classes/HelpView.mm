#import <QuartzCore/QuartzCore.h>
#import "AppDelegate.h"
#import "AppView.h"
#import "Calc.h"
#import "HelpView.h"

static void Position(UIView* view, float x, float y)
{
	CGAffineTransform transform = CGAffineTransformIdentity;
	transform = CGAffineTransformTranslate(transform, x, y);
	transform = CGAffineTransformRotate(transform, Calc::mPI2);
	[view setCenter:CGPointMake(0.0f, 0.0f)];
	[view.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
	[view setTransform:transform];
}

@implementation HelpView

-(id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) 
	{
		// add help image
		
		helpImage = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"HelpImage"]] autorelease];
		Position(helpImage, 320.0f, 0.0f);
		[self addSubview:helpImage];
		
		// add back button
		
		backButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[backButton setFrame:CGRectMake(0.0f, 0.0f, 120.0f, 44.0f)];
		Position(backButton, 110.0f, 350.0f);
		[backButton addTarget:self action:@selector(handleBack:) forControlEvents:UIControlEventTouchUpInside];
		[backButton setTitle:@"Back" forState:UIControlStateNormal];
		[self addSubview:backButton];
    }
	
    return self;
}

-(void)handleBack:(id)sender
{
	[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].appView];
}

-(void)dealloc
{
    [super dealloc];
}

@end
