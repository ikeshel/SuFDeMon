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

#include "TSuFDeMonGui.h"

#include <TGButton.h>
#include <TGFrame.h>
#include <TGLabel.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr Int_t kConfiguredServerButtonBase = 1000;

struct ServerConfig
{
    std::string type;
    std::string instance;
    std::string hostname;
    int port = 0;
};

std::string Trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int DetectorOrder(const std::string& type)
{
    if (type == "MUSIC") return 0;
    if (type == "PLSCI") return 1;
    if (type == "SCIFI") return 2;
    return 3;
}

std::pair<std::string, int> NaturalInstanceKey(const std::string& value)
{
    std::size_t split = value.size();
    while (split > 0 && value[split - 1] >= '0' && value[split - 1] <= '9')
        --split;

    int number = 0;
    if (split < value.size()) {
        try {
            number = std::stoi(value.substr(split));
        } catch (...) {
            number = 0;
        }
    }
    return {value.substr(0, split), number};
}

std::filesystem::path FindConfigDirectory()
{
    if (const char* configured = std::getenv("CONFIG_DIR")) {
        const std::filesystem::path path(configured);
        if (std::filesystem::is_directory(path)) return path;
    }

    for (const std::filesystem::path& path :
         {std::filesystem::path("config/servers"),
          std::filesystem::path("../config/servers"),
          std::filesystem::path("../../config/servers")}) {
        if (std::filesystem::is_directory(path)) return path;
    }

    return {};
}

std::vector<ServerConfig> ReadServerConfigs()
{
    std::vector<ServerConfig> configs;
    const auto configDirectory = FindConfigDirectory();
    if (configDirectory.empty()) return configs;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(configDirectory)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".conf")
                continue;

            std::ifstream input(entry.path());
            if (!input) continue;

            std::map<std::string, std::string> values;
            std::string line;
            while (std::getline(input, line)) {
                line = Trim(line);
                if (line.empty() || line.front() == '#') continue;

                const auto separator = line.find('=');
                if (separator == std::string::npos) continue;

                values[Trim(line.substr(0, separator))] =
                    Trim(line.substr(separator + 1));
            }

            ServerConfig config;
            config.type = values["type"];
            config.instance = values["instance"];
            config.hostname = values["hostname"];

            try {
                config.port = std::stoi(values["port"]);
            } catch (...) {
                config.port = 0;
            }

            if ((config.type == "MUSIC" || config.type == "PLSCI" ||
                 config.type == "SCIFI") &&
                !config.instance.empty() && !config.hostname.empty() &&
                config.port > 0 && config.port <= 65535) {
                configs.push_back(std::move(config));
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        return {};
    }

    std::sort(configs.begin(), configs.end(),
              [](const ServerConfig& left, const ServerConfig& right) {
                  const int leftOrder = DetectorOrder(left.type);
                  const int rightOrder = DetectorOrder(right.type);
                  if (leftOrder != rightOrder) return leftOrder < rightOrder;

                  const auto leftKey = NaturalInstanceKey(left.instance);
                  const auto rightKey = NaturalInstanceKey(right.instance);
                  if (leftKey.first != rightKey.first)
                      return leftKey.first < rightKey.first;
                  return leftKey.second < rightKey.second;
              });

    return configs;
}
}

void TSuFDeMonGui::BuildServerConnectionsTab(TGCompositeFrame* tab)
{
    auto* overview =
        new TGGroupFrame(tab, "Configured Server Connections", kVerticalFrame);
    auto* overviewRow = new TGHorizontalFrame(overview);

    fRefreshServerConnectionsButton =
        new TGTextButton(overviewRow, "Refresh Status");
    fServerConnectionsSummaryLabel =
        new TGLabel(overviewRow, "Configured endpoints; click a server button to connect.");

    overviewRow->AddFrame(
        fRefreshServerConnectionsButton,
        new TGLayoutHints(kLHintsCenterY, 4, 12, 8, 8));
    overviewRow->AddFrame(
        fServerConnectionsSummaryLabel,
        new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 4, 4, 8, 8));
    overview->AddFrame(
        overviewRow,
        new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));
    tab->AddFrame(
        overview,
        new TGLayoutHints(kLHintsExpandX, 8, 8, 4, 4));

    fRefreshServerConnectionsButton->Connect(
        "Clicked()", "TSuFDeMonGui", this, "RefreshServerConnections()");

    auto* columns = new TGHorizontalFrame(tab);
    auto* musicGroup = new TGGroupFrame(columns, "MUSIC", kVerticalFrame);
    auto* plsciGroup = new TGGroupFrame(columns, "PLSCI", kVerticalFrame);
    auto* scifiGroup = new TGGroupFrame(columns, "SCIFI", kVerticalFrame);

    columns->AddFrame(
        musicGroup,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 3, 3, 3, 3));
    columns->AddFrame(
        plsciGroup,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 3, 3, 3, 3));
    columns->AddFrame(
        scifiGroup,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 3, 3, 3, 3));

    const auto configs = ReadServerConfigs();
    for (std::size_t index = 0; index < configs.size(); ++index) {
        const auto& config = configs[index];
        TGGroupFrame* group = nullptr;
        if (config.type == "MUSIC") group = musicGroup;
        if (config.type == "PLSCI") group = plsciGroup;
        if (config.type == "SCIFI") group = scifiGroup;
        if (!group) continue;

        fServerTypes.push_back(config.type);
        fServerInstances.push_back(config.instance);
        fServerHosts.push_back(config.hostname);
        fServerPorts.push_back(config.port);
        fServerStates.push_back(0);

        const std::size_t storedIndex = fServerButtons.size();
        const Int_t buttonId =
            kConfiguredServerButtonBase + static_cast<Int_t>(storedIndex);
        const std::string label =
            config.instance + "  " + config.hostname + ":" +
            std::to_string(config.port) + "  [Disconnected - Connect]";

        auto* button = new TGTextButton(group, label.c_str(), buttonId);
        button->Associate(this);
        fServerButtons.push_back(button);
        group->AddFrame(
            button,
            new TGLayoutHints(kLHintsExpandX, 4, 4, 3, 3));
    }

    if (fServerButtons.empty()) {
        musicGroup->AddFrame(
            new TGLabel(musicGroup, "No server configs found."),
            new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 8, 8, 12, 12));
    }

    tab->AddFrame(
        columns,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 8, 8, 4, 8));

    UpdateServerConnectionsSummary();
}

Bool_t TSuFDeMonGui::ProcessMessage(
    Longptr_t msg, Longptr_t parm1, Longptr_t parm2)
{
    if (GET_MSG(msg) == kC_COMMAND &&
        GET_SUBMSG(msg) == kCM_BUTTON &&
        parm1 >= kConfiguredServerButtonBase) {
        const auto index =
            static_cast<std::size_t>(parm1 - kConfiguredServerButtonBase);
        if (index < fServerButtons.size()) {
            ConnectConfiguredServer(index);
            return kTRUE;
        }
    }

    return TGMainFrame::ProcessMessage(msg, parm1, parm2);
}
