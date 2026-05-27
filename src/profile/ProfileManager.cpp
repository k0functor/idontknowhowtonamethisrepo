#include "ProfileManager.hpp"

#include <stdexcept>

ProfileManager::ProfileManager() {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].index = static_cast<int>(i);
        slots_[i].data.slotIndex = static_cast<int>(i);
        slots_[i].data.isEmpty = true;
    }
}

const std::array<ProfileSlot, 3>& ProfileManager::slots() const {
    return slots_;
}

ProfileData& ProfileManager::selectSlot(const std::size_t index) {
    if (index >= slots_.size()) {
        throw std::runtime_error("Profile slot index is out of range");
    }

    selectedSlotIndex_ = index;
    hasSelectedSlot_ = true;

    ProfileData& profile = slots_[index].data;
    profile.isEmpty = false;

    if (profile.unlockedArchetypeIds.empty()) {
        profile.unlockedArchetypeIds.push_back("rusted_knight");
        profile.unlockedArchetypeIds.push_back("herbalist");
        profile.unlockedArchetypeIds.push_back("sadist_masochist");
    }

    return profile;
}

const ProfileData* ProfileManager::selectedProfile() const {
    if (!hasSelectedSlot_) {
        return nullptr;
    }

    return &slots_[selectedSlotIndex_].data;
}

ProfileData* ProfileManager::selectedProfile() {
    if (!hasSelectedSlot_) {
        return nullptr;
    }

    return &slots_[selectedSlotIndex_].data;
}

std::size_t ProfileManager::selectedSlotIndex() const {
    return selectedSlotIndex_;
}
