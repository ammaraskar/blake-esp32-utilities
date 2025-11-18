#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <map>
#include <unordered_map>
#include <vector>
#include "Button_Flash.h"
#include "LED_Utils.h"
#include "EventDeclarations.h"
#include "OLED_Window.h"
#include "Settings_Window.h"
#include "Compass_Window.h"
#include "GPS_Window.h"
#include "LoRa_Test_Window.h"
#include "ReceivedMessagesWindow.h"
// #include "Ping_Window.h"
#include "Home_Window.h"
#include "SOS_Window.h"
#include "EditStatusMessagesWindow.h"
#include "EditSavedLocationsWindow.h"
#include "Menu_Window.h"
#include "OTA_Update_Window.h"
#include "LoraUtils.h"
#include "NavigationUtils.h"
#include "SaveStatusMessageState.h"
#include "SaveLocationState.h"
#include "DiagnosticsWindow.h"
#include "WiFiRpcWindow.h"
#include "PairBluetoothWindow.h"

#include "Lock_State.h"

#include "Select_Content_List_State.h"

#include "globalDefines.h"
// #include "esp_event_base.h"

#define DEBOUNCE_DELAY 100
#define DISPLAY_COMMAND_QUEUE_LENGTH 1

using callbackPointer = void (*)(uint8_t);
using inputCallbackPointer = void (*)();

class Display_Manager
{
public:

    static Adafruit_SSD1306 display;

    static OLED_Window *currentWindow;
    static OLED_Window *rootWindow;

    static std::map<uint32_t, callbackPointer> callbackMap;
    static std::map<uint8_t, inputCallbackPointer> inputCallbackMap;
    // static std::unordered_map<size_t, uint8_t> inputMap;

    static void init();

    static OLED_Window *attachNewWindow();
    static void attachNewWindow(OLED_Window *window);

    static QueueHandle_t getDisplayCommandQueue() { return displayCommandQueue; }

    // Input handler using FreeRTOS task notifications.
    static void processCommandQueue(void *taskParams);
    static void initializeCallbacks();
    static void processEventCallback(uint32_t resourceID, uint8_t inputID);
    static void processInputCallback(uint8_t inputID);
    static void registerCallback(uint32_t resourceID, callbackPointer callback);
    static void registerInputCallback(uint8_t inputID, inputCallbackPointer callback);
    // static void registerInput(uint32_t resourceID, uint8_t inputID);
    static void displayLowBatteryShutdownNotice();

    // Enables the lock screen state. Setting timeoutMS above 0 will lock the screen after a specified time.
    // Setting timeoutMS to 0 will only lock the screen when explicitly called.
    static void enableLockScreen(size_t timeoutMS);
    static void cbLockDevice(xTimerHandle xTimer);

    // Callbacks
    static void goBack(uint8_t inputID);
    static void select(uint8_t inputID);
    static void generateHomeWindow(uint8_t inputID);
    static void generateSettingsWindow(uint8_t inputID);
    // static void generatePingWindow(uint8_t inputID);
    static void generateStatusesWindow(uint8_t inputID);
    static void generateMenuWindow(uint8_t inputID);
    static void generateCompassWindow(uint8_t inputID);
    static void generateGPSWindow(uint8_t inputID);
    static void generateLoRaTestWindow(uint8_t inputID);
    static void flashDefaultSettings(uint8_t inputID);
    static void rebootDevice(uint8_t inputID);
    static void toggleFlashlight(uint8_t inputID);
    static void shutdownDevice(uint8_t inputID);
    static void toggleSilentMode(uint8_t inputID);
    static void quickActionMenu(uint8_t inputID);
    static void openSavedMsg(uint8_t inputID);
    static void switchWindowState(uint8_t inputID);
    static void callFunctionalWindowState(uint8_t inputID);
    static void returnFromFunctionWindowState(uint8_t inputID);
    static void lockDevice(uint8_t inputID);
    static void openOTAWindow(uint8_t inputID);
    static void openSavedLocationsWindow(uint8_t inputID);
    static void openDiagnosticsWindow(uint8_t inputID);
    static void openWiFiRpcWindow(uint8_t inputID);

    static void initializeBle(uint8_t inputID);
    // static void callFunctionWindowState(uint8_t inputID);

    // Input callbacks
    static void processMessageReceived();
    static void openSOS();

private:
    static StaticTimer_t refreshTimer;
    static int refreshTimerID;

    static int buttonFlashAnimationID;

    static void refreshTimerCallback(TimerHandle_t xTimer)
    {
        if (currentWindow != nullptr)
        {
            currentWindow->drawWindow();
        }
    }

    static TickType_t lastButtonPressTick;
    // static std::vector<uint8_t> getInputsFromNotification(uint32_t notification);

    static int lockStateTimerID;
    static Lock_State *lockState;

    static QueueHandle_t displayCommandQueue;
    static StaticQueue_t displayCommandQueueBuffer;
    static uint8_t displayCommandQueueStorage[DISPLAY_COMMAND_QUEUE_LENGTH * sizeof(DisplayCommandQueueItem)];
};