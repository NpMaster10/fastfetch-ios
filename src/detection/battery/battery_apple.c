#include "fastfetch.h"
#include "battery.h"
#include "util/apple/cf_helpers.h"
#include "util/apple/smc_temps.h"

#if defined(__APPLE__) && !defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
    #include <IOKit/IOKitLib.h>
    #include <IOKit/pwr_mgt/IOPM.h>
#endif

const char* ffDetectBattery(FFBatteryOptions* options, FFlist* results)
{
#if defined(__APPLE__) && !defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
    FF_IOOBJECT_AUTO_RELEASE io_iterator_t iterator = IO_OBJECT_NULL;
    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("AppleSmartBattery"), &iterator) != kIOReturnSuccess)
        return "IOServiceGetMatchingServices() failed";

    io_registry_entry_t registryEntry;
    while ((registryEntry = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        FF_IOOBJECT_AUTO_RELEASE io_registry_entry_t entryBattery = registryEntry;
        FF_CFTYPE_AUTO_RELEASE CFMutableDictionaryRef properties = NULL;
        if (IORegistryEntryCreateCFProperties(entryBattery, &properties, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
            continue;

        // ... existing macOS battery code ...
    }

    return NULL;
#else
    // iOS stub: battery info not available via IOKit
    FFBatteryResult* battery = ffListAdd(results);
    battery->temperature = FF_BATTERY_TEMP_UNSET;
    ffStrbufInitS(&battery->manufacturer, "Unknown (iOS)");
    ffStrbufInitS(&battery->modelName, "Unknown");
    ffStrbufInitS(&battery->serial, "Unknown");
    ffStrbufInitS(&battery->technology, "Unknown");
    ffStrbufInitS(&battery->status, "Unknown");
    ffStrbufInitS(&battery->manufactureDate, "Unknown");
    battery->capacity = -1;
    battery->cycleCount = 0;
    battery->timeRemaining = -1;
    return NULL;
#endif
}
