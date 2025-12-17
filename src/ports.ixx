// ports.ixx
module;
#include <vector>
#include <string>
#include <sstream>
#include <random>

export module ports;
export namespace ports {
    export const std::string_view COMMON_PORTS = "21,22,23,25,53,80,110,135,139,143,443,993,995,1723,1812,1813,3306,3389,5432,5900,8080,8443";

    export const std::vector<std::string_view> PROXIES = {
        "Direct", "127.0.0.1:8080", "45.67.89.12:8080"
    };

    export std::vector<uint16_t> parse_ports(std::string_view ports_str) {
        std::vector<uint16_t> ports;
        std::istringstream iss({ std::string(ports_str) });
        std::string token;
        while (std::getline(iss, token, ',')) {
            size_t dash = token.find('-');
            if (dash != std::string::npos) {
                int start = std::stoi(token.substr(0, dash));
                int end = std::stoi(token.substr(dash + 1));
                for (int p = start; p <= end && p <= 65535; ++p) ports.push_back((uint16_t)p);
            }
            else {
                try {
                    int port = std::stoi(token);
                    if (port <= 65535 && port > 0) ports.push_back((uint16_t)port);
                }
                catch (...) {}
            }
        }
        return ports;
    }
}