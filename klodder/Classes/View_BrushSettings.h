#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ViewBase.h"

// displays sliders to adjust generated brush settings

@interface View_BrushSettings : UIView 
{
	@private
	
	View_ToolSelectMgr* controller;
	View_Gauge* sizeGauge;
	View_Gauge* hardnessGauge;
}

-(id)initWithFrame:(CGRect)frame controller:(View_ToolSelectMgr*)controller;
-(void)sizeChanged:(NSNumber*)value;
-(void)hardnessChanged:(NSNumber*)value;
-(void)brushSettingsChanged;
//-(NSString*)gaugeSizeText:(NSNumber*)gaugeValue;

@end
