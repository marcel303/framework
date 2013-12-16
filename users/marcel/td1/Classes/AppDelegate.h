#import <UIKit/UIKit.h>
#import <vector>
#import "level_pack.h"

@class AppViewMgr;

@interface td1AppDelegate : NSObject <UIApplicationDelegate> 
{
    UIWindow *window;
	UIViewController* mgr;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) UIViewController* mgr;

+(std::vector<LevelPackDescription>) scanLevelPacks;
+(std::vector<LevelDescription>)scanLevels:(NSString*)path;

@end

