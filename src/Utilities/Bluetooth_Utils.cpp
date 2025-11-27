#include "Bluetooth_Utils.h"

#include "RpcManager.h"

static bool gBluetoothConnected = false;
static int gBluetoothPin = 0;
static bool gBluetoothPaired = false;

bool Bluetooth_Utils::bluetoothConnected()
{
    return gBluetoothConnected;
}
bool Bluetooth_Utils::bluetoothPaired()
{
    return gBluetoothPaired;
}
int Bluetooth_Utils::bluetoothPin()
{
    return gBluetoothPin;
}

class SystemBLEServer : public NimBLEServerCallbacks {
    void onConnect(BLEServer* pServer, NimBLEConnInfo& connInfo) override {
        // Require all connections to be paired.
        BLEDevice::startSecurity(connInfo.getConnHandle());
        gBluetoothConnected = true;
    }

    void onDisconnect(BLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        // Start advertising again after the old client disconnects.
        BLEDevice::startAdvertising();
        gBluetoothConnected = false;
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if (connInfo.isBonded()) {
            gBluetoothPaired = true;
        } else {
            gBluetoothPaired = false;
        }
    }

    uint32_t onPassKeyDisplay() override {
        uint32_t pass_key = random(100000, 999999);
        gBluetoothPin = pass_key;
        return pass_key;
    }
};

class WifiNameCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
    }
};


class WifiPassCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
    }
};

#define MAX_BLE_RPC_PACKET_SIZE 1024 * 4
// 512 is the max BLE packet size, use 500 here to be safe.
#define MAX_OUTGOING_BLE_PACKET_SIZE 500

class RpcCharacteristicCallbacks : public NimBLECharacteristicCallbacks {

public:
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        // RPC requests come in here.
        NimBLEAttValue data = pCharacteristic->getValue();
        uint32_t payload_size = data.length() - 1;
        Serial.printf("[BLE] Incoming RPC packet size: %d\n", payload_size);

        if (payload_size + _incomingPacketBufferIndex > MAX_BLE_RPC_PACKET_SIZE) {
            // Overflow, reset buffer
            _incomingPacketBufferIndex = 0;
            ESP_LOGW("RpcCharacteristicCallbacks", "Incoming RPC packet overflow, resetting buffer");
            return;
        }

        memcpy(&_incomingPacketBuffer[_incomingPacketBufferIndex], &data.c_str()[1], payload_size);
        _incomingPacketBufferIndex += payload_size;

        // First byte tells us if more data is coming.
        bool moreComing = data[0];
        if (!moreComing) {
            processIncomingRpc();
            _incomingPacketBufferIndex = 0;
        }
    }

    void processIncomingRpc() {
        // Process the packet
        DynamicJsonDocument doc(1000);
        DeserializationError error = deserializeMsgPack(doc, _incomingPacketBuffer, _incomingPacketBufferIndex);
        if (error) {
            Serial.println("[BLE] Invalid MessagePack data");
            return;
        }

        auto returnCode = RpcModule::Utilities::CallRpc(doc[RpcModule::Utilities::RPC_FUNCTION_NAME_FIELD()].as<std::string>(), doc);

        size_t packedSize = measureMsgPack(doc);
        if (packedSize > MAX_BLE_RPC_PACKET_SIZE) {
            Serial.println("[BLE] Outgoing RPC packet too large");
            return;
        }
        _outgoingPacketBufferIndex = 0;
        _outgoingPacketSize = packedSize;
        Serial.printf("[BLE] Outgoing RPC packet size: %d\n", packedSize);

        serializeMsgPack(doc, _outgoingPacketBuffer, packedSize);
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        // Responses to RPCs go here.
        if (_outgoingPacketBufferIndex >= _outgoingPacketSize) {
            // No more data to send.
            pCharacteristic->setValue(_outgoingPacketBuffer, 0);
            return;
        }
        uint32_t bytesRemaining = _outgoingPacketSize - _outgoingPacketBufferIndex;

        bool moreComing = bytesRemaining > MAX_OUTGOING_BLE_PACKET_SIZE;
        uint32_t chunkSize = moreComing ? MAX_OUTGOING_BLE_PACKET_SIZE : bytesRemaining;

        uint8_t characteristic_buffer[MAX_OUTGOING_BLE_PACKET_SIZE + 1];
        // Set first byte to indicate if more data is coming.
        characteristic_buffer[0] = moreComing ? 1 : 0;
        memcpy(&characteristic_buffer[1], &_outgoingPacketBuffer[_outgoingPacketBufferIndex], chunkSize);

        pCharacteristic->setValue(characteristic_buffer, chunkSize + 1);
        _outgoingPacketBufferIndex += chunkSize;
    }

    uint8_t _incomingPacketBuffer[MAX_BLE_RPC_PACKET_SIZE];
    uint32_t _incomingPacketBufferIndex = 0;

    uint8_t _outgoingPacketBuffer[MAX_BLE_RPC_PACKET_SIZE];
    uint32_t _outgoingPacketBufferIndex = 0;
    uint32_t _outgoingPacketSize = 0;
};

void Bluetooth_Utils::initBluetooth()
{
    std::string device_name = FilesystemModule::Utilities::SettingsFile()["Device Name"]["cfgVal"];
    std::string ble_name = "DegenBeacon " + device_name;
    BLEDevice::init(ble_name);
    NimBLEDevice::setMTU(BLE_ATT_MTU_MAX); // Use maximum MTU for largest packets.

    // -- Set security parameters
    // Require pairing (bonding) and SC for secure connection pairing.
    BLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/true, /*sc=*/true);
    // State that we can display a PIN code for pairing, but no input supported.
    BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new SystemBLEServer());

    BLEService* pService = pServer->createService(DEGEN_SERVICE_UUID);

    BLECharacteristic* pRpcCharacteristic = pService->createCharacteristic(
        RPC_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::READ_ENC |
        NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ_AUTHEN |
        NIMBLE_PROPERTY::WRITE_AUTHEN
    );
    pRpcCharacteristic->setCallbacks(new RpcCharacteristicCallbacks());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setName(ble_name);
    pAdvertising->addServiceUUID(DEGEN_SERVICE_UUID);
    // Show up with a watch icon lol.
    #define BLE_APPEARANCE_GENERIC_WATCH 192
    pAdvertising->setAppearance(BLE_APPEARANCE_GENERIC_WATCH);

    BLEDevice::startAdvertising();
}
