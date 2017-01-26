#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ActiveLabel.h"

@implementation View_ActiveLabel

-(id)initWithFrame:(CGRect)frame andText:(NSString*)text andFont:(NSString*)font ofSize:(CGFloat)size andAlignment:(NSTextAlignment)alignment andColor:(UIColor*)color andDelegate:(id)_delegate andClicked:(SEL)_clicked
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		delegate = _delegate;
		clicked = _clicked;
		
		UILabel* label = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, frame.size.width, frame.size.height)] autorelease];
		[label setOpaque:FALSE];
		[label setBackgroundColor:[UIColor clearColor]];
		[label setText:text];
		[label setTextColor:color];
		[label setFont:[UIFont fontWithName:@"Helvetica" size:size]];
		[label setTextAlignment:alignment];
		[self addSubview:label];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[delegate performSelector:clicked];
	
	HandleExceptionObjcEnd(false);
}

- (void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ActiveLabel", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
