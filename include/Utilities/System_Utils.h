#pragma once

#include "globalDefines.h"
#include <Arduino.h>
#include <unordered_map>
#include <vector>
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "driver/adc.h"
#include "EventHandler.h"
#include <string>

#include "esp_ota_ops.h"
#include "mbedtls/base64.h"

#include "esp_rom_crc.h"

#include "FilesystemUtils.h"
#include <NimBLEDevice.h>

namespace 
{
    const char * COMMAND_FIELD PROGMEM = "cmd";
    const char * DISPLAY_BUFFER_FIELD PROGMEM = "buffer";
    const char * DISPLAY_WIDTH PROGMEM = "width";
    const char * DISPLAY_HEIGHT PROGMEM = "height";

    const char *DEVICE_NAME PROGMEM = "ESP32";

    const char* DEGEN_SERVICE_UUID = "033c3d34-8405-46db-8326-07169d5353a9";
    const char* WIFI_NAME_CHARACTERISTIC_UUID = "033c3d35-8405-46db-8326-07169d5353a9";
    const char* WIFI_PASSWORD_CHARACTERISTIC_UUID = "033c3d36-8405-46db-8326-07169d5353a9";
}

struct {
    esp_ota_handle_t handle = 0;
    const esp_partition_t* partition = nullptr;
    size_t total_size = 0;
    size_t bytes_written = 0;
    bool active = false;
} ota_state;

enum DebugCommand
{
    DISPLAY_CONTENTS = 0,
    REGISTER_INPUT = 1,
};

class System_Utils
{
public:
    static std::string DeviceName;
    static size_t DeviceID;

    static bool silentMode;
    static bool time24Hour;
    // static Adafruit_SSD1306 *OLEDdisplay;

    static void init();
    static long getBatteryPercentage();
    static void monitorSystemHealth(TimerHandle_t xTimer);
    static void shutdownBatteryWarning();

    // Timer functionality
    static int registerTimer(const char *timerName, size_t periodMS, TimerCallbackFunction_t callback);
    static int registerTimer(const char *timerName, size_t periodMS, TimerCallbackFunction_t callback, StaticTimer_t &timerBuffer);
    static void deleteTimer(int timerID);
    static bool isTimerActive(int timerID);
    static void startTimer(int timerID);
    static void stopTimer(int timerID);
    static void resetTimer(int timerID);
    static void changeTimerPeriod(int timerID, size_t timerPeriodMS);

    // Queue functionality
    static int registerQueue(size_t queueLength, size_t itemSize);
    static int registerQueue(size_t queueLength, size_t itemSize, uint8_t *queueData, StaticQueue_t &queueBuffer);
    static QueueHandle_t getQueue(int queueID);
    static void deleteQueue(int queueID);
    static void resetQueue(int queueID);
    static bool sendToQueue(int queueID, void *item, size_t timeoutMS); 

    // Task functionality
    // Dynamic memory allocation, no pinned core
    static int registerTask(
        TaskFunction_t taskFunction, 
        const char *taskName, 
        uint32_t taskStackSize, 
        void *taskParameters, 
        UBaseType_t taskPriority);

    // Dynamic memory allocation, pinned to core
    static int registerTask(
        TaskFunction_t taskFunction, 
        const char *taskName, 
        uint32_t taskStackSize, 
        void *taskParameters, 
        UBaseType_t taskPriority,
         BaseType_t coreID);

    // Static memory allocation, no pinned core
    static int registerTask(
        TaskFunction_t taskFunction, 
        const char *taskName, 
        uint32_t taskStackSize, 
        void *taskParameters, 
        UBaseType_t taskPriority, 
        StackType_t &stackBuffer, 
        StaticTask_t &taskBuffer);

    // Static memory allocation, pinned to core
    static int registerTask(
        TaskFunction_t taskFunction,
        const char *taskName, 
        uint32_t taskStackSize, 
        void *taskParameters, 
        UBaseType_t taskPriority, 
        StackType_t &stackBuffer, 
        StaticTask_t &taskBuffer, 
        BaseType_t coreID);    

    static void suspendTask(int taskID);
    static void resumeTask(int taskID);
    static void deleteTask(int taskID);
    static TaskHandle_t getTask(int taskID);

    // Bluetooth functionality
    static void initBluetooth();

    // WiFi functionality
    static bool enableWiFi();
    static void disableWiFi();
    static IPAddress getLocalIP();

    // 2.4Ghz Radio functionality
    static void enableRadio();
    static void disableRadio();

    // OTA Firmware Update
    static bool otaInitialized;
    static int otaTimerID;
    static bool initializeOTA();
    static void startOTA();
    static void stopOTA();

    // RPC OTA
    static int DecodeBase64(const char* input, uint8_t* output, size_t output_len);
    static void StartOtaRpc(JsonDocument &doc);
    static void UploadOtaChunkRpc(JsonDocument &doc);
    static void EndOtaRpc(JsonDocument &doc);

    // Debug Companion Functionality
    static void sendDisplayContents(Adafruit_SSD1306 *display);

    // Event Handler public invoke functions
    static void enableInterruptsInvoke();
    static void disableInterruptsInvoke();
    static void systemShutdownInvoke();

    // Event Handler Getters
    static EventHandler &getEnableInterrupts() { return enableInterrupts; }
    static EventHandler &getDisableInterrupts() { return disableInterrupts; }
    static EventHandler &getSystemShutdown() { return systemShutdown; }

    static void UpdateSettings(JsonDocument &settings);

 

private:
    // Event Handlers
    static EventHandler enableInterrupts;
    static EventHandler disableInterrupts;
    static EventHandler systemShutdown;

    // Timer functionality
    static std::unordered_map<int, TimerHandle_t> systemTimers;
    static int nextTimerID;

    // Task functionality
    static std::unordered_map<int, TaskHandle_t> systemTasks;
    static int nextTaskID;

    // Queue functionality
    static std::unordered_map<int, QueueHandle_t> systemQueues;
    static int nextQueueID;

    // ADC Users
    static std::unordered_map<uint8_t, bool> adcUsers;

    static StaticTimer_t healthTimerBuffer;
    static int healthTimerID;
    static int otaTaskID;
};