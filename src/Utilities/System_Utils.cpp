#include "System_Utils.h"
std::string System_Utils::DeviceName = "ESP32";
size_t System_Utils::DeviceID = 0;

bool System_Utils::silentMode = true;
bool System_Utils::time24Hour = false;

std::unordered_map<int, TimerHandle_t> System_Utils::systemTimers;
int System_Utils::nextTimerID = 0;

std::unordered_map<int, TaskHandle_t> System_Utils::systemTasks;
int System_Utils::nextTaskID = 0;

std::unordered_map<int, QueueHandle_t> System_Utils::systemQueues;
int System_Utils::nextQueueID = 0;

std::unordered_map<uint8_t, bool> System_Utils::adcUsers;

StaticTimer_t System_Utils::healthTimerBuffer;
int System_Utils::healthTimerID;
// Adafruit_SSD1306 *System_Utils::OLEDdisplay = nullptr;
bool System_Utils::otaInitialized = false;
int System_Utils::otaTaskID = -1;

// Event Handlers
EventHandler System_Utils::enableInterrupts;
EventHandler System_Utils::disableInterrupts;
EventHandler System_Utils::systemShutdown;

void System_Utils::init()
{
#if HARDWARE_VERSION == 1
    healthTimerID = registerTimer("System Health Monitor", 60000, monitorSystemHealth, healthTimerBuffer);
    startTimer(healthTimerID);
    monitorSystemHealth(nullptr);
#endif
}

// TODO: Make actual battery curve
long System_Utils::getBatteryPercentage()
{
    uint16_t voltage = analogRead(BATT_SENSE_PIN);

#if DEBUG == 1
    // Serial.print("Battery voltage: ");
    // Serial.println(voltage);
#endif

    // Show full battery if BATT_SENSE_PIN is low. Device is plugged in.

    if (voltage == 0)
    {
        return 100;
    }

    if (voltage > 2100)
    {
        voltage = 2100;
    }
    else if (voltage < 1750)
    {
        voltage = 1750;
    }
    long percentage = map(voltage, 1750, 2100, 0, 100);
    return percentage;
}

void System_Utils::monitorSystemHealth(TimerHandle_t xTimer)
{
    uint16_t voltage = analogRead(BATT_SENSE_PIN);

    if (voltage < 1750)
    {
        // Battery is low. Shut down.

        // Show message and flash leds before turning off

        digitalWrite(KEEP_ALIVE_PIN, LOW);
    }
}

void System_Utils::shutdownBatteryWarning()
{
    systemShutdown.Invoke();
}


void System_Utils::enableInterruptsInvoke()
{
    #if DEBUG == 1
    // Serial.println("Enabling interrupts");
    #endif
    enableInterrupts.Invoke();
}

void System_Utils::disableInterruptsInvoke()
{
    #if DEBUG == 1
    // Serial.println("Disabling interrupts");
    #endif
    disableInterrupts.Invoke();
}

void System_Utils::systemShutdownInvoke()
{
    #if DEBUG == 1
    // Serial.println("Shutting down system");
    #endif
    systemShutdown.Invoke();
}

int System_Utils::registerTimer(const char *timerName, size_t periodMS, TimerCallbackFunction_t callback)
{
#if DEBUG == 1
    // Serial.print("Registering timer: ");
    // Serial.println(timerName);
#endif

    TimerHandle_t handle = xTimerCreate(timerName, periodMS, pdTRUE, (void *)0, callback);

    if (handle != nullptr)
    {
        systemTimers[nextTimerID] = handle;
        return nextTimerID++;
    }
    else
    {
        return -1;
    }
}

int System_Utils::registerTimer(const char *timerName, size_t periodMS, TimerCallbackFunction_t callback, StaticTimer_t &timerBuffer)
{
#if DEBUG == 1
    // Serial.print("Registering static timer: ");
    // Serial.println(timerName);
#endif
    TimerHandle_t handle = xTimerCreateStatic(timerName, periodMS, pdTRUE, (void *)0, callback, &timerBuffer);

    if (handle != nullptr)
    {
        systemTimers[nextTimerID] = handle;
        return nextTimerID++;
    }
    else
    {
        return -1;
    }
}

void System_Utils::deleteTimer(int timerID)
{
#if DEBUG == 1
    // Serial.print("Deleting timer: ");
    // Serial.println(timerID);
#endif

    if (systemTimers.find(timerID) != systemTimers.end())
    {
        xTimerDelete(systemTimers[timerID], 0);
        systemTimers.erase(timerID);
    }
}

bool System_Utils::isTimerActive(int timerID)
{
#if DEBUG == 1
    // Serial.print("Checking if timer is active: ");
    // Serial.println(timerID);
#endif
    if (systemTimers.find(timerID) != systemTimers.end())
    {
        return xTimerIsTimerActive(systemTimers[timerID]);
    }
    return false;
}

void System_Utils::startTimer(int timerID)
{
#if DEBUG == 1
    // Serial.print("Starting timer: ");
    // Serial.println(timerID);
#endif

    if (systemTimers.find(timerID) != systemTimers.end())
    {
        xTimerStart(systemTimers[timerID], 1000);
    }
}

void System_Utils::stopTimer(int timerID)
{
#if DEBUG == 1
    // Serial.print("Stopping timer: ");
    // Serial.println(timerID);
#endif

    if (systemTimers.find(timerID) != systemTimers.end())
    {
        if (xTimerStop(systemTimers[timerID], 1000) == pdFAIL)
        {
            #if DEBUG == 1
            Serial.println("Error stopping timer");
            #endif
        }
    }
    else
    {
        #if DEBUG == 1
        Serial.println("unable to find timer");
        #endif
    }
}

void System_Utils::resetTimer(int timerID)
{
#if DEBUG == 1
    // Serial.print("Resetting timer: ");
    // Serial.println(timerID);
#endif

    if (systemTimers.find(timerID) != systemTimers.end())
    {
        xTimerReset(systemTimers[timerID], 0);
    }
}

void System_Utils::changeTimerPeriod(int timerID, size_t timerPeriodMS)
{
#if DEBUG == 1
    // Serial.print("Changing timer period: ");
    // Serial.println(timerID);
#endif

    if (systemTimers.find(timerID) != systemTimers.end())
    {
        xTimerChangePeriod(systemTimers[timerID], pdMS_TO_TICKS(timerPeriodMS), 0);
    }
}

// Queue functionality

int System_Utils::registerQueue(size_t queueLength, size_t itemSize)
{
    QueueHandle_t handle = xQueueCreate(queueLength, itemSize);

    if (handle != nullptr)
    {
        systemQueues[nextQueueID] = handle;
        return nextQueueID++;
    }
    else
    {
        return -1;
    }
}

int System_Utils::registerQueue(size_t queueLength, size_t itemSize, uint8_t *queueData, StaticQueue_t &queueBuffer)
{
    QueueHandle_t handle = xQueueCreateStatic(queueLength, itemSize, queueData, &queueBuffer);

    if (handle != nullptr)
    {
        systemQueues[nextQueueID] = handle;
        return nextQueueID++;
    }
    else
    {
        return -1;
    }
}

QueueHandle_t System_Utils::getQueue(int queueID)
{
    if (systemQueues.find(queueID) != systemQueues.end())
    {
        return systemQueues[queueID];
    }
    else
    {
        return nullptr;
    }
}

void System_Utils::deleteQueue(int queueID)
{
    if (systemQueues.find(queueID) != systemQueues.end())
    {
        vQueueDelete(systemQueues[queueID]);
        systemQueues.erase(queueID);
    }
}

void System_Utils::resetQueue(int queueID)
{
    if (systemQueues.find(queueID) != systemQueues.end())
    {
        xQueueReset(systemQueues[queueID]);
    }
}

bool System_Utils::sendToQueue(int queueID, void *item, size_t timeoutMS)
{
    if (systemQueues.find(queueID) != systemQueues.end())
    {
        return xQueueSend(systemQueues[queueID], item, pdMS_TO_TICKS(timeoutMS)) == pdPASS;
    }
    else
    {
        return false;
    }
}

// Task functionality

int System_Utils::registerTask(
    TaskFunction_t taskFunction, 
    const char *taskName, 
    uint32_t taskStackSize, 
    void *taskParameters, 
    UBaseType_t taskPriority)
{
    TaskHandle_t handle;
    BaseType_t status = xTaskCreate(taskFunction, taskName, taskStackSize, taskParameters, taskPriority, &handle);

    if (status == pdPASS)
    {
        // Add task to systemTasks
        systemTasks[nextTaskID] = handle;
        return nextTaskID++;
    }
    else
    {
        return -1;
    }
}

int System_Utils::registerTask(
    TaskFunction_t taskFunction, 
    const char *taskName, 
    uint32_t taskStackSize, 
    void *taskParameters, 
    UBaseType_t taskPriority, 
    BaseType_t coreID)
{
    TaskHandle_t handle;
    BaseType_t status = xTaskCreatePinnedToCore(taskFunction, taskName, taskStackSize, taskParameters, taskPriority, &handle, coreID);

    if (status == pdPASS)
    {
        // Add task to systemTasks
        systemTasks[nextTaskID] = handle;
        return nextTaskID++;
    }
    else
    {
        #if DEBUG == 1
        Serial.print("Unable to create task: ");
        Serial.println(taskName);
        #endif
        return -1;
    }
    
}

int System_Utils::registerTask(
    TaskFunction_t taskFunction, 
    const char *taskName, 
    uint32_t taskStackSize, 
    void *taskParameters, 
    UBaseType_t taskPriority, 
    StackType_t &stackBuffer, 
    StaticTask_t &taskBuffer)
{
    auto handle = xTaskCreateStatic(taskFunction, taskName, taskStackSize, taskParameters, taskPriority, &stackBuffer, &taskBuffer);

    if (handle != nullptr)
    {
        // Add task to systemTasks
        systemTasks[nextTaskID] = handle;
        return nextTaskID++;
    }
    else
    {
        return -1;
    }

}

int System_Utils::registerTask(
    TaskFunction_t taskFunction, 
    const char *taskName, 
    uint32_t taskStackSize, 
    void *taskParameters, 
    UBaseType_t taskPriority, 
    StackType_t &stackBuffer, 
    StaticTask_t &taskBuffer, 
    BaseType_t coreID)
{
    auto handle = xTaskCreateStaticPinnedToCore(taskFunction, taskName, taskStackSize, taskParameters, taskPriority, &stackBuffer, &taskBuffer, coreID);

    if (handle != nullptr)
    {
        // Add task to systemTasks
        systemTasks[nextTaskID] = handle;
        return nextTaskID++;
    }
    else
    {
        return -1;
    }
}

void System_Utils::suspendTask(int taskID)
{
    if (systemTasks.find(taskID) != systemTasks.end())
    {
        vTaskSuspend(systemTasks[taskID]);
    }
}

void System_Utils::resumeTask(int taskID)
{
    if (systemTasks.find(taskID) != systemTasks.end())
    {
        vTaskResume(systemTasks[taskID]);
    }
}

void System_Utils::deleteTask(int taskID)
{
    if (systemTasks.find(taskID) != systemTasks.end())
    {
        vTaskDelete(systemTasks[taskID]);
        systemTasks.erase(taskID);
    }
}

TaskHandle_t System_Utils::getTask(int taskID)
{
    if (systemTasks.find(taskID) != systemTasks.end())
    {
        return systemTasks[taskID];
    }
    else
    {
        return nullptr;
    }
}

// TODO: kill this
bool System_Utils::enableWiFi()
{
    enableRadio();
    WiFi.disconnect(false);  // Reconnect the network
    WiFi.mode(WIFI_STA);    // Switch WiFi on
 
    WiFi.begin("ESP32-OTA", "e65v41ev");
 
    size_t timeoutCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (timeoutCounter++ > 20)
        {
            return false;
        }
    }

    return true;
}

class SystemBLEServer : public NimBLEServerCallbacks {
    void onConnect(BLEServer* pServer, NimBLEConnInfo& connInfo) override {
        // Require all connections to be paired.
        BLEDevice::startSecurity(connInfo.getConnHandle());
    }

    void onDisconnect(BLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        // Start advertising again after the old client disconnects.
        BLEDevice::startAdvertising();
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if (connInfo.isBonded()) {
            // TODO: display this on screen
            // Serial.println("Pairing successful");
        } else {
            Serial.println("Pairing failed");
            // Disconnect them if pairing failed maybe?
        }
    }

    uint32_t onPassKeyDisplay() override {
        uint32_t pass_key = random(100000, 999999);
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

void System_Utils::initBluetooth()
{
    // TODO: get beacon name.
    std::string device_name = FilesystemModule::Utilities::SettingsFile()["Device Name"]["cfgVal"];
    std::string ble_name = "DegenBeacon " + device_name;
    BLEDevice::init(ble_name);

    // # Set security parameters
    // Require pairing (bonding) and SC for secure connection pairing.
    BLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/true, /*sc=*/true);
    // State that we can display a PIN code for pairing, but no input supported.
    BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new SystemBLEServer());

    BLEService* pService = pServer->createService(DEGEN_SERVICE_UUID);

    BLECharacteristic* pWifiNameCharacteristic = pService->createCharacteristic(
        WIFI_NAME_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::READ_ENC |
        NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ_AUTHEN |
        NIMBLE_PROPERTY::WRITE_AUTHEN
    );
    BLECharacteristic* pWifiPassCharacteristic = pService->createCharacteristic(
        WIFI_PASSWORD_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::READ_ENC |
        NIMBLE_PROPERTY::WRITE_ENC |
        NIMBLE_PROPERTY::READ_AUTHEN |
        NIMBLE_PROPERTY::WRITE_AUTHEN
    );
    pWifiNameCharacteristic->setValue("default_ssid");
    pWifiPassCharacteristic->setValue("default_password");

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setName(ble_name);
    pAdvertising->addServiceUUID(DEGEN_SERVICE_UUID);
    // Show up with a watch icon lol.
    #define BLE_APPEARANCE_GENERIC_WATCH 192
    pAdvertising->setAppearance(BLE_APPEARANCE_GENERIC_WATCH);

    BLEDevice::startAdvertising();
}

void System_Utils::disableWiFi()
{
    disableRadio();
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
}

IPAddress System_Utils::getLocalIP()
{
    return WiFi.localIP();
}

void System_Utils::enableRadio()
{
    WiFi.setSleep(false);
}

void System_Utils::disableRadio()
{
    WiFi.setSleep(true);
}

bool System_Utils::initializeOTA()
{
    if (otaInitialized)
    {
        return false;
    }

    ArduinoOTA.setHostname("ESP32-OTA");
    ArduinoOTA.setPort(8266);

    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

    otaTaskID = registerTask([](void *pvParameters) {
        for (;;)
        {
            ArduinoOTA.handle();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }, "OTA Handler", 8192, nullptr, 1);

    otaInitialized = true;

    return true;
}

void System_Utils::startOTA()
{
    if (!otaInitialized)
    {
        initializeOTA();
    }

    ArduinoOTA.begin();
    resumeTask(otaTaskID);
}

void System_Utils::stopOTA()
{
    suspendTask(otaTaskID);
    ArduinoOTA.end();
}

int System_Utils::DecodeBase64(const char* input, uint8_t* output, size_t output_len)
{
    size_t out_len = 0;
    int err = mbedtls_base64_decode(output, output_len, &out_len,
                                    reinterpret_cast<const unsigned char*>(input),
                                    strlen(input));
    return err == 0 ? out_len : -1;
}

void System_Utils::StartOtaRpc(JsonDocument &doc)
{
    size_t size = doc["size"] | 0;
    doc.clear();

    if (size == 0) {
        doc["error"] = "Missing or invalid 'size'";
        return;
    }

    ota_state.partition = esp_ota_get_next_update_partition(nullptr);
    if (!ota_state.partition) {
        doc["error"] = "No update partition available";
        return;
    }

    esp_err_t err = esp_ota_begin(ota_state.partition, size, &ota_state.handle);
    if (err != ESP_OK) {
        doc["error"] = "esp_ota_begin failed";
        doc["code"] = err;
        return;
    }

    ota_state.total_size = size;
    ota_state.bytes_written = 0;
    ota_state.active = true;

    doc["status"] = "OTA begin successful";
}

void System_Utils::UploadOtaChunkRpc(JsonDocument &doc)
{
    #if DEBUG == 1
    // Serial.println("UploadOtaChunkRpc packet: ");
    // serializeJsonPretty(doc, Serial);
    // Serial.println();
    #endif

    if (!doc.containsKey("chunk")) {
        doc.clear();
        doc["error"] = "Missing or invalid 'chunk'";
        return;
    }
    auto b64 = doc["chunk"].as<std::string>();
    
    if (!doc.containsKey("checksum")) {
        doc.clear();
        doc["error"] = "Missing or invalid 'checksum'";
        return;
    }
    auto checksum = doc["checksum"].as<uint32_t>();
    

    if (!ota_state.active) {
        doc.clear();
        doc["error"] = "OTA inactive";
        return;
    }

    size_t b64Len = strlen(b64.c_str());
    size_t binLen = (b64Len * 3) / 4;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[binLen]);

    int actualLen = DecodeBase64(b64.c_str(), buffer.get(), binLen);
    if (actualLen <= 0) {
        doc.clear();
        doc["error"] = "Base64 decode failed";
        return;
    }

    doc.clear();

    uint32_t calculatedChecksum = 0;
    for (size_t i = 0; i < actualLen; i++) {
        calculatedChecksum += buffer[i];
    }

    if (calculatedChecksum != checksum) {
        doc["error"] = "CRC mismatch";
        #if DEBUG == 1
        Serial.printf("expected checksum: %08X\n", checksum);
        Serial.printf("calculated checksum: %08X\n", calculatedChecksum);
        #endif
        return;
    }

    esp_err_t err = esp_ota_write(ota_state.handle, buffer.get(), actualLen);
    if (err != ESP_OK) {
        esp_ota_abort(ota_state.handle);
        ota_state.active = false;
        doc["error"] = "esp_ota_write failed";
        doc["code"] = err;
        return;
    }

    ota_state.bytes_written += actualLen;
    doc["written"] = actualLen;
    doc["total_written"] = ota_state.bytes_written;
    doc["remaining"] = ota_state.total_size - ota_state.bytes_written;
}

void System_Utils::EndOtaRpc(JsonDocument &doc)
{
    doc.clear();

    if (!ota_state.active) {
        doc["error"] = "OTA not active";
        return;
    }

    if (ota_state.bytes_written < ota_state.total_size) {
        doc["error"] = "Not enough data written";
        return;
    }

    esp_err_t err = esp_ota_end(ota_state.handle);
    if (err != ESP_OK) {
        doc["error"] = "esp_ota_end failed";
        doc["code"] = err;
        return;
    }

    err = esp_ota_set_boot_partition(ota_state.partition);
    if (err != ESP_OK) {
        doc["error"] = "esp_ota_set_boot_partition failed";
        doc["code"] = err;
        return;
    }

    ota_state.active = false;
    doc["status"] = "OTA complete";
}


void System_Utils::sendDisplayContents(Adafruit_SSD1306 *display)
{
    DynamicJsonDocument doc(10000);
    auto buffer = display->getBuffer();

    size_t bufferLength = (display->width() * display->height()) / 8;

    doc[COMMAND_FIELD] = (int)DISPLAY_CONTENTS;
    auto displayBuffer = doc.createNestedArray(DISPLAY_BUFFER_FIELD);

    doc[DISPLAY_WIDTH] = display->width();
    doc[DISPLAY_HEIGHT] = display->height();

    for (size_t i = 0; i < display->height() / 8; i++)
    {
        doc[DISPLAY_BUFFER_FIELD].createNestedArray();
    }

    for (size_t i = 0; i < display->width(); i++)
    {
        for (size_t j = 0; j < display->height() / 8; j++)
        {
            displayBuffer[j].add(buffer[i * (display->height() / 8) + j]);
        }
    }

    Serial.printf("Row size: %d\n", displayBuffer[0].size());

    ArduinoJson::serializeJson(doc, Serial);
}

void System_Utils::UpdateSettings(JsonDocument &settings)
{
    if (settings.containsKey("UserID"))
    {
        DeviceID = 0 | settings["UserID"]["cfgVal"].as<int>();
    }

    if (settings.containsKey("Device Name"))
    {
        DeviceName = settings["Device Name"]["cfgVal"].as<std::string>();
    }
}