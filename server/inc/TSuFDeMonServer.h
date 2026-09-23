// Author: Irakli Keshelashvili, 2026
//
// SuFDeMon - Super-FRS Detector Monitoring Software
//
// Copyright (C) 2026 Irakli Keshelashvili
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// See the LICENSE file in the project root for the full license text.

#ifndef TSUFDEMONSERVER_H
#define TSUFDEMONSERVER_H

#include <array>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

class TH1D;
class TRandom3;
class TServerSocket;
class TSocket;

class TSuFDeMonServer
{
public:
    explicit TSuFDeMonServer(int port);
    ~TSuFDeMonServer();

    TSuFDeMonServer(const TSuFDeMonServer&) = delete;
    TSuFDeMonServer& operator=(const TSuFDeMonServer&) = delete;

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
