"""Guard the project's documentation against stale or inflated feature claims.

Original-game-free and offline: deliberately does not fetch live GitHub data.
The declared implementation checkpoint may advance after future PRs, but the
README, STATUS, ROADMAP, and handoff must remain mutually consistent.
"""
import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
README = ROOT / "README.md"
STATUS = ROOT / "docs/STATUS.md"
ROADMAP = ROOT / "docs/ROADMAP.md"
HANDOFF = ROOT / "docs/HANDOFF.md"
DOCS = (README, STATUS, ROADMAP, HANDOFF)


class ProjectStatusClaimsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = {p.name: p.read_text(encoding="utf-8") for p in DOCS}

    def test_consistent_current_implementation_phase(self):
        readme = re.search(r"Implementation checkpoint:\s*Phase\s*(\d+)", self.text["README.md"])
        status = re.search(r"Current implementation status:\s*Phase\s*(\d+)", self.text["STATUS.md"])
        self.assertIsNotNone(readme, "README must state an implementation checkpoint")
        self.assertIsNotNone(status, "STATUS must state an implementation checkpoint")
        self.assertEqual(readme.group(1), status.group(1))
        self.assertGreaterEqual(int(readme.group(1)), 98)

    def test_not_stale_phase56_scaffold(self):
        for name, content in self.text.items():
            self.assertNotIn("Phase 56 starting scaffold", content, name)
            self.assertNotIn("No real Quest 3/Android ARM64 build", content, name)
        self.assertIn("Phase 56 historical evidence", self.text["STATUS.md"])

    def test_missing_engine_and_vr_features_are_admitted(self):
        for name in ("README.md", "STATUS.md", "HANDOFF.md"):
            content = self.text[name].lower()
            for term in ("quest 3", "sourcevr", "headset", "not"):
                self.assertIn(term, content, (name, term))
        self.assertIn("no real SourceVR", self.text["README.md"])
        self.assertIn("No successful integrated execution", self.text["STATUS.md"])

    def test_distinguish_android_build_and_emulator(self):
        for name in ("README.md", "STATUS.md", "HANDOFF.md"):
            content = self.text[name]
            self.assertRegex(content, r"ARM64", name)
            self.assertRegex(content, r"x86_64", name)
            self.assertRegex(content.lower(), r"(emulator|bionic)", name)
        self.assertIn("not Quest/VR execution", self.text["ROADMAP.md"])

    def test_dynamic_difficulty_is_not_called_absent_or_complete(self):
        readme = self.text["README.md"]
        self.assertIn("14-variable", readme)
        self.assertIn("six", readme)
        self.assertIn("performance-to-adjustment weights/signs", readme)
        self.assertNotRegex(readme.lower(), r"dynamic difficulty[^\n]*not yet implemented")
        status = self.text["STATUS.md"]
        self.assertIn("Phase 98", status)
        self.assertIn("Still unknown:", status)
        self.assertIn("diagnostic logging", status)

    def test_source_paths_match_actual_build(self):
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        for source in (
            "src/weapon_runtime.cpp",
            "src/entity_io_runtime.cpp",
            "src/npc_maker_runtime.cpp",
            "src/grunt_tactical_controls.cpp",
            "src/combat_spread_rules.cpp",
            "src/stat_schema.cpp",
            "src/info_challenge_runtime.cpp",
            "src/dynamic_difficulty_gpv.cpp",
            "src/difficulty_feedback_tick.cpp",
            "src/difficulty_feedback_routing.cpp",
            "src/difficulty_publication_deadline.cpp",
            "src/difficulty_category_bands.cpp",
        ):
            with self.subTest(source=source):
                self.assertIn(source, cmake)
                self.assertTrue((ROOT / source).is_file())

    def test_host_abi_scope_is_explicit(self):
        self.assertTrue((ROOT / "src/host_abi_v1.cpp").is_file())
        self.assertTrue((ROOT / "include/sin_compat/host_abi_v1.h").is_file())
        self.assertIn("sin_compat_host_abi", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
        self.assertIn("not the original game DLL ABI", self.text["STATUS.md"])

    def test_roadmap_acceptance_is_not_claimed_complete(self):
        road = self.text["ROADMAP.md"]
        for milestone in ("R0:", "R1:", "R2:", "R3:", "R4:", "R5:", "R6:"):
            self.assertIn(milestone, road)
        self.assertIn("R1: engine host", road)
        self.assertIn("not implemented", road.lower())
        self.assertIn("Partial reference implementation only", road)
        self.assertIn("Not integrated", road)

    def test_internal_markdown_links_refer_to_real_files(self):
        # Only Markdown links in these four checked documents. Paths with
        # external URLs, anchors and dynamic content are intentionally ignored.
        expression = re.compile(r"(?<!!)\[[^\]]+\]\(([^)\s]+)\)")
        for doc in DOCS:
            for target in expression.findall(doc.read_text(encoding="utf-8")):
                if target.startswith(("https://", "http://", "#", "mailto:")):
                    continue
                file_path = target.split("#", 1)[0]
                if not file_path:
                    continue
                resolved = (doc.parent / file_path).resolve()
                with self.subTest(document=str(doc.relative_to(ROOT)), link=target):
                    self.assertTrue(resolved.is_relative_to(ROOT), "link escapes repository")
                    self.assertTrue(resolved.is_file(), "broken documentation link")


if __name__ == "__main__":
    unittest.main()
