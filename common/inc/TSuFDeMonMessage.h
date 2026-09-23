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

#ifndef TSUFDEMONMESSAGE_H
#define TSUFDEMONMESSAGE_H

#include <string>

class TSuFDeMonMessage
{
public:
    TSuFDeMonMessage() = default;
    explicit TSuFDeMonMessage(std::string command);

    const std::string& GetCommand() const noexcept;
    void SetCommand(std::string command);

private:
    std::string fCommand;
};

#endif
