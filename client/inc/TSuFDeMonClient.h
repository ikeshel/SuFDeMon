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

#ifndef TSUFDEMONCLIENT_H
#define TSUFDEMONCLIENT_H

#include <memory>
#include <string>

class TH1;
class TSocket;

class TSuFDeMonClient
{
public:
    TSuFDeMonClient(std::string host, int port);
    ~TSuFDeMonClient();

    TSuFDeMonClient(const TSuFDeMonClient&) = delete;
    TSuFDeMonClient& operator=(const TSuFDeMonClient&) = delete;

    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    std::string Info();
    bool Ping();
    std::string ListHistograms();
    std::unique_ptr<TH1> GetHistogram(const std::string& name);
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
