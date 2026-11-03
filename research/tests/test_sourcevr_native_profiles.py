"""Original-file-free SourceVR 0.5.5 seven-profile regression fixtures."""
from __future__ import annotations
import io
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sourcevr_native_profiles import (
    BOOTSTRAP, GAMES, ROOT, ManifestError,
    inspect_sourcevr_zip,parse_native_manifest,parse_asset_manifest,
)

DIGEST_A="a"*64
DIGEST_B="b"*64
DIGEST_C="c"*64

def fake_arm64(marker):
    raw=bytearray(64)
    raw[0:6]=b"\x7fELF\x02\x01"
    raw[18:20]=(183).to_bytes(2,"little")
    return bytes(raw)+marker+b"\x00"

def build_archive(*, mutate=None):
    with io.BytesIO() as output:
        with zipfile.ZipFile(output,"w",compression=zipfile.ZIP_DEFLATED) as z:
            z.writestr("source vr/assets/version.properties",
                       "vrVersionName=0.5.5+fixture\nvrSemver=0.5.5\n")
            for game in GAMES:
                one=f"sourcevr/v1/native/blobs/{DIGEST_A}/libclient.so"
                two=f"sourcevr/v1/native/blobs/{DIGEST_B}/libserver.so"
                rows=[f"0000\t{DIGEST_A}\tlibclient.so\tlibclient.so\t{one}\t-",
                      f"0001\t{DIGEST_B}\tlibserver.so\tlibserver.so\t{two}\tlibclient.so"]
                load=[one,two]
                if mutate=="bad_order" and game=="hl2": load.reverse()
                if mutate=="bad_dependency" and game=="hl2":
                    rows[1]=rows[1].replace("libclient.so", "libmissing.so", 1) if False else (
                        f"0001\t{DIGEST_B}\tlibserver.so\tlibserver.so\t{two}\tlibmissing.so")
                z.writestr(ROOT+f"native/{game}.manifest",
                           f"SOURCEVR_NATIVE_V1\t{game}\n"+"\n".join(rows)+"\n")
                z.writestr(ROOT+f"native/{game}.load","\n".join(load)+"\n")
            z.writestr("source vr/assets/"+one,b"client binary bytes")
            if mutate!="missing_native":z.writestr("source vr/assets/"+two,b"server binary bytes")
            z.writestr(ROOT+f"blobs/sha256/{DIGEST_C}",b"content bytes")
            for scope in ("overlay","vr"):
                for idx,game in enumerate(GAMES):
                    text=f"{DIGEST_C}  cfg/{game}.cfg\n"
                    if mutate=="bad_asset" and scope=="overlay" and game=="hl2":
                        text=f"{DIGEST_C}  ../escape.cfg\n"
                    z.writestr(ROOT+f"{scope}/{game}.manifest",text)
            z.writestr("source vr/lib/arm64-v8a/libsourcevrbootstrap.so",
                       fake_arm64(b"ANativeActivity_onCreate"))
            z.writestr("source vr/lib/arm64-v8a/libhl2vrbridge.so",
                       fake_arm64(b"JNI_OnLoad"))
        return output.getvalue()

def analyze_bytes(raw):
    with tempfile.TemporaryDirectory() as td:
        p=Path(td)/"source vr.zip"
        p.write_bytes(raw)
        return inspect_sourcevr_zip(p,verify_bootstrap=False)

class SourceVRTests(unittest.TestCase):
    def test_all_seven_load_lists_and_asset_references(self):
        result=analyze_bytes(build_archive())
        self.assertEqual(result["native_module_sets"],7)
        self.assertEqual(result["unique_native_payload_names"],2)
        self.assertEqual(result["game_profiles"]["hl2"]["modules"],2)
        self.assertEqual(len(result["overlay_vr_asset_manifests"]["overlay"]),7)
        self.assertFalse(result["sin_profile_present"])
    def test_refuse_changed_load_order(self):
        with self.assertRaisesRegex(ManifestError,"load order"):
            analyze_bytes(build_archive(mutate="bad_order"))
    def test_refuse_unknown_native_dependency(self):
        with self.assertRaisesRegex(ManifestError,"unsatisfied"):
            analyze_bytes(build_archive(mutate="bad_dependency"))
    def test_refuse_missing_native_blob(self):
        with self.assertRaisesRegex(ManifestError,"missing"):
            analyze_bytes(build_archive(mutate="missing_native"))
    def test_refuse_path_traversal(self):
        with self.assertRaisesRegex(ManifestError,"unsafe"):
            analyze_bytes(build_archive(mutate="bad_asset"))
    def test_refuse_wrong_native_digest(self):
        text=(f"SOURCEVR_NATIVE_V1\thl2\n"
              f"0000\twrong\tlibserver.so\tlibserver.so\t"
              f"sourcevr/v1/native/blobs/wrong/libserver.so\t-\n")
        with self.assertRaisesRegex(ManifestError,"digest"):
            parse_native_manifest(text,"hl2")
    def test_bootstrap_digest_pins_required_by_default(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/"source vr.zip";p.write_bytes(build_archive())
            with self.assertRaisesRegex(ManifestError,"SHA-256"):
                inspect_sourcevr_zip(p,verify_bootstrap=True)
    def test_reject_bad_overlay_hash(self):
        with self.assertRaisesRegex(ManifestError,"invalid"):
            parse_asset_manifest("xyz  cfg/example.cfg\n","overlay","hl2")
    def test_old_apk_comparison_uses_hashes_not_names_only(self):
        ref={"libraries":{"libclient.so":{"sha256":DIGEST_A},
                          "libserver.so":{"sha256":"d"*64}}}
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/"source vr.zip";p.write_bytes(build_archive())
            result=inspect_sourcevr_zip(p,reference=ref,verify_bootstrap=False)
        comparison=result["reference_0989_comparison"]["hl2"]
        self.assertEqual(comparison["matched_hashes"],["libclient.so"])
        self.assertEqual(comparison["different_hashes"],["libserver.so"])

if __name__=="__main__":
    unittest.main()
