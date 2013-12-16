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

#import "OFFBDialog.h"
#import "OpenFeint+Private.h"

@implementation OFFBDialog

- (void)showInView:(UIView*)containerView
{
    [self show];
    if ([OpenFeint isLargeScreen])
    {
        if ([OpenFeint isInLandscapeModeOniPad])
        {
            CGRect fbFrame = self.frame;
            fbFrame.size.width = 400;
            
            if ([OpenFeint getDashboardOrientation] == UIInterfaceOrientationLandscapeRight)
            {
                fbFrame.origin.x += 50;
            }
            
            self.frame = fbFrame;
        }
    }
}

- (void)keyboardWillShow:(NSNotification*)notification
{
    if ([OpenFeint isLargeScreen])
    {
        [self moveUp:YES];
    }
    else
    {
        [(id)super keyboardWillShow:notification];
    }
}

- (void)keyboardWillHide:(NSNotification*)notification
{
    if ([OpenFeint isLargeScreen])
    {
        [self moveUp:NO];
    }
    else
    {
        [(id)super keyboardWillHide:notification];
    }
}

- (void)moveUp:(BOOL)directionIsUp
{
    if (![OpenFeint isLargeScreen]) return;
    
    
    NSInteger direction = directionIsUp ? 1 : -1;
    CGRect fbFrame = self.frame;
    
    switch ([OpenFeint getDashboardOrientation]) {
        case UIInterfaceOrientationPortrait:
            fbFrame.origin.y -= 150 * direction;
            break;
            
        case UIInterfaceOrientationPortraitUpsideDown:
            fbFrame.origin.y += 150 * direction;
            break;
            
        case UIInterfaceOrientationLandscapeLeft:
            fbFrame.origin.x -= 150 * direction;
            break;
            
        case UIInterfaceOrientationLandscapeRight:
            fbFrame.origin.x += 150 * direction;
            break;            
    }
    
    [UIView beginAnimations:nil context:nil];
    self.frame = fbFrame;
    [UIView commitAnimations];
    
}


@end
