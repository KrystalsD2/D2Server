#include "D2Server100.h"
#include "CharServBridge100.h"
#include "Callbacks100.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
using namespace d2server100;

static std::vector<u8> lastMcp,lastChar;
static std::vector<std::vector<u8>> allMcp;
static bool SendMcp(const void* p,std::size_t n){
    lastMcp.assign((const u8*)p,(const u8*)p+n);
    allMcp.push_back(lastMcp);
    return true;
}
static bool SendChar(const void* p,std::size_t n){lastChar.assign((const u8*)p,(const u8*)p+n);return true;}
static u32 gotKey=0, gotMode=0; static u16 gotChunk=0,gotTotal=0; static std::vector<u8> gotSave;
static int D2_STDCALL Fake10007(u32 key,const void* data,u16 chunk,u16 total,u32 mode){
    gotKey=key;gotChunk=chunk;gotTotal=total;gotMode=mode;gotSave.assign((const u8*)data,(const u8*)data+chunk);return 7;
}

int main(){
    bindings().sendToMcp=&SendMcp; bindings().sendToCharServer=&SendChar;

    const auto& cb=eventCallbackTable100();
    for (auto p:cb.slot) assert(p!=nullptr);

    Callback_CloseGame(0x12345678);
    assert(lastMcp.size()==5&&lastMcp[0]==5&&read_u32(lastMcp.data()+1)==0x12345678);

    assert(Callback_UnlockDatabaseCharacter("Hero")==1);
    assert(lastChar.size()==6&&lastChar[0]==7&&std::strcmp((char*)lastChar.data()+1,"Hero")==0);

    assert(Callback_GetDatabaseCharacter("Hero",0xAABBCCDD)==1);
    assert(lastChar.size()==14 && lastChar[0]==3);
    assert(read_u32(lastChar.data()+1)==1);
    assert(read_u32(lastChar.data()+5)==0xAABBCCDD);
    assert(std::strcmp((char*)lastChar.data()+9,"Hero")==0);

    Callback_EnterGame(7,"Hero",2,33,0);
    assert(lastMcp[0]==0x0A && read_u32(lastMcp.data()+1)==7);
    assert(read_u16(lastMcp.data()+5)==2 && read_u16(lastMcp.data()+7)==33);
    assert(std::strcmp((char*)lastMcp.data()+11,"Hero")==0);

    allMcp.clear();
    std::vector<u8> guild(450);
    for (std::size_t i=0;i<guild.size();++i) guild[i]=(u8)i;
    assert(Callback_SaveDatabaseGuild("ABC",guild.data(),(u32)guild.size())==1);
    assert(allMcp.size()==2);
    assert(allMcp[0][0]==0x0C && read_u16(allMcp[0].data()+5)==0 && read_u16(allMcp[0].data()+7)==0x190);
    assert(read_u16(allMcp[0].data()+9)==450);
    assert(allMcp[1][0]==0x0C && read_u16(allMcp[1].data()+5)==0x190 && read_u16(allMcp[1].data()+7)==50);

    Callback09("TAG",3,"hello",0x11223344);
    assert(lastMcp.size()==0x1A && lastMcp[0]==0x0E && lastMcp[1]==3);
    assert(read_u32(lastMcp.data()+6)==0x11223344);

    // Ladder opcode 0x0F state: zero thresholds permits a positive-exp update.
    std::array<u8,0x88> ladder{};
    UpdateLadderStateBlob100(ladder.data(),ladder.size());
    assert(Callback_UpdateCharacterLadder("Hero",2,40,100,0,0)==1);
    assert(lastMcp[0]==0x0F && read_u32(lastMcp.data()+5)==100);
    assert(std::strcmp((char*)lastMcp.data()+15,"Hero")==0);

    Callback_UpdateGameInformation(9,"Hero",2,40);
    assert(lastMcp[0]==0x10 && read_u32(lastMcp.data()+1)==9 && lastMcp[5]==2 && lastMcp[6]==40);

    // Early LeaveGame unlocks the character first, then emits MCP opcode 08.
    Callback_LeaveGame(9,2,40,0x11111111,0x22222222,0,"Hero","PORTRAIT");
    assert(lastChar[0]==0x07);
    assert(lastMcp[0]==0x08 && read_u32(lastMcp.data()+1)==9);
    assert(read_u16(lastMcp.data()+9)==40);
    assert(read_u32(lastMcp.data()+11)==0x11111111);
    assert(read_u32(lastMcp.data()+15)==0x22222222);
    assert(std::strcmp((char*)lastMcp.data()+21,"Hero")==0);

    // Save checksum is rotate-left-one plus each byte.
    const u8 checkBytes[]={1,2,3};
    assert(SaveChecksum100(checkBytes,3)==11); // (((0 rol1)+1 rol1)+2 rol1)+3

    // Reconstruction adds safe framing guards around the two consecutive MCP
    // strings. Retail assumes trusted MCP input; malformed packets must not
    // make this source walk one byte past its receive buffer.
    std::array<u8,13> badCreate{}; badCreate[0]=0x06; badCreate[12]='\0';
    assert(DispatchFromMcp(badCreate.data(),(u32)badCreate.size())==0);
    std::array<u8,18> badJoin{}; badJoin[0]=0x07; badJoin[17]='\0';
    assert(DispatchFromMcp(badJoin.data(),(u32)badJoin.size())==0);

    bindings().d2game.ord10007=&Fake10007;
    std::vector<u8> p(0x14); write_u32(p.data()+5,0xAABBCCDD); write_u16(p.data()+9,3); write_u16(p.data()+0xB,3); write_u32(p.data()+0xD,9); p[0x11]=1;p[0x12]=2;p[0x13]=3;
    assert(DeliverDatabaseCharacterReply100(p.data(),(u32)p.size())==7);
    assert(gotKey==0xAABBCCDD&&gotChunk==3&&gotTotal==3&&gotMode==9&&gotSave.size()==3&&gotSave[2]==3);

    std::cout << "callback packet / #10007 bridge tests passed\n";
}
