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

#include "OFBase.h"
#import "OFDependencies.h"
#import "OFTableSectionDescription.h"
#import "OFTableSectionCellDescription.h"
#import "OFPaginatedSeries.h"
#import "OFResourceViewHelper.h"
#import "OFPaginatedSeriesHeader.h"

@implementation OFTableSectionDescription

@synthesize title;
@synthesize identifier;
@dynamic page;
@synthesize leadingCellName;
@synthesize trailingCellName;
@synthesize headerView;
@synthesize footerView;
@synthesize staticCells;
@synthesize expandedViews;

- (void)setHeaderView:(UIView*)newHeaderView
{
	OFSafeRelease(headerView);
	headerView = [newHeaderView retain];
}

- (void)setFooterView:(UIView*)newFooterView
{
	OFSafeRelease(footerView);
	footerView = [newFooterView retain];
}

+ (id)sectionWithTitle:(NSString*)title andPage:(OFPaginatedSeries*)page
{
	return [[[OFTableSectionDescription alloc] initWithTitle:title andPage:page] autorelease];
}

+ (id)sectionWithTitle:(NSString*)title andCell:(OFTableSectionCellDescription*)cellDescription
{
	OFPaginatedSeries* page = [OFPaginatedSeries paginatedSeriesWithObject:cellDescription];
	return [[[OFTableSectionDescription alloc] initWithTitle:title andPage:page] autorelease];
}

+ (id)sectionWithTitle:(NSString*)title andStaticCells:(NSMutableArray*)cellHelpers
{
	OFTableSectionDescription* sectionDesc = [[[OFTableSectionDescription alloc] initWithTitle:title andPage:nil] autorelease];
	sectionDesc.staticCells = cellHelpers;
	return sectionDesc;
}

-(void)matchupExpandedViews {
    //for some tables, the number of objects can be added without changing the page.   For this reason, we need to keep some sanity checks around
    //to avoid the table running off the end
    NSUInteger pageCount = [self.page count];
    NSUInteger expandedCount = [self.expandedViews count];
    for(NSUInteger i=expandedCount; i<pageCount; ++i) {
        [self.expandedViews addObject:[NSNumber numberWithBool:NO]];
    }
    
}

-(void)setPage:(OFPaginatedSeries*)_page {
    [page release];
    page = [_page retain];
    self.expandedViews = [NSMutableArray arrayWithCapacity:[self.page count]];
    [self matchupExpandedViews];
}

-(OFPaginatedSeries*)page {
    return page;
}

- (id)initWithTitle:(NSString*)_title andPage:(OFPaginatedSeries*)_page
{
	self = [super init];
	if (self != nil)
	{
		self.title = _title;
		self.page = _page;
                              
	}
	return self;
	
}

- (void)dealloc
{
	self.title = nil;
	self.identifier = nil;
	self.page = nil;
	self.leadingCellName = nil;
	self.trailingCellName = nil;
	self.headerView = nil;
	self.footerView = nil;
	self.staticCells = nil;
    self.expandedViews = nil;
	[super dealloc];
}

- (unsigned int)_countWithAssumption:(unsigned int)baseItemAssumption
{
	if(self.leadingCellName)
	{
		++baseItemAssumption;
	}
	
	if(self.trailingCellName)
	{
		++baseItemAssumption;
	}
    for(NSNumber* val in self.expandedViews) {
        if([val boolValue]) ++baseItemAssumption;
    }
	
	return baseItemAssumption;
}
	
- (unsigned int)countEntireItemSet
{
	return [self _countWithAssumption:self.page.header.totalObjects];
}

- (unsigned int)countPageItems
{
	return [self _countWithAssumption:[self.staticCells count] + [self.page count]];
}

- (BOOL)isRowFirstObject:(NSUInteger)row
{
	if (self.leadingCellName)
	{
		return row == 1;
	}
	return row == 0;
}

- (BOOL)isRowLastObject:(NSUInteger)row
{
	NSUInteger totalRows = [self countEntireItemSet];
	if (self.trailingCellName)
	{
		totalRows--;
	}
	return row == (totalRows - 1);
}

-(void) gatherDataForRow:(NSUInteger) row dataRow:(NSUInteger&) dataRow cellType:(NSUInteger&)cellType {
    [self matchupExpandedViews];
    if(self.leadingCellName) {
        if(row == 0) {
            cellType = OFTableSectionCellTypes::Leading;
            return;
        }
        --row;
    }
    //making the assumption that static cells do not mix with resource cells.  If they do, then we will need to decrement row 
    if(row < [self.staticCells count]) {
        dataRow = row;
        cellType = OFTableSectionCellTypes::Static;
        return;
    }
    dataRow = 0;
    BOOL isActionCell = NO;
    while(row && dataRow < [self.page count]) {
        if(row == 0) break;
        --row;
        if([[self.expandedViews objectAtIndex:dataRow] boolValue]) {
            if(row == 0) {
                isActionCell = YES;
                break;
            }
            else {
                --row;
            }
        }  
        ++dataRow;
    }
    if(dataRow == [self.page count]) {
        cellType = (self.trailingCellName && row==0) ? OFTableSectionCellTypes::Trailing : OFTableSectionCellTypes::Error;
    }
    else {
        cellType = isActionCell ? OFTableSectionCellTypes::Action : OFTableSectionCellTypes::Resource;
    }
}

#pragma mark Testing Code
-(void)runTestLength:(NSUInteger) len {
    for(unsigned int i=0; i<len; ++i) {
        NSUInteger type;
        NSUInteger dataRow;
        [self gatherDataForRow:i dataRow:dataRow cellType:type];
        OFLog(@"Row %d is type %d  dataRow %d", i, type, dataRow);
    }
}

+(void)testRowGathering {
    OFTableSectionDescription* mock = [[OFTableSectionDescription alloc] init];
    mock.leadingCellName = @"LEAD";
    mock.trailingCellName = @"TRAIL";
    mock.page = [NSArray arrayWithObjects:@"1", @"2", @"3", nil];
    mock.expandedViews = [NSArray arrayWithObjects:[NSNumber numberWithBool:YES], [NSNumber numberWithBool:NO], [NSNumber numberWithBool:NO], nil];
    [mock runTestLength:7];

    mock.trailingCellName = nil;
    [mock runTestLength:6];
    
    mock.leadingCellName = nil;
    [mock runTestLength:6];
    
    mock.staticCells = [NSArray arrayWithObjects:@"1", nil];
    [mock runTestLength:6];
    
    
}



@end
