// Author: Irakli Keshelashvili, 2026
//
// SupFDetMon - Super-FRS Detector Monitoring Software
//
// Copyright (C) 2026 Irakli Keshelashvili
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// See the LICENSE file in the project root for the full license text.

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
    bool ClearAll();
    bool ShutdownServer();
    bool DrawHistogram(const std::string& name);

private:
    bool SendCommand(const std::string& command);
    std::string ReceiveText();

    std::string fHost;
    int fPort;
    std::unique_ptr<TSocket> fSocket;
};

#endif
