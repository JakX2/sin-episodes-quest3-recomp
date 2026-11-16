"""Synthetic-only cross-platform shared-library factory smoke."""
import ctypes
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE = Path(__file__).with_name("factory.cpp")

class FactorySmoke(unittest.TestCase):
    def test_nonforwarding_shared_library_factory(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.skipTest("No host C++ compiler")
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / ("factory.dll" if sys.platform == "win32" else "libfactory.so")
            flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-shared"]
            if sys.platform != "win32":
                flags.append("-fPIC")
            subprocess.run([compiler, *flags, str(SOURCE), "-o", str(output)], check=True)
            dll = ctypes.CDLL(str(output))
            factory = dll.CreateInterface
            factory.argtypes = (ctypes.c_char_p, ctypes.POINTER(ctypes.c_int))
            factory.restype = ctypes.c_void_p
            code = ctypes.c_int(-99)
            client = factory(b"SIN_RECON_TEST_CLIENT_001", ctypes.byref(code))
            self.assertTrue(client)
            self.assertEqual(code.value, 0)
            self.assertEqual(factory(b"SIN_RECON_TEST_CLIENT_001", None), client)
            server = factory(b"SIN_RECON_TEST_SERVER_001", None)
            self.assertTrue(server)
            self.assertNotEqual(client, server)
            self.assertIsNone(factory(b"VClient017", ctypes.byref(code)))
            self.assertEqual(code.value, 1)
            self.assertIsNone(factory(None, ctypes.byref(code)))
            self.assertEqual(code.value, 1)

if __name__ == "__main__":
    unittest.main()
