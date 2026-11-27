#!/usr/bin/env python3
"""Read-only, owner-invoked original 2006 engine.dll edict-global observer.

Only accepts an actual SinEpisodes.exe process whose loaded engine.dll file
matches the SHA-256-pinned 2006 owner original. Uses PROCESS_VM_READ only.
NEVER injects, hooks, protects, changes or dumps game memory; reports the
four independently authenticated engine globals, not edict/entity contents.

--self-test is portable and original-free. Live mode requires Windows.
A plausible snapshot is NOT a lifetime pin or atomic allocator transaction.
"""
import argparse
import ctypes
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import sys
import time

from audit_sin_original_engine_gameents_consumer import ENGINE_SHA, IMAGE_BASE
from attest_sin_original_engine_live_allocator_code import verify_loaded_code

# Original engine.dll SHA-pinned instruction witnesses from the independent
# allocator/reuse audit. All values are original VAs, NOT process-relative
# addresses: subtract IMAGE_BASE and add the actual loaded engine module base.
ORIGINAL_GLOBALS = {
    "protected_last": 0x2079DA64,
    "current_count": 0x2079DAFC,
    "capacity": 0x2079DB00,
    "edict_base": 0x2079DB04,
}
ORIGINAL_EDICT_STRIDE = 0x14
MAX_DIAGNOSTIC_CAPACITY = 65536  # diagnostic safety cap, NOT a retail constant


@dataclass(frozen=True)
class Descriptor:
    edict_base: int = 0
    current_count: int = 0
    capacity: int = 0
    protected_last: int = 0
    classification: str = "unreadable"

    @property
    def plausible(self):
        return self.classification == "plausible"


def classify(edict_base, current_count, capacity, protected_last):
    """Classify four separately sampled DWORDs, without dereferencing any."""
    values = (edict_base, current_count, capacity, protected_last)
    if any(not isinstance(x, int) or x < 0 or x > 0xFFFFFFFF for x in values):
        return "invalid_dword"
    if edict_base == 0 and current_count == 0 and capacity == 0:
        return "not_initialized"
    if edict_base == 0 or capacity == 0:
        return "partial_initialization"
    if current_count > capacity:
        return "count_exceeds_capacity"
    if capacity > MAX_DIAGNOSTIC_CAPACITY:
        return "capacity_exceeds_observer_safety_limit"
    if edict_base & 3:
        return "unaligned_pointer"
    if edict_base + capacity * ORIGINAL_EDICT_STRIDE > 0x100000000:
        return "pointer_extent_overflow"
    if current_count == 0:
        return "no_used_edicts"
    # This may be a transient non-atomic read, not an engine failure.
    if protected_last >= current_count:
        return "protected_index_not_in_count"
    return "plausible"


def read_descriptor(read_dword, loaded_module_base):
    """Injectable reader used by live ReadProcessMemory and offline selftest."""
    try:
        if loaded_module_base <= 0 or loaded_module_base > 0xFFFFFFFF:
            raise ValueError("invalid image base")
        fields = {}
        for role, original_va in ORIGINAL_GLOBALS.items():
            address = loaded_module_base + original_va - IMAGE_BASE
            if address < 0 or address > 0xFFFFFFFF - 4:
                raise ValueError("PE32 relocated global overflows address space")
            fields[role] = read_dword(address)
        return Descriptor(
            edict_base=fields["edict_base"],
            current_count=fields["current_count"],
            capacity=fields["capacity"],
            protected_last=fields["protected_last"],
            classification=classify(fields["edict_base"],
                                    fields["current_count"],
                                    fields["capacity"],
                                    fields["protected_last"]),
        )
    except (OSError, KeyError, ValueError):
        return Descriptor()


def summary(descriptors):
    """Only adjacent plausible samples can establish an observed pointer move."""
    plausible = 0
    observed_changes = 0
    previous = None
    for desc in descriptors:
        if not desc.plausible:
            previous = None
            continue
        plausible += 1
        if previous is not None and previous.edict_base != desc.edict_base:
            observed_changes += 1
        previous = desc
    return plausible, observed_changes


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


class Win32Reader:
    """The only live mode. All process rights are read/query, never write."""
    PROCESS_VM_READ = 0x0010
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    TH32CS_SNAPMODULE = 0x00000008
    TH32CS_SNAPMODULE32 = 0x00000010

    def __init__(self, pid):
        if os.name != "nt":
            raise RuntimeError("live owner observation requires Windows")
        if not 1 <= pid <= 0xFFFFFFFF:
            raise ValueError("invalid owner process ID")
        from ctypes import wintypes

        self.wintypes = wintypes
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        k = self.kernel
        k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL,
                                  wintypes.DWORD]
        k.OpenProcess.restype = wintypes.HANDLE
        k.CloseHandle.argtypes = [wintypes.HANDLE]
        k.CloseHandle.restype = wintypes.BOOL
        k.QueryFullProcessImageNameW.argtypes = [
            wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR,
            ctypes.POINTER(wintypes.DWORD)]
        k.QueryFullProcessImageNameW.restype = wintypes.BOOL
        k.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
        k.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
        k.Module32FirstW.argtypes = [wintypes.HANDLE, ctypes.c_void_p]
        k.Module32FirstW.restype = wintypes.BOOL
        k.Module32NextW.argtypes = [wintypes.HANDLE, ctypes.c_void_p]
        k.Module32NextW.restype = wintypes.BOOL
        k.ReadProcessMemory.argtypes = [
            wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
            ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        k.ReadProcessMemory.restype = wintypes.BOOL

        self.handle = k.OpenProcess(
            self.PROCESS_VM_READ | self.PROCESS_QUERY_LIMITED_INFORMATION,
            False, pid)
        if not self.handle:
            raise OSError(ctypes.get_last_error(),
                          "OpenProcess read/query denied for owner PID")
        try:
            self.game_path = self._query_game_path()
            (self.engine_base, self.engine_path,
             self.engine_size) = self._find_engine(pid)
            self._verify_owner_installation()
            # A matching on-disk DLL is not enough. The runtime instructions
            # that read the four globals must also match SHA-backed original
            # x86 witnesses, with OS-loader PE32 relocation accounted for.
            self.attest_loaded_allocator_code()
        except BaseException:
            self.close()
            raise

    def _query_game_path(self):
        path = ctypes.create_unicode_buffer(32768)
        size = self.wintypes.DWORD(len(path))
        if not self.kernel.QueryFullProcessImageNameW(
                self.handle, 0, path, ctypes.byref(size)):
            raise OSError(ctypes.get_last_error(),
                          "unable to identify target process image")
        return Path(path.value)

    def _find_engine(self, pid):
        wt = self.wintypes

        class MODULEENTRY32W(ctypes.Structure):
            _fields_ = [
                ("dwSize", wt.DWORD), ("th32ModuleID", wt.DWORD),
                ("th32ProcessID", wt.DWORD), ("GlblcntUsage", wt.DWORD),
                ("ProccntUsage", wt.DWORD),
                ("modBaseAddr", ctypes.c_void_p),
                ("modBaseSize", wt.DWORD), ("hModule", wt.HMODULE),
                ("szModule", wt.WCHAR * 256), ("szExePath", wt.WCHAR * 260),
            ]

        handle = self.kernel.CreateToolhelp32Snapshot(
            self.TH32CS_SNAPMODULE | self.TH32CS_SNAPMODULE32, pid)
        if not handle or handle == ctypes.c_void_p(-1).value:
            raise OSError(ctypes.get_last_error(),
                          "cannot enumerate owner engine modules")
        try:
            item = MODULEENTRY32W()
            item.dwSize = ctypes.sizeof(item)
            ok = self.kernel.Module32FirstW(handle, ctypes.byref(item))
            while ok:
                if item.szModule.casefold() == "engine.dll":
                    return (int(item.modBaseAddr),
                            Path(item.szExePath), int(item.modBaseSize))
                ok = self.kernel.Module32NextW(handle, ctypes.byref(item))
        finally:
            self.kernel.CloseHandle(handle)
        raise RuntimeError("owner process has not loaded engine.dll")

    def _verify_owner_installation(self):
        if self.game_path.name.casefold() != "sinepisodes.exe":
            raise RuntimeError("refusing a process other than SinEpisodes.exe")
        # Expected original installation: SinEpisodes.exe in root, engine.dll
        # in root/bin. Only local existing files are read; nothing is staged.
        if (self.engine_path.name.casefold() != "engine.dll" or
                self.engine_path.parent.name.casefold() != "bin" or
                self.engine_path.parent.parent.resolve() !=
                self.game_path.parent.resolve()):
            raise RuntimeError("loaded engine.dll path is not from game bin")
        if sha256_file(self.engine_path) != ENGINE_SHA:
            raise RuntimeError("refusing unknown/non-original engine.dll SHA")
        if not 0 < self.engine_base <= 0xFFFFFFFF:
            raise RuntimeError("loaded engine.dll is not mapped as PE32")

    def read_bytes(self, address, count):
        """Read at most 16 code bytes; never inspect a remote pointer target."""
        # Python itself can be 64-bit on a Windows runner even when the
        # target is a 32-bit game; the verifier separately enforces PE32
        # addresses and the in-module read bound. Permit the 64-bit local
        # self-process smoke pointer without widening any remote witness.
        pointer_end = 1 << (8 * ctypes.sizeof(ctypes.c_void_p))
        if (not isinstance(address, int) or
                not isinstance(count, int) or
                not 0 < address < pointer_end or
                not 0 < count <= 16 or address + count > pointer_end):
            raise ValueError("unbounded process code witness read")
        buffer = ctypes.create_string_buffer(count)
        got = ctypes.c_size_t(0)
        if not self.kernel.ReadProcessMemory(
                self.handle, ctypes.c_void_p(address),
                ctypes.byref(buffer), count, ctypes.byref(got)
        ) or got.value != count:
            raise OSError(ctypes.get_last_error(),
                          "read-only PE32 original opcode unavailable")
        return buffer.raw

    def attest_loaded_allocator_code(self):
        """Two unpinned witness passes, not an atomic/continuous guarantee."""
        report = verify_loaded_code(
            self.read_bytes, self.engine_base, self.engine_size)
        if not report.accepted:
            raise RuntimeError(
                "loaded original engine allocator witness refused: " +
                report.summary())
        return report

    def read_dword(self, address):
        value = ctypes.c_uint32(0)
        got = ctypes.c_size_t(0)
        if not self.kernel.ReadProcessMemory(
                self.handle, ctypes.c_void_p(address),
                ctypes.byref(value), ctypes.sizeof(value),
                ctypes.byref(got)) or got.value != 4:
            raise OSError(ctypes.get_last_error(),
                          "read-only edict global unavailable")
        return int(value.value)

    def sample(self):
        return read_descriptor(self.read_dword, self.engine_base)

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


def test_win32_readonly_api_on_self():
    """Never accesses a game: Windows test reads only our own DWORD."""
    if os.name != "nt":
        return
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL,
                                   wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    kernel.ReadProcessMemory.argtypes = [
        wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
        ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel.ReadProcessMemory.restype = wintypes.BOOL
    handle = kernel.OpenProcess(
        Win32Reader.PROCESS_VM_READ |
        Win32Reader.PROCESS_QUERY_LIMITED_INFORMATION,
        False, os.getpid())
    if not handle:
        raise AssertionError("Windows read-only self OpenProcess failed")
    try:
        original = ctypes.c_uint32(0x1A2B3C4D)
        observed = ctypes.c_uint32(0)
        transferred = ctypes.c_size_t(0)
        assert kernel.ReadProcessMemory(
            handle, ctypes.cast(ctypes.byref(original),ctypes.c_void_p),
            ctypes.byref(observed),4,ctypes.byref(transferred))
        assert transferred.value == 4 and observed.value == original.value
        from types import SimpleNamespace
        self_reader = SimpleNamespace(handle=handle, kernel=kernel)
        assert Win32Reader.read_bytes(
            self_reader, ctypes.addressof(original), 4) == (
            original.value.to_bytes(4, "little"))
        try:
            Win32Reader.read_bytes(
                self_reader, ctypes.addressof(original), 17)
        except ValueError:
            pass
        else:
            raise AssertionError("unbounded process code read accepted")

        # Exercise ALL TEN instruction windows through REAL Windows
        # ReadProcessMemory over synthetic local buffers. A mapping adapter
        # links fake PE32 VAs to these owned buffers; no actual game module
        # or original process is loaded by the Windows CI fixture.
        from attest_sin_original_engine_live_allocator_code import (
            WITNESSES, expected_instruction)
        fake_pe32_base = 0x25000000
        expected = {
            fake_pe32_base + w.va - IMAGE_BASE:
                expected_instruction(w, fake_pe32_base)
            for w in WITNESSES
        }
        buffers = {
            va: ctypes.create_string_buffer(raw)
            for va, raw in expected.items()
        }
        def read_owned_witness(va, length):
            if va not in buffers:
                raise OSError("no synthetic original-free witness backing")
            return Win32Reader.read_bytes(
                self_reader, ctypes.addressof(buffers[va]), length)
        code = verify_loaded_code(
            read_owned_witness, fake_pe32_base, 0x900000)
        assert code.accepted and code.code_windows_checked == 20
        first_va = fake_pe32_base + WITNESSES[0].va - IMAGE_BASE
        original_code = expected[first_va]
        tampered = bytes([original_code[0] ^ 1]) + original_code[1:]
        ctypes.memmove(buffers[first_va], tampered, len(tampered))
        assert verify_loaded_code(
            read_owned_witness, fake_pe32_base,
            0x900000).status == "code_witness_mismatch"
    finally:
        assert kernel.CloseHandle(handle)


def self_test():
    assert IMAGE_BASE == 0x20000000
    assert len(ENGINE_SHA) == 64
    assert ORIGINAL_GLOBALS == {
        "protected_last": 0x2079DA64,
        "current_count": 0x2079DAFC,
        "capacity": 0x2079DB00,
        "edict_base": 0x2079DB04,
    }
    synthetic_loaded_base = 0x25000000
    mapped = {
        synthetic_loaded_base + va - IMAGE_BASE: value
        for role, va, value in (
            ("protected_last", ORIGINAL_GLOBALS["protected_last"], 1),
            ("current_count", ORIGINAL_GLOBALS["current_count"], 3),
            ("capacity", ORIGINAL_GLOBALS["capacity"], 6),
            ("edict_base", ORIGINAL_GLOBALS["edict_base"], 0x12001000),
        )
    }
    first = read_descriptor(lambda va: mapped[va],synthetic_loaded_base)
    assert first == Descriptor(0x12001000,3,6,1,"plausible")
    assert read_descriptor(lambda _: (_ for _ in ()).throw(OSError("gone")),
                           synthetic_loaded_base).classification == "unreadable"
    assert read_descriptor(lambda va: mapped[va],0xFFFFFFFF).classification == "unreadable"
    assert classify(0,0,0,0) == "not_initialized"
    assert classify(0,2,6,1) == "partial_initialization"
    assert classify(0x11000000,7,6,1) == "count_exceeds_capacity"
    assert classify(0x11000000,3,65537,1) == "capacity_exceeds_observer_safety_limit"
    assert classify(0x11000001,3,6,1) == "unaligned_pointer"
    assert classify(0xFFFFFFF0,3,6,1) == "pointer_extent_overflow"
    assert classify(0x11000000,3,6,3) == "protected_index_not_in_count"
    assert classify(0x11000000,0,6,0) == "no_used_edicts"
    second = Descriptor(0x12002000,3,6,1,"plausible")
    assert summary([first,second]) == (2,1)
    assert summary([first,Descriptor(),second]) == (2,0)
    assert summary([first,first,first]) == (3,0)
    assert hashlib.sha256(b"abc").hexdigest() == (
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
    # Verify the actual Win32Reader gate, not just the standalone code
    # verifier, using fake in-process instruction windows and no game PID.
    from types import SimpleNamespace
    from attest_sin_original_engine_live_allocator_code import (
        WITNESSES, expected_instruction)
    memory = {
        synthetic_loaded_base + witness.va - IMAGE_BASE:
            expected_instruction(witness, synthetic_loaded_base)
        for witness in WITNESSES
    }
    fake = SimpleNamespace(
        engine_base=synthetic_loaded_base,
        engine_size=0x900000,
        read_bytes=lambda at, length: memory[at][:length])
    report = Win32Reader.attest_loaded_allocator_code(fake)
    assert report.accepted and report.code_windows_checked == 20
    first_code_address = synthetic_loaded_base + WITNESSES[0].va - IMAGE_BASE
    pristine = memory[first_code_address]
    memory[first_code_address] = bytes([pristine[0] ^ 1]) + pristine[1:]
    try:
        Win32Reader.attest_loaded_allocator_code(fake)
    except RuntimeError as err:
        assert "select_protected_last" in str(err)
    else:
        raise AssertionError("Win32Reader accepted tampered live code")
    test_win32_readonly_api_on_self()
    print("PASS original-free engine edict owner observer: 4 SHA-grounded global RVAs, shifted image bases, torn/unreadable states, opcode-attestation gate refusal, overflow and optional Windows read-only self process API")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--pid", type=int,
                       help="explicit PID of the owner's running original game")
    parser.add_argument("--samples", type=int, default=40,
                        help="read-only samples, 1..2000 (default 40)")
    parser.add_argument("--interval-ms", type=int, default=100,
                        help="milliseconds between samples, 20..2000")
    parser.add_argument("--owner-read-only-confirmed", action="store_true",
                        help="required for live original-process observation")
    parser.add_argument("--show-pointers", action="store_true",
                        help="print PE32 edict base pointer values locally")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.owner_read_only_confirmed:
        parser.error("live observation requires --owner-read-only-confirmed")
    if os.name != "nt":
        parser.error("live observation requires Windows")
    if not 1 <= args.samples <= 2000 or not 20 <= args.interval_ms <= 2000:
        parser.error("samples or interval outside bounded read-only limits")
    if args.pid is None or args.pid <= 0:
        parser.error("invalid owner game PID")

    try:
        with Win32Reader(args.pid) as reader:
            print("ORIGINAL_ENGINE_SHA256_VERIFIED=true")
            print("ORIGINAL_ENGINE_EDICT_VA_WITNESSES=4")
            print("LOADED_ENGINE_ALLOCATOR_CODE_WITNESSES_BEGIN=10")
            observations = []
            for index in range(args.samples):
                snapshot = reader.sample()
                observations.append(snapshot)
                label = (
                    f"SAMPLE {index + 1} state={snapshot.classification}"
                    f" count={snapshot.current_count} capacity={snapshot.capacity}"
                    f" protected_last={snapshot.protected_last}"
                )
                if args.show_pointers:
                    label += f" edict_base=0x{snapshot.edict_base:08x}"
                print(label,flush=True)
                if index + 1 < args.samples:
                    time.sleep(args.interval_ms / 1000)
            # Match the same loaded original code after the sampling window.
            # Code could still change/ABA in between: no continuous claim.
            reader.attest_loaded_allocator_code()
            print("LOADED_ENGINE_ALLOCATOR_CODE_WITNESSES_END=10")
            plausible, moved = summary(observations)
            print(f"OWNER_EDICT_PLAUSIBLE_SAMPLES={plausible}")
            print(f"OWNER_EDICT_OBSERVED_ADJACENT_BASE_CHANGES={moved}")
            print("OWNER_EDICT_CONTINUOUS_PHYSICAL_PIN_PROVEN=false")
            print("OWNER_EDICT_REAL_SERVER_GAMEENTS_REGISTRATION_PROVEN=false")
            return 0 if plausible else 2
    except (OSError, RuntimeError, ValueError) as error:
        print(f"OWNER_EDICT_OBSERVER_REFUSED={type(error).__name__}: {error}",
              file=sys.stderr)
        return 3


if __name__ == "__main__":
    sys.exit(main())
