/// @file tool_change.cpp
#include "tool_change.h"
#include "compat.h"
#include "../modules/buttons.h"
#include "../modules/fsensor.h"
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/permanent_storage.h"
#include "../modules/pulley.h"
#include "../modules/user_input.h"
#include "../debug.h"

namespace logic {

ToolChange toolChange;

bool ToolChange::Reset(uint8_t param) {
    if (!CheckToolIndex(param)) {
        return false;
    }

    if (param == mg::globals.ActiveSlot() && (mg::globals.FilamentLoaded() == mg::FilamentLoadState::InNozzle)) {
        // we are already at the correct slot and the filament is loaded in the nozzle - nothing to do
        dbg_logic_P(PSTR("we are already at the correct slot and the filament is loaded - nothing to do\n"));
        return true;
    }

    // @@TODO establish printer in charge of UI processing for the ToolChange command only.
    // We'll see how that works and then probably we'll introduce some kind of protocol settings to switch UI handling.
    mui::userInput.SetPrinterInCharge(true);

    // we are either already at the correct slot, just the filament is not loaded - load the filament directly
    // or we are standing at another slot ...
    plannedSlot = param;

if (mg::globals.FilamentLoaded() >= mg::FilamentLoadState::InSelector) {
    // scarico necessario → partiamo dall’unload
    state = ProgressCode::UnloadingFilament;
    unl.Reset(mg::globals.ActiveSlot());
} else {
    // salto lo unload, passo subito al Bondtech feed
    mg::globals.SetFilamentLoaded(plannedSlot, mg::FilamentLoadState::InSelector);
    GoToFeedingToBondtech();
}
    return true;
}

void logic::ToolChange::GoToFeedingToBondtech() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::blink0, ml::off);
    james.Reset(3);
    state = ProgressCode::FeedingToBondtech;
    error = ErrorCode::RUNNING;
}

void logic::ToolChange::ToolChangeFinishedCorrectly() {
    ml::leds.SetPairButOffOthers(mg::globals.ActiveSlot(), ml::on, ml::off);
    mui::userInput.SetPrinterInCharge(false);
    FinishedOK();
}

bool ToolChange::StepInner() {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        if (unl.StepInner()) {
            // unloading sequence finished - basically, no errors can occurr here
            // as UnloadFilament should handle all the possible error states on its own
            // There is no way the UnloadFilament to finish in an error state
            // But planning the next move can fail if Selector refuses moving to the next slot
            // - that scenario is handled inside GoToFeedingToFinda
            GoToFeedingToBondtech();
        }
        break;

    case ProgressCode::FeedingToBondtech:
        if (james.Step()) {
            switch (james.State()) {
            case FeedToBondtech::Failed:
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_DIDNT_SWITCH_ON); // signal loading error
                break;
            case FeedToBondtech::FSensorTooEarly:
                GoToErrDisengagingIdler(ErrorCode::FSENSOR_TOO_EARLY); // signal loading error
                break;
            default:
                ToolChangeFinishedCorrectly();
            }
        }
        break;
    case ProgressCode::OK:
        return true;

    case ProgressCode::ERRDisengagingIdler:
        ErrDisengagingIdler();
        return false;
    case ProgressCode::ERRWaitingForUser: {
        return false;
    }
    default: // we got into an unhandled state, better report it
        state = ProgressCode::ERRInternal;
        error = ErrorCode::INTERNAL;
        return true;
    }
    return false;
}

ProgressCode ToolChange::State() const {
    switch (state) {
    case ProgressCode::UnloadingFilament:
        return unl.State(); // report sub-automaton states properly
    case ProgressCode::FeedingToBondtech:
        // only process the important states
        switch (james.State()) {
        case FeedToBondtech::PushingFilamentToFSensor:
            return ProgressCode::FeedingToFSensor;
        case FeedToBondtech::PushingFilamentIntoNozzle:
            return ProgressCode::FeedingToNozzle;
        case FeedToBondtech::DisengagingIdler:
            return ProgressCode::DisengagingIdler;
        }
        [[fallthrough]]; // everything else is reported as is
    default:
        return state;
    }
}

ErrorCode ToolChange::Error() const {
    switch (state) {
    case ProgressCode::UnloadingFilament: {
        ErrorCode ec = unl.Error(); // report sub-automaton errors properly, only filter out OK and replace them with RUNNING
        return ec == ErrorCode::OK ? ErrorCode::RUNNING : ec;
    }
    default:
        return error;
    }
}

} // namespace logic
