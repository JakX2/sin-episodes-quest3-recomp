#!/usr/bin/env python3
"""Offline, fail-closed client Init dependency-gate regression matrix.

This is an independent synthetic state-machine test, NOT original engine execution.
It records the reviewed Phase 538-540 ordering constraints without fabricating
unknown original C++ ABI signatures or calling original DLL code.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from dataclasses import dataclass

REQUIRED_VGUI = ("VGUI_InputInternal001", "VGUI_System010")
REQUIRED_GAME_SYSTEMS = 12
GATES = ("engine_interfaces", "material_derived", "game_systems",
         "vgui_client_prerequisite", "vgui_input", "vgui_system",
         "vgui_final_virtual_result")
NONFATAL = ("late_client_object_allocation",)


@dataclass(frozen=True)
class Result:
    success: bool
    failed_gate: str | None
    visited: tuple[str, ...]


def simulate(gates: dict[str, bool], late_allocation: bool = True) -> Result:
    """A deliberately conservative abstract model, not a native Init emulator."""
    if set(gates) != set(GATES):
        raise ValueError("exact gate set required; no missing or unknown gates")
    if any(type(value) is not bool for value in gates.values()):
        raise TypeError("gate values must be booleans")
    if type(late_allocation) is not bool:
        raise TypeError("late_allocation must be boolean")
    visited = []
    for name in GATES:
        visited.append(name)
        if not gates[name]:
            return Result(False, name, tuple(visited))
    return Result(True, None, tuple(visited))


def report() -> dict:
    baseline = dict.fromkeys(GATES, True)
    cases = [{"case": "all_gates_pass", "success": True, "failed_gate": None},
             {"case": "late_allocation_fails", "success": True, "failed_gate": None}]
    for name in GATES:
        altered = dict(baseline)
        altered[name] = False
        outcome = simulate(altered)
        cases.append({"case": "reject_" + name, "success": outcome.success,
                      "failed_gate": outcome.failed_gate})
    canonical = json.dumps(cases, sort_keys=True, separators=(",", ":")).encode()
    return {"schema": 1, "evidence": "synthetic-only",
            "source_phases": [538, 539, 540],
            "required_vgui_interfaces": list(REQUIRED_VGUI),
            "game_system_registration_calls": REQUIRED_GAME_SYSTEMS,
            "nonfatal_observations": list(NONFATAL),
            "cases": cases, "cases_sha256": hashlib.sha256(canonical).hexdigest()}


def self_test() -> None:
    baseline = dict.fromkeys(GATES, True)
    assert simulate(baseline).success
    assert simulate(baseline, late_allocation=False).success
    for index, name in enumerate(GATES):
        altered = dict(baseline)
        altered[name] = False
        result = simulate(altered)
        assert result == Result(False, name, GATES[:index + 1])
    for invalid in ({}, {**baseline, "unreviewed": True}):
        try:
            simulate(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("fail-open gate schema")
    for invalid in (None, 1, "false", [], {}):
        altered = dict(baseline)
        altered[GATES[0]] = invalid
        try:
            simulate(altered)
        except TypeError:
            pass
        else:
            raise AssertionError("fail-open gate type")
    first = json.dumps(report(), sort_keys=True)
    assert first == json.dumps(report(), sort_keys=True)
    assert len(report()["cases"]) == len(GATES) + 2
    print("PASS: synthetic client Init gate matrix, fail-closed schema, deterministic report")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(report(), sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
