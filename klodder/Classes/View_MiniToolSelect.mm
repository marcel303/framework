#import "ExceptionLoggerObjC.h"
#import "View_ImageButton.h"
#import "View_MiniToolSelect.h"
#import "View_ToolType.h"

//@"quicktool_brush.png"
//@"quicktool_smudge.png"
//@"quicktool_eraser.png"
#define IMG_BRUSH @"magic_main.png"
#define IMG_SMUDGE @"magic_main.png"
#define IMG_ERASER @"magic_main.png"

@implementation View_MiniToolSelect

-(id)initWithFrame:(CGRect)frame delegate:(id)_delegate action:(SEL)_action
{
	HandleExceptionObjcBegin();
	
	float sx = 40.0f;
	float sy = 40.0f;
	float spacing = 4.0f;
	
	frame.size.width = spacing * 2.0f + sx * 3.0f;
	frame.size.height = sy;
	
	if ((self = [super initWithFrame:frame]))
	{
#ifdef DEBUG
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:0.1f]];
		[self setClearsContextBeforeDrawing:TRUE];
#endif
		[self setUserInteractionEnabled:TRUE];
		
		delegate = _delegate;
		action = _action;
		
		float x = 0.0f;
		
		View_ImageButton* brush = [[[View_ImageButton alloc] initWithFrame:CGRectMake(x, 0.0f, sx, sy) andImage:IMG_BRUSH andDelegate:self andClicked:@selector(handleBrush)] autorelease];
		[self addSubview:brush];
		x += sx + spacing;
		
		View_ImageButton* smudge = [[[View_ImageButton alloc] initWithFrame:CGRectMake(x, 0.0f, sx, sy) andImage:IMG_SMUDGE andDelegate:self andClicked:@selector(handleSmudge)] autorelease];
		[self addSubview:smudge];
		x += sx + spacing;
		
		View_ImageButton* eraser = [[[View_ImageButton alloc] initWithFrame:CGRectMake(x, 0.0f, sx, sy) andImage:IMG_ERASER andDelegate:self andClicked:@selector(handleEraser)] autorelease];
		[self addSubview:eraser];
		x += sx + spacing;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleBrush
{
	[delegate performSelector:action withObject:[NSNumber numberWithInt:ToolViewType_Brush]];
}

-(void)handleSmudge
{
	[delegate performSelector:action withObject:[NSNumber numberWithInt:ToolViewType_Smudge]];
}

-(void)handleEraser
{
	[delegate performSelector:action withObject:[NSNumber numberWithInt:ToolViewType_Eraser]];
}

@end
