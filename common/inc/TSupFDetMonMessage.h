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

#ifndef TSUPFDETMONMESSAGE_H
#define TSUPFDETMONMESSAGE_H

#include <string>

class TSupFDetMonMessage
{
public:
    TSupFDetMonMessage() = default;
    explicit TSupFDetMonMessage(std::string command);

    const std::string& GetCommand() const noexcept;
    void SetCommand(std::string command);

private:
    std::string fCommand;
};

#endif
