#pragma once

#include <NimBLEDevice.h>

namespace 
{
    const char* DEGEN_SERVICE_UUID = "033c3d34-8405-46db-8326-07169d5353a9";

    const char* RPC_CHARACTERISTIC_UUID = "033c3d37-8405-46db-8326-07169d5353a9";
}

class Bluetooth_Utils {
public:
    static void initBluetooth();
    static bool bluetoothConnected();
    static bool bluetoothPaired();
    static int bluetoothPin();
};
