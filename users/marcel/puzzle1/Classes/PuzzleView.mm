#import "PuzzlePieceView.h"
#import "PuzzleView.h"
#import "TimeView.h"
#import "MovesView.h"

@implementation PuzzleView

@synthesize image;
@synthesize puzzle;
@synthesize toolBar;

-(id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
		[self setUserInteractionEnabled:YES];
    }
	
    return self;
}

-(void)setup:(UIImage*)_image puzzle:(Puzzle*)_puzzle delegate:(id<PuzzleViewDelegate>)_delegate controller:(vwPuzzle*)_controller
{
	self.image = _image;
	puzzle = _puzzle;
	
	[pieces release];
	pieces = [[NSMutableArray alloc] init];
	
	Vec2I regionSize(image.size.width / puzzle->Size_get()[0], image.size.height / puzzle->Size_get()[1]);
	
	for (int y = 0; y < puzzle->Size_get()[1]; ++y)
	{
		for (int x = 0; x < puzzle->Size_get()[0]; ++x)
		{
			PuzzlePieceView* piece = [[[PuzzlePieceView alloc] initWithImage:image region:CGRectMake(x * regionSize[0], y * regionSize[1], regionSize[0], regionSize[1]) puzzle:puzzle piece:puzzle->Piece_get(Vec2I(x, y)) puzzleView:self delegate:_delegate] autorelease];
			[pieces addObject:piece];
			[self addSubview:piece];
		}
	}
	
	UIBarButtonItem* item_Replay = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemRewind target:_controller action:@selector(handleShuffle:)] autorelease];
	timeView = [[[TimeView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 90.0f, 20.0f)] autorelease];
	UIBarButtonItem* item_Time = [[[UIBarButtonItem alloc] initWithCustomView:timeView] autorelease];
	movesView = [[[MovesView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 90.0f, 20.0f)] autorelease];
	UIBarButtonItem* item_Moves = [[[UIBarButtonItem alloc] initWithCustomView:movesView] autorelease];
	UIBarButtonItem* item_Solution = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSearch target:_controller action:@selector(handleShowSolution:)] autorelease];
	UIBarButtonItem* item_Space = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil] autorelease];
	
	[toolBar setItems:[NSArray arrayWithObjects:item_Replay, item_Space, item_Time, item_Space, item_Moves, item_Solution, nil] animated:NO];
	
	[self updateUi];
}

-(void)updateUi
{
	// update location of pieces
	
	for (PuzzlePieceView* piece in pieces)
	{
		[piece updateUi];
	}
}

-(void)updateUi_Time:(int)time
{
	[timeView updateUi:time];
}

-(void)updateUi_Move:(int)moveCount
{
	[movesView updateUi:moveCount];
}

-(PuzzlePieceView*)getPieceView:(Vec2I)location
{
	for (PuzzlePieceView* piece in pieces)
	{
		if (![piece isPieceAtLocation:location])
			continue;
		
		return piece;
	}
	
	LOG_DBG("getPieceView: not found", 0);
	
	return nil;
}

-(void)killTimers
{
	for (PuzzlePieceView* piece in pieces)
		[piece killTimers];
}

-(void)dealloc 
{
	LOG_DBG("dealloc: PuzzleView", 0);
	
	for (PuzzlePieceView* piece in pieces)
		[piece removeFromSuperview];
	
	image = nil;
	puzzle = 0;
	
	[pieces release];
	pieces = nil;
	
	toolBar = nil;
	timeView = nil;
	movesView = nil;
	
    [super dealloc];
}

@end
