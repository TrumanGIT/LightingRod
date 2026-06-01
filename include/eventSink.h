#pragma once

namespace EventSinks {

    // ideal for checking location is inteiror or exterior or checking for keywords, only runs on player
    struct PlayerCellEvent : RE::BSTEventSink<RE::BGSActorCellEvent> {
        static void RegisterEventSink();

        static PlayerCellEvent* GetSingleton() {
            static PlayerCellEvent singleton;
            return &singleton;
        }

    private:
        RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event,
            RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
    };
}