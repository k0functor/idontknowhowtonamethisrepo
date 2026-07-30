#pragma once

#include "challenges/ChallengeDefinition.hpp"
#include "data/ContentRegistry.hpp"
#include "localization/LocalizationManager.hpp"
#include "profile/ProfileData.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class ChallengeScene final : public Scene {
public:
    ChallengeScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const ContentRegistry& content,
        std::vector<const ChallengeDefinition*> challenges,
        const ProfileData* profile,
        bool hasExistingRunSave,
        std::function<void(std::string)> onStartChallenge,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    enum class ChallengeFilter {
        All,
        Available,
        Completed,
        Locked
    };

private:
    void moveSelection(int direction);
    void selectIndex(std::size_t index);
    void setFilter(ChallengeFilter filter);
    void rebuildFilteredChallenges();
    bool matchesFilter(const ChallengeDefinition& challenge) const;
    std::size_t firstVisibleIndex(std::size_t visibleRows) const;
    void requestStartSelected();
    void confirmPendingStart();
    void cancelPendingStart();
    const ChallengeDefinition* selectedChallenge() const;
    bool isSelectedCompleted() const;
    bool canStartSelected() const;
    bool isCompleted(const ChallengeDefinition& challenge) const;
    bool isUnlocked(const ChallengeDefinition& challenge) const;
    std::string filterText(ChallengeFilter filter) const;
    std::string statusText(const ChallengeDefinition& challenge) const;
    std::string lockText(const ChallengeDefinition& challenge) const;
    std::vector<std::string> loadoutLines(const ChallengeDefinition& challenge) const;
    std::string contentNameOrId(const std::string& contentId, const std::string& type) const;
    std::string summarizedContentList(const std::vector<std::string>& contentIds, const std::string& type) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const ContentRegistry& content_;
    std::vector<const ChallengeDefinition*> challenges_;
    const ProfileData* profile_ = nullptr;
    bool hasExistingRunSave_ = false;
    std::function<void(std::string)> onStartChallenge_;
    std::function<void()> onBack_;
    ChallengeFilter filter_ = ChallengeFilter::All;
    std::vector<std::size_t> filteredChallengeIndices_;
    std::size_t selectedIndex_ = 0;
    float detailsScroll_ = 0.f;
    std::string pendingStartChallengeId_;
};
