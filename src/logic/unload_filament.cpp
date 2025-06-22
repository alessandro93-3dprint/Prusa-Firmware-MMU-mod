#include "unload_filament.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"

namespace logic {

UnloadFilament unloadFilament;

bool UnloadFilament::Reset(uint8_t param) {
    slot = param;                    // memorizza lo slot
    error = ErrorCode::RUNNING;
    state = ProgressCode::EngagingIdler;
    ml::leds.SetAllOff();
    mi::idler.Engage(slot);          // ingaggia l’idler sullo slot giusto
    return true;
}

bool UnloadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
        case P::EngagingIdler:
            if (mi::idler.Engaged()) {
                mpu::pulley.InitAxis();
                constexpr auto UNLOAD_MM       = unit::U_mm{-100};
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
    // registra lo stato per lo slot corretto: non tocchiamo ActiveSlot!
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);
}

} // namespace logic
