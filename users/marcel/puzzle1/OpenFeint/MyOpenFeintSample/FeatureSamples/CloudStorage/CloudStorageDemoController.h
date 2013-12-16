//
//  CloudStorageDemoController.h
//  OpenFeint
//
//  Created by Joe on 11/13/09.
//  Copyright 2009 Aurora Feint, Inc.  All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OFCloudStorage.h"

@interface CloudStorageDemoController : UIViewController<OFCloudStorageDelegate>
{
	IBOutlet	UILabel			*statusTextBox;
	IBOutlet	UITextField		*blobNameTextBox;
	IBOutlet	UITextField		*blobPayloadTextBox;
}

@property (retain, nonatomic) UILabel		*statusTextBox;
@property (retain, nonatomic) UITextField	*blobNameTextBox;
@property (retain, nonatomic) UITextField	*blobPayloadTextBox;

- (IBAction) onBtnBlobSend:(id) sender;
- (IBAction) onBtnBlobFetch:(id) sender;
- (IBAction) onBtnClear:(id) sender;
- (IBAction) onBtnPrintAllCurrentUsersDataKeys:(id) sender;
- (IBAction) onTextFieldDoneEditing:(id) sender;


@end
