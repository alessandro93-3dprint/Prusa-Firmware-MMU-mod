#pragma once
#include <stdint.h>
#include "command_base.h"
#include "progress_codes.h"
#include "../unit.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"

namespace logic {

/// Stato macchina per scaricare il filamento da un dato slot
class UnloadFilament : public CommandBase {
public:
  constexpr UnloadFilament()
    : CommandBase()
    , slot(0)
    , result(ResultCode::OK) 
    {}

  /// @param param = indice dello slot da scaricare
  bool Reset(uint8_t param) override;
  bool StepInner() override;
  ResultCode Result() const override { return result; }

private:
  void finishOK();

  uint8_t     slot;
  ResultCode  result;
};

/// Istanza globale
extern UnloadFilament unloadFilament;

} // namespace logic
