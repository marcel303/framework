#import <cmath>
#import <QuartzCore/CALayer.h>
#import "AppDelegate.h"
#import "Log.h"
#import "PuzzlePieceView.h"

#define ANIM_INTERVAL (1.0f / 60.0f)
#define ANIM_SPEED 10.0f
#define ANIM_SPEED_FAST 20.0f

@implementation PuzzlePieceView

@synthesize piece;

-(id)initWithImage:(UIImage*)_image region:(CGRect)_region puzzle:(Puzzle*)_puzzle piece:(PuzzlePiece*)_piece puzzleView:(PuzzleView*)_puzzleView delegate:(id<PuzzleViewDelegate>)_delegate
{
	CGRect frame = _region;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setUserInteractionEnabled:YES];
		[self setMultipleTouchEnabled:NO];
		image = _image;
		region = _region;
		puzzle = _puzzle;
		piece = _piece;
		puzzleView = _puzzleView;
		delegate = _delegate;
		down = false;
    }
	
    return self;
}

-(void)updateUi
{
	Vec2F location;
	location[0] = piece->mAnimationTimer * piece->mLocation[0] + (1.0f - piece->mAnimationTimer) * piece->mLocationPrev[0];
	location[1] = piece->mAnimationTimer * piece->mLocation[1] + (1.0f - piece->mAnimationTimer) * piece->mLocationPrev[1];
	
	int x = location[0] * region.size.width;
	int y = location[1] * region.size.height;
	
	[self setCenter:CGPointMake(0.0f, 0.0f)];
	[self.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
	[self.layer setPosition:CGPointMake(x, y)];
	
	if (puzzle->Mode_get() == PuzzleMode_Switch)
	{
		[self setHidden:piece->mIsFree];
	}
}

-(void)animationBegin:(bool)speedy
{
	[self animationEnd];
	
	Assert(animationTimer == nil);
	
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:ANIM_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
	animationSpeedy = speedy;
	
	piece->mAnimationTimer = 0.0f;
}

-(void)animationEnd
{
	if (animationTimer == nil)
		return;
	
	[animationTimer invalidate];
	animationTimer = nil;
	
	piece->mAnimationTimer = 1.0f;
}

-(void)animationUpdate
{
	piece->mAnimationTimer += ANIM_INTERVAL * (animationSpeedy ? ANIM_SPEED_FAST : ANIM_SPEED);
	
	if (piece->mAnimationTimer >= 1.0f)
	{
		[self animationEnd];
		
		[delegate pieceDidFinishedMove:self];
		
		[AppDelegate playSound:@"move1"];
	}
	
	[self updateUi];
}

-(bool)isPieceAtLocation:(Vec2I)location
{
	Vec2I temp = piece->mLocation;
	
	return temp[0] == location[0] && temp[1] == location[1];
}

-(void)killTimers
{
	[self animationEnd];
}

-(void)drawRect:(CGRect)rect 
{
	[image drawAtPoint:CGPointMake(-region.origin.x, -region.origin.y)];
	
/*	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGPoint points1[4] = { CGPointMake(0.0f, self.bounds.size.height), CGPointMake(self.bounds.size.width, self.bounds.size.height), CGPointMake(0.0f, self.bounds.size.height), CGPointMake(0.0f, 0.0f) };
	CGPoint points2[4] = { CGPointMake(self.bounds.size.width, self.bounds.size.height), CGPointMake(self.bounds.size.width, 0.0f), CGPointMake(0.0f, 0.0f), CGPointMake(self.bounds.size.width, 0.0f) };
	CGContextSetLineWidth(ctx, 2.0f);
	CGContextSetStrokeColorWithColor(ctx, [UIColor darkGrayColor].CGColor);
	CGContextStrokeLineSegments(ctx, points2, 2);
	CGContextSetStrokeColorWithColor(ctx, [UIColor lightGrayColor].CGColor);
	CGContextStrokeLineSegments(ctx, points1, 2);*/
	UIRectFrame(self.bounds);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (![delegate playIsActive])
		return;
	
	Assert(!down);
	
	down = true;
		
	if (puzzle->Mode_get() == PuzzleMode_Switch)
	{
		bool moved = puzzle->TryMove(piece->mLocation);
		
		LOG_DBG("moved: %d", moved?1:0);
		
		if (moved)
		{
			[self animationBegin:false];
			
			[puzzleView updateUi];
		}
	}
	
	if (puzzle->Mode_get() == PuzzleMode_Free)
	{
		[puzzleView bringSubviewToFront:self];
	}
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (!down)
		return;
	
	if (puzzle->Mode_get() == PuzzleMode_Free)
	{
		UITouch* touch = [touches anyObject];
		
		CGPoint location1 = [touch previousLocationInView:puzzleView];
		CGPoint location2 = [touch locationInView:puzzleView];
		
		CGPoint position = self.layer.position;
		
		position.x += location2.x - location1.x;
		position.y += location2.y - location1.y;
		
		[self.layer setPosition:position];
	}
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (!down)
		return;
	
	down = false;
	
	[puzzleView updateUi];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (!down)
		return;
	
	down = false;
	
	if (puzzle->Mode_get() == PuzzleMode_Free)
	{
		LOG_DBG("move", 0);
		
		UITouch* touch = [touches anyObject];
		
		CGPoint location = [touch locationInView:puzzleView];
		
		int x = std::floor(location.x / region.size.width);
		int y = std::floor(location.y / region.size.height);
		
		if (x >= 0 && x < puzzle->Size_get()[0] && y >= 0 && y < puzzle->Size_get()[1])
		{
			PuzzlePieceView* view = [puzzleView getPieceView:Vec2I(x, y)];
			
			puzzle->Switch(piece->mLocation, Vec2I(x, y));
			
			[view animationBegin:false];
			
			[puzzleView updateUi];
		}
		else
		{
			[self updateUi];
		}
	}
}

-(void)dealloc 
{
	LOG_DBG("dealloc: PuzzlePieceView", 0);
	
	[self animationEnd];
	
	image = nil;
	puzzle = 0;
	piece = 0;
	puzzleView = nil;
	delegate = nil;
	
    [super dealloc];
}

@end
