#import <UIKit/UIKit.h>
#import "AlbumDelegate.h"

@interface vwAlbum : UIViewController 
{
	UIScrollView* scrollView;
	id<AlbumDelegate> delegate;
}

@property (nonatomic, retain) IBOutlet UIScrollView* scrollView;

-(id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil delegate:(id<AlbumDelegate>)delegate;
-(void)handleSelect:(UIImage*)image;
-(IBAction)handleCancel:(id)sender;

@end
