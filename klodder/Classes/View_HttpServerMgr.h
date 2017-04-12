#if BUILD_HTTPSERVER

#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ViewControllerBase.h"

@interface View_HttpServerMgr : ViewControllerBase
{
	HTTPServer* httpServer;
	NSString* errorString;
}

@property (nonatomic, retain) HTTPServer* httpServer;
@property (nonatomic, readonly, assign) NSDictionary* addresses;

-(IBAction)handleBack:(id)sender;
-(void)handleAddressUpdate:(NSNotification*)notification;
-(void)startServer;
-(void)stopServer;
-(void)updateUi;
-(NSDictionary*)addresses;

@end

#endif
