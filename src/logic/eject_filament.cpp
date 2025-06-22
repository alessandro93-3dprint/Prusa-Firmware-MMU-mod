/// @file eject_filament.cpp
#include "eject_filament.h"
#include "compat.h"
#include "../modules/buttons.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../modules/user_input.h"
#include "../debug.h"


namespace logic {

EjectFilament ejectFilament;

bool EjectFilament::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    error = ErrorCode::RUNNING;
    slot = param;

 // sempre: prima scarico, poi passo all’idler
    state = ProgressCode::UnloadingFilament;
    unl.Reset(param);
    return true;
}


bool EjectFilament::StepInner() {
    switch (state) {
     case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // scarico finito → ingaggia subito l’idler
            state = ProgressCode::EngagingIdler;
            mi::idler.Engage(slot);
        }
        break;
    case ProgressCode::EngagingIdler:
        if (mi::idler.Engaged()) {
            state = ProgressCode::EjectingFilament;
            mpu::pulley.InitAxis();
            mpu::pulley.PlanMove(config::ejectFromCuttingEdge, config::pulleySlowFeedrate);
        }
        break;
    case ProgressCode::EjectingFilament:
        if (mm::motion.QueueEmpty()) { // filament ejected
            GoToErrDisengagingIdler(ErrorCode::FILAMENT_EJECTED);
        }
        break;
    case ProgressCode::ERRDisengagingIdler:
        ErrDisengagingIdler();
        return false;
// se proprio non vuoi gestire l’errore a schermo, puoi semplificare:
    case ProgressCode::ERRWaitingForUser:
        return false;
    case ProgressCode::OK:
        return true;
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

ProgressCode EjectFilament::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    default:
        return state;
    }
}

ErrorCode EjectFilament::Error() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.Error(); // report sub-automaton errors properly
    default:
        return error;
    }
}

} // namespace logic
