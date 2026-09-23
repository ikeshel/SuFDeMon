#ifndef SUFDEMON_SERVER_CONFIG_H
#define SUFDEMON_SERVER_CONFIG_H

#include <string>

namespace SuFDeMon {

enum class DetectorType { MUSIC, PLSCI, SCIFI };

const char* DetectorTypeName(DetectorType type);
DetectorType ParseDetectorType(const std::string& name);

struct ServerConfig {
    DetectorType type = DetectorType::MUSIC;
    std::string instance = "MUSIC1";
    // Advertised client hostname; the listening socket uses all local interfaces.
    std::string hostname = "localhost";
    int port = 10001;
};

// A config file contains type, instance, hostname and port as key=value lines.
ServerConfig ReadServerConfig(const std::string& path);
void ValidateServerConfig(const ServerConfig& config);
int ParseServerPort(const std::string& text);

} // namespace SuFDeMon

#endif
