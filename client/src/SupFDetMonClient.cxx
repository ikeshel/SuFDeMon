#include "TSupFDetMonClient.h"

#include "SupFDetMonProtocol.h"

#include <TH1D.h>

#include <iostream>
#include <sstream>
#include <string>

namespace {

void PrintHelp()
{
    std::cout
        << "Commands:\n"
        << "  ping                    Test the server connection\n"
        << "  list                    List available histograms\n"
        << "  get <name>              Fetch histogram and print statistics\n"
        << "  draw <name>             Fetch and draw histogram\n"
        << "  clear <name>            Clear one server histogram\n"
        << "  clear all               Clear all server histograms\n"
        << "  help                    Show this help\n"
        << "  quit                    Disconnect and exit\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SupFDetMon::Protocol::kDefaultPort;

    if (argc > 3) {
        std::cerr << "Usage: " << argv[0] << " [host] [port]" << std::endl;
        return 1;
    }

    if (argc >= 2) {
        host = argv[1];
    }

    if (argc == 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[2] << std::endl;
            return 1;
        }
    }

    TSupFDetMonClient client(host, port);

    if (!client.Connect()) {
        return 1;
    }

    std::cout << "Connected to " << host << ':' << port << std::endl;
    PrintHelp();

    std::string line;
    while (true) {
        std::cout << "SupFDetMon> " << std::flush;

        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if (line == "quit" || line == "exit") {
            break;
        }

        if (line == "help") {
            PrintHelp();
            continue;
        }

        if (line == "ping") {
            std::cout << (client.Ping() ? "PONG" : "Ping failed") << std::endl;
            continue;
        }

        if (line == "list") {
            std::cout << client.ListHistograms();
            continue;
        }

        if (line == "clear all") {
            std::cout << (client.ClearAll() ? "OK" : "Clear failed") << std::endl;
            continue;
        }

        std::istringstream input(line);
        std::string command;
        std::string name;
        input >> command >> name;

        if (command == "get" && !name.empty()) {
            auto histogram = client.GetHistogram(name);

            if (histogram) {
                std::cout << histogram->GetName()
                          << ": entries=" << histogram->GetEntries()
                          << ", mean=" << histogram->GetMean()
                          << ", rms=" << histogram->GetRMS()
                          << std::endl;
            }
            continue;
        }

        if (command == "draw" && !name.empty()) {
            client.DrawHistogram(name);
            continue;
        }

        if (command == "clear" && !name.empty()) {
            std::cout << (client.ClearHistogram(name) ? "OK" : "Clear failed")
                      << std::endl;
            continue;
        }

        std::cout << "Unknown command. Type 'help'." << std::endl;
    }

    client.Disconnect();
    return 0;
}
