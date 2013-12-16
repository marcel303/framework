#import "AppDelegate.h"
#import "Benchmark.h"
#import "ExceptionLoggerObjC.h"
#import "View_ImageButton.h"
#import "View_ToolType.h"

static NSString* toolImages(int index, int state)
{
#define CASE(_index, _state, name) if (index == _index && state == _state) return name
	
	CASE(0, 0, @"tool_brush.png");
	CASE(0, 1, @"tool_brush_sel.png");
	CASE(1, 0, @"tool_smudge.png");
	CASE(1, 1, @"tool_smudge_sel.png");
	CASE(2, 0, @"tool_eraser.png");
	CASE(2, 1, @"tool_eraser_sel.png");
	
	return nil;
}

@implementation View_ToolType

-(id)initWithFrame:(CGRect)frame toolType:(ToolViewType)type delegate:(id<ToolTypeSelect>)_delegate
{
	HandleExceptionObjcBegin();
	
	Benchmark bm("View_ToolType: init");
	
	frame.origin.x = 25.0f;
	frame.origin.y = 55.0f;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		delegate = _delegate;
		
		switch (type)
		{
			case ToolViewType_Brush:
				index = 0;
				break;
			case ToolViewType_Smudge:
				index = 1;
				break;
			case ToolViewType_Eraser:
				index = 2;
				break;
/*			case ToolViewType_Blur:
				index = 3;
				break;*/
				
			default:
				throw ExceptionNA();
		}
		
		float x = 0.0f;
		float step = 90.0f;
		
		SEL target[3] = { @selector(selectBrush), @selector(selectSmudge), @selector(selectEraser) };
		
		for (int i = 0; i < 3; ++i)
		{
			NSString* image = toolImages(i, i == index ? 1 : 0);
			
			View_ImageButton* button = [[[View_ImageButton alloc] initWithFrame:CGRectMake(x, 0.0f, 0.0f, 0.0f) andImage:image andDelegate:self andClicked:target[i]] autorelease];
			[self addSubview:button];
			
			x += step;
		}
    }

    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)selectBrush
{
	[delegate toolViewTypeChanged:ToolViewType_Brush];
}

-(void)selectSmudge
{
	[delegate toolViewTypeChanged:ToolViewType_Smudge];
}

-(void)selectEraser
{
	[delegate toolViewTypeChanged:ToolViewType_Eraser];
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ToolType", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
