#pragma once

enum class CardPlayFailureReason {
    None,
    NoPlayerActor,
    NotPlayerTurn,
    CardNotInHand,
    InvalidCardSource,
    CardSourceDefeated,
    WrongActorTurn,
    WrongActorForCard,
    NotEnoughEnergy,
    NotEnoughStress,
    UnplayableKeyword
};
