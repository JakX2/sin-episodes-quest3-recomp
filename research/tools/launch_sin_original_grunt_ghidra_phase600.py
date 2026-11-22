#!/usr/bin/env python3
"""Phase600: run the SHA-pinned original SiN 2006 Grunt Ghidra metadata pass.

The original server/client DLLs remain local. Requires Ghidra and Java/JDK.
Ghidra imports the executable into a temporary project, and the Phase599
read-only script exports metadata (addresses/hashes), never executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
GHIDRA_SCRIPT='GruntBehaviorDispatchPhase599.py'
SOURCE_REPORT='grunt_behavior_phase598_targets.json'
OUTPUT='grunt_behavior_phase599_ghidra.json'

def sha_file(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1<<20),b''):
            digest.update(block)
    return digest.hexdigest()

def headless_launcher(ghidra_home=None):
    exe='analyzeHeadless.bat' if os.name=='nt' else 'analyzeHeadless'
    locations=[]
    if ghidra_home:locations.append(Path(ghidra_home))
    if os.getenv('GHIDRA_HOME'):locations.append(Path(os.getenv('GHIDRA_HOME')))
    locations.append(Path(__file__).resolve().parent)
    for var in ('ProgramFiles','LOCALAPPDATA','USERPROFILE'):
        if not os.getenv(var):continue
        parent=Path(os.getenv(var))
        if parent.is_dir():
            locations.extend(sorted(parent.glob('ghidra*')))
            if var=='USERPROFILE' and (parent/'Desktop').is_dir():
                locations.extend(sorted((parent/'Desktop').glob('ghidra*')))
    for location in locations:
        if location.is_file() and location.name.lower()==exe.lower():
            return location.resolve()
        candidate=location/'support'/exe
        if candidate.is_file():return candidate.resolve()
    return None

def authenticate(server,client,report,script):
    for path in (server,client,report,script):
        if not path.is_file():raise ValueError('Missing required local input: '+str(path))
    if sha_file(server)!=SERVER_SHA:raise ValueError('Wrong/modified 2006 retail server.dll')
    if sha_file(client)!=CLIENT_SHA:raise ValueError('Wrong/modified 2006 retail client.dll')
    data=json.loads(report.read_text(encoding='utf-8'))
    if not (data.get('status')=='PHASE598_ORIGINAL_EIGHT_BEHAVIOR_VTABLE_TARGETS'
            and data.get('source_server_sha256')==SERVER_SHA
            and data.get('source_client_sha256_authenticated')==CLIENT_SHA
            and data.get('original_vtable_rtti_count_verified')==8
            and data.get('original_virtual_entries_at_known_vtables')==16
            and len(data.get('behaviors',[]))==8):
        raise ValueError('Original Phase598 report mismatch')
    return data

def make_command(launcher,project_dir,server,report,script,output,timeout):
    return [str(launcher),str(project_dir),'SiN_Phase600_Temporary',
            '-import',str(server),'-overwrite','-deleteProject',
            '-analysisTimeoutPerFile',str(timeout),
            '-scriptPath',str(script.parent),
            '-postScript',script.name,str(report),str(server),str(output)]

def validate_output(output):
    if not output.is_file():raise ValueError('Ghidra report missing')
    data=json.loads(output.read_text(encoding='utf-8'))
    if not (data.get('status')=='PHASE599_GHIDRA_ESTIMATED_NATIVE_FUNCTION_BOUNDS'
            and data.get('source_server_sha256')==SERVER_SHA
            and data.get('original_pe_verified_against_disk') is True
            and data.get('source_phase598_vtable_pointers_matched_loaded_program')==16
            and data.get('unique_original_vtable_targets')==13
            and len(data.get('functions',[]))==13
            and data.get('exported_proprietary_machine_code') is False):
        raise ValueError('Incomplete or mismatched Ghidra metadata')
    return data

def self_test():
    from copy import deepcopy
    with tempfile.TemporaryDirectory() as root:
        root=Path(root)
        src=root/'original retail'/'server.dll'
        inp=root/'phase598 report.json'
        script=root/'ghidra scripts'/GHIDRA_SCRIPT
        out=root/OUTPUT
        args=make_command(root/'support'/'analyzeHeadless.bat',root/'project',
                          src,inp,script,out,1800)
        assert args[3:5]==['-import',str(src)]
        assert args[-5:]==['-postScript',GHIDRA_SCRIPT,str(inp),str(src),str(out)]
        data={'status':'PHASE599_GHIDRA_ESTIMATED_NATIVE_FUNCTION_BOUNDS',
              'source_server_sha256':SERVER_SHA,'original_pe_verified_against_disk':True,
              'source_phase598_vtable_pointers_matched_loaded_program':16,
              'unique_original_vtable_targets':13,'functions':[{}]*13,
              'exported_proprietary_machine_code':False}
        out.write_text(json.dumps(data),encoding='utf-8')
        assert len(validate_output(out)['functions'])==13
        bad=deepcopy(data);bad['source_phase598_vtable_pointers_matched_loaded_program']=15
        out.write_text(json.dumps(bad),encoding='utf-8')
        try:validate_output(out)
        except ValueError:pass
        else:raise AssertionError('Accepted corrupt Ghidra report')
    print('PHASE600_GHIDRA_HEADLESS_LAUNCHER_SYNTHETIC_PASS')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bin-dir',type=Path,default=Path(__file__).resolve().parent)
    ap.add_argument('--ghidra-home',type=Path)
    ap.add_argument('--server',type=Path)
    ap.add_argument('--client',type=Path)
    ap.add_argument('--phase598',type=Path)
    ap.add_argument('--ghidra-script',type=Path)
    ap.add_argument('--out',type=Path)
    ap.add_argument('--log',type=Path)
    ap.add_argument('--analysis-timeout',type=int,default=1800)
    ap.add_argument('--self-test',action='store_true')
    a=ap.parse_args()
    if a.self_test:return self_test()
    directory=a.bin_dir.resolve()
    server=(a.server or directory/'server.dll').resolve()
    client=(a.client or directory/'client.dll').resolve()
    report=(a.phase598 or directory/SOURCE_REPORT).resolve()
    script=(a.ghidra_script or Path(__file__).resolve().parent/GHIDRA_SCRIPT).resolve()
    output=(a.out or directory/OUTPUT).resolve()
    log=(a.log or directory/'grunt_behavior_phase600_headless.log').resolve()
    if not 60<=a.analysis_timeout<=14400:
        ap.error('Analysis timeout must be 60 to 14400 seconds')
    try:
        authenticate(server,client,report,script)
        launcher=headless_launcher(a.ghidra_home)
        if launcher is None:
            raise ValueError('Ghidra not found. Set GHIDRA_HOME or pass --ghidra-home "C:\\path\\to\\ghidra"')
        if output.exists():output.unlink()
        with tempfile.TemporaryDirectory(prefix='sin_grunt_original_') as project:
            cmd=make_command(launcher,Path(project),server,report,script,output,
                             a.analysis_timeout)
            print('Analyzing untouched original server.dll using',launcher,flush=True)
            print('Ghidra log:',log,flush=True)
            with log.open('w',encoding='utf-8',errors='replace') as handle:
                proc=subprocess.run(cmd,stdout=handle,stderr=subprocess.STDOUT,check=False)
            if proc.returncode!=0:
                raise RuntimeError('Ghidra returned %d; inspect log %s'%(proc.returncode,log))
        result=validate_output(output)
        from triage_sin_original_grunt_ghidra_phase600 import inspect
        triage=inspect(result)
        dest=output.with_name('grunt_behavior_phase600_triage.json')
        dest.write_text(json.dumps(triage,indent=2)+'\n',encoding='utf-8')
        print('Ghidra recognized %d/13 native entry addresses'%result.get('ghidra_function_starts',0))
        print('Upload only the detailed metadata:',output)
        return 0
    except (ValueError,RuntimeError,OSError,json.JSONDecodeError) as exc:
        print('PHASE600 REFUSED:',exc,file=sys.stderr)
        return 2

if __name__=='__main__':raise SystemExit(main())
