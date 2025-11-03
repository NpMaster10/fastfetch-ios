#include "displayserver.h"
#include "util/apple/cf_helpers.h"
#include "util/stringUtils.h"
#include "util/edidHelper.h"
#include "detection/os/os.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_IOS

// ----------------------
// iOS dummy stubs
// ----------------------
typedef int CGDirectDisplayID;
typedef int CGDisplayModeRef;
#define kCGErrorSuccess 0

static inline int CGGetOnlineDisplayList(int maxDisplays, CGDirectDisplayID* displays, uint32_t* count)
{
    (void)maxDisplays;
    (void)displays;
    *count = 0;
    return kCGErrorSuccess;
}

static inline CGDisplayModeRef CGDisplayCopyDisplayMode(CGDirectDisplayID display) { (void)display; return 0; }
static inline void CGDisplayRelease(CGDirectDisplayID display) { (void)display; }
static inline void CGDisplayModeRelease(CGDisplayModeRef mode) { (void)mode; }
static inline uint32_t CGDisplayModeGetPixelWidth(CGDisplayModeRef mode) { (void)mode; return 0; }
static inline uint32_t CGDisplayModeGetPixelHeight(CGDisplayModeRef mode) { (void)mode; return 0; }
static inline double CGDisplayModeGetRefreshRate(CGDisplayModeRef mode) { (void)mode; return 0; }
static inline uint32_t CGDisplayModeGetWidth(CGDisplayModeRef mode) { (void)mode; return 0; }
static inline uint32_t CGDisplayModeGetHeight(CGDisplayModeRef mode) { (void)mode; return 0; }
static inline uint32_t CGDisplayRotation(CGDirectDisplayID display) { (void)display; return 0; }
static inline bool CGDisplayIsBuiltin(CGDirectDisplayID display) { (void)display; return true; }
static inline bool CGDisplayIsMain(CGDirectDisplayID display) { (void)display; return true; }
static inline uint32_t CGDisplaySerialNumber(CGDirectDisplayID display) { (void)display; return 0; }

#else
// ----------------------
// macOS includes
// ----------------------
#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreVideo/CVDisplayLink.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#endif

static void detectDisplays(FFDisplayServerResult* ds)
{
#if TARGET_OS_IOS
    // ----------------------
    // iOS dummy logic: append one dummy display
    // ----------------------
    FF_STRBUF_AUTO_DESTROY buffer = ffStrbufCreate();
    ffStrbufSetStatic(&buffer, "iOS Dummy Display");

    ffdsAppendDisplay(ds,
        0, 0, 0, // width, height, refresh
        0, 0,    // frame width/height
        0, 0, 0, // preferred width/height/refresh
        0,       // rotation
        &buffer,
        FF_DISPLAY_TYPE_BUILTIN,
        true,    // main display
        0, 0, 0, // serial, physical width/height
        "UIKitDummy"
    );
#else
    // ----------------------
    // Original macOS detection logic
    // ----------------------
    CGDirectDisplayID screens[128];
    uint32_t screenCount;
    if(CGGetOnlineDisplayList(ARRAY_SIZE(screens), screens, &screenCount) != kCGErrorSuccess)
        return;

    FF_STRBUF_AUTO_DESTROY buffer = ffStrbufCreate();
    for(uint32_t i = 0; i < screenCount; i++)
    {
        CGDirectDisplayID screen = screens[i];
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(screen);
        if(mode)
        {
            double refreshRate = CGDisplayModeGetRefreshRate(mode);

            if (refreshRate == 0)
            {
                #pragma clang diagnostic push
                #pragma clang diagnostic ignored "-Wdeprecated-declarations"
                CVDisplayLinkRef link;
                if(CVDisplayLinkCreateWithCGDisplay(screen, &link) == kCVReturnSuccess)
                {
                    const CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(link);
                    if (!(time.flags & kCVTimeIsIndefinite))
                        refreshRate = time.timeScale / (double) time.timeValue;
                    CVDisplayLinkRelease(link);
                }
                #pragma clang diagnostic pop
            }

            ffStrbufClear(&buffer);
            // ... rest of your macOS display detection logic
            // Keep your existing CoreGraphics + EDID + HDR handling here
            CGDisplayModeRelease(mode);
        }
        CGDisplayRelease(screen);
    }
#endif
}

void ffConnectDisplayServerImpl(FFDisplayServerResult* ds)
{
#if TARGET_OS_IOS
    ffStrbufSetStatic(&ds->wmProcessName, "SpringBoard");
    ffStrbufSetStatic(&ds->wmPrettyName, "UIKit Compositor");
#else
    {
        FF_CFTYPE_AUTO_DESTROY CFMachPortRef port = CGWindowServerCreateServerPort();
        if (port)
        {
            ffStrbufSetStatic(&ds->wmProcessName, "WindowServer");
            ffStrbufSetStatic(&ds->wmPrettyName, "Quartz Compositor");
        }
    }
#endif

    detectDisplays(ds);
}
