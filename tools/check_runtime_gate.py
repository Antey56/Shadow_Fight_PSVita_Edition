#!/usr/bin/env python3
"""Dependency/import gate. Empty adapter map intentionally blocks execution."""
import argparse,json,pathlib
def check(report,adapters):
    libs=report['libraries'];names={pathlib.PurePosixPath(x['path']).name:x for x in libs}
    providers={}
    for name,lib in names.items():
        for symbol in lib['exports']:providers.setdefault(symbol,[]).append(name)
    result={'execution_allowed':False,'modules':[],'adapter_count':len(adapters),'limitations':['Export provider candidates are NOT a validated ELF lookup scope.','Adapter name alone does not certify ABI or implementation.','JNI, constructors, graphics and device tests remain mandatory.']}
    for name,lib in names.items():
        missing=[];weak=[];internal=[];host=[]
        for imp in lib['imports']:
            s=imp['name'];candidates=[p for p in providers.get(s,[]) if p!=name]
            if s in adapters:host.append(s)
            elif candidates:internal.append({'symbol':s,'candidate_providers':candidates})
            elif imp['weak']:weak.append(s)
            else:missing.append(s)
        result['modules'].append({'name':name,'armv7':lib['bits']==32 and lib['machine']==40,'packaged_dependencies':[d for d in lib['needed'] if d in names],'external_dependencies':[d for d in lib['needed'] if d not in names],'missing_required_imports':missing,'missing_weak_imports':weak,'internal_candidates':internal,'declared_host_adapters':host})
    result['missing_required_records']=sum(len(x['missing_required_imports']) for x in result['modules'])
    result['unique_missing_required']=sorted({s for x in result['modules'] for s in x['missing_required_imports']})
    return result
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=pathlib.Path);p.add_argument('--adapters',type=pathlib.Path);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    r=json.loads(a.report.read_text(encoding='utf-8'));adapters=json.loads(a.adapters.read_text(encoding='utf-8')) if a.adapters else {}
    result=check(r,adapters);a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('Execution blocked; missing required import records:',result['missing_required_records'],'unique:',len(result['unique_missing_required']))
    raise SystemExit(1)
