#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace mh125 {
constexpr uint32_t On = 1417341355;
constexpr uint32_t Off = 652747463;
constexpr size_t EventSize = 0xe8;
constexpr uint64_t MaxAgeMs = 120000;
constexpr size_t MaxTargets = 256;
using Event = std::array<uint8_t, EventSize>;
template<class T> T field(const Event& e, size_t offset) {
    T value{}; std::memcpy(&value, e.data() + offset, sizeof(value)); return value;
}
template<class T> void set(Event& e, size_t offset, T value) {
    std::memcpy(e.data() + offset, &value, sizeof(value));
}
struct Identity {
    uintptr_t component{}, actor{};
    uint32_t actorId{}, ownerId{};
    bool operator==(const Identity&) const = default;
};
struct Target {
    Identity id{};
    Event on{}, off{};
    bool occupied{}, haveOn{}, haveOff{}, nativeOn{}, syntheticOn{};
    uint64_t capturedAt{}, sentTime{};
    uint32_t revision{};
    uint32_t pending{}, pendingSerial{}, sentEvent{}, sentSerial{};
};
struct Dispatch {
    Identity id{};
    Event event{};
    uint32_t kind{}, serial{}, revision{};
};
// All access is serialized externally. Never dereferences remembered addresses.
struct State {
    std::array<Target, MaxTargets> targets{};
    uint32_t serial{};
    uint32_t revisionCounter{};
    uint64_t deadline{};
    unsigned overflow{};
    Target* find(const Identity& id) {
        for (auto& t : targets) if (t.occupied && t.id == id) return &t;
        return nullptr;
    }
    bool capture(const Identity& id, const Event& event, uint64_t now) {
        const auto kind = field<uint32_t>(event, 0);
        if (kind != On && kind != Off) return false;
        Target* t = find(id);
        if (!t && kind == On) {
            for (auto& slot : targets) {
                if (!slot.occupied || (!slot.syntheticOn && now - slot.capturedAt > MaxAgeMs)) {
                    slot = {}; slot.id = id; slot.occupied = true; t = &slot; break;
                }
            }
        }
        if (!t) { if (kind == On) ++overflow; return false; }
        t->revision=++revisionCounter;
        t->pending = 0;
        t->syntheticOn = false;
        if (kind == On) {
            t->on = event; t->haveOn = true; t->haveOff = false;
            t->nativeOn = true; t->capturedAt = now;
            t->sentEvent = 0;
        } else if (t->haveOn &&
                   field<uint64_t>(t->on, 0x10) == field<uint64_t>(event, 0x10)) {
            t->off = event; t->haveOff = true; t->nativeOn = false;
        } else {
            t->haveOff = false;
        }
        return true;
    }
    bool eligible(const Target& t, uint64_t now) const {
        return t.occupied && t.haveOn && t.haveOff && !t.nativeOn &&
               now - t.capturedAt <= MaxAgeMs;
    }
    unsigned request(uint32_t kind, uint64_t now) {
        ++serial; deadline = now + 5000;
        unsigned count = 0;
        for (auto& t : targets) {
            t.pending = 0;
            if ((kind == On && eligible(t, now) && !t.syntheticOn) ||
                (kind == Off && t.occupied && t.syntheticOn && !t.nativeOn)) {
                t.pending = kind; t.pendingSerial = serial; ++count;
            }
        }
        return count;
    }
    bool next(uint64_t now, Dispatch& out) {
        for(auto& t : targets) {
            if (!t.pending) continue;
            const uint32_t kind = t.pending;
            t.pending = 0;
            if (now > deadline || (kind == On && !eligible(t, now)) || t.nativeOn) continue;
            out = {t.id, kind == On ? t.on : t.off, kind, t.pendingSerial, t.revision};
            return true;
        }
        return false;
    }
    bool hasPending() const {
        for(const auto& t:targets) if(t.pending) return true;
        return false;
    }
    bool current(const Dispatch& d) {
        const auto* t=find(d.id);
        return t && t->revision==d.revision && !t->nativeOn && d.serial==serial;
    }
    void stamp(const Dispatch& d, uint64_t time) {
        if (auto* t = find(d.id); t && current(d)) { t->sentTime = time; t->sentEvent = d.kind; t->sentSerial = d.serial; }
    }
    void result(const Dispatch& d, uint32_t result) {
        if (auto* t = find(d.id); t && result == 0 && t->revision==d.revision && !t->nativeOn)
            t->syntheticOn = d.kind == On;
    }
    unsigned eligibleCount(uint64_t now) const {
        unsigned n=0; for (const auto& t : targets) if (eligible(t, now)) ++n; return n;
    }
    unsigned syntheticCount() const {
        unsigned n=0; for (const auto& t : targets) if (t.occupied && t.syntheticOn) ++n; return n;
    }
};
}
