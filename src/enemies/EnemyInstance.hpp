#pragma once

#include "enemies/EnemyDefinition.hpp"
#include "entities/CombatEntity.hpp"

inline CombatEntity makeEnemyEntity(
    const EnemyDefinition& definition,
    const EntityId entityId
) {
    CombatEntity entity;
    entity.id = entityId;
    entity.type = EntityType::Enemy;
    entity.definitionId = definition.id.value;
    entity.nameTextId = definition.nameTextId;
    entity.health = Health(definition.maxHp);
    entity.block = definition.startingBlock;
    return entity;
}
