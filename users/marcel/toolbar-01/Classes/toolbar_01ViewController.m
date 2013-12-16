//
//  toolbar_01ViewController.m
//  toolbar-01
//
//  Created by Marcel Smit on 01-04-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import "toolbar_01ViewController.h"
#import "MyCell.h"

@implementation toolbar_01ViewController



// The designated initializer. Override to perform setup that is required before the view is loaded.
//- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
-(id)initWithCoder:(NSCoder *)aDecoder
{
//    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
	if (self = [super initWithCoder:aDecoder])
	{
        // Custom initialization
//		[self setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
//		[self setModalTransitionStyle:UIModalTransitionStyleFlipHorizontal];
		[self setModalTransitionStyle:UIModalTransitionStyleCrossDissolve];
		
		UITableView* table = [[UITableView alloc] initWithFrame:self.view.frame style:UITableViewStyleGrouped];
//		UITableView* table = [[UITableView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 100.0f) style:UITableViewStylePlain];
	    table.autoresizingMask = UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth;
		table.delegate = self;
		table.dataSource = self;
		table.allowsSelection = FALSE;
		[table reloadData];
		[self.view addSubview:table];
//		self.view = table;
		[self.view sendSubviewToBack:table];
		[table release];
    }
    return self;
}

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/


/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

-(void)close
{
	[self dismissModalViewControllerAnimated:TRUE];
}

-(void)presentModal
{
	UIViewController* vc = [[toolbar_01ViewController alloc] initWithNibName:@"toolbar_01ViewController" bundle:[NSBundle mainBundle]];
	[self presentModalViewController:vc animated:TRUE];
	[vc release];
}

-(void)takePicture
{
	// todo: check image picker supports taking camera pictures
	UIImagePickerController* ip = [[UIImagePickerController alloc] init];
	[ip setSourceType:UIImagePickerControllerSourceTypeCamera];
	UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 100.0f, 20.0f)];
	[label setText:@"OverLay!!!"];
	ip.cameraOverlayView = label;
	ip.showsCameraControls = TRUE;
	[label release];
	[self presentModalViewController:ip animated:TRUE];
	[ip release];
}

-(void)getPicture
{
	UIImagePickerController* ip = [[UIImagePickerController alloc] init];
	[ip setSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
	[self presentModalViewController:ip animated:TRUE];
	[ip release];
}

-(NSInteger)tableView:(UITableView*)table numberOfRowsInSection:(NSInteger)section
{
	return 4;
}

-(UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
//	UITableViewCell* cell = [[UITableViewCell alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 10.0f, 80.0f) reuseIdentifier:nil];
//	UITableViewCell* cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
	MyCell* cell = [[MyCell alloc] initWithStyle:UITableViewStylePlain reuseIdentifier:nil];
	
	return cell;
}

-(CGFloat)tableView:(UITableView*)tableView heightForRowAtIndexPath:(NSIndexPath*)indexPath
{
	return 80.0f;
}

-(NSInteger)numberOfSectionsInTableView:(UITableView*)tableView
{
	return 2;
}

-(NSString*)tableView:(UITableView*)tableView titleForHeaderInSection:(NSInteger)section
{
	return @"Hello World";
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}

@end
