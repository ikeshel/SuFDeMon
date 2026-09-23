// Author: Irakli Keshelashvili, 2026
// SuFDeMon - Super-FRS Detector Monitoring Software. GPL-3.0; see LICENSE.
#include "TSuFDeMonClient.h"
#include <TH1D.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    TSuFDeMonClient client("localhost", std::stoi(argv[1]));
    if (!client.Connect()) return 3;
    const std::string mode = argv[2];
    std::cout << "READY" << std::endl;
    if (mode == "idle") {
        std::string go;
        std::getline(std::cin, go);
        for (int retry = 0; retry < 100 && client.IsConnected(); ++retry)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else if (mode == "ping") {
        if (client.Ping()) return 4;
    } else if (mode == "list") {
        if (!client.ListHistograms().empty()) return 5;
    } else if (mode == "get") {
        if (client.GetHistogram("TH1D_MUSIC_ADC_FC1_ADC0")) return 6;
    } else if (mode == "clear") {
        if (client.ClearAll()) return 7;
    } else return 8;
    if (client.IsConnected()) return 9;
    // Disconnect after a detected failure must return without waiting for QUIT.
    client.Disconnect();
    std::cout << "DISCONNECTED" << std::endl;
    return 0;
}
