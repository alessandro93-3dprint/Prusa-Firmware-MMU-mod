/// @file load_filament.cpp
#include "load_filament.h"
#include "progress_codes.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../unit.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    // 1) Salva lo slot
    slot = param;

    // 2) Imposta stato globale (slot attivo + AtPulley)
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);

    // 3) Prepara comando
    error = ErrorCode::RUNNING;
    state = ProgressCode::EngagingIdler;

    // 4) Ingaggia idler
    mi::idler.Engage(slot);

    // 5) Spegni LED
    ml::leds.SetAllOff();

    return true;
}

bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
    case P::EngagingIdler:
        if (mi::idler.Engaged()) {
            // avvia preload
            mpu::pulley.InitAxis();
            constexpr auto PRELOAD_MM       = unit::U_mm{100};   // regolati qui
            constexpr auto PRELOAD_FEEDRATE = unit::U_mm_s{50};  // regolati qui
            mpu::pulley.PlanMove(PRELOAD_MM, PRELOAD_FEEDRATE);
            state = P::FeedingToNozzle;
        }
        break;

    case P::FeedingToNozzle:
        if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
            // finito preload → sgancia
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

void LoadFilament::LoadFinishedCorrectly() {
    FinishedOK();
    mpu::pulley.Disable();
    ml::leds.SetAllOff();
}

} // namespace logic
