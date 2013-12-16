#import <UIKit/UIKit.h>
#import "puzzle.h"
#import "PuzzleView.h"

@interface PuzzlePieceView : UIView 
{
	UIImage* image;
	CGRect region;
	Puzzle* puzzle;
	PuzzlePiece* piece;
	PuzzleView* puzzleView;
	id<PuzzleViewDelegate> delegate;
	NSTimer* animationTimer;
	bool animationSpeedy;
	bool down;
}

@property (nonatomic, assign) PuzzlePiece* piece;

-(id)initWithImage:(UIImage*)image region:(CGRect)region puzzle:(Puzzle*)puzzle piece:(PuzzlePiece*)piece puzzleView:(PuzzleView*)puzzleView delegate:(id<PuzzleViewDelegate>)delegate;
-(void)updateUi;
-(void)animationBegin:(bool)speedy;
-(void)animationEnd;
-(void)animationUpdate;
-(bool)isPieceAtLocation:(Vec2I)location;
-(void)killTimers;

@end
