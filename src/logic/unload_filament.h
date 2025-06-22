#pragma once
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "progress_codes.h"

namespace logic {

class UnloadFilament : public CommandBase {
public:
  // zero-init slot
  constexpr UnloadFilament()
    : CommandBase()
    , slot(0)
    {}

  bool Reset(uint8_t param) override;
  bool StepInner() override;

private:
  void UnloadFinishedCorrectly();
  uint8_t slot;  // lo slot che ci passa Marlin
};

extern UnloadFilament unloadFilament;

} // namespace logic
