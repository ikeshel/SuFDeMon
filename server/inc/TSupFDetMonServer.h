#ifndef TSUPFDETMONSERVER_H
#define TSUPFDETMONSERVER_H

#include <array>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

class TH1D;
class TRandom3;
class TServerSocket;
class TSocket;

class TSupFDetMonServer
{
public:
    explicit TSupFDetMonServer(int port);
    ~TSupFDetMonServer();

    TSupFDetMonServer(const TSupFDetMonServer&) = delete;
    TSupFDetMonServer& operator=(const TSupFDetMonServer&) = delete;

    int Run();

private:
    static constexpr int kNFieldCages = 3;
    static constexpr int kNAdcChannels = 32;

    using HistogramRow = std::array<std::unique_ptr<TH1D>, kNAdcChannels>;
    using HistogramArray = std::array<HistogramRow, kNFieldCages>;

    void CreateHistograms();
    void FillHistograms();
    void FillLoop();
    bool HandleClient(TSocket& socket);
    bool HandleCommand(TSocket& socket, const std::string& command);

    TH1D* FindHistogram(const std::string& name);
    std::string HistogramList() const;

    int fPort;
    HistogramArray fMusicAdc;
    std::unique_ptr<TRandom3> fRandom;
    std::unique_ptr<TServerSocket> fServerSocket;
    std::atomic<bool> fFillRunning{false};
    std::thread fFillThread;
    std::atomic<bool> fServerRunning{true};
};

#endif
