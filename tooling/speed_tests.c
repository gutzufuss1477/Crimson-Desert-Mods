#include <stdio.h>
#include <string.h>
#include "../mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/variants/all-mounts-level-5-speed/src/AllMountsLvL5Speed_20402.c"


static unsigned checks;
#define REQUIRE(x) do { ++checks; if (!(x)) { printf("FAIL line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    U8 actor[0x300],context[0x500],target;
    U32 stat,sub,invalid;
    EXCEPTION_RECORD_MIN record;
    EXCEPTION_POINTERS_MIN pointers;
    // A small host PE must be refused before the guarded game RVAs are read.
    REQUIRE(DllMain(0,DLL_PROCESS_ATTACH,0)==1);
    for(stat=2;stat<=4;++stat) for(sub=14;sub<=19;++sub) for(invalid=0;invalid<7;++invalid) {
        U8* field=actor+0x230;
        int eligible;
        memset(actor,0,sizeof(actor));memset(context,0,sizeof(context));memset(&record,0,sizeof(record));
        wr64(field,2000);wr64(field+8,5000);wr64(field+16,0);wr64(field+24,1000);
        field[0x25]=(U8)stat;field[0x26]=(U8)sub;*(U16*)(field+0x28)=(U16)(sub+32);
        setctx64(context,0xF0,(U64)field);setctx64(context,0xE8,(U64)actor);
        setctx64(context,0x80,2000);setctx64(context,0x88,0x12345678);
        if(invalid==1)wr64(field+8,6000);
        if(invalid==2)wr64(field+16,1);
        if(invalid==3)wr64(field+24,999);
        if(invalid==4)*(U16*)(field+0x28)=0;
        if(invalid==5)wr64(field,6000);
        if(invalid==6)setctx64(context,0xF0,(U64)field+8);
        target=0xCC;g_target=&target;g_original=0x4D;g_flush=0;g_singleStep=0;
        record.ExceptionCode=STATUS_BREAKPOINT;record.ExceptionAddress=&target;
        pointers.ExceptionRecord=&record;pointers.ContextRecord=context;
        eligible=stat==3 && sub==15 && invalid==0;

        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
        REQUIRE(rd64(field)==(eligible?5000:(invalid==5?6000:2000)));
        REQUIRE(ctx64(context,0x80)==(eligible?5000:2000));
        REQUIRE(ctx64(context,0x88)==0x12345678); // old RDX path must stay untouched
        REQUIRE(target==0x4D && ctx64(context,0xF8)==(U64)&target);
        REQUIRE((ctx32(context,0x44)&TF_FLAG)!=0);
        record.ExceptionCode=STATUS_SINGLE_STEP;
        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_EXECUTION);
        REQUIRE(target==0xCC && !(ctx32(context,0x44)&TF_FLAG));
        record.ExceptionCode=0;
        REQUIRE(veh_handler(&pointers)==EXCEPTION_CONTINUE_SEARCH);
    }
    printf("PASS: %u mount register/filter/breakpoint checks (host only)\n",checks);
    return 0;
}
