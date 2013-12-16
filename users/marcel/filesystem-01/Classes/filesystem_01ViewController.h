#import <UIKit/UIKit.h>

@interface filesystem_01ViewController : UIViewController {
	IBOutlet UILabel* label;
}

-(IBAction)resourceRead:(id)sender;
-(IBAction)userDocsRead:(id)sender;
-(IBAction)userDocsWrite:(id)sender;
-(IBAction)propertiesRead:(id)sender;
-(IBAction)propertiesWrite:(id)sender;
-(IBAction)cacheRead:(id)sender;
-(IBAction)cacheWrite:(id)sender;
-(IBAction)tempRead:(id)sender;
-(IBAction)tempWrite:(id)sender;
-(IBAction)bundleRead:(id)sender;
-(IBAction)bundleWrite:(id)sender;

-(void)touch:(CGPoint)location;

@end

