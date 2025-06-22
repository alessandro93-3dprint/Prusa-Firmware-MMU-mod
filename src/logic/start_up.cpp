/// @file
#include "start_up.h"
#include "compat.h"
#include "../modules/buttons.h"
#include "../modules/globals.h"
#include "../modules/user_input.h"

namespace logic {

StartUp startUp;

bool StartUp::Reset(uint8_t) {
    // Non facciamo più controlli su FINDA, partiamo sempre OK
    return true;

}

bool StartUp::StepInner() {
    switch (state) {
    case ProgressCode::OK:
        return true;
    default:
        // Do nothing
        break;
    }
    return false;
}

} // namespace logic
