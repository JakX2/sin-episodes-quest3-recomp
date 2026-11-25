#!/usr/bin/env python3
"""Authenticate original weapon activity table rows and primary virtual slots."""
import argparse, hashlib, pathlib, struct
SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
BASE = 0x22000000
TABLES = [
("assault",0x224776ac,0x2252d210,60,"36461a77e73a4239c97bd581e109f2218933d14ffd7ba96d9306b4f7188285f9"),
("chaingun",0x22477c8c,0x2252d7c8,46,"1c26fea276f4d4f81474cb7815ec751231204ab2ba95d125bfd93002d9ce1feb"),
("grenade",0x22478214,0x2252db40,1,"b4edfff831f3c527141be9c80290b8ccef842a9f60a343830dc606f103a33329"),
("magnum",0x2247875c,0x2252dca0,28,"6cab072d6e49f1f50ed3c7c11ec407e75ef306797bbb597af24859bd66fdadf7"),
("melee",0x22478d0c,0x2252de48,2,"bcae5485967af5e4eee2958da6d04024f476752f5d5ef95c94dd21c396c39d6a"),
("npcpistol",0x2247924c,0x2252df60,28,"6cab072d6e49f1f50ed3c7c11ec407e75ef306797bbb597af24859bd66fdadf7"),
("scatter",0x2247a1e4,0x2252e660,46,"8f68d853e4be0e53a88ffedd6e391af9664344b0cc57103a743f7c23bf280a68"),
("smg",0x2247ab9c,0x2252ea30,37,"77d671c0a864eb22a376bde64565ab6070beec6fe811faf9f7d5784df76c42ed")]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("server",type=pathlib.Path)
    p.add_argument("--cpp",type=pathlib.Path,help="compare authored native table arrays")
    a=p.parse_args();data=a.server.read_bytes()
    if hashlib.sha256(data).hexdigest()!=SHA: raise ValueError("unexpected owner server hash")
    def u32(address): return struct.unpack_from("<I",data,address-BASE)[0]
    def body(address,n): return data[address-BASE:address-BASE+n]
    source=a.cpp.read_text() if a.cpp else None
    import re
    for name,vt,addr,count,digest in TABLES:
        if u32(vt+0x47c)!=0x222ac480: raise ValueError(name+" translator")
        for slot,expected in [(0x480,addr),(0x484,count)]:
            code=body(u32(vt+slot),6)
            if code[0]!=0xb8 or code[-1]!=0xc3 or struct.unpack_from("<I",code,1)[0]!=expected: raise ValueError(name+" getter")
        raw=body(addr,count*12)
        if hashlib.sha256(raw).hexdigest()!=digest: raise ValueError(name+" table hash")
        rows=[struct.unpack_from("<iiB",raw,i*12) for i in range(count)]
        if source is not None:
            key="magnum" if name=="npcpistol" else name
            match=re.search(r"constexpr ServerWeaponActivityRow "+key+r"\[\]\{(.*?)\};",source,re.S)
            if not match: raise ValueError(key+" missing")
            parsed=[(int(x),int(y),int(z=="true")) for x,y,z in re.findall(r"\{(-?\d+),(-?\d+),(true|false)\}",match[1])]
            if parsed!=rows: raise ValueError(key+" authored rows mismatch")
        print(name,count,"PASS")
    print("PASS",sum(x[3] for x in TABLES),"original rows")
if __name__=="__main__": main()
