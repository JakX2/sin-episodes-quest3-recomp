#!/usr/bin/env python3
"""Map *hypothetical* Source SDK 2004 virtual names onto reviewed SiN vtable VAs.

This is ABI triage, not proof of slot equivalence. Does not bundle Valve code.
References an archived Source SDK 2004 tree at:
https://github.com/Source-SDK-Archives/source-sdk-2004/tree/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

SDK_REVISION = "962ba835716d3e6d778054ab3fb4534bf3b35e1a"
SDK_ROOT = f"https://github.com/Source-SDK-Archives/source-sdk-2004/blob/{SDK_REVISION}/src_mod/"
# Only symbol identifiers and virtual declaration order are recorded, not
# Valve's header contents, implementations or expression text.
SDK_METHODS = {
    "VClient011": """Init Shutdown LevelInitPreEntity LevelInitPostEntity LevelShutdown
        GetAllClasses HudVidInit HudProcessInput HudUpdate HudReset HudText
        IN_ActivateMouse IN_DeactivateMouse IN_MouseEvent IN_Accumulate
        IN_ClearStates IN_IsKeyDown IN_KeyEvent CreateMove ExtraMouseSample
        WriteUsercmdDeltaToBuffer EncodeUserCmdToBuffer DecodeUserCmdFromBuffer
        View_Render RenderView View_Fade SetCrosshairAngle InitSprite
        ShutdownSprite GetSpriteSize VoiceStatus InstallStringTableCallback
        FrameStageNotify DispatchUserMessage SaveInit SaveWriteFields
        SaveReadFields PreSave Save WriteSaveHeaders ReadRestoreHeaders
        Restore DispatchOnRestore GetStandardRecvProxies WriteSaveGameScreenshot
        EmitSentenceCloseCaption EmitCloseCaption""".split(),
    "ServerGameDLL003": """DLLInit GameInit LevelInit ServerActivate GameFrame
        PreClientUpdate LevelShutdown GameShutdown DLLShutdown GetTickInterval
        GetAllServerClasses GetGameDescription CreateNetworkStringTables
        SaveInit SaveWriteFields SaveReadFields SaveGlobalState RestoreGlobalState
        PreSave Save GetSaveComment WriteSaveHeaders ReadRestoreHeaders Restore
        IsRestoring CreateEntityTransitionList BuildAdjacentMapList
        GetUserMessageInfo GetStandardSendProxies""".split(),
    "ServerGameClients003": """GetPlayerLimits ClientConnect ClientActive
        ClientDisconnect ClientPutInServer ClientCommand SetCommandClient
        ClientSettingsChanged ClientSetupVisibility ProcessUsercmds
        PostClientMessagesSent GetPlayerState ClientEarPosition GetReplayDelay
        GetBugReportInfo""".split(),
    "ServerGameEnts001": """~IServerGameEnts SetDebugEdictBase
        MarkEntitiesAsTouching FreeContainingEntity BaseEntityToEdict
        EdictToBaseEntity CheckTransmit""".split(),
    "GameClientExports001": """~IBaseInterface IsPlayerGameVoiceMuted
        MutePlayerGameVoice UnmutePlayerGameVoice""".split(),
}
SDK_HEADERS = {
    "VClient011": "public/cdll_int.h",
    "ServerGameDLL003": "public/eiface.h",
    "ServerGameClients003": "public/eiface.h",
    "ServerGameEnts001": "public/eiface.h",
    "GameClientExports001": "public/cl_dll/IGameClientExports.h",
}
EXPECTED_PREFIXES = {
    "client.dll": {"GameClientExports001": 4, "VClient011": 48},
    "server.dll": {
        "ServerGameDLL003": 31, "ServerGameEnts001": 7,
        "ServerGameClients003": 15,
    },
}
EXPECTED_SHA256 = {
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}


def pe32(value):
    if not isinstance(value, str) or not re.fullmatch(r"0x[0-9a-fA-F]{1,8}", value):
        raise ValueError("Invalid original PE32 VA")
    n = int(value, 16)
    if n == 0:
        raise ValueError("Null original PE32 VA")
    return n


def build(evidence):
    if (evidence.get("schema") != "sin-owner-original-interface-vtable-prefix-v1"
            or evidence.get("evidence_tier") !=
            "SHA256_authenticated_original_PE32_vtable_pointer_values_only"
            or evidence.get("original_code_or_pseudocode_embedded") is not False):
        raise ValueError("Unexpected or unauthenticated original vtable evidence tier")
    if set(evidence.get("modules", {})) != set(EXPECTED_PREFIXES):
        raise ValueError("Wrong original DLL module set")
    modules = {}
    for module, spec in EXPECTED_PREFIXES.items():
        record = evidence["modules"][module]
        if record.get("full_file_sha256") != EXPECTED_SHA256[module]:
            raise ValueError(f"{module}: original PE32 SHA256 mismatch")
        interfaces = record.get("interfaces", [])
        if len(interfaces) != len(spec):
            raise ValueError(f"{module}: interface count mismatch")
        mapped = {}
        for item in interfaces:
            name = item["interface"]
            if name not in spec or name in mapped:
                raise ValueError(f"{module}: unexpected or duplicate interface")
            slots = item["slots"]
            vtable = pe32(item["original_vtable_va"])
            if len(slots) != spec[name] or item["prefix_slots"] != spec[name]:
                raise ValueError(f"{module}: incomplete authenticated vtable prefix")
            methods = SDK_METHODS[name]
            hints = []
            addresses = set()
            for position, slot in enumerate(slots):
                if slot["slot"] != position:
                    raise ValueError(f"{module}: noncontiguous original vtable slot")
                va = pe32(slot["target_va"])
                if va in addresses:
                    raise ValueError(f"{module}: duplicate original method pointer")
                addresses.add(va)
                if pe32(slot["pointer_entry_va"]) != vtable + 4 * position:
                    raise ValueError(f"{module}: inconsistent vtable pointer VA")
                hinted = methods[position] if position < len(methods) else None
                hints.append({
                    "slot": position,
                    "original_function_candidate_va": f"0x{va:08X}",
                    "source_sdk_2004_method_name_hypothesis": hinted,
                    "sdk_symbol_name_verified_as_sin_method": False,
                })
            mapped[name] = {
                "sdk_header": SDK_ROOT + SDK_HEADERS[name],
                "sdk_declared_virtual_slots": len(methods),
                "authenticated_original_pointer_prefix_slots": len(slots),
                "sdk_exact_version_name_in_2004_headers": True,
                "slot_count_matches": len(methods) == len(slots),
                "slot_order_proven": False,
                "extra_original_prefix_entries_beyond_sdk": max(0, len(slots) - len(methods)),
                "slots": hints,
            }
        if set(mapped) != set(spec):
            raise ValueError(f"{module}: missing original interface")
        modules[module] = dict(sorted(mapped.items()))
    return {
        "schema": "sin-sdk2004-original-vtable-candidate-crosswalk-v1",
        "sdk_revision": SDK_REVISION,
        "evidence_tier": "source_sdk_2004_method_names_are_unverified_abi_hypotheses",
        "source_sdk_code_embedded": False,
        "original_game_code_embedded": False,
        "verified_original_method_signatures_added": 0,
        "modules": modules,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    report = build(json.loads(args.evidence.read_text(encoding="utf-8")))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n",
                        encoding="utf-8")
    for module, entries in report["modules"].items():
        for name, entry in entries.items():
            print(f"{module} {name}: original_pointer_prefix={entry['authenticated_original_pointer_prefix_slots']} "
                  f"SDK2004_virtuals={entry['sdk_declared_virtual_slots']} "
                  "names_are_unverified_hypotheses")


if __name__ == "__main__":
    main()
