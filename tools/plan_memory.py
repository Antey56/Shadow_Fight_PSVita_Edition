#!/usr/bin/env python3
"""Plan relative ELF module slots. Does NOT reserve Vita virtual memory."""
import argparse,json,pathlib
LIMIT=1<<32
def align_up(n,a):return (n+a-1)//a*a
def plan(libraries,budget,page=4096,guard_pages=1):
    if page<=0 or page&(page-1):raise ValueError('page must be a power of two')
    if budget<=0 or budget>LIMIT or guard_pages<0:raise ValueError('invalid arena budget or guards')
    cursor=0;modules=[];errors=[];seen=set()
    for lib in libraries:
        name=lib['path']
        if name in seen:raise ValueError('duplicate module path')
        seen.add(name)
        if lib['bits']!=32 or lib['machine']!=40:
            errors.append(name+': not ARM32');continue
        segs=lib['load_segments']
        live=[];alignment=page
        for s in segs:
            sa=s.get('align',page)
            if sa>1 and sa&(sa-1):raise ValueError(name+': invalid segment alignment')
            alignment=max(alignment,sa)
            v,m=s['vaddr'],s['memsz']
            if v<0 or m<0 or v+m>LIMIT:raise ValueError(name+': invalid 32-bit segment range')
            if m:live.append((v,v+m))
        live.sort()
        if not live:errors.append(name+': no nonempty LOAD');continue
        for left,right in zip(live,live[1:]):
            if right[0]<left[1]:errors.append(name+': overlapping LOAD memory ranges')
        low=live[0][0]//alignment*alignment;high=align_up(max(b for _,b in live),page)
        start=align_up(cursor,alignment);size=high-low
        modules.append({'path':name,'arena_offset':start,'required_alignment':alignment,'size':size,'elf_vaddr_min_page':low,'elf_vaddr_end_page':high,'segments':[{'arena_offset':start+a-low,'size':b-a} for a,b in live]})
        cursor=start+size+guard_pages*page
    required=cursor if modules else 0
    if required>budget:errors.append('module slots and guards exceed requested arena budget')
    return {'schema_version':1,'page_size':page,'guard_pages_between_and_after_modules':guard_pages,'budget':budget,'required':required,'remaining':budget-required,'layout_valid':not errors,'execution_allowed':False,'modules':modules,'errors':errors,'limitations':['Relative offsets only; Vita address allocation and collisions with host/GPU mappings are NOT checked.','Budget is user-selected, not detected available memory. Heap, stacks, graphics and Android/Unity services are excluded.','Guard slots are planning gaps, not installed memory protections.','Page-rounded segment ranges may share a boundary page; exact overlapping LOAD memory ranges are rejected.']}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=pathlib.Path);p.add_argument('--budget-mib',type=int,required=True);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    r=json.loads(a.report.read_text(encoding='utf-8'));result=plan(r['libraries'],a.budget_mib*1024**2)
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('Relative layout valid:',result['layout_valid'],'required:',result['required'],'budget:',result['budget'],'execution allowed: false')
    raise SystemExit(0 if result['layout_valid'] else 1)
