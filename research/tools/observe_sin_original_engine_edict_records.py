#!/usr/bin/env python3
"""Read-only, owner-invoked original 2006 PE32 engine edict RECORD observer.

Phase 1649: optional, explicitly indexed, strictly bounded readback of three
source-authenticated DWORDs from each chosen 0x14-byte edict:
  +0x00 flags, +0x04 network serial, +0x10 x87 reuse-time float bits.
Never read +0x08 networkable or +0x0C IServerUnknown pointer, and NEVER
follow entity pointers. Does not open commercial DLLs in --self-test.

This is sampled, unpinned process memory. Two matching reads do NOT prove
atomicity, the absence of an intervening ABA, or actual owner lifetime.
"""
import argparse
from dataclasses import dataclass
import os
import sys
import time

from observe_sin_original_engine_edict_owner import (
    Descriptor, ENGINE_SHA, IMAGE_BASE, ORIGINAL_EDICT_STRIDE,
    ORIGINAL_GLOBALS, Win32Reader, read_descriptor,
)

FIELD_OFFSETS = (0x00, 0x04, 0x10)
MAX_INDICES_PER_SAMPLE = 4


@dataclass(frozen=True)
class EdictFields:
    index: int
    flags: int
    serial: int
    reuse_time_bits: int


@dataclass(frozen=True)
class RecordBatch:
    owner: Descriptor
    records: tuple = ()
    status: str = "unreadable"

    @property
    def double_checked_unpinned(self):
        return self.status == "double_checked_unpinned"


def _bounded_index(index, owner):
    return (
        isinstance(index, int)
        and not isinstance(index, bool)
        and 0 <= index < owner.current_count
        and index < owner.capacity
        and 0 <= owner.edict_base + index * ORIGINAL_EDICT_STRIDE
        and owner.edict_base +
        index * ORIGINAL_EDICT_STRIDE + FIELD_OFFSETS[-1] + 4
        <= 0x100000000
    )


def read_records(read_dword, module_base, indices):
    """Read globals, twice read only three DWORDs per index, globals again.

    read_dword is injected for portable original-free tests or bound to the
    Windows PROCESS_VM_READ-only callback. No pointer-following is possible.
    A matched pair of scans is called double_checked_unpinned, NEVER pinned.
    """
    before = read_descriptor(read_dword, module_base)
    if not before.plausible:
        return RecordBatch(before, status="owner_not_plausible")
    if (not indices or len(indices) > MAX_INDICES_PER_SAMPLE
            or len(set(indices)) != len(indices)
            or any(not _bounded_index(i, before) for i in indices)):
        return RecordBatch(before, status="invalid_or_unallocated_index")

    def read_one_pass():
        found = []
        for index in indices:
            base = before.edict_base + index * ORIGINAL_EDICT_STRIDE
            fields = tuple(read_dword(base + offset)
                           for offset in FIELD_OFFSETS)
            if any(not isinstance(value, int)
                   or not 0 <= value <= 0xFFFFFFFF
                   for value in fields):
                raise ValueError("non-DWORD remote edict field")
            found.append(EdictFields(index, *fields))
        return tuple(found)

    try:
        first = read_one_pass()
        second = read_one_pass()
    except (OSError, KeyError, ValueError):
        return RecordBatch(before, status="edict_read_unavailable")

    after = read_descriptor(read_dword, module_base)
    if not after.plausible or before != after:
        return RecordBatch(before, status="owner_changed_or_unreadable")
    if first != second:
        return RecordBatch(before, status="edict_fields_changed_during_read")
    # Still no mutual exclusion with retail original engine. The underlying
    # edict may be destroyed/reused between reads without either scan seeing
    # that fact; the caller must never treat this as physical pin evidence.
    return RecordBatch(before, first, "double_checked_unpinned")


def observed_transitions(batches):
    """Count only adjacent stable-read observations of the SAME base/index.

    Differences are observations, not proof of exactly one engine transition.
    Inconclusive samples, moved base, and changed capacity break adjacency.
    """
    serial_changes = 0
    flag_changes = 0
    time_changes = 0
    comparable_pairs = 0
    previous = None
    for batch in batches:
        if not batch.double_checked_unpinned:
            previous = None
            continue
        if previous is not None and (
                previous.owner.edict_base == batch.owner.edict_base
                and previous.owner.capacity == batch.owner.capacity):
            older = {row.index: row for row in previous.records}
            for row in batch.records:
                original = older.get(row.index)
                if original is None:
                    continue
                comparable_pairs += 1
                serial_changes += int(original.serial != row.serial)
                flag_changes += int(original.flags != row.flags)
                time_changes += int(original.reuse_time_bits != row.reuse_time_bits)
        previous = batch
    return (comparable_pairs, serial_changes, flag_changes, time_changes)


def make_fixture():
    loaded = 0x25000000
    edict_base = 0x11000000
    global_values = {
        loaded + ORIGINAL_GLOBALS["protected_last"] - IMAGE_BASE: 1,
        loaded + ORIGINAL_GLOBALS["current_count"] - IMAGE_BASE: 4,
        loaded + ORIGINAL_GLOBALS["capacity"] - IMAGE_BASE: 6,
        loaded + ORIGINAL_GLOBALS["edict_base"] - IMAGE_BASE: edict_base,
    }
    memory = dict(global_values)
    for index, flags, serial, time_bits in (
            (2, 2, 0xFFFFFFFF, 0x40800000),
            (3, 0, 7, 0x3F800000)):
        base = edict_base + index * ORIGINAL_EDICT_STRIDE
        for offset, value in zip(FIELD_OFFSETS, (flags, serial, time_bits)):
            memory[base + offset] = value
    return loaded, memory


def self_test():
    assert FIELD_OFFSETS == (0, 4, 0x10)
    assert ORIGINAL_EDICT_STRIDE == 0x14 and MAX_INDICES_PER_SAMPLE == 4
    assert len(ENGINE_SHA) == 64 and IMAGE_BASE == 0x20000000
    loaded, memory = make_fixture()
    seen = []
    def read(addr):
        seen.append(addr)
        return memory[addr]

    valid = read_records(read, loaded, [2, 3])
    assert valid.double_checked_unpinned
    assert valid.owner == Descriptor(0x11000000, 4, 6, 1, "plausible")
    assert valid.records == (
        EdictFields(2, 2, 0xFFFFFFFF, 0x40800000),
        EdictFields(3, 0, 7, 0x3F800000))
    # Exactly two 4-global passes and two 3-DWORD passes for each record.
    assert len(seen) == 8 + 2 * len(FIELD_OFFSETS) * 2
    forbidden = {
        valid.owner.edict_base + i * ORIGINAL_EDICT_STRIDE + offset
        for i in (2, 3) for offset in (0x08, 0x0C)}
    assert not forbidden.intersection(seen)

    for indices in ([], [0, 1, 2, 3, 4], [2, 2], [-1], [4],
                    [0x100000000], [True], ["2"]):
        earlier = len(seen)
        bad = read_records(read, loaded, indices)
        assert bad.status == "invalid_or_unallocated_index"
        assert len(seen) == earlier + 4  # only four descriptor globals
    assert read_records(read, loaded + 0x500000000, [2]).status == (
        "owner_not_plausible")

    # A hostile remote read cannot leak a partial field set as a valid sample.
    del memory[valid.owner.edict_base + 2 * ORIGINAL_EDICT_STRIDE + 0x10]
    assert read_records(read, loaded, [2]).status == "edict_read_unavailable"
    loaded, memory = make_fixture()

    # Simulate the real engine moving its edict table mid-scan, even when
    # both consecutive edict field passes look identical in the old table.
    base_global = loaded + ORIGINAL_GLOBALS["edict_base"] - IMAGE_BASE
    read_count = [0]
    def owner_changed(addr):
        if addr == base_global:
            read_count[0] += 1
            if read_count[0] == 2:
                return 0x12000000
        return memory[addr]
    assert read_records(owner_changed, loaded, [2]).status == (
        "owner_changed_or_unreadable")

    # A field mutates between first and second scan, even if the global
    # descriptor looks byte-for-byte identical.
    watched = 0x11000000 + 2 * ORIGINAL_EDICT_STRIDE + 0x04
    read_count = [0]
    def record_changed(addr):
        if addr == watched:
            read_count[0] += 1
            if read_count[0] == 2:
                return 0
        return memory[addr]
    assert read_records(record_changed, loaded, [2]).status == (
        "edict_fields_changed_during_read")

    first = read_records(memory.__getitem__, loaded, [2])
    second = read_records(memory.__getitem__, loaded, [2])
    assert observed_transitions([first, second]) == (1, 0, 0, 0)
    changed = dict(memory)
    changed[watched] = 0
    changed[0x11000000 + 2 * ORIGINAL_EDICT_STRIDE + 0] = 0
    changed[0x11000000 + 2 * ORIGINAL_EDICT_STRIDE + 0x10] = 0
    third = read_records(changed.__getitem__, loaded, [2])
    assert observed_transitions([first, third]) == (1, 1, 1, 1)
    assert observed_transitions([first, RecordBatch(first.owner), third]) == (
        0, 0, 0, 0)
    relocated = dict(memory)
    moved_owner = 0x11001000
    relocated[base_global] = moved_owner
    for offset in FIELD_OFFSETS:
        relocated[moved_owner + 2 * ORIGINAL_EDICT_STRIDE + offset] = (
            memory[0x11000000 + 2 * ORIGINAL_EDICT_STRIDE + offset])
    fourth = read_records(relocated.__getitem__, loaded, [2])
    assert fourth.double_checked_unpinned
    assert observed_transitions([first, fourth]) == (0, 0, 0, 0)

    # Count/capacity and extent guards must reject before any record read.
    inconsistent = dict(memory)
    inconsistent[loaded + ORIGINAL_GLOBALS["current_count"] - IMAGE_BASE] = 7
    assert read_records(inconsistent.__getitem__, loaded, [2]).status == (
        "owner_not_plausible")
    overflow = dict(memory)
    overflow[base_global] = 0xFFFFFFF0
    assert read_records(overflow.__getitem__, loaded, [2]).status == (
        "owner_not_plausible")

    print("PASS original-free bounded 2006 edict record observer: 0x14 stride, 3 DWORD fields only, 2-pass readback, array move/partial reads, serial/flag/time changes and invalid-index refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--pid", type=int,
                       help="explicit PID of the owner's unmodified SinEpisodes.exe")
    parser.add_argument("--owner-read-only-confirmed", action="store_true",
                        help="required for live retail read-only sampling")
    parser.add_argument("--edict-index", type=int, action="append",
                        help="specific 0-based edict index to sample; repeat up to 4")
    parser.add_argument("--samples", type=int, default=40,
                        help="read-only samples, 1..2000")
    parser.add_argument("--interval-ms", type=int, default=100,
                        help="milliseconds between samples, 20..2000")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.owner_read_only_confirmed:
        parser.error("live observation requires --owner-read-only-confirmed")
    if os.name != "nt":
        parser.error("live observation requires Windows")
    if args.pid is None or args.pid <= 0:
        parser.error("valid explicit owner game PID required")
    if (not args.edict_index or
            len(args.edict_index) > MAX_INDICES_PER_SAMPLE or
            len(set(args.edict_index)) != len(args.edict_index) or
            any(not 0 <= n < 65536 for n in args.edict_index)):
        parser.error("provide 1..4 distinct --edict-index values within 0..65535")
    if not 1 <= args.samples <= 2000 or not 20 <= args.interval_ms <= 2000:
        parser.error("samples or interval outside bounded read-only limits")

    try:
        with Win32Reader(args.pid) as reader:
            print("ORIGINAL_ENGINE_SHA256_VERIFIED=true")
            print("LOADED_ENGINE_ALLOCATOR_CODE_WITNESSES_BEGIN=10")
            print("OWNER_EDICT_FIELD_OFFSETS=0x00,0x04,0x10")
            print("OWNER_EDICT_NETWORKABLE_UNKNOWN_POINTERS_READ=false")
            observations = []
            for number in range(args.samples):
                result = read_records(
                    reader.read_dword, reader.engine_base, args.edict_index)
                observations.append(result)
                print(f"SAMPLE {number + 1} state={result.status}"
                      f" count={result.owner.current_count}"
                      f" capacity={result.owner.capacity}",flush=True)
                if result.double_checked_unpinned:
                    for edict in result.records:
                        print(f"  EDICT index={edict.index}"
                              f" flags=0x{edict.flags:08x}"
                              f" serial=0x{edict.serial:08x}"
                              f" reuse_time_bits=0x{edict.reuse_time_bits:08x}",
                              flush=True)
                if number + 1 < args.samples:
                    time.sleep(args.interval_ms / 1000)
            # The entire observation window is bracketed by independently
            # checked, rebased in-process code witnesses (still unpinned).
            reader.attest_loaded_allocator_code()
            print("LOADED_ENGINE_ALLOCATOR_CODE_WITNESSES_END=10")
            pairs, serial, flags, reuse_time = observed_transitions(observations)
            print(f"OWNER_EDICT_ADJACENT_COMPARABLE_PAIRS={pairs}")
            print(f"OWNER_EDICT_OBSERVED_SERIAL_CHANGES={serial}")
            print(f"OWNER_EDICT_OBSERVED_FLAG_CHANGES={flags}")
            print(f"OWNER_EDICT_OBSERVED_REUSE_TIME_CHANGES={reuse_time}")
            print("OWNER_EDICT_CONTINUOUS_PHYSICAL_PIN_PROVEN=false")
            print("OWNER_EDICT_ATOMIC_SNAPSHOT_PROVEN=false")
            print("OWNER_EDICT_RETAIL_ALLOCATOR_HOOK_INSTALLED=false")
            return 0 if any(x.double_checked_unpinned for x in observations) else 2
    except (OSError, RuntimeError, ValueError) as error:
        print(f"OWNER_EDICT_RECORD_OBSERVER_REFUSED={type(error).__name__}: {error}",
              file=sys.stderr)
        return 3


if __name__ == "__main__":
    sys.exit(main())
