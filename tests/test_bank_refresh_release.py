from __future__ import annotations

import configparser
import hashlib
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods/bank-refresh"
RELEASE = ROOT / "release-assets/2.03.02"


def test_bank_refresh_sealed_release() -> None:
    report = json.loads((ROOT / "reports/BUILD_REPORT_BANK_REFRESH_1.1.0.json").read_text())
    assert report["mod_version"] == "1.1.0"
    assert report["steam_build"] == "25474236"
    assert report["diagnostic_logging"] is False
    sums = {name: digest for digest, name in (line.split() for line in (RELEASE / "SHA256SUMS.txt").read_text().splitlines())}
    asi = report["asi"]["file"]
    binary = (MOD / asi).read_bytes()
    assert binary[:2] == b"MZ"
    assert b"BankRefresh_TestInitResult" not in binary
    assert "1.1.0".encode("utf-16le") in binary
    assert "bonds-test1".encode("utf-16le") not in binary
    assert "Bank_Refresh.log".encode("utf-16le") not in binary
    assert hashlib.sha256(binary).hexdigest() == report["asi"]["sha256"] == sums[asi]
    package = RELEASE / report["package"]["file"]
    assert hashlib.sha256(package.read_bytes()).hexdigest() == report["package"]["sha256"] == sums[package.name]
    with zipfile.ZipFile(package) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == {asi, "Bank_Refresh.ini", "README.txt", "CHANGELOG.txt", "LICENSE.txt", "MinHook-LICENSE.txt"}
        assert archive.read(asi) == binary
        for packaged, source in {
            "Bank_Refresh.ini": "Bank_Refresh.ini",
            "README.txt": "README.md",
            "CHANGELOG.txt": "CHANGELOG.txt",
            "LICENSE.txt": "LICENSE.txt",
            "MinHook-LICENSE.txt": "src/vendor/minhook/LICENSE.txt",
        }.items():
            assert archive.read(packaged).decode("utf-8") == (MOD / source).read_text(encoding="utf-8-sig")


def test_bank_refresh_config_and_provenance() -> None:
    config = configparser.ConfigParser()
    config.read(MOD / "Bank_Refresh.ini")
    assert set(config["BankRefresh"]) == {"enabled", "intervalgameminutes"}
    assert config.getint("BankRefresh", "Enabled") == 1
    assert config.getint("BankRefresh", "IntervalGameMinutes") == 15
    assert set(config.sections()) == {"BankRefresh", "Bonds"}
    assert set(config["Bonds"]) == {"enabled", "intervalgameminutes"}
    assert config.getint("Bonds", "Enabled") == 1
    assert config.getint("Bonds", "IntervalGameMinutes") == 15
    report = json.loads((ROOT / "reports/BUILD_REPORT_BANK_REFRESH_1.1.0.json").read_text())
    assert report["default_game_minutes"] == 15
    assert report["default_bond_game_minutes"] == 15
    assert report["automatic_bond_reinvestment"] is False
    for relative, digest in report["sources"].items():
        assert not Path(relative).is_absolute()
        assert hashlib.sha256((ROOT / relative).read_text(encoding="utf-8-sig").encode("utf-8")).hexdigest() == digest
    source = (MOD / "src/bank_refresh.cpp").read_text()
    for removed in ("open_shared_log", "logFile", "fprintf", "void push(", "for(;;)"):
        assert removed not in source
    assert "std::strcmp(hash,ExpectedSha)" in source
    assert "caller!=(bonds ? BondTimerReturnRva : TimerReturnRva)" in source
    assert "eligible(name,kind,type)" in source
    assert "originalBonds : original" in source
    assert "bondsEnabled" in source and "hooksReady" in source
