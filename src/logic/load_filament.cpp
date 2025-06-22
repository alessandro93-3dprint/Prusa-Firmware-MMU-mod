/// @file load_filament.cpp
#include "load_filament.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../debug.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    slot = param;                     // memorizzo lo slot che mi passa la stampante
    // NON chiamare qui mg::globals.SetActiveSlot(): quel compito spetta a ToolChange!
    error = ErrorCode::RUNNING;       // partiamo in stato “running”
    state = ProgressCode::EngagingIdler;
    ml::leds.SetAllOff();
    mi::idler.Engage(slot);           // ingaggio l’idler sullo slot giusto
    return true;
}

bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
      case P::EngagingIdler:
        if (mi::idler.Engaged()) {
          // appena ingaggiato → avvia prericarica
          mpu::pulley.InitAxis();
          constexpr auto PRELOAD_MM       = unit::U_mm{100};   // regola a piacere
          constexpr auto PRELOAD_FEEDRATE = unit::U_mm_s{50};
          mpu::pulley.PlanMove(PRELOAD_MM, PRELOAD_FEEDRATE);
          state = P::FeedingToNozzle;
        }
        break;

      case P::FeedingToNozzle:
        if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
          mi::idler.Disengage();
          state = P::DisengagingIdler;
        }
        break;

      case P::DisengagingIdler:
        if (mi::idler.Disengaged()) {
          LoadFinishedCorrectly();
        }
        break;

      case P::OK:
        return true;

      default:
        state = P::ERRInternal;
        error = EC::INTERNAL;
        return true;
    }

    return false;
}

ResultCode LoadFilament::Result() const {
    // se non c’è stato errore restituisco OK, altrimenti “Cancelled”
    return (error == ErrorCode::OK)
           ? ResultCode::OK
           : ResultCode::Cancelled;
}

void LoadFilament::LoadFinishedCorrectly() {
    FinishedOK();                      // porta state a OK
    mpu::pulley.Disable();
    ml::leds.SetAllOff();
    // informo il modulo globals che questo slot ora è “InNozzle”
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::InNozzle);
}

} // namespace logic
