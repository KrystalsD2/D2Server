#include "RealmConfig100.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
using namespace d2server100;

static void writeFile(const char* path,const char* text){ std::ofstream f(path); f<<text; }

int main(){
    const char* path="realm_test.ini";
    writeFile(path,
        "[realm]\nname=wrong\nbnetip=1.1.1.1\nmcpip=1.1.1.1\ncharserv=2.2.2.2\naccept=10.0.0.1/24\n"
        "[realm]\nname=right\nbnetip=192.168.1.4\nmcpip=192.168.1.5\ncharserv=192.168.1.6,192.168.1.7,192.168.1.8,ignored\n"
        "accept=192.168.1.0/24\nmulticlient=7\nannounce=hello\n");
    u32 local=0; assert(ParseIPv4HostOrder100("192.168.1.44",local));
    RealmConfig100 c; std::string err; assert(c.load(path,local,&err));
    const auto* r=c.selected(); assert(r && r->name=="right" && r->charServerCount==3 && r->multiclient==7);

    // Original parser does not normalize 10.0.0.1/24 to 10.0.0.0/24.
    u32 ten=0; ParseIPv4HostOrder100("10.0.0.7",ten); AcceptRule100 a{};
    ParseIPv4HostOrder100("10.0.0.1",a.network); a.prefix=24; assert(!a.matches(ten));

    // Invalid MULTICLIENT (> 0x400 or negative) is ignored rather than clamped.
    writeFile(path,"[realm]\nname=x\nbnetip=1.2.3.4\ncharserv=1.2.3.5\naccept=192.168.1.0/24\nmulticlient=5000\n");
    assert(c.load(path,local,&err)); assert(c.selected()->multiclient==0);
    writeFile(path,"[realm]\nname=x\nbnetip=1.2.3.4\ncharserv=1.2.3.5\naccept=192.168.1.0/24\nmulticlient=-1\n");
    assert(c.load(path,local,&err)); assert(c.selected()->multiclient==0);

    // A realm without ACCEPT is never selected as a fallback.
    writeFile(path,"[realm]\nname=x\nbnetip=1.2.3.4\ncharserv=1.2.3.5\n");
    assert(!c.load(path,local,&err));

    // Once selected, NAME / BNETIP / CHARSERV are mandatory in the original.
    writeFile(path,"[realm]\nbnetip=1.2.3.4\ncharserv=1.2.3.5\naccept=192.168.1.0/24\n");
    assert(!c.load(path,local,&err));
    writeFile(path,"[realm]\nname=x\ncharserv=1.2.3.5\naccept=192.168.1.0/24\n");
    assert(!c.load(path,local,&err));
    writeFile(path,"[realm]\nname=x\nbnetip=1.2.3.4\naccept=192.168.1.0/24\n");
    assert(!c.load(path,local,&err));

    std::remove(path);
    std::cout << "realms.ini binary-parity tests passed\n";
}
