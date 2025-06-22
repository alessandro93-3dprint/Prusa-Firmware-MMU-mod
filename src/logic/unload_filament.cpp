#include "unload_filament.h"
#include "../unit.h"
#include "progress_codes.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"

namespace logic {

UnloadFilament unloadFilament;

bool UnloadFilament::Reset(uint8_t param) {
  slot = param;
  
  mg::globals.SetActiveSlot(slot);          // ← aggiorniamo anche l’ActiveSlot globale

  // 1) prepara la puleggia
  mpu::pulley.InitAxis();

  // 2) ingaggia l’idler sullo slot giusto
  state = ProgressCode::EngagingIdler;
  error = ErrorCode::RUNNING;
  mi::idler.Engage(slot);

  // 3) pulizia LED
  ml::leds.SetAllOff();
  return true;
}

bool UnloadFilament::StepInner() {
    using P = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
    case P::EngagingIdler:
        if (mi::idler.Engaged()) {
            mpu::pulley.InitAxis();
            constexpr auto UNLOAD_MM       = unit::U_mm{-100};  // esempio 100 mm indietro
            constexpr auto UNLOAD_FEEDRATE = unit::U_mm_s{50};
            mpu::pulley.PlanMove(UNLOAD_MM, UNLOAD_FEEDRATE);
            state = P::UnloadingToPulley;
        }
        break;

    case P::UnloadingToPulley:
        if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
            mi::idler.Disengage();
            state = P::DisengagingIdler;
        }
        break;

    case P::DisengagingIdler:
        if (mi::idler.Disengaged()) {
            FinishedOK();
            mpu::pulley.Disable();
            ml::leds.SetAllOff();
            mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley); 
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


} // namespace logic
