#pragma once

namespace globals {

	inline bool formsInitialized = false; 

	inline bool currentCellIsInterior = false;

	inline bool lastCellWasInterior = false;

	inline constexpr float fHeight = 1000.0f;

	inline float g_StrikeChance = 5.0f;
	inline float g_LightningDamageMultiplier = 80.0f;
	inline float g_waterDamageMultiplier = 500.0f;

	inline bool TakeAltitudeIntoConsideration = true;
	inline float playerAltitude = 0.0f;


	inline RE::SpellItem* lightningSpell = nullptr;
	inline RE::EffectSetting* shockDamageEffect = nullptr;
	inline RE::TESBoundObject* activatorBaseObject = nullptr;

	inline std::vector<RE::BGSKeyword*> keywords;

}

 