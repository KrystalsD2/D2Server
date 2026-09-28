#include "RealmConfig100.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

namespace d2server100 {
namespace {
std::string trim(std::string s) {
    auto ws=[](unsigned char c){ return std::isspace(c)!=0; };
    while (!s.empty() && ws((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && ws((unsigned char)s.back())) s.pop_back();
    return s;
}
std::string lower(std::string s) {
    for (char& c:s) c=(char)std::tolower((unsigned char)c);
    return s;
}
std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> out; std::stringstream ss(s); std::string item;
    while (std::getline(ss,item,',')) { item=trim(item); if (!item.empty()) out.push_back(item); }
    return out;
}
bool parseAccept(const std::string& text, AcceptRule100& out) {
    std::string ip=text; std::string pfx;
    const auto slash=ip.find('/');
    if (slash!=std::string::npos) { pfx=ip.substr(slash+1); ip=ip.substr(0,slash); }
    if (!ParseIPv4HostOrder100(trim(ip),out.network)) return false;
    out.prefix=32;
    if (!pfx.empty()) {
        char* end=nullptr; long v=std::strtol(pfx.c_str(),&end,10);
        if (!end || *end || v<0 || v>32) return false;
        out.prefix=(u8)v;
    }
    return true;
}
}

bool AcceptRule100::matches(u32 ip) const noexcept {
    if (prefix==0) return true;
    const u32 mask = prefix==32 ? 0xFFFFFFFFu : (0xFFFFFFFFu << (32-prefix));
    // Deliberately compare against network exactly as written. This mirrors the
    // original Inifile.cpp behavior instead of normalizing network &= mask.
    return (ip & mask) == network;
}

bool ParseIPv4HostOrder100(const std::string& text, u32& out) noexcept {
    unsigned a=0,b=0,c=0,d=0; char tail=0;
    if (std::sscanf(text.c_str(),"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)!=4) return false;
    if (a>255||b>255||c>255||d>255) return false;
    out=(a<<24)|(b<<16)|(c<<8)|d;
    return true;
}

u32 DetectLocalIPv4HostOrder100() noexcept {
#if defined(_WIN32)
    WSADATA w{};
    if (WSAStartup(MAKEWORD(2,2),&w)!=0) return 0;
    char host[256]{};
    u32 result=0;
    if (gethostname(host,sizeof(host)-1)==0) {
        addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM;
        addrinfo* res=nullptr;
        if (getaddrinfo(host,nullptr,&hints,&res)==0) {
            for (auto* p=res;p;p=p->ai_next) {
                auto* sin=reinterpret_cast<sockaddr_in*>(p->ai_addr);
                const u32 n=ntohl(sin->sin_addr.s_addr);
                if ((n>>24)!=127) { result=n; break; }
                if (!result) result=n;
            }
            freeaddrinfo(res);
        }
    }
    WSACleanup();
    return result;
#else
    return 0;
#endif
}

bool RealmConfig100::load(const char* path,u32 localIp,std::string* error) {
    realms.clear(); selectedIndex=-1;
    std::ifstream in(path ? path : "realms.ini");
    if (!in) { if(error)*error="could not open realms.ini"; return false; }
    RealmConfigEntry100* cur=nullptr;
    std::string line;
    while (std::getline(in,line)) {
        const auto sem=line.find(';'); if (sem!=std::string::npos) line.erase(sem);
        const auto hash=line.find('#'); if (hash!=std::string::npos) line.erase(hash);
        line=trim(line); if (line.empty()) continue;
        if (line.front()=='[' && line.back()==']') {
            if (lower(trim(line.substr(1,line.size()-2)))=="realm") {
                realms.emplace_back(); cur=&realms.back();
            }
            continue;
        }
        if (!cur) continue;
        const auto eq=line.find('='); if (eq==std::string::npos) continue;
        auto key=lower(trim(line.substr(0,eq))); auto val=trim(line.substr(eq+1));
        if (key=="name") cur->name=val;
        else if (key=="desc") cur->desc=val;
        else if (key=="adminpass") cur->adminPass=val;
        else if (key=="publicpass") cur->publicPass=val;
        else if (key=="bnetip") cur->bnetIp=val;
        else if (key=="mcpip") cur->mcpIp=val;
        else if (key=="charserv") {
            auto v=splitComma(val); cur->charServerCount=(u32)std::min<std::size_t>(3,v.size());
            for (u32 i=0;i<cur->charServerCount;++i) cur->charServers[i]=v[i];
        } else if (key=="accept") {
            for (const auto& t:splitComma(val)) {
                if (cur->accept.size()>=256) break;
                AcceptRule100 r{}; if (parseAccept(t,r)) cur->accept.push_back(r);
            }
        } else if (key=="multiclient") {
            // Original parser accepts only signed decimal values 0..0x400.
            // Invalid values are logged/ignored; the getter later maps zero to 1.
            char* end=nullptr; long v=std::strtol(val.c_str(),&end,10);
            if (end && !*end && v>=0 && v<=0x400) cur->multiclient=(u32)v;
        } else if (key=="announce") {
            if (cur->announce.size()<20) cur->announce.push_back(val);
        }
    }
    if (realms.empty()) { if(error)*error="no [realm] blocks"; return false; }

    for (std::size_t i=0;i<realms.size();++i) {
        const auto& r=realms[i];
        // The original has no "first realm" fallback: a realm is selected only
        // when its ACCEPT list contains this machine's IPv4 address.
        for (const auto& a:r.accept) if (a.matches(localIp)) { selectedIndex=(int)i; break; }
        if (selectedIndex==(int)i) break;
    }
    if (selectedIndex<0) { if(error)*error="[INIFILE] Error -- Unable to find realm for this machine."; return false; }

    const auto& selectedRealm=realms[(std::size_t)selectedIndex];
    if (selectedRealm.name.empty()) {
        if(error)*error="[INIFILE] Error -- Realm info didn't contain name of realm.";
        return false;
    }
    if (selectedRealm.bnetIp.empty()) {
        if(error)*error="[INIFILE] Error -- Realm info didn't contain battle net ip.";
        return false;
    }
    if (selectedRealm.accept.empty()) {
        if(error)*error="[INIFILE] Error -- Realm info missing accept list.";
        return false;
    }
    if (!selectedRealm.charServerCount) {
        if(error)*error="[INIFILE] Error -- Realm info missing character server list.";
        return false;
    }
    return true;
}

const RealmConfigEntry100* RealmConfig100::selected() const noexcept {
    return selectedIndex>=0 && (std::size_t)selectedIndex<realms.size() ? &realms[(std::size_t)selectedIndex] : nullptr;
}

} // namespace d2server100
