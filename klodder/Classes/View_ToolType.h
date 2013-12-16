#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ITool.h"

// displays a clickable button for each tool view type

enum ToolViewType
{
	ToolViewType_Undefined,
	ToolViewType_Brush,
	ToolViewType_Smudge,
	ToolViewType_Eraser
};

@protocol ToolTypeSelect

-(void)toolViewTypeChanged:(ToolViewType)type;

@end

@interface View_ToolType : UIView 
{
	int index;
	id<ToolTypeSelect> delegate;
}

-(id)initWithFrame:(CGRect)frame toolType:(ToolViewType)type delegate:(id<ToolTypeSelect>)delegate;

-(void)selectBrush;
-(void)selectSmudge;
-(void)selectEraser;

@end
