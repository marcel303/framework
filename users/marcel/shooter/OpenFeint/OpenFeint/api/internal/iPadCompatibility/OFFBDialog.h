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

#import "FBLoginDialog.h"

// This category provides an additional method that allow the Facebook Connect
// dialog window to open sized to fit within a containing view.  This overrides
// the default behavior of resizing to the top window, fullscreen.
@interface OFFBDialog : FBLoginDialog {
    
}

- (void)showInView:(UIView*)containerView;
- (void)moveUp:(BOOL)directionIsUp;

@end
