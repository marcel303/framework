#import "FlickrLoginDialog.h"

static CGFloat kTransitionDuration = 0.3;

@implementation FlickrLoginDialog

static UIButton* MakeButton(id self, SEL action, float x, float y, float sx, float sy, NSString* text)
{
	UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[button setTitle:text forState:UIControlStateNormal];
	[button setFrame:CGRectMake(x, y, sx, sy)];
	[button setHighlighted:TRUE];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

- (id)initWithFrame:(CGRect)frame
{
	float border = 10.0f;
	
	frame.size.width -= border * 2.0f;
	frame.size.height -= border * 2.0f;
	frame.origin.x += border;
	frame.origin.y += border;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor colorWithRed:1.0f green:0.0f blue:0.5f alpha:1.0f]];
		 
		// add web view
		
		float buttonHeight = 30.0f;
		float buttonSpacing = 5.0f;
		float spacing = 8.0f;
		
		CGRect webRect = CGRectMake(spacing, spacing, frame.size.width - spacing * 2.0f, frame.size.height - buttonHeight - buttonSpacing * 2.0f - spacing);
		
		UIWebView* web = [[UIWebView alloc] initWithFrame:webRect];
		[web setScalesPageToFit:TRUE];
		[web loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.google.nl"]]];
		[self addSubview:web];
		[web release];
		
		// todo: redirect web view to flickr site
			
		// add done button
		
		UIButton* button_Done = MakeButton(self, @selector(handleDone), frame.size.width - 100.0f - spacing, frame.size.height - buttonSpacing - buttonHeight, 100.0f, buttonHeight, @"I'm Done!");
		[self addSubview:button_Done];
		
    }
    return self;
}

- (CGAffineTransform)transformForOrientation
{
	UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
	
	if (orientation == UIInterfaceOrientationLandscapeLeft) 
		return CGAffineTransformMakeRotation(M_PI*1.5);
	else if (orientation == UIInterfaceOrientationLandscapeRight)
		return CGAffineTransformMakeRotation(M_PI/2);
	else if (orientation == UIInterfaceOrientationPortraitUpsideDown)
		return CGAffineTransformMakeRotation(-M_PI);
	else 
		return CGAffineTransformIdentity;
}

- (void)bounce1AnimationStopped {
  [UIView beginAnimations:nil context:nil];
  [UIView setAnimationDuration:kTransitionDuration/2];
  [UIView setAnimationDelegate:self];
  [UIView setAnimationDidStopSelector:@selector(bounce2AnimationStopped)];
  self.transform = CGAffineTransformScale([self transformForOrientation], 0.9, 0.9);
  [UIView commitAnimations];
}

- (void)bounce2AnimationStopped {
  [UIView beginAnimations:nil context:nil];
  [UIView setAnimationDuration:kTransitionDuration/2];
  self.transform = [self transformForOrientation];
  [UIView commitAnimations];
}

- (void)dismiss:(BOOL)animated
{
	if (animated) 
	{
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:kTransitionDuration];
		[UIView setAnimationDelegate:self];
		[UIView setAnimationDidStopSelector:@selector(postDismissCleanup)];
		self.alpha = 0.0f;
		[UIView commitAnimations];
	} 
	else
	{
		// ...
	}
}

-(void)handleDone
{
	[self dismiss:TRUE];
}

- (void)postDismissCleanup
{
  [self removeFromSuperview];
}

-(void)show
{
  UIWindow* window = [UIApplication sharedApplication].keyWindow;
  if (!window)
  {
    window = [[UIApplication sharedApplication].windows objectAtIndex:0];
  }
  [window addSubview:self];
    
  self.transform = CGAffineTransformScale([self transformForOrientation], 0.001, 0.001);
  [UIView beginAnimations:nil context:nil];
  [UIView setAnimationDuration:kTransitionDuration/1.5];
  [UIView setAnimationDelegate:self];
  [UIView setAnimationDidStopSelector:@selector(bounce1AnimationStopped)];
  self.transform = CGAffineTransformScale([self transformForOrientation], 1.1, 1.1);
  [UIView commitAnimations];
}

-(void)drawRect:(CGRect)rect
{
	float border = 2.0f;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(ctx, [UIColor blackColor].CGColor);
	CGContextFillRect(ctx, CGRectMake(border, border, self.frame.size.width - border * 2.0f, self.frame.size.height - border * 2.0f));
}

- (void)dealloc
{
    [super dealloc];
}

@end
