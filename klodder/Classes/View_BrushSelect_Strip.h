#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

#define PREVIEW_DIAMETER 31
#define PREVIEW_SPACING 7
#define PREVIEW_SIZE (PREVIEW_DIAMETER + PREVIEW_SPACING)

@interface View_BrushSelect_Strip : UIView 
{
	Vec2F location;
	BrushItemList* itemList;
}

-(id)initWithLocation:(Vec2F)location itemList:(BrushItemList*)itemList;
-(void)updateUi;
-(void)updateTransform:(float)scrollPosition;

@end
