"""Static guardrails for the Quest Browser vertical slice; no headset implied."""
from pathlib import Path
import re
p=Path(__file__).resolve().parents[1]/'docs'/'quest3-prototype'/'index.html'
s=p.read_text()
assert '<script type="importmap">' in s
assert 'three@0.162.0' in s
assert 'renderer.xr.enabled=true' in s
assert "renderer.xr.setReferenceSpaceType('local-floor')" in s
assert "'sessionstart'" in s and "'sessionend'" in s
assert "'selectstart'" in s and "'squeezestart'" in s
assert 'getSession()' in s and '.gamepad' in s
assert 'function startGame' in s and 'function shoot' in s and 'function activateTerminal' in s
assert 'function reload' in s and 'function finish' in s
assert 'function updateMovement' in s
assert "state='win'" not in s or 'success' in s
assert 'dlopen' not in s and 'server.dll' not in s
print('PASS: WebXR source static contract')