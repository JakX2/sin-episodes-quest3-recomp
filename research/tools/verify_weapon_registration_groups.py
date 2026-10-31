#!/usr/bin/env python3
"""Phase 57: inspect original x86 SiN weapon descriptor registrations.

Takes *user-supplied* unmodified SiN DLLs. No game binary data is distributed.
This proves static layout/registration stores; it cannot establish who consumes
registrations, or whether a weapon actually reloads one round at a time.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError, inspect_binary

STRIDE = 52
# Weapon data array start VA, registration stub VA, count, count store VA, ptr store VA.
REGISTRATIONS = {
    "server_matched": ("server", 0x2251F864, 0x222AC4F0, 29, 0x2251F7F8, 0x2251F7F4),
    "client_matched": ("client", 0x242F62FC, 0x24090EB0, 29, 0x242F6264, 0x242F6260),
    "server_extended": ("server", 0x2251FFEC, 0x222B0E07, 42, 0x2251F814, 0x2251F810),
}


def read_c_string(pe: PE32, va: int) -> str:
    sec = next((s for s in pe.sections if s.rva <= va - pe.base < s.rva + min(s.virtual_size, s.raw_size)), None)
    if sec is None or sec.name != ".rdata":
        raise VerificationError(f"descriptor name {va:#x} does not point into .rdata")
    pos = pe.va_to_offset(va)
    limit = sec.raw_offset + sec.raw_size
    end = pe.content.find(b"\0", pos, min(pos + 96, limit))
    if end <= pos:
        raise VerificationError(f"descriptor name {va:#x} is not a short C string")
    data = pe.content[pos:end]
    if not all(32 <= b <= 126 for b in data):
        raise VerificationError(f"descriptor name {va:#x} is not printable ASCII")
    return data.decode("ascii")


def field_records(pe: PE32, start_va: int, count: int, *, allow_uninitialized_tail: int = 0) -> list[dict]:
    entries = []
    for i in range(count):
        va = start_va + STRIDE * i
        pos = pe.va_to_offset(va)
        typ, ptr, off = struct.unpack_from("<III", pe.content, pos)
        is_uninitialized = i >= count - allow_uninitialized_tail
        if is_uninitialized:
            if any(pe.content[pos:pos + STRIDE]):
                raise VerificationError(f"expected file-zeroed runtime field descriptor at {va:#x}")
            entries.append({"index": i, "descriptor_va": hex(va), "initialized_at_runtime": True})
            continue
        if not (0 < typ <= 30 and off < 0x2000):
            raise VerificationError(f"invalid static descriptor at {va:#x}: type={typ}, offset={off:#x}")
        entries.append({"index": i, "descriptor_va": hex(va), "field": read_c_string(pe, ptr),
                        "type_id": typ, "object_offset": off})
    return entries


def immediate_store(destination: int, value: int) -> bytes:
    """Encode `mov dword ptr ds:[absolute], immediate` (x86 C7 05)."""
    return b"\xC7\x05" + struct.pack("<II", destination, value)


def verify_registration(pe: PE32, label: str, start: int, stub: int, count: int, count_at: int, ptr_at: int) -> None:
    code = pe.content[pe.va_to_offset(stub):pe.va_to_offset(stub) + 48]
    for name, pattern in (("count", immediate_store(count_at, count)),
                          ("descriptor pointer", immediate_store(ptr_at, start))):
        if pattern not in code:
            raise VerificationError(f"{label}: registration {name} store not found at {stub:#x}")


def assert_mirror(server: list[dict], client: list[dict]) -> dict:
    if len(server) != len(client) or not server:
        raise VerificationError("29-entry server/client registration lengths differ")
    offset_counts: dict[str, int] = {}
    exceptions = []
    for i, (s, c) in enumerate(zip(server, client)):
        if (s["field"], s["type_id"]) != (c["field"], c["type_id"]):
            raise VerificationError(f"field/type/order mismatch at entry {i}")
        delta = c["object_offset"] - s["object_offset"]
        offset_counts[hex(delta)] = offset_counts.get(hex(delta), 0) + 1
        if delta != 0x2F4:
            exceptions.append({"field": s["field"], "index": i, "server_offset": hex(s["object_offset"]),
                               "client_offset": hex(c["object_offset"]), "delta": hex(delta)})
    if offset_counts != {"0x10": 1, "0x2f4": 28} or [x["field"] for x in exceptions] != ["m_nNextThinkTick"]:
        raise VerificationError(f"unexpected object-layout delta pattern: {offset_counts}")
    return {"offset_deltas": offset_counts, "exceptions": exceptions}


def verify_runtime_tail(pe: PE32, start: int, static_count: int, total_count: int) -> list[str]:
    code = pe.content[pe.va_to_offset(0x222B0940):pe.va_to_offset(0x222B0E07)]
    references = []
    for i in range(static_count, total_count):
        va = start + STRIDE * i
        # Every trailing descriptor has initialization references to its name pointer
        # and offset in the code region. This does not identify the consumer.
        for delta in (4, 8):
            if struct.pack("<I", va + delta) not in code:
                raise VerificationError(f"no initializer reference to field {i} +{delta} at {va:#x}")
        references.append(hex(va))
    return references


def inspect(server_path: Path, client_path: Path) -> dict:
    # Phase 56 checks both source identities and four named reload fields.
    metadata = {role: inspect_binary(path, role) for role, path in (("server", server_path), ("client", client_path))}
    pe = {"server": PE32(server_path.read_bytes()), "client": PE32(client_path.read_bytes())}
    lists = {}
    for label, (role, start, stub, count, count_at, ptr_at) in REGISTRATIONS.items():
        verify_registration(pe[role], label, start, stub, count, count_at, ptr_at)
        lists[label] = field_records(pe[role], start, count,
                                     allow_uninitialized_tail=10 if label == "server_extended" else 0)
    mirror = assert_mirror(lists["server_matched"], lists["client_matched"])
    if len([x for x in lists["server_extended"] if not x.get("initialized_at_runtime")]) != 32:
        raise VerificationError("unexpected extended server table static entry count")
    tail = verify_runtime_tail(pe["server"], REGISTRATIONS["server_extended"][1], 32, 42)
    return {"result": "pass", "scope": "build-specific PE descriptor registration",
            "server_sha256": metadata["server"]["sha256"], "client_sha256": metadata["client"]["sha256"],
            "registrations": [{"label": k, "start_va": hex(v[1]), "registration_stub_va": hex(v[2]),
                               "declared_count": v[3]} for k, v in REGISTRATIONS.items()],
            "server_client_mirror": mirror, "extended_server": {"static_descriptors": 32,
                                       "runtime_initialized_slots": 10, "runtime_slot_vas": tail},
            "warning": "No classification of registration consumer or single-round runtime activation is implied."}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server", type=Path)
    parser.add_argument("client", type=Path)
    args = parser.parse_args()
    try:
        result = inspect(args.server, args.client)
    except (OSError, VerificationError, struct.error) as e:
        parser.exit(1, f"VERIFICATION FAILED: {e}\n")
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
