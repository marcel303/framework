#import <QuartzCore/CoreAnimation.h>
#import "AppDelegate.h"
#import "Benchmark.h"
#import "Brush_Pattern.h"
#import "BrushSettingsLibrary.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_BrushSelect.h"
#import "View_BrushSelect_Strip.h"
#import "View_ToolSelectMgr.h"

/*
 requirements:
 - [must] update scroll position based on selection index (focus)
 - [must] convert scroll position into nearest selection index (touch end)
 - [must] handle touch begin/move/end to allow swiping through brush list
 - [nice] animate selected brush into focus
 */

#define SPACING_SY 12.0f
#define ANIMATION_INTERVAL (1.0f / 60.0f)

static void GetMinMax(int index, float& min, float& max)
{
	min = index * PREVIEW_SIZE;
	max = min + PREVIEW_SIZE;
}

BrushItem::BrushItem()
{
	mToolType = ToolType_Undefined;
	mPatternId = 0;
	mPattern = 0;
}

BrushItem::BrushItem(ToolType type, Brush_Pattern* pattern)
{
	Assert(pattern);
	
	mToolType = type;
	mPatternId = pattern->mPatternId;
	mPattern = pattern;
}

@implementation View_BrushSelect

@synthesize animationTimer;
@synthesize scrollPosition;

-(id)initWithFrame:(CGRect)frame controller:(View_ToolSelectMgr*)_controller delegate:(id<BrushSelectDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
	frame.size.height = PREVIEW_DIAMETER + SPACING_SY;
	
	if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setClipsToBounds:YES];
		
		controller = _controller;
		delegate = _delegate;
		
		touchActive = false;
		touchHasMoved = false;
		isAnimating = false;
		scrollPosition = 0.0f;
		selectionIndex = -1;
		
		tick = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:@IMG("indicator_tick")]] autorelease];
		[self addSubview:tick];
		[tick.layer setAnchorPoint:CGPointMake(0.5f, 0.0f)];
		[tick.layer setPosition:CGPointMake(frame.size.width / 2.0f, 0.0f)];
		
		strip = [[[View_BrushSelect_Strip alloc] initWithLocation:Vec2F(frame.origin.x, frame.origin.y + SPACING_SY) itemList:&itemList] autorelease];
		[self addSubview:strip];
		
		// Initialization code
		
		[self loadBrushes];
		
		[self brushSettingsChanged];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)loadBrushes
{
	HandleExceptionObjcBegin();
	
	UsingBegin(Benchmark bm("loadBrushes"))
	{
		itemList.clear();
		
		std::vector<Brush_Pattern*> patternList = [delegate getPatternList];
		
		for (size_t i = 0; i < patternList.size(); ++i)
		{
			Brush_Pattern* pattern = patternList[i];
			
			itemList.push_back(BrushItem(ToolType_PatternBrush, pattern));
		}
	}
	UsingEnd()
	
	[strip updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)beginScroll:(bool)animated
{
	scrollTargetPosition = [self indexToScroll:selectionIndex] - (PREVIEW_DIAMETER + PREVIEW_SPACING) * 0.5f;
	
	LOG_DBG("target position: %f", scrollTargetPosition);
	
	if (animated)
	{
		[self animationBegin];
	}
	else
	{
		[self setScrollPosition:scrollTargetPosition];
	}
}

-(int)locationToIndex:(float)x
{
	int result = (x - scrollPosition) / PREVIEW_SIZE;
	
	return Calc::Mid(result, 0, itemList.size() - 1);
}

-(int)scrollToIndex
{
	return [self locationToIndex:0.0f];
}

-(float)indexToScroll:(int)index
{
	return -index * PREVIEW_SIZE;
}

-(void)brushSettingsChanged
{
	uint32_t patternId = controller.brushSettings->patternId;
	
	int index = -1;
	
	for (size_t i = 0; i < itemList.size(); ++i)
		if (itemList[i].mPatternId == patternId)
			index = i;
	
	if (index == -1)
		return;
	
	if (index == selectionIndex)
		return;
	
	[self setSelectionIndex:index];
	
	[self beginScroll:false];
}

-(void)setScrollPosition:(float)position
{
//	LOG_DBG("old scroll position: %f", scrollPosition);
	
	scrollPosition = position;
	
//	LOG_DBG("new scroll position: %f", scrollPosition);
	
	[strip updateTransform:self.frame.size.width / 2.0f + scrollPosition];
}

-(void)setSelectionIndex:(int)index
{
	if (index == selectionIndex)
		return;
	
	LOG_DBG("old index: %d", selectionIndex);
	
	selectionIndex = index;
	
	const BrushItem& item = itemList[index];
	
	if (item.mPattern)
		[delegate handleBrushPatternSelect:item.mPattern->mPatternId];
	else
		[delegate handleBrushSoftSelect];
	
	LOG_DBG("new index: %d", selectionIndex);
}

-(void)animationBegin
{
	LOG_DBG("brush strip: start animation", 0);
	
	[self animationEnd];
	
	//Assert(self.animationTimer == nil);
	
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:ANIMATION_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
}

-(void)animationEnd
{
	LOG_DBG("brush strip: stop animation", 0);
	
	[animationTimer invalidate];
	
	self.animationTimer = nil;
}

-(bool)animationTargetReached
{
	float delta = scrollTargetPosition - scrollPosition;
	
	return Calc::Abs(delta) <= 1.0f;
}

-(void)animationUpdate
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("brush strip: animate", 0);
	
	// update layer in the appropriate direction
	
	float location1 = scrollPosition;
	float location2 = scrollTargetPosition;
		
	float delta = location2 - location1;
	
	float speed = 400.0f;
	
	float step = Calc::Sign(delta) * Calc::Min(Calc::Abs(delta), speed * ANIMATION_INTERVAL);

	float location = location1 + step;
	
	[self setScrollPosition:location];
	
	 if ([self animationTargetReached])
		 [self animationEnd];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	touchActive = true;
	touchHasMoved = false;
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	if (isAnimating)
		return;
	
	touchHasMoved = true;
	
	UITouch* touch = [touches anyObject];
	
	CGPoint location1 = [touch previousLocationInView:self];
	CGPoint location2 = [touch locationInView:self];
	
	Vec2F delta = Vec2F(location2.x, location2.y) - Vec2F(location1.x, location1.y);
	
	[self setScrollPosition:scrollPosition + delta.x];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self touchesEnded:touches withEvent:event];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	if (!touchActive)
		return;
	
	if (!touchHasMoved)
	{
		UITouch* touch = [touches anyObject];
		CGPoint location = [touch locationInView:self];
		int index = [self locationToIndex:location.x - self.frame.size.width / 2.0f];
		[self setSelectionIndex:index];
		[self beginScroll:true];
		//[self setScrollPosition:[self indexToScroll:index]];
	}
	else
	{
		[self setSelectionIndex:[self scrollToIndex]];
		[self beginScroll:true];
	}
		
	touchActive = false;
	touchHasMoved = false;
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BrushSelect", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
