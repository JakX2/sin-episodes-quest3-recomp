import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from sourcevr_isolated_modlib_patch import (
    EXPECTED_DEX_SHA256, SETUP_DESCRIPTOR, find_activity, patch_smali, verify_original_dex
)

BASE = """\
.class public Lcom/sourcevrport/hl2vr/MainActivity;
.super Landroid/app/NativeActivity;

.method private setupEngineEnvironment(Ljava/util/List;Ljava/util/List;Lcom/sourcevrport/hl2vr/ProfileAssetStage$NativeGraph;)V
    .locals 2

    invoke-virtual {p0}, Lcom/sourcevrport/hl2vr/MainActivity;->getApplicationInfo()Landroid/content/pm/ApplicationInfo;
    move-result-object v0
    iget-object v0, v0, Landroid/content/pm/ApplicationInfo;->nativeLibraryDir:Ljava/lang/String;

    .line 121
    const-string v1, "APP_LIB_PATH"
    invoke-static {v1, v0}, Lcom/sourcevrport/hl2vr/MainActivity;->setenv(Ljava/lang/String;Ljava/lang/String;)V

    invoke-static {}, Lcom/sourcevrport/hl2vr/importer/ContentPaths;->userRoot()Ljava/io/File;
    return-void
.end method

.method protected onCreate(Landroid/os/Bundle;)V
    .locals 0
    return-void
.end method
"""


class PatchTests(unittest.TestCase):
    def test_single_injection_after_original(self):
        patched, message = patch_smali(BASE)
        self.assertIn("nativeLibraryDir", message)
        self.assertEqual(patched.count('"APP_MOD_LIB"'), 1)
        self.assertIn(
            'const-string v1, "APP_LIB_PATH"\n'
            '    invoke-static {v1, v0}, Lcom/sourcevrport/hl2vr/MainActivity;->setenv(Ljava/lang/String;Ljava/lang/String;)V\n'
            '    # SourceVR private loader preflight:',
            patched,
        )
        self.assertIn(
            'const-string v1, "APP_MOD_LIB"\n'
            '    invoke-static {v1, v0}, Lcom/sourcevrport/hl2vr/MainActivity;->setenv',
            patched
        )
        self.assertEqual(patched.count(".method"), BASE.count(".method"))
        self.assertIn('invoke-static {}, Lcom/sourcevrport/hl2vr/importer/ContentPaths;', patched)
        with self.assertRaisesRegex(ValueError, "already present"):
            patch_smali(patched)

    def test_original_dex_descriptor_is_used(self):
        self.assertEqual(SETUP_DESCRIPTOR,
            "(Ljava/util/List;Ljava/util/List;"
            "Lcom/sourcevrport/hl2vr/ProfileAssetStage$NativeGraph;)V")
        with self.assertRaisesRegex(ValueError, "verified setupEngineEnvironment"):
            patch_smali(BASE.replace(SETUP_DESCRIPTOR, "()V"))

    def test_different_registers(self):
        source = BASE.replace("iget-object v0, v0,", "iget-object v4, v0,").replace(
            "invoke-static {v1, v0}", "invoke-static {v1, v4}")
        patched, _ = patch_smali(source)
        self.assertIn('invoke-static {v1, v4}, Lcom/sourcevrport/hl2vr/MainActivity;->setenv', patched)

    def test_fail_closed_mismatches(self):
        with self.assertRaises(ValueError):
            patch_smali("not SourceVR")
        with self.assertRaisesRegex(ValueError, "exactly one APP_LIB_PATH"):
            patch_smali(BASE.replace('"APP_LIB_PATH"', '"OTHER_KEY"'))
        with self.assertRaisesRegex(ValueError, "register does not match"):
            patch_smali(BASE.replace("invoke-static {v1, v0}", "invoke-static {v2, v0}"))
        with self.assertRaisesRegex(ValueError, "nativeLibraryDir"):
            patch_smali(BASE.replace("nativeLibraryDir", "dataDir"))
        with self.assertRaisesRegex(ValueError, "unexpected instructions"):
            patch_smali(BASE.replace('const-string v1, "APP_LIB_PATH"\n',
                                     'const-string v1, "APP_LIB_PATH"\n    const/4 v1, 0x0\n'))

    def test_dex_pin(self):
        self.assertEqual(len(EXPECTED_DEX_SHA256), 64)
        with self.assertRaisesRegex(ValueError, "Unsupported classes.dex"):
            verify_original_dex(b"synthetic not game code")

    def test_find_only_expected_activity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            path = root / "smali_classes2/com/sourcevrport/hl2vr/MainActivity.smali"
            path.parent.mkdir(parents=True)
            path.write_text(BASE)
            self.assertEqual(find_activity(root), path)
            clone = root / "smali/com/sourcevrport/hl2vr/MainActivity.smali"
            clone.parent.mkdir(parents=True)
            clone.write_text(BASE)
            with self.assertRaisesRegex(ValueError, "exactly one"):
                find_activity(root)


if __name__ == "__main__":
    unittest.main()
