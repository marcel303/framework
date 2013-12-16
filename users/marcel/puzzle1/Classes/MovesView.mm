#import "Log.h"
#import "MovesView.h"

@implementation MovesView

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

-(void)updateUi:(int)moveCount
{
	[label setText:[NSString stringWithFormat:@"Moves:%d", moveCount]];
}

-(void)dealloc 
{
	LOG_DBG("dealloc: MovesView", 0);
	
    [super dealloc];
}

@end
