//
//  NSString+VersionCompare.h
//  mari
//
//  Created by croath on 11/13/14.
//  Copyright (c) 2014 Croath. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (VersionCompare)

- (BOOL)versionLessThan:(NSString*)aVersion;
- (BOOL)versionEqualTo:(NSString*)aVersion;

- (BOOL)versionGreaterThan:(NSString*)aVersion;
- (BOOL)versionLessThanOrEqualTo:(NSString*)aVersion;
- (BOOL)versionGreaterThanOrEqualTo:(NSString*)aVersion;

@end
