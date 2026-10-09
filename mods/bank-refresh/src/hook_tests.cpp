// Host-side integration tests. Engine calls are doubles; no game/save is opened.
// Include the real hook implementation to exercise its reads, locking, routing,
// write and notification path without exporting a production test API.
#include "bank_refresh.cpp"
#include <cstdlib>

namespace {
unsigned checks{};
void require(bool ok,const char* message) {
    ++checks;
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
struct Info {
    uint64_t reserved{};
    const char** holder{};
    uint8_t padding[0x68]{};
    uint8_t kind{};
};
static_assert(offsetof(Info,kind)==0x78);
struct Component {
    uintptr_t reserved{},actor{};
    uint8_t padding[0x100]{};
    BankRecord* records{};
    uint32_t count{},unknown11c{1};
};
static_assert(offsetof(Component,records)==0x110 && offsetof(Component,count)==0x118);
enum class NativeAction { waiting, completeBond, rollGold, error, nullResult };
struct Fixture {
    const char* names[2]{"Bank_01","Bank_02"};
    Info info[2]{};
    uintptr_t vtable[5]{},lock[1]{},actor[2]{};
    BankRecord records[2]{};
    Component component{};
    Calendar date{};
    uint64_t now{},goldLimit{},bondLimit{};
    unsigned goldCalls{},bondCalls{},lookupCalls{},acquires{},releases{},notifications{};
    uint16_t notifiedKey{};
    bool held{};
    NativeAction action=NativeAction::waiting;
};
Fixture* fixture{};
void __fastcall acquire_stub(uintptr_t,uint8_t) {}
void __fastcall release_stub(uintptr_t lock,uint8_t mode) {
    require(lock==reinterpret_cast<uintptr_t>(fixture->lock) && mode==0 && fixture->held,"release correct write lock");
    fixture->held=false; ++fixture->releases;
}
void* __fastcall lock_stub(void* guard,uintptr_t lock,uint8_t refFlag) {
    require(lock==reinterpret_cast<uintptr_t>(fixture->lock) && refFlag==0 && !fixture->held,"acquire correct non-refcount write guard");
    require(fixture->goldCalls+fixture->bondCalls>0,"original called before adjustment lock");
    fixture->held=true; ++fixture->acquires;
    return guard;
}
uintptr_t __fastcall lookup_stub(const uint16_t* key) {
    ++fixture->lookupCalls;
    if(*key==7) return reinterpret_cast<uintptr_t>(&fixture->info[0]);
    if(*key==8) return reinterpret_cast<uintptr_t>(&fixture->info[1]);
    return 0;
}
void __fastcall notify_stub(uintptr_t component,uint16_t key) {
    require(!fixture->held && fixture->acquires==fixture->releases,"notify only after write lock released");
    require(component==reinterpret_cast<uintptr_t>(&fixture->component),"notify original component");
    ++fixture->notifications; fixture->notifiedKey=key;
}
int* native_stub(BankType type,uintptr_t component,int* result,const Calendar* calendar,uint16_t key) {
    require(component==reinterpret_cast<uintptr_t>(&fixture->component) && calendar==&fixture->date &&
        !fixture->held,"original receives component/calendar before mod lock");
    ++(type==BankType::bonds ? fixture->bondCalls : fixture->goldCalls);
    *result=fixture->action==NativeAction::error ? 17 : 0;
    if(fixture->action==NativeAction::completeBond) {
        require(type==BankType::bonds && key==8,"complete only intended bond");
        fixture->records[1].next=0;
    }
    if(fixture->action==NativeAction::rollGold) {
        require(type==BankType::gold && key==7,"renew only regular gold timer");
        fixture->records[0].next=fixture->now+3*DayTicks;
    }
    return fixture->action==NativeAction::nullResult ? nullptr : result;
}
int* __fastcall gold_stub(uintptr_t c,int* r,const Calendar* d,uint16_t k) {
    return native_stub(BankType::gold,c,r,d,k);
}
int* __fastcall bond_stub(uintptr_t c,int* r,const Calendar* d,uint16_t k) {
    return native_stub(BankType::bonds,c,r,d,k);
}
void reset(Fixture& f) {
    f=Fixture{}; fixture=&f;
    for(unsigned i=0;i<2;++i) {
        f.info[i].holder=&f.names[i]; f.info[i].kind=static_cast<uint8_t>(i);
        f.records[i].key=static_cast<uint16_t>(7+i);
        f.records[i].unknown08=123+i;
        f.records[i].history[0]=static_cast<uint8_t>(42+i);
    }
    f.vtable[3]=reinterpret_cast<uintptr_t>(&acquire_stub);
    f.vtable[4]=reinterpret_cast<uintptr_t>(&release_stub);
    f.lock[0]=reinterpret_cast<uintptr_t>(f.vtable);
    f.actor[1]=reinterpret_cast<uintptr_t>(f.lock);
    f.component.actor=reinterpret_cast<uintptr_t>(f.actor);
    f.component.records=f.records; f.component.count=2;
    f.date.day=12; f.date.hour=12; f.date.rate=12;
    require(calendar_ticks(f.date,f.now) && game_deadline(f.date,15,f.goldLimit) &&
        game_deadline(f.date,30,f.bondLimit,BankType::bonds),"fixture dates");
    f.records[0].next=f.now+3*DayTicks; f.records[1].next=f.now+7*DayTicks;
    intervalGameMinutes=15; bondGameMinutes=30;
    goldEnabled=bondsEnabled=true; hooksReady=true; faulted=false;
    original=gold_stub; originalBonds=bond_stub;
    lookupBank=lookup_stub; lockBank=lock_stub; notifyBank=notify_stub;
}
void run(BankType type,uintptr_t caller,uint16_t key) {
    const auto oldGold=fixture->goldCalls,oldBond=fixture->bondCalls;
    int result=-1;
    auto returned=dispatch_refresh(type,caller,reinterpret_cast<uintptr_t>(&fixture->component),&result,&fixture->date,key);
    require(returned==(fixture->action==NativeAction::nullResult ? nullptr : &result),"preserve exact original return pointer");
    require(result==(fixture->action==NativeAction::error ? 17 : 0),"preserve native result value");
    require(fixture->goldCalls==oldGold+(type==BankType::gold ? 1u : 0u) &&
        fixture->bondCalls==oldBond+(type==BankType::bonds ? 1u : 0u),"correct original called exactly once");
    require(!fixture->held && fixture->acquires==fixture->releases,"all locks released");
}
void bond() { run(BankType::bonds,BondTimerReturnRva,8); }
void gold() { run(BankType::gold,TimerReturnRva,7); }
}
int main() {
    Fixture f{}; reset(f);
    BankRecord expected[2]; std::memcpy(expected,f.records,sizeof(expected));
    expected[1].next=f.bondLimit;
    bond();
    require(std::memcmp(expected,f.records,sizeof(expected))==0 && f.notifications==1 && f.notifiedKey==8,"bond changes ONLY its deadline and not adjacent bank/history/money");
    expected[0].next=f.goldLimit; gold();
    require(std::memcmp(expected,f.records,sizeof(expected))==0 && f.notifications==2 && f.notifiedKey==7,"independent gold interval");
    f.date.minute=1; bond();
    require(f.records[1].next==f.bondLimit && f.notifications==2,"repeated bond callback does not slide or notify unnecessarily");
    reset(f); f.records[1].next=0; bond();
    require(f.records[1].next==0 && f.notifications==0,"inactive bond never armed");
    reset(f); f.records[1].next=f.now; f.action=NativeAction::completeBond; bond();
    require(f.records[1].next==0 && f.notifications==0,"native completion not reversed");
    f.action=NativeAction::waiting; bond();
    require(f.records[1].next==0,"completed bond remains inactive on next callback");
    reset(f); f.records[1].next=f.now; bond();
    require(f.records[1].next==f.now && f.notifications==0,"due bond not postponed if game has not completed it");
    reset(f); f.records[1].next=f.now-1; bond();
    require(f.records[1].next==f.now-1,"overdue bond unchanged");
    reset(f); f.records[1].next=f.now+10; bond();
    require(f.records[1].next==f.now+10 && f.notifications==0,"shorter bond unchanged");
    reset(f); f.records[0].next=f.now; f.action=NativeAction::rollGold; gold();
    require(f.records[0].next==f.goldLimit,"native gold renewal still capped after original");
    reset(f); bondsEnabled=false; bond(); gold();
    require(f.records[1].next==f.now+7*DayTicks && f.records[0].next==f.goldLimit,"bond disabled leaves gold enabled");
    reset(f); goldEnabled=false; gold(); bond();
    require(f.records[0].next==f.now+3*DayTicks && f.records[1].next==f.bondLimit,"bond-only mode leaves gold untouched");
    for(unsigned scenario=0;scenario<14;++scenario) {
        reset(f);
        switch(scenario) {
            case 0: hooksReady=false; break;
            case 1: faulted=true; break;
            case 2: f.action=NativeAction::error; break;
            case 3: f.action=NativeAction::nullResult; break;
            case 4: f.info[1].kind=0; break;
            case 5: f.names[1]="Bank_01"; break;
            case 6: f.names[1]="Bank_03"; break;
            case 7: f.date.rate=0; break;
            case 8: f.component.count=33; break;
            case 9: f.records[0].key=8; break;
            case 10: f.component.records=nullptr; break;
            case 11: f.info[1].holder=nullptr; break;
            case 12: f.component.actor=0; break;
            case 13: f.component.records=reinterpret_cast<BankRecord*>(uintptr_t(1)); break;
        }
        bond(); require(f.records[1].next==f.now+7*DayTicks && f.notifications==0,"fail-closed bond guard");
    }
    reset(f); run(BankType::bonds,TimerReturnRva,8);
    require(f.lookupCalls==0 && f.notifications==0,"gold caller cannot enter bond adjustment");
    reset(f); run(BankType::gold,BondTimerReturnRva,7);
    require(f.lookupCalls==0 && f.notifications==0,"bond caller cannot enter gold adjustment");
    reset(f); run(BankType::bonds,BondTimerReturnRva,7);
    require(f.notifications==0 && f.records[0].next==f.now+3*DayTicks,"gold key not eligible through bond hook");
    reset(f); run(BankType::gold,TimerReturnRva,8);
    require(f.notifications==0 && f.records[1].next==f.now+7*DayTicks,"bond key not eligible through gold hook");
    reset(f); run(BankType::bonds,BondTimerReturnRva,123);
    require(f.notifications==0,"unknown key rejected");
    std::printf("PASS: %u host-side hook checks (engine doubles, NOT an in-game test)\n",checks);
}
