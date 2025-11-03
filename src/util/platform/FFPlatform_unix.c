#include "FFPlatform_private.h"
#include "util/FFstrbuf.h"
#include "util/stringUtils.h"
#include "fastfetch_config.h"
#include "common/io/io.h"

#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <sys/utsname.h>
#include <paths.h>

#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif


#ifdef __APPLE__
    #include <sys/sysctl.h>
    #if TARGET_OS_MAC && !TARGET_OS_IPHONE
        #include <libproc.h>
    #endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    #include <sys/sysctl.h>
#elif defined(__HAIKU__)
    #include <image.h>
    #include <OS.h>
#endif

static void getExePath(FFPlatform* platform)
{
    char exePath[PATH_MAX + 1];
#if defined(__linux__) || defined(__GNU__)
    ssize_t exePathLen = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (exePathLen >= 0)
        exePath[exePathLen] = '\0';
#elif defined(__APPLE__)
    int exePathLen = 0;
    #if TARGET_OS_MAC && !TARGET_OS_IPHONE
        exePathLen = proc_pidpath((int) getpid(), exePath, sizeof(exePath));
    #endif
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    size_t exePathLen = sizeof(exePath);
    if(sysctl(
        (int[]){CTL_KERN,
        #ifdef __FreeBSD__
            KERN_PROC, KERN_PROC_PATHNAME, (int) getpid()
        #else
            KERN_PROC_ARGS, (int) getpid(), KERN_PROC_PATHNAME
        #endif
        }, 4,
        exePath, &exePathLen,
        NULL, 0
    ) < 0)
        exePathLen = 0;
    else
        exePathLen--; // remove terminating NUL
#elif defined(__OpenBSD__)
    size_t exePathLen = 0;
#elif defined(__sun)
    ssize_t exePathLen = readlink("/proc/self/path/a.out", exePath, sizeof(exePath) - 1);
    if (exePathLen >= 0)
        exePath[exePathLen] = '\0';
#elif defined(__HAIKU__)
    size_t exePathLen = 0;
    image_info info;
    int32 cookie = 0;

    while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
        if (info.type == B_APP_IMAGE) {
            exePathLen = strlcpy(exePath, info.name, PATH_MAX);
            break;
        }
    }
#endif

    if (exePathLen > 0)
    {
        ffStrbufEnsureFree(&platform->exePath, PATH_MAX);
        if (realpath(exePath, platform->exePath.chars))
            ffStrbufRecalculateLength(&platform->exePath);
        else
            ffStrbufSetNS(&platform->exePath, (uint32_t) exePathLen, exePath);
    }
}
