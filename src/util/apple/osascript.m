
#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

#if TARGET_OS_MAC && !TARGET_OS_IPHONE

#include "osascript.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <CoreData/CoreData.h>

bool ffOsascript(const char* input, FFstrbuf* result)
{
    NSAppleScript* script = [NSAppleScript.alloc initWithSource:@(input)];
    NSDictionary* errInfo = nil;
    NSAppleEventDescriptor* descriptor = [script executeAndReturnError:&errInfo];
    if (errInfo)
        return false;

    ffStrbufSetS(result, descriptor.stringValue.UTF8String);
    return true;
}

#endif

#if TARGET_OS_IPHONE
#include "osascript.h"
bool ffOsascript(const char* input, FFstrbuf* result)
{
    // Not supported on iOS
    return false;
}
#endif

