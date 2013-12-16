#import <UIKit/UIKit.h>

@protocol AlbumDelegate

-(void)albumImageSelected:(UIImage*)image;
-(void)albumImageDismissed;

@end
