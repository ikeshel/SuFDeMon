#include "SuFDeMonServerConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>

namespace SuFDeMon {
namespace {
std::string Trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? ""
        : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
}

const char* DetectorTypeName(DetectorType type)
{
    switch (type) {
    case DetectorType::MUSIC: return "MUSIC";
    case DetectorType::PLSCI: return "PLSCI";
    case DetectorType::SCIFI: return "SCIFI";
    }
    throw std::invalid_argument("Invalid detector type");
}

DetectorType ParseDetectorType(const std::string& name)
{
    if (name == "MUSIC") return DetectorType::MUSIC;
    if (name == "PLSCI") return DetectorType::PLSCI;
    if (name == "SCIFI") return DetectorType::SCIFI;
    throw std::invalid_argument("Detector type must be MUSIC, PLSCI or SCIFI");
}

int ParseServerPort(const std::string& text)
{
    if (text.empty() || text.size() > 5 ||
        !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); }))
        throw std::invalid_argument("Port must be an integer in 1..65535");
    const int port = std::stoi(text);
    if (port < 1 || port > 65535)
        throw std::invalid_argument("Port must be in 1..65535");
    return port;
}

void ValidateServerConfig(const ServerConfig& config)
{
    DetectorTypeName(config.type);
    ParseServerPort(std::to_string(config.port));
    if (config.instance.empty() || !std::all_of(config.instance.begin(), config.instance.end(),
        [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-'; }))
        throw std::invalid_argument("Instance must contain only letters, digits, '_' or '-'");
    if (config.hostname.empty() || std::any_of(config.hostname.begin(), config.hostname.end(),
        [](unsigned char c) { return std::isspace(c) || std::iscntrl(c); }))
        throw std::invalid_argument("Hostname must be nonempty and contain no whitespace");
}

ServerConfig ReadServerConfig(const std::string& path)
{
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open server config: " + path);
    ServerConfig config;
    std::set<std::string> keys;
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos)
            throw std::invalid_argument("Expected key=value in " + path);
        const auto key = Trim(line.substr(0, separator));
        const auto value = Trim(line.substr(separator + 1));
        if (!keys.insert(key).second)
            throw std::invalid_argument("Duplicate config key: " + key);
        if (key == "type") config.type = ParseDetectorType(value);
        else if (key == "instance") config.instance = value;
        else if (key == "hostname") config.hostname = value;
        else if (key == "port") config.port = ParseServerPort(value);
        else throw std::invalid_argument("Unknown config key: " + key);
    }
    if (input.bad()) throw std::runtime_error("Cannot read server config: " + path);
    for (const auto* key : {"type", "instance", "hostname", "port"})
        if (!keys.count(key)) throw std::invalid_argument(std::string("Missing config key: ") + key);
    ValidateServerConfig(config);
    return config;
}
} // namespace SuFDeMon
