// Crimson Desert 2.03.02 - All Mounts LvL 5 Speed Only
// Freestanding x64 ASI, no CRT/imports. DMM-ready.
// Runtime path validated in-game with Rokade: StatIndex=3, Speed SubIndex 15.

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef long long I64;
typedef int I32;
typedef U64 SIZE_T;
typedef void* PVOID;
typedef int BOOL;

#define DLL_PROCESS_ATTACH 1
#define EXCEPTION_CONTINUE_EXECUTION ((I64)-1)
#define EXCEPTION_CONTINUE_SEARCH ((I64)0)
#define STATUS_BREAKPOINT ((U32)0x80000003u)
#define STATUS_SINGLE_STEP ((U32)0x80000004u)
#define PAGE_EXECUTE_READWRITE 0x40u
#define TF_FLAG 0x100u

extern U64 __readgsqword(unsigned long Offset);
#pragma intrinsic(__readgsqword)

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY* Flink;
    struct _LIST_ENTRY* Blink;
} LIST_ENTRY;

typedef struct _EXCEPTION_RECORD_MIN {
    U32 ExceptionCode;
    U32 ExceptionFlags;
    struct _EXCEPTION_RECORD_MIN* ExceptionRecord;
    PVOID ExceptionAddress;
    U32 NumberParameters;
    U32 __pad;
    U64 ExceptionInformation[15];
} EXCEPTION_RECORD_MIN;

typedef struct _EXCEPTION_POINTERS_MIN {
    EXCEPTION_RECORD_MIN* ExceptionRecord;
    U8* ContextRecord;
} EXCEPTION_POINTERS_MIN;

typedef PVOID (*RtlAddVectoredExceptionHandler_t)(U32 First, I64 (*Handler)(EXCEPTION_POINTERS_MIN*));
typedef I32 (*NtProtectVirtualMemory_t)(PVOID ProcessHandle, PVOID* BaseAddress, SIZE_T* RegionSize, U32 NewProtect, U32* OldProtect);
typedef I64 (*NtFlushInstructionCache_t)(PVOID ProcessHandle, PVOID BaseAddress, U32 Length);

static U8* g_gameBase = (U8*)0;
static U8* g_target = (U8*)0;
static U8 g_original = 0;
static volatile U32 g_singleStep = 0;
static volatile U64 g_stepTid = 0;
static NtFlushInstructionCache_t g_flush = (NtFlushInstructionCache_t)0;

static int streq(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a; ++b;
    }
    return *a == *b;
}

static U32 rd32(const U8* p) { return *(const volatile U32*)p; }
static U16 rd16(const U8* p) { return *(const volatile U16*)p; }
static U64 rd64(const U8* p) { return *(const volatile U64*)p; }
static void wr64(U8* p, U64 v) { *(volatile U64*)p = v; }

static PVOID resolve_export(U8* module, const char* wanted) {
    if (!module) return (PVOID)0;
    if (*(volatile U16*)module != 0x5A4D) return (PVOID)0; // MZ
    U32 peoff = rd32(module + 0x3C);
    U8* nt = module + peoff;
    if (rd32(nt) != 0x00004550u) return (PVOID)0; // PE\0\0
    U8* opt = nt + 24;
    if (rd16(opt) != 0x20B) return (PVOID)0; // PE32+
    U32 exportRva = rd32(opt + 112);
    U32 exportSize = rd32(opt + 116);
    if (!exportRva || !exportSize) return (PVOID)0;
    U8* ed = module + exportRva;
    U32 nfunc = rd32(ed + 20);
    U32 nname = rd32(ed + 24);
    U32 funcsRva = rd32(ed + 28);
    U32 namesRva = rd32(ed + 32);
    U32 ordsRva  = rd32(ed + 36);
    U32* funcs = (U32*)(module + funcsRva);
    U32* names = (U32*)(module + namesRva);
    U16* ords = (U16*)(module + ordsRva);
    U32 i;
    for (i = 0; i < nname; ++i) {
        const char* name = (const char*)(module + names[i]);
        if (streq(name, wanted)) {
            U16 ord = ords[i];
            if ((U32)ord >= nfunc) return (PVOID)0;
            U32 rva = funcs[ord];
            // We deliberately do not follow forwarded exports. ntdll exports used here are direct.
            if (rva >= exportRva && rva < exportRva + exportSize) return (PVOID)0;
            return (PVOID)(module + rva);
        }
    }
    return (PVOID)0;
}

static int find_ntdll_and_apis(RtlAddVectoredExceptionHandler_t* outVeh,
                               NtProtectVirtualMemory_t* outProtect,
                               NtFlushInstructionCache_t* outFlush) {
    U8* peb = (U8*)__readgsqword(0x60);
    if (!peb) return 0;
    U8* ldr = *(U8**)(peb + 0x18);
    if (!ldr) return 0;
    LIST_ENTRY* head = (LIST_ENTRY*)(ldr + 0x20); // InMemoryOrderModuleList
    LIST_ENTRY* cur = head->Flink;
    U32 guard = 0;
    while (cur && cur != head && guard++ < 128) {
        U8* entry = (U8*)cur - 0x10; // InMemoryOrderLinks offset in LDR_DATA_TABLE_ENTRY
        U8* base = *(U8**)(entry + 0x30);
        if (base) {
            RtlAddVectoredExceptionHandler_t veh =
                (RtlAddVectoredExceptionHandler_t)resolve_export(base, "RtlAddVectoredExceptionHandler");
            NtProtectVirtualMemory_t prot =
                (NtProtectVirtualMemory_t)resolve_export(base, "NtProtectVirtualMemory");
            NtFlushInstructionCache_t flush =
                (NtFlushInstructionCache_t)resolve_export(base, "NtFlushInstructionCache");
            if (veh && prot && flush) {
                *outVeh = veh;
                *outProtect = prot;
                *outFlush = flush;
                return 1;
            }
        }
        cur = cur->Flink;
    }
    return 0;
}

static U64 get_tid(void) {
    // TEB.ClientId.UniqueThread on x64.
    return __readgsqword(0x48);
}

static U64 ctx64(U8* c, U32 off) { return *(volatile U64*)(c + off); }
static void setctx64(U8* c, U32 off, U64 v) { *(volatile U64*)(c + off) = v; }
static U32 ctx32(U8* c, U32 off) { return *(volatile U32*)(c + off); }
static void setctx32(U8* c, U32 off, U32 v) { *(volatile U32*)(c + off) = v; }

static int valid_ptr(U64 p) {
    return p >= 0x10000ull && p < 0x0000800000000000ull;
}

static void set_break_byte(U8 v) {
    *g_target = v;
    if (g_flush) g_flush((PVOID)(I64)-1, g_target, 1);
}

static I64 veh_handler(EXCEPTION_POINTERS_MIN* ep) {
    if (!ep || !ep->ExceptionRecord || !ep->ContextRecord || !g_target)
        return EXCEPTION_CONTINUE_SEARCH;

    U32 code = ep->ExceptionRecord->ExceptionCode;
    U8* ctx = ep->ContextRecord;

    if (code == STATUS_BREAKPOINT && ep->ExceptionRecord->ExceptionAddress == (PVOID)g_target) {
        // Windows x64 CONTEXT offsets:
        // EFlags 0x44, RDX 0x88, R12 0xD8, R14 0xE8, RIP 0xF8.
        U64 r12 = ctx64(ctx, 0xD8);
        U64 r14 = ctx64(ctx, 0xE8);

        if (valid_ptr(r12) && valid_ptr(r14) && r12 >= r14 && (r12 - r14) == 0x230ull) {
            U8* field = (U8*)r12;
            U64 current = rd64(field + 0x00);
            U64 maxv    = rd64(field + 0x08);
            U64 minv    = rd64(field + 0x10);
            U64 step    = rd64(field + 0x18);
            U8 statIndex = *(volatile U8*)(field + 0x25);
            U8 subIndex  = *(volatile U8*)(field + 0x26);
            U16 ordinal  = *(volatile U16*)(field + 0x28);

            // Structure validated in 2.03.00 and statically rechecked in 2.03.02.
            if (statIndex == 3 &&
                subIndex == 15 &&
                ordinal == (U16)(subIndex + 32) &&
                maxv == 5000 && minv == 0 && step == 1000 &&
                current <= maxv) {
                wr64(field, maxv);
                setctx64(ctx, 0x88, maxv); // RDX = max, essential part of validated v5 path.
            }
        }

        // Execute the original instruction once, then re-arm INT3 via single-step.
        set_break_byte(g_original);
        setctx64(ctx, 0xF8, (U64)g_target);
        setctx32(ctx, 0x44, ctx32(ctx, 0x44) | TF_FLAG);
        g_stepTid = get_tid();
        g_singleStep = 1;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (code == STATUS_SINGLE_STEP && g_singleStep && get_tid() == g_stepTid) {
        set_break_byte(0xCC);
        setctx32(ctx, 0x44, ctx32(ctx, 0x44) & ~TF_FLAG);
        g_singleStep = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

__declspec(dllexport) BOOL DllMain(PVOID hModule, U32 reason, PVOID reserved) {
    (void)hModule; (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return 1;

    U8* peb = (U8*)__readgsqword(0x60);
    if (!peb) return 1;
    U8* imageBase = *(U8**)(peb + 0x10); // PEB.ImageBaseAddress
    if (!imageBase) return 1;
    g_gameBase = imageBase;

    // Strict Crimson Desert 2.03.02 guard. The sequence is unique in build 25474236.
    static const U8 guardBytes[16] = {
        0x4D,0x8B,0x9E,0x30,0x02,0x00,0x00,0x4D,
        0x3B,0xD8,0x0F,0x85,0xCA,0xFB,0xFF,0xFF
    };
    U8* guardAt = imageBase + 0x00E68B5C;
    U32 i;
    for (i = 0; i < 16; ++i) {
        if (guardAt[i] != guardBytes[i]) return 1; // Wrong build: fail closed.
    }

    RtlAddVectoredExceptionHandler_t addVeh = (RtlAddVectoredExceptionHandler_t)0;
    NtProtectVirtualMemory_t protect = (NtProtectVirtualMemory_t)0;
    NtFlushInstructionCache_t flush = (NtFlushInstructionCache_t)0;
    if (!find_ntdll_and_apis(&addVeh, &protect, &flush)) return 1;
    g_flush = flush;

    g_target = imageBase + 0x00E68F09;
    static const U8 targetBytes[16] = {
        0x4D,0x8B,0x96,0x30,0x02,0x00,0x00,0x49,
        0x3B,0xD2,0x75,0x09,0x49,0x3B,0x8E,0x38
    };
    for (i = 0; i < 16; ++i) {
        if (g_target[i] != targetBytes[i]) return 1; // Wrong build: fail closed.
    }
    g_original = *g_target;
    if (g_original == 0xCC) return 1;

    if (!addVeh(1, veh_handler)) return 1;

    PVOID page = (PVOID)((U64)g_target & ~0xFFFull);
    SIZE_T region = 0x1000;
    U32 oldProtect = 0;
    if (protect((PVOID)(I64)-1, &page, &region, PAGE_EXECUTE_READWRITE, &oldProtect) < 0)
        return 1;

    set_break_byte(0xCC);
    return 1;
}
