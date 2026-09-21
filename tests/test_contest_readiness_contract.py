#!/usr/bin/env python3
"""Contract for the final contest/release readiness gate."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def main() -> int:
    config = json.loads(text("config/app.json"))
    assert config["debug"]["enabled"] is False

    main_cpp = text("src/main.cpp")
    assert "--smoke-test" in main_cpp
    assert "Application app;" in main_cpp

    menu = text("src/scenes/MainMenuScene.cpp")
    assert "ui.credits_later" not in menu
    assert "localization_.get(TextId(\"ui.exit\"))" in menu

    package_script = text("tools/package_windows_release.ps1")
    assert "Invoke-ContestReadinessValidator" in package_script
    assert "Invoke-PackagedSmokeTest" in package_script
    assert 'Arguments @("--smoke-test")' in package_script
    assert '@("saves", "logs")' in package_script
    assert package_script.index("Smoke-starting packaged executable") < package_script.index("Validating package directory")

    cmake = text("CMakeLists.txt")
    assert "validate_contest_readiness.py" in cmake
    assert "contest_readiness_contract" in cmake

    ci = text(".github/workflows/ci.yml")
    assert "--smoke-test" in ci
    assert "timeout 5s" not in ci

    completed = subprocess.run(
        [sys.executable, str(ROOT / "tools/validate_contest_readiness.py")],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise AssertionError(completed.stdout + completed.stderr)

    print("Contest readiness contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
