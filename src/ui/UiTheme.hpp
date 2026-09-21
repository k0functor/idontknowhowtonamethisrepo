#pragma once

#include "enemies/EnemyIntent.hpp"
#include "ui/StatusViewModel.hpp"

#include <raylib.h>

#include <algorithm>

namespace UiTheme {
enum class Tone {
    Neutral,
    Primary,
    Accent,
    Positive,
    Warning,
    Danger,
    Info,
    Disabled
};

inline constexpr Color canvas{18, 20, 26, 255};
inline constexpr Color topBar{25, 28, 36, 255};
inline constexpr Color panel{31, 34, 44, 248};
inline constexpr Color panelRaised{40, 44, 56, 252};
inline constexpr Color panelInset{24, 27, 35, 255};
inline constexpr Color border{92, 101, 126, 255};
inline constexpr Color borderSoft{60, 66, 82, 255};
inline constexpr Color textPrimary{240, 242, 248, 255};
inline constexpr Color textSecondary{195, 201, 218, 255};
inline constexpr Color textMuted{139, 147, 170, 255};
inline constexpr Color shadow{0, 0, 0, 72};

inline constexpr float panelRoundness = 0.08f;
inline constexpr float controlRoundness = 0.18f;
inline constexpr float chipRoundness = 0.32f;
inline constexpr float borderThickness = 2.f;

inline Color withAlpha(const Color color, const float opacity) {
    const float clamped = std::clamp(opacity, 0.f, 1.f);
    return Color{
        color.r,
        color.g,
        color.b,
        static_cast<unsigned char>(static_cast<float>(color.a) * clamped)
    };
}

inline Color toneFill(const Tone tone) {
    switch (tone) {
        case Tone::Primary: return Color{50, 63, 92, 255};
        case Tone::Accent: return Color{78, 61, 35, 255};
        case Tone::Positive: return Color{38, 76, 57, 255};
        case Tone::Warning: return Color{90, 65, 31, 255};
        case Tone::Danger: return Color{88, 40, 45, 255};
        case Tone::Info: return Color{36, 65, 82, 255};
        case Tone::Disabled: return Color{36, 38, 46, 255};
        case Tone::Neutral: return panelRaised;
    }

    return panelRaised;
}

inline Color toneHoverFill(const Tone tone) {
    switch (tone) {
        case Tone::Primary: return Color{65, 82, 119, 255};
        case Tone::Accent: return Color{108, 82, 42, 255};
        case Tone::Positive: return Color{48, 101, 73, 255};
        case Tone::Warning: return Color{119, 84, 37, 255};
        case Tone::Danger: return Color{119, 51, 58, 255};
        case Tone::Info: return Color{45, 86, 108, 255};
        case Tone::Disabled: return Color{36, 38, 46, 255};
        case Tone::Neutral: return Color{55, 60, 74, 255};
    }

    return Color{55, 60, 74, 255};
}

inline Color toneBorder(const Tone tone) {
    switch (tone) {
        case Tone::Primary: return Color{145, 168, 225, 255};
        case Tone::Accent: return Color{236, 196, 86, 255};
        case Tone::Positive: return Color{116, 224, 157, 255};
        case Tone::Warning: return Color{244, 187, 89, 255};
        case Tone::Danger: return Color{244, 116, 127, 255};
        case Tone::Info: return Color{118, 200, 235, 255};
        case Tone::Disabled: return Color{83, 88, 103, 255};
        case Tone::Neutral: return border;
    }

    return border;
}

inline Color toneText(const Tone tone) {
    switch (tone) {
        case Tone::Accent: return Color{252, 232, 174, 255};
        case Tone::Positive: return Color{204, 247, 218, 255};
        case Tone::Warning: return Color{255, 225, 166, 255};
        case Tone::Danger: return Color{255, 211, 216, 255};
        case Tone::Info: return Color{210, 238, 249, 255};
        case Tone::Disabled: return textMuted;
        case Tone::Primary:
        case Tone::Neutral:
            return textPrimary;
    }

    return textPrimary;
}

inline Color stressBand(const int bandIndex) {
    switch (bandIndex) {
        case 0: return Color{105, 178, 162, 255};
        case 1: return Color{210, 190, 92, 255};
        case 2: return Color{226, 150, 72, 255};
        case 3: return Color{222, 92, 76, 255};
        case 4: return Color{190, 62, 104, 255};
        case 5: return Color{115, 42, 58, 255};
        default: return Color{180, 150, 190, 255};
    }
}

inline Color statusFill(const StatusViewModel& status) {
    if (status.debuff) {
        return Color{88, 42, 51, 238};
    }
    if (status.buff) {
        return Color{38, 77, 57, 238};
    }
    return Color{52, 57, 70, 238};
}

inline Color statusBorder(const StatusViewModel& status) {
    if (status.debuff) {
        return Color{236, 112, 126, 255};
    }
    if (status.buff) {
        return Color{120, 226, 154, 255};
    }
    return Color{176, 184, 208, 255};
}

inline Color intentFill(const EnemyIntentType type) {
    switch (type) {
        case EnemyIntentType::Attack: return Color{96, 40, 43, 250};
        case EnemyIntentType::Block: return Color{34, 66, 96, 250};
        case EnemyIntentType::Buff: return Color{38, 78, 56, 250};
        case EnemyIntentType::Debuff: return Color{80, 45, 94, 250};
        case EnemyIntentType::Special: return Color{91, 69, 34, 250};
        case EnemyIntentType::Unknown: return Color{51, 55, 67, 250};
    }

    return Color{51, 55, 67, 250};
}

inline Color intentBorder(const EnemyIntentType type) {
    switch (type) {
        case EnemyIntentType::Attack: return Color{255, 135, 122, 255};
        case EnemyIntentType::Block: return Color{128, 202, 255, 255};
        case EnemyIntentType::Buff: return Color{125, 232, 156, 255};
        case EnemyIntentType::Debuff: return Color{220, 150, 240, 255};
        case EnemyIntentType::Special: return Color{255, 218, 116, 255};
        case EnemyIntentType::Unknown: return Color{190, 194, 205, 255};
    }

    return Color{190, 194, 205, 255};
}
}
