#pragma once
#include "Compat.h"
#include <array>
#include <string>
#include <vector>

namespace d2server100 {

struct AcceptRule100 {
    u32 network = 0;   // host-order IPv4 as written; original parser does not normalize
    u8 prefix = 32;
    bool matches(u32 ipv4HostOrder) const noexcept;
};

struct RealmConfigEntry100 {
    std::string name;
    std::string desc;
    std::string adminPass;
    std::string publicPass;
    std::string bnetIp;
    std::string mcpIp;
    std::array<std::string,3> charServers{};
    u32 charServerCount = 0;
    std::vector<AcceptRule100> accept;
    u32 multiclient = 0;
    std::vector<std::string> announce;
};

struct RealmConfig100 {
    std::vector<RealmConfigEntry100> realms;
    int selectedIndex = -1;

    bool load(const char* path, u32 localIpv4HostOrder, std::string* error = nullptr);
    const RealmConfigEntry100* selected() const noexcept;
};

bool ParseIPv4HostOrder100(const std::string& text, u32& out) noexcept;
u32 DetectLocalIPv4HostOrder100() noexcept;

} // namespace d2server100
