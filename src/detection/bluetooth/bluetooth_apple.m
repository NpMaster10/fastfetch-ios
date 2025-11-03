#include "bluetooth.h"

#if defined(__APPLE__) && !TARGET_OS_IPHONE

#import <IOBluetooth/IOBluetooth.h>

@interface IOBluetoothDevice()
@property (nonatomic) uint8_t batteryPercentCase;
@property (nonatomic) uint8_t batteryPercentCombined;
@property (nonatomic) uint8_t batteryPercentLeft;
@property (nonatomic) uint8_t batteryPercentRight;
@property (nonatomic) uint8_t batteryPercentSingle;
@end

const char* ffDetectBluetooth(FFBluetoothOptions* options, FFlist* devices /* FFBluetoothResult */)
{
    NSArray<IOBluetoothDevice*>* ioDevices = IOBluetoothDevice.pairedDevices;
    if(!ioDevices)
        return "IOBluetoothDevice.pairedDevices failed";

    for(IOBluetoothDevice* ioDevice in ioDevices)
    {
        if (!options->showDisconnected && !ioDevice.isConnected)
            continue;

        FFBluetoothResult* device = ffListAdd(devices);
        ffStrbufInitS(&device->name, ioDevice.name.UTF8String);
        ffStrbufInitS(&device->address, ioDevice.addressString.UTF8String);
        ffStrbufReplaceAllC(&device->address, '-', ':');
        ffStrbufUpperCase(&device->address);
        ffStrbufInit(&device->type);

        if (ioDevice.batteryPercentSingle)
            device->battery = ioDevice.batteryPercentSingle;
        else if (ioDevice.batteryPercentCombined)
            device->battery = ioDevice.batteryPercentCombined;
        else if (ioDevice.batteryPercentCase)
            device->battery = ioDevice.batteryPercentCase;

        device->connected = !!ioDevice.isConnected;
        // ... rest of the macOS code remains unchanged
    }

    return NULL;
}

#else

// iOS stub
const char* ffDetectBluetooth(FFBluetoothOptions* options, FFlist* devices)
{
    (void)options;
    (void)devices;
    return NULL; // iOS has no IOBluetooth API
}

#endif
