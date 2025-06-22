/// @file load_filament.cpp
#include "load_filament.h"
#include "compat.h"
#include "../unit.h"    // per unit::U_mm e unit::U_mm_s
#include "../modules/globals.h"
#include "../modules/idler.h"
#include "../modules/leds.h"
#include "../modules/motion.h"
#include "../modules/pulley.h"
#include "../debug.h"

namespace logic {

LoadFilament loadFilament;

bool LoadFilament::Reset(uint8_t param) {
    // 1) controlla che lo slot sia valido
    if (!CheckToolIndex(param)) return false;

    // 2) evita di iniziare il caricamento se c'è già filamento oltre la puleggia
    if (mg::globals.FilamentLoaded() > mg::FilamentLoadState::AtPulley
        && mg::globals.ActiveSlot() != param) {
        return false;
    }

    // 3) registra lo slot attivo e lo stato di caricamento
    mg::globals.SetFilamentLoaded(param, mg::FilamentLoadState::AtPulley);

    // 4) inizializza la puleggia e metti il comando in esecuzione
    error = ErrorCode::RUNNING;
    mpu::pulley.InitAxis();

    // 5) ingaggia subito l’idler
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
        // appena l’idler ha afferrato il filamento...
        if (mi::idler.Engaged()) {
          // 2° step: parti con il caricamento verso l’ugello
          constexpr auto FEED_MM       = unit::U_mm{60};    // quanti mm caricare
          constexpr auto FEED_FEEDRATE = unit::U_mm_s{100}; // velocità
          mpu::pulley.PlanMove(FEED_MM, FEED_FEEDRATE);
          state = P::FeedingToNozzle;
        }
        break;

      case P::FeedingToNozzle:
        // aspetto che la coda di movimento finisca
        if (mm::motion.QueueEmpty(mm::Axis::Pulley)) {
          // 3° step: quando ha finito, sgancio l’idler
          state = P::DisengagingIdler;
          mi::idler.Disengage();
        }
        break;

      case P::DisengagingIdler:
        // una volta sganciato, ho finito con successo
        if (mi::idler.Disengaged()) {
          FinishedOK();
          mpu::pulley.Disable();
          ml::leds.SetAllOff();
          // lo stato di globals rimane AtPulley, pronto per next Unload/Load
        }
        break;

      case P::OK:
        return true;

      default:
        // stato non riconosciuto → errore interno
        state = P::ERRInternal;
        error = EC::INTERNAL;
        return true;
    }

    return false;
}


} // namespace logic
