#import <UIKit/UIView.h>
#import "ColorPickerState.h"

@interface View_ColorPicker_OpacityIndicator : UIView 
{
	id<PickerDelegate> delegate;
}

-(id)initWithDelegate:(id<PickerDelegate>)delegate;

@end
