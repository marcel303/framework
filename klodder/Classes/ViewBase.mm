#import "ViewBase.h"

@implementation ViewBase

-(void)handleFocus
{
}

-(void)handleFocusLost
{
}

-(void)drawFrameBorder
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	UIGraphicsPushContext(ctx);
	CGContextSetStrokeColorWithColor(ctx, [UIColor greenColor].CGColor);
	CGContextBeginPath(ctx);
	CGContextAddRect(ctx, CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height));
	CGContextClosePath(ctx);
	CGContextDrawPath(ctx, kCGPathStroke);
	UIGraphicsPopContext();
}

UILabel* CreateLabel(float x, float y, float sx, float sy, NSString* text, NSString* font, float fontSize, UITextAlignment alignment)
{
	UILabel* label = [[[UILabel alloc] initWithFrame:CGRectMake(x, y, sx, sy)] autorelease];
	[label setOpaque:FALSE];
	[label setBackgroundColor:[UIColor clearColor]];
	[label setText:text];
	[label setTextColor:[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.55f]];
	[label setTextAlignment:alignment];
	[label setFont:[UIFont fontWithName:font size:fontSize]];
	
	return label;
}

UIToolbar* CreateToolBar()
{
	return [[UIToolbar alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 40.0f)];
}

@end
