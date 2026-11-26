#!/usr/bin/env python3
"""Original-only full-SHA SiN 2006 gameplay triggers audit, with no original bytes output.

This verifies CTriggerDynamicDifficulty / CTriggerSecret factory registrations,
RTTI/vtable dispatch, complete touch/spawn method digests, DATADESC map keys,
and typed game-event helpers. It does NOT reconstruct engine-owned classes
or prove native Windows game startup. Production input must be the owner's
unmodified original 2006 server.dll. --self-test uses synthetic scalars only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
SERVER_BASE=0x22000000
REGISTRY_GETTER=0x2228B110
ORIGINAL_METHODS={
  "dynamic_touch":(0x221CCB70,137,"d8f847721d4465de7a3915bd2897933f82f33d16324c0e5975d4e2a7cbec5abe"),
  "secret_touch":(0x221CCC10,67,"9b7765825c74c2c7e59ba80143af3b833fa3ceb57c62b0844da18dfa49a2fc18"),
  "dynamic_spawn":(0x221D0740,69,"3309b7d25f963dbcb77ae2e1d6f8101fc97cf4ae0a1635c3e172d06d0f011c27"),
  "secret_spawn":(0x221D0790,99,"561260115c2318a26943c49e29d43494f7ef6ceea2108f12acafde9f27d8c329"),
  "dynamic_event_writer":(0x222B64D0,218,"ced0637d1c68f71408cb3836f6d6f4757fead802a02b6cef6cd36350d815db65"),
  "secret_game_event_writer":(0x222B63B0,103,"e2945d32f935b69d714477daece0bb9c7043213be53afc9e87c39ed8b58a4941"),
  "mark_difficulty_manager":(0x222B93E0,5,"de588dcd40cb4837e51bcf7c3a550256faafcae88cb9ab44e6df9c9ac3ac57bf"),
  "dynamic_allocate":(0x221D54C0,67,"efd9b71bcd9c99d36598aa0d5b231bedf58ad3bc22da45e39e4bfc10d1a35ef2"),
  "secret_allocate":(0x221D5550,67,"94afb40d951eb20fab91a93f27513fdb043c1112b83415b4f5303fe848709692"),
  "dynamic_factory_create":(0x221D54A0,23,"6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9"),
  "secret_factory_create":(0x221D5530,23,"6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9"),
  "dynamic_destructor":(0x221D5510,30,"7a42d4784b57899101d20b1dd175b08969c9e3c932f66cf3c346de5a4626533c"),
  "secret_destructor":(0x221D55A0,30,"a85becf758e4065d8797d78a96326e353d719bdbdb4b071c332582c9a4a0dbc7"),
  "base_trigger_destructor":(0x221BE440,55,"8dd1922c7422c4bff012df312cd510ff40dffb76f42aa69c772deb493a7935d1"),
  "trigger_vector_purge":(0x22044C90,83,"21ca9b861643d42fa04c71a15bed163bb01899d87923126d98e3eeb6f8ca203e"),
  "base_trigger_datamap_init":(0x221D0C50,415,"4b241f1aec6b3ec76084d1d299c1a0513551685cfd9fb919ea296e42bda25ae8"),
  "trigger_enable":(0x221CFC70,58,"0ac157475a35f2d1891cd71722fbf3f92aba7e681b4ac0385b364b952de01237"),
  "trigger_disable":(0x221CFCB0,58,"5596e04afc248ff7fc90db294757a4440dac20fc5bbb3d396def3ed8ac027f05"),
  "trigger_toggle":(0x221CE660,80,"9066c5bdb0190a5ead275af0ca04a6277a81c710f13cf54ca044be8e3b474baf"),
  "trigger_input_thunks":(0x22060810,40,"f3e2e0d7e9570fbc90c89c244af37b938bc49d983465e998d6ca5ddb6a1cfaf6"),
  "base_toggle_datamap_tail":(0x221C0EFF,36,"9ff28959f19ab7d5aecdfb87a5717b4bfe8fda81f8152f7ed7110b375358c533"),
  "base_kill":(0x2213CDC0,80,"f46fec6fac87b05f1e8f2485b7ccf8b7e5748a8d4147ba27d01e46d4a0ff25e6"),
  "find_by_name":(0x221619E0,190,"4d2372f524268ab2ce2a8df08cbf48f0adaa3dae121c4c8963108759d4099bdd"),
  "get_handle":(0x22001B00,7,"0e8c31f2e49bf659ab4d49f07c04d6b715991eba3737f43ca492e011d3d9b8e7"),
  "is_player_true":(0x22253C30,3,"e18aacce29affcdddaa5f641ac1ec12552e0fe9d066e6408941b1792818b8619"),
  "is_player_false":(0x220017E0,3,"01cb47d078b4841b8408ec4fe278efa83115c6f6e101972987507d2b2b57dcf0"),
  "contact_touch_notify":(0x2221E740,41,"1c31ff18df45681bbacb993d69fb9fe4c16391394082a160262e815a194b7cbf"),
  "contact_start_touch_notify":(0x2221E770,55,"d9cfe927b17a5181d07c6635cab4e8deba30cb3d59da8df378cb72d6c0d503b5"),
  "inherited_start_touch":(0x221D0DF0,91,"79882580908eeaf2ae0699834bb06403cc8dde65c79eaa1a958d94d6a6e49368"),
  "trigger_handle_append":(0x220A22B0,100,"b657875404237bca8be9251e9847b5217a062cfd4fa7307c4858d91cecc3669a"),
  "void_output_fire":(0x22156900,84,"9831538fbf90681abd884187a73a262d05aa6031d2b58a5156dd3b8c717c0e26"),
  "inherited_end_touch":(0x221D3590,302,"a406f6f83bbde10c9ca03f6b28cdb3da0ab376c2b866e890bc90057cdc56c4cb"),
  "trigger_handle_identity_find":(0x220A21F0,131,"b29f95c429940c6193361871918e463b8a258ea174cfabd6971f38141bed9050"),
  "trigger_activate":(0x221CE3E0,98,"3e336eef7e02b9aa9feff8001b45dd597314be5699fdaf257edf5a0d72a64a39"),
  "multi_filter_activate":(0x2216B3E0,78,"8275ee5da2a598acc65fa3d2452d1fffcfe80d5bc2e3351cb1ecbbcb6f75a6c8"),
  "base_filter_allocate":(0x2216BBC0,61,"95043533ef67d48d781e0fcb6199c797c849df325d6dc64a5bc6e8e042f2caa2"),
  "base_filter_construct":(0x2216BC00,61,"640467fd2f279239b8c2f8dbbc26e966b2da027251976811f004ce1f5868cde0"),
  "multi_filter_allocate":(0x2216BCA0,88,"e9115d5b73adf58c4680504f2d239086c731ff947620268f4418167c9c1b2fee"),
  "name_filter_allocate":(0x2216BD60,66,"7c9fa593c490b51d6a13243787a278ad61d16ee14c7b01784040386b835f4f92"),
  "class_filter_allocate":(0x2216BEA0,67,"3de21a3a9907ef489be628d4d3feef96dbf876338c68c1084633cd509e0445ed"),
  "filter_handle_init":(0x22028F40,9,"b4863fcf4bff0a00d9a803b851c59bf0421e835e51d8e921d2a1ef38942775e8"),
  "base_filter_datamap_init":(0x2216B790,320,"0a5f3a996331ee97b74c34864aeb6411b3491656eaad0c929cb852a42a3459aa"),
  "trigger_filter":(0x221CE520,314,"460637a5e0d1149d281a94ed22991457e36daa803120f5bab56c0ec39eabda85"),
  "trigger_classname_compare":(0x221387F0,130,"5993dfdb12306b0c2d95ae3e02a79a50cec0bca37e65f2c5457cd090ffc86aa7"),
  "trigger_npc_query":(0x22139F60,19,"d5ea9253030765bcd70227e7860e4620758e52644047e564b5756b5d78eaef44"),
  "trigger_vehicle_query":(0x22035100,56,"53a6bdb618de249bc00c327d2326511c8dd944c26a2f4692f3e366f8fd756c6a"),
  "trigger_custom_filter":(0x2216B2A0,46,"e3cf0c2990ba6e8464a74f92378827c0e2d2123b27cab476a05fcdbc5acf2324"),
  "sequence_factory_create":(0x221b0190,23,"6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9"),
  "sequence_allocate":(0x221b01b0,61,"2a497e9523f4165248e97c3a94423401660ed92787096e16811ef89cf9211858"),
  "sequence_construct":(0x221b01f0,186,"63ea7f6a0b665f294a7b5afd05fdf73de0bf9015a12e3df6744bcedd3e0f0685"),
  "sequence_datamap_getter":(0x221ade90,6,"4725fcea4951ab15a4c353095f4b9ac3df933d1460d1130074dbf1e2fcb3fe31"),
  "sequence_datamap_init":(0x221b05d0,1542,"f9b3f1f7e67490ca4f1cff22736397fe552b4973a16a39a75dcbe7f0de1653bb"),
  "sequence_move_input":(0x221b0430,204,"8597b8b37c16d3107b4519ab220ec897ae7386771836aff10d83dec2e25af75c"),
  "sequence_begin_input":(0x221b0500,197,"5e247dff62d1b99839a4cfb9a1941e90dd20c9bdaadc1495fce6e3b6aa8bbeeb"),
  "sequence_cancel_input":(0x221af970,86,"aa5e777a8bc17a4dce8001ecc169dbd0fc345f44429bcc42e8d8847af76e26c2"),
  "sequence_install_think":(0x221b0410,29,"8291389d71c181925f8a90ac96c69cf7b22dc6984072d03e8fdddae231bfecb6"),
  "sequence_npc_true":(0x22044210,3,"e18aacce29affcdddaa5f641ac1ec12552e0fe9d066e6408941b1792818b8619"),
  "sequence_permission":(0x2203e690,167,"ac5cee4d2cc4274f7097e65b2f64b4df306264b8f55a206f9aa0fe9493740571"),
  "sequence_default_tick":(0x2214d7b0,179,"a197b8c975179097a8db85aef03d98e0c062077a83b2c326365b0eb3ad756cb9"),
  "sequence_possession":(0x221ae8f0,690,"7959f531e918810258c5d3316ed6a9071f5676e1800c0df9f7a5703cd4bafd63"),
  "sequence_sync":(0x221ae370,184,"6cf634e537015ea7087725034d5845ca14c7c9beea81e1b3c6f83f6949fb53d1"),
  "sequence_npc_goal_handle":(0x22031820,42,"66e7b24fd6194ebbf77c3f86c89fd3afaaa4b9c1b45063f6d04929f56083f5fb"),
  "sequence_select_npc":(0x221ae040,525,"ef7320f6e11b52e9b11d2c97d331b2e60efa20159024adc3bec1f7a6b236085e"),
  "sequence_commit_npc":(0x221ae250,100,"ec38e3277a8ce1182382640f55f48e87cf790746267be5a05f05ecb301ba72f6"),
  "sequence_name_radius_search":(0x22161d30,293,"9e313e59c7fbe8a24ff56596a83a204791836a0a8c4ac94862eaca164ef8b707"),
  "sequence_class_radius_search":(0x22161f70,271,"eb3fcbe5b58de5bbddbda9dfb7e81a00ecb07d8aaf214fa4aaafc32705208bd1"),
  "sequence_link_policy":(0x221aed80,251,"0fe3f4c3408689e43a747e3e910a8fad6b61c5ca26c1cd37360c5d07c858c2b6"),
  "sequence_acquisition_think":(0x221afc90,212,"32c783f72624edd7c93f65097c7377b1d9f213970fcabb49005e8ecaa08d8c20"),
  "sequence_failed_group":(0x221afc00,129,"327ae85fc9a7a7904334127ec696a3b6e6cfa3a83de8b6307515507f9dd67796"),
  "sequence_cancel_actor":(0x221af840,268,"455d82a4e51a4fed2a532ea7331c67daae7dcd6264ab23c9d7f828b3f0802e9c"),
  "sequence_default_think_wrapper":(0x22002ca0,10,"0c459ceb12ba13c47ce1349aa8fef3445670c88fa3cf44d310ba8c66f26a4920"),
  "sequence_primary_dispatch":(0x22222b90,159,"4a54921e4c08d9f979a4936bbc371ac866f8999960ddace5cb0a2f63ba93264a"),
  "sequence_context_dispatch":(0x22222c30,108,"f245d67593b46c7fcd441e17b389db2522f5dd014786663857ec0d4fdba23ed7"),
  "sequence_no_edict_dispatch":(0x2221c35f,11,"9c23c792f098465e7195ea4cb2beb52fc12515c5c333a507efe67762debd4ec8"),
  "sequence_spawn":(0x221b0be0,162,"3c4f39ea81c7c0947ff24a3589e856c98bb2fbf8e90df9eb7a942746866d89df"),
  "sequence_activate":(0x221ae430,185,"afac35e0b8d1af3292ba95737ad8d1aedf83c6b946ccf78f9d07c17d8d273aa1"),
  "sequence_name_or_class":(0x22162080,50,"80d1108d7549b153063fe08bc537d2c9597cb3662c58676339ca34b586710a11"),
  "sequence_factory_destroy":(0x221ae880,16,"ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
  "sequence_deleting_destructor":(0x221b02c0,30,"84f958d98ceec4a02885ba20d3d6f6e7e2410029c48fbf91e35fd23f7e5ea0e5"),
  "sequence_destructor":(0x221b02e0,63,"d9f389c3c9e2c9f92cb7bdef84277eab2d67cdce326c507472fbfa3eca2cdd02"),
  "sequence_update_on_remove":(0x221af950,20,"3dd7303fc511a55b3e40923d53fa5fc40df8833bb8bed3b8b97c03a96e3dd526"),
  "sequence_start_animation":(0x221aebc0,141,"71179552b2ba336d9b46df465ee2384488a31ab9d77af316c9d3215444ab17a0"),
  "sequence_play_done":(0x221ae2c0,140,"f6c568e1ea02f0004e3e1d76d8def58c8734afc79148465e20c457a106d32798"),
  "sequence_terminal_finish":(0x221afa70,394,"c90cb9ffac6fcf9292b9bba1ad8070776ebe348ea15f0624766d247c2455bde3"),
  "sequence_schedule_reset":(0x221af490,102,"07a9798beec834dccfb8551e5a89cd45f96e40e99aab7c6b87e777363f02755c"),
  "sequence_remove_think":(0x221c0d40,57,"07f35ebb311076475ecc55737b7e8a84a20380d13c163dfeaa6672c342f75bc7"),
  "sequence_handle_equal":(0x22028fe0,64,"379165bed57270bc9df20b615fa07fb3f61dc939f61ec1f6f3503e1652515f3f"),
  "sequence_npc_task106_case":(0x22054601,169,"fca27774d1234ccca366c103f04bef7259cc7f0add087fc4e2b2cbb923c2e927"),
  "sequence_npc_task108_case":(0x220546aa,109,"1bdd5f3b504b749d69576999fbe565412baf37769a3ddcd9abb0318c651f45df"),
  "sequence_npc_task109_case":(0x22054717,106,"ddfa07de6d9c2faf1136c3f4f1f55644151bc5f889d8ae3ecb23a8dde5b33e03"),
  "sequence_npc_task110_case":(0x22054781,90,"454d9fe9ed6815614cf0ac3d2896a6efe2056881444241c997ec1b4873b69131"),
  "sequence_group_ready":(0x221adef0,12,"7d1509fd885708ef67ca1b4cd152b0983ba10ee6a6205bb8c00d1bf889ea47ba"),
  "sequence_waiting":(0x221adf00,7,"bab98b95dd0fd5c04e42353458bbffa92caaefd05db8bca11908779c9ed77109"),
  "sequence_begin_output":(0x221adf70,16,"642e3595f71cb26e9f47a7c838ce487bbd3c6e0336ab408c2605e9dc431ce9b9"),
  "sequence_numbered_event":(0x221adec0,36,"667990e7ea437aaf3162218796f66917ae288409a6303b2ff78a6d40c9d82f4a"),
  "filter_test_activator":(0x2216B300,85,"6573bbfe1b919c5f45333c52c11acdc9a7bd51b89c40e39c14a4d8ef3656b6bf"),
  "base_filter_destructor":(0x2216BC40,54,"b2478b0e023b0501b74ed2f2a2aeddea39a22e0ebf7215261fd960d3b9c7425a"),
  "multi_filter_destructor":(0x2216BD00,54,"a2236c56ca198071b36f73169ff97317f1bbb54db4140a0973688fd09640ae2a"),
  "name_filter_destructor":(0x2216BE40,54,"43af6b3d48ca7ed82ff50b9d7d392d82400c89c2f1885471c0a15ff365aa1fa1"),
  "class_filter_destructor":(0x2216BF20,54,"cb9b85c3d1e444b9e863beb8de961a4ec7defa8cf0908d8cb4950e136cfc7da5"),
  "filter_output_destructor":(0x221544D0,38,"bd8b25bd36b1dcc840d9a0abf29fdd318cef1c00c716ba2d065c34c99729bc62"),
  "base_filter_factory_destroy":(0x2216B490,16,"ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
  "multi_filter_factory_destroy":(0x2216B4C0,16,"ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
  "name_filter_factory_destroy":(0x2216B4F0,16,"ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
  "class_filter_factory_destroy":(0x2216B520,16,"ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21"),
  "base_filter_raw":(0x2216B290,5,"f4c6d7ae520f88aecb3ea65952e885437fa4a6ce4b5c3439a161d1c5d8e42863"),
  "name_filter_raw":(0x2216BDB0,139,"0040852e4e4dd65b0a35c530d22e3894599441c146d65b3b23e02ab80a6c7f48"),
  "class_filter_raw":(0x2216BEF0,38,"ffeb9f83148ef71a7ab6d225b4dedcd6911da474b47e0b98322441d7323e7ba8"),
  "multi_filter_raw":(0x2216B5D0,238,"4c1eb94c4e0325d494a73c075963abff90ffd65f19f6bf38acd47b36438b32cb"),
  "filter_targetname_compare":(0x22138740,164,"eef6cd79c9ad7cbfeb9fccc98f7cc219c1093f25af066fdb82d6fb30cda7f6ef"),
  "filter_vehicle_handle_resolve":(0x220280A0,42,"3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6"),
}
FILTER_FACTORY_ROWS=(
    (0x223bbce0,0x22415930,0x224f8ad4,0x22414800,0x2216bba0,"filter_base"),
    (0x223bbd10,0x2241593c,0x224f8ad8,0x2241480c,0x2216bc80,"filter_multi"),
    (0x223bbd40,0x2241594c,0x224f8adc,0x22414818,0x2216bd40,"filter_activator_name"),
    (0x223bbd70,0x22415964,0x224f8ae0,0x22414824,0x2216be80,"filter_activator_class"),
)
FILTER_KEY_ROWS=(
    (0x224f8c4c,6,0x394,0x22414868,"Negated"),
    (0x224f8cb4,11,0x398,0x223f2de4,"OnPass"),
    (0x224f8d54,5,0x3c8,0x22414930,"FilterType"),
    (0x224f8d88,2,0x3cc,0x22414910,"Filter01"),
    (0x224f8dbc,2,0x3d0,0x224148f0,"Filter02"),
    (0x224f8df0,2,0x3d4,0x224148d0,"Filter03"),
    (0x224f8e24,2,0x3d8,0x224148b0,"Filter04"),
    (0x224f8e58,2,0x3dc,0x22414890,"Filter05"),
    (0x224f8ef4,2,0x3c8,0x22405d5c,"filtername"),
    (0x224f8f5c,2,0x3c8,0x2241494c,"filterclass"),
)
CUSTOM_FILTER_ROWS=(
    (0x224149a4,0x224bd6c4,0x224ecf0c,0x2216b290,".?AVCBaseFilter@@"),
    (0x22414ed4,0x224bd784,0x224f9080,0x2216bdb0,".?AVCFilterName@@"),
    (0x2241516c,0x224bd7e4,0x224f909c,0x2216bef0,".?AVCFilterClass@@"),
    (0x22414c3c,0x224bd724,0x224f9060,0x2216b5d0,".?AVCFilterMultiple@@"),
)
PLAYER_ROWS=(
    (0x224aec3c,0x224d3ff0,0x2251e69c,0x22253c30,".?AVCHL2_Player@@"),
    (0x2245222c,0x224c8498,0x224e5e60,0x22253c30,".?AVCBasePlayer@@"),
    (0x2240ac2c,0x224bbc10,0x224d88d4,0x220017e0,".?AVCBaseEntity@@"),
    (0x2243b96c,0x224c3f84,0x2250d3fc,0x220017e0,".?AVCTriggerDynamicDifficulty@@"),
    (0x2243bc0c,0x224c3fe0,0x2250d424,0x220017e0,".?AVCTriggerSecret@@"),
    (0x2241f0d4,0x224bee34,0x224fb494,0x220017e0,".?AVCLogicAuto@@"),
    (0x22420e14,0x224bf3f0,0x224fd920,0x220017e0,".?AVCTimerEntity@@"),
    (0x22422294,0x224bf6d0,0x224fda0c,0x220017e0,".?AVCLogicBranch@@"),
    (0x22421d74,0x224bf618,0x224fd9d4,0x220017e0,".?AVCLogicCase@@"),
    (0x22422004,0x224bf674,0x224fd9f0,0x220017e0,".?AVCLogicCompare@@"),
    (0x22421ae4,0x224bf5bc,0x224fd9b8,0x220017e0,".?AVCMathCounter@@"),
    (0x22420b84,0x224bf394,0x224fd8fc,0x220017e0,".?AVCLogicCompareInteger@@"),
    (0x2241f9e4,0x224bf2dc,0x224fb91c,0x220017e0,".?AVCLogicAutosave@@"),
    (0x2241ed04,0x224bed98,0x224fb118,0x220017e0,".?AVCLogicNavigation@@"),
    (0x22422534,0x224bf75c,0x224fda28,0x220017e0,".?AVCLogicMenuListener@@"),
    (0x22421334,0x224bf4a8,0x224fd960,0x220017e0,".?AVCMathRemap@@"),
    (0x224215c4,0x224bf504,0x224fd97c,0x220017e0,".?AVCMathColorBlend@@"),
    (0x224228cc,0x224bf844,0x224fdabc,0x220017e0,".?AVCLogicRelay@@"),
)
BASE_TOGGLE_ROWS=(0x22508C74,780,"e2ac6ab967effdb36eae1c1a1af07c5b2cb753716a9616a8c0dc514cca019f0b")
CONTROL_ROWS=(
    (0x2250BAA0,0x223EDD00,0x22060810,"Enable"),
    (0x2250BAD4,0x223EDCE8,0x22060820,"Disable"),
    (0x2250BB08,0x223D4914,0x22060830,"Toggle"),
)
CONTROL_SLOTS=((8,0x22001B00),(0x50,0x221435B0),(0x28C,0x221CFC70),
               (0x290,0x221CFCB0),(0x294,0x221CE660))
CONTROL_CLASSES=((0x2243B96C,0x221CCB60),(0x2243BC0C,0x221CCC00))
SUBJECT_SITES=(0x222B63EC,0x222B6529)

CLASSES={
  "trigger_dynamic_difficulty":(0x223BE4A0,0x2243CAF4,0x2250B404,
     0x2243B96C,0x224C3F84,0x2250D3FC,
     ".?AVCTriggerDynamicDifficulty@@",0x221CCB70,0x221D0740),
  "trigger_secret":(0x223BE4D0,0x2243CB10,0x2250B408,
     0x2243BC0C,0x224C3FE0,0x2250D424,
     ".?AVCTriggerSecret@@",0x221CCC10,0x221D0790),
}
FIELDS=(
    (0x2250CCFC,1,0x470,0x224391e8,"m_estimatedTimeToGetHere",
      0x224391d8,"EstimatedTime"),
    (0x2250CD30,5,0x474,0x224391c0,"m_estimatedDifficulty",
      0x224391ac,"EstimatedDifficulty"),
    (0x2250CD64,5,0x478,0x22439194,"m_finishesPlaythrough",
      0x22439188,"EndOfGame"),
)
class EvidenceRefused(ValueError):pass

class OriginalPE:
    def __init__(self,data:bytes):
        self.data=data
        if data[:2]!=b"MZ" or len(data)<0x100:
            raise EvidenceRefused("not a PE image")
        pe=struct.unpack_from("<I",data,0x3C)[0]
        if pe+24>len(data) or data[pe:pe+4]!=b"PE\0\0":
            raise EvidenceRefused("not a complete PE")
        machine,nsections=struct.unpack_from("<HH",data,pe+4)
        optlen=struct.unpack_from("<H",data,pe+20)[0]
        op=pe+24
        if machine!=0x14C or not 1<=nsections<=96 or (
                op+optlen+nsections*40>len(data)) or (
                struct.unpack_from("<H",data,op)[0]!=0x10B):
            raise EvidenceRefused("not original PE32 x86")
        self.base=struct.unpack_from("<I",data,op+28)[0]
        if self.base!=SERVER_BASE:raise EvidenceRefused("unexpected PE base")
        self.sections=[]
        for idx in range(nsections):
            off=op+optlen+idx*40
            name=data[off:off+8].split(b"\0")[0].decode("ascii","replace")
            size,va,rawsize,raw=struct.unpack_from("<IIII",data,off+8)
            self.sections.append((name,va,max(size,rawsize),raw,rawsize))
    def at(self,va:int,count:int)->bytes:
        if count<0 or count>4096:raise EvidenceRefused("invalid read size")
        rva=va-self.base
        for name,start,size,raw,rawsize in self.sections:
            if start<=rva and rva-start+count<=min(rawsize,size):
                off=raw+rva-start
                if off+count<=len(self.data):return self.data[off:off+count]
        raise EvidenceRefused(f"unmapped original RVA {rva:#x}")
    def u32(self,va:int)->int:
        return struct.unpack("<I",self.at(va,4))[0]
    def text(self,va:int,value:str)->bool:
        return self.at(va,len(value)+1)==value.encode("ascii")+b"\0"

def verify_datadesc(read_u32,read_text):
    for base,typ,offset,int_name,int_str,ext_name,ext_str in FIELDS:
        if (read_u32(base),read_u32(base+4),read_u32(base+8),
            read_u32(base+20))!=(typ,int_name,offset,ext_name):
            raise EvidenceRefused("original trigger DATADESC field shape mismatch")
        if not read_text(int_name,int_str) or not read_text(ext_name,ext_str):
            raise EvidenceRefused("original trigger DATADESC field key mismatch")
    return len(FIELDS)

def verify_secret_spawn_control(at):
    """Validate only three original call edges and tiny opcode witnesses.

    The whole 99-byte method is separately SHA-pinned before this helper;
    this function's synthetic tests never need or materialize original DLLs.
    """
    edges=(
        (0x221D07A6,0x22286BF0), # deferred remove
        (0x221D07D2,0x221CFCF0), # base Trigger::Spawn
        (0x221D07E8,0x222B63B0), # game event writer
    )
    for site,target in edges:
        raw=at(site,5)
        if len(raw)!=5 or raw[0]!=0xE8 or (
                site+5+struct.unpack_from("<i",raw,1)[0])!=target:
            raise EvidenceRefused("CTriggerSecret::Spawn callee changed")
    # Read the original field displacement and a few independent
    # opcode/predicate witnesses instead of storing entire code runs.
    if (at(0x221D07D7,2)!=bytes((0x8A,0x8E)) or
        struct.unpack("<I",at(0x221D07D9,4))[0]!=0x120 or
        at(0x221D07DD,3)!=bytes((0xC0,0xE9,0x03)) or
        at(0x221D07E0,3)!=bytes((0xF6,0xC1,0x01)) or
        at(0x221D07E3,2)!=bytes((0x74,0x0B))):
        raise EvidenceRefused("CTriggerSecret::Spawn bit3 field gate changed")
    if at(0x221D07E6,2)!=bytes((0x6A,0x0B)):
        raise EvidenceRefused("CTriggerSecret::Spawn descriptor11 changed")
    return len(edges)

def verify_dynamic_spawn_control(at,read_text):
    """Fully SHA-pinned dynamic Spawn, then separate original flag warnings."""
    edges=((0x221D0756,0x22286BF0),
           (0x221D0762,0x221CFCF0))
    for site,target in edges:
        code=at(site,5)
        if len(code)!=5 or code[0]!=0xE8 or (
            site+5+struct.unpack_from("<i",code,1)[0])!=target:
            raise EvidenceRefused("CTriggerDynamicDifficulty Spawn edge changed")
    if (at(0x221D0767,2)!=bytes((0x8B,0x8E)) or
        struct.unpack("<I",at(0x221D0769,4))[0]!=0xB4 or
        at(0x221D076D,3)!=bytes((0xC1,0xE9,0x0A)) or
        at(0x221D0770,3)!=bytes((0xF6,0xC1,0x01))):
        raise EvidenceRefused("original this+0xB4 bit10 gate changed")
    if (at(0x221D0775,1)!=bytes((0x68,)) or
        struct.unpack("<I",at(0x221D0776,4))[0]!=0x22438650 or
        not read_text(0x22438650,"DD Trigger with don't destroy set\n")):
        raise EvidenceRefused("original dynamic Spawn warning string changed")
    return len(edges)

def verify_trigger_control(read_u32,read_text):
    for base,external,handler,label in CONTROL_ROWS:
        if tuple(read_u32(base+offset) for offset in (0,8,16,20,28)) != (
                0,0,0x80001,external,handler):
            raise EvidenceRefused("inherited trigger input row changed "+label)
        if not read_text(external,label):
            raise EvidenceRefused("inherited trigger input name changed "+label)
    for table,getter in CONTROL_CLASSES:
        for slot,target in CONTROL_SLOTS+((0x34,getter),):
            if read_u32(table+slot)!=target:
                raise EvidenceRefused("trigger control virtual slot changed")
    return len(CONTROL_ROWS),len(CONTROL_CLASSES)*(len(CONTROL_SLOTS)+1)

def verify_contact_slots(read_u32):
    for table,touch in ((0x2243B96C,0x221CCB70),(0x2243BC0C,0x221CCC10)):
        if (read_u32(table+0x88),read_u32(table+0x160),read_u32(table+0x164),read_u32(table+0x168),read_u32(table+0x298))!=(0x221CE3E0,0x221D0DF0,touch,0x221D3590,0x221CE520):
            raise EvidenceRefused("campaign contact dispatch slot changed")
    return 10

def verify_filter_factory_keys(read_u32,read_text):
    for site,name_ptr,obj,table,create,name in FILTER_FACTORY_ROWS:
        if (read_u32(site+8),read_u32(site+13),read_u32(obj),read_u32(table))!=(name_ptr,obj,table,create) or not read_text(name_ptr,name):
            raise EvidenceRefused("filter factory registration changed "+name)
    for row,typ,offset,external,name in FILTER_KEY_ROWS:
        if (read_u32(row),read_u32(row+8),read_u32(row+20))!=(typ,offset,external) or not read_text(external,name):
            raise EvidenceRefused("filter map key changed "+name)
    if tuple(read_u32(0x224f8c80+off) for off in (0,8,16,20,28))!=(20,0,0x80001,0x22414844,0x2216b300) or not read_text(0x22414844,"TestActivator"):
        raise EvidenceRefused("filter TestActivator row changed")
    if not read_text(0x2241487c,"OnFail"):
        raise EvidenceRefused("runtime OnFail descriptor literal changed")
    return len(FILTER_FACTORY_ROWS),len(FILTER_KEY_ROWS)

FILTER_DESTRUCTORS={0x224149a4:0x2216bc40,0x22414c3c:0x2216bd00,0x22414ed4:0x2216be40,0x2241516c:0x2216bf20}
FILTER_REMOVAL_SLOTS=((0x14,0x22001ae0),(0x50,0x221435b0),(0x17c,0x221446f0),(0x224,0x2214a060),(0x268,0x220019f0))
SEQUENCE_WORDS={
    # RunTask 220538d0: byte table indexed by task-2, then jump table.
    # Pin neighboring bytes too, not guessed task numbers.
    0x22054bac:0x1b1a2519,0x22054bb0:0x1d0d251c,
    0x22054b10:0x22054601,0x22054b14:0x220546aa,0x22054b18:0x22054717,0x22054b1c:0x22054781,
    0x223bd498:0x2242ee1c,0x223bd49d:0x22505008,0x22505008:0x2242ee70,0x2242ee70:0x221b0190,0x2242ee74:0x221ae880,
    0x2242f668:0x224c1b4c,0x224c1b4c:0,0x224c1b50:0,0x224c1b58:0x22505014,
    **{0x221aeba4+4*i:target for i,target in enumerate((0x221aeb7e,0x221aeaef,0x221aeafa,0x221aeb06,0x221aeb12,0x221aeb7e))},
    0x223d2590:0,0x223d22f8:0x3f800000,0x223d9788:0x3dcccccd,
    0x2242f66c:0x221b02c0,0x2242f66c+0x17c:0x221af950,0x2242f66c+8:0x22001b00,0x2242f66c+0x34:0x221ade90,0x2242f66c+0x90:0x22141400,0x2242f66c+0xc0:0x22001740,0x2242f66c+0xbc:0x22002ca0,0x2242f66c+0x64:0x221b0be0,0x2242f66c+0x88:0x221ae430,
    **{vt+off:target for vt in (0x2248df04,0x224954a4,0x2249aaa4,0x224a416c,0x2248c514,0x2248e83c,0x2249964c,0x224a4a4c)
        for off,target in ((0xf4,0x22044210),(0x574,0x2203e690))},
}
SEQUENCE_KEYS=(
    (2,0x394,0x2242fc74,"m_iszIdle"),(2,0x398,0x2242fc68,"m_iszPlay"),
    (2,0x39c,0x2242fc58,"m_iszPostIdle"),(2,0x3a0,0x2242fc48,"m_iszCustomMove"),
    (2,0x3a4,0x2242fc38,"m_iszNextScript"),(2,0x3a8,0x2242f52c,"m_iszEntity"),
    (5,0x3ac,0x2242fc2c,"m_fMoveTo"),(1,0x3b0,0x223ec258,"m_flRadius"),
    (1,0x3b4,0x2242f5d0,"m_flRepeat"),
)
SEQUENCE_TEXT={0x2242ee1c:"scripted_sequence",
    0x2242fe3c:"MoveToPosition",0x2242fe18:"BeginSequence",0x2242fdf4:"CancelSequence",0x2250501c:".?AVCAI_ScriptedSequence@@",
    0x2242fdd0:"OnBeginSequence",0x2242fdb0:"OnEndSequence",
    **{0x2242fd8c-(i-1)*0x24:f"OnScriptEvent{i:02d}" for i in range(1,9)}}
def verify_sequence_words():
    words=dict(SEQUENCE_WORDS);texts=dict(SEQUENCE_TEXT)
    for i,(typ,offset,external,name) in enumerate(SEQUENCE_KEYS):
        row=0x22505794+i*0x34
        words.update({row:typ,row+8:offset,row+16:0x60001,row+20:external})
        texts[external]=name
    return words,texts

def verify_scripted_sequence(read_u32,read_text):
    words,texts=verify_sequence_words()
    if any(read_u32(va)!=value for va,value in words.items()):
        raise EvidenceRefused("scripted sequence RTTI, factory or key row changed")
    if any(not read_text(va,value) for va,value in texts.items()):
        raise EvidenceRefused("scripted sequence key or output literal changed")
    return len(SEQUENCE_KEYS)

def verify_custom_filter_slots(read_u32,read_text):
    for table,col,typedesc,target,name in CUSTOM_FILTER_ROWS:
        if (read_u32(table-4),read_u32(col),read_u32(col+4),read_u32(col+12),
            read_u32(table+0x28c),read_u32(table+8),read_u32(table+0x88))!=(
                col,0,0,typedesc,target,0x22001b00,0x2216b3e0 if table==0x22414c3c else 0x22146bd0):
            raise EvidenceRefused("custom filter RTTI or predicate slot changed "+name)
        if read_u32(table)!=FILTER_DESTRUCTORS[table] or any(read_u32(table+slot)!=target for slot,target in FILTER_REMOVAL_SLOTS):
            raise EvidenceRefused("filter removal/destructor slot changed "+name)
        if not read_text(typedesc+8,name):
            raise EvidenceRefused("custom filter RTTI name changed "+name)
    if not read_text(0x223d994c,"!player"):
        raise EvidenceRefused("custom filter special player literal changed")
    return len(CUSTOM_FILTER_ROWS)

def verify_player_predicates(read_u32,read_text):
    for table,col,typedesc,target,name in PLAYER_ROWS:
        if (read_u32(table-4),read_u32(col),read_u32(col+4),
            read_u32(col+12),read_u32(table+0x120),read_u32(table+8),read_u32(table+0xf4))!=(col,0,0,typedesc,target,0x22001B00,0x22001780):
            raise EvidenceRefused("primary player predicate slot/RTTI changed "+name)
        if not read_text(typedesc+8,name):
            raise EvidenceRefused("player predicate RTTI name changed "+name)
    return len(PLAYER_ROWS)

def verify_subject_operands(at,read_text):
    # The other original event helper at 222b6420 uses advisor.
    # These two helpers instead push the actual subject literal.
    for site in SUBJECT_SITES:
        operand=at(site,5)
        if operand!=b"\x68"+struct.pack("<I",0x223EC29C):
            raise EvidenceRefused("trigger event subject operand changed")
    if not read_text(0x223EC29C,"subject"):
        raise EvidenceRefused("trigger event subject literal changed")
    return len(SUBJECT_SITES)

def verify_raw(client:bytes,server:bytes)->dict:
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise EvidenceRefused("original client SHA-256 mismatch")
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
        raise EvidenceRefused("original server SHA-256 mismatch")
    pe=OriginalPE(server)
    for label,(va,length,expected) in ORIGINAL_METHODS.items():
        if hashlib.sha256(pe.at(va,length)).hexdigest()!=expected:
            raise EvidenceRefused("changed full original method "+label)
    for name,(register,name_ptr,factory,vtable,col,typedesc,rtti,touch,spawn) in CLASSES.items():
        if not pe.text(name_ptr,name):
            raise EvidenceRefused(name+" original class-name string mismatch")
        raw=pe.at(register,22)
        if not(raw[:1]==b"\xE8" and raw[5:8]==b"\x8B\x10\x68" and
               raw[12:13]==b"\x68" and raw[17:]==b"\x8B\xC8\xFF\x12\xC3"):
            raise EvidenceRefused(name+" factory registration not recognized")
        rel=struct.unpack_from("<i",raw,1)[0]
        if(register+5+rel,name_ptr,factory)!=(
            REGISTRY_GETTER,struct.unpack_from("<I",raw,8)[0],
            struct.unpack_from("<I",raw,13)[0]):
            raise EvidenceRefused(name+" factory pointer and registry mismatch")
        if pe.u32(vtable-4)!=col or pe.u32(col+12)!=typedesc:
            raise EvidenceRefused(name+" RTTI object locator mismatch")
        if not pe.text(typedesc+8,rtti):
            raise EvidenceRefused(name+" RTTI class string mismatch")
        if pe.u32(vtable+89*4)!=touch or pe.u32(vtable+25*4)!=spawn:
            raise EvidenceRefused(name+" virtual touch/spawn slot mismatch")
    spawn_edges=verify_secret_spawn_control(pe.at)
    dynamic_spawn_edges=verify_dynamic_spawn_control(pe.at,pe.text)
    count=verify_datadesc(pe.u32,pe.text)
    control_rows,control_slots=verify_trigger_control(pe.u32,pe.text)
    subject_sites=verify_subject_operands(pe.at,pe.text)
    player_rows=verify_player_predicates(pe.u32,pe.text)
    custom_rows=verify_custom_filter_slots(pe.u32,pe.text)
    filter_factories,filter_keys=verify_filter_factory_keys(pe.u32,pe.text)
    sequence_keys=verify_scripted_sequence(pe.u32,pe.text)
    if not pe.text(0x22423278,"func_pushable"):
        raise EvidenceRefused("original trigger pushable literal changed")
    contact_slots=verify_contact_slots(pe.u32)
    va,length,digest=BASE_TOGGLE_ROWS
    if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
        raise EvidenceRefused("changed inherited CBaseToggle datamap")
    if any((pe.u32(va+i*0x34+16)>>16)&8 for i in range(15)):
        raise EvidenceRefused("unexpected CBaseToggle input shadow")
    # Cross-check the exact event names/types/field labels the two
    # complete original event helpers use.
    names={
        0x2245C82C:"dynamic_difficulty",0x2245C824:"game",
        0x2245C84C:"descriptor",0x2245C858:"owner",
        0x223EC29C:"subject",0x2245C878:"decision",
        0x2240B13C:"name",0x223E0BBC:"value",
        0x22444D70:"x",0x22444D5C:"y",0x2245C88C:"z",
    }
    for va,label in names.items():
        if not pe.text(va,label):
            raise EvidenceRefused("original event field witness changed "+label)
    return {
        "status":"ORIGINAL_2006_TRIGGER_GAMEPLAY_EVIDENCE_VERIFIED",
        "full_native_method_hashes":len(ORIGINAL_METHODS),
        "full_original_method_bytes":sum(x[1] for x in ORIGINAL_METHODS.values()),
        "registered_trigger_classnames":list(CLASSES),
        "original_datadesc_keyvalues":[row[6] for row in FIELDS],
        "dynamic_touch_virtual_slot":89,
        "secret_touch_virtual_slot":89,
        "secret_spawn_descriptor":11,
        "secret_spawn_game_event_bitmask":8,
        "secret_spawn_ordered_direct_calls":spawn_edges,
        "dynamic_spawn_ordered_direct_calls":dynamic_spawn_edges,
        "dynamic_spawn_warning_flag_mask":1024,
        "original_event_names":["dynamic_difficulty","game"],
        "trigger_control_input_rows":control_rows,
        "trigger_control_virtual_slots":control_slots,
        "subject_operand_sites":subject_sites,
        "player_predicate_primary_rows":player_rows,
        "custom_filter_primary_rows":custom_rows,
        "filter_factory_rows":filter_factories,
        "filter_static_key_rows":filter_keys,
        "scripted_sequence_static_keys":sequence_keys,
        "campaign_contact_dispatch_slots":contact_slots,
        "base_toggle_rows_without_inputs":15,
        "original_source_hashes":{"client":CLIENT_SHA,"server":SERVER_SHA},
        "limitations":"No real game entity creation, map load, AI or playable DLL Init."
    }

def self_test()->None:
    values={}
    texts={}
    for base,typ,offset,name,name_str,external,external_str in FIELDS:
        values.update({base:typ,base+4:name,base+8:offset,base+20:external})
        texts.update({name:name_str,external:external_str})
    assert verify_datadesc(lambda va:values.get(va,0),
                           lambda va,s:texts.get(va)==s)==3
    for base,*_ in FIELDS:
        bad=dict(values);bad[base+8]+=4
        try:verify_datadesc(lambda va:bad.get(va,0),
                            lambda va,s:texts.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated original field offset accepted")
    for key in list(texts):
        bad=dict(texts);bad[key]="invented"
        try:verify_datadesc(lambda va:values.get(va,0),
                            lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("forged original map key accepted")
    # Three synthetic CALL rel32 edges and disjoint branch/opcode
    # witnesses, with each mutated independently.
    chunks={}
    for site,target in ((0x221D07A6,0x22286BF0),
                        (0x221D07D2,0x221CFCF0),
                        (0x221D07E8,0x222B63B0)):
        chunks[site]=bytes((0xE8,))+struct.pack("<i",target-site-5)
    chunks.update({
        0x221D07D7:bytes((0x8A,0x8E)),
        0x221D07D9:struct.pack("<I",0x120),
        0x221D07DD:bytes((0xC0,0xE9,0x03)),
        0x221D07E0:bytes((0xF6,0xC1,0x01)),
        0x221D07E3:bytes((0x74,0x0B)),
        0x221D07E6:bytes((0x6A,0x0B))
    })
    get=lambda va,n:chunks.get(va,b"")[:n]
    assert verify_secret_spawn_control(get)==3
    for site in chunks:
        altered=dict(chunks)
        altered[site]=bytes((chunks[site][0]^1,))+chunks[site][1:]
        try:verify_secret_spawn_control(
            lambda va,n:altered.get(va,b"")[:n])
        except EvidenceRefused:pass
        else:raise AssertionError("mutated secret Spawn gate accepted")
    dynamic_chunks={}
    for site,target in ((0x221D0756,0x22286BF0),
                        (0x221D0762,0x221CFCF0)):
        dynamic_chunks[site]=bytes((0xE8,))+struct.pack("<i",target-site-5)
    dynamic_chunks.update({
        0x221D0767:bytes((0x8B,0x8E)),
        0x221D0769:struct.pack("<I",0xB4),
        0x221D076D:bytes((0xC1,0xE9,0x0A)),
        0x221D0770:bytes((0xF6,0xC1,0x01)),
        0x221D0775:bytes((0x68,)),
        0x221D0776:struct.pack("<I",0x22438650)
    })
    read_dynamic=lambda va,n:dynamic_chunks.get(va,b"")[:n]
    warning=lambda va,value:va==0x22438650 and value=="DD Trigger with don't destroy set\n"
    assert verify_dynamic_spawn_control(read_dynamic,warning)==2
    for site in dynamic_chunks:
        altered=dict(dynamic_chunks)
        altered[site]=bytes((dynamic_chunks[site][0]^1,))+dynamic_chunks[site][1:]
        try:verify_dynamic_spawn_control(
            lambda va,n:altered.get(va,b"")[:n],warning)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated original dynamic Spawn flag accepted")
    try:verify_dynamic_spawn_control(read_dynamic,lambda va,value:False)
    except EvidenceRefused:pass
    else:raise AssertionError("forged original dynamic warning text accepted")
    control_words={}
    control_text={}
    for base,external,handler,label in CONTROL_ROWS:
        control_words.update({base:0,base+8:0,base+16:0x80001,
                              base+20:external,base+28:handler})
        control_text[external]=label
    for table,getter in CONTROL_CLASSES:
        for slot,target in CONTROL_SLOTS+((0x34,getter),):
            control_words[table+slot]=target
    assert verify_trigger_control(control_words.get,
                                  lambda va,s:control_text.get(va)==s)==(3,12)
    for key in control_words:
        bad=dict(control_words);bad[key]^=1
        try:verify_trigger_control(bad.get,lambda va,s:control_text.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated trigger input row or slot accepted")
    for key in control_text:
        bad=dict(control_text);bad[key]="invented"
        try:verify_trigger_control(control_words.get,lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated trigger input name accepted")
    contact_words={0x2243B96C+0x160:0x221D0DF0,0x2243B96C+0x164:0x221CCB70,
                   0x2243BC0C+0x160:0x221D0DF0,0x2243BC0C+0x164:0x221CCC10}
    contact_words.update({0x2243B96C+0x298:0x221CE520,0x2243BC0C+0x298:0x221CE520})
    contact_words.update({0x2243B96C+0x168:0x221D3590,0x2243BC0C+0x168:0x221D3590})
    contact_words.update({0x2243B96C+0x88:0x221CE3E0,0x2243BC0C+0x88:0x221CE3E0})
    assert verify_contact_slots(contact_words.get)==10
    for key in contact_words:
        bad=dict(contact_words);bad[key]^=1
        try:verify_contact_slots(bad.get)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated campaign contact slot accepted")
    factory_words={}
    factory_text={0x2241487c:"OnFail"}
    for site,name_ptr,obj,table,create,name in FILTER_FACTORY_ROWS:
        factory_words.update({site+8:name_ptr,site+13:obj,obj:table,table:create})
        factory_text[name_ptr]=name
    for row,typ,offset,external,name in FILTER_KEY_ROWS:
        factory_words.update({row:typ,row+8:offset,row+20:external})
        factory_text[external]=name
    factory_words.update({0x224f8c80:20,0x224f8c88:0,0x224f8c90:0x80001,0x224f8c94:0x22414844,0x224f8c9c:0x2216b300})
    factory_text[0x22414844]="TestActivator"
    assert verify_filter_factory_keys(factory_words.get,lambda va,s:factory_text.get(va)==s)==(4,10)
    for key in factory_words:
        bad=dict(factory_words);bad[key]^=1
        try:verify_filter_factory_keys(bad.get,lambda va,s:factory_text.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated filter factory or key accepted")
    seq_words,seq_text=verify_sequence_words()
    assert verify_scripted_sequence(seq_words.get,lambda va,s:seq_text.get(va)==s)==9
    for key in seq_words:
        bad=dict(seq_words);bad[key]^=1
        try:verify_scripted_sequence(bad.get,lambda va,s:seq_text.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated sequence word accepted")
    for key in seq_text:
        bad=dict(seq_text);bad[key]="invalid"
        try:verify_scripted_sequence(seq_words.get,lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated sequence literal accepted")
    filter_words={}
    filter_text={0x223d994c:"!player"}
    for table,col,typedesc,target,name in CUSTOM_FILTER_ROWS:
        filter_words.update({table-4:col,col:0,col+4:0,col+12:typedesc,table+0x28c:target,
                             table+8:0x22001b00,table+0x88:0x2216b3e0 if table==0x22414c3c else 0x22146bd0})
        filter_words[table]=FILTER_DESTRUCTORS[table]
        filter_words.update({table+slot:target for slot,target in FILTER_REMOVAL_SLOTS})
        filter_text[typedesc+8]=name
    assert verify_custom_filter_slots(filter_words.get,lambda va,s:filter_text.get(va)==s)==4
    for key in filter_words:
        bad=dict(filter_words);bad[key]^=1
        try:verify_custom_filter_slots(bad.get,lambda va,s:filter_text.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated custom filter slot accepted")
    for key in filter_text:
        bad=dict(filter_text);bad[key]="invalid"
        try:verify_custom_filter_slots(filter_words.get,lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated custom filter literal accepted")
    player_words={}
    player_text={}
    for table,col,typedesc,target,name in PLAYER_ROWS:
        player_words.update({table-4:col,col:0,col+4:0,col+12:typedesc,table+0x120:target,table+8:0x22001B00,table+0xf4:0x22001780})
        player_text[typedesc+8]=name
    assert verify_player_predicates(player_words.get,lambda va,s:player_text.get(va)==s)==18
    for key in player_words:
        bad=dict(player_words);bad[key]^=1
        try:verify_player_predicates(bad.get,lambda va,s:player_text.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated primary player predicate accepted")
    for key in player_text:
        bad=dict(player_text);bad[key]="invented"
        try:verify_player_predicates(player_words.get,lambda va,s:bad.get(va)==s)
        except EvidenceRefused:pass
        else:raise AssertionError("mutated predicate RTTI name accepted")
    subject_chunks={site:b"\x68"+struct.pack("<I",0x223EC29C)
                    for site in SUBJECT_SITES}
    assert verify_subject_operands(lambda va,n:subject_chunks[va][:n],
                                  lambda va,s:va==0x223EC29C and s=="subject")==2
    for site in SUBJECT_SITES:
        bad=dict(subject_chunks)
        bad[site]=b"\x68"+struct.pack("<I",0x2245C884)
        try:verify_subject_operands(lambda va,n:bad[va][:n],
                                   lambda va,s:True)
        except EvidenceRefused:pass
        else:raise AssertionError("advisor operand accepted for subject")
    try:verify_subject_operands(lambda va,n:subject_chunks[va][:n],
                               lambda va,s:False)
    except EvidenceRefused:pass
    else:raise AssertionError("changed subject literal accepted")
    try:verify_raw(b"synthetic",b"synthetic")
    except EvidenceRefused:pass
    else:raise AssertionError("synthetic DLL accepted as original")
    print("PASS original-free 3 DATADESC fields, 3 original secret Spawn CALL rel32 edges, source bit3/descriptor11 witnesses, dynamic bit10 warning, 3 inherited input rows, 12 virtual slots, 2 subject operands and SHA refusal")

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",type=Path)
    ap.add_argument("--server",type=Path)
    ap.add_argument("--self-test",action="store_true")
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    if args.self_test:
        self_test();return 0
    if not args.client or not args.server:
        ap.error("original --client and --server are both required")
    try:
        result=verify_raw(args.client.read_bytes(),args.server.read_bytes())
    except (OSError,EvidenceRefused,struct.error) as e:
        ap.exit(1,"ORIGINAL TRIGGER EVIDENCE REFUSED: "+str(e)+"\n")
    out=json.dumps(result,indent=2)+"\n"
    if args.output:args.output.write_text(out)
    else:print(out,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
