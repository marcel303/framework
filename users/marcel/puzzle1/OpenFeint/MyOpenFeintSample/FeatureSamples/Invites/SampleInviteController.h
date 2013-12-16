//
//  SampleInviteController.h
//  OpenFeint
//
//  Created by Ron on 6/25/10.
//  Copyright 2010 Aurora Feint, Inc.  All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFFriendPickerController.h"


@interface SampleInviteController : UIViewController<OFFriendPickerDelegate, OFInviteSendDelegate, OFInviteDefinitionDelegate>
{
    UILabel* definitionLabel;
    UITextField* alternateDefinitionText;
    OFInviteDefinition* loadedDefinition;
    
    UITextField* defSenderParam;
    UITextField* defReceiverParam;
    UITextField* defDevMessage;
    UITextField* defReceiverNotification;
    UITextField* defSenderNotification;
    UITextField* defSenderIncentive;
    UIImageView* defIcon;
}
@property (nonatomic, retain) IBOutlet UILabel* definitionLabel;
@property (nonatomic, retain) IBOutlet UITextField* alternateDefinitionText;
@property (nonatomic, retain) IBOutlet OFInviteDefinition* loadedDefinition;

@property (nonatomic, retain) IBOutlet UITextField* defSenderParam;
@property (nonatomic, retain) IBOutlet UITextField* defReceiverParam;
@property (nonatomic, retain) IBOutlet UITextField* defDevMessage;
@property (nonatomic, retain) IBOutlet UITextField* defReceiverNotification;
@property (nonatomic, retain) IBOutlet UITextField* defSenderNotification;
@property (nonatomic, retain) IBOutlet UITextField* defSenderIncentive;
@property (nonatomic, retain) IBOutlet UIImageView* defIcon; 

-(IBAction) loadAlternate;
-(IBAction) onTextFieldDoneEditing:(id) sender ;
-(IBAction) chooseFriend;
-(IBAction) displayInviteScreen;
    

@end
