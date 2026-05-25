#pragma once

#include "profile/ProfileSlot.hpp"

#include <array>
#include <cstddef>

class ProfileManager {
public:
    ProfileManager();

    const std::array<ProfileSlot, 3>& slots() const;

    ProfileData& selectSlot(std::size_t index);
    const ProfileData* selectedProfile() const;
    ProfileData* selectedProfile();

    std::size_t selectedSlotIndex() const;

private:
    std::array<ProfileSlot, 3> slots_{};
    std::size_t selectedSlotIndex_ = 0;
    bool hasSelectedSlot_ = false;
};
