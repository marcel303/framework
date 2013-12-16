#import <UIKit/UIKit.h>
#import <vector>
#import "level_pack.h"

@interface LevelPackSelectMgr : UIViewController {

	std::vector<LevelPackDescription> packList;
	UIScrollView* scrollView;
}

@property (nonatomic, retain) IBOutlet UIScrollView* scrollView;

-(IBAction)handleBack:(id)sender;
-(void)handleLevelSelect:(id)sender;

@end
