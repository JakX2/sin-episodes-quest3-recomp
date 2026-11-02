import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from sourcevr_isolated_apk_builder import (
    ANDROID, build, change_manifest, fqcn, validate_arm64_probe, validate_signed_apk,
)

SAMPLE = f'''<manifest xmlns:android="{ANDROID}" package="com.sourcevrport.hl2vr">
 <permission android:name="com.sourcevrport.hl2vr.SAFE"/>
 <uses-permission android:name="android.permission.INTERNET"/>
 <application android:name=".App" android:extractNativeLibs="false">
   <activity android:name=".MainActivity"/>
   <activity-alias android:name=".Alias" android:targetActivity=".MainActivity"/>
   <provider android:name="com.sourcevrport.hl2vr.SomeProvider" android:authorities="com.sourcevrport.hl2vr.data"/>
 </application>
</manifest>'''


class ApkBuilderTests(unittest.TestCase):
    def test_component_resolution(self):
        self.assertEqual(fqcn("org.old", ".Main"), "org.old.Main")
        self.assertEqual(fqcn("org.old", "Main"), "org.old.Main")
        self.assertEqual(fqcn("org.old", "org.other.Class"), "org.other.Class")

    def test_manifest_identity_and_android_extraction(self):
        import xml.etree.ElementTree as ET
        doc = ET.fromstring(change_manifest(SAMPLE, "com.sourcevrport.hl2vr.sinprobe"))
        attr = lambda name: "{%s}%s" % (ANDROID, name)
        self.assertEqual(doc.get("package"), "com.sourcevrport.hl2vr.sinprobe")
        app = doc.find("application")
        self.assertEqual(app.get(attr("name")), "com.sourcevrport.hl2vr.App")
        self.assertEqual(app.get(attr("extractNativeLibs")), "true")
        self.assertEqual(app.find("activity").get(attr("name")), "com.sourcevrport.hl2vr.MainActivity")
        alias = app.find("activity-alias")
        self.assertEqual(alias.get(attr("targetActivity")), "com.sourcevrport.hl2vr.MainActivity")
        self.assertEqual(app.find("provider").get(attr("authorities")),
                         "com.sourcevrport.hl2vr.sinprobe.data")
        self.assertEqual(doc.find("uses-permission").get(attr("name")), "android.permission.INTERNET")
        self.assertEqual(doc.find("permission").get(attr("name")), "com.sourcevrport.hl2vr.sinprobe.SAFE")

    def test_reject_unrecognized_app_packages(self):
        with self.assertRaisesRegex(ValueError, "unsupported original"):
            change_manifest(SAMPLE.replace("com.sourcevrport.hl2vr", "org.other"), "org.test.debug")
        with self.assertRaisesRegex(ValueError, "must differ"):
            change_manifest(SAMPLE, "com.sourcevrport.hl2vr")
        with self.assertRaisesRegex(ValueError, "invalid debug package"):
            change_manifest(SAMPLE, "invalid package!")
        with self.assertRaisesRegex(ValueError, "sharedUserId"):
            change_manifest(SAMPLE.replace("<manifest ", '<manifest android:sharedUserId="org.unsafe" '),
                            "org.test.debug")

    def test_probe_architecture_and_symbol_guards(self):
        blob = bytearray(256)
        blob[:6] = b"\x7fELF\x02\x01"
        blob[18:20] = (183).to_bytes(2, "little")
        blob.extend(b"CreateInterface\x00SinFactoryProbeRequestCount\x00")
        self.assertEqual(len(validate_arm64_probe(bytes(blob))), 64)
        with self.assertRaisesRegex(ValueError, "ARM64"):
            validate_arm64_probe(b"junk")
        with self.assertRaisesRegex(ValueError, "expected factory"):
            validate_arm64_probe(bytes(blob[:256]))

    def test_signed_apk_guards(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "synthetic.apk"
            with zipfile.ZipFile(path, "w") as z:
                z.writestr("classes.dex", b"a" * 16000)
            with self.assertRaisesRegex(ValueError, "missing DEX or diagnostic"):
                validate_signed_apk(path, "org.new.debug")

    def test_no_output_created_on_invalid_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            original, probe, output = root / "original.apk", root / "probe.so", root / "build"
            original.write_bytes(b"not a ZIP")
            probe.write_bytes(b"not an ARM64 ELF")
            with self.assertRaises((ValueError, zipfile.BadZipFile)):
                build(original, probe, output, "org.test.probe",
                      "apktool", "zipalign", "apksigner", "keytool")
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
