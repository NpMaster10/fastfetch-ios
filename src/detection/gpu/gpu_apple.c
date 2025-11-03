#include "gpu.h"
#include "util/apple/cf_helpers.h"
#include "util/apple/smc_temps.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_OSX
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/IOKitLib.h>
#endif

static double detectGpuTemp(const FFstrbuf* gpuName)
{
#if TARGET_OS_OSX
    // Original macOS logic
    double result = 0;
    const char* error = NULL;

    if (ffStrbufStartsWithS(gpuName, "Apple M"))
    {
        switch (strtol(gpuName->chars + strlen("Apple M"), NULL, 10))
        {
            case 0: error = "Invalid Apple Silicon GPU"; break;
            case 1: error = ffDetectSmcTemps(FF_TEMP_GPU_M1X, &result); break;
            case 2: error = ffDetectSmcTemps(FF_TEMP_GPU_M2X, &result); break;
            case 3: error = ffDetectSmcTemps(FF_TEMP_GPU_M3X, &result); break;
            case 4: error = ffDetectSmcTemps(FF_TEMP_GPU_M4X, &result); break;
            default: error = "Unsupported Apple Silicon GPU"; break;
        }
    }
    else if (ffStrbufStartsWithS(gpuName, "Intel"))
        error = ffDetectSmcTemps(FF_TEMP_GPU_INTEL, &result);
    else if (ffStrbufStartsWithS(gpuName, "Radeon") || ffStrbufStartsWithS(gpuName, "AMD"))
        error = ffDetectSmcTemps(FF_TEMP_GPU_AMD, &result);
    else
        error = ffDetectSmcTemps(FF_TEMP_GPU_UNKNOWN, &result);

    if (error)
        return FF_GPU_TEMP_UNSET;

    return result;
#else
    (void)gpuName;
    return FF_GPU_TEMP_UNSET;
#endif
}

const char* ffDetectGPUImpl(const FFGPUOptions* options, FFlist* gpus)
{
#if TARGET_OS_OSX
    // Original macOS IOKit logic goes here...
    // Keep all your existing code
    return NULL;
#else
    // iOS dummy GPU
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
