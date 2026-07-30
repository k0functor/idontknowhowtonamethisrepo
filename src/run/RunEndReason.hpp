#pragma once

enum class RunEndReason {
    Victory,
    Defeat,
    Abandoned,
    ChallengeCompleted,
    ChallengeFailed
};

inline bool runEndReasonCountsAsVictory(const RunEndReason reason) {
    return reason == RunEndReason::Victory || reason == RunEndReason::ChallengeCompleted;
}

inline bool runEndReasonCountsAsDefeat(const RunEndReason reason) {
    return reason == RunEndReason::Defeat ||
        reason == RunEndReason::Abandoned ||
        reason == RunEndReason::ChallengeFailed;
}
