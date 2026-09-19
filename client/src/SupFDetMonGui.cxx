#include "TSupFDetMonGui.h"
#include "SupFDetMonProtocol.h"

#include <TGClient.h>
#include <TRint.h>

#include <exception>
#include <iostream>
#include <string>

TSupFDetMonGui* gSupFDetMonGui = nullptr;

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SupFDetMon::Protocol::kDefaultPort;

    if (argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [host] [port]\n";
        return 1;
    }
    if (argc >= 2) host = argv[1];
    if (argc == 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (const std::exception&) {
            std::cerr << "Invalid port: " << argv[2] << '\n';
            return 1;
        }
        if (port < 1 || port > 65535) {
            std::cerr << "Port must be in the range 1..65535.\n";
            return 1;
        }
    }

    // TRint gives the GUI process the normal interactive ROOT command line too.
    int rootArgc = 1;
    char* rootArgv[] = {argv[0], nullptr};
    TRint application("SupFDetMonGui", &rootArgc, rootArgv);

    gSupFDetMonGui = new TSupFDetMonGui(gClient->GetRoot(), 430, 360, host, port);

    std::cout << "\nSupFDetMon GUI started. ROOT command line is active.\n"
              << "The control window and TCanvas are separate windows.\n"
              << "Global GUI pointer: gSupFDetMonGui\n\n";

    application.Run();

    gSupFDetMonGui = nullptr;
    return 0;
}
