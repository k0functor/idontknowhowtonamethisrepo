[CmdletBinding()]
param(
    [string]$ConfigurePreset = "gcc-release",
    [string]$BuildPreset = "",
    [string]$ExecutableName = "i_dont_know_how_to_name_this_game.exe",
    [string]$PackageName = "idontknowhowtonamethisrepo-windows-x64",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message"
}

function Invoke-CheckedCommand {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
    }
}

function Get-ProjectRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-BinaryDirectoryFromPreset {
    param(
        [string]$ProjectRoot,
        [string]$PresetName
    )

    $presetPath = Join-Path $ProjectRoot "CMakePresets.json"
    if (-not (Test-Path $presetPath)) {
        throw "CMakePresets.json was not found at: $presetPath"
    }

    $presetJson = Get-Content $presetPath -Raw | ConvertFrom-Json
    $preset = @($presetJson.configurePresets) | Where-Object { $_.name -eq $PresetName } | Select-Object -First 1
    if ($null -eq $preset) {
        throw "Configure preset '$PresetName' was not found in CMakePresets.json"
    }

    $binaryDir = [string]$preset.binaryDir
    if ([string]::IsNullOrWhiteSpace($binaryDir)) {
        throw "Configure preset '$PresetName' does not define binaryDir"
    }

    return $binaryDir.Replace('${sourceDir}', $ProjectRoot)
}

function Copy-DirectoryFresh {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        return
    }

    if (Test-Path $Destination) {
        Remove-Item $Destination -Recurse -Force
    }

    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item $Source $Destination -Recurse -Force
}

function Test-IsSystemDll {
    param([string]$DllName)

    $name = $DllName.ToLowerInvariant()
    if ($name.StartsWith("api-ms-win-")) { return $true }
    if ($name.StartsWith("ext-ms-win-")) { return $true }

    $systemDlls = @(
        "advapi32.dll",
        "bcrypt.dll",
        "cfgmgr32.dll",
        "comdlg32.dll",
        "crypt32.dll",
        "dwmapi.dll",
        "gdi32.dll",
        "gdi32full.dll",
        "imm32.dll",
        "kernel32.dll",
        "kernelbase.dll",
        "msvcrt.dll",
        "msvcp_win.dll",
        "ntdll.dll",
        "ole32.dll",
        "oleaut32.dll",
        "opengl32.dll",
        "rpcrt4.dll",
        "sechost.dll",
        "setupapi.dll",
        "shell32.dll",
        "shlwapi.dll",
        "ucrtbase.dll",
        "user32.dll",
        "version.dll",
        "winmm.dll",
        "winspool.drv",
        "ws2_32.dll"
    )

    return $systemDlls -contains $name
}

function Get-DllNamesFromObjdump {
    param([string]$ExePath)

    $objdump = Get-Command objdump -ErrorAction SilentlyContinue
    if ($null -eq $objdump) {
        return @()
    }

    $output = & $objdump.Source -p $ExePath 2>$null
    if ($LASTEXITCODE -ne 0) {
        return @()
    }

    $names = foreach ($line in $output) {
        if ($line -match "DLL Name:\s*(.+)$") {
            $matches[1].Trim()
        }
    }

    return @($names | Sort-Object -Unique)
}

function Find-DllFile {
    param(
        [string]$DllName,
        [string[]]$ExtraDirectories
    )

    foreach ($directory in $ExtraDirectories) {
        if ([string]::IsNullOrWhiteSpace($directory)) { continue }
        $candidate = Join-Path $directory $DllName
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    foreach ($directory in ($env:PATH -split ';')) {
        if ([string]::IsNullOrWhiteSpace($directory)) { continue }
        $candidate = Join-Path $directory $DllName
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Copy-RuntimeDlls {
    param(
        [string]$ExePath,
        [string]$PackageDir
    )

    $exeDir = Split-Path -Parent $ExePath
    $detectedDlls = @(Get-DllNamesFromObjdump -ExePath $ExePath | Where-Object { -not (Test-IsSystemDll $_) })
    $usedObjdump = $detectedDlls.Count -gt 0

    if (-not $usedObjdump) {
        Write-Warning "objdump did not provide DLL dependencies. Falling back to common MinGW/raylib DLL names."
        $detectedDlls = @(
            "libgcc_s_seh-1.dll",
            "libstdc++-6.dll",
            "libwinpthread-1.dll",
            "raylib.dll"
        )
    }

    $copied = New-Object System.Collections.Generic.List[string]
    $searchDirs = @(
        $exeDir,
        (Join-Path (Split-Path -Parent $exeDir) "bin")
    )

    foreach ($dllName in $detectedDlls) {
        $dllPath = Find-DllFile -DllName $dllName -ExtraDirectories $searchDirs
        if ($null -eq $dllPath) {
            if ($usedObjdump) {
                throw "Required runtime DLL was not found on PATH or near the executable: $dllName"
            }

            Write-Warning "Optional/common DLL was not found and was not copied: $dllName"
            continue
        }

        Copy-Item $dllPath (Join-Path $PackageDir $dllName) -Force
        $copied.Add($dllName) | Out-Null
    }

    $localDlls = Get-ChildItem $exeDir -Filter "*.dll" -File -ErrorAction SilentlyContinue
    foreach ($dll in $localDlls) {
        Copy-Item $dll.FullName (Join-Path $PackageDir $dll.Name) -Force
        if (-not $copied.Contains($dll.Name)) {
            $copied.Add($dll.Name) | Out-Null
        }
    }

    return @($copied | Sort-Object -Unique)
}

function Write-PackageReadme {
    param(
        [string]$PackageDir,
        [string]$ExecutableName
    )

    $readme = @"
IDontKnowHowToNameThisGame - Windows portable build

How to run:
1. Extract the whole ZIP archive into a separate folder.
2. Run $ExecutableName from that extracted folder.
3. Do not move the EXE away from config/, data/, assets/ and saves/.

If Windows SmartScreen warns about an unknown publisher, use:
More info -> Run anyway.
This happens because the build is not code-signed yet.

If the game does not start, keep the whole extracted folder intact and send the developer a screenshot of the error.
If Windows shows 0xc000007b, the build is probably mixing incompatible 32-bit/64-bit DLLs. Do not add random DLLs from the internet; rebuild and repackage from the same MSYS2 UCRT64 environment.
"@

    Set-Content -Path (Join-Path $PackageDir "README.txt") -Value $readme -Encoding UTF8
}

$projectRoot = Get-ProjectRoot
if ([string]::IsNullOrWhiteSpace($BuildPreset)) {
    $BuildPreset = $ConfigurePreset
}

$buildDir = Get-BinaryDirectoryFromPreset -ProjectRoot $projectRoot -PresetName $ConfigurePreset
$binDir = Join-Path $buildDir "bin"
$exePath = Join-Path $binDir $ExecutableName
$distRoot = Join-Path $projectRoot "dist"
$packageDir = Join-Path $distRoot $PackageName
$zipPath = Join-Path $distRoot "$PackageName.zip"

Push-Location $projectRoot
try {
    if (-not $SkipBuild) {
        Write-Step "Configuring CMake preset '$ConfigurePreset'"
        Invoke-CheckedCommand -FilePath "cmake" -Arguments @(
            "--preset", $ConfigurePreset,
            "-DGAME_STATIC_MINGW_RUNTIME=ON",
            "-DGAME_FORCE_FETCH_RAYLIB=ON",
            "-DBUILD_SHARED_LIBS=OFF"
        )

        Write-Step "Building CMake preset '$BuildPreset'"
        Invoke-CheckedCommand -FilePath "cmake" -Arguments @("--build", "--preset", $BuildPreset)
    }

    if (-not (Test-Path $exePath)) {
        throw "Executable was not found after build: $exePath"
    }

    Write-Step "Creating package directory"
    if (Test-Path $packageDir) {
        Remove-Item $packageDir -Recurse -Force
    }
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

    Write-Step "Copying executable"
    Copy-Item $exePath (Join-Path $packageDir $ExecutableName) -Force

    Write-Step "Copying runtime data"
    Copy-DirectoryFresh -Source (Join-Path $projectRoot "config") -Destination (Join-Path $packageDir "config")
    Copy-DirectoryFresh -Source (Join-Path $projectRoot "data") -Destination (Join-Path $packageDir "data")
    Copy-DirectoryFresh -Source (Join-Path $projectRoot "assets") -Destination (Join-Path $packageDir "assets")

    $packageSavesDir = Join-Path $packageDir "saves"
    New-Item -ItemType Directory -Path $packageSavesDir -Force | Out-Null
    $settingsPath = Join-Path $projectRoot "saves/settings.json"
    if (Test-Path $settingsPath) {
        Copy-Item $settingsPath (Join-Path $packageSavesDir "settings.json") -Force
    }

    Write-Step "Copying required DLLs"
    $copiedDlls = @(Copy-RuntimeDlls -ExePath $exePath -PackageDir $packageDir)

    Write-Step "Writing README.txt"
    Write-PackageReadme -PackageDir $packageDir -ExecutableName $ExecutableName

    Write-Step "Creating ZIP archive"
    Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath -Force

    Write-Host ""
    Write-Host "Portable package created: $zipPath"
    if ($copiedDlls.Count -gt 0) {
        Write-Host "Copied DLLs: $($copiedDlls -join ', ')"
    } else {
        Write-Host "Copied DLLs: none detected or needed"
    }
    Write-Host "Test the ZIP on a machine without MSYS2 before sending it to players. Obviously."
}
finally {
    Pop-Location
}
