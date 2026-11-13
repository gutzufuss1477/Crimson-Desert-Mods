#define MOUNT_TESTING
#include <stdio.h>
#include <string.h>
#include "../mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-speed/src/AllMountsLvL5Speed_adaptive.cpp"


#include "native_mount_continuation.hpp"
static unsigned checks;
#define REQUIRE(x) do { ++checks; if (!(x)) { printf("FAIL line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    U8 actor[0x300],context[0x500],target;
    U32 stat,sub,invalid;
    EXCEPTION_RECORD_MIN record;
    EXCEPTION_POINTERS_MIN pointers;
    // A small host PE must be refused before the guarded game RVAs are read.
    REQUIRE(initialize_mount(0)==1);
    for(stat=2;stat<=4;++stat) for(sub=14;sub<=19;++sub) for(invalid=0;invalid<8;++invalid) {
        U8* field=actor+0x230;
        int eligible;
        memset(actor,0,sizeof(actor));memset(context,0,sizeof(context));memset(&record,0,sizeof(record));
        wr64(field,2000);wr64(field+8,5000);wr64(field+16,0);wr64(field+24,1000);
        field[0x25]=(U8)stat;field[0x26]=(U8)sub;*(U16*)(field+0x28)=(U16)(sub+35);
        setctx64(context,0xF0,0x12345678);setctx64(context,0xE8,(U64)actor);
        setctx64(context,0x88,2000);setctx64(context,0x80,0x12345678);
        if(invalid==1)wr64(field+8,6000);
        if(invalid==2)wr64(field+16,1);
        if(invalid==3)wr64(field+24,999);
        if(invalid==4)*(U16*)(field+0x28)=0;
        if(invalid==5)wr64(field,6000);
        if(invalid==6)setctx64(context,0xE8,0);
        if(invalid==7)*(U16*)(field+0x28)=(U16)(sub+32); // Obsolete build must not silently match.
        target=0xCC;g_target=&target;g_original=0x4D;g_flush=0;g_singleStep=0;
        record.ExceptionCode=STATUS_BREAKPOINT;record.ExceptionAddress=&target;
        pointers.ExceptionRecord=&record;pointers.ContextRecord=context;
        eligible=stat==3 && sub==15 && invalid==0;

        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
        REQUIRE(rd64(field)==(eligible?5000:(invalid==5?6000:2000)));
        REQUIRE(ctx64(context,0x88)==2000);
        REQUIRE(ctx64(context,0x80)==0x12345678); // RCX max-value path must stay untouched
        REQUIRE(target==0x4D && ctx64(context,0xF8)==(U64)&target);
        REQUIRE((ctx32(context,0x44)&TF_FLAG)!=0);
        record.ExceptionCode=STATUS_SINGLE_STEP;
        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
        REQUIRE(target==0xCC && !(ctx32(context,0x44)&TF_FLAG));
        record.ExceptionCode=0;
        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_SEARCH);
    }
    // Exercise the observed field values without publishing raw runtime captures.
    // Semantic regression cases derived from observed fields. No memory dumps.
    struct Case { U64 current,maximum,minimum,step; U8 stat,sub; U16 ordinal; };
    static const Case cases[]={
        {15000,15000,0,1000,3,22,8},
        {7000,15000,0,1000,3,22,60},
        {0,15000,0,1000,3,22,61},
        {0,15000,0,1000,3,22,62},
        {4000,15000,0,1000,3,22,9},
        {105000,10000000,0,1000,3,1,5},
        {865000,865000,0,1000,0,22,0},
        {315000,315000,0,1000,3,22,25},
        {4000,5000,0,1000,3,15,50},
        {4000,15000,0,1000,3,22,9},
        {5000,5000,0,1000,3,17,52},
        {15000,15000,0,1000,3,22,7},
        {38000,10000000,0,1000,3,0,4},
        {4000,5000,0,1000,3,16,51},
    };
    U8 captured[sizeof(cases)/sizeof(cases[0])][80]{};
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        const auto& c=cases[i];auto* field=captured[i];
        wr64(field,c.current);wr64(field+8,c.maximum);wr64(field+16,c.minimum);wr64(field+24,c.step);
        field[0x25]=c.stat;field[0x26]=c.sub;*reinterpret_cast<U16*>(field+0x28)=c.ordinal;
    }
    for(const auto& data:captured) {
        memset(actor,0,sizeof(actor));memset(context,0,sizeof(context));
        U8* field=actor+0x230;memcpy(field,data,80);
        const U64 before=rd64(field);
        setctx64(context,0xE8,(U64)actor);setctx64(context,0x88,before);
        target=0xCC;g_target=&target;g_original=0x4D;g_flush=0;g_singleStep=0;
        record.ExceptionCode=STATUS_BREAKPOINT;record.ExceptionAddress=&target;
        pointers.ExceptionRecord=&record;pointers.ContextRecord=context;
        const bool eligible=field[0x25]==3 && field[0x26]==15;
        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
        REQUIRE(rd64(field)==(eligible?5000:before));
        REQUIRE(ctx64(context,0x88)==before);
        REQUIRE(memcmp(field+8,data+8,72)==0);
    }
    puts("PASS: semantic mount field cases apply; adjacent non-mount UI fields unchanged.");
    REQUIRE(test_native_mount_notification());
    puts("PASS: actual native continuation emits level 5 gauge update; unchanged value does not emit again.");
    printf("PASS: %u mount register/filter/breakpoint checks (host only)\n",checks);
    return 0;
}
