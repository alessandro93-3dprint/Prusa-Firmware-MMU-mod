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
    // memorizza lo slot che ci passa la printer
    slot = param;

    // 1) inizializza la puleggia
    mpu::pulley.InitAxis();

    // 2) ingaggia l’idler sullo slot corretto
    state = ProgressCode::EngagingIdler;
    error = ErrorCode::RUNNING;
    mi::idler.Engage(slot);

    // 3) spegni eventuali LED di stato
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
                constexpr auto UNLOAD_MM       = unit::U_mm{-100};  // ad esempio -100 mm
                constexpr auto UNLOAD_FEEDRATE = unit::U_mm_s{50};  // a 50 mm/s
                mpu::pulley.PlanMove(UNLOAD_MM, UNLOAD_FEEDRATE);
                state = P::UnloadingToPulley;
            }
            break;

        case P::UnloadingToPulley:
            if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
                // poi sgancia l’idler
                mi::idler.Disengage();
                state = P::DisengagingIdler;
            }
            break;

        case P::DisengagingIdler:
            if (mi::idler.Disengaged()) {
                UnloadFinishedCorrectly();
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

void UnloadFilament::UnloadFinishedCorrectly() {
    FinishedOK();
    mpu::pulley.Disable();
    ml::leds.SetAllOff();

    // registra lo stato “AtPulley” per lo slot corretto
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);
}

} // namespace logic
