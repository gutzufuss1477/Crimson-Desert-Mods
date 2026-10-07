#include "bank_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace bank_refresh;
unsigned checks{};
void require(bool condition,const char* message) {
    ++checks;
    if(!condition) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
// Independent minute-step oracle for the native day/night calendar mapping.
uint64_t reference_ticks(uint64_t calendarMinutes) {
    uint64_t ticks=(calendarMinutes/1440)*DayTicks;
    for (uint32_t m=0;m<calendarMinutes%1440;++m) ticks+=(m<180 || m>=1260) ? 30000 : 60000;
    return ticks;
}
int main() {
    Calendar c{}; c.rate=12;
    uint64_t now{};
    require(calendar_ticks(c,now) && now==0,"midnight");
    c.hour=2; require(calendar_ticks(c,now) && now==3600000,"night conversion");
    c.hour=3; require(calendar_ticks(c,now) && now==5400000,"03:00 boundary");
    c.hour=12; require(calendar_ticks(c,now) && now==37800000,"noon");
    c.hour=21; require(calendar_ticks(c,now) && now==70200000,"21:00 boundary");
    c.hour=23; c.minute=59; c.second=59; c.millisecond=998;
    require(calendar_ticks(c,now) && now==DayTicks-1,"day wrap");
    c={}; c.rate=12; c.day=2; require(calendar_ticks(c,now) && now==2*DayTicks,"day count");
    c.hour=24; require(!calendar_ticks(c,now),"reject invalid hour");
    c.hour=0; c.rate=0; require(!calendar_ticks(c,now),"reject frozen/unknown clock rate");
    c.rate=1001; require(!calendar_ticks(c,now),"reject absurd rate");
    c.rate=12; c.minute=60; require(!calendar_ticks(c,now),"reject invalid minute");
    c.minute=0; c.second=60; require(!calendar_ticks(c,now),"reject invalid second");
    c.second=0; c.millisecond=1000; require(!calendar_ticks(c,now),"reject invalid milliseconds");
    c.millisecond=0; c.day=1000001; require(!calendar_ticks(c,now),"reject absurd day");
    require(eligible("Bank_01",0),"only observed recurring bank");
    require(!eligible("Bank_02",0) && !eligible("Bank_01",1) && !eligible(nullptr,0),"investment/other bank excluded");
    require(valid_interval(60) && valid_interval(1800) && !valid_interval(59) && !valid_interval(1801),"interval bounds");
    constexpr uint64_t start=5*DayTicks;
    const auto target=start+1800ULL*1000*12;
    require(cap_deadline(start+3*DayTicks,start,12,1800)==target,"30 minute cap");
    require(cap_deadline(start+3*DayTicks,start,12,60)==start+720000,"60 second test");
    require(cap_deadline(0,start,12,60)==0,"inactive timer unchanged");
    require(cap_deadline(start,start,12,60)==start,"due timer unchanged");
    require(cap_deadline(start-1,start,12,60)==start-1,"overdue timer unchanged");
    require(cap_deadline(start+100,start,12,60)==start+100,"never delay shorter timer");
    require(cap_deadline(target,start+1000,12,1800)==target,"do not slide deadline every callback");
    require(cap_deadline(target,target+1000,12,1800)==target,"native refresh remains in charge");
    require(cap_deadline(target+3*DayTicks,target,12,1800)==target+21600000,"cap subsequent native cycle");
    const auto maximum=std::numeric_limits<uint64_t>::max();
    require(cap_deadline(maximum,maximum-10,12,1800)==maximum,"overflow fail closed");
    require(cap_deadline(maximum,start,0,1800)==maximum && cap_deadline(maximum,start,12,0)==maximum,"bad parameters unchanged");
    BankRecord records[2]{}; records[0].key=7; records[0].next=start+3*DayTicks;
    records[1].key=8; records[1].next=999;
    records[0].unknown08=123; records[0].history[0]=42;
    records[0].next=cap_deadline(records[0].next,start,12,1800);
    require(records[0].next==target && records[0].unknown08==123 && records[0].history[0]==42 && records[1].next==999,"record layout / adjacent fields preserved");
    size_t bytes{};
    require(record_span(0x1000,2,bytes) && bytes==0x70,"live two-record array bounds");
    require(!record_span(0,2,bytes) && !record_span(0x1001,2,bytes),"null / unaligned array");
    require(!record_span(0x1000,0,bytes) && !record_span(0x1000,33,bytes),"empty / excessive count");
    require(!record_span(std::numeric_limits<uintptr_t>::max()-7,2,bytes),"array end overflow");
    uint32_t index{};
    require(unique_record(records,2,7,index) && index==0,"exact record selected");
    require(unique_record(records,2,8,index) && index==1,"second record selected");
    require(!unique_record(records,2,9,index),"missing bank rejected");
    records[1].key=7;
    require(!unique_record(records,2,7,index),"duplicate bank keys rejected");
    require(!unique_record(nullptr,2,7,index) && !unique_record(records,33,7,index),"invalid record snapshot");
    // Regression: BR-02 rejected the real count=2/header11c=1 state.
    // The unknown header word must not control array bounds or deadline writes.
    records[0].key=0; records[0].next=12020400000ULL; records[1].key=1; records[1].next=0;
    constexpr uint64_t observedNow=11759086620ULL;
    require(unique_record(records,2,0,index) &&
        cap_deadline(records[index].next,observedNow,12,60)==observedNow+720000,"observed BR-02 rejection regression");
    uint32_t setting=99;
    require(parse_game_minutes(L"10",setting) && setting==10,"10 game minute config");
    require(parse_game_minutes(L" 1440\t",setting) && setting==1440,"trimmed one day config");
    require(parse_game_minutes(L"1",setting) && setting==1 && parse_game_minutes(L"4320",setting) && setting==4320,"config bounds");
    const wchar_t* invalid[]={nullptr,L"",L" ",L"0",L"4321",L"-10",L"+10",L"10.5",L"10min",L"1 0",L"99999999999999999999999"};
    for (const auto text:invalid) {
        setting=99;
        require(!parse_game_minutes(text,setting) && setting==99,"invalid config fail closed");
    }
    uint64_t deadline{};
    c={}; c.rate=12; c.hour=12;
    require(calendar_ticks(c,now) && game_deadline(c,10,deadline) && deadline-now==600000,"10 daytime game minutes are 50 real seconds at rate 12");
    const auto dayDeadline=deadline;
    c.rate=24;
    require(game_deadline(c,10,deadline) && deadline==dayDeadline,"game interval independent of clock rate");
    c.hour=1; c.rate=12;
    require(calendar_ticks(c,now) && game_deadline(c,10,deadline) && deadline-now==300000,"night minutes use half the ticks");
    c.hour=2; c.minute=55;
    require(calendar_ticks(c,now) && game_deadline(c,10,deadline) && deadline-now==450000,"03:00 transition");
    c.hour=20; c.minute=55;
    require(calendar_ticks(c,now) && game_deadline(c,10,deadline) && deadline-now==450000,"21:00 transition");
    c.hour=23; c.minute=55;
    require(calendar_ticks(c,now) && game_deadline(c,10,deadline) && deadline==DayTicks+150000,"midnight rollover");
    c.day=2; c.hour=12; c.minute=5; c.second=37; c.millisecond=998;
    require(calendar_ticks(c,now) && game_deadline(c,1440,deadline) && deadline-now==DayTicks,"whole day preserves seconds and milliseconds");
    require(game_deadline(c,4320,deadline) && deadline-now==3*DayTicks,"three day interval");
    require(!game_deadline(c,0,deadline) && !game_deadline(c,4321,deadline),"invalid game interval");
    c.day=1000000; c.hour=23; c.minute=59;
    require(!game_deadline(c,10,deadline),"calendar upper bound");
    c.day=1; c.rate=0;
    require(!game_deadline(c,10,deadline),"invalid calendar clock");
    require(cap_game_deadline(0,100,200)==0 && cap_game_deadline(99,100,200)==99 && cap_game_deadline(100,100,200)==100,"inactive / due game deadlines preserved");
    require(cap_game_deadline(1000,100,200)==200 && cap_game_deadline(150,100,200)==150,"cap without extending shorter deadline");
    require(cap_game_deadline(1000,100,100)==1000,"invalid game limit preserved");
    require(cap_game_deadline(200,110,210)==200,"game deadline never slides");
    const uint32_t intervals[]={1,10,15,30,60,360,1440,4320};
    for (uint32_t minuteOfDay=0;minuteOfDay<1440;++minuteOfDay) {
        c={}; c.day=15; c.hour=minuteOfDay/60; c.minute=minuteOfDay%60; c.rate=12;
        require(calendar_ticks(c,now) && now==reference_ticks(15*1440+minuteOfDay),"calendar encoding minute-step oracle");
        uint64_t previous{};
        for (const auto duration:intervals) {
            require(game_deadline(c,duration,deadline) && deadline==reference_ticks(15*1440+minuteOfDay+duration) &&
                deadline>now && deadline>previous,"game interval every starting minute / boundary / monotonicity");
            previous=deadline;
        }
    }
    std::printf("PASS: %u bank scheduling / config checks\n",checks);
}
