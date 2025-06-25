#pragma once
#include <cstdint>

namespace modules {
namespace selector {

struct Selector {
    enum OperationResult { Accepted, Refused, Ready };

    uint8_t state;
    uint8_t plannedSlot;
    bool homingValid;

    constexpr Selector()
        : state(0)
        , plannedSlot(0)
        , homingValid(false) {}

    void Init() {}
    void Step() {}

    OperationResult MoveToSlot(unsigned slot) {
    plannedSlot = slot;
    return Accepted;
    }
    OperationResult PlanHome() { return Refused; }
    int Slot() const { return plannedSlot; }
    bool HomingValid() const { return homingValid; }
    void InvalidateHoming() { homingValid = false; }
    bool IsOnHold() const { return state & 0x80; }
    void HoldOn() { state |= 0x80; }
    void Resume() { state &= ~0x80; }
    uint8_t State() const { return state; }

}; 


extern Selector selector;

}  // namespace selector
}  // namespace modules
