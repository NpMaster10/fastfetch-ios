#include "fastfetch.h"
#include "poweradapter.h"
#include "util/apple/cf_helpers.h"

#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE

#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

const char* ffDetectPowerAdapter(FFlist* results)
{
    FF_CFTYPE_AUTO_RELEASE CFDictionaryRef details = IOPSCopyExternalPowerAdapterDetails();
    if (details && CFDictionaryContainsKey(details, CFSTR(kIOPSPowerAdapterWattsKey)))
    {
        FFPowerAdapterResult* adapter = ffListAdd(results);

        ffStrbufInit(&adapter->name);
        ffStrbufInit(&adapter->description);
        ffStrbufInit(&adapter->manufacturer);
        ffStrbufInit(&adapter->modelName);
        ffStrbufInit(&adapter->serial);
        adapter->watts = 0;

        ffCfDictGetString(details, CFSTR(kIOPSNameKey), &adapter->name);
        ffCfDictGetString(details, CFSTR("Model"), &adapter->modelName);
        ffCfDictGetString(details, CFSTR("Manufacturer"), &adapter->manufacturer);
        ffCfDictGetString(details, CFSTR("Description"), &adapter->description);
        ffCfDictGetString(details, CFSTR("SerialString"), &adapter->serial);
        ffCfDictGetInt(details, CFSTR(kIOPSPowerAdapterWattsKey), &adapter->watts);
    }

    return NULL;
}

#else

// iOS / unsupported platforms
const char* ffDetectPowerAdapter(FFlist* results)
{
    (void)results; // silence unused parameter warning
    return "Power adapter detection not supported on this platform";
}

#endif
