#import <UIKit/UIView.h>
#import "ImageId.h"
#import "klodder_forward_objc.h"

// represents a single image in the image gallery

@interface View_PictureGalleryImage : UIView 
{
	@private
	
	AppDelegate* app;
	id target;
	SEL action;
	
	@public
	
	ImageId imageId;
}

@property (assign) ImageId* imageId;

- (id)initWithApp:(AppDelegate*)app imageId:(ImageId*)imageId target:(id)target action:(SEL)action;
-(ImageId*)imageId;
-(void)setImageId:(ImageId *)imageId;

@end
