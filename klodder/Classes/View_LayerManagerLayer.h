#pragma once

#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "Nullable.h"

enum LayerMoveMode
{
	LayerMoveMode_Manual,
	LayerMoveMode_TargetAnim,
	LayerMoveMode_TargetInstant
};

@interface View_LayerManagerLayer : UIView 
{
	AppDelegate* app;
	View_LayersMgr* controller;
	View_LayerManager* parent;
	int index; // data layer index
	
//	Vec2F layerLocation; // location according to layer index
	@public
	Vec2F animationLocation; // animated location. used to sort layers as well
//	@protected
	Vec2F targetLocation; // target location for animation
	
	MacImage preview;
	
	Nullable<float> previewOpacity;
	
	bool isFocused;
	bool isDragging;
	
	LayerMoveMode moveMode;
	
	NSTimer* animationTimer;
	
	UIImageView* selectionOverlay;
	UIImageView* visibilityIcon;
}

@property (nonatomic, assign) int index;
@property (nonatomic, assign) Vec2F animationLocation;
@property (nonatomic, assign) Vec2F targetLocation;
@property (nonatomic, assign) LayerMoveMode moveMode;
//@property (nonatomic, assign) bool isFocused;
@property (nonatomic, assign) Nullable<float> previewOpacity;
@property (nonatomic, assign) NSTimer* animationTimer;
@property (nonatomic, retain) UIImageView* selectionOverlay;

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(View_LayersMgr*)controller parent:(View_LayerManager*)parent index:(int)index layerIndex:(int)layerIndex;
-(void)handleFocus;

// --------------------
// visual
// --------------------

-(void)updatePreview;
-(void)updateUi;
-(void)setPreviewOpacity:(Nullable<float>)opacity;
-(void)setAnimationLocation:(Vec2F)location;
-(void)setTargetLocation:(Vec2F)location;
-(bool)targetReached;
-(void)animationCheck;
-(void)animationBegin;
-(void)animationEnd;
-(void)animationUpdate;

// --------------------
// interaction
// --------------------

//-(void)setIsFocused:(bool)isFocused;
-(void)setMoveMode:(LayerMoveMode)mode;
-(void)moveBegin;
-(void)moveEnd;
-(void)moveUpdate:(float)delta;

// --------------------
// drawing
// --------------------

- (void)drawRect:(CGRect)rect;

@end
