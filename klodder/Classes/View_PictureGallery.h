#import <UIKit/UIView.h>
#import <vector>
#import "klodder_forward_objc.h"

// represents the picture gallery; a scrollable view with image thumbnails

@interface View_PictureGallery : UIView 
{
	AppDelegate* app;
	View_PictureGalleryMgr* controller;
	UIScrollView* scroll;
	
//	UILabel* ipLabel;
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(View_PictureGalleryMgr*)controller;
-(void)scan;
-(void)addImage:(ImageId)imageId;
-(void)scrollIntoView:(ImageId)imageId;
-(void)scrollToBottom;

@end
