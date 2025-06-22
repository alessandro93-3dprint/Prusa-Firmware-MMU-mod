#pragma once
#include <stdint.h>
#include "command_base.h"
#include "progress_codes.h"
#include "../unit.h"

namespace logic {

class UnloadFilament : public CommandBase {
public:
  inline constexpr UnloadFilament()
    : CommandBase(), slot(0), result(ResultCode::OK) {}

  bool Reset(uint8_t param) override;
  bool StepInner() override;
  ResultCode Result() const override { return result; }

private:
  void UnloadFinishedCorrectly();

  uint8_t      slot;   // lo slot che ci passa Marlin
  ResultCode   result;
};

extern UnloadFilament unloadFilament;

} // namespace logic
