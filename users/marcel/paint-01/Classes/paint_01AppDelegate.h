#import <UIKit/UIKit.h>
#import <UIKit/UIImagePickerController.h>
#import "PaintViewController.h"

@interface paint_01AppDelegate : NSObject <UIApplicationDelegate, UINavigationControllerDelegate> {
	UIWindow *window;
	PaintViewController* controller;
	
	UIImagePickerController* imagePicker;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@end

