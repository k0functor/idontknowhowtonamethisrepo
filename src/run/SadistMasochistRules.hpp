#pragma once

#include <string>

class CombatState;
struct GameEvent;

namespace SadistMasochistRules {
inline constexpr const char* MechanicId = "sadist_masochist_party";
inline constexpr const char* SadistActorDefinitionId = "sadist";
inline constexpr const char* MasochistActorDefinitionId = "masochist";
inline constexpr const char* PleasureStatusId = "sadist_pleasure";
inline constexpr const char* PainStatusId = "masochist_pain";

bool appliesTo(const std::string& mechanicId);
void handleEvent(CombatState& state, const GameEvent& event);
} // namespace SadistMasochistRules
