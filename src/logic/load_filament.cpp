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
    slot   = param;  // salvo lo slot
    result = ResultCode::OK;

    // non c’era controllo? lo rimetti se serve:
    if (!CheckToolIndex(slot)) return false;

    // setto subito ActiveSlot
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);

    error = ErrorCode::RUNNING;
    state = ProgressCode::EngagingIdler;
    mi::idler.Engage(slot);  // uso 'slot' esplicitamente
    ml::leds.SetAllOff();
    return true;
}

bool LoadFilament::StepInner() {
    using P  = ProgressCode;
    using EC = ErrorCode;

    switch (state) {
        case P::EngagingIdler:
            if (mi::idler.Engaged()) {
                // avvio estrusione a lunghezza fissa o infinita, come vuoi
                mpu::pulley.InitAxis();
                constexpr auto LOAD_MM       = unit::U_mm{ 200 };   // es.
                constexpr auto LOAD_FEEDRATE = unit::U_mm_s{ 50 };
                mpu::pulley.PlanMove(LOAD_MM, LOAD_FEEDRATE);
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

    // già fatto in Reset, ma puoi ripetere se vuoi:
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);
    result = ResultCode::OK;
}

} // namespace logic
