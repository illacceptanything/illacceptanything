//
//  NSString+VersionCompare.m
//  mari
//
//  Created by croath on 11/13/14.
//  Copyright (c) 2014 Croath. All rights reserved.
//

#import "NSString+VersionCompare.h"

@implementation NSString (VersionCompare)

//MARK: Internal
- (NSArray*)versionNumbers{
    return [self componentsSeparatedByString:@"."];
}


//MARK: Basic
- (BOOL)versionLessThan:(NSString*)aVersion{
    NSArray *nums = [self versionNumbers];
    NSArray *aNums = [aVersion versionNumbers];
    
    __block BOOL lessThan = NO;
    __block BOOL decide = NO;
    [nums enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSInteger v = [obj integerValue];
        if ([aNums count] > idx) {
            NSInteger aV = [[aNums objectAtIndex:idx] integerValue];
            if (v < aV) {
                lessThan = YES;
                decide = YES;
                *stop = YES;
            } else if (v > aV) {
                lessThan = NO;
                decide = YES;
                *stop = YES;
            }
        } else {
            *stop = YES;
        }
    }];
    
    if ([aNums count] > [nums count] && decide == NO) {
        for (NSInteger i = [nums count]; i < [aNums count]; i ++) {
            NSInteger num = [[aNums objectAtIndex:i] integerValue];
            if (num != 0) {
                lessThan = YES;
                break;
            }
        }
    }
    
    return lessThan;
}

- (BOOL)versionEqualTo:(NSString*)aVersion{
    NSArray *nums = [self versionNumbers];
    NSArray *aNums = [aVersion versionNumbers];
    
    __block BOOL equal = YES;
    __block BOOL decide = NO;
    [nums enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSInteger v = [obj integerValue];
        if ([aNums count] > idx) {
            NSInteger aV = [[aNums objectAtIndex:idx] integerValue];
            if (v != aV) {
                equal = NO;
                decide = YES;
                *stop = YES;
            }
        } else {
            if (v != 0) {
                equal = NO;
                decide = YES;
            }
        }
    }];
    
    if ([aNums count] > [nums count] && decide == NO) {
        for (NSInteger i = [nums count]; i < [aNums count]; i ++) {
            NSInteger num = [[aNums objectAtIndex:i] integerValue];
            if (num != 0) {
                equal = NO;
                break;
            }
        }
    }
    
    return equal;
}


//MARK: Extensions
- (BOOL)versionGreaterThan:(NSString*)aVersion{
    return ![self versionLessThanOrEqualTo:aVersion];
}

- (BOOL)versionLessThanOrEqualTo:(NSString*)aVersion{
    return [self versionLessThan:aVersion] || [self versionEqualTo:aVersion];
}

- (BOOL)versionGreaterThanOrEqualTo:(NSString*)aVersion{
    return ![self versionLessThan:aVersion];
}

@end
