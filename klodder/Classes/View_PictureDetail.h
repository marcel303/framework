#import <UIKit/UIView.h>
#import "ImageId.h"
#import "klodder_forward_objc.h"

@interface View_PictureDetail : UIView 
{
	View_PictureDetailMgr* controller;
	ImageId imageId;
}

-(id)initWithFrame:(CGRect)frame controller:(View_PictureDetailMgr*)controller imageId:(ImageId)imageId;

@end
