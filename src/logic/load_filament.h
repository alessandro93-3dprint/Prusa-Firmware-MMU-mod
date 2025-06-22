/// @file load_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "progress_codes.h"
#include "../unit.h"      // per unit::U_mm e unit::U_mm_s

namespace logic {

class LoadFilament : public CommandBase {
public:
  // qui inizializziamo solo slot: state ed error sono già settati da CommandBase()
  constexpr LoadFilament() : CommandBase(), slot(0) {}

  bool Reset(uint8_t param) override;
  bool StepInner() override;
  ResultCode Result() const override;

private:
  void LoadFinishedCorrectly();

  uint8_t slot;          ///< il numero di slot da caricare
};

extern LoadFilament loadFilament;

} // namespace logic
