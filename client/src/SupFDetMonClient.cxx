#include "TSupFDetMonClient.h"
#include "SupFDetMonProtocol.h"

#include <TH1D.h>
#include <TInterpreter.h>
#include <TRint.h>

#include <iostream>
#include <memory>
#include <string>

namespace {
std::unique_ptr<TSupFDetMonClient> gClient;
std::unique_ptr<TH1D> gHistogram;
}

bool SupFDetMonPing()
{
    return gClient && gClient->Ping();
}

void SupFDetMonList()
{
    if (gClient) std::cout << gClient->ListHistograms();
}

TH1D* SupFDetMonGet(const char* name)
{
    if (!gClient) return nullptr;
    gHistogram = gClient->GetHistogram(name);
    return gHistogram.get();
}

TH1D* SupFDetMonDraw(const char* name)
{
    TH1D* histogram = SupFDetMonGet(name);
    if (histogram) histogram->Draw();
    return histogram;
}

bool SupFDetMonClear(const char* name)
{
    return gClient && gClient->ClearHistogram(name);
}

bool SupFDetMonClearAll()
{
    return gClient && gClient->ClearAll();
}

int main(int argc, char** argv)
{
    std::string host = "localhost";
    int port = SupFDetMon::Protocol::kDefaultPort;

    // Consume our host/port arguments before TRint sees its own ROOT options.
    if (argc >= 2 && argv[1][0] != '-') host = argv[1];
    if (argc >= 3 && argv[2][0] != '-') {
        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port: " << argv[2] << std::endl;
            return 1;
        }
    }

    int rootArgc = 1;
    char* rootArgv[] = {argv[0], nullptr};
    TRint application("SupFDetMonClient", &rootArgc, rootArgv);

    gClient = std::make_unique<TSupFDetMonClient>(host, port);
    if (!gClient->Connect()) return 1;

    gInterpreter->Declare(R"(
        class TH1D;
        bool SupFDetMonPing();
        void SupFDetMonList();
        TH1D* SupFDetMonGet(const char*);
        TH1D* SupFDetMonDraw(const char*);
        bool SupFDetMonClear(const char*);
        bool SupFDetMonClearAll();
    )");

    std::cout << "\nSupFDetMon connected to " << host << ':' << port << "\n"
              << "ROOT prompt is active. Normal ROOT/C++ commands work here.\n"
              << "SupFDetMon helpers:\n"
              << "  SupFDetMonPing()\n"
              << "  SupFDetMonList()\n"
              << "  TH1D* h = SupFDetMonGet(\"TH1D_MUSIC_ADC_FC1_ADC0\")\n"
              << "  SupFDetMonDraw(\"TH1D_MUSIC_ADC_FC1_ADC0\")\n"
              << "  SupFDetMonClear(\"TH1D_MUSIC_ADC_FC1_ADC0\")\n"
              << "  SupFDetMonClearAll()\n\n";

    application.Run();

    gClient->Disconnect();
    gHistogram.reset();
    gClient.reset();
    return 0;
}
