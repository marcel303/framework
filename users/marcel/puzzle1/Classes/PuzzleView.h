#import <UIKit/UIKit.h>
#import "puzzle.h"
#import "TimeView.h"
#import "MovesView.h"

@class PuzzlePieceView;
@class vwPuzzle;

@protocol PuzzleViewDelegate

-(void)pieceDidFinishedMove:(PuzzlePieceView*)piece;
-(bool)playIsActive;

@end

@interface PuzzleView : UIView 
{
	UIImage* image;
	Puzzle* puzzle;
	NSMutableArray* pieces;
	UIToolbar* toolBar;
	TimeView* timeView;
	MovesView* movesView;
}

@property (nonatomic, retain) UIImage* image;
@property (nonatomic, assign) Puzzle* puzzle;
@property (nonatomic, assign) IBOutlet UIToolbar* toolBar;

-(void)setup:(UIImage*)image puzzle:(Puzzle*)puzzle delegate:(id<PuzzleViewDelegate>)delegate controller:(vwPuzzle*)controller;
-(void)updateUi;
-(void)updateUi_Time:(int)time;
-(void)updateUi_Move:(int)moveCount;
-(PuzzlePieceView*)getPieceView:(Vec2I)location;
-(void)killTimers;

@end
