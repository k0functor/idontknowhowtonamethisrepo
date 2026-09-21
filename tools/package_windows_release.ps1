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

function Get-CMakeCacheValue {
    param(
        [string]$BuildDirectory,
        [string]$Name
    )

    $cachePath = Join-Path $BuildDirectory "CMakeCache.txt"
    if (-not (Test-Path $cachePath)) {
        throw "CMake cache was not found after configuration: $cachePath"
    }

    $line = Get-Content $cachePath | Where-Object { $_ -match "^$([regex]::Escape($Name)):[^=]+=" } | Select-Object -First 1
    if ($null -eq $line) {
        throw "CMake cache variable '$Name' was not found in $cachePath"
    }

    return ($line -split "=", 2)[1].Trim()
}

function Assert-CMakeCacheBool {
    param(
        [string]$BuildDirectory,
        [string]$Name,
        [bool]$Expected
    )

    $actualValue = (Get-CMakeCacheValue -BuildDirectory $BuildDirectory -Name $Name).ToUpperInvariant()
    $actual = switch ($actualValue) {
        { $_ -in @("1", "ON", "TRUE", "YES", "Y") } { $true; break }
        { $_ -in @("0", "OFF", "FALSE", "NO", "N") } { $false; break }
        default { throw "CMake cache variable '$Name' has unsupported boolean value '$actualValue'" }
    }
    if ($actual -ne $Expected) {
        throw "CMake cache variable '$Name' is '$actualValue'; expected '$Expected'"
    }
}

function Assert-StaticRuntimeDependencies {
    param([string]$ExePath)

    $objdump = Get-Command objdump -ErrorAction SilentlyContinue
    if ($null -eq $objdump) {
        throw "objdump is required to verify release runtime dependencies"
    }

    $dllNames = @(Get-DllNamesFromObjdump -ExePath $ExePath)
    if ($dllNames.Count -eq 0) {
        throw "objdump did not report DLL dependencies for: $ExePath"
    }

    $nonSystemDlls = @($dllNames | Where-Object { -not (Test-IsSystemDll $_) })
    if ($nonSystemDlls.Count -gt 0) {
        throw "Release executable still depends on non-system DLLs: $($nonSystemDlls -join ', ')"
    }
}

function Write-ReleaseConfig {
    param([string]$ConfigPath)

    if (-not (Test-Path $ConfigPath)) {
        throw "Packaged app config was not found: $ConfigPath"
    }

    $config = Get-Content $ConfigPath -Raw | ConvertFrom-Json
    if ($null -eq $config.debug) {
        $config | Add-Member -NotePropertyName "debug" -NotePropertyValue ([pscustomobject]@{})
    }
    if ($null -eq $config.debug.PSObject.Properties["enabled"]) {
        $config.debug | Add-Member -NotePropertyName "enabled" -NotePropertyValue $false
    } else {
        $config.debug.enabled = $false
    }

    $config | ConvertTo-Json -Depth 32 | Set-Content -Path $ConfigPath -Encoding UTF8
}

function Invoke-ReleasePackageValidator {
    param(
        [string]$ProjectRoot,
        [string]$PackagePath,
        [string]$ExecutableName
    )

    $validatorPath = Join-Path $ProjectRoot "tools/validate_release_package.py"
    if (-not (Test-Path $validatorPath)) {
        throw "Release package validator was not found: $validatorPath"
    }

    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($null -eq $python) {
        throw "Python 3 is required to validate release packages"
    }

    Invoke-CheckedCommand -FilePath $python.Source -Arguments @(
        $validatorPath,
        $PackagePath,
        "--executable", $ExecutableName
    )
}

function Invoke-ContestReadinessValidator {
    param([string]$ProjectRoot)

    $validatorPath = Join-Path $ProjectRoot "tools/validate_contest_readiness.py"
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($null -eq $python) {
        throw "Python 3 is required to validate contest readiness"
    }

    Invoke-CheckedCommand -FilePath $python.Source -Arguments @($validatorPath)
}

function Invoke-PackagedSmokeTest {
    param(
        [string]$PackageDir,
        [string]$ExecutableName
    )

    Push-Location $PackageDir
    try {
        Invoke-CheckedCommand -FilePath (Join-Path $PackageDir $ExecutableName) -Arguments @("--smoke-test")
    } finally {
        Pop-Location
        foreach ($runtimeDir in @("saves", "logs")) {
            $path = Join-Path $PackageDir $runtimeDir
            if (Test-Path $path) {
                Remove-Item $path -Recurse -Force
            }
        }
    }
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
3. Keep the EXE next to config/, data/ and assets/. The saves/ directory is created on first launch.

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
    Write-Step "Validating contest readiness"
    Invoke-ContestReadinessValidator -ProjectRoot $projectRoot

    if (-not $SkipBuild) {
        Write-Step "Configuring CMake preset '$ConfigurePreset'"
        Invoke-CheckedCommand -FilePath "cmake" -Arguments @(
            "--preset", $ConfigurePreset,
            "-DGAME_STATIC_LINK_LIBRARIES=ON",
            "-DGAME_FORCE_FETCH_RAYLIB=ON",
            "-DBUILD_SHARED_LIBS=OFF"
        )

        Write-Step "Building CMake preset '$BuildPreset'"
        Invoke-CheckedCommand -FilePath "cmake" -Arguments @("--build", "--preset", $BuildPreset)
    }

    Assert-CMakeCacheBool -BuildDirectory $buildDir -Name "GAME_STATIC_LINK_LIBRARIES" -Expected $true
    Assert-CMakeCacheBool -BuildDirectory $buildDir -Name "BUILD_SHARED_LIBS" -Expected $false

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
    Write-ReleaseConfig -ConfigPath (Join-Path $packageDir "config/app.json")

    Write-Step "Verifying static runtime dependencies"
    Assert-StaticRuntimeDependencies -ExePath $exePath
    Write-Step "Writing README.txt"
    Write-PackageReadme -PackageDir $packageDir -ExecutableName $ExecutableName

    Write-Step "Smoke-starting packaged executable"
    Invoke-PackagedSmokeTest -PackageDir $packageDir -ExecutableName $ExecutableName

    Write-Step "Validating package directory"
    Invoke-ReleasePackageValidator -ProjectRoot $projectRoot -PackagePath $packageDir -ExecutableName $ExecutableName

    Write-Step "Creating ZIP archive"
    Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath -Force

    Write-Step "Validating ZIP archive"
    Invoke-ReleasePackageValidator -ProjectRoot $projectRoot -PackagePath $zipPath -ExecutableName $ExecutableName

    Write-Host ""
    Write-Host "Portable package created: $zipPath"
    Write-Host "Runtime DLLs: only Windows system libraries detected"
    Write-Host "Test the ZIP on a machine without MSYS2 before sending it to players. Obviously."
}
finally {
    Pop-Location
}
