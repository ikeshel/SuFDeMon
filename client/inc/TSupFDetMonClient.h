#ifndef TSUPFDETMONCLIENT_H
#define TSUPFDETMONCLIENT_H

#include <memory>
#include <string>

class TH1D;
class TSocket;

class TSupFDetMonClient
{
public:
    TSupFDetMonClient(std::string host, int port);
    ~TSupFDetMonClient();

    TSupFDetMonClient(const TSupFDetMonClient&) = delete;
    TSupFDetMonClient& operator=(const TSupFDetMonClient&) = delete;

    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    bool Ping();
    std::string ListHistograms();
    std::unique_ptr<TH1D> GetHistogram(const std::string& name);
    bool ClearHistogram(const std::string& name);
    bool ClearAll();\n    bool ShutdownServer();
    bool DrawHistogram(const std::string& name);

private:
    bool SendCommand(const std::string& command);
    std::string ReceiveText();

    std::string fHost;
    int fPort;
    std::unique_ptr<TSocket> fSocket;
};

#endif
