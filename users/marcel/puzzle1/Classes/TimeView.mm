#import "Log.h"
#import "TimeView.h"

@implementation TimeView

-(id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		label = [[[UILabel alloc] initWithFrame:self.bounds] autorelease];
		[label setTextColor:[UIColor whiteColor]];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setOpaque:NO];
		[label setBackgroundColor:[UIColor clearColor]];
		[self addSubview:label];
		
		[self updateUi:0];
    }
	
    return self;
}

-(void)updateUi:(int)time
{
	[label setText:[NSString stringWithFormat:@"Time:%d", time]];
}

-(void)dealloc 
{
	LOG_DBG("dealloc: TimeView", 0);
	
    [super dealloc];
}

@end
