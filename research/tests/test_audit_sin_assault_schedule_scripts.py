"""Synthetic 2006 SiN assault-script fixtures, no original game text or DLL."""
import importlib.util
import re
from pathlib import Path
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "assault_schedule_scripts", TOOLS / "audit_sin_assault_schedule_scripts.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def fake_script(name, tasks, interrupts):
    # Match the original Source AI schedule-script grammar, not original bytes.
    body = "\n\tSchedule\n\t\t" + name + "\n\tTasks\n"
    for task, param in tasks:
        body += "\t\t" + task + "\t" + param + "\n"
    body += "\tInterrupts\n"
    for condition in interrupts:
        body += "\t\t" + condition + "\n"
    return body


def original_synthetic_scripts():
    return {
        va: (fake_script(name, tasks, interrupts).encode("ascii") + b"\0").ljust(960, b"\0")
        for name, va, tasks, interrupts in a.SCHEDULE_SKELETONS
    }


class FakePE:
    def __init__(self, regions):
        self.regions = regions

    def at(self, va, count):
        if va not in self.regions or len(self.regions[va]) < count:
            raise a.VerificationError(f"missing synthetic original address {va:#x}")
        return self.regions[va][:count]


class OriginalAssaultScheduleScriptTests(unittest.TestCase):
    def test_all_nine_schedule_scripts_with_ordered_parameters_and_interrupts(self):
        result = a.audit_schedules(FakePE(original_synthetic_scripts()))
        self.assertEqual(result["schedule_count"], 9)
        self.assertEqual(result["total_tasks"], 29)
        self.assertEqual(result["total_interrupt_declarations"], 35)
        by_name = {r["schedule"]: r for r in result["schedule_skeletons"]}
        self.assertEqual([t["symbol"] for t in by_name["SCHED_MOVE_TO_RALLY_POINT"]["tasks"]][-1],
                         "TASK_SET_SCHEDULE")
        self.assertEqual(
            by_name["SCHED_MOVE_TO_RALLY_POINT"]["tasks"][-1]["parameter"],
            "SCHEDULE:SCHED_HOLD_RALLY_POINT")
        self.assertEqual(
            by_name["SCHED_MOVE_TO_ASSAULT_POINT"]["tasks"][-1]["symbol"],
            "TASK_HIT_ASSAULT_POINT")
        self.assertIn("COND_HEAR_BULLET_IMPACT",
                      by_name["SCHED_HOLD_ASSAULT_POINT"]["interrupt_conditions"])

    def test_rejects_order_mutation(self):
        regions = original_synthetic_scripts()
        name, va, tasks, conditions = a.SCHEDULE_SKELETONS[0]
        modified = list(tasks)
        modified[0], modified[1] = modified[1], modified[0]
        regions[va] = (fake_script(name, modified, conditions).encode() + b"\0").ljust(960, b"\0")
        with self.assertRaises(a.VerificationError):
            a.audit_schedules(FakePE(regions))

    def test_rejects_wrong_task_parameter(self):
        regions = original_synthetic_scripts()
        name, va, tasks, conditions = a.SCHEDULE_SKELETONS[4]
        changed = list(tasks)
        changed[-1] = (changed[-1][0], "4")
        regions[va] = (fake_script(name, changed, conditions).encode() + b"\0").ljust(960, b"\0")
        with self.assertRaises(a.VerificationError):
            a.audit_schedules(FakePE(regions))

    def test_rejects_interrupt_mutation(self):
        regions = original_synthetic_scripts()
        name, va, tasks, conditions = a.SCHEDULE_SKELETONS[2]
        changed = list(conditions)
        changed[-1] = "COND_NO_PRIMARY_AMMO"
        regions[va] = (fake_script(name, tasks, changed).encode() + b"\0").ljust(960, b"\0")
        with self.assertRaises(a.VerificationError):
            a.audit_schedules(FakePE(regions))

    def test_rejects_invalid_section_text_and_unknown_parameter(self):
        good = fake_script("SCHED_MOVE_TO_RALLY_POINT", [
            ("TASK_WAIT", "0")], ["COND_NEW_ENEMY"])
        with self.assertRaises(a.VerificationError):
            a.parse_schedule_string(good.replace("Interrupts", "Interrupted"))
        with self.assertRaises(a.VerificationError):
            a.parse_schedule_string(good.replace("TASK_WAIT\t0", "TASK_WAIT\tBAD_PARAM"))
        with self.assertRaises(a.VerificationError):
            a.parse_schedule_string(good + "unexpected trailer")

    def test_portable_cpp_task_witnesses_match_original_binary_audit(self):
        # Prevent accidental divergence between source-verified Python audit
        # and portable C++ metadata used by the future native AI scheduler.
        header = (Path(__file__).resolve().parents[2] /
                  "include/sin_compat/original_assault_tasks.hpp").read_text()
        cpp_enum = {
            "SCHED_MOVE_TO_RALLY_POINT": "MoveToRallyPoint",
            "SCHED_ASSAULT_FAILED_TO_MOVE": "AssaultFailedToMove",
            "SCHED_FAIL_MOVE_TO_RALLY_POINT": "FailMoveToRallyPoint",
            "SCHED_MOVE_TO_ASSAULT_POINT": "MoveToAssaultPoint",
            "SCHED_HOLD_RALLY_POINT": "HoldRallyPoint",
            "SCHED_HOLD_ASSAULT_POINT": "HoldAssaultPoint",
            "SCHED_WAIT_AND_CLEAR": "WaitAndClear",
            "SCHED_ASSAULT_MOVE_AWAY": "AssaultMoveAway",
            "SCHED_CLEAR_ASSAULT_POINT": "ClearAssaultPoint",
        }
        expected_tasks = []
        expected_interrupts = []
        for name, _, tasks, interrupts in a.SCHEDULE_SKELETONS:
            expected_tasks.extend((cpp_enum[name], i, symbol, param)
                                  for i, (symbol, param) in enumerate(tasks))
            expected_interrupts.extend((cpp_enum[name], condition)
                                       for condition in interrupts)
        task_re = re.compile(
            r'\{AssaultSchedule::(\w+),\s*(\d+),\s*"(TASK_[A-Z0-9_]+)",\s*"([^"]+)"\}')
        interrupt_re = re.compile(
            r'\{AssaultSchedule::(\w+),\s*"(COND_[A-Z0-9_]+)"\}')
        found_tasks = [(ename, int(index), task, param)
                       for ename, index, task, param in task_re.findall(header)]
        found_interrupts = interrupt_re.findall(header)
        self.assertEqual(found_tasks, expected_tasks)
        self.assertEqual(found_interrupts, expected_interrupts)

    def test_unknown_routine_rejects_original_hash(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"a synthetic file, not the 2006 DLL")


if __name__ == "__main__":
    unittest.main()
