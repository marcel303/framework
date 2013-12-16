#import <UIKit/UIKit.h>
#import "OFCallbackable.h"

@interface SampleRootController : UIViewController<OFCallbackable>

- (IBAction) onLaunchDashboard;

- (IBAction) onExploreChallenges;
- (IBAction) onExploreLeaderboards;
- (IBAction) onExploreAchivements;
- (IBAction) onExploreSocialNetworkPosts;
- (IBAction) onExploreCloudStorage;
- (IBAction) onExploreInvites;
- (IBAction) onExploreAnnouncements;
- (IBAction) onExploreUser;
- (IBAction) onExploreFeaturedGames;
@end
