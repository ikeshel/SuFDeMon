#include <TSocket.h>
#include <TMessage.h>
#include <TH1.h>
#include <TH2D.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <algorithm>

namespace {
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
std::string Request(TSocket& socket, const std::string& command)
{
    Require(socket.Send(command.c_str()) > 0, "send failed");
    char reply[65536] = {};
    Require(socket.Recv(reply, sizeof(reply)) > 0, "receive failed");
    return reply;
}
std::unique_ptr<TH1> Get(TSocket& socket, const std::string& name)
{
    Require(socket.Send(("GET " + name).c_str()) > 0, "GET send failed");
    TMessage* raw = nullptr;
    Require(socket.Recv(raw) > 0, "GET receive failed");
    std::unique_ptr<TMessage> message(raw);
    Require(message && message->What() == kMESS_OBJECT, "Expected histogram object");
    auto* object = message->ReadObject(message->GetClass());
    auto* histogram = dynamic_cast<TH1*>(object);
    if (!histogram) { delete object; throw std::runtime_error("Expected TH1"); }
    histogram->SetDirectory(nullptr);
    return std::unique_ptr<TH1>(histogram);
}
}

int main(int argc, char** argv)
{
    try {
        Require(argc == 4, "Expected port, type, instance");
        const int port = std::stoi(argv[1]);
        const std::string type = argv[2], instance = argv[3];
        std::unique_ptr<TSocket> socket;
        for (int retry = 0; retry < 50; ++retry) {
            socket = std::make_unique<TSocket>("localhost", port);
            if (socket->IsValid()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        Require(socket->IsValid(), "Server did not start");
        Require(Request(*socket, "PING") == "PONG", "PING failed");
        const auto info = Request(*socket, "INFO");
        Require(info == "type=" + type + "\ninstance=" + instance
            + "\nhostname=localhost\nport=" + std::to_string(port) + "\n", "Incorrect identity");
        const auto list = Request(*socket, "LIST");
        const int channels = type == "PLSCI" ? ((instance == "PLSCI4" || instance == "PLSCI6") ? 8 : 6) : 32;
        if (type == "MUSIC") {
            Require(std::count(list.begin(), list.end(), '\n') == 192, "MUSIC histogram count changed");
            Require(list.find("h" + instance + "_FC3_ADC31\n") != std::string::npos,
                    "Missing MUSIC channel");
            const std::string requested = "h" + instance + "_FC2_ADC26";
            Require(list.find(requested + "\n") != std::string::npos, "Missing ROOT-style name");
            Require(std::string(Get(*socket, requested)->GetName()) == requested,
                    "Serialized histogram name differs from LIST");
            Require(Request(*socket, "CLEAR " + requested) == "OK", "ROOT-style CLEAR failed");
            const std::string name = "h" + instance + "_FC1_ADC0";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Require(Get(*socket, name)->GetEntries() > 0, "MUSIC not filling");
            Require(Request(*socket, "CLEAR " + name) == "OK", "CLEAR failed");
            Require(Get(*socket, name)->GetNbinsX() == 4096, "Histogram binning changed");
        } else if (type == "SCIFI") {
            Require(list == "h" + instance + "_ToT\nh" + instance + "_TDC\n", "SCIFI must publish exactly two maps");
            for (const std::string quantity : {"ToT", "TDC"}) {
                const auto name = "h" + instance + "_" + quantity;
                auto histogram = Get(*socket, name);
                Require(dynamic_cast<TH2D*>(histogram.get()) != nullptr, "Expected SCIFI TH2D");
                Require(histogram->GetNbinsX() == 2048 && histogram->GetNbinsY() == 256,
                    "Wrong SCIFI map dimensions");
                Require(histogram->GetXaxis()->GetXmin() == -0.5 && histogram->GetXaxis()->GetXmax() == 2047.5,
                    "Wrong SCIFI channel range");
                Require(histogram->GetYaxis()->GetXmin() == 0 && histogram->GetYaxis()->GetXmax() == 4096,
                    "Wrong SCIFI timing range");
                for (int retry = 0; histogram->GetEntries() == 0 && retry < 20; ++retry) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    histogram = Get(*socket, name);
                }
                for (int channel : {1, 2048}) {
                    double entries = 0;
                    for (int bin = 1; bin <= histogram->GetNbinsY(); ++bin)
                        entries += histogram->GetBinContent(channel, bin);
                    Require(entries > 0, "SCIFI boundary channel not filling");
                }
                Require(Request(*socket, "CLEAR " + name) == "OK", "SCIFI CLEAR failed");
            }
            Require(Request(*socket, "GET h" + instance + "_ADC0") == "ERROR histogram not found",
                "Obsolete SCIFI per-channel histogram exposed");
        } else {
            Require(std::count(list.begin(), list.end(), '\n') == channels * 2, "ADC/TDC count changed");
        }
        for (const std::string quantity : {"ADC", "TDC"}) {
            if (type == "SCIFI") break;
            if (type == "PLSCI") {
                const auto invalid = "h" + instance + "_" + quantity + std::to_string(channels);
                Require(Request(*socket, "GET " + invalid) == "ERROR histogram not found", "Nonexistent PMT exposed");
                Require(Request(*socket, "CLEAR " + invalid) == "ERROR histogram not found", "Nonexistent PMT cleared");
            }
            const int cages = type == "MUSIC" ? 3 : 0;
            for (int fc = cages ? 1 : 0; fc <= cages; ++fc) {
                for (int channel = 0; channel < channels; ++channel) {
                    const auto name = "h" + instance + "_" +
                        (fc ? "FC" + std::to_string(fc) + "_" : "") + quantity + std::to_string(channel);
                    Require(list.find(name + "\n") != std::string::npos, "Missing ADC/TDC channel");
                    auto histogram = Get(*socket, name);
                    Require(std::string(histogram->GetName()) == name, "Wrong object name");
                    Require(histogram->GetNbinsX() == 4096 && histogram->GetXaxis()->GetXmin() == 0.0
                        && histogram->GetXaxis()->GetXmax() == 4096.0, "Wrong raw-count binning");
                    for (int retry = 0; histogram->GetEntries() == 0 && retry < 20; ++retry) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        histogram = Get(*socket, name);
                    }
                    Require(histogram->GetEntries() > 0, "Channel not filling");
                    Require(Request(*socket, "CLEAR " + name) == "OK", "Channel CLEAR failed");
                }
            }
        }
        Require(Request(*socket, "GET missing") == "ERROR histogram not found", "Missing GET failed");
        Require(Request(*socket, "CLEAR missing") == "ERROR histogram not found", "Missing CLEAR failed");
        Require(Request(*socket, "CLEAR ALL") == "OK", "CLEAR ALL failed");
        Require(Request(*socket, "UNKNOWN") == "ERROR unknown command", "Unknown command failed");
        Require(Request(*socket, "QUIT") == "BYE", "QUIT failed");
        socket->Close();
        socket = std::make_unique<TSocket>("localhost", port);
        Require(socket->IsValid(), "Reconnect failed");
        Require(Request(*socket, "SHUTDOWN") == "BYE", "SHUTDOWN failed");
        socket->Close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
