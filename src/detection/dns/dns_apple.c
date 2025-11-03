#include "detection/dns/dns.h"
#include "common/io/io.h"
#include "util/mallocHelper.h"
#include "util/stringUtils.h"
#include "util/apple/cf_helpers.h"
#include "util/debug.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(TARGET_OS_MAC) && !TARGET_OS_IPHONE
#include <SystemConfiguration/SystemConfiguration.h>

static const char* detectDnsFromConf(const char* path, FFDNSOptions* options, FFlist* results)
{
    // ... keep your existing resolv.conf parsing code here ...
}

const char* ffDetectDNS(FFDNSOptions* options, FFlist* results)
{
    // macOS: Use SystemConfiguration framework first
    FF_CFTYPE_AUTO_RELEASE SCDynamicStoreRef store = SCDynamicStoreCreate(NULL, CFSTR("fastfetch"), NULL, NULL);
    if (store)
    {
        FF_CFTYPE_AUTO_RELEASE CFStringRef key = SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetDNS);
        if (key)
        {
            FF_CFTYPE_AUTO_RELEASE CFDictionaryRef dict = SCDynamicStoreCopyValue(store, key);
            if (dict)
            {
                CFArrayRef dnsServers = CFDictionaryGetValue(dict, kSCPropNetDNSServerAddresses);
                if (dnsServers)
                {
                    FF_STRBUF_AUTO_DESTROY buffer = ffStrbufCreate();
                    for (CFIndex i = 0; i < CFArrayGetCount(dnsServers); i++)
                    {
                        if (ffCfStrGetString(CFArrayGetValueAtIndex(dnsServers, i), &buffer) == NULL)
                        {
                            if ((ffStrbufContainC(&buffer, ':') && !(options->showType & FF_DNS_TYPE_IPV6_BIT)) ||
                                (ffStrbufContainC(&buffer, '.') && !(options->showType & FF_DNS_TYPE_IPV4_BIT)))
                                continue;

                            FFstrbuf* item = (FFstrbuf*) ffListAdd(results);
                            ffStrbufInitMove(item, &buffer);
                        }
                    }
                }
            }
        }
    }

    if (results->length > 0)
        return NULL;

    // Fallback to resolv.conf
    return detectDnsFromConf("/var/run/resolv.conf", options, results);
}

#else
// iOS stub: DNS detection is not available
const char* ffDetectDNS(FFDNSOptions* options, FFlist* results)
{
    (void)options;
    (void)results;
    return "DNS detection is not supported on iOS";
}
#endif
