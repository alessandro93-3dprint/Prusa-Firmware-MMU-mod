#include "unload_filament.h"
#include "../unit.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"

namespace logic {

UnloadFilament unloadFilament;

bool UnloadFilament::Reset(uint8_t param) {
    // 1) seleziona lo slot
    mg::globals.SetActiveSlot(param);
    slot = param;

    // 2) inizializza la puleggia
    mpu::pulley.InitAxis();

    // 3) ingaggia l’idler sullo slot scelto
    state = ProgressCode::EngagingIdler;
    error = ErrorCode::RUNNING;
    mi::idler.Engage(slot);

    // 4) spegni eventuali LED di stato
    ml::leds.SetAllOff();

    return true;
}

bool UnloadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
      case P::EngagingIdler:
        if (mi::idler.Engaged()) {
            // appena ingaggiato → pianifica retrazione
            mpu::pulley.InitAxis();
            constexpr auto UNLOAD_DISTANCE   = unit::U_mm{-100}; // retrai 100 mm
            constexpr auto UNLOAD_FEEDRATE   = unit::U_mm_s{ 50}; // a 50 mm/s
            mpu::pulley.PlanMove(UNLOAD_DISTANCE, UNLOAD_FEEDRATE);
            state = P::UnloadingToPulley;
        }
        break;

      case P::UnloadingToPulley:
        if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
            // retrazione completata → sgancia l’idler
            mi::idler.Disengage();
            state = P::DisengagingIdler;
        }
        break;

      case P::DisengagingIdler:
        if (mi::idler.Disengaged()) {
            finishOK();
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

void UnloadFilament::finishOK() {
    // termina con OK e pulisce
    FinishedOK();
    mpu::pulley.Disable();
    ml::leds.SetAllOff();

    // aggiorna lo stato del filamento in globals
    mg::globals.SetFilamentLoaded(
        slot,
        mg::FilamentLoadState::AtPulley
    );

    result = ResultCode::OK;
}

} // namespace logic
