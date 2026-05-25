from __future__ import annotations

import json
from pathlib import Path

ROOT = Path.cwd()
CMAKE = ROOT / "CMakeLists.txt"

FILES_TO_ADD = [
    "src/inspect/InspectEntry.hpp",
    "src/inspect/InspectPanelModel.hpp",
    "src/inspect/InspectModelBuilder.hpp",
    "src/inspect/InspectModelBuilder.cpp",
    "src/ui/InspectPanelView.hpp",
    "src/ui/InspectPanelView.cpp",
    "src/ui/RelicInspectModal.hpp",
    "src/ui/RelicInspectModal.cpp",
]

RU_KEYS = {
    "inspect.enemy.intent": "Намерение",
    "inspect.enemy.no_details.name": "Нет видимых эффектов",
    "inspect.enemy.no_details.description": "У этого врага сейчас нет видимых намерений или статусов.",
    "inspect.combat.block.name": "Защита",
    "inspect.combat.block.description": "Снижает входящий урон до потери здоровья.",
    "inspect.card.effect.name": "Эффект",
    "inspect.card.no_details.name": "Нет подробностей",
    "inspect.card.no_details.description": "У этой карты нет ключевых слов или связанных механик.",
    "inspect.status.unknown": "Описание статуса пока недоступно.",
    "inspect.effect.damage": "Наносит урон",
    "inspect.effect.block": "Даёт защиту",
    "inspect.effect.heal": "Лечит",
    "inspect.effect.draw_cards": "Добирает карты",
    "inspect.effect.discard_cards": "Сбрасывает карты",
    "inspect.effect.apply_status": "Накладывает статус",
    "inspect.effect.gain_energy": "Даёт энергию",
    "inspect.effect.gain_stress": "Добавляет стресс",
    "inspect.effect.lose_hp": "Теряет здоровье",
    "inspect.effect.unknown": "Эффект",
    "inspect.target.self": "себя",
    "inspect.target.single_enemy": "один враг",
    "inspect.target.all_enemies": "все враги",
    "inspect.target.random_enemy": "случайный враг",
    "inspect.target.ally": "союзник",
    "inspect.target.all_allies": "все союзники",
    "inspect.target.random_ally": "случайный союзник",
    "keyword.exhaust.name": "Сжигается",
    "keyword.exhaust.description": "После розыгрыша карта удаляется из боя и не возвращается в discard pile.",
    "keyword.retain.name": "Оставляется",
    "keyword.retain.description": "Эта карта не сбрасывается из руки в конце хода.",
    "keyword.ethereal.name": "Эфирная",
    "keyword.ethereal.description": "Если карта осталась в руке в конце хода, она сжигается.",
    "keyword.innate.name": "Врождённая",
    "keyword.innate.description": "Эта карта появляется в стартовой руке, если правила боя позволяют это.",
    "keyword.unplayable.name": "Нельзя сыграть",
    "keyword.unplayable.description": "Эту карту нельзя разыграть обычным способом.",
}

EN_KEYS = {
    "inspect.enemy.intent": "Intent",
    "inspect.enemy.no_details.name": "No visible effects",
    "inspect.enemy.no_details.description": "This enemy currently has no visible intent details or statuses.",
    "inspect.combat.block.name": "Block",
    "inspect.combat.block.description": "Prevents incoming damage before HP is lost.",
    "inspect.card.effect.name": "Effect",
    "inspect.card.no_details.name": "No details",
    "inspect.card.no_details.description": "This card has no keywords or linked mechanics.",
    "inspect.status.unknown": "No status description is available yet.",
    "inspect.effect.damage": "Deals damage",
    "inspect.effect.block": "Gains block",
    "inspect.effect.heal": "Heals",
    "inspect.effect.draw_cards": "Draws cards",
    "inspect.effect.discard_cards": "Discards cards",
    "inspect.effect.apply_status": "Applies status",
    "inspect.effect.gain_energy": "Gains energy",
    "inspect.effect.gain_stress": "Gains stress",
    "inspect.effect.lose_hp": "Loses HP",
    "inspect.effect.unknown": "Effect",
    "inspect.target.self": "self",
    "inspect.target.single_enemy": "single enemy",
    "inspect.target.all_enemies": "all enemies",
    "inspect.target.random_enemy": "random enemy",
    "inspect.target.ally": "ally",
    "inspect.target.all_allies": "all allies",
    "inspect.target.random_ally": "random ally",
    "keyword.exhaust.name": "Exhaust",
    "keyword.exhaust.description": "After being played, this card is removed from combat instead of going to the discard pile.",
    "keyword.retain.name": "Retain",
    "keyword.retain.description": "This card is not discarded from your hand at the end of turn.",
    "keyword.ethereal.name": "Ethereal",
    "keyword.ethereal.description": "If this card remains in your hand at the end of turn, it is exhausted.",
    "keyword.innate.name": "Innate",
    "keyword.innate.description": "This card appears in your opening hand if combat rules allow it.",
    "keyword.unplayable.name": "Unplayable",
    "keyword.unplayable.description": "This card cannot be played normally.",
}


def patch_cmake() -> None:
    if not CMAKE.exists():
        raise SystemExit("CMakeLists.txt not found. Run this script from the project root.")

    text = CMAKE.read_text(encoding="utf-8")
    if not (ROOT / "CMakeLists.txt.bak.inspect").exists():
        (ROOT / "CMakeLists.txt.bak.inspect").write_text(text, encoding="utf-8")

    lines = text.splitlines()
    lines = [
        line for line in lines
        if "src/scenes/DebugCombatScene.hpp" not in line
        and "src/scenes/DebugCombatScene.cpp" not in line
    ]
    text = "\n".join(lines) + "\n"

    missing = [path for path in FILES_TO_ADD if path not in text]
    if missing:
        anchor = "    src/ui/CombatView.cpp"
        if anchor not in text:
            anchor = "    src/scenes/CombatScene.hpp"
        insert = "\n".join(f"    {path}" for path in missing)
        text = text.replace(anchor, anchor + "\n" + insert)

    CMAKE.write_text(text, encoding="utf-8")


def merge_json(path: Path, values: dict[str, str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        data = json.loads(path.read_text(encoding="utf-8-sig"))
    else:
        data = {}

    changed = False
    for key, value in values.items():
        if key not in data:
            data[key] = value
            changed = True

    if changed:
        path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    patch_cmake()
    merge_json(ROOT / "data" / "localization" / "ru.json", RU_KEYS)
    merge_json(ROOT / "data" / "localization" / "en.json", EN_KEYS)
    print("Inspect system patch applied: CMake updated, localization keys merged.")


if __name__ == "__main__":
    main()
