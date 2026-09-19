#include "TSupFDetMonGui.h"
#include "SupFDetMonProtocol.h"

#include <TApplication.h>
#include <TGClient.h>

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SupFDetMon::Protocol::kDefaultPort;

    if (argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [host] [port]\n";
        return 1;
    }

    if (argc >= 2)
        host = argv[1];

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

    // ROOT GUI itself does not need the SupFDetMon host/port arguments.
    int rootArgc = 1;
    char* rootArgv[] = {argv[0], nullptr};
    TApplication application("SupFDetMonGui", &rootArgc, rootArgv);

    new TSupFDetMonGui(gClient->GetRoot(), 1200, 700, host, port);

    application.Run();
    return 0;
}
