#include "physicaldisk.h"
#include "util/apple/cf_helpers.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(TARGET_OS_MAC) && !TARGET_OS_IPHONE

#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <IOKit/storage/IOStorageProtocolCharacteristics.h>

#ifdef MAC_OS_X_VERSION_10_15
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>
#endif

// existing macOS code here, unchanged
static const char* detectSsdTemp(io_service_t entryPhysical, double* temp)
{
    #ifdef MAC_OS_X_VERSION_10_15
    __attribute__((__cleanup__(wrapIoDestroyPlugInInterface))) IOCFPlugInInterface** pluginInf = NULL;
    int32_t score;
    if (IOCreatePlugInInterfaceForService(entryPhysical, kIONVMeSMARTUserClientTypeID, kIOCFPlugInInterfaceID, &pluginInf, &score) != kIOReturnSuccess)
        return "IOCreatePlugInInterfaceForService() failed";

    IONVMeSMARTInterface** smartInf = NULL;
    if ((*pluginInf)->QueryInterface(pluginInf, CFUUIDGetUUIDBytes(kIONVMeSMARTInterfaceID), (LPVOID) &smartInf) != kIOReturnSuccess)
        return "QueryInterface() failed";

    NVMeSMARTData smartData;
    const char* error = NULL;
    if ((*smartInf)->SMARTReadData(smartInf, &smartData) == kIOReturnSuccess)
        *temp = smartData.TEMPERATURE - 273;
    else
        error = "SMARTReadData() failed";

    (*pluginInf)->Release(smartInf);
    return error;
    #else
    return "No support for old MacOS version";
    #endif
}

const char* ffDetectPhysicalDisk(FFlist* result, FFPhysicalDiskOptions* options)
{
    // full macOS implementation here
}

#else

// iOS stub
const char* ffDetectPhysicalDisk(FFlist* result, FFPhysicalDiskOptions* options)
{
    (void)result;
    (void)options;
    return "Physical disk detection is not supported on iOS";
}

#endif
