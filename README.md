# I Don't Know How To Name This Game

## English

### About the game

A dark card-based roguelike about building a party, shaping powerful decks, and surviving a five-floor journey through increasingly hostile encounters.

Choose from seven distinct archetypes, each with its own mechanics, cards, relic synergies, and playstyle. Manage health, energy, stress, consumables, active items, and long-term deck growth while navigating branching maps filled with battles, elites, shops, events, and bosses.

Stress is a dangerous resource: most characters must carefully control and spend it through cards, relics, and other effects, while one archetype can deliberately thrive on the edge of a breakdown. Other characters revolve around mechanics such as combat stances, deployable drones, poison, economy, or controlling two fighters in the same battle.

Runs feature hundreds of cards and encounters, dozens of relics and events, multiple bosses across five floors, and three possible final bosses. The focus is on build variety, tactical combat, and risky decisions that can turn a barely surviving deck into a powerful engine.

### AI usage

AI tools were used as an auxiliary development aid for some code-related questions, debugging, code review, and discussion of implementation approaches. The game design, project direction, integration, testing, balancing decisions, and final responsibility for the code remain with the developer.

### Building the project

The project is intended to be built in **Release** mode. The tested Windows toolchain is GCC from MSYS2 UCRT64 with CMake and Ninja.

From the project root, configure and build with:

```powershell
cmake --preset gcc-release
cmake --build --preset gcc-release
```

The executable is produced at:

```text
build/gcc-release/bin/i_dont_know_how_to_name_this_game.exe
```

Run it with:

```powershell
.\build\gcc-release\bin\i_dont_know_how_to_name_this_game.exe
```

To create the validated portable Windows release package, run:

```powershell
pwsh -NoProfile -File .\tools\package_windows_release.ps1
```

---

## Русский

### Об игре

Мрачный карточный roguelike о построении группы, развитии колоды и прохождении пяти этажей, наполненных всё более опасными противниками.

Игроку доступны семь различных архетипов со своими механиками, картами, синергиями реликвий и стилями игры. В течение забега нужно управлять здоровьем, энергией, стрессом, расходниками, активными предметами и развитием колоды, выбирая путь по разветвлённым картам с боями, элитами, магазинами, событиями и боссами.

Стресс является опасным ресурсом: большинство персонажей должны контролировать и расходовать его с помощью карт, реликвий и других эффектов, тогда как один из архетипов способен намеренно играть на грани нервного срыва. Другие персонажи строятся вокруг стоек, дронов, яда, экономики или управления сразу двумя бойцами в одном бою.

В игре представлены сотни карт и встреч, десятки реликвий и событий, несколько боссов на протяжении пяти этажей и три возможных финальных босса. Основной акцент сделан на разнообразии билдов, тактических боях и рискованных решениях, способных превратить едва выживающую колоду в мощную систему взаимодействий.

### Использование ИИ

Инструменты на основе ИИ использовались как вспомогательное средство для отдельных вопросов, связанных с кодом, отладкой, ревью и обсуждением вариантов реализации. Игровой дизайн, направление проекта, интеграция изменений, тестирование, решения по балансу и итоговая ответственность за код остаются за разработчиком.

### Сборка проекта

Проект предполагается собирать в режиме **Release**. Проверенная конфигурация под Windows использует GCC из MSYS2 UCRT64, CMake и Ninja.

Из корня проекта выполните:

```powershell
cmake --preset gcc-release
cmake --build --preset gcc-release
```

Исполняемый файл будет создан по пути:

```text
build/gcc-release/bin/i_dont_know_how_to_name_this_game.exe
```

Запуск:

```powershell
.\build\gcc-release\bin\i_dont_know_how_to_name_this_game.exe
```

Для создания проверенного portable-архива Windows используйте:

```powershell
pwsh -NoProfile -File .\tools\package_windows_release.ps1
```
