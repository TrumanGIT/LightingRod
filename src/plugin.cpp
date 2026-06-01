#include <spdlog/sinks/basic_file_sink.h>

#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include "Functions.h"
#include "eventSink.h"

namespace logger = SKSE::log;



void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
     globals::formsInitialized = Initialize(); 
     EventSinks::PlayerCellEvent::RegisterEventSink(); 
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();


    //TODO:: make a ini parser function
   std::ifstream iniFile("Data\\SKSE\\Plugins\\LightningRod.ini");
    if (!iniFile.is_open()) {
        logger::warn("INI file not found or failed to open.");
    } else {
        std::string line;
        while (std::getline(iniFile, line)) {
            if (line.starts_with("fStrikeChance=")) {
                try {
                    globals::g_StrikeChance = std::stof(line.substr(std::string("fStrikeChance=").length()));
                } catch (...) {
                    logger::warn("Failed to parse fStrikeChance from INI.");
                }
            } else if (line.starts_with("fLightningDamageMultiplier=")) {
                try {
                    globals::g_LightningDamageMultiplier =
                        std::stof(line.substr(std::string("fLightningDamageMultiplier=").length()));
                } catch (...) {
                    logger::warn("Failed to parse fLightningDamageMultiplier from INI.");
                }
            } else if (line.starts_with("fWaterDamageMultiplier=")) {
                try {
                    globals::g_waterDamageMultiplier = std::stof(line.substr(std::string("fWaterDamageMultiplier=").length()));
                } catch (...) {
                    logger::warn("Failed to parse fWaterDamageMultiplier from INI.");
                }
            } else if (line.starts_with("bTakeAltitudeIntoConsideration=")) {
                auto valueStr = line.substr(std::string("bTakeAltitudeIntoConsideration=").length());
                std::transform(valueStr.begin(), valueStr.end(), valueStr.begin(), ::tolower);
                if (valueStr == "1" || valueStr == "true") {
                    globals::TakeAltitudeIntoConsideration = true;
                } else if (valueStr == "0" || valueStr == "false") {
                    globals::TakeAltitudeIntoConsideration = false;
                } else {
                    logger::warn("Failed to parse bTakeAltitudeIntoConsideration from INI. Using default = true.");
                }
            }
        }
    }
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}