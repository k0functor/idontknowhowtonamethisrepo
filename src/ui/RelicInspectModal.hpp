#pragma once

#include "ui/RelicViewModel.hpp"
#include "ui/UiFont.hpp"
#include "localization/LocalizationManager.hpp"

#include <cstddef>
#include <optional>
#include <vector>

class RelicInspectModal {
public:
    void open(std::size_t index);
    void close();

    bool isOpen() const;
    std::optional<std::size_t> currentIndex() const;

    void update(std::size_t relicCount);
    void render(
        const UiFont& font,
        const LocalizationManager& localization,
        const std::vector<RelicViewModel>& relics
    ) const;

private:
    void previous(std::size_t relicCount);
    void next(std::size_t relicCount);

private:
    std::optional<std::size_t> currentIndex_;
};
