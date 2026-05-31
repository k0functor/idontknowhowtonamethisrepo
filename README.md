# IDontKnowHowToNameThisGame

## Windows portable package

To create a Windows build that other people can run without compiling the project themselves, use the packaging script from PowerShell:

```powershell
.\tools\package_windows_release.ps1
```

The script uses the `gcc-release` CMake preset, builds the game, creates `dist/idontknowhowtonamethisrepo-windows-x64/`, copies the executable, runtime data, assets, default settings and required DLL files, then creates:

```text
dist/idontknowhowtonamethisrepo-windows-x64.zip
```

Send this ZIP archive to testers. They should extract the whole folder and run:

```text
i_dont_know_how_to_name_this_game.exe
```

The executable should stay next to `config/`, `data/`, `assets/` and `saves/`. Moving only the `.exe` somewhere else will break file loading, because apparently executables need their organs nearby.

Useful options:

```powershell
.\tools\package_windows_release.ps1 -SkipBuild
.\tools\package_windows_release.ps1 -PackageName "my-game-test-build"
.\tools\package_windows_release.ps1 -ConfigurePreset gcc-release -BuildPreset gcc-release
```

Before sending the archive to other people, test it by extracting the ZIP into a new folder outside the repository and launching the game from there.
