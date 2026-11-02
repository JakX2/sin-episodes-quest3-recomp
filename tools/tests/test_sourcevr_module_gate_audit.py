import struct
import unittest

from tools.sourcevr_module_gate_audit import (
    add_immediate, adrp, branch_target, const_true_stub, examine, sections, sign_extend,
)


def fake_engine() -> bytes:
    blob = bytearray(0x400)
    blob[:16] = b"\x7fELF\x02\x01\x01" + b"\0" * 9
    struct.pack_into("<HHIQQQIHHHHHH", blob, 16,
                     3, 183, 1, 0, 0, 0x300, 0, 64, 0, 0, 64, 4, 1)
    shstrtab = b"\0.shstrtab\0.text\0.rodata\0"
    blob[0x280:0x280 + len(shstrtab)] = shstrtab
    hdr = "<IIQQQQIIQQ"
    struct.pack_into(hdr, blob, 0x300 + 64, 1, 3, 0, 0, 0x280, len(shstrtab), 0, 0, 1, 0)
    struct.pack_into(hdr, blob, 0x300 + 128, 11, 1, 6, 0x1000, 0x100, 0x50, 0, 0, 4, 0)
    struct.pack_into(hdr, blob, 0x300 + 192, 17, 1, 2, 0x3000, 0x200, 0x80, 0, 0, 1, 0)
    # ADRP x0, 0x3000; ADD x0,x0,#0; BL 0x1020
    struct.pack_into("<III", blob, 0x100, 0xd0000000, 0x91000000, 0x94000006)
    # Return 1 immediately
    struct.pack_into("<II", blob, 0x120, 0x52800020, 0xd65f03c0)
    blob[0x200:0x200 + len(b"server.so\0GAMEBIN\0")] = b"server.so\0GAMEBIN\0"
    return bytes(blob)


class ModuleGateTests(unittest.TestCase):
    def test_instruction_decoders(self):
        self.assertEqual(sign_extend(0x1ffffe, 21), -2)
        self.assertEqual(adrp(0xd0000000, 0x1000), (0, 0x3000))
        self.assertEqual(add_immediate(0x91000000), (0, 0, 0))
        self.assertEqual(branch_target(0x94000006, 0x1008), 0x1020)
        self.assertIsNone(branch_target(0xd65f03c0, 0x1020))

    def test_synthetic_xref_and_true_gate(self):
        data = fake_engine()
        elf = sections(data)
        self.assertTrue(const_true_stub(data, elf[".text"], 0x1020))
        self.assertFalse(const_true_stub(data, elf[".text"], 0x1000))
        report = examine(data, "fixture")
        self.assertEqual(report["profile"], "fixture")
        self.assertEqual(len(report["string_references"]["server.so"]), 1)
        xref = report["string_references"]["server.so"][0]
        self.assertEqual(xref["string_va"], "0x3000")
        self.assertIn("0x1008 -> 0x1020", xref["nearby_const_true_calls"])
        self.assertIn("GAMEBIN", report["candidate_missing"])

    def test_reject_invalid_elf(self):
        for value in (b"", b"not an ELF" + b"\0"*100):
            with self.assertRaises(ValueError):
                sections(value)


if __name__ == "__main__":
    unittest.main()
