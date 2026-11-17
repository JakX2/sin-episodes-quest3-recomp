"""Original-free synthetic PE32 tests of the SDK2004 x86 ABI shape verifier."""
import hashlib
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from test_recover_original_vtable_prefixes import fake_original, spec
from verify_original_sdk2004_init_abi import Shape, verify_module


def fixture():
    image = fake_original()
    # Synthetic .text entry 0x10001010 with manually placed generic
    # x86 RET imm16 at 0x10001020. Not a commercial function body.
    image[0x420:0x423] = b"\xC2\x0C\x00"
    case = Shape("synthetic.dll", "Synthetic001", "Init", 12,
                 (0x10001020,), 0x40)
    return image, case


class OwnerSdk2004AbiShapeTests(unittest.TestCase):
    def test_full_authentication_vptr_rtti_and_ret_cleanup(self):
        image, case = fixture()
        report = verify_module(bytes(image), spec(image), (case,))
        self.assertEqual(len(report), 1)
        self.assertEqual(report[0]["x86_callee_stack_cleanup_bytes"], 12)
        self.assertTrue(report[0]["observed_stack_cleanup_matches_sdk_2004_candidate"])
        self.assertFalse(report[0]["sdk_method_semantics_or_name_fully_verified"])
        self.assertFalse(report[0]["retail_engine_invocation_tested"])

    def test_tampered_original_is_rejected_before_opcode_read(self):
        image, case = fixture()
        pinned = spec(image)
        image[0x420] = 0x90
        with self.assertRaisesRegex(ValueError, "SHA256"):
            verify_module(bytes(image), pinned, (case,))

    def test_wrong_ret_immediate_rejected_even_with_matching_sha(self):
        image, case = fixture()
        struct.pack_into("<H", image, 0x421, 16)
        with self.assertRaisesRegex(ValueError, "contradicts"):
            verify_module(bytes(image), spec(image), (case,))

    def test_missing_ret_rejected_even_with_matching_sha(self):
        image, case = fixture()
        image[0x420] = 0x90
        with self.assertRaisesRegex(ValueError, "no longer RET"):
            verify_module(bytes(image), spec(image), (case,))

    def test_return_site_outside_reviewed_entry_window(self):
        image, case = fixture()
        invalid = Shape(case.module, case.interface, case.sdk2004_candidate,
                        case.expected_stack_pop, (0x10001050,), 0x30)
        with self.assertRaisesRegex(ValueError, "bounded"):
            verify_module(bytes(image), spec(image), (invalid,))

    def test_duplicate_return_site_rejected(self):
        image, case = fixture()
        duplicate = Shape(case.module, case.interface, case.sdk2004_candidate,
                          case.expected_stack_pop,
                          (0x10001020, 0x10001020), case.max_entry_to_return)
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            verify_module(bytes(image), spec(image), (duplicate,))

    def test_missing_reviewed_vtable_interface_rejected(self):
        image, case = fixture()
        invalid = Shape(case.module, "VClient011", case.sdk2004_candidate,
                        case.expected_stack_pop, case.return_sites, 0x40)
        with self.assertRaisesRegex(ValueError, "unreviewed"):
            verify_module(bytes(image), spec(image), (invalid,))

    def test_duplicate_case_rejected(self):
        image, case = fixture()
        with self.assertRaisesRegex(ValueError, "Repeated"):
            verify_module(bytes(image), spec(image), (case, case))

    def test_non_executable_return_site_rejected(self):
        image, case = fixture()
        invalid = Shape(case.module, case.interface, case.sdk2004_candidate,
                        case.expected_stack_pop, (0x10002040,), 0x2000)
        with self.assertRaisesRegex(ValueError, "not executable"):
            verify_module(bytes(image), spec(image), (invalid,))

    def test_wrong_vptr_rejected(self):
        image, case = fixture()
        struct.pack_into("<I", image, 0x800, 0x10002024)
        with self.assertRaisesRegex(ValueError, "vptr"):
            verify_module(bytes(image), spec(image), (case,))


if __name__ == "__main__":
    unittest.main()
