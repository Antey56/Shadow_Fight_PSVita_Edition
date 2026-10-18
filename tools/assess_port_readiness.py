#!/usr/bin/env python3
"""Evidence-based six-stage readiness report; never authorizes execution."""
import argparse
import collections
import json
import pathlib


def assess(gate,deep,bootstrap,bridge,hardware=None):
    diagnostic_verified = bool(hardware and hardware.get("build_id")=="0.7-gfxtrace1"
                               and all(hardware.get(k) is True for k in
                                       ("probes_passed","draw_returned","swap_returned","input_recorded","start_exit_recorded")))
    implemented=set(bridge['implemented_slots'])
    observed=set(bootstrap['observed_jni_slots'])
    missing=sorted(observed-implemented)
    modules=[{'module':m['name'],'required_unimplemented_count':len(m['missing_required_imports']),
              'platform_imports':[n for n in m['missing_required_imports']
                                  if n.startswith(('egl','ANativeWindow','pthread','slCreateEngine','AMedia'))],
              'external_libraries':m['external_dependencies']} for m in gate['modules']]
    dynamic=collections.Counter()
    for names in deep['dynamic_api_strings'].values():
        for n in names:
            for prefix in ('egl','gl','vk','AMedia','AAudio'):
                if n.startswith(prefix):dynamic[prefix]+=1;break
    core=[m for m in modules if m['module'] in ('libmain.so','libunity.so','libil2cpp.so')]
    return {'execution_allowed':False,'stages':[
        {'stage':1,'name':'JNI registration discovery','status':'partial_verified',
         'confirmed_static_methods':bootstrap['confirmed_method_count'],
         'note':'Three groups statically traced; remaining registrations/lifecycle not fully traced.'},
        {'stage':2,'name':'JNI runtime semantics','status':'blocked','observed_missing_slots':missing,
         'note':'Class initialization, nested object arrays and worker threads remain incomplete; leaf object arrays, primitive arrays and direct buffers are bounded harness implementations.'},
        {'stage':3,'name':'Android and bionic dependencies','status':'blocked',
         'unique_required_unimplemented':len(gate['unique_missing_required']),
         'note':'Candidate adapters are not device-certified ABI implementations.'},
        {'stage':4,'name':'Executable ELF loading and ARM ABI','status':'blocked',
         'note':'Executable mapper/VM smoke path exists for constructor-free authored ELF; hardware execution pending. Missing imports, TLS and constructor/runtime services still block game JNI_OnLoad.'},
        {'stage':5,'name':'Graphics audio lifecycle integration','status':'blocked',
         'note':'Diagnostic vitaGL output is not a Unity surface, EGL context lifecycle or engine audio backend.'},
        {'stage':6,'name':'Physical Vita validation','status':'diagnostic_verified' if diagnostic_verified else 'not_run',
         'engine_validated':False,
         'note':('0.7 diagnostic probes, draw/swap, input and normal START exit recorded on Vita; Unity not started.'
                 if diagnostic_verified else 'No complete diagnostic hardware evidence supplied; cross-build alone is not device execution.')}],
        'core_modules':core,'all_modules':modules,'dynamic_api_string_counts':dict(dynamic),
        'string_evidence_caveat':'Backend/API name presence does not prove runtime use; Vulkan and codec strings alone are not requirements.',
        'next_required_work':['Complete Java class initialization and worker-thread JNI semantics; registration is currently a bounded harness.',
                              'Implement and test each required bionic/Android adapter with correct ABI and ownership.',
                              'Validate executable memory/cache and complete symbol resolution before constructors.',
                              'Integrate EGL/audio/lifecycle and validate on hardware.'],
        'drm_bypassed':False,'game_executed':False}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('report_directory',type=pathlib.Path)
    p.add_argument('--out',type=pathlib.Path,required=True)
    p.add_argument('--hardware',type=pathlib.Path,help='Reviewed diagnostic evidence JSON; never enables engine execution')
    a=p.parse_args()
    def load(name):return json.loads((a.report_directory/name).read_text(encoding='utf-8'))
    r=assess(load('runtime-gate.json'),load('deep-audit.json'),load('unity-bootstrap.json'),load('jni-bridge-status.json'),
             json.loads(a.hardware.read_text(encoding='utf-8')) if a.hardware else None)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(r,indent=2),encoding='utf-8')
    print('Execution blocked; missing observed JNI slots:',r['stages'][1]['observed_missing_slots'],
          '; required unique runtime imports:',r['stages'][2]['unique_required_unimplemented'])
