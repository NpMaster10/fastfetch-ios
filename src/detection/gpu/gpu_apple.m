#include "gpu.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_OSX
#import <Metal/MTLDevice.h>
#import <IOKit/kext/KextManager.h>
#endif

const char* ffGpuDetectDriverVersion(FFlist* gpus)
{
#if TARGET_OS_OSX
    if (@available(macOS 10.7, *))
    {
        NSMutableArray* arr = NSMutableArray.new;
        FF_LIST_FOR_EACH(FFGPUResult, x, *gpus)
            [arr addObject:@(x->driver.chars)];

        NSDictionary* dict = CFBridgingRelease(KextManagerCopyLoadedKextInfo((__bridge CFArrayRef)arr, (__bridge CFArrayRef)@[@"CFBundleVersion"]));
        FF_LIST_FOR_EACH(FFGPUResult, x, *gpus)
        {
            NSString* version = dict[@(x->driver.chars)][@"CFBundleVersion"];
            if (version)
            {
                ffStrbufAppendC(&x->driver, ' ');
                ffStrbufAppendS(&x->driver, version.UTF8String);
            }
        }
        return NULL;
    }
    return "Unsupported macOS version";
#else
    // iOS stub
    (void)gpus;
    return NULL;
#endif
}

const char* ffGpuDetectMetal(FFlist* gpus)
{
#if TARGET_OS_OSX
    if (@available(macOS 10.13, *))
    {
        for (id<MTLDevice> device in MTLCopyAllDevices())
        {
            FFGPUResult* gpu = NULL;
            FF_LIST_FOR_EACH(FFGPUResult, x, *gpus)
            {
                if (x->deviceId == device.registryID)
                {
                    gpu = x;
                    break;
                }
            }
            if (!gpu) continue;

            gpu->type = device.hasUnifiedMemory ? FF_GPU_TYPE_INTEGRATED : FF_GPU_TYPE_DISCRETE;
            gpu->index = (uint32_t) device.locationNumber;

            if (device.hasUnifiedMemory && device.recommendedMaxWorkingSetSize > 0)
                gpu->shared.total = device.recommendedMaxWorkingSetSize;
        }
        return NULL;
    }
    return "Metal API is not supported by this macOS version";
#else
    // iOS stub: just assign default Metal device
    FFGPUResult* gpu = ffListAdd(gpus);
    gpu->index = 0;
    gpu->type = FF_GPU_TYPE_INTEGRATED;
    ffStrbufInitStatic(&gpu->platformApi, "Metal");
    ffStrbufInit(&gpu->name);
    ffStrbufAppendS(&gpu->name, "Apple GPU");
    ffStrbufInit(&gpu->vendor);
    ffStrbufAppendS(&gpu->vendor, "Apple");
    gpu->frequency = FF_GPU_FREQUENCY_UNSET;
    gpu->dedicated.total = gpu->dedicated.used = 0;
    gpu->shared.total = gpu->shared.used = 0;
    gpu->coreCount = 0;
    gpu->coreUsage = 0;
    gpu->temperature = FF_GPU_TEMP_UNSET;
    ffStrbufInit(&gpu->driver);
    ffStrbufAppendS(&gpu->driver, "Metal");
    return NULL;
#endif
}
