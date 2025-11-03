#include "common/font.h"
#include "common/io/io.h"
#include "font.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_OSX
#import <AppKit/NSFont.h>
#endif

static void generateString(FFFontResult* font)
{
    if(font->fonts[0].length > 0)
    {
        ffStrbufAppend(&font->display, &font->fonts[0]);
        ffStrbufAppendS(&font->display, " [System]");
        if(font->fonts[1].length > 0)
            ffStrbufAppendS(&font->display, ", ");
    }

    if(font->fonts[1].length > 0)
    {
        ffStrbufAppend(&font->display, &font->fonts[1]);
        ffStrbufAppendS(&font->display, " [User]");
    }
}

const char* ffDetectFontImpl(FFFontResult* result)
{
#if TARGET_OS_OSX
    ffStrbufAppendS(&result->fonts[0], [NSFont systemFontOfSize:12].familyName.UTF8String);
    ffStrbufAppendS(&result->fonts[1], [NSFont userFontOfSize:12].familyName.UTF8String);

    #ifdef MAC_OS_X_VERSION_10_15
    ffStrbufAppendS(&result->fonts[2], [NSFont monospacedSystemFontOfSize:12 weight:400].familyName.UTF8String);
    #else
    ffStrbufAppendS(&result->fonts[2], "");
    #endif

    ffStrbufAppendS(&result->fonts[3], [NSFont userFixedPitchFontOfSize:12].familyName.UTF8String);

#else
    // iOS dummy values
    ffStrbufAppendS(&result->fonts[0], "System Font");
    ffStrbufAppendS(&result->fonts[1], "User Font");
    ffStrbufAppendS(&result->fonts[2], "Monospaced Font");
    ffStrbufAppendS(&result->fonts[3], "Fixed Pitch Font");
#endif

    generateString(result);

    return NULL;
}
