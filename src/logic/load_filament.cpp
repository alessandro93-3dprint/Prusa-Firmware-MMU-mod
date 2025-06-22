/// @file load_filament.cpp
#include "load_filament.h"
#include "progress_codes.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../unit.h"        // per unit::U_mm e unit::U_mm_s

#include "../modules/globals.h"
#include "../debug.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    // 1) Salva lo slot che ci manda la stampante
    slot = param;

    // 2) Aggiorna lo stato globale (in selector/pulley)
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);

    // 3) Inizia il comando
    error = ErrorCode::RUNNING;
    state = ProgressCode::EngagingIdler;

    // 4) Ingaggia l’idler sullo slot giusto
    mi::idler.Engage(slot);

    // 5) Spegni eventuali LED
    ml::leds.SetAllOff();

    return true;
}


bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

        switch (state) {
        case P::EngagingIdler:
            if (mi::idler.Engaged()) {
                // appena ingaggiato → avvia caricamento by length
                mpu::pulley.InitAxis();
                constexpr auto PRELOAD_MM       = unit::U_mm{100};      // metti il valore voluto
                constexpr auto PRELOAD_FEEDRATE = unit::U_mm_s{50};  // es. 100 mm/s
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

void LoadFilament::LoadFinishedCorrectly() {
    FinishedOK();
    mpu::pulley.Disable();
    ml::leds.SetAllOff();

}

} // namespace logic
