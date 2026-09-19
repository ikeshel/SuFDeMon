#include "TSupFDetMonClient.h"

#include "SupFDetMonProtocol.h"

#include <TCanvas.h>
#include <TClass.h>
#include <TH1D.h>
#include <TMessage.h>
#include <TSocket.h>
#include <TSystem.h>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

TSupFDetMonClient::TSupFDetMonClient(std::string host, int port)
    : fHost(std::move(host)), fPort(port) {}

TSupFDetMonClient::~TSupFDetMonClient() { Disconnect(); }

bool TSupFDetMonClient::Connect()
{
    Disconnect();
    fSocket = std::make_unique<TSocket>(fHost.c_str(), fPort);
    if (!fSocket->IsValid()) {
        std::cerr << "Failed to connect to " << fHost << ':' << fPort << std::endl;
        fSocket.reset();
        return false;
    }
    return true;
}

void TSupFDetMonClient::Disconnect()
{
    if (!fSocket) return;
    if (fSocket->IsValid()) {
        fSocket->Send(std::string(SupFDetMon::Protocol::kQuit).c_str());
        char reply[256] = {};
        fSocket->Recv(reply, sizeof(reply));
    }
    fSocket->Close();
    fSocket.reset();
}

bool TSupFDetMonClient::IsConnected() const { return fSocket && fSocket->IsValid(); }

bool TSupFDetMonClient::SendCommand(const std::string& command)
{
    if (!IsConnected()) {
        std::cerr << "Client is not connected." << std::endl;
        return false;
    }
    return fSocket->Send(command.c_str()) > 0;
}

std::string TSupFDetMonClient::ReceiveText()
{
    if (!IsConnected()) return {};
    char buffer[65536] = {};
    const int received = fSocket->Recv(buffer, sizeof(buffer));
    return received > 0 ? std::string(buffer) : std::string{};
}

bool TSupFDetMonClient::Ping()
{
    return SendCommand(std::string(SupFDetMon::Protocol::kPing)) && ReceiveText() == "PONG";
}

std::string TSupFDetMonClient::ListHistograms()
{
    if (!SendCommand(std::string(SupFDetMon::Protocol::kList))) return {};
    return ReceiveText();
}

std::unique_ptr<TH1D> TSupFDetMonClient::GetHistogram(const std::string& name)
{
    if (!SendCommand(std::string(SupFDetMon::Protocol::kGet) + " " + name)) return nullptr;

    TMessage* rawMessage = nullptr;
    const int received = fSocket->Recv(rawMessage);
    std::unique_ptr<TMessage> message(rawMessage);

    if (received <= 0 || !message) {
        std::cerr << "Failed to receive histogram." << std::endl;
        return nullptr;
    }
    if (message->What() == kMESS_STRING) {
        char error[4096] = {};
        message->ReadString(error, sizeof(error));
        std::cerr << error << std::endl;
        return nullptr;
    }
    if (message->What() != kMESS_OBJECT) {
        std::cerr << "Unexpected message type received from server." << std::endl;
        return nullptr;
    }

    TObject* object = message->ReadObject(message->GetClass());
    auto* histogram = dynamic_cast<TH1D*>(object);
    if (!histogram) {
        delete object;
        std::cerr << "Received object is not a TH1D." << std::endl;
        return nullptr;
    }
    histogram->SetDirectory(nullptr);
    return std::unique_ptr<TH1D>(histogram);
}

bool TSupFDetMonClient::ClearHistogram(const std::string& name)
{
    return SendCommand(std::string(SupFDetMon::Protocol::kClear) + " " + name)
        && ReceiveText() == "OK";
}

bool TSupFDetMonClient::ClearAll()
{
    return SendCommand(std::string(SupFDetMon::Protocol::kClearAll))
        && ReceiveText() == "OK";
}

bool TSupFDetMonClient::ShutdownServer()
{
    if (!SendCommand(std::string(SupFDetMon::Protocol::kShutdown)))
        return false;

    const bool acknowledged = ReceiveText() == "BYE";
    if (fSocket) {
        fSocket->Close();
        fSocket.reset();
    }
    return acknowledged;
}

bool TSupFDetMonClient::DrawHistogram(const std::string& name)
{
    auto histogram = GetHistogram(name);
    if (!histogram) return false;

    auto canvas = std::make_unique<TCanvas>("SupFDetMonCanvas", name.c_str(), 1000, 700);
    histogram->Draw();
    canvas->Modified();
    canvas->Update();
    gSystem->ProcessEvents();

    std::cout << "Histogram displayed. Press Enter to close it..." << std::endl;
    std::cin.get();

    canvas->Close();
    gSystem->ProcessEvents();
    return true;
}
