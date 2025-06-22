/// @file load_filament.cpp
#include "load_filament.h"
#include "../unit.h"    // per unit::U_mm e unit::U_mm_s
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    // 1) aggiorno globals con il nuovo slot e imposto stato "AtPulley"
    mg::globals.SetFilamentLoaded(param, mg::FilamentLoadState::AtPulley);

    // 2) resetto eventuali errori e metto RUNNING
    error = ErrorCode::RUNNING;

    // 3) primo step: ingaggio l’idler sullo slot "param"
    state = ProgressCode::EngagingIdler;
    mi::idler.Engage(param);

    // 4) spengo i LED
    ml::leds.SetAllOff();
    return true;
}

bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
        case P::EngagingIdler:
            if (mi::idler.Engaged()) {
                // appena ingaggiato → pianifica caricamento a lunghezza fissa
                mpu::pulley.InitAxis();
                constexpr auto PRELOAD_MM       = unit::U_mm{60};
                constexpr auto PRELOAD_FEEDRATE = unit::U_mm_s{100};
                mpu::pulley.PlanMove(PRELOAD_MM, PRELOAD_FEEDRATE);
                state = P::UnloadingToPulley;  // o il tuo ProgressCode giusto per "FeedToNozzle"
            }
            break;

        case P::UnloadingToPulley:
            if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
                // fine precarica → sgancio idler
                mi::idler.Disengage();
                state = P::DisengagingIdler;
            }
            break;

        case P::DisengagingIdler:
            if (mi::idler.Disengaged()) {
                // completato con successo
                FinishedOK();
                mpu::pulley.Disable();
                ml::leds.SetAllOff();
                // lo stato globals è già stato aggiornato in Reset
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
    // reportto printer: OK o ERROR
    return (error == ErrorCode::OK) ? ResultCode::OK : ResultCode::ERROR;
}

} // namespace logic
