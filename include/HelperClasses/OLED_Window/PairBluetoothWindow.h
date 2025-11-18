#pragma once

#include "OLED_Window.h"
#include "AwaitWifiState.h"
#include "IpUtils.h"
#include "RadioUtils.h"
#include "RpcUtils.h"

namespace
{
    const int BT_AWAITING_CONNECTION = 0;
    const int BT_DISPLAYING_PIN = 1;
    const int BT_CONNECTED = 2;
}

class PairBluetoothWindow : public OLED_Window
{
public:
    PairBluetoothWindow(OLED_Window *parent) : OLED_Window(parent) 
    {
        _wifiConnectState.assignInput(BUTTON_3, ACTION_BACK, "Back");
        _otherState.assignInput(BUTTON_3, ACTION_BACK, "Back");


        setInitialState(&_wifiConnectState);
        Display_Utils::enableRefreshTimer(500);
    }

    ~PairBluetoothWindow() 
    {
        // Disable bluetooth advertising here.
    }

    void drawWindow() 
    {
        OLED_Window::drawWindow();

        std::string msg;
        if (_btConnectionState == BT_AWAITING_CONNECTION) {
            msg = "Waiting for connection, go to degen.ammaraskar.com";
        } else if (_btConnectionState == BT_DISPLAYING_PIN) {
            msg = "Displaying PIN, enter on connecting device";
        } else if (_btConnectionState == BT_CONNECTED) {
            msg = "Device connected!";
        }

        TextFormat prompt;
        prompt.horizontalAlignment = ALIGN_CENTER_HORIZONTAL;
        prompt.verticalAlignment = ALIGN_CENTER_VERTICAL;

        Display_Utils::printFormattedText(msg.c_str(), prompt);

        Display_Utils::UpdateDisplay().Invoke();
    }

protected:
    // State to connect to an AP via saved credentials or SmartConfig
    AwaitWifiState _wifiConnectState;
    
    // State that displays IP the user can connect to
    Window_State _otherState;

private:
    int _btConnectionState = BT_AWAITING_CONNECTION;
};