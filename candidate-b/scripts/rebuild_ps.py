"""Fresh Vitis 2025.2 standalone platform/app build using the rebuilt Candidate-B handoff XSA.

Run: vitis -s scripts/rebuild_ps.py
Outputs only under a fresh short directory in this release (for Windows GCC).
"""
from pathlib import Path
import os
import shutil
import uuid
import vitis

root=Path(__file__).resolve().parent.parent
workspace=root/('b'+uuid.uuid4().hex[:4])
print('F21C_BUILD_WORKSPACE',workspace,flush=True)
if workspace.exists():
    raise FileExistsError(f'Refusing to reuse build workspace: {workspace}')
xsa=Path(os.environ.get('F21C_XSA', str(root/'build/FINAL_RUNTIME.xsa'))).resolve()
if not xsa.is_file():
    raise FileNotFoundError(f'Build the Candidate-B hardware handoff first: {xsa}')
source=root/'snapshots/ps_workspace/F21C_FINAL_RUNTIME_APP/src'
client=vitis.create_client()
try:
    client.set_workspace(str(workspace))
    platform=client.create_platform_component(name='p',hw_design=str(xsa),os='standalone',cpu='ps7_cortexa9_0',domain_name='standalone_ps7_cortexa9_0',no_boot_bsp=True)
    result=platform.build()
    print('F21C_PLATFORM_BUILD_RESULT',result,flush=True)
    if result not in (0,None):raise RuntimeError(result)
    xpfm=workspace/'p/export/p/p.xpfm'
    app=client.create_app_component(name='F21C_FINAL_RUNTIME_APP',platform=str(xpfm),domain='standalone_ps7_cortexa9_0',template='empty_application')
    destination=workspace/'F21C_FINAL_RUNTIME_APP/src'
    for p in source.rglob('*'):
        if p.is_file() and p.name not in ('.clangd','.compile_commands','compile_commands.json'):
            dst=destination/p.relative_to(source);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dst)
    # Preserve frozen lscript.ld; never regenerate it implicitly in this script.
    result=app.build()
    print('F21C_PS_BUILD_RESULT',result,flush=True)
    if result not in (0,None):raise RuntimeError(result)
    print('F21C_RELEASE_PS_REBUILD_PASS',flush=True)
finally:
    vitis.dispose()
