#pragma once

namespace modules {
namespace selector {

struct Selector {
    enum OperationResult { Accepted, Refused, Ready };

    void Init() {}
    void Step() {}
    OperationResult MoveToSlot(unsigned) { return Accepted; }
    int Slot() const { return 0; }
    bool HomingValid() const { return true; }
    void InvalidateHoming() {}
};

extern Selector selector;

}  // namespace selector
}  // namespace modules
