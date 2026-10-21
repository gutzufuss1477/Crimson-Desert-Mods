#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>

namespace bank_refresh {
inline constexpr uint64_t DayTicks = 75600000;
enum class BankType : uint8_t { gold, bonds };
inline constexpr uint32_t maximum_minutes(BankType type) {
    return type==BankType::bonds ? 10080 : 4320;
}
inline constexpr char ExpectedSha[] = "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";

// Observed in 1420e55a0 / 1420d3460, not an ordinary wall-clock struct.
struct Calendar {
    uint32_t day, hour, minute, second, millisecond;
    uint8_t weekday, unused;
    uint16_t rate;
    uint64_t referenceClock;
};
static_assert(sizeof(Calendar) == 32);

inline bool calendar_ticks(const Calendar& c, uint64_t& out) {
    if (c.day > 1000000 || c.hour > 23 || c.minute > 59 || c.second > 59 ||
        c.millisecond > 999 || c.rate == 0 || c.rate > 1000) return false;
    uint64_t time = ((uint64_t(c.hour) * 60 + c.minute) * 60 + c.second) * 1000 + c.millisecond;
    if (time < 10800000) time /= 2;
    else if (time < 75600000) time -= 5400000;
    else time = (time + 64800000) / 2;
    out = uint64_t(c.day) * DayTicks + time;
    return true;
}

inline bool valid_interval(uint32_t seconds) { return seconds >= 60 && seconds <= 1800; }

inline uint64_t cap_deadline(uint64_t current, uint64_t now, uint16_t rate, uint32_t seconds) {
    // Never arm an inactive timer, postpone a due timer, or extend a shorter one.
    if (!valid_interval(seconds) || rate == 0 || rate > 1000 || current == 0 || current <= now) return current;
    const uint64_t duration = uint64_t(seconds) * 1000 * rate;
    if (now > std::numeric_limits<uint64_t>::max() - duration) return current;
    const auto limit = now + duration;
    return current > limit ? limit : current;
}

inline bool valid_game_minutes(uint32_t minutes,BankType type=BankType::gold) {
    return minutes>=1 && minutes<=maximum_minutes(type);
}

inline bool parse_game_minutes(const wchar_t* text, uint32_t& minutes,BankType type=BankType::gold) {
    if (!text) return false;
    while (*text==L' ' || *text==L'\t') ++text;
    if (*text<L'0' || *text>L'9') return false;
    uint32_t value=0;
    do {
        value=value*10+uint32_t(*text-L'0');
        if (value>maximum_minutes(type)) return false;
        ++text;
    } while (*text>=L'0' && *text<=L'9');
    while (*text==L' ' || *text==L'\t') ++text;
    if (*text || !valid_game_minutes(value,type)) return false;
    minutes=value;
    return true;
}

// Add displayed calendar minutes BEFORE the native non-linear day/night encoding.
// Multiplying minutes by rate or by a constant tick value is not calendar time.
inline bool game_deadline(const Calendar& date, uint32_t minutes, uint64_t& deadline,BankType type=BankType::gold) {
    uint64_t now{};
    if (!valid_game_minutes(minutes,type) || !calendar_ticks(date,now)) return false;
    const uint64_t total=uint64_t(date.hour)*60+date.minute+minutes;
    Calendar target=date;
    const uint64_t day=uint64_t(date.day)+total/1440;
    if (day>1000000) return false;
    target.day=static_cast<uint32_t>(day);
    target.hour=static_cast<uint32_t>((total%1440)/60);
    target.minute=static_cast<uint32_t>(total%60);
    return calendar_ticks(target,deadline) && deadline>now;
}

inline uint64_t cap_game_deadline(uint64_t current, uint64_t now, uint64_t limit) {
    // Leave inactive, due and already shorter deadlines to the original game.
    if (!current || current<=now || limit<=now) return current;
    return current>limit ? limit : current;
}

struct BankRecord {
    uint16_t key;
    uint8_t propensity;
    uint8_t padding[5];
    uint64_t unknown08;
    uint64_t next;
    uint8_t history[32];
};
static_assert(sizeof(BankRecord) == 0x38);
static_assert(offsetof(BankRecord, next) == 0x10);

inline bool record_span(uintptr_t address, uint32_t count, size_t& bytes) {
    bytes=0;
    if (!address || address%alignof(BankRecord)!=0 || count==0 || count>32) return false;
    const size_t length=size_t(count)*sizeof(BankRecord);
    if (address>std::numeric_limits<uintptr_t>::max()-length) return false;
    bytes=length;
    return true;
}

inline bool unique_record(const BankRecord* records, uint32_t count, uint16_t key, uint32_t& index) {
    if (!records || count==0 || count>32) return false;
    bool found=false;
    for (uint32_t i=0;i<count;++i) {
        if (records[i].key!=key) continue;
        if (found) return false;
        index=i;
        found=true;
    }
    return found;
}

inline bool eligible(const char* name, uint8_t investmentKind,BankType type=BankType::gold) {
    const char* expected=type==BankType::bonds ? "Bank_02" : "Bank_01";
    const uint8_t expectedKind=type==BankType::bonds ? 1 : 0;
    if (!name || investmentKind != expectedKind) return false;
    for (unsigned i = 0; i < 8; ++i) if (name[i] != expected[i]) return false;
    return true;
}
}
