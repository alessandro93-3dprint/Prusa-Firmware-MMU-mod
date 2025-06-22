/// @file load_filament.cpp
#include "load_filament.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../unit.h"        // per unit::U_mm e unit::U_mm_s
#include "../modules/progress_codes.h"
#include "../modules/globals.h"
#include "../debug.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) return false;

    // imposta slot attivo e stato di caricamento
    mg::globals.SetFilamentLoaded(param, mg::FilamentLoadState::AtPulley);
    error = ErrorCode::RUNNING;

    // primo step: ingaggia idler
    state = ProgressCode::EngagingIdler;
    mi::idler.Engage(param);
    ml::leds.SetAllOff();
    return true;
}

bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
        case P::EngagingIdler:
            if (mi::idler.Engaged()) {
                // appena ingaggiato → pianifica precarica (preload)
                mpu::pulley.InitAxis();
                constexpr auto PRELOAD_MM       = unit::U_mm{60};
                constexpr auto PRELOAD_FEEDRATE = unit::U_mm_s{100};
                mpu::pulley.PlanMove(PRELOAD_MM, PRELOAD_FEEDRATE);
                state = P::FeedingToNozzle;
            }
            break;

        case P::FeedingToNozzle:
            // attendi che termini la movimentazione
            if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
                // sgancia idler e chiudi
                mi::idler.Disengage();
                state = P::DisengagingIdler;
            }
            break;

        case P::DisengagingIdler:
            if (mi::idler.Disengaged()) {
                // completato OK
                FinishedOK();
                mpu::pulley.Disable();
                ml::leds.SetAllOff();
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
    return result;
}

} // namespace logic
