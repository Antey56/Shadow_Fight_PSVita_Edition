#!/usr/bin/env python3
"""Single static startup gate for the exact local SF2 Unity/IL2CPP pair.
Reports actionable blockers; never maps or executes Android instructions."""
import argparse,collections,hashlib,json,pathlib,struct
from elf_dynamic import DynamicELF
from audit_unity_bootstrap import UNITY_SHA,audit
HASHES={'libmain.so':'dad352d6cc2a8cf184d4e0a9dfe6d7f3768c329b4c684587c28e48eb01511d71',
        'libunity.so':UNITY_SHA,'libil2cpp.so':'4f416f6992684f4a003f6baab7e5b33969f7cbff7f289b48a11e2fbe494f79b8'}
def referenced_imports(elf):
    symbols=elf.report()['referenced_symbols']
    return {label:sorted({s['name'] for s in symbols if not s['section_index'] and s['binding']==binding})
            for label,binding in [('required',1),('weak',2)]}
def category(name):
    if name.startswith(('pthread_','__pthread_','sem_','sched_')):return 'threads_and_synchronization'
    if name.startswith(('egl','ANativeWindow_')):return 'egl_surface_and_graphics'
    if name.startswith(('ALooper_','ASensor')):return 'android_looper_and_sensors'
    if name.startswith(('__aeabi_','__cxa_','__gnu_','__stack_')):return 'compiler_and_cpp_runtime'
    if name in ('clock_gettime','gettimeofday','time','nanosleep','usleep','clock'):return 'time'
    if name.startswith(('__android_log_','__system_property_')):return 'android_system'
    return 'remaining_libc_os_and_io'
def analyze(directory,declarations):
    modules=[];all_required=set();unity=None
    for name,expected in HASHES.items():
        data=(directory/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=expected:raise ValueError('unsupported module hash: '+name)
        elf=DynamicELF(data);imports=referenced_imports(elf);all_required.update(imports['required'])
        span=max(p[2]+p[5] for p in elf.loads)-min(p[2] for p in elf.loads)
        modules.append(dict(name=name,sha256=expected,span=span,file_bytes=len(data),relocation_count=len(elf.relocations()),
                            data_link_transient_bytes=len(data)+span+40*len(elf.relocations()),rounded_span=(span+4095)&~4095,
                            required_relocation_imports=imports['required'],weak_relocation_imports=imports['weak'],
                            needed=[elf.string(i) for i in elf.tags.get(1,[])],
                            tls_segments=sum(p[0]==7 for p in elf.ph),
                            writable_executable_segments=sum(p[0]==1 and p[6]&3==3 for p in elf.ph)))
        if name=='libunity.so':unity=(data,elf)
    data,elf=unity;profile=audit(data,declarations)
    for address,expected in ((0x65f9e2,0x2001),(0x65f9f2,0x2007)):
        if struct.unpack_from('<H',data,elf.file_offset(address,2))[0]!=expected:
            raise ValueError('timing clock ID instruction mismatch')
    groups=collections.defaultdict(list)
    for name in sorted(all_required):groups[category(name)].append(name)
    il2cpp=next(x for x in modules if x['name']=='libil2cpp.so')
    free_user=225443840;arena=64*1024*1024;vgl_user_pool=203423744;heap=32*1024*1024
    return dict(execution_allowed=False,game_feasibility_proven=False,modules=modules,
                import_scope='Undefined global symbols referenced by REL/JMPREL only; required lists are not certified implementations.',
                selected_module_unique_required=len(all_required),required_by_subsystem=dict(groups),
                direct_onload_helpers=profile['jni_onload_direct_calls'],registration_groups=len(profile['confirmed_registration_groups']),
                registration_methods=profile['confirmed_method_count'],timing=dict(helper=0x19fd98,clock_ids=[1,7],
                requirement='Separate verified MONOTONIC (1) and BOOTTIME (7) backend semantics; return values are not checked in reviewed call sites.'),
                memory=dict(native_rounded_spans=sum(x['rounded_span'] for x in modules),proposed_arena=arena,
                observed_pre_graphics_free_user=free_user,observed_vitagl_user_pool=vgl_user_pool,
                current_heap=heap,il2cpp_staging_span=il2cpp['span'],
                il2cpp_span_exceeds_current_heap=il2cpp['span']>heap,
                arena_plus_observed_graphics_pool_fits_free_user=arena+vgl_user_pool<=free_user,
                excess_user_bytes=max(0,arena+vgl_user_pool-free_user),
                candidate_heap=128*1024*1024,candidate_graphics_user_cap=32*1024*1024,
                candidate_headroom_from_observed=free_user+heap-128*1024*1024-arena-32*1024*1024,
                note='Observed diagnostic measurements and proposed capacity only, not actual engine peak memory. Transient estimate includes input + staged image + 40 bytes per relocation, excludes allocator/runtime overhead.'),
                blockers=['Current heap is smaller than a single staged IL2CPP image.',
                          '64 MiB arena plus current vitaGL user pool exceeds measured pre-graphics free USER memory.',
                          'Installed kubridge executable-memory path has not been device-validated.',
                          'Required Android ABI services are incomplete; clock 7 suspend behavior is unverified.',
                          'Original Unity/IL2CPP constructors and JNI_OnLoad remain unexecuted.',
                          'Unity EGL/surface/audio/Java lifecycle and actual game startup remain unimplemented.'])
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=pathlib.Path)
    p.add_argument('--audit',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
    a=p.parse_args();r=analyze(a.directory,json.loads(a.audit.read_text(encoding='utf-8'))['native_methods'])
    a.out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print('BLOCKED:',r['selected_module_unique_required'],'unique required relocation imports;',r['registration_methods'],'native registrations;',len(r['blockers']),'blocker classes')
