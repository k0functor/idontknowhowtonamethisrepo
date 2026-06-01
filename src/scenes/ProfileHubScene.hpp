#pragma once

#include "actors/PlayerActorDatabase.hpp"
#include "archetypes/PlayableArchetypeDefinition.hpp"
#include "data/CardDatabase.hpp"
#include "localization/LocalizationManager.hpp"
#include "relics/RelicDatabase.hpp"
#include "scenes/Scene.hpp"
#include "ui/UiFont.hpp"

#include <functional>
#include <vector>

class ProfileHubScene final : public Scene {
public:
    ProfileHubScene(
        const UiFont& font,
        const LocalizationManager& localization,
        const PlayerActorDatabase& actors,
        const CardDatabase& cards,
        const RelicDatabase& relics,
        std::vector<const PlayableArchetypeDefinition*> archetypes,
        bool hasSavedRun,
        std::function<void(PlayableArchetypeId)> onStartRun,
        std::function<void()> onContinueRun,
        std::function<void()> onBack
    );

    void update(float deltaSeconds) override;
    void render() const override;

private:
    void moveSelection(int direction);
    const PlayableArchetypeDefinition& selectedArchetype() const;

    void updateDetailsModal();
    void renderDetailsModal() const;
    float detailsContentHeight(Rectangle contentBounds) const;
    float renderDetailsContent(Rectangle contentBounds, float scrollY, bool draw) const;

    std::string cardName(const std::string& cardId) const;
    std::string actorName(const std::string& actorId) const;
    std::string relicName(const std::string& relicId) const;

private:
    const UiFont& font_;
    const LocalizationManager& localization_;
    const PlayerActorDatabase& actors_;
    const CardDatabase& cards_;
    const RelicDatabase& relics_;
    std::vector<const PlayableArchetypeDefinition*> archetypes_;
    bool hasSavedRun_ = false;
    std::function<void(PlayableArchetypeId)> onStartRun_;
    std::function<void()> onContinueRun_;
    std::function<void()> onBack_;

    std::size_t selectedIndex_ = 0;
    bool detailsOpen_ = false;
    float detailsScrollY_ = 0.f;
    mutable std::string notification_;
};
