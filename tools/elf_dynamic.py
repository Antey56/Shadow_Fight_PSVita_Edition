#!/usr/bin/env python3
"""Bounded ARM32 PT_DYNAMIC parser; no section table or execution required."""
import argparse,collections,json,pathlib,struct
class DynamicELF:
    def __init__(self,b):
        self.b=b
        self.range(0,52)
        self.h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
        if b[:7]!=b'\x7fELF\x01\x01\x01' or self.h[1]!=3 or self.h[2]!=40:raise ValueError('requires ET_DYN ARM32 LE ELF')
        if self.h[9]!=32 or not 0<self.h[10]<=128:raise ValueError('invalid program table')
        self.range(self.h[5],32*self.h[10])
        self.ph=[struct.unpack_from('<IIIIIIII',b,self.h[5]+32*i) for i in range(self.h[10])]
        self.loads=[p for p in self.ph if p[0]==1 and p[5]]
        if not self.loads:raise ValueError('no LOAD')
        for p in self.loads:
            self.range(p[1],p[4])
            if p[4]>p[5] or p[2]+p[5]>2**32:raise ValueError('invalid LOAD size')
        ranges=sorted((p[2],p[2]+p[5]) for p in self.loads)
        if any(b[0]<a[1] for a,b in zip(ranges,ranges[1:])):raise ValueError('overlapping LOAD ranges')
        dyn=[p for p in self.ph if p[0]==2]
        if len(dyn)!=1:raise ValueError('requires exactly one DYNAMIC segment')
        d=dyn[0];self.range(d[1],d[4])
        if d[4]%8:raise ValueError('invalid DYNAMIC size')
        if self.file_offset(d[2],d[4])!=d[1]:raise ValueError('DYNAMIC offset/vaddr mismatch')
        self.tags=collections.defaultdict(list);terminated=False
        for off in range(d[1],d[1]+d[4],8):
            tag,val=struct.unpack_from('<II',b,off)
            if tag==0:terminated=True;break
            self.tags[tag].append(val)
        if not terminated:raise ValueError('missing DT_NULL')
        self.strtab=self.single(5);self.strsize=self.single(10)
        self.strfile=self.file_offset(self.strtab,self.strsize)
        self.symtab=self.single(6)
        if self.single(11)!=16:raise ValueError('unsupported symbol stride')
    def range(self,offset,size):
        if offset<0 or size<0 or offset>len(self.b) or size>len(self.b)-offset:raise ValueError('out-of-file data')
    def file_offset(self,va,size):
        found=[p[1]+va-p[2] for p in self.loads if p[2]<=va and va+size<=p[2]+p[4]]
        if len(found)!=1:raise ValueError('virtual range not uniquely file-backed')
        return found[0]
    def single(self,tag,default=None):
        values=self.tags.get(tag,[])
        if len(values)>1:raise ValueError('duplicate singleton dynamic tag '+hex(tag))
        if not values:
            if default is not None:return default
            raise ValueError('missing dynamic tag '+hex(tag))
        return values[0]
    def string(self,index):
        if index>=self.strsize:raise ValueError('string offset out of bounds')
        start=self.strfile+index;end=self.b.find(b'\0',start,self.strfile+self.strsize)
        if end<0:raise ValueError('unterminated string')
        return self.b[start:end].decode('utf-8','strict')
    def symbol(self,index):
        off=self.file_offset(self.symtab+16*index,16)
        name,value,size,info,other,shndx=struct.unpack_from('<IIIBBH',self.b,off)
        return {'index':index,'name':self.string(name),'value':value,'size':size,'binding':info>>4,'type':info&15,'visibility':other&3,'section_index':shndx}
    def relocations(self):
        # Refuse silently incomplete extraction when unsupported tables exist.
        forbidden={7:'RELA',8:'RELASZ',35:'RELRSZ',36:'RELR',37:'RELRENT',0x6000000f:'ANDROID_REL',0x60000010:'ANDROID_RELSZ',0x60000011:'ANDROID_RELA',0x60000012:'ANDROID_RELASZ',0x6fffe000:'ANDROID_RELR',0x6fffe001:'ANDROID_RELRSZ'}
        for tag,label in forbidden.items():
            if tag in self.tags:raise ValueError('unsupported relocation encoding '+label)
        tables=[]
        if 17 in self.tags or 18 in self.tags:
            va=self.single(17);size=self.single(18)
            if self.single(19)!=8 or size%8:raise ValueError('invalid REL stride/size')
            if size:tables.append(('REL',va,size))
        if 23 in self.tags or 2 in self.tags:
            va=self.single(23);size=self.single(2)
            if self.single(20)!=17 or size%8:raise ValueError('PLT relocations must be REL')
            if size:tables.append(('JMPREL',va,size))
        if len(tables)==2:
            _,a,n=tables[0];_,b,m=tables[1]
            if max(a,b)<min(a+n,b+m):raise ValueError('overlapping REL tables')
        result=[];places=set()
        for table,va,size in tables:
            off=self.file_offset(va,size)
            for p in range(off,off+size,8):
                target,info=struct.unpack_from('<II',self.b,p);typ=info&255;idx=info>>8
                if typ and (target%4 or not any(x[2]<=target and target+4<=x[2]+x[5] for x in self.loads)):raise ValueError('invalid relocation target')
                if typ and target in places:raise ValueError('duplicate relocation target')
                if typ:places.add(target)
                result.append({'target_vaddr':target,'type':typ,'symbol_index':idx,'table':table})
        return result
    def report(self):
        rels=self.relocations();symbols=[self.symbol(i) for i in sorted({r['symbol_index'] for r in rels if r['symbol_index']})]
        return {'needed':[self.string(i) for i in self.tags.get(1,[])],'relocation_counts':dict(collections.Counter(str(r['type']) for r in rels)),'relocations':rels,'referenced_symbols':symbols,'flags':hex(self.h[7]),'section_headers_used':False,'execution_allowed':False,'limitations':['Referenced symbols only, not full export enumeration.','No symbol version resolution, lookup scope, IFUNC execution or mapping into Vita memory.']}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('module',type=pathlib.Path);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    report=DynamicELF(a.module.read_bytes()).report();a.out.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Dynamic relocations:',report['relocation_counts'],'referenced symbols:',len(report['referenced_symbols']))
