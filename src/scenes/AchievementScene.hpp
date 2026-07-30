#pragma once

#include "achievements/AchievementDefinition.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileData.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class AchievementScene final : public Scene {
public:
    AchievementScene(
        const UiFont& font,
        const LocalizationManager& localization,
        std::vector<const AchievementDefinition*> achievements,
        const ProfileData* profile,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class AchievementFilter {
        All,
        Available,
        Completed,
        Locked
    };

private:
    void moveSelection(int direction);
    void pageSelection(int direction);
    void setFilter(AchievementFilter filter);
    void rebuildFilteredAchievements();
    bool matchesFilter(const AchievementDefinition& achievement) const;
    const AchievementDefinition* selectedAchievement() const;
    std::size_t firstVisibleIndex(std::size_t visibleRows) const;
    bool isCompleted(const AchievementDefinition& achievement) const;
    bool isUnlocked(const AchievementDefinition& achievement) const;
    std::string filterText(AchievementFilter filter) const;
    std::string statusText(const AchievementDefinition& achievement) const;
    std::string lockText(const AchievementDefinition& achievement) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    std::vector<const AchievementDefinition*> achievements_;
    const ProfileData* profile_ = nullptr;
    std::function<void()> onBack_;
    AchievementFilter filter_ = AchievementFilter::All;
    std::vector<std::size_t> filteredAchievementIndices_;
    std::size_t selectedIndex_ = 0;
};
