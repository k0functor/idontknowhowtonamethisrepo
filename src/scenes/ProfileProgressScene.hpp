#pragma once

#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "localization/TextId.hpp"
#include "profile/ProfileData.hpp"
#include "profile/ProfileProgressEntry.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <string>
#include <vector>

class ProfileProgressScene final : public Scene {
public:
    ProfileProgressScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const ContentRegistry& content,
        const ProfileData* profile,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class ViewMode {
        Journal,
        Statistics
    };

    struct StatisticEntry {
        TextId labelTextId;
        TextId descriptionTextId;
        int value = 0;
    };

private:
    void setViewMode(ViewMode mode);
    void rebuildStatistics();
    void moveSelection(int direction);
    void pageSelection(int direction);
    const ProfileProgressEntry* selectedEntry() const;
    const StatisticEntry* selectedStatistic() const;
    std::size_t visibleEntryCount() const;
    std::size_t firstVisibleIndex(std::size_t visibleRows) const;
    std::string typeLabel(const ProfileProgressEntry& entry) const;
    std::string entryName(const ProfileProgressEntry& entry) const;
    std::string entryDescription(const ProfileProgressEntry& entry) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const ContentRegistry& content_;
    const ProfileData* profile_ = nullptr;
    std::function<void()> onBack_;
    std::vector<ProfileProgressEntry> entries_;
    std::vector<StatisticEntry> statistics_;
    ViewMode viewMode_ = ViewMode::Journal;
    std::size_t selectedIndex_ = 0;
};
