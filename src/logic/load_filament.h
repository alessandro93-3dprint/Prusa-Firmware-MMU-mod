/// @file load_filament.h
#pragma once
#include <stdint.h>
#include "command_base.h"
#include "../unit.h"  // per unit::U_mm e unit::U_mm_s

namespace logic {

class LoadFilament : public CommandBase {
public:
  constexpr LoadFilament()
    : CommandBase()
    , result(ResultCode::OK)
    {}

  bool Reset(uint8_t param) override;
  bool StepInner() override;

  // Restituisce il risultato dell’ultimo comando
  ResultCode Result() const override 

private:
  void Reset2(bool feedPhaseLimited);
  void LoadFinishedCorrectly();

  uint8_t      verifyLoadedFilament{0};
  ResultCode   result;
};

extern LoadFilament loadFilament;

} // namespace logic
