#import <UIKit/UIKit.h>
#import "GameView.h"

@interface GameViewController : UIViewController
{
	GameView* gameView;
}

@property (nonatomic, retain) IBOutlet GameView* gameView;

-(id)initWithGameView:(GameView*)gameView;

@end
