//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import "OFDependencies.h"
#import "OFTableControllerHelper+ViewDelegate.h"
#import "OFTableControllerHelper+Overridables.h"
#import "OFTableCellHelper.h"
#import "OFResourceControllerMap.h"
#import "OFResource.h"
#import "OFControllerLoader.h"
#import "OFTableCellHelper.h"
#import "OFTableSectionDescription.h"
#import <objc/runtime.h>

@implementation OFTableControllerHelper (ViewDelegate)

//- (void)scrollViewDidScroll:(UIScrollView *)scrollView;
//{
//	[self clearSwipedCell];
//}



- (OFTableCellHelper*)loadCell:(NSString*)cellName
{
	OFTableCellHelper* cell = (OFTableCellHelper*)OFControllerLoader::loadCell(cellName, self);
	cell.owningTable = self;
	CGRect cellRect = cell.frame;
	cellRect.size.width = self.tableView.frame.size.width;
	cell.frame = cellRect;
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	OFTableSectionDescription* section = [self getSectionForIndexPath:indexPath];
	NSUInteger cellType, dataRow;
    [section gatherDataForRow:indexPath.row dataRow:dataRow cellType:cellType];
    switch(cellType) {
        case OFTableSectionCellTypes::Leading:
            [self onLeadingCellWasClickedForSection:section];
            break;
        case OFTableSectionCellTypes::Trailing:
            [self onTrailingCellWasClickedForSection:section];
            break;
        default:
            [self onCellWasClicked:[self getCellAtIndexPath:indexPath] indexPathInTable:indexPath];
            
    }
/*    
//	[self clearSwipedCell];
	if(indexPath.row == 0 && section.leadingCellName)
	{
		[self onLeadingCellWasClickedForSection:section];
	}
	else if(indexPath.row == [section countPageItems] - 1 && section.trailingCellName)
	{
		[self onTrailingCellWasClickedForSection:section];
	}
	else
	{	
		NSIndexPath* resourceIndexPath = indexPath;
		if(section.leadingCellName)
		{
			resourceIndexPath = [NSIndexPath indexPathForRow:indexPath.row - 1 inSection:indexPath.section];
		}
		
		[self onCellWasClicked:[self getCellAtIndexPath:resourceIndexPath] indexPathInTable:indexPath];
	}*/
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
	[self tableView:tableView didSelectRowAtIndexPath:indexPath];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
	return [self getCellForIndex:indexPath inTable:tableView useHelperCell:YES].frame.size.height;
}

- (void)isCellAtPath:(NSIndexPath*)indexPath leadingCell:(BOOL&)isLeadingCell trailingCell:(BOOL&)isTrailingCell
{
	OFTableSectionDescription* section = [self getSectionForIndexPath:indexPath];
    NSUInteger cellType, dataRow;
    [section gatherDataForRow:indexPath.row dataRow:dataRow cellType:cellType];
    isLeadingCell = cellType == OFTableSectionCellTypes::Leading;
    isTrailingCell = cellType == OFTableSectionCellTypes::Trailing;
    
    //from previous code: what is that newContentShownAtBottom trying to do?  The actual indexPath isn't reversed, is it?
//	if([self isNewContentShownAtBottom])
//	{	
//		row = (sectionCellCount - 1) - row;
//	}
    
}


- (UITableViewCell*)getCellForIndex:(NSIndexPath*)indexPath inTable:(UITableView*)tableView useHelperCell:(BOOL)useHelperCell
{	
	unsigned int row = indexPath.row;	

	OFTableSectionDescription* section = [self getSectionForIndexPath:indexPath];

	const unsigned int sectionCellCount = [self hasStreamingSections] ? [section countEntireItemSet] : [section countPageItems];

	OFTableCellRoundedEdge roundedEdge = kRoundedEdge_None;
	BOOL isOdd = (row & 1);
	if (row == 0)
	{
		roundedEdge = kRoundedEdge_Top;
	}
	if (row == sectionCellCount - 1)
	{
		if (roundedEdge == kRoundedEdge_Top)
		{
			roundedEdge = kRoundedEdge_TopAndBottom;
		}
		else
		{
			roundedEdge = kRoundedEdge_Bottom;
		}
	}
	
	if([self isNewContentShownAtBottom])
	{	
		row = (sectionCellCount - 1) - row;
	}
	
	if(![self hasStreamingSections])
	{
	    NSAssert2(row < sectionCellCount, @"Somehow we are requesting a cell outside of the number of rows we have. (expecting %d but only have %d)", row, sectionCellCount);
	}

    NSUInteger cellType, dataRow;
    [section gatherDataForRow:row dataRow:dataRow cellType:cellType];
	NSString* tableCellName;
    id resource = nil;
    BOOL isLeadingCell = cellType == OFTableSectionCellTypes::Leading;
    BOOL isTrailingCell = cellType == OFTableSectionCellTypes::Trailing;
    switch(cellType) {
        case OFTableSectionCellTypes::Static:
        {
            OFTableCellHelper* cellHelper = [section.staticCells objectAtIndex:dataRow];
            [self configureCell:cellHelper asLeading:NO asTrailing:NO asOdd:isOdd];
            return cellHelper;
        }
        case OFTableSectionCellTypes::Leading:
            tableCellName = section.leadingCellName;
            resource = section;
            break;
        case OFTableSectionCellTypes::Trailing:
            tableCellName = section.trailingCellName;
            resource = section;
            break;
        case OFTableSectionCellTypes::Resource:
            tableCellName = [self getCellControllerNameFromSection:section.page.objects atRow:dataRow];
            resource = [self getResourceFromSection:section.page.objects atRow:dataRow];
            break;
        case OFTableSectionCellTypes::Action:
            tableCellName = [self actionCellClassName];
            resource = [self getResourceFromSection:section.page.objects atRow:dataRow];
            break;
        case OFTableSectionCellTypes::Error:
        default:
            @throw [NSException exceptionWithName:@"NSTypeException" reason:@"Unhandledn cell type" userInfo:nil];
    }
	if(useHelperCell)
	{
		bool hasConfiguredUniformHelper = YES;
		OFTableCellHelper* cellHelper = [mHelperCellsForHeight objectForKey:tableCellName];
		if(!cellHelper)
		{
			hasConfiguredUniformHelper = NO;
			cellHelper = [self loadCell:tableCellName];
			[mHelperCellsForHeight setObject:cellHelper forKey:tableCellName];
		}
        
		if (![cellHelper hasConstantHeight] || !hasConfiguredUniformHelper)
		{
			[self configureCell:cellHelper asLeading:isLeadingCell asTrailing:isTrailingCell asOdd:isOdd];
			[cellHelper changeResource:resource];
			[self _changeResource:resource forCell:cellHelper withIndex:row]; //?? should this be row or dataRow?
		}
		return cellHelper;
	}
    
	OFTableCellHelper* cell = (OFTableCellHelper*)[tableView dequeueReusableCellWithIdentifier:tableCellName];
	if(!cell)
	{
		cell = [self loadCell:tableCellName];		
        NSAssert3([cell isKindOfClass:[OFTableCellHelper class]], @"Expected UITableCellView named '%@' to be derived from '%s' but was '%s'", tableCellName, class_getName([OFTableCellHelper class]), class_getName([cell class]));			
	}
	
	if([self hasStreamingSections])
	{
	    [self configureCell:cell asLeading:isLeadingCell asTrailing:isTrailingCell asOdd:isOdd];
	    [self _changeResource:resource forCell:cell withIndex:row];
	}
	else
	{
	    [self configureCell:cell asLeading:isLeadingCell asTrailing:isTrailingCell asOdd:isOdd];
	    [cell changeResource:resource];
	}
		
	if (isLeadingCell)
	{
		[self onLeadingCellWasLoaded:cell forSection:section];
	}
	else if (isTrailingCell)
	{
		[self onTrailingCellWasLoaded:cell forSection:section];
	}
    
	return cell;
}



@end
