#!/usr/bin/env python3
"""Phase1699: original owner-private client PE32 auxiliary dynamic receivers.

All caller/factory bytes and their target fields are verified against the
unchanged 2006 client.dll SHA-256, with full original function/callsite hashes.
No DLL code is copied into the repository or public CI.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_client_aux_recvtables import verify as verify_aux
from audit_sin_original_client_class_registry import need

NESTED_ARRAY_BUILDER=0x24126E90
TYPE4_ARRAY_BUILDER=0x24126BD0
OVERLAY_BUILDER=0x241271B0
TYPE5=5
TYPE4=4
TYPE0=0

# Exact original compiler code windows. The first three PUSH count, stride,
# original parent-object byte offset; the number at native child prop+0x2C
# starts from ZERO and increments by stride. The 351B array factory
# is common with the 11 class-attached dynamic nested arrays from Phase1697.
# (source aux table, name, source descriptor VA, source code VA, bytes,
#  whole original SHA, elements, stride, OUTER base, original child type)
NESTED_ARRAYS=(
 ("DT_BCCLocalPlayerExclusive","m_hMyWeapons",0x24337540,
  0x240C37C4,43,"b43812d7cb0a33498b14ac6ab4df7c2f6ba6cd13d71663d3f20d73868184436f",
  48,4,0xC80,TYPE0),
 ("DT_Local","m_chAreaBits",0x2433936C,
  0x240D332A,40,"e7e5b9dba03989a0142abf231cad509801b1d48357855925105f8988afe8b228",
  32,1,4,TYPE0),
 ("DT_LocalPlayerExclusive","m_iAmmo",0x2433A058,
  0x240D3997,43,"1aace3b00eba9bba81a29f467f576d1be69a0f1eebb323e4e0ab474d30580ebc",
  32,4,0xC00,TYPE0),
)
# Original type4 factory makes NO dynamic nested child RecvTable.
SPECIAL_ARRAY4={
 "source_aux_table":"DT_LocalPlayerExclusive",
 "original_property_name":"m_hViewModel",
 "source_descriptor_va":0x2433A328,
 "source_callsite_va":0x240D3AFB,
 "source_code_bytes":21,
 "source_code_sha256":"8b9ac443d2e549508c2abf94f8f7ce87b1dd71929dc95fc15f594e0e1465ea04",
 "source_count_arg":2,
 "source_width_arg":4,
 "source_native_type":TYPE4,
}
# 621B original overlay constructor allocates (count + 1) * 60 descriptor
# bytes, then uses a distinct loop and original callback paths. It is
# NOT equivalent to 0x24126E90 or a simple flat class-attribute array.
OVERLAY={
 "source_aux_table":"DT_OverlayVars",
 "original_property_name":"m_AnimOverlay",
 "source_descriptor_va":0x24337424,
 "source_callsite_va":0x240C36B6,
 "source_code_bytes":48,
 "source_code_sha256":"fb22001d97755db34f3115158e6fc0b215ab7b002f6cca779223752e6c63657f",
 "source_requested_overlay_count":15,
 "source_allocated_descriptor_count":16,
 "source_component_byte_stride":32,
 "source_outer_parent_x86_offset":0x7A4,
 "source_native_type":TYPE5,
}

SOURCE_FUNCTIONS=(
 (NESTED_ARRAY_BUILDER,351,
  "c6c8d8af7d77994a6ff4292d9a432172bad6e9a3d72d29b6ec8284c3a3eac0db"),
 (TYPE4_ARRAY_BUILDER,102,
  "dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd"),
 (OVERLAY_BUILDER,621,
  "a41703fce2d6cd865d2457b21738fa927ee63cda2ca34b473188d213528df4c6"),
 # Native overlay proxy dispatch routines (not yet emulated).
 (0x24127150,44,
  "d242eefc3be987bc2737d990d2570fcb1b8baee658423c52d5e9cb50d19863f5"),
 (0x24127180,46,
  "10b210d9c6cec8dbd257b131df49f1c0631a9fbf50af8e4f439f536d8fbb948e"),
)


def rel_call(pe:PEImage,site:int,target:int)->None:
    raw=pe.at(site,5)
    need(raw[0]==0xE8 and site+5+struct.unpack("<i",raw[1:])[0]==target,
         "source original auxiliary CALL target drift "+hex(site))


def verify_source_shapes(pe:PEImage,aux:dict)->list[dict]:
    by_pair={(t["table_name"],p["name"]):p
             for t in aux["source_auxiliary_20_tables"]
             for p in t["properties"]}
    rows=[]
    for table,name,dest,codeva,size,sha,count,stride,outer,childtype in NESTED_ARRAYS:
        need(hashlib.sha256(pe.at(codeva,size)).hexdigest()==sha,
             "original nested aux caller SHA drift "+table+"."+name)
        p=by_pair[(table,name)]
        need(int(p["source_property_descriptor_va"],16)==dest and
             int(p["source_callsite_va"],16)==codeva+size-12 and
             p["native_type"]==TYPE5 and
             p["native_constructor_route"]=="nested-array",
             "original auxiliary source type5 property changed "+table+"."+name)
        caller=codeva+size-12  # PUSH output; REP MOVSD; CALL
        need(pe.at(caller,5)==b"\x68"+struct.pack("<I",dest),
             "original nested aux output descriptor changed")
        need(pe.at(caller+5,2)==b"\xF3\xA5",
             "original nested aux descriptor 60B copy missing")
        rel_call(pe,caller+7,NESTED_ARRAY_BUILDER)
        # Exact source PUSH count / stride / parent-object base is
        # authenticated by whole 40/43-byte caller SHA above; verify
        # same operands independently, with 8-bit or DWORD base.
        operand=codeva+10
        if table=="DT_Local":
            # 6A20 6A01 6A04 at VA 0x240D3334.
            need(pe.at(operand,6)==bytes((0x6A,count,0x6A,stride,
                                           0x6A,outer)),
                 "original 32-byte area-bits array args changed")
        else:
            need(pe.at(operand,9)==
                 b"\x6A"+bytes((count,0x6A,stride,0x68))+
                 struct.pack("<I",outer),
                 "original aux array count/stride/outer-base changed")
        # Child element type comes from source integer factory, and the
        # generic array helper copies that source template per element.
        rows.append({
            "source_aux_table":table,
            "original_property_name":name,
            "source_descriptor_va":hex(dest),
            "source_callsite_va":hex(caller),
            "source_native_outer_type":TYPE5,
            "source_element_native_type":childtype,
            "source_element_count":count,
            "source_element_byte_stride":stride,
            "original_outer_parent_object_offset":outer,
            "original_child_recvprop_offsets":[i*stride for i in range(count)],
            "derived_parent_object_x86_offsets":[outer+i*stride
                                                  for i in range(count)],
            "original_indexed_element_names":[f"{i:03d}" for i in range(count)],
            "source_builder_va":hex(NESTED_ARRAY_BUILDER),
        })
    return rows


def verify_special(pe:PEImage,aux:dict)->dict:
    by_pair={(t["table_name"],p["name"]):p
             for t in aux["source_auxiliary_20_tables"]
             for p in t["properties"]}
    x=SPECIAL_ARRAY4
    p=by_pair[(x["source_aux_table"],x["original_property_name"])]
    codeva=x["source_callsite_va"]
    need(hashlib.sha256(pe.at(codeva,x["source_code_bytes"])).hexdigest()
         ==x["source_code_sha256"],"original viewmodel native type4 caller drift")
    need(int(p["source_property_descriptor_va"],16)==x["source_descriptor_va"]
         and p["native_type"]==TYPE4,
         "source 2-count viewmodel type4 property changed")
    need(pe.at(codeva,21)==
         b"\x6A\x00\x68"+struct.pack("<I",0x2428A43C)+
         b"\x6A\x04\x6A\x02\x68"+
         struct.pack("<I",x["source_descriptor_va"])+
         b"\xE8"+struct.pack("<i",TYPE4_ARRAY_BUILDER-(codeva+21)),
         "source viewmodel count2 stride4 native factory arguments changed")
    y=OVERLAY
    p=by_pair[(y["source_aux_table"],y["original_property_name"])]
    codeva=y["source_callsite_va"]
    need(hashlib.sha256(pe.at(codeva,y["source_code_bytes"])).hexdigest()
         ==y["source_code_sha256"],"original overlay source call SHA drift")
    need(int(p["source_property_descriptor_va"],16)==y["source_descriptor_va"]
         and p["native_type"]==TYPE5,
         "source overlay native type5 property changed")
    need(pe.at(0x240C36BB,2)==b"\x6A\x0F" and
         pe.at(0x240C36C7,2)==b"\x6A\x20" and
         pe.at(0x240C36C9,5)==b"\x68"+struct.pack("<I",0x7A4),
         "source overlay requested count15 / 32B stride / 1956 offset drift")
    rel_call(pe,0x240C36E1,OVERLAY_BUILDER)
    # Original 621B overlay helper explicitly allocates one MORE
    # descriptor than the input count, and later installs x86 proxy
    # callbacks 0x24127150 and 0x24127180. These are native callbacks
    # not yet replaced in the independent DLL.
    need(pe.at(0x241271D1,3)==b"\x8D\x77\x01" and
         pe.at(0x241271D4,11)==
         b"\x8B\xCE\xC1\xE1\x04\x2B\xCE\x03\xC9\x03\xC9",
         "source overlay count+1 allocation arithmetic drift")
    need(pe.at(0x24127394,7)==b"\xC7\x40\x10"+struct.pack("<I",0x24127180)
         and pe.at(0x2412739D,7)==
         b"\xC7\x40\x0C"+struct.pack("<I",0x24127150),
         "source original overlay specialized proxy callback pointers changed")
    # Both original source overlay DataTableRecvVarProxy paths have a
    # fully authenticated, common index*stride + parent DWORD formula.
    # A pointer to their allocated 28-byte helper sits at property+0x14;
    # the property array index is at +0x30; helper stride is +0x10,
    # and helper container-field offset is +0x14. These methods differ
    # in which original callback function pointer they invoke.
    for site,code,why in (
        (0x24127156,b"\x8B\x41\x14","proxy+4 metadata at RecvProp+20"),
        (0x24127159,b"\x8B\x49\x30","proxy+4 property index +48"),
        (0x2412715C,b"\x0F\xAF\x48\x10","proxy+4 index times stride"),
        (0x24127161,b"\x8B\x70\x14","proxy+4 dynamic base field"),
        (0x24127169,b"\x03\x0C\x3E","proxy+4 add parent dynamic base"),
        (0x24127173,b"\xFF\x50\x04","proxy+4 native callback dispatch"),
        (0x24127188,b"\x8B\x41\x14","proxy+0 metadata at RecvProp+20"),
        (0x2412718C,b"\x8B\x70\x14","proxy+0 dynamic base field"),
        (0x24127195,b"\x8B\x51\x30","proxy+0 property index +48"),
        (0x24127198,b"\x0F\xAF\x50\x10","proxy+0 index times stride"),
        (0x2412719C,b"\x03\x14\x3E","proxy+0 add parent dynamic base"),
        (0x241271A6,b"\xFF\x10","proxy+0 native callback dispatch"),
    ):
        need(pe.at(site,len(code))==code,
             "original overlay "+why+" opcode changed")
    return {
        "original_type4_array_descriptor":{
            "source_table":x["source_aux_table"],
            "name":x["original_property_name"],
            "source_count_arg":x["source_count_arg"],
            "source_width_arg":x["source_width_arg"],
            "source_native_type":TYPE4,
            "source_builder_va":hex(TYPE4_ARRAY_BUILDER),
            "limitations":"No nested RecvTable is created by type-four factory.",
        },
        "original_overlay_array":{
            "source_table":y["source_aux_table"],
            "name":y["original_property_name"],
            "original_requested_count":15,
            "original_allocated_descriptor_count":16,
            "original_descriptor_bytes":16*60,
            "original_component_byte_stride":32,
            "original_outer_parent_x86_offset":0x7A4,
            "native_outer_type":TYPE5,
            "original_overlay_constructor_va":hex(OVERLAY_BUILDER),
            "unimplemented_native_proxy_callbacks":[
                hex(0x24127150),hex(0x24127180)],
            "source_proxy_arithmetic":
                "computed = *(pStruct + helper+0x14) + "
                "RecvProp[+0x30] * helper[+0x10]",
            "original_native_callback_pointer_offsets":["helper+0","helper+4"],
            "limitations":"Special animation overlay data and proxy semantics "
                          "are not equivalent to the generic 0x24126E90 arrays.",
        }
    }


def verify(client:bytes, *,full_inventory:bool=False)->dict:
    need(hashlib.sha256(client).hexdigest()==ORIGINALS["client"]["sha256"],
         "owner original complete client.dll SHA256 mismatch")
    pe=PEImage(client)
    aux=verify_aux(client,full_inventory=True)
    need(aux["auxiliary_original_prop_slots"]==171,
         "phase1698 auxiliary source type inventory changed")
    for va,n,sha in SOURCE_FUNCTIONS:
        need(pe.section_of(va)==".text" and
             hashlib.sha256(pe.at(va,n)).hexdigest()==sha and
             pe.at(va+n,1)==b"\xCC",
             "whole original auxiliary native factory/proxy code boundary drift "+
             hex(va))
    arrays=verify_source_shapes(pe,aux)
    need(len(arrays)==3 and
         sum(a["source_element_count"] for a in arrays)==112,
         "three source generic auxiliary arrays do not total 112 elements")
    extra=verify_special(pe,aux)
    result={
       "status":"ORIGINAL_AUX_DYNAMIC_3_NESTED_ARRAYS_112_ELEMENTS_AND_OVERLAY_AUTHENTICATED",
       "original_client_sha256":ORIGINALS["client"]["sha256"],
       "generic_dynamic_aux_nested_tables":3,
       "generic_dynamic_aux_inner_recvprops":112,
       "special_aux_type4_array_descriptor":1,
       "special_overlay_requested_count":15,
       "special_overlay_allocated_descriptor_count":16,
       "source_whole_original_functions_authenticated":len(SOURCE_FUNCTIONS),
       "limits":("The 3 generic arrays' original child descriptor offsets "
                 "are relative to the nested table, not parent object. "
                 "Overlay proxy code/descriptor semantics are identified "
                 "but NOT independently executed. No 2006 native retail "
                 "ClientClass/RecvTable callback ABI or game boot."),
       **extra
    }
    if full_inventory:result["source_generic_auxiliary_dynamic_arrays"]=arrays
    return result


def self_test()->None:
    need(len(NESTED_ARRAYS)==3 and
         sum(a[6] for a in NESTED_ARRAYS)==112,
         "source three-aux-array count distribution drift")
    need(OVERLAY["source_allocated_descriptor_count"] ==
         OVERLAY["source_requested_overlay_count"]+1,
         "source specialized overlay count+1 not modeled")
    need(SPECIAL_ARRAY4["source_native_type"]==4,
         "type4 viewmodel incorrectly modeled as nested type5")
    for t in NESTED_ARRAYS:
        local=[i*t[7] for i in range(t[6])]
        effective=[t[8]+x for x in local]
        need(local[0]==0 and effective[0]==t[8],
             "source child-native offset must start at zero")
    try:
        verify(b"fabricated original auxiliary game DLL")
    except VerificationError:pass
    else:raise AssertionError("fabricated game DLL SHA was accepted")
    print("PASS Phase1699 source auxiliary arrays: 48+32+32=112 relative child elements, special type4 length2, animation overlay count15+1, full-original SHA refusal")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="read-only owner-private unchanged 2006 client.dll")
    parser.add_argument("--full-json",type=Path,
                        help="owner-private source metadata output only")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.client is None and not args.self_test:
        parser.error("supply --self-test or --client")
    if args.full_json is not None and args.client is None:
        parser.error("--full-json requires owner-private original")
    if args.self_test:self_test()
    if args.client is not None:
        report=verify(args.client.read_bytes(),full_inventory=args.full_json is not None)
        if args.full_json:
            args.full_json.write_text(
                json.dumps(report,indent=2,sort_keys=True)+"\n",encoding="utf-8")
            print("PASS original three nested aux arrays (112 elements), type4 viewmodel, specialized overlay evidence exported")
        else:
            print(json.dumps(report,indent=2,sort_keys=True))

if __name__=="__main__":
    main()
