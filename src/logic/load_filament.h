/// @file load_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "progress_codes.h"
#include "../unit.h"  // per unit::U_mm e unit::U_mm_s

namespace logic {

class LoadFilament : public CommandBase {
public:
  constexpr LoadFilament() : CommandBase(), slot(0), error(ResultCode::OK) {}

  bool Reset(uint8_t param) override;
  bool StepInner() override;
  ResultCode Result() const override { return (error == ErrorCode::OK) ? ResultCode::OK : ResultCode::ERROR_INTERNAL; }

private:
  void LoadFinishedCorrectly();

  uint8_t slot;
  ErrorCode error;
};

extern LoadFilament loadFilament;

} // namespace logic
