# Phase 995: correct named speech caption fixture scope

Phase 993 accidentally inserted the explicit caption token in check_native_vcd_metadata rather than VcdOwnedPlaybackFixture::root_text. Restore the two identical metadata tokens and put caption fields in the intended root VCD, preserving native parser and speech behavior. Existing named startup warm sound and regular precache still use the primary sound. CI pending.
