#include "keyboard.h"
#include "util/mallocHelper.h"
#include "util/apple/cf_helpers.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_OSX
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#endif

const char* ffDetectKeyboard(FFlist* devices /* List of FFKeyboardDevice */)
{
#if TARGET_OS_OSX
    // Original macOS implementation
    IOHIDManagerRef FF_CFTYPE_AUTO_RELEASE manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
        return "IOHIDManagerOpen() failed";

    CFDictionaryRef FF_CFTYPE_AUTO_RELEASE matching1 = CFDictionaryCreate(kCFAllocatorDefault, (const void **)(CFStringRef[]){
        CFSTR(kIOHIDDeviceUsagePageKey),
        CFSTR(kIOHIDDeviceUsageKey)
    }, (const void **)(CFNumberRef[]){
        ffCfCreateInt(kHIDPage_GenericDesktop),
        ffCfCreateInt(kHIDUsage_GD_Keyboard)
    }, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    IOHIDManagerSetDeviceMatching(manager, matching1);

    CFSetRef FF_CFTYPE_AUTO_RELEASE set = IOHIDManagerCopyDevices(manager);
    if (set)
        CFSetApplyFunction(set, (CFSetApplierFunction) &enumSet, devices);
    IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);

    return NULL;
#else
    // iOS stub: just add one virtual keyboard
    FFKeyboardDevice* device = (FFKeyboardDevice*) ffListAdd(devices);
    ffStrbufInit(&device->name);
    ffStrbufAppendS(&device->name, "Apple Virtual Keyboard");
    ffStrbufInit(&device->serial);
    ffStrbufAppendS(&device->serial, "000000");
    return NULL;
#endif
}
