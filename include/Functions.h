#pragma once

#include "global.h"
#include "logger.h"

bool Initialize()
{
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        logger::error("DataHandler is null");
        return false;
    }

    globals::keywords = {
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x01E718),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x01E719),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x0C5C00),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x01E71C),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x01E71A),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x10AA1A),
        RE::TESForm::LookupByID<RE::BGSKeyword>(0x04026230)
    };

globals::lightningSpell =
        dataHandler->LookupForm<RE::SpellItem>(0x808, "Lightning Rod.esp");

    if (!globals::lightningSpell) {
        logger::error("Failed to find lightning spell");
        return false;
    }

globals::shockDamageEffect =
        RE::TESForm::LookupByID<RE::EffectSetting>(0x0E4CB6);

    if (!globals::shockDamageEffect) {
        logger::error("Failed to find shock damage effect");
        return false;
    }

    auto activator =
        RE::TESForm::LookupByEditorID("Lightning Activator");

    if (!activator) {
        logger::error("Failed to find activator");
        return false;
    }

    globals::activatorBaseObject = activator->As<RE::TESBoundObject>();

    if (!globals::activatorBaseObject) {
        logger::error("Activator is not a TESBoundObject");
        return false;
    }

    for (auto& effect : globals::lightningSpell->effects) {
        if (effect && effect->baseEffect == globals::shockDamageEffect) {
            effect->effectItem.magnitude =
                globals::g_LightningDamageMultiplier;
            break;
        }
    }

    logger::info("LightningRod initialized successfully");
    return true;
}

bool StrikeConditionsMet(RE::Actor* actor) {
    if (!actor) {
        return false;
    }

    auto npcState = actor->AsActorState();

    if (!npcState || !npcState->IsWeaponDrawn() || !actor->Is3DLoaded()) return false;

    auto rightWeapon = actor->GetEquippedObject(false);  // false = right hand
    if (!rightWeapon) {
        return false;
    }

    auto weapon = rightWeapon->As<RE::TESObjectWEAP>();
    if (!weapon) {
        return false;
    }

    for (auto& keyword : globals::keywords) {
        if (weapon->HasKeyword(keyword)) {
            return true;
        }
    }

    return false;
}

bool ShouldTriggerLightning(float strikeChance, RE::Actor* actor) {

    if (!actor || !StrikeConditionsMet(actor)) return false; 

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);

    float roll = dist(gen);

    return roll <= strikeChance; 
}

bool isLightning() {
    auto sky = RE::Sky::GetSingleton();
    if (!sky) {
        return false;
    }

    if (sky->IsSnowing() || !sky->IsRaining()) {
        return false;
    }

    auto weather = sky->currentWeather;
    if (!weather) {
        return false;
    }


    std::uint8_t lightningFreq = static_cast<std::uint8_t>(weather->data.thunderLightningFrequency);
    //   logger::info(" wether found and lightningFreq = {}", lightningFreq);
    if (lightningFreq < 255) {
        return true;
    }
    return false;
}

bool CastSpell(RE::TESObjectREFR* activatorRef, RE::MagicItem* spell, RE::Actor* target) {
    //   logger::info("CastSpell called");

    if (!activatorRef || !spell || !target) {
        logger::warn("Invalid caster or spell or target");
        return false;
    }

    auto casterMagicCaster = activatorRef->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
    if (!casterMagicCaster) {
        logger::warn("Failed to get caster's MagicCaster");
        return false;
    }

    casterMagicCaster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, nullptr);
    // logger::info("Spell cast successfully");
    return true;
}

float RandomFloat(float min = 0.0, float max = 100.0) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

RE::TESObjectREFR* CreatePersistentActivator(RE::TESObjectREFR* caster, RE::TESBoundObject* activatorBase) {
    if (!caster || !activatorBase) {
        logger::error("CreatePersistentActivator: Missing caster or base");
        return nullptr;
    }

    auto ref = caster->PlaceObjectAtMe(activatorBase, false);
    if (!ref) {
        logger::error("CreatePersistentActivator: Failed to place activator");
        return nullptr;
    }

    //  logger::info("CreatePersistentActivator: Created activator at initial location");
    return ref.get();  // raw pointer for reuse
}

bool MoveActivatorRandomly(RE::TESObjectREFR* caster, RE::TESObjectREFR* activatorRef) {
    if (!caster || !activatorRef) {
        //    logger::error("MoveActivatorRandomly: Missing caster or activatorRef");
        return false;
    }

    const auto casterPos = caster->GetPosition();

    float posX = casterPos.x;
    float posY = casterPos.y;
    float posZ = casterPos.z + globals::fHeight;

    activatorRef->SetPosition(posX, posY, posZ);

    //   logger::info("MoveActivatorRandomly: Moved to X={}, Y={}, Z={}", posX, posY, posZ);
    return true;
}

void LightningStrike() {

    if (!globals::formsInitialized) {
        logger::error("LightingRod Forms Not Intiialized Cannot Work"); 
        return;
    }

        auto player = RE::PlayerCharacter::GetSingleton();

        auto ui = RE::UI::GetSingleton();

        if (ui && ui->GameIsPaused()) {
            return; 
        }

        auto processLists = RE::ProcessLists::GetSingleton();

        RE::Actor* closestNPC = nullptr;
        float closestDistance = FLT_MAX;

        if (processLists) {
            for (auto& handle : processLists->highActorHandles) {
                auto actor = handle.get();
                if (!actor || actor->IsDead() || !actor->IsHostileToActor(player) || actor->IsPlayer()) continue;

                float distance = player->GetPosition().GetDistance(actor->GetPosition());
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestNPC = actor.get();
                }
            }
        }

        auto playerActor = player->As<RE::Actor>();
        auto playerState = playerActor ? playerActor->AsActorState() : nullptr;

        auto playerZHeight = player->GetPositionZ();

        float strikeChance = globals::g_StrikeChance;

        if (globals::TakeAltitudeIntoConsideration) {
            if (playerZHeight >= 20000.0f) {
                strikeChance += 20.0f;
            }
            else if (playerZHeight >= 15000.0f) {
                strikeChance += 15.0f;
            }
            else if (playerZHeight >= 10000.0f) {
                strikeChance += 10.0f;
            }
            else if (playerZHeight >= 5000.0f) {
                strikeChance += 5.0f;
            }
        }

        bool playerStrike = ShouldTriggerLightning(strikeChance, playerActor);
        bool npcStrike = ShouldTriggerLightning(strikeChance, closestNPC);

        if (npcStrike) {
            auto npcValueOwner = closestNPC->AsActorValueOwner();

                SKSE::GetTaskInterface()->AddTask(
                    [player, closestNPC, npcValueOwner]() {
                        if (!player || !closestNPC || !npcValueOwner) {
                            logger::warn("Lightning task skipped: null pointer in captured variables.");
                            return;
                        }

                        auto lightningActivator = CreatePersistentActivator(player, globals::activatorBaseObject);
                        if (!lightningActivator) {
                            logger::error("Failed to create activator for NPC");
                            return;
                        }

                        if (MoveActivatorRandomly(player, lightningActivator) &&
                            CastSpell(lightningActivator, globals::lightningSpell, closestNPC)) {
                            if (closestNPC->IsInWater()) {
                                npcValueOwner->RestoreActorValue(RE::ActorValue::kHealth, globals::g_waterDamageMultiplier);
                            }

                            lightningActivator->Disable();
                            lightningActivator->SetDelete(true);
                        }
                    });
        }

        if (playerStrike) {
            SKSE::GetTaskInterface()->AddTask(
                [player]() {
                    if (!player) {
                        logger::warn("Lightning task skipped: null pointer in captured in skse variables.");
                        return;
                    }

                    auto playerValueOwner = player->AsActorValueOwner();

                    if (!playerValueOwner) return; 

                    auto lightningActivator = CreatePersistentActivator(player, globals::activatorBaseObject);
                    if (!lightningActivator) {
                        logger::error("Failed to create activator for player");
                        return;
                    }

                    if (MoveActivatorRandomly(player, lightningActivator) &&
                        CastSpell(lightningActivator, globals::lightningSpell, player)) {
                        if (player->IsInWater()) {
                            playerValueOwner->RestoreActorValue(RE::ActorValue::kHealth, globals::g_waterDamageMultiplier);
                        }

                        lightningActivator->Disable();
                        lightningActivator->SetDelete(true);
                    }
                });
        }

        return; 
}
