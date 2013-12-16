#import "Test_MultiTouchView.h"


@implementation Test_MultiTouchView

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	/*
	UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"Hello World" message:@"This Is A Test" delegate:self cancelButtonTitle:@"CANCEL" otherButtonTitles:@"OK", @"Wha?!", nil];
	[alert show];
	[alert release];
	*/
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [[event allTouches] anyObject];
	
	CGPoint location = [touch locationInView:self];
	
	[controller touch:location];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}


- (void)dealloc {
    [super dealloc];
}


@end
