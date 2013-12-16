#import "ExceptionLoggerObjC.h"
#import "View_MiniBrushSize.h"
#import "View_MiniBrushSizeContainer.h"

@implementation View_MiniBrushSizeContainer

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
//		[self setOpaque:NO];
//		[self setBackgroundColor:[UIColor clearColor]];
		
		int sx = 250;
		int sy = 140;
		int x = (self.frame.size.width - sx) / 2;
		int y = (self.frame.size.height - sy) / 2;
		
		brushSize = [[[View_MiniBrushSize alloc] initWithFrame:CGRectMake(x, y, sx, sy) app:app parent:self] autorelease];
		[self addSubview:brushSize];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)show
{
	[brushSize show];
	
	[self setHidden:FALSE];
}

-(void)hide
{
	[self setHidden:TRUE];
	
	[brushSize hide];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[self hide];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	// nop
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	// nop
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
