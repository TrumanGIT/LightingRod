#include "eventSink.h"
#include "logger.h"
#include "global.h"
#include "functions.h"

namespace EventSinks {


static auto lastStrikeCheck = std::chrono::steady_clock::now();

RE::BSEventNotifyControl PlayerCellEvent::ProcessEvent(const RE::BGSActorCellEvent* event,
    RE::BSTEventSource<RE::BGSActorCellEvent>*) {
    if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
        return RE::BSEventNotifyControl::kContinue;
    }

    static bool s_firstCellEvent = true;

    auto player = RE::PlayerCharacter::GetSingleton();

    if (!player) return RE::BSEventNotifyControl::kContinue;

    auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
    if (!cell) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // set lastcellwasinterior or not and last worldspace on intial startup
    if (s_firstCellEvent) {
        s_firstCellEvent = false;
        globals::lastCellWasInterior = cell->IsInteriorCell();
        logger::info("player is in interior on startup: {}", globals::lastCellWasInterior);
        return RE::BSEventNotifyControl::kContinue;
    }

    globals::currentCellIsInterior = cell->IsInteriorCell();

        if (!globals::lastCellWasInterior && !globals::currentCellIsInterior)
        {
            if (!isLightning()) return RE::BSEventNotifyControl::kContinue;

            auto now = std::chrono::steady_clock::now();
            auto timeSinceLast =
                std::chrono::duration_cast<std::chrono::seconds>(now - lastStrikeCheck).count();

            if (timeSinceLast < 60)
                return RE::BSEventNotifyControl::kContinue;

            lastStrikeCheck = now;
            LightningStrike();
        }

    globals::lastCellWasInterior = globals::currentCellIsInterior;

    return RE::BSEventNotifyControl::kContinue;
}

void PlayerCellEvent::RegisterEventSink() {
    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
        player->AsBGSActorCellEventSource()->AddEventSink(PlayerCellEvent::GetSingleton());
        logger::info("BGSActorCellEvent sink registered");
    }
}

}

