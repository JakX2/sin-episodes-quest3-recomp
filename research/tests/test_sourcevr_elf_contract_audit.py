"""Synthetic ELF64 fixture tests; no SourceVR or SiN binaries required."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import sourcevr_elf_contract_audit as audit


def synthetic_elf(*, sections=True, bad_link=False, missing_dynsym=False):
    strings = b"\0libc.so\0libvrxr.so\0libtest.so\0CreateInterface\0VClient018\0JNI_OnLoad\0dlopen\0"
    location = lambda text: strings.index(text.encode() + b"\0")
    sym_entry = lambda name, bind, section: struct.pack(
        "<IBBHQQ", location(name), (bind << 4) | 2, 0, section, 0, 0
    )
    symbols = b"\0" * 24 + b"".join((
        sym_entry("CreateInterface", 1, 1),
        sym_entry("VClient018", 1, 1),
        sym_entry("JNI_OnLoad", 1, 1),
        sym_entry("dlopen", 1, 0),
    ))
    dyn = b"".join((
        struct.pack("<qQ", 1, location("libc.so")),
        struct.pack("<qQ", 1, location("libvrxr.so")),
        struct.pack("<qQ", 14, location("libtest.so")),
        struct.pack("<qQ", 0, 0),
    ))
    blob = bytearray(1280)
    blob[:4] = b"\x7fELF"
    blob[4] = 2
    blob[5] = 1
    blob[6] = 1
    shoff = 768 if sections else 0
    num = 4 if sections else 0
    struct.pack_into(
        "<HHIQQQIHHHHHH", blob, 16,
        3, 183, 1, 0, 0, shoff, 0, 64, 0, 0, 64, num, 0
    )
    blob[128:128+len(strings)] = strings
    blob[256:256+len(symbols)] = symbols
    blob[512:512+len(dyn)] = dyn
    shdr = lambda kind, offset, length, link=0, entry=0: struct.pack(
        "<IIQQQQIIQQ", 0, kind, 0, 0, offset, length, link, 0, 1, entry
    )
    if sections:
        headers = [
            b"\0" * 64,
            shdr(3, 128, len(strings)),
            shdr(0 if missing_dynsym else 11, 256, len(symbols),
                 link=9 if bad_link else 1, entry=24),
            shdr(6, 512, len(dyn), link=1, entry=16),
        ]
        for idx, header in enumerate(headers):
            blob[shoff + idx*64: shoff + (idx+1)*64] = header
    return bytes(blob)


class ElfContractAuditTest(unittest.TestCase):
    def with_fixture(self, binary):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "libserver.so"
        path.write_bytes(binary)
        return path

    def test_aarch64_symbols_imports_and_dependencies(self):
        report = audit.audit_elf(self.with_fixture(synthetic_elf()))
        self.assertEqual(report["machine"], "aarch64")
        self.assertEqual(report["elf_class"], "ELF64")
        self.assertTrue(report["has_dynamic_symbols"])
        self.assertEqual(report["soname"], "libtest.so")
        self.assertEqual(report["needed"], ["libc.so", "libvrxr.so"])
        self.assertEqual(report["export_count"], 3)
        self.assertEqual(report["import_count"], 1)
        self.assertTrue(report["entry_point_exports"]["CreateInterface"])
        self.assertFalse(report["entry_point_exports"]["GetEntityAPI2"])
        self.assertIn("VClient018", report["interface_name_hints"])
        self.assertIn("dlopen", report["runtime_linker_import_samples"])

    def test_no_section_headers_means_unknown_not_absence(self):
        report = audit.audit_elf(self.with_fixture(synthetic_elf(sections=False)))
        self.assertFalse(report["has_section_headers"])
        self.assertFalse(report["has_dynamic_symbols"])
        self.assertIsNone(report["entry_point_exports"])
        self.assertTrue(any("unknown" in w for w in report["warnings"]))

    def test_invalid_section_link_fails_closed(self):
        with self.assertRaisesRegex(audit.ElfAuditError, "invalid string-table link"):
            audit.audit_elf(self.with_fixture(synthetic_elf(bad_link=True)))

    def test_non_elf_rejected(self):
        with self.assertRaisesRegex(audit.ElfAuditError, "not an ELF"):
            audit.audit_elf(self.with_fixture(b"x" * 64))

    def test_elf32_rejected_with_clear_error(self):
        data = bytearray(synthetic_elf())
        data[4] = 1
        with self.assertRaisesRegex(audit.ElfAuditError, "ELF64"):
            audit.audit_elf(self.with_fixture(bytes(data)))

    def test_invalid_section_offset_rejected(self):
        data = bytearray(synthetic_elf())
        # section #2 offset, encoded 24 bytes into its header
        struct.pack_into("<Q", data, 768 + 2*64 + 24, 999999)
        with self.assertRaisesRegex(audit.ElfAuditError, "outside file"):
            audit.audit_elf(self.with_fixture(bytes(data)))

    def test_missing_dynsym_is_not_a_negative_createinterface_result(self):
        report = audit.audit_elf(self.with_fixture(synthetic_elf(missing_dynsym=True)))
        self.assertFalse(report["has_dynamic_symbols"])
        self.assertIsNone(report["entry_point_exports"])
        self.assertTrue(report["has_dynamic_entries"])
        self.assertEqual(len(report["needed"]), 2)

    def test_dynamic_table_parse_handles_missing_terminator_as_bounded(self):
        data = bytearray(synthetic_elf())
        struct.pack_into("<qQ", data, 512+3*16, 14, 0)
        report = audit.audit_elf(self.with_fixture(bytes(data)))
        self.assertEqual(len(report["needed"]), 2)

    def test_command_writes_compact_json_without_binary_bytes(self):
        path = self.with_fixture(synthetic_elf())
        report_path = path.parent / "analysis.json"
        self.assertEqual(audit.main([str(path), "-o", str(report_path)]), 0)
        report = json.loads(report_path.read_text(encoding="utf-8"))
        self.assertEqual(report["machine"], "aarch64")
        self.assertEqual(report["entry_point_exports"]["CreateInterface"], True)
        self.assertLess(len(report_path.read_bytes()), 5000)

    def test_bad_section_header_entry_size(self):
        data = bytearray(synthetic_elf())
        # e_shentsize at ELF header byte offset 58
        struct.pack_into("<H", data, 58, 5)
        with self.assertRaisesRegex(audit.ElfAuditError, "section header"):
            audit.audit_elf(self.with_fixture(bytes(data)))


if __name__ == "__main__":
    unittest.main()
