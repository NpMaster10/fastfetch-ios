#include "sound.h"
#include "util/apple/cf_helpers.h"

#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
#include <CoreAudio/CoreAudio.h>
#include <AvailabilityMacros.h>

#ifndef MAC_OS_VERSION_12_0
#define kAudioObjectPropertyElementMain kAudioObjectPropertyElementMaster
#endif
#endif

const char* ffDetectSound(FFlist* devices /* List of FFSoundDevice */)
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE

    AudioDeviceID mainDeviceId;
    UInt32 dataSize = sizeof(mainDeviceId);
    if(AudioObjectGetPropertyData(kAudioObjectSystemObject, &(AudioObjectPropertyAddress){
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMain
    }, 0, NULL, &dataSize, &mainDeviceId) != kAudioHardwareNoError)
        return "AudioObjectGetPropertyData(kAudioHardwarePropertyDefaultOutputDevice) failed";

    AudioObjectID deviceIds[32] = {};
    dataSize = sizeof(deviceIds);
    if(AudioObjectGetPropertyData(kAudioObjectSystemObject, &(AudioObjectPropertyAddress){
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMain
    }, 0, NULL, &dataSize, &deviceIds) != kAudioHardwareNoError)
        return "AudioObjectGetPropertyData(kAudioHardwarePropertyDevices) failed";

    for(uint32_t index = 0, length = dataSize / sizeof(*deviceIds); index < length; ++index)
    {
        AudioDeviceID deviceId = deviceIds[index];

        // Ignore input devices
        if(AudioObjectGetPropertyDataSize(deviceId, &(AudioObjectPropertyAddress){
            kAudioDevicePropertyStreams,
            kAudioObjectPropertyScopeInput,
            kAudioObjectPropertyElementMain
        }, 0, NULL, &dataSize) == kAudioHardwareNoError && dataSize > 0)
            continue;

        FFSoundDevice* device = (FFSoundDevice*) ffListAdd(devices);
        device->main = deviceId == mainDeviceId;
        device->active = false;
        device->volume = FF_SOUND_VOLUME_UNKNOWN;
        ffStrbufInit(&device->identifier);
        ffStrbufInit(&device->name);
        ffStrbufInitStatic(&device->platformApi, "Core Audio");

        // Device UID
        FF_CFTYPE_AUTO_RELEASE CFStringRef uid = NULL;
        dataSize = sizeof(uid);
        if(AudioObjectGetPropertyData(deviceId, &(AudioObjectPropertyAddress) {
            kAudioDevicePropertyDeviceUID,
            kAudioObjectPropertyScopeOutput,
            kAudioObjectPropertyElementMain
        }, 0, NULL, &dataSize, &uid) == kAudioHardwareNoError)
            ffCfStrGetString(uid, &device->identifier);
        else
            ffStrbufAppendF(&device->identifier, "ID-%u", (unsigned) deviceId);

        // Device Name
        FF_CFTYPE_AUTO_RELEASE CFStringRef name = NULL;
        dataSize = sizeof(name);
        if(AudioObjectGetPropertyData(deviceId, &(AudioObjectPropertyAddress){
            kAudioObjectPropertyName,
            kAudioObjectPropertyScopeOutput,
            kAudioObjectPropertyElementMain
        }, 0, NULL, &dataSize, &name) == kAudioHardwareNoError)
            ffCfStrGetString(name, &device->name);
        else
            ffStrbufSet(&device->name, &device->identifier);

        uint32_t muted;
        dataSize = sizeof(muted);
        if(AudioObjectGetPropertyData(deviceId, &(AudioObjectPropertyAddress){
            kAudioDevicePropertyMute,
            kAudioObjectPropertyScopeOutput,
            kAudioObjectPropertyElementMain
        }, 0, NULL, &dataSize, &muted) != kAudioHardwareNoError)
            muted = false; // Device may not support volume control

        uint32_t active;
        dataSize = sizeof(active);
        if(AudioObjectGetPropertyData(deviceId, &(AudioObjectPropertyAddress){
            kAudioDevicePropertyDeviceIsAlive,
            kAudioObjectPropertyScopeOutput,
            kAudioObjectPropertyElementMain
        }, 0, NULL, &dataSize, &active) == kAudioHardwareNoError)
            device->active = !!active;

        if (muted)
            device->volume = 0;
        else
        {
            float volume;
            dataSize = sizeof(volume);
            if(AudioObjectGetPropertyData(deviceId, &(AudioObjectPropertyAddress){
                kAudioDevicePropertyVolumeScalar,
                kAudioObjectPropertyScopeOutput,
                kAudioObjectPropertyElementMain
            }, 0, NULL, &dataSize, &volume) == kAudioHardwareNoError)
                device->volume = (uint8_t) (volume * 100 + 0.5);
        }
    }

    return NULL;

#else
    (void)devices; // suppress unused parameter warning
    return "Sound detection not supported on iOS";
#endif
}
