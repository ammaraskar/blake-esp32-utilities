#pragma once

#include "OLED_Window.h"
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
        _state.assignInput(BUTTON_3, ACTION_BACK, "Back");

        setInitialState(&_state);
        Display_Utils::enableRefreshTimer(500);
    }

    ~PairBluetoothWindow() 
    {
        // Disable bluetooth here.
    }

    void drawWindow() 
    {
        OLED_Window::drawWindow();

        std::string msg;
        if (!System_Utils::bluetoothConnected()) {
            TextFormat tf;
            tf.horizontalAlignment = ALIGN_CENTER_HORIZONTAL;
            tf.verticalAlignment = TEXT_LINE;
            tf.line = 1;
            Display_Utils::printFormattedText("Waiting for", tf);
            
            tf.line = 2;
            Display_Utils::printFormattedText("Connection...", tf);

            tf.line = 4;
            Display_Utils::printFormattedText("Visit", tf);

            tf.line = 5;
            Display_Utils::printFormattedText("degen.ammaraskar.com", tf);
        } else if (System_Utils::bluetoothConnected() && !System_Utils::bluetoothPaired()) {
            TextFormat prompt;
            prompt.horizontalAlignment = ALIGN_CENTER_HORIZONTAL;
            prompt.verticalAlignment = ALIGN_CENTER_VERTICAL;

            std::string msg = "PIN: " + std::to_string(System_Utils::bluetoothPin());
            Display_Utils::printFormattedText(msg.c_str(), prompt);
        } else if (System_Utils::bluetoothConnected() && System_Utils::bluetoothPaired()) {
            TextFormat prompt;
            prompt.horizontalAlignment = ALIGN_CENTER_HORIZONTAL;
            prompt.verticalAlignment = ALIGN_CENTER_VERTICAL;

            Display_Utils::printFormattedText("Connected!", prompt);
        }



        Display_Utils::UpdateDisplay().Invoke();
    }

protected:
    Window_State _state;
};