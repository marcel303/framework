#import <UIKit/UIKit.h>
#import "deck_forward.h"
#import "deck_forward_objc.h"

@interface vwGameMgr : UIViewController
{
	WorldView* worldView;
}

@property (nonatomic, assign) IBOutlet WorldView* worldView;

@end
