////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "OFChoiceList.h"
#import "OpenFeint+Private.h"

@interface OFChoiceListController : UIViewController {
@private
    NSObject<OFChoiceListDelegate> *delegate;
    NSArray *choices;
    CGPoint location;
}

@property (nonatomic, retain) NSObject<OFChoiceListDelegate>* delegate;
@property (nonatomic, retain) NSArray *choices;
@property (nonatomic) CGPoint location;
-(void) dismissWithAnswer:(int)answer;
@end

@interface OFChoiceButton : UIButton {
    int returnValue;
    OFChoiceListController*controller;
}
@property (nonatomic) int returnValue;
@property (nonatomic, retain) OFChoiceListController* controller;

@end

@implementation OFChoiceButton
@synthesize returnValue, controller;
-(OFChoiceButton*) initWithList:(OFChoiceListController*) _controller text:(NSString*) _text value:(int) _value {
    if([super init] !=nil) {
        self.controller = _controller;
        self.returnValue = _value;
        [self setTitle:_text forState:UIControlStateNormal];
        [self addTarget:self action:@selector(processButton) forControlEvents:UIControlEventTouchUpInside];
    }
    return self;
}

-(void) processButton {
    [self.controller dismissWithAnswer:self.returnValue];
}

@end


@implementation OFChoiceListController
@synthesize delegate, choices, location;

-(void) dismissWithAnswer:(int)answer {
    [self.delegate userDidChoiceListWithAnswer:answer];
    [self dismissModalViewControllerAnimated:NO];
}

-(void) dismissSelf {
    [self dismissWithAnswer:-1];
}

// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
    OFLog(@"hi!");
    self.view =[[UIView alloc] init];
    //create a background object that will catch any presses outside the form proper.
    
    UIButton *backgroundButton = [UIButton buttonWithType:UIButtonTypeCustom];
//    [backgroundButton setBackgroundColor:[UIColor redColor]];
    CGRect rect = [UIScreen mainScreen].applicationFrame;
    backgroundButton.frame = rect;
    [backgroundButton addTarget:self action:@selector(dismissSelf) forControlEvents:UIControlEventTouchUpInside];
    [UIView beginAnimations:nil context:nil];
    backgroundButton.alpha = 0.85f;
    [backgroundButton setBackgroundColor:[UIColor blackColor]];
    [UIView commitAnimations];
    [self.view addSubview:backgroundButton];
        
    //for each choice in the list, create a button that will return that choice to the delegate
    for(unsigned int i=0; i<[self.choices count]; ++i)
    {
        NSString *itemText = [self.choices objectAtIndex:i];
        OFChoiceButton *button = [[OFChoiceButton alloc] initWithList:self text:itemText value:i];
        button.frame = CGRectMake(self.location.x, self.location.y + i * 20, 100, 20);
        [self.view addSubview:button];
        //create a button, but somehow tack on the return value.....
    }    
}

-(OFChoiceListController*) initWithDelegate:(NSObject<OFChoiceListDelegate>*) _delegate location:(CGPoint) _location choices:(NSArray*) _choices {
    OFLog(@"init called");
    self = [super init];
    if(self != nil) {
        self.delegate = _delegate;
        self.location = _location;
        self.choices = _choices;
    }
    return self;
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

//will we need to handle autorotation inside here?

- (void)dealloc {
    [super dealloc];
}
@end

@implementation OFChoiceList
+(void) showChoiceListWithDelegate:(NSObject<OFChoiceListDelegate>*) delegate location:(CGPoint) location choices:(NSArray*) choices {
    OFChoiceListController* listController = [[[OFChoiceListController alloc] initWithDelegate:delegate location:location choices:choices] autorelease];
    [[OpenFeint getRootController] presentModalViewController:listController animated:NO];
}


@end
