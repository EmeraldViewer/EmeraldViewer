/* Copyright (c) 2010 Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "growlnotifiermacosx-objc.h"
#import <Cocoa/Cocoa.h>
#import "Growl/Growl.h"

void growlApplicationBridgeNotify(const std::string& withTitle, const std::string& description, const std::string& notificationName, 
                                void *iconData, unsigned int iconDataSize, int priority, bool isSticky)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *ns_title = [NSString stringWithCString:withTitle.c_str() encoding:NSUTF8StringEncoding];
    NSString *ns_description = [NSString stringWithCString:description.c_str() encoding:NSUTF8StringEncoding];
    NSString *ns_name = [NSString stringWithCString:notificationName.c_str() encoding:NSUTF8StringEncoding];
    NSData *ns_icon = nil;
    if(iconData != NULL)
        ns_icon = [NSData dataWithBytes:iconData length:iconDataSize];
    
    [GrowlApplicationBridge notifyWithTitle:ns_title
                                description:ns_description
                           notificationName:ns_name
                                   iconData:ns_icon
                                   priority:priority
                                   isSticky:isSticky
                               clickContext:nil
     ];
    [pool release];
}

void growlApplicationBridgeInit()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [GrowlApplicationBridge setGrowlDelegate:@""];
    [pool release];
}

bool growlApplicationBridgeIsGrowlInstalled()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    BOOL installed = [GrowlApplicationBridge isGrowlInstalled];
    [pool release];
    return installed;
}
