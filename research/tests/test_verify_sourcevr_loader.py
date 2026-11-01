"""Synthetic AArch64 ELF fixtures only. No original Source engine bytes."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "verify_sourcevr_loader.py"
SPEC = importlib.util.spec_from_file_location("verify_sourcevr_loader", SCRIPT)
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


def adrp(pc, target, reg=0):
    imm = ((target >> 12) - (pc >> 12)) & ((1 << 21) - 1)
    return 0x90000000 | ((imm & 3) << 29) | (((imm >> 2) & 0x7ffff) << 5) | reg


def add(imm, reg=0):
    return 0x91000000 | ((imm & 0xfff) << 10) | (reg << 5) | reg


def cbnz(pc, dst):
    return 0xb5000000 | ((((dst - pc) // 4) & 0x7ffff) << 5)


def branch(pc, dst):
    return 0x14000000 | (((dst - pc) // 4) & 0x3ffffff)


def fake_elf(valid_warning=True):
    strings = [module.WARNING, "VClient017", "ClientVirtualReality001"]
    rodata = b""
    locations = {}
    for value in strings:
        locations[value] = 0x20000 + len(rodata)
        rodata += value.encode("ascii") + b"\x00"
    code = bytearray(0x100)

    def op(pc, opcode):
        struct.pack_into("<I", code, pc - 0x10000, opcode)

    op(0x1002c, cbnz(0x1002c, 0x10060))
    op(0x10030, adrp(0x10030, locations[module.WARNING]))
    op(0x10034, add(locations[module.WARNING] & 0xfff))
    op(0x10038, 0x94000000)  # BL
    op(0x1003c, branch(0x1003c, 0x10060) if valid_warning else 0xd65f03c0)
    for pc, name in ((0x10070, "VClient017"), (0x10078, "ClientVirtualReality001")):
        op(pc, adrp(pc, locations[name]))
        op(pc + 4, add(locations[name] & 0xfff))

    names = b"\x00.text\x00.rodata\x00.shstrtab\x00"
    blob = bytearray(0x5000)
    blob[:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<H", blob, 18, 183)
    struct.pack_into("<Q", blob, 40, 0x4000)
    struct.pack_into("<HHH", blob, 58, 64, 4, 3)
    blob[0x1000:0x1100] = code
    blob[0x2000:0x2000 + len(rodata)] = rodata
    blob[0x3000:0x3000 + len(names)] = names
    fmt = "<IIQQQQIIQQ"
    for i, (nameoff, addr, off, size) in enumerate([
        (0, 0, 0, 0), (1, 0x10000, 0x1000, 0x100),
        (7, 0x20000, 0x2000, len(rodata)), (15, 0, 0x3000, len(names)),
    ]):
        struct.pack_into(fmt, blob, 0x4000 + 64*i, nameoff, 1, 0, addr, off, size, 0, 0, 1, 0)
    return bytes(blob)


class VerifyLoaderTests(unittest.TestCase):
    def test_adrp_add_decoder(self):
        self.assertEqual(module.adrp_page(adrp(0x10030, 0x20000), 0x10030), 0x20000)
        self.assertEqual(module.add_immediate(add(0x12)), (0, 0, 0x12))

    def test_branch_decoding(self):
        self.assertEqual(module.branch_target(branch(0x1003c, 0x10060), 0x1003c), 0x10060)
        self.assertEqual(module.conditional_nz_target(cbnz(0x1002c, 0x10060), 0x1002c), 0x10060)
        self.assertIsNone(module.branch_target(0xd65f03c0, 0x1003c))

    def test_synthetic_elf_verified_but_no_gameplay_claim(self):
        blob = fake_elf()
        module.PROFILES["fixture"] = {
            "sha256": hashlib.sha256(blob).hexdigest(),
            "warning_adrp": 0x10030,
            "strings": ("VClient017", "ClientVirtualReality001"),
        }
        try:
            result = module.verify_bytes(blob, "fixture")
            self.assertTrue(result["warning_paths_reconverge"])
            self.assertFalse(result["sourcevr_module_loading_verified"])
            self.assertFalse(result["sin_quest_runtime_verified"])
        finally:
            del module.PROFILES["fixture"]

    def test_warning_flow_change_rejected_despite_matching_hash(self):
        blob = fake_elf(False)
        module.PROFILES["fixture"] = {
            "sha256": hashlib.sha256(blob).hexdigest(),
            "warning_adrp": 0x10030,
            "strings": ("VClient017",),
        }
        try:
            with self.assertRaisesRegex(module.VerificationError, "reconverges"):
                module.verify_bytes(blob, "fixture")
        finally:
            del module.PROFILES["fixture"]

    def test_sha256_mismatch_rejected(self):
        blob = fake_elf()
        module.PROFILES["fixture"] = {
            "sha256": "0" * 64, "warning_adrp": 0x10030, "strings": (),
        }
        try:
            with self.assertRaisesRegex(module.VerificationError, "SHA-256 mismatch"):
                module.verify_bytes(blob, "fixture")
        finally:
            del module.PROFILES["fixture"]

    def test_non_arm64_elf_rejected(self):
        blob = bytearray(fake_elf())
        struct.pack_into("<H", blob, 18, 62)
        with self.assertRaisesRegex(module.VerificationError, "AArch64"):
            module.sections(bytes(blob))

    def test_truncated_section_table_rejected(self):
        blob = bytearray(fake_elf())
        struct.pack_into("<Q", blob, 40, len(blob) - 1)
        with self.assertRaisesRegex(module.VerificationError, "section table"):
            module.sections(bytes(blob))

    def test_missing_interface_string_rejected(self):
        blob = fake_elf()
        code = blob[0x1000:0x1100]
        rodata = blob[0x2000:0x2080]
        with self.assertRaisesRegex(module.VerificationError, "missing string"):
            module.xrefs(code, 0x10000, rodata, 0x20000, "MissingInterface001")


if __name__ == "__main__":
    unittest.main()
