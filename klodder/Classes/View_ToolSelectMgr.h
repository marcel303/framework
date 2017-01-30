#pragma once

#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "View_ToolType.h"
#import "ViewBase.h"
#import "ViewControllerBase.h"

@protocol ToolView

-(void)load; // Called when view settings ought to be loaded
-(void)save; // Called when view settings ought to be saved
-(void)brushSettingsChanged; // Called when tool settings have changed (eg, after settings have been loaded). Receiving view should redraw it's (sub)views.

@end

@interface View_ToolSelectMgr : ViewControllerBase <ToolTypeSelect> 
{
	@private
	
	ViewBase<ToolView>* activeView;
	ToolType _toolType;
	
	@public
	
	ToolViewType toolViewType;
	BrushSettings* brushSettings;
	BrushSettingsLibrary* brushSettingsLibrary;
}

@property (nonatomic, assign) ToolViewType toolViewType;
@property (nonatomic, readonly) BrushSettings* brushSettings;
@property (nonatomic, assign) BrushSettingsLibrary* brushSettingsLibrary;
@property (nonatomic, readonly) ToolType toolType;


-(void)load:(BOOL)restoreLastBrush;
-(void)save;
-(void)toolViewTypeChanged:(ToolViewType)type;
-(ToolType)toolType;
-(void)brushPatternChanged:(uint32_t)patternId;
-(void)brushSettingsChanged;
-(ViewBase*)createView:(ToolViewType)type;
-(void)changeView:(ViewBase*)view;
-(void)apply;
-(void)loadViewWithCurrentToolSettings;

-(BrushSettings*)brushSettings;

@end
