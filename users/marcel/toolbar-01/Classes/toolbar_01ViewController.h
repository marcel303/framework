//
//  toolbar_01ViewController.h
//  toolbar-01
//
//  Created by Marcel Smit on 01-04-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface toolbar_01ViewController : UIViewController <UITableViewDelegate, UITableViewDataSource> {

}

-(IBAction)close;
-(IBAction)presentModal;
-(IBAction)takePicture;
-(IBAction)getPicture;

-(NSInteger)tableView:(UITableView*)table numberOfRowsInSection:(NSInteger)section;
-(UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath;
-(CGFloat)tableView:(UITableView*)tableView heightForRowAtIndexPath:(NSIndexPath*)indexPath;
-(NSInteger)numberOfSectionsInTableView:(UITableView*)tableView;
-(NSString*)tableView:(UITableView*)tableView titleForHeaderInSection:(NSInteger)section;

@end

