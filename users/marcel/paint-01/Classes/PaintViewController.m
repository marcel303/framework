#import "PaintViewController.h"

@implementation PaintViewController

- init {
	if (self = [super init]) {
	}
	return self;
}

// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}

- (void)viewDidLoad {
	CGRect appRect = [[UIScreen mainScreen] applicationFrame];
	
	paintView = [[PaintView alloc] initWithFrame:appRect];
	paintView.backgroundColor = [UIColor redColor];
	self.view = paintView;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)dealloc {
	[paintView dealloc];
    [super dealloc];
}

@end
